#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "SPI.h"

bool SPI::setSPIMode (uint8_t mode) {
    this->mode = mode;
    int ret = ioctl(fd, SPI_IOC_WR_MODE, &this->mode);
    if (ret == -1) {
        printf("can't set spi mode\n");
        return false;
    } else {
        printf("Success setting spi mode\n");
        return true;
    }
}

bool SPI::setSPISpeed (uint32_t speed) {
    this->speed = speed;
    int ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &this->speed);
    if (ret == -1) {
        printf("can't set spi speed\n");
        return false;
    } else {
        printf("Success setting spi speed\n");
        return true;
    }
}

bool SPI::setSPIBitLength (uint8_t bits) {
    this->bits = bits;

    fd = open(this->device, O_RDWR);
    int ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &this->bits);
    close(fd);
    if (ret == -1) {
        printf("can't set bits per word\n");
        return false;
    } else {
        printf("Success setting bits per word\n");
        return true;
    }
}

int SPI::init (void) {
    int ret;

    fd = open(this->device, O_RDWR);
    if (fd < 0) {
        printf("can't open device\n");
        return 1;
    }

    ret = ioctl(fd, SPI_IOC_WR_MODE, &this->mode);
    if (ret == -1) {
        printf("can't set spi mode\n");
        return 1;
    }

    ret = ioctl(fd, SPI_IOC_RD_MODE, &this->mode);
    if (ret == -1) {
        printf("can't get spi mode\n");
        return 1;
    }

    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &this->bits);
    if (ret == -1) {
        printf("can't set bits per word\n");
        return 1;
    }

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &this->bits);
    if (ret == -1) {
        printf("can't get bits per word\n");
        return 1;
    }

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &this->speed);
    if (ret == -1) {
        printf("can't set max speed Hz\n");
        return 1;
    }

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &this->speed);
    if (ret == -1) {
        printf("can't get max speed Hz\n");
        return 1;
    }
}

bool SPI::transfer(void)
{
	int ret;
	uint8_t tx[5] = {0xF5,0x58,0x44,0x44,0x44};
	uint8_t rx[5] = {0,0,0,0,0};
	struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 5;
    tr.delay_usecs = this->delay;
    tr.speed_hz = this->speed;
    tr.bits_per_word = this->bits;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 0) {
        printf("Fail to send message\n");
        return false;
	}

	for (ret = 0; ret < 5; ret++) {
		if (rx[ret] != tx[ret]) {
		}
		printf("%.2X ", rx[ret]);
	}
	puts("");
    return true;
}
