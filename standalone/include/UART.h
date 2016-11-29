#include <iostream>
#include <cstdlib>
#include <string>

class UART {
private:
    int fid;                                // fid is the file handler for /dev/tty files
public:
    UART(int ID, int BAUDRATE);             // Constructer, take into ID (which UART bus) and BAUDRATE (speed in bps)
    ~UART(void);
    int peek(void);                         // retrieve number of bytes in the buffer
    int send(uint8_t *output, int len);     // send function
    int receive(int len);                   // receive function
    
    uint8_t input[96];                      // byte array containing data received from UART
};
