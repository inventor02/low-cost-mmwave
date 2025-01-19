#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "radar_commands.h"
#include "transmitreceive.h"
#include "gpio.h"

/* Background on commands.

        Each command is sent as pairs of bytes.

        Each pair is sent sequentially.

        However, if there is a data value more than 4 bytes long, it is little endian,
        but only with respect to its pairs.

        eg: 0xAABBCCDD is sent as ... 0xCCDD _ 0xAABB

        Also, just to make it worse, when a pair is two single byte fields, 
                eg:  AWR_DEV_PMICCLOCK_CONF_SET_SB bytes 0 and 1...
        They are flipped in the pair. 

        For example, if AA BB CCDD were 1byte, 1byte and 2byte fields:
        It would be sent as BBAA _ CCDD.

        To combine the two rules:
                AA BB CCDD EEFFGGHH 1,1,2,4 byte field message is sent as:
        SYNC _ HEADER _ SBLKID/LEN _ BBAA _ CCDD _ GGHH _ EEFF _ CRC  

*/

void zero_array(uint8_t *array, uint8_t length){
        // Not Secure!

        for(uint8_t i = 0; i < length; i++){
                array[i] = 0x00;
        }
}

int radar_bootup(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length){

        /*

        Instruction order TAKEN FROM mmWave-Radar-Interface Guide.

        1. Power up the device
        2. Wait for AWR_AE_MSSPOWERUPDONE_SB
        3. Wait for AWR_AE_MSS_BOOTERRORSTATUS_SB if flash is not connected (Boot over SPI)
        4. AWR_DEV_CONFIGURATION_SET_SB
        5. AWR_DEV_RFPOWERUP_SB
        6. Wait for AWR_AE_RFPOWERUPDONE_SB

        */

        uint8_t error_code = 0;
        uint8_t bootup_step = 0;
        uint8_t rx_message_length = 0; // This message length is inclusive of sync, so is the exact length in bytes of the rx_buffer.
        uint8_t SBLKDATA[12];
        zero_array(SBLKDATA, 12);

        // Run the retry loop a maximum of POWER_UP_RETRIES (+ number of bootup steps).
        for(int i = 0; i < POWER_UP_RETRIES + bootup_step; i++){
                switch(bootup_step){
                        case 0: // Boot radar and wait for AWR_AE_MSSPOWERUPDONE_SB
                                turnOnRadar();
                                sleep(1);

                                if(error_code = receive_message(spi_handle, rx_buffer, buffer_length, &rx_message_length)) {
                                        printf("Receive Failed with Error code: %X", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                }
                                else if(rx_message_length > 24){
                                        uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
                                        uint8_t direction = opcode & 0b0000000000001111;
                                        uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
                                        uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

                                        if(msgid == 0x280){
                                                printf("AWR_DEV_ASYNC_EVENT_MSG\n");

                                                uint8_t nsbc = (rx_buffer[12] << 8) + rx_buffer[13];
                                                // Repeat for each subblock. Removed for now, to only allow a single subblock.
                                                for(int subblock = 0; subblock < 1; subblock++){
                                                        uint16_t sblkid = (rx_buffer[16] << 8) + rx_buffer[17];
                                                        uint16_t sblklen = (rx_buffer[18] << 8) + rx_buffer[19];

                                                        if(sblkid == 0x5000){
                                                                printf("-> AWR_AE_DEV_MSSPOWERUPDONE_SB\n");
                                                                printf("0x%X bytes length.\n");

                                                                uint32_t mss_powerup_time = (rx_buffer[20] << 24) + (rx_buffer[21] << 16) + (rx_buffer[22] << 8) + rx_buffer[23];
                                                                printf("Bootup time was %d ns\n", (uint64_t)(mss_powerup_time * 5));

                                                                bootup_step = SPI_FLASH_PRESENT + 1;
                                                                usleep(500);
                                                                break; // Break out of the for loop.
                                                        }
                                                }
                                                break; // Break out of the case statement.
                                        }
                                }

                                // Reboot the radar if power up successful not received.
                                turnOffRadar();
                                sleep(1);
                                break;

                        case 1: // Wait for AWR_AE_MSS_BOOTERRORSTATUS_SB
                                if(error_code = receive_message(spi_handle, rx_buffer, buffer_length, &rx_message_length)) {
                                        printf("Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                }
                                else if(rx_message_length > 24){
                                        uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
                                        uint8_t direction = opcode & 0b0000000000001111;
                                        uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
                                        uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

                                        if(msgid == 0x280){
                                                printf("AWR_DEV_ASYNC_EVENT_MSG\n");

                                                uint8_t nsbc = (rx_buffer[12] << 8) + rx_buffer[13];
                                                // Repeat for each subblock. Removed for now, to only allow a single subblock.
                                                for(int subblock = 0; subblock < 1; subblock++){
                                                        uint16_t sblkid = (rx_buffer[16] << 8) + rx_buffer[17];
                                                        uint16_t sblklen = (rx_buffer[18] << 8) + rx_buffer[19];

                                                        if(sblkid == 0x5005){
                                                                printf("-> AWR_AE_MSS_BOOTERRORSTATUS_SB\n");
                                                                printf("0x%X bytes length.\n");

                                                                uint32_t mss_powerup_time = (rx_buffer[20] << 24) + (rx_buffer[21] << 16) + (rx_buffer[22] << 8) + rx_buffer[23];
                                                                printf("Bootup time was %d ns\n", (uint64_t)(mss_powerup_time * 5));

                                                                bootup_step = 2;
                                                                usleep(500);
                                                                break;
                                                        }
                                                }
                                                break;
                                        }
                                }

                                // Reboot the radar if message not received.
                                turnOffRadar();
                                bootup_step = 0;
                                sleep(1);
                                break;

                        case 2: // AWR_DEV_CONFIGURATION_SET_SB
				if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x202, 0x404C, 16, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                bootup_step = 3;
                                break;        

                        case 3: // Get MSS Firmware Version AWR_MSSVERSION_GET_SB
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x207, 0x40E0, 4, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                if(rx_message_length > 35){
                                        uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
                                        uint8_t direction = opcode & 0b0000000000001111;
                                        uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
                                        uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

                                        if(msgid == 0x207){
                                                printf("AWR_DEV_STATUS_GET_MSG\n");

                                                uint8_t nsbc = (rx_buffer[12] << 8) + rx_buffer[13];
                                                // Repeat for each subblock. Removed for now, to only allow a single subblock.
                                                for(int subblock = 0; subblock < 1; subblock++){
                                                        uint16_t sblkid = (rx_buffer[16] << 8) + rx_buffer[17];
                                                        uint16_t sblklen = (rx_buffer[18] << 8) + rx_buffer[19];

                                                        if(sblkid == 0x40E0){
                                                                printf("-> AWR_MSSVERSION_SB\n");
                                                                printf("0x%X bytes message length.\n", rx_message_length);
                                                                uint8_t MSSHWVersionMajor =      rx_buffer[20];
                                                                uint8_t MSSHWVariant =           rx_buffer[21];
                                                                uint8_t MSSFWVersionMajor =      rx_buffer[22];
                                                                uint8_t MSSHWVersionMinor =      rx_buffer[23];
                                                                uint8_t MSSFWVersionBuild =      rx_buffer[24];
                                                                uint8_t MSSFWVersionMinor =      rx_buffer[25];
                                                                uint8_t MSSFWVersionYear =       rx_buffer[26];
                                                                uint8_t MSSFWVersionDebug =      rx_buffer[27];
                                                                uint8_t MSSFWVersionDay =        rx_buffer[28];
                                                                uint8_t MSSFWVersionMonth =      rx_buffer[29];
                                                                uint8_t MSSFWVersionPatchMinor = rx_buffer[30];
                                                                uint8_t MSSFWVersionPatchMajor = rx_buffer[31];
                                                                uint8_t MSSFWVersionPatchMonth = rx_buffer[32];
                                                                uint8_t MSSFWVersionPatchYear =  rx_buffer[33];
                                                                uint8_t MSSFWVersionPatchBuild = rx_buffer[34] >> 4;
                                                                uint8_t MSSFWVersionPatchDebug = rx_buffer[34] & 0x0F;
                                                                uint8_t MSSFWVersionPatchDay =   rx_buffer[35];
                                                                printf("MSS HW Version: %d.%d Variant %d\n", MSSHWVersionMajor, MSSHWVersionMinor, MSSHWVariant); 
                                                                printf("MSS FW Version: %d.%d.%d.%d Dated %d/%d/%d\n", MSSFWVersionMajor, MSSFWVersionMinor, MSSFWVersionBuild, MSSFWVersionDebug, MSSFWVersionDay, MSSFWVersionMonth, MSSFWVersionYear);
                                                                
                                                                printf("MSS FW Patch: %d.%d.%d.%d Dated %d/%d/%d\n", MSSFWVersionPatchMajor, MSSFWVersionPatchMinor, MSSFWVersionPatchBuild, MSSFWVersionPatchDebug, MSSFWVersionPatchDay, MSSFWVersionPatchMonth, MSSFWVersionPatchYear);
                                                                bootup_step = 4;
                                                                usleep(500);
                                                                break;
                                                        }
                                                }
                                                break;
                                        }
                                }
                                usleep(250);
                                bootup_step = 4;
                                break;        

                        case 4: // AWR_DEV_RFPOWERUP_SB
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x200, 0x4000, 4, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                bootup_step = 5;
                                break;

                        case 5: // Wait for AWR_AE_RFPOWERUPDONE_SB
                                if(error_code = receive_message(spi_handle, rx_buffer, buffer_length, &rx_message_length)) {
                                        printf("Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                }
                                else if(rx_message_length > 24){
                                        uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
                                        uint8_t direction = opcode & 0b0000000000001111;
                                        uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
                                        uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

                                        if(msgid == 0x280){
                                                printf("AWR_DEV_ASYNC_EVENT_MSG\n");

                                                uint8_t nsbc = (rx_buffer[12] << 8) + rx_buffer[13];
                                                // Repeat for each subblock. Removed for now, to only allow a single subblock.
                                                for(int subblock = 0; subblock < 1; subblock++){
                                                        uint16_t sblkid = (rx_buffer[16] << 8) + rx_buffer[17];
                                                        uint16_t sblklen = (rx_buffer[18] << 8) + rx_buffer[19];

                                                        if(sblkid == 0x5001){
                                                                printf("-> AWR_AE_RFPOWERUPDONE_SB\n");
                                                                printf("0x%X bytes length.\n");

                                                                uint32_t bss_powerup_bist_status_flags = (rx_buffer[20] << 24) + (rx_buffer[21] << 16) + (rx_buffer[22] << 8) + rx_buffer[23];
                                                                printf("RF Startup flags were %X\n", (bss_powerup_bist_status_flags));

                                                                bootup_step = 6;
                                                                usleep(500);
                                                                break;
                                                        }
                                                }
                                                break;
                                        }
                                }

                                // Wait longer for startup if message not received/received correctly.
                                usleep(10000);
                                break;

                        case 6: // Get BSS Firmware Version AWR_RF_VERSION_GET_SB
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x11, 0x0220, 4, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                if(rx_message_length > 36){
                                        uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
                                        uint8_t direction = opcode & 0b0000000000001111;
                                        uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
                                        uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

                                        if(msgid == 0x11){
                                                printf("AWR_RF_STATUS_GET_MSG\n");

                                                uint8_t nsbc = (rx_buffer[12] << 8) + rx_buffer[13];
                                                // Repeat for each subblock. Removed for now, to only allow a single subblock.
                                                for(int subblock = 0; subblock < 1; subblock++){
                                                        uint16_t sblkid = (rx_buffer[16] << 8) + rx_buffer[17];
                                                        uint16_t sblklen = (rx_buffer[18] << 8) + rx_buffer[19];

                                                        if(sblkid == 0x0220){
                                                                printf("-> AWR_RF_VERSION_SB\n");
                                                                printf("0x%X bytes message length.\n", rx_message_length);

                                                                uint8_t BSSHWVersionMajor =      rx_buffer[20];
                                                                uint8_t BSSHWVariant =           rx_buffer[21];
                                                                uint8_t BSSFWVersionMajor =      rx_buffer[22];
                                                                uint8_t BSSHWVersionMinor =      rx_buffer[23];
                                                                uint8_t BSSFWVersionBuild =      rx_buffer[24];
                                                                uint8_t BSSFWVersionMinor =      rx_buffer[25];
                                                                uint8_t BSSFWVersionYear =       rx_buffer[26];
                                                                uint8_t BSSFWVersionDebug =      rx_buffer[27];
                                                                uint8_t BSSFWVersionDay =        rx_buffer[28];
                                                                uint8_t BSSFWVersionMonth =      rx_buffer[29];
                                                                uint8_t BSSFWVersionPatchMinor = rx_buffer[30];
                                                                uint8_t BSSFWVersionPatchMajor = rx_buffer[31];
                                                                uint8_t BSSFWVersionPatchMonth = rx_buffer[32];
                                                                uint8_t BSSFWVersionPatchYear =  rx_buffer[33];
                                                                uint8_t BSSFWVersionPatchBuild = rx_buffer[34] >> 4;
                                                                uint8_t BSSFWVersionPatchDebug = rx_buffer[34] & 0x0F;
                                                                uint8_t BSSFWVersionPatchDay =   rx_buffer[35];
                                                                printf("BSS HW Version: %d.%d Variant %d\n", BSSHWVersionMajor, BSSHWVersionMinor, BSSHWVariant); 
                                                                printf("BSS FW Version: %d.%d.%d.%d Dated %d/%d/%d\n", BSSFWVersionMajor, BSSFWVersionMinor, BSSFWVersionBuild, BSSFWVersionDebug, BSSFWVersionDay, BSSFWVersionMonth, BSSFWVersionYear);
                                                                
                                                                printf("BSS FW Patch: %d.%d.%d.%d Dated %d/%d/%d\n", BSSFWVersionPatchMajor, BSSFWVersionPatchMinor, BSSFWVersionPatchBuild, BSSFWVersionPatchDebug, BSSFWVersionPatchDay, BSSFWVersionPatchMonth, BSSFWVersionPatchYear);
                                                                
                                                                bootup_step = 7;
                                                                usleep(500);
                                                                break;
                                                        }
                                                }
                                                break;
                                        }
                                }
                                usleep(250);
                                bootup_step = 7;
                                break;

                        case 7:
                                printf("Radar Bootup Successful!\n");
                                return 0;
                }
        }

        printf("Radar Bootup Unsuccessful - TIMED OUT!\n");
        return EXIT_FAILURE;

}

int static_config(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length){

        /*

        Instructions taken from mmWave-Radar-Interface Guide.

        7. AWR_RF_STATIC_CONF_SET_MSG
                a. AWR_RF_DEVICE_CFG_SB
                b. AWR_CHAN_CONF_SET_SB
                c. AWR_ADCOUT_CONF_SET_SB
                d. AWR_RF_LDO_BYPASS_SB with RFLDOBYPASS_EN set to 1 if RF supply is 1.0 V
                e. AWR_LOWPOWERMODE_CONF_SET_SB
                f. AWR_DYNAMICPOWERSAVE_CONF_SET_SB
                g. AWR_CAL_MON_FREQUENCY_TX_POWER_LIMITS_SB
                h. AWR_RF_RADAR_MISC_CTL_SB if per chirp phase shifter and Advance chirp configuration
                needs to be enabled.
                i. AWR_APLL_SYNTH_BW_CONTROL_SB

				DONT SEEM TO WORK!
				Using workflow from mmwave studio!

        */


        uint8_t error_code = 0;
        uint8_t config_step = 0;
        uint8_t rx_message_length = 0; // This message length is inclusive of sync, so is the exact length in bytes of the rx_buffer.
        uint8_t SBLKDATA[28];
        zero_array(SBLKDATA, 28);

        // Run the retry loop a maximum of STATIC_CONFIG_RETRIES (+ number of config steps).
        for(int i = 0; i < STATIC_CONFIG_RETRIES + config_step; i++){
                switch(config_step){
                        case 0: // AWR_RF_DEVICE_CFG_SB
                                SBLKDATA[3] = 0b00000101;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0086, 16, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 1;
                                break;

                        case 1: // AWR_CHAN_CONF_SET_SB
                                SBLKDATA[1] = 0b00001111;
                                SBLKDATA[3] = 0b00000011;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0080, 12, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 2;
                                break;

                        case 2: // AWR_ADCOUT_CONF_SB
                                zero_array(SBLKDATA, 28);
                                SBLKDATA[1] = 0b00000010;
                                SBLKDATA[3] = 0b00000000;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0082, 12, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 3;
                                break;

			case 3: // AWR_DEV_RX_DATA_FORMAT_CONF_SET_SB
                                SBLKDATA[1] = 0b00001111;
                                SBLKDATA[3] = 0b00000010;
                                SBLKDATA[5] = 0b00000000;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x202, 0x4041, 16, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 4;
                                break;

                        case 4: // AWR_RF_LDO_BYPASS_SB
                                /*

                                CAUTION: Do not enable RF LDO bypass option when the PMIC is configured
                                                to supply 1.3V to VIN_13RF1 and VIN_13RF2 analog and RF
                                                power supply inputs. This may damage the device. Typically in TI
                                                EVMs, PMIC is configured to supply 1.3V to the RF supplies.

                                */

                                zero_array(SBLKDATA, 28);
                                SBLKDATA[1] = 0b00000011; // LDO_BYPASS and PA_LDO_DISABLE
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x16, 0x02CC, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 5;
                                break;

                        case 5: // AWR_LOWPOWERMODE_CONF_SET_SB
                                SBLKDATA[1] = 0x01;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0083, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 6;
                                break;

                        case 6: // AWR_DYNAMICPOWERSAVE_CONF_SET_SB
                                SBLKDATA[1] = 0b00000111;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0084, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 7;
                                break;

                        case 7: // AWR_CAL_MON_FREQUENCY_TX_POWER_LIMITS_SB
                                SBLKDATA[0] = (uint8_t)(7600 >> 8);
                                SBLKDATA[1] = (uint8_t)(7600 & 0xFF);
                                SBLKDATA[2] = (uint8_t)(7600 >> 8);
                                SBLKDATA[3] = (uint8_t)(7600 & 0xFF);
                                SBLKDATA[4] = (uint8_t)(7600 >> 8);
                                SBLKDATA[5] = (uint8_t)(7600 & 0xFF);
                                SBLKDATA[6] = (uint8_t)(8100 >> 8);
                                SBLKDATA[7] = (uint8_t)(8100 & 0xFF);
                                SBLKDATA[8] = (uint8_t)(8100 >> 8);
                                SBLKDATA[9] = (uint8_t)(8100 & 0xFF);
                                SBLKDATA[10] = (uint8_t)(8100 >> 8);
                                SBLKDATA[11] = (uint8_t)(8100 & 0xFF);
                                // Set min and max frequencies for each TX.

                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x008A, 28, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 8;
                                break;

                        case 8: // AWR_APLL_SYNTH_BW_CONTROL_SB
                                zero_array(SBLKDATA, 28);
                                SBLKDATA[0] = 0x08;
                                SBLKDATA[1] = 0x01;
                                SBLKDATA[2] = 0x09;
                                SBLKDATA[3] = 0x26;
                                SBLKDATA[4] = 0x00;
                                SBLKDATA[5] = 0x00;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x008D, 20, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 9;
                                break;

                        case 9:
                                printf("Radar Static Config Successful!\n");
                                return EXIT_SUCCESS;
                }
        }

        printf("Radar Static Config Unsuccessful - TIMED OUT!\n");
        return EXIT_FAILURE;

}

int datapath_config(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length){

        /*

        Instructions taken from mmWave-Radar-Interface Guide.

        8. Data path configurations
                a. AWR_DEV_RX_DATA_FORMAT_CONF_SET_SB
                b. AWR_DEV_RX_DATA_PATH_CONF_SET_SB
                c. AWR_DEV_RX_DATA_PATH_LANE_EN_SB
                d. AWR_DEV_RX_DATA_PATH_CLK_SET_SB
                e. AWR_HIGHSPEEDINTFCLK_CONF_SET_SB
                f. AWR_DEV_LVDS_CFG_SET_SB / AWR_DEV_CSI2_CFG_SET_SB

        */


        uint8_t error_code = 0;
        uint8_t config_step = 0;
        uint8_t rx_message_length = 0; // This message length is inclusive of sync, so is the exact length in bytes of the rx_buffer.
        uint8_t SBLKDATA[12];
        zero_array(SBLKDATA, 12);

        // Run the retry loop a maximum of DATAPATH_CONFIG_RETRIES (+ number of config steps).
        for(int i = 0; i < DATAPATH_CONFIG_RETRIES + config_step; i++){
                switch(config_step){
                        
                        case 0: // AWR_DEV_RX_DATA_PATH_CONF_SET_SB
                                zero_array(SBLKDATA, 12);
                                SBLKDATA[0] = 0x01; // DATA_TRANS_FMT_PKT0
                                //SBLKDATA[1] = 0x01; // LVDS
                                SBLKDATA[2] = 0x02; // CQ_CONFIG
                                SBLKDATA[4] = 0x40; // CQ1_TRANS_SIZE
                                SBLKDATA[5] = 0x40; // CQ0_TRANS_SIZE
                                SBLKDATA[7] = 0x40; // CQ2_TRANS_SIZE
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x202, 0x4042, 12, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 1;
                                break;

                        case 1: // AWR_DEV_RX_DATA_PATH_CLK_SET_SB
                                zero_array(SBLKDATA, 12);
                                SBLKDATA[0] = 0b00000001;
                                SBLKDATA[1] = 0b00000001;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x202, 0x4044, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 2;
                                break;

                        case 2: // AWR_HIGHSPEEDINTFCLK_CONF_SET_SB
                                SBLKDATA[0] = 0b00000000;
                                SBLKDATA[1] = 0b00001001;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0085, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 3;
                                break;

			case 3: // AWR_DEV_RX_DATA_PATH_LANE_EN_SB
                                zero_array(SBLKDATA, 12);
                                SBLKDATA[1] = 0b00001111;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x202, 0x4043, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 4;
                                break;

                        case 4: // AWR_DEV_CSI2_CFG_SET_SB
                                SBLKDATA[0] = 0b01010100;
                                SBLKDATA[1] = 0b00100001;
                                SBLKDATA[2] = 0b00000000;
                                SBLKDATA[3] = 0b00000011; // 0b 00000000 0000 0011-0101-0100-0010-0001
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x202, 0x4047, 12, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 5;
                                break;

                        case 5:
                                printf("Radar Data Config Successful!\n");
                                return 0;
                }
        }

        printf("Radar Data Path Config Unsuccessful - TIMED OUT!\n");
        return EXIT_FAILURE;

}

int rf_init(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length){
        static uint8_t already_initialised = 0;
        if(already_initialised) return RF_ALREADY_INITIALISED;

        uint8_t error_code = 0;
        uint8_t config_step = 0;
        uint8_t rx_message_length = 0; // This message length is inclusive of sync, so is the exact length in bytes of the rx_buffer.
        uint8_t SBLKDATA[2] = {0x00, 0x00};

        // Run the retry loop a maximum of RF_INIT_RETRIES (+ number of config steps).
        for(int i = 0; i < RF_INIT_RETRIES + config_step; i++){
                switch(config_step){

                        case 0: // AWR_RF_INIT_SB
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x06, 0x00C0, 4, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 1;
                                break;

                        case 1: // Wait for AWR_AE_RFPOWERUPDONE_SB
                                if(error_code = receive_message(spi_handle, rx_buffer, buffer_length, &rx_message_length)) {
                                        printf("Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                }
                                else if(rx_message_length > 24){
                                        uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
                                        uint8_t direction = opcode & 0b0000000000001111;
                                        uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
                                        uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

                                        if(msgid == 0x80){
                                                printf("AWR_RF_ASYNC_EVENT_MSG1\n");

                                                uint8_t nsbc = (rx_buffer[12] << 8) + rx_buffer[13];
                                                // Repeat for each subblock. Removed for now, to only allow a single subblock.
                                                for(int subblock = 0; subblock < 1; subblock++){
                                                        uint16_t sblkid = (rx_buffer[16] << 8) + rx_buffer[17];
                                                        uint16_t sblklen = (rx_buffer[18] << 8) + rx_buffer[19];

                                                        if(sblkid == 0x1004){
                                                                printf("-> AWR_AE_RF_INITCALIBSTATUS_SB\n");
                                                                printf("0x%X bytes length.\n");

                                                                uint32_t calibration_status = (rx_buffer[20] << 24) + (rx_buffer[21] << 16) + (rx_buffer[22] << 8) + rx_buffer[23];
                                                                printf("RF Init Calibration Status is %X\n", (calibration_status));

                                                                config_step = 2;
                                                                usleep(500);
                                                                break;
                                                        }
                                                }
                                                break;
                                        }
                                }

                                // Wait 1 Second for RF Init.
                                usleep(1000000);
                                break;

                        case 2:
                                printf("RF Init Successful!\n");
                                return 0;
                }
        }
}

int dynamic_config(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length){

        /*

        Instructions taken from mmWave-Radar-Interface Guide.

        10. AWR_RF_DYNAMIC_CONF_SET_MSG
                a. AWR_PROG_FILT_COEFF_RAM_SET_SB
                b. AWR_PROG_FILT_CONF_SET_SB
                c. AWR_PROFILE_CONF_SET_SB
                d. Chirp configuration API
                        a. AWR_CHIRP_CONF_SET_SB or
                        b. AWR_ADVANCE_CHIRP_CONF_SB and
                        c. AWR_ADVANCE_CHIRP_GENERIC_LUT_LOAD_SB
                e. AWR_LOOPBACK_BURST_CONF_SET_SB (if using loopback burst in advance frame
                        config API)
                f. AWR_FRAME_CONF_SET_SB or AWR_ADVANCED_FRAME_CONF_SB with SW
                        or HW triggered mode and AWR_DEV_FRAME_CONFIG_APPLY_MSG.
                g. AWR_CALIB_MON_TIME_UNIT_CONF_SB with CALIB_MON_TIME_UNIT value set
                        to a value such that the total frame idle time across multiple CALIB_MON_TIME_
                        UNITs is sufficient for all calibrations and monitoring. See Section 12 for details on
                        calibration and monitoring durations. If any error AWR_CAL_MON_TIMING_FAIL_
                        REPORT_AE_SB AE will be generated when frame is triggered. The calibrations
                        and monitors will not run properly if this error is generated.
                h. Set NUM_OF_CASCADED_DEV to 1, DEVICE_ID to 0 and Set MONITORING_
                        MODE = 0 (MONITORING_MODE 1 is recommended only in cascade mode) in
                        AWR_CALIB_MON_TIME_UNIT_CONF_SB API
                i. AWR_RUN_TIME_CALIBRATION_CONF_AND_TRIGGER_SB (set all ONE_TIME_
                        CALIB_ENABLE_MASK and set ENABLE_CAL_REPORT = 1)
                j. Wait for AWR_RUN_TIME_CALIBRATION_SUMMARY_REPORT_AE_SB
                k. AWR_RUN_TIME_CALIBRATION_CONF_AND_TRIGGER_SB (set all RUN_TIME_
                        CALIB_ENABLE_MASK and set ENABLE_CAL_REPORT = 0 to avoid receiving periodic
                        async events)
                l. AWR_DEV_FRAME_CONFIG_APPLY_SB or AWR_DEV_ADV_FRAME_CONFIG_APPLY_SB

        */


        uint8_t error_code = 0;
        uint8_t config_step = 2;
        uint8_t rx_message_length = 0; // This message length is inclusive of sync, so is the exact length in bytes of the rx_buffer.
        uint8_t SBLKDATA[44];
        zero_array(SBLKDATA, 44);

        // Run the retry loop a maximum of DYNAMIC_CONFIG_RETRIES (+ number of config steps).
        for(int i = 0; i < DYNAMIC_CONFIG_RETRIES + config_step; i++){
                switch(config_step){
                        case 0: // AWR_PROG_FILT_COEFF_RAM_SET_SB

                                // First Three decimation filter commands are not currently used.

                                /*SBLKDATA[3] = 0b00000101;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0086, 16, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250); */
                                config_step = 1;
                                break;

                        case 1: // AWR_PROG_FILT_CONF_SET_SB
                                /*SBLKDATA[0] = 0x00; // Filter Coeff Start Index
                                SBLKDATA[1] = 0x00; // PF_INDEX
                                SBLKDATA[3] = 0b00000011;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0080, 12, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250); */
                                config_step = 2;
                                break;

                        case 2: // AWR_PROFILE_CONF_SET_SB
                                SBLKDATA[1] = 0x00; // PF_INDX

                                SBLKDATA[3] = 0b00000010; // PF_VCO_SELECT

                                SBLKDATA[4] = 0x4B; // PF_FREQ_START_CONST 0x558E4BBB
                                SBLKDATA[5] = 0xBB;
                                SBLKDATA[6] = 0x55;
                                SBLKDATA[7] = 0x8E;

                                SBLKDATA[8] = 0x04; // PF_IDLE_TIME_CONST 0x044C
                                SBLKDATA[9] = 0x4C;

                                SBLKDATA[12] = 0x01; // PF_ADC_START_TIME_CONST 0x018F
                                SBLKDATA[13] = 0x8F;

                                SBLKDATA[16] = 0x29; // PF_RAMP_END_TIME 0x29F4
                                SBLKDATA[17] = 0xF4;

                                SBLKDATA[28] = 0x02; // PF_FREQ_SLOPE_CONST 0x027D
                                SBLKDATA[29] = 0x7D;

                                SBLKDATA[30] = 0x00; // PF_TX_START_TIME 0x0000
                                SBLKDATA[31] = 0x00;
                                SBLKDATA[32] = 0x04; // PF_NUM_ADC_SAMPLES 0x0400
                                SBLKDATA[33] = 0x00;
                                SBLKDATA[34] = 0x29; // PF_DIGITAL_OUTPUT_SAMPLING_RATE 0x2920
                                SBLKDATA[35] = 0x22;
                                SBLKDATA[38] = 0x10000001; // TX_CAL_EN_CFG
                                SBLKDATA[39] = 0b00010001;
                                SBLKDATA[41] = 0b10011110; // RX_GAIN

                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x08, 0x0100, 48, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 3;
                                break;

                        case 3: // AWR_CHIRP_CONF_SET_SB (for TX 1)
                                zero_array(SBLKDATA, 44);
                                SBLKDATA[1] = 0x00; // CHIRP_START_INDX
                                SBLKDATA[3] = 0x00; // CHIRP_END_INDX
                                SBLKDATA[5] = 0x00; // PROFILE_INDX
                                SBLKDATA[19] = 0b00000001; // CHIRP_TX_EN
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x08, 0x0101, 24, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 4;
                                break;

                        case 4: // AWR_CHIRP_CONF_SET_SB (for TX 2)
                                zero_array(SBLKDATA, 44);
                                SBLKDATA[1] = 0x01; // CHIRP_START_INDX
                                SBLKDATA[3] = 0x01; // CHIRP_END_INDX
                                SBLKDATA[5] = 0x00; // PROFILE_INDX
                                SBLKDATA[19] = 0b00000010; // CHIRP_TX_EN
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x08, 0x0101, 24, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 5;
                                break;

                        case 5: // AWR_ADVANCE_CHIRP_CONF_SB
                                zero_array(SBLKDATA, 44);
                                /*if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0083, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250); */
                                config_step = 6;
                                break;

                        case 6: // AWR_ADVANCE_CHIRP_GENERIC_LUT_LOAD_SB

                                /*if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x04, 0x0084, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250); */
                                config_step = 7;
                                break;

                        case 7: // AWR_FRAME_CONF_SET_SB
                                SBLKDATA[3] = 0x00; // CHIRP_START_INDEX
                                SBLKDATA[5] = 0x01; // CHIRP_END_INDEX
                                SBLKDATA[7] = 0x0A; // NUM_LOOPS
                                SBLKDATA[9] = 0x08; // NUM_FRAMES
                                SBLKDATA[12] = 0xE3; // FRAME_PERIODICITY 0xBE3D4
                                SBLKDATA[13] = 0xD4;
                                SBLKDATA[14] = 0x00;
                                SBLKDATA[15] = 0x0B;
                                SBLKDATA[17] = 0x01; // TRIGGER_SELECT

                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x08, 0x0102, 28, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 8;
                                break;

                        case 8: // AWR_DEV_FRAME_CONFIG_APPLY_SB
                                zero_array(SBLKDATA, 44);
                                SBLKDATA[3] = 0x01;
                                SBLKDATA[4] = 0x04;
                                SBLKDATA[5] = 0x00;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x206, 0x40C0, 12, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 9;
                                break;

                        case 9:
                                printf("Radar Dynamic Config Successful!\n");
                                return EXIT_SUCCESS;
                }
        }

        printf("Radar Dynamic Config Unsuccessful - TIMED OUT!\n");
        return EXIT_FAILURE;

}

int test_source_enable(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length){
        uint8_t error_code = 0;
        uint8_t config_step = 0;
        uint8_t rx_message_length = 0; // This message length is inclusive of sync, so is the exact length in bytes of the rx_buffer.
        uint8_t SBLKDATA[4] = {0x00, 0x00, 0x00, 0x00};

        // Run the retry loop a maximum of RF_INIT_RETRIES (+ number of config steps).
        for(int i = 0; i < RF_INIT_RETRIES + config_step; i++){
                switch(config_step){

                        case 0: // AWR_DEV_TESTPATTERN_GEN_SET_SB
                                SBLKDATA[1] = 0x01;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x16, 0x02C3, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 1;
                                break;

                        case 1:
                                printf("Test Pattern Get Successful!\n");
                                return 0;
                }
        }
}

int take_radar_image(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length){
        uint8_t error_code = 0;
        uint8_t config_step = 0;
        uint8_t rx_message_length = 0; // This message length is inclusive of sync, so is the exact length in bytes of the rx_buffer.
        uint8_t SBLKDATA[4] = {0x00, 0x00, 0x00, 0x00};

        // Run the retry loop a maximum of RF_INIT_RETRIES (+ number of config steps).
        for(int i = 0; i < RF_INIT_RETRIES + config_step; i++){
                switch(config_step){

                        case 0: // AWR_FRAMESTARTSTOP_CONF_SB (Start)
                                SBLKDATA[1] = 0x01;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x0A, 0x0140, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 1;
                                break;

                        case 1: // Wait for AWR_AE_RF_FRAME_TRIGGER_RDY_SB
                                if(error_code = receive_message(spi_handle, rx_buffer, buffer_length, &rx_message_length)) {
                                        printf("Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                }
                                else if(rx_message_length > 16){
                                        uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
                                        uint8_t direction = opcode & 0b0000000000001111;
                                        uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
                                        uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

                                        if(msgid == 0x80){
                                                printf("AWR_RF_ASYNC_EVENT_MSG1\n");

                                                uint8_t nsbc = (rx_buffer[12] << 8) + rx_buffer[13];
                                                // Repeat for each subblock. Removed for now, to only allow a single subblock.
                                                for(int subblock = 0; subblock < 1; subblock++){
                                                        uint16_t sblkid = (rx_buffer[16] << 8) + rx_buffer[17];
                                                        uint16_t sblklen = (rx_buffer[18] << 8) + rx_buffer[19];

                                                        if(sblkid == 0x100B){
                                                                printf("-> AWR_AE_RF_FRAME_TRIGGER_RDY_SB\n");

                                                                config_step = 3;
                                                                usleep(500);
                                                                break;
                                                        }
                                                }
                                                break;
                                        }
                                }

                                // Wait 1 Second for RF Init.
                                usleep(100000);
                                break;

                        case 2: // AWR_FRAMESTARTSTOP_CONF_SB (Stop)
                                SBLKDATA[1] = 0x00;
                                if(error_code = make_transmit_receive_message(spi_handle, tx_buffer, rx_buffer, buffer_length, 0x0A, 0x0140, 8, SBLKDATA, &rx_message_length)) {
                                        printf("Transmit/Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                        else break;
                                }
                                usleep(250);
                                config_step = 3;
                                break;

                        case 3: // Wait for AWR_FRAME_END_AE_SB
                                if(error_code = receive_message(spi_handle, rx_buffer, buffer_length, &rx_message_length)) {
                                        printf("Receive Failed with Error code: %X\n", error_code);
                                        if(error_code == EXIT_FAILURE) return EXIT_FAILURE;
                                }
                                else if(rx_message_length > 16){
                                        uint16_t opcode = (rx_buffer[4] << 8) + rx_buffer[5];
                                        uint8_t direction = opcode & 0b0000000000001111;
                                        uint8_t msgtype =  (opcode & 0b0000000000110000) >> 4;
                                        uint16_t msgid =   (opcode & 0b1111111111000000) >> 6;

                                        if(msgid == 0x80){
                                                printf("AWR_RF_ASYNC_EVENT_MSG1\n");

                                                uint8_t nsbc = (rx_buffer[12] << 8) + rx_buffer[13];
                                                // Repeat for each subblock. Removed for now, to only allow a single subblock.
                                                for(int subblock = 0; subblock < 1; subblock++){
                                                        uint16_t sblkid = (rx_buffer[16] << 8) + rx_buffer[17];
                                                        uint16_t sblklen = (rx_buffer[18] << 8) + rx_buffer[19];

                                                        if(sblkid == 0x100F){
                                                                printf("-> AWR_FRAME_END_AE_SB\n");

                                                                config_step = 4;
                                                                usleep(500);
                                                                break;
                                                        }
                                                }
                                                break;
                                        }
                                }

                                // Wait 1 Second for RF Init.
                                usleep(100000);
                                break;

                        case 4:
                                printf("Radar Image Taken Successfully!\n");
                                return 0;
                }
        }
}
