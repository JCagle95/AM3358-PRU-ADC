#ifndef UART_H
#define UART_H

#include <iostream>
#include <stdarg.h>
#include <cstdlib>
#include <string>

class UART {
private:
    int fid;                                // fid is the file handler for /dev/tty files
public:
    ~UART(void);
    bool init(int ID, int BAUDRATE);
    int peek(void);                         // retrieve number of bytes in the buffer
    int send(uint8_t *output, int len);     // send function
    int receive(uint8_t *input, int len);                   // receive function
    int printf(const char *format, ...);
};

#endif
