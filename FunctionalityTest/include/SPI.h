#ifndef SPI_H
#define SPI_H

class SPI {
private:
    const char *device = "/dev/spidev1.0";
    int fd;
    uint16_t delay;
    uint8_t bits = 8;
    uint32_t speed = 500000;
    uint8_t mode = 0;
public:
    int init(void);
    bool setSPIMode(uint8_t mode);
    bool setSPISpeed(uint32_t speed);
    bool setSPIBitLength(uint8_t bits);
    bool transfer(uint8_t *tx, uin8_t *rx, int length);
};

#endif
