#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "transmitreceive.h"
#include "crc_compute.h"
#include "gpio.h"

uint8_t dummy_bytes[16] = { 0x56, 0x78, 0x87, 0x65, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

uint8_t find_command_direction(uint16_t msgid){

	/* 	Function Structure interpreted from rl_driver.c lines 412->483 
		Removed the need for multi-platforms 							*/
	
	if((msgid > 0x00) && (msgid < 0x80)){
		return 0x01; // HOST TO BSS
	}
	else if((msgid >= 0x200) && (msgid < 0x280)){
		return 0x05; // HOST TO MSS (Master)
	}
	else if((msgid >= 0x100) && (msgid < 0x180)){
		return 0x03; // HOST TO DSS
	}
	else{
		printf("Invalid Direction Selected! Is the MSGID correct? : %X\n", msgid);
		return 0x00; // INVALID DIRECTION!
	}
}


uint64_t calculate_crc(uint8_t *data_pointer, uint8_t message_length, uint8_t crc_bits){
	uint8_t* crc_data = malloc(message_length - 4 - crc_bits / 8);
	for(uint8_t bit_pair = 4; bit_pair < message_length - crc_bits / 8; bit_pair += 2){
		crc_data[bit_pair - 3] = data_pointer[bit_pair];
		crc_data[bit_pair - 4] = data_pointer[bit_pair + 1];
	}

	uint64_t crc = computeCRC(crc_data, message_length - 4 - crc_bits / 8, crc_bits);
	free(crc_data);
	return crc;
}

uint16_t calculate_checksum(uint8_t* message){
    uint16_t checksum = (uint16_t)(message[5] + message[7] + message[9] + message[11] + message[13]) ;

    checksum += (uint16_t)((message[4] + message[6] + message[8] + message[10] + message[12]) << 8);

    return ~checksum;
}

int16_t make_command_params(uint8_t* message, int16_t message_buffer_size, uint16_t MSGID, uint16_t SBLKID, uint16_t SBLKLEN, uint8_t* SBLKDATA, uint8_t params){
	uint8_t message_length = 18 + SBLKLEN; // message_length is in bytes.
	// minimum message length (16 bit CRC) - 4 + SBLKLEN
	if(message_length > message_buffer_size) return -1;

	// SYNC Bits
	message[0] = 0x12;
	message[1] = 0x34;
	message[2] = 0x43;
	message[3] = 0x21;

	// Message Header
	// Opcode
	uint16_t opcode = 0x0000;
	opcode = find_command_direction(MSGID); // Find the direction host to ...
	opcode += 0b00 << 4; // New command.
	opcode += MSGID << 6;
	//printf("%X\n",(uint8_t)(opcode));
	message[4] = (uint8_t)(opcode >> 8);
	message[5] = (uint8_t)(opcode);

	// Length
	message[6] = (uint8_t)((message_length-4 >> 8) & 0b00001111);
	message[7] = (uint8_t)(message_length-4);

	// Flags
	static uint8_t SEQNUM = 0x00;
	message[8] = (uint8_t)(SEQNUM << 4);
	message[8] = (uint8_t)(message[8] | 0x0000); // CRC Length and appended.
	uint8_t retry = params & 0x01;
	if(retry)  message[9] = 0b00000011 * retry; // Retry if params[0] = 1, ack required, protocol version 0.
	else{
		message[9] = 0x00;
		SEQNUM += 1 - retry;
	}

	// Remaining Chunks
	message[10] = 0x00;
	message[11] = 0x00;

	// NSBC - Currently only one sub block supported.
	message[12] = 0x00;
	message[13] = 0x01;

	// Checksum.
	uint16_t checksum = calculate_checksum(message);
	message[14] = (uint8_t)(checksum >> 8);
	message[15] = (uint8_t)(checksum);

	// SBLKID
	message[16] = (uint8_t)(SBLKID >> 8);
	message[17] = (uint8_t)(SBLKID);

	// SBLKLEN
	message[18] = (uint8_t)(SBLKLEN >> 8);
	message[19] = (uint8_t)(SBLKLEN);

	// SBLKDATA
	uint8_t data_increment;
	for(data_increment = 0; data_increment < SBLKLEN - 4; data_increment++){
		message[20 + data_increment] = SBLKDATA[data_increment];
	}

	// CRC
	uint64_t crc = calculate_crc(message, message_length, 16);
	message[20 + data_increment] = (uint8_t)(crc >> 8);
	message[21 + data_increment] = (uint8_t)(crc);

	return message_length;
}

int transmit_message(int spi_handle, uint8_t *message, uint8_t bytes_to_transmit){

	if(bytes_to_transmit % 2) return -1; // Return if not byte-equal.

	printf("Message being sent is: 0x ");
	for(int byteset = 0; byteset < bytes_to_transmit/2; byteset++){
		uint8_t tx_temp[2] = {message[2 * byteset],message[2 * byteset + 1]};
		printf("%02X%02X ", tx_temp[0], tx_temp[1]);
		
		struct spi_ioc_transfer spi_setup = {
			.tx_buf = (unsigned long)tx_temp,
			.rx_buf = (unsigned long)0,
			.len = 2,
			.speed_hz = SPI_SPEED,
			.bits_per_word = SPI_BITS_PER_WORD,
		};
		if (ioctl(spi_handle, SPI_IOC_MESSAGE(1), spi_setup) < 1) {
			perror("Failed to perform SPI transfer\n");
			close(spi_handle);
			return EXIT_FAILURE;
		}
		usleep(150);
	}
	
	printf("\n");
	return 0;
}

int receive_message(int spi_handle, uint8_t *rx_buffer, uint8_t max_bytes_to_read, uint8_t *rx_message_length){

	if(max_bytes_to_read < 16){
		printf("RX Buffer Not Large Enough!\n");
		return -1; // Return if not big enough buffer.
	}

	int timeout_error = TIMEOUT_BEFORE_SYNC;
	for(int timeout = 0; timeout < HOST_INT_TIMEOUT_US; timeout += 10){
		if(getHostInt()){
			// If Hostint is high, you can transmit the dummy bytes.
			if(transmit_message(spi_handle, dummy_bytes, 16)) return EXIT_FAILURE;
			usleep(500);
			timeout_error = 0x00;
			break;
		}
		usleep(10);
	}
	if(timeout_error){
		printf("Timed out waiting for HOSTINT to go High\n");
		return timeout_error;
	}

	timeout_error = TIMEOUT_AFTER_SYNC;
	for(int timeout = 0; timeout < HOST_INT_TIMEOUT_US; timeout += 10){
		if(!getHostInt()){
			// If Hostint is low, you can read the message.
			timeout_error = 0x00;
			break;
		}
		usleep(10);
	}
	if(timeout_error){
		printf("Timed out waiting for HOSTINT to go Low\n");
		return timeout_error;
	}

	// Read SYNC bytes, and Message Header.
	uint8_t byteset_triggered = 0;
	for(int byteset = 0; byteset < MAX_RECEIVE_MESSAGE_RETRIES + byteset_triggered; byteset++){
		uint8_t tx_temp[2] = {0xFF, 0xFF};
		uint8_t rx_temp[2];
		struct spi_ioc_transfer spi_setup = {
			.tx_buf = (unsigned long)tx_temp,
			.rx_buf = (unsigned long)rx_temp,
			.len = 2,
			.speed_hz = SPI_SPEED,
			.bits_per_word = SPI_BITS_PER_WORD,
		};
		if (ioctl(spi_handle, SPI_IOC_MESSAGE(1), spi_setup) < 1) {
			perror("Failed to perform SPI transfer\n");
			close(spi_handle);
			return EXIT_FAILURE;
		}

		usleep(150);

		if(byteset_triggered > 1){
			rx_buffer[2*byteset_triggered] = rx_temp[0];
			rx_buffer[2*byteset_triggered + 1] = rx_temp[1];
			byteset_triggered++;
		}
		else if(byteset_triggered == 1){
			if(rx_temp[0] == 0xAB && rx_temp[1] == 0xCD){
				rx_buffer[2] = rx_temp[0];
				rx_buffer[3] = rx_temp[1];
				byteset_triggered = 2;
			}
			else byteset_triggered = 0;
		}
		else if(rx_temp[0] == 0xDC && rx_temp[1] == 0xBA){
			rx_buffer[0] = rx_temp[0];
			rx_buffer[1] = rx_temp[1];
			byteset_triggered = 1;
		}

		if(byteset_triggered > 7) break;

		if(byteset == MAX_RECEIVE_MESSAGE_RETRIES + byteset_triggered){
			printf("SPI Recieve hit max retries\n");
				return -1;
		}
	}
	uint16_t message_length = (rx_buffer[6] << 8) + rx_buffer[7];
	printf("The message to recieve is 0x%X bytes long.\n", message_length);
	if(message_length + 4 > max_bytes_to_read){
		return -1; // Too large of a message to store in the buffer.
		printf("RX Buffer not large enough!\n");
	}

	uint16_t checksum_rxd = (rx_buffer[14] << 8) + rx_buffer[15];
	uint16_t checksum_calc = calculate_checksum(rx_buffer);
	printf("Checksum Received is 0x%X, Checksum Calculated is 0x%X ... ", checksum_rxd, checksum_calc);
	if(checksum_rxd == checksum_calc) printf("Checksum Calculated Matches!\n");
	else return CHECKSUM_NO_MATCH;

	for(int byteset = 0; byteset < (message_length - 12)/2; byteset++){
		uint8_t tx_temp[2] = {0xFF, 0xFF};
		uint8_t rx_temp[2];
		struct spi_ioc_transfer spi_setup = {
			.tx_buf = (unsigned long)tx_temp,
			.rx_buf = (unsigned long)rx_temp,
			.len = 2,
			.speed_hz = SPI_SPEED,
			.bits_per_word = SPI_BITS_PER_WORD,
		};
		if (ioctl(spi_handle, SPI_IOC_MESSAGE(1), spi_setup) < 1) {
			perror("Failed to perform SPI transfer\n");
			close(spi_handle);
			return EXIT_FAILURE;
		}

		rx_buffer[2*(byteset+8)] = rx_temp[0];
		rx_buffer[2*(byteset+8) + 1] = rx_temp[1];

		usleep(150);
	}

	printf("Received: ");
	for (int i = 0; i < message_length + 4; i++) {
		printf("%02X ", rx_buffer[i]);
	}
	printf("\n");

	// CRC Checking

	uint16_t flags = (rx_buffer[8] << 8) + rx_buffer[9];
	uint8_t crc_len = ( 0b00001100 & rx_buffer[8] ) >> 2;
	uint8_t crc_req = 0b00000011 & rx_buffer[8];

	if(!crc_req){
		switch(crc_len) {
			case 0b00: 
				uint16_t crc_rxd = (rx_buffer[message_length + 2] << 8) + rx_buffer[message_length + 3];
				uint16_t crc_expected = calculate_crc(rx_buffer, message_length + 4, 16);
				printf("16-Bit CRC Appended: 0x%X. CRC Expected: 0x%X ...", crc_rxd, crc_expected);
				if(crc_expected != crc_rxd){
					printf("CRCs do NOT match!\n");
					return CRC_NO_MATCH;
				}
				else printf("CRCs Match!\n");
				break;
			case 0b01:
				printf("32-Bit CRC, currently unsupported. Assumed correct.\n");
				break;
			case 0b10:
				printf("64-Bit CRC, currently unsupported. Assumed correct.\n");
				break;
			default:
				printf("Invalid CRC Length!\n");
				return INVALID_CRC_LENGTH;
				break;
		};
	}
	else printf("No CRC appended, lucky you!");

	*rx_message_length = message_length + 4;

	return 0;
}

int make_transmit_receive_message(int spi_handle, uint8_t* tx_buffer, uint8_t* rx_buffer, int16_t tx_rx_buffer_size, uint16_t MSGID, uint16_t SBLKID, uint16_t SBLKLEN, uint8_t* SBLKDATA, uint8_t SBLKDATA_length, uint8_t *rx_message_length){
	uint8_t txrx_step = 1;
	uint8_t nack = 0;
	uint8_t error_code = 0;
	uint8_t message_length = make_command(tx_buffer, tx_rx_buffer_size, MSGID, SBLKID, SBLKLEN, SBLKDATA, SBLKDATA_length);
	for(int i = 0; i < TRANSMIT_RECEIVE_RETRIES + txrx_step; i++){
		switch(txrx_step){
			case 0: // Read Any Remaining Receive Messages.
				if(error_code = receive_message(spi_handle, rx_buffer, tx_rx_buffer_size, rx_message_length)) {
					printf("Receive Failed with Error code: %X\n", error_code);
					if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
					else if(error_code == TIMEOUT_BEFORE_SYNC || error_code == TIMEOUT_AFTER_SYNC){
						printf("Transmitter Timed Out during Receive!\n");
						return error_code;
					}
					else printf("Unknown error occured with Receive...\n");
				}
				usleep(250);
				txrx_step = 1; // If successful, Attempt TX again.
				break;

			case 1: // START HERE !
				if(getHostInt()){
					printf("HOSTINT is still high, please read message before Transmitting.\n");
					txrx_step = 0; // Another Receive Message Waiting?
					break;
				}
				if(error_code = transmit_message(spi_handle, tx_buffer, message_length)) {
					printf("Transmit Failed with Error code: %X\n", error_code);
					return EXIT_FAILURE;
				}
				usleep(250);
				txrx_step = 2; // If successful TX, continue.
				break;

			case 2: // Wait for OK.
				if(error_code = receive_message(spi_handle, rx_buffer, tx_rx_buffer_size, rx_message_length)) {
					printf("Receive Failed with Error code: %X\n", error_code);
					if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
					else if(error_code == TIMEOUT_BEFORE_SYNC || error_code == TIMEOUT_AFTER_SYNC){
						printf("Transmitter Timed Out, Retry Comms.\n");
						txrx_step = 3; // Retry Comms
						break;
					}
					else printf("Unknown error occured with Receive...\n");
				}
				else if(*rx_message_length > 16){
					uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
					uint8_t direction = opcode & 0b0000000000001111;
					uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
					uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

					if(msgtype == 0b01 && msgid == 0x00){
						printf("Message ERROR received.\n");
					}
					else if(msgtype == 0b01) printf("Message Acknowledged.\n");
					else if(msgtype == 0b10) {
						printf("ERROR: Message Not Acknowledged.\n");
						if(!nack){
							printf(" Trying same message again...\n");
							txrx_step = 1;
							nack = 1;
							break;
						}
						printf("\n");
						return MESSAGE_NACK;
					}
				}
				usleep(250);
				txrx_step = 5;
				break;
				
			case 3:
				if(getHostInt()){
					printf("HOSTINT is still high, please read message before Transmitting.\n");
					txrx_step = 2; // Another Receive Message Waiting?
					break;
				}
				message_length = make_command_retry(tx_buffer, tx_rx_buffer_size, MSGID, SBLKID, SBLKLEN, SBLKDATA, SBLKDATA_length);
				if(error_code = transmit_message(spi_handle, tx_buffer, message_length)) {
					printf("Transmit Failed with Error code: %X\n", error_code);
					return EXIT_FAILURE;
				}
				usleep(250);
				txrx_step = 4;
				break;

			case 4: // Wait for OK.
				if(error_code = receive_message(spi_handle, rx_buffer, tx_rx_buffer_size, rx_message_length)) {
					printf("Receive Failed with Error code: %X\n", error_code);
					if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
					else if(error_code == TIMEOUT_BEFORE_SYNC || error_code == TIMEOUT_AFTER_SYNC){
						printf("RETRY Failed, No received communications. Perhaps incorrectly formatted message?\n");
						return error_code;
					}
					else printf("Unknown error occured with Receive...\n");
				}
				else if(*rx_message_length > 16){
					uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
					uint8_t direction = opcode & 0b0000000000001111;
					uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
					uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

					if(msgtype == 0b01 && msgid == 0x00){
						printf("Message ERROR received.\n");
					}
					else if(msgtype == 0b01) printf("Message Acknowledged.\n");
					else if(msgtype == 0b10) {
						printf("ERROR: Message Not Acknowledged.");
						if(!nack){
							printf(" Trying same message again...");
							txrx_step = 1;
							nack = 1;
							break;
						}
						printf("\n");
					}
				}
				usleep(250);
				txrx_step = 5;
				break;

			case 5:
				printf("Communication end. \n");
				return EXIT_SUCCESS;
		}
	}
	return EXIT_FAILURE;
}