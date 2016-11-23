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
};

// Analog-to-Digital Converter Code
int analogRead(int id);
