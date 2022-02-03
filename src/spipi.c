/**
 * @file 
 * @brief 
 * @details 
 * @copyright Copyright © Alkgrove Electronics 2018 Company Confidential
 * @author Robert Alkire 
 * @date 
 *
 **/
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "spipi.h"


static uint8_t spi_mode = SPI_MODE;
static uint8_t spi_bpw = SPI_BPW;
static char dev[32] = SPI_CHANNEL;
static uint32_t spi_speed = SPI_SPEED;

#ifdef USE_SYSLOG
#define SPI_ERROR(x) syslog(LOG_ERR, x); exit(-1)
#else
#define SPI_ERROR(x) fprintf(stderr, x); exit(-1)
#endif

int spi_open()
{
    int fd;
    int rv;
    fd = open(dev, O_RDWR);
    if (fd < 0) {
        SPI_ERROR("failed to open "SPI_CHANNEL);
    }
    rv = ioctl(fd, SPI_IOC_WR_MODE, &spi_mode);
	if (rv < 0) {
	    SPI_ERROR("can't set SPI (WR) mode");
	}
    rv = ioctl(fd, SPI_IOC_RD_MODE, &spi_mode);
	if (rv < 0) {
	    SPI_ERROR("can't set SPI (RD) mode");
	}
    rv = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bpw);
	if (rv < 0) {
	    SPI_ERROR("can't set SPI bits per word");
	}
    rv = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
	if (rv < 0) {
	    SPI_ERROR("can't set SPI speed");
	}
	return fd;
}
	
int spi_transfer(int fd, uint8_t *txbuf, uint8_t *rxbuf, uint32_t length) 
{

    int rv;
    struct spi_ioc_transfer spi = {
        .tx_buf = (uint32_t) txbuf,
        .rx_buf = (uint32_t) rxbuf,
        .len = length,
        .delay_usecs = SPI_DELAY,
        .speed_hz = spi_speed,
        .bits_per_word = spi_bpw,
        .cs_change = 1,
    };
    rv = ioctl(fd, SPI_IOC_MESSAGE(1), &spi);
    if (rv < 0) {
        SPI_ERROR("Unable to do SPI transfer");
    }
    return rv;
}