/**
 * @file spipi.h
 * @brief 
 * @details b
 * @copyright Copyright © Alkgrove Electronics 2018 Company Confidential
 * @author Robert Alkire 
 * @date 
 *
 **/
#ifndef __SPIPI_H__
#define __SPIPI_H__
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#define SPI_BPW 8
#define SPI_CHANNEL "/dev/spidev0.1"
// SPI_MODE_0 (0,0) 	CPOL = 0, CPHA = 0, Clock idle low, data is clocked in on rising edge, output data (change) on falling edge
// SPI_MODE_1 (0,1) 	CPOL = 0, CPHA = 1, Clock idle low, data is clocked in on falling edge, output data (change) on rising edge
// SPI_MODE_2 (1,0) 	CPOL = 1, CPHA = 0, Clock idle high, data is clocked in on falling edge, output data (change) on rising edge
// SPI_MODE_3 (1,1) 	CPOL = 1, CPHA = 1, Clock idle high, data is clocked in on rising, edge output data (change) on falling edge
#define SPI_MODE SPI_MODE_1
#define SPI_SPEED 500000
#define SPI_DELAY 0

int spi_open();

int spi_transfer(int fd, uint8_t *txbuf, uint8_t *rxbuf, uint32_t length);

#endif /* __SPIPI_H__ */