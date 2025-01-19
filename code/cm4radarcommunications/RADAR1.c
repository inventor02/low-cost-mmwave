#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "gpio.h"
#include "radar_commands.h"
#include "transmitreceive.h"


#define RAD1_SPI "/dev/spidev6.0"

#define TX_SIZE 255

int main() {

    int fd;

    uint8_t mode = SPI_MODE;
    uint8_t bits = SPI_BITS_PER_WORD;
    uint32_t speed = SPI_SPEED;

    uint8_t tx_buffer[TX_SIZE];
    uint8_t rx_buffer[TX_SIZE];

    gpioSetup();

    // Ensure they begin turned off.
    turnOffRadar();

    sleep(2);

    // Open the SPI device
    fd = open(RAD1_SPI, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        return EXIT_FAILURE;
    }

    // Set SPI mode
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("Failed to set SPI mode");
        close(fd);
        return EXIT_FAILURE;
    }

    // Set bits per word
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("Failed to set bits per word");
        close(fd);
        return EXIT_FAILURE;
    }

    // Set max speed
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("Failed to set max speed");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("SPI Set Up Correctly\n");

    // Perform SPI transfer

    if (radar_bootup(fd, tx_buffer, rx_buffer, TX_SIZE)) {
        perror("Failed to start RADAR1.\n");
        close(fd);
        turnOffRadar();
        return EXIT_FAILURE;
    }   

    usleep(100000);

    if (static_config(fd, tx_buffer, rx_buffer, TX_SIZE)) {
        perror("Failed to perform static config on RADAR1.\n");
        close(fd);
        turnOffRadar();
        return EXIT_FAILURE;
    }  

    usleep(100000);

    if (rf_init(fd, tx_buffer, rx_buffer, TX_SIZE)) {
        perror("Failed to perform rf_init on RADAR1.\n");
        close(fd);
        turnOffRadar();
        return EXIT_FAILURE;
    } 

    usleep(100000);

    if (datapath_config(fd, tx_buffer, rx_buffer, TX_SIZE)) {
        perror("Failed to perform Datapath Config on RADAR1.\n");
        close(fd);
        turnOffRadar();
        return EXIT_FAILURE;
    } 

    usleep(100000);

    if (dynamic_config(fd, tx_buffer, rx_buffer, TX_SIZE)) {
        perror("Failed to Set Dynamic Config for RADAR1.\n");
        close(fd);
        turnOffRadar();
        return EXIT_FAILURE;
    } 

    usleep(100000);

    if (test_source_enable(fd, tx_buffer, rx_buffer, TX_SIZE)) {
        perror("Failed to Set Test Source for RADAR1.\n");
        close(fd);
        turnOffRadar();
        return EXIT_FAILURE;
    } 

    usleep(100000);

    if (take_radar_image(fd, tx_buffer, rx_buffer, TX_SIZE)) {
        perror("Failed to take Radar Image on RADAR1.\n");
        close(fd);
        turnOffRadar();
        return EXIT_FAILURE;
    } 

    usleep(100000);

    // Close the SPI device
    close(fd);
    sleep(10);
    turnOffRadar();
    return EXIT_SUCCESS;
}
