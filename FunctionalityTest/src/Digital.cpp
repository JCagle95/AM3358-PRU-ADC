#include "Digital.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>

using namespace std;

/*  This function will call GPIO Export file to activate pins in GPIO mode
    pin is the pinID.
    condition True = Export, condition False = Unexport */
int GPIO::exportGPIO (int pin, bool condition) {
    // Construct file string
    string path = GPIO_PATH;
    string file;
    if (condition) file = "export";
    else file = "unexport";

    // Open file
    ofstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()){
        perror("GPIO: Failed to export");
        return -1;
    }

    // Echo ID
    stringstream s;
    s << pin;
    fs << s.str();
    fs.close();
    return 0;
}

/*  This function set the GPIO Direction.
    pin is pinID.
    dir is GPIO_DIRECTION, either GPIO_INPUT or GPIO_OUTPUT */
int GPIO::setDirection (int pin, int dir) {
    // Construct file string
    ostringstream s;
    s << "gpio" << pin;
    string path = GPIO_PATH + string(s.str()) + "/";
    string file = "direction";

    // Open file
    ofstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()) {
        perror("GPIO: Failed to open direction file");
        return -1;
    }

    // Echo direction
    if (dir == 0) fs << "in";
    else fs << "out";
    fs.close();
    return 0;
}

/*  This function set the GPIO output.
    pin is pinID
    val is either GPIO_HIGH or GPIO_LOW */
int GPIO::setValue (int pin, int val) {
    // Construct file string
    ostringstream s;
    s << "gpio" << pin;
    string path = GPIO_PATH + string(s.str()) + "/";
    string file = "value";

    // Open file
    ofstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()) {
        perror("GPIO: Failed to open value file");
        return -1;
    }

    // Echo GPIO output value
    if (val == 0) fs << "0";
    else fs << "1";
    fs.close();
    return 0;
}

/*  This function read the GPIO state.
    pin is pinID
    return either GPIO_HIGH or GPIO_LOW */
int GPIO::readValue (int pin) {
    // Construct file string
    ostringstream s;
    s << "gpio" << pin;
    string path = GPIO_PATH + string(s.str()) + "/";
    string file = "value";

    // Open file
    ifstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()) {
        perror("GPIO: Failed to open reading file");
        return -1;
    }

    // Read the GPIO value
    string input;
    getline(fs, input);
    fs.close();
    if (input == "1") return GPIO_HIGH;
    return GPIO_LOW;
}

int PWM::exportPWM (int pwmID, bool condition) {
    string path = PWM_PATH;
    string file;
    if (condition) file = "export";
    else file = "unexport";

    // Open file
    ofstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()){
        perror("PWM: Failed to export");
        return -1;
    }

    // Echo ID
    stringstream s;
    s << pwmID;
    fs << s.str();
    fs.close();
    return 0;
}

int PWM::setPeriod (int pwmID, int Period) {
    // Construct file string
    ostringstream s;
    s << "pwm" << pwmID;
    string path = PWM_PATH + string(s.str()) + "/";
    string file = "period";

    // Open file
    ofstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()) {
        perror("PWM: Failed to open period file");
        return -1;
    }

    // Echo direction
    stringstream input;
    input << Period;
    fs << s.str();
    fs.close();
    return 0;
}

int PWM::setDutyCycle (int pwmID, int DutyCycle) {
    // Construct file string
    ostringstream s;
    s << "pwm" << pwmID;
    string path = PWM_PATH + string(s.str()) + "/";
    string file = "duty_cycle";

    // Open file
    ofstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()) {
        perror("PWM: Failed to open duty_cycle file");
        return -1;
    }

    // Echo direction
    stringstream input;
    input << DutyCycle;
    fs << s.str();
    fs.close();
    return 0;
}

int PWM::setMode (int pwmID, int enable) {
    // Construct file string
    ostringstream s;
    s << "pwm" << pwmID;
    string path = PWM_PATH + string(s.str()) + "/";
    string file = "enable";

    // Open file
    ofstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()) {
        perror("PWM: Failed to open enable file");
        return -1;
    }

    // Echo direction
    if (enable == PWM_ENABLE) fs << "1";
    else fs << "0";
    fs.close();
    return 0;
}

int PWM::getDutyCycle (int pwmID) {
    int ret = this->readConfig(pwmID, "duty_cycle");
    return ret;
}

int PWM::getPeriod (int pwmID) {
    int ret = this->readConfig(pwmID, "period");
    return ret;
}

int PWM::getMode (int pwmID) {
    int ret = this->readConfig(pwmID, "enable");
    return ret;
}

int PWM::readConfig (int pwmID, string file) {
    // Construct file string
    ostringstream s;
    s << "pwm" << pwmID;
    string path = PWM_PATH + string(s.str()) + "/";

    // Open file
    ifstream fs;
    fs.open((path + file).c_str());
    if (!fs.is_open()) {
        perror("PWM: Failed to open file");
        return -1;
    }

    // Echo direction
    string line;
    if (!getline(fs, line)) {
        perror("Cannot Read");
        return -1;
    }

    fs.close();
    return atoi(line.c_str());
}

// Analog-to-Digital Converter Function
int analogRead (int id) {
    // Open the ADC pins indicated by id, if failed: print error and return 0.
    char filename[80];
    sprintf(filename, "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw", id);
    int fid = open(filename, O_RDONLY);
    if (fid < 0) {
        perror("Analog-to-Digital Converter: Failed to Open");
        return 0;
    }

    // The input is actually ASCII, so maximumly 12-bit is 4095, takes up to 4 bytes to store
    char input[4];
    read(fid, &input, 4);
    close(fid);

    // return ASCII to Interger converted data
    return atoi(input);
}
