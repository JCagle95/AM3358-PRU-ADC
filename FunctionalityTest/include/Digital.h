#ifndef DIGITAL_H
#define DIGITAL_H

#include <iostream>
#include <fstream>
#include <string>
#include <iostream>

// Predefined Values
#define GPIO_PATH       "/sys/class/gpio/"
#define PWM_PATH        "/sys/class/pwm/pwmchip0/"
#define GPIO_OUTPUT     1
#define GPIO_INPUT      0
#define GPIO_HIGH       1
#define GPIO_LOW        0
#define PWM_ENABLE      1
#define PWM_DISABLE     0

// Generic GPIO Control Class
class GPIO {
public:
    int exportGPIO(int pin, bool condition);
    int setDirection(int pin, int dir);
    int setValue(int pin, int val);
    int readValue(int pin);
};

class PWM {
private:
    int readConfig(int pwmID, std::string file);
public:
    int exportPWM(int pwmID, bool condition);
    int setPeriod(int pwmID, int Period);
    int setDutyCycle(int pwmID, int DutyCycle);
    int setMode(int pwmID, int enable);
    int getPeriod(int pwmID);
    int getDutyCycle(int pwmID);
    int getMode(int pwmID);
};

// Analog-to-Digital Converter Code
int analogRead(int id);
#endif
