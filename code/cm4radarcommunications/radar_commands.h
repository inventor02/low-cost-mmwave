#ifndef __RADAR_COMMANDS_H__
#define __RADAR_COMMANDS_H__

#include <stdint.h>

#define RF_ALREADY_INITIALISED 0x50

#define SPI_FLASH_PRESENT 1

#define POWER_UP_RETRIES 10
#define STATIC_CONFIG_RETRIES 10
#define DYNAMIC_CONFIG_RETRIES 10
#define DATAPATH_CONFIG_RETRIES 10
#define RF_INIT_RETRIES 10

int radar_bootup(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length);

int static_config(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length);

int datapath_config(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length);

int rf_init(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length);

int dynamic_config(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length);

int test_source_enable(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length);

int take_radar_image(int spi_handle, uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t buffer_length);

#endif // __RADAR_COMMANDS_H__