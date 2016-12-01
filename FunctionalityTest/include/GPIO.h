#include <iostream>
#include <fstream>

// Predefined Values
#define GPIO_PATH       "/sys/class/gpio/"
#define GPIO_OUTPUT     1
#define GPIO_INPUT      0
#define GPIO_HIGH       1
#define GPIO_LOW        0

// Generic GPIO Control Class
class GPIO {
public:
    int exportGPIO(int pin, bool condition);
    int setDirection(int pin, int dir);
    int setValue(int pin, int val);
    int readValue(int pin);

    int pinID[43] = {60, 50, 48, 51, 49, 15, 117, 14, 115, 113, 111, 112, 110, 20, 7,
        66, 67, 69, 68, 45, 44, 23, 26, 47, 46, 27, 65, 22, 79, 76, 77,
        86, 61, 88, 87, 89, 10, 11, 9, 81, 8, 80, 78};
    int numLED = 43;
};

// Analog-to-Digital Converter Code
int analogRead(int id);
