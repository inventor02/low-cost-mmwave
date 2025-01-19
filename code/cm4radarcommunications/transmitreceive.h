#ifndef TRANSMITRECEIVE_H_
#define TRANSMITRECEIVE_H_

#include <stdint.h>
#include "crc_compute.h"

#define SPI_MODE SPI_MODE_0
#define SPI_BITS_PER_WORD 8
#define SPI_SPEED 100000

#define HOST_INT_TIMEOUT_US 1000
#define MAX_RECEIVE_MESSAGE_RETRIES 128
#define TRANSMIT_RECEIVE_RETRIES 10
#define MAX_CHECKSUM_RETRIES 1
#define MAX_CRC_RETRIES 1
 

#define TIMEOUT_BEFORE_SYNC 0x10
#define TIMEOUT_AFTER_SYNC 0x11
#define CHECKSUM_NO_MATCH 0x12
#define INVALID_CRC_LENGTH 0x13
#define CRC_NO_MATCH 0x14
#define HOST_INT_HIGH_TX 0x15
#define MESSAGE_NACK 0x16

int16_t make_command_params(uint8_t* message, int16_t message_buffer_size, uint16_t MSGID, uint16_t SBLKID, uint16_t SBLKLEN, uint8_t* SBLKDATA, uint8_t params);

static int16_t inline make_command(uint8_t* message, int16_t message_buffer_size, uint16_t MSGID, uint16_t SBLKID, uint16_t SBLKLEN, uint8_t* SBLKDATA)
{ make_command_params(message,message_buffer_size, MSGID, SBLKID, SBLKLEN, SBLKDATA, 0); }

static int16_t inline make_command_retry(uint8_t* message, int16_t message_buffer_size, uint16_t MSGID, uint16_t SBLKID, uint16_t SBLKLEN, uint8_t* SBLKDATA)
{ make_command_params(message,message_buffer_size, MSGID, SBLKID, SBLKLEN, SBLKDATA, 1); }

int transmit_message(int spi_handle, uint8_t *message, uint8_t bytes_to_transmit);

int receive_message(int spi_handle, uint8_t *rx_buffer, uint8_t max_bytes_to_read, uint8_t *rx_message_length);

int make_transmit_receive_message(int spi_handle, uint8_t* tx_buffer, uint8_t* rx_buffer, int16_t tx_rx_buffer_size, uint16_t MSGID, uint16_t SBLKID, uint16_t SBLKLEN, uint8_t* SBLKDATA, uint8_t *rx_message_length);

#endif