#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <string.h>
#include <sstream>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "UART.h"

using namespace std;

/* Constructer */
UART::UART(int ID, int BAUDRATE) {

    // Initialize UART File Control
    char filename[80];
    sprintf(filename, "/dev/ttyO%d", ID);

    fid = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fid < 0) {
        perror("UART: Failed to Open");
        return;
    }
    fcntl(fid, F_SETFL, FNDELAY);

    // termios is the UART Configuration structure
    struct termios old_config, config;
    tcgetattr(fid, &old_config);        // Get Old Configuration
    bzero(&config, sizeof(config));     // Clear Configuration

    // Convert Int to Linux Baudrate value
    int Linux_BAUDRATE;
    switch (BAUDRATE) {
        case 1200:
            Linux_BAUDRATE = B1200;
            break;
        case 2400:
            Linux_BAUDRATE = B2400;
            break;
        case 4800:
            Linux_BAUDRATE = B4800;
            break;
        case 19200:
            Linux_BAUDRATE = B19200;
            break;
        case 38400:
            Linux_BAUDRATE = B38400;
            break;
        case 57600:
            Linux_BAUDRATE = B57600;
            break;
        case 115200:
            Linux_BAUDRATE = B115200;
            break;
        case 9600:
        default:
            Linux_BAUDRATE = B9600;
    }

    // Configuration: This is minimum setup.
    config.c_cflag = Linux_BAUDRATE | CS8 | CLOCAL | CREAD;
    config.c_iflag = 0;
    config.c_oflag = 0;
    config.c_lflag = 0;
    config.c_cc[VTIME] = 0;
    config.c_cc[VMIN] = 96;
    tcflush(fid, TCIFLUSH);
    tcsetattr(fid, TCSANOW, &config);
}

/* Close the file handler when destroying this object */
UART::~UART(void) {
    close(fid);
}

/*  Check for available bytes.
    Return bytes available in the UART buffer */
int UART::peek(void) {
    int BytesAvailable;
    ioctl(fid, FIONREAD, &BytesAvailable);
    return BytesAvailable;
}

/* Send Data through UART */
int UART::send(uint8_t *output, int len) {
    write(fid, output, len);
    return 0;
}

/*  Receive Data from UART, and stored in byte array "input"
    Return number of Bytes read  */
int UART::receive(int len) {
    return read(fid, input, len);
}

/* Use this function as if you are using printf function */
int UART::printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buf[96];
    vsprintf(buf, format, args);
    this->send((uint8_t *) buf, strlen(buf));
    return 0;
}
