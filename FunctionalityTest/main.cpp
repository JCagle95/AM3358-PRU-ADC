#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <chrono>

#include <pthread.h>
#include <time.h>

#include "Digital.h"
#include "UART.h"
#include "ADS8329.h"
#include "BMP180.h"
#include "MCP4725.h"

using namespace std;
typedef chrono::high_resolution_clock Time;     // Typedef chrono to simplify useage.
int DIVISION = 1;                               // Division is variable to scale program duration

// ThreadParams contain variables to be passed to POSIX Thread
struct ThreadParams {
    Time::time_point start;
    float RunTime_Sec;
};

double readTemperature(void);
struct tm logTime(ofstream &file);
void* GPIOTest(void *arg);
void* I2CTest(void *arg);
void* PRUTest(void *arg);
void* Stress(void *arg);
void* SPITest(void *arg);

int main(int argc, char *argv[]){

    // Initializing the file handler for the main logging file.
    char debug[80];
    ofstream MainLog;
    sprintf(debug, "/root/SD/Data/mainLog.txt");
    MainLog.open(debug, ofstream::out | ofstream::app);

    // Initializing Thread ID
    pthread_t I2C_tid, GPIO_tid, PRU_tid, Stress_tid;

    // Initializing Required Variables
    double Temperature;
    struct tm tm;
    struct ThreadParams args;

    // Initializing UART Class
    UART Serial;
    Serial.init(4, 9600);

    // Initial Block Start - 30 minutes Idle with temperature logging through UART
    for (int i = 0; i < 1800 / DIVISION; i += 5) {

        // Read Temperature and Write to UART through printf
        Temperature = readTemperature();
        Serial.printf("Current Time: %d/%d\t Temperature Reading: %.2f\n\r", i, 1800/DIVISION, Temperature);

        // log current time-stamp and retrieve the time structure
        tm = logTime(MainLog);
        MainLog << "Reading: " << Temperature << " C." << endl;
        sleep(5);

    }

    // Second thread starting.
    // Initialize Thread Arguments.
    // Start is the current time, RunTime_Sec is the seconds to run each thread for
    args.start = Time::now();
    args.RunTime_Sec = 1800 / DIVISION;

    // Log time and start I2C Thread
    tm = logTime(MainLog);
    MainLog << "I2C Start" << endl;
    Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t I2C Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    pthread_create(&I2C_tid, NULL, I2CTest, (void *) &args);

    // Log time and start GPIO Thread
    tm = logTime(MainLog);
    MainLog << "GPIO Start" << endl;
    Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t GPIO Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    pthread_create(&GPIO_tid, NULL, GPIOTest, (void *) &args);

    // Log time and start PRU Thread
    tm = logTime(MainLog);
    MainLog << "PRU Start" << endl;
    Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t PRU Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    pthread_create(&PRU_tid, NULL, PRUTest, (void *) &args);

    while (true) {

        // Log time and send current time and temperature through UART
        tm = logTime(MainLog);
        MainLog << "Reading: " << Temperature << " C." << endl;
        Temperature = readTemperature();
        Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t Temperature Reading: %.2f\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, Temperature);

        // Run SPI_Test function to test SPI
        system("/root/FunctionalityTest/spi_test");

        // Check if current time surpassed RunTime_Sec limit
        chrono::duration<float> elapsed_time = chrono::duration_cast<chrono::duration<float>>(Time::now() - args.start);
        cout << elapsed_time.count() << endl;
        if (elapsed_time.count() > args.RunTime_Sec) {
            break;
        }

        sleep(5);
    }

    // Indicating that program will not continue unless all three device threads are terminated correctly
    pthread_join(I2C_tid, NULL);
    pthread_join(GPIO_tid, NULL);
    pthread_join(PRU_tid, NULL);

    // Third Block - Idle starting, same as first block
    for (int i = 0; i < 1800 / DIVISION; i += 5) {
        Temperature = readTemperature();
        Serial.printf("Current Time: %d/%d\t Temperature Reading: %.2f\n\r", i, 1800/DIVISION, Temperature);

        tm = logTime(MainLog);
        MainLog << "Reading: " << Temperature << " C." << endl;
        sleep(5);
    }

    // Fourth Block starting
    args.start = Time::now();
    args.RunTime_Sec = 1800 / DIVISION;

    // log time and start Stress on CPU
    tm = logTime(MainLog);
    MainLog << "Stress Start" << endl;
    Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t Stress Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    pthread_create(&Stress_tid, NULL, Stress, (void *) &args);

    while (true) {

        // Log time and send current time and temperature through UART
        tm = logTime(MainLog);
        MainLog << "Reading: " << Temperature << " C." << endl;
        Temperature = readTemperature();
        Serial.printf("Currently Stressing - Time: %d-%d-%d %d:%d:%d\t Temperature Reading: %.2f\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, Temperature);

        // Check if current time surpassed RunTime_Sec limit
        chrono::duration<float> elapsed_time = chrono::duration_cast<chrono::duration<float>>(Time::now() - args.start);
        cout << elapsed_time.count() << endl;
        if (elapsed_time.count() > args.RunTime_Sec) {
            break;
        }

        sleep(5);
    }

    // Wait till Stress terminated
    pthread_join(Stress_tid, NULL);

    // fifth block starting, Idle, the same as first block
    for (int i = 0; i < 1800 / DIVISION; i += 5) {
        Temperature = readTemperature();
        Serial.printf("Current Time: %d/%d\t Temperature Reading: %.2f\n\r", i, 1800/DIVISION, Temperature);

        tm = logTime(MainLog);
        MainLog << "Reading: " << Temperature << " C." << endl;
        sleep(5);
    }

    // Last block starting.
    args.start = Time::now();
    args.RunTime_Sec = 4200 / DIVISION;

    // Log time and start I2C Thread
    tm = logTime(MainLog);
    MainLog << "I2C Start" << endl;
    Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t I2C Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    pthread_create(&I2C_tid, NULL, I2CTest, (void *) &args);

    // Log time and start GPIO Thread
    tm = logTime(MainLog);
    MainLog << "GPIO Start" << endl;
    Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t GPIO Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    pthread_create(&GPIO_tid, NULL, GPIOTest, (void *) &args);

    // Log time and start PRU Thread
    tm = logTime(MainLog);
    MainLog << "PRU Start" << endl;
    Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t PRU Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    pthread_create(&PRU_tid, NULL, PRUTest, (void *) &args);

    // Log time and start Stress Thread
    tm = logTime(MainLog);
    MainLog << "Stress Start" << endl;
    Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t Stress Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    pthread_create(&Stress_tid, NULL, Stress, (void *) &args);

    while (true) {

        // Log time and Transmit current temperature
        tm = logTime(MainLog);
        MainLog << "Reading: " << Temperature << " C." << endl;
        Temperature = readTemperature();
        Serial.printf("Current Time: %d-%d-%d %d:%d:%d\t Temperature Reading: %.2f\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, Temperature);

        // Test SPI
        system("/root/FunctionalityTest/spi_test");

        chrono::duration<float> elapsed_time = chrono::duration_cast<chrono::duration<float>>(Time::now() - args.start);
        cout << elapsed_time.count() << endl;
        if (elapsed_time.count() > args.RunTime_Sec) {
            break;
        }

        sleep(5);
    }

    pthread_join(I2C_tid, NULL);
    pthread_join(GPIO_tid, NULL);
    pthread_join(PRU_tid, NULL);
    pthread_join(Stress_tid, NULL);

    MainLog.close();

    return 0;
}

// read temperature from AIN ports.
double readTemperature(void){
    // analogRead please reference GPIO.h
    uint16_t ADC = analogRead(0);

    // temperature convertion for TMP36. Refer to datasheet on Adafruit.com
    double temp = (double)ADC * 1.8 * 100.0 * (2156.0 + 504.2) / (504.2 * 4096.0) - 500.0;
    return temp;
}

// Simplified code for writing current time to the file handler passed through arguments
struct tm logTime(ofstream &file){
    // Time Acquisition please refer Time.h
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    file << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    return tm;
}

// GPIO Thread. This thread will continuously flashing LEDs in the order indicated in GPIO.h
void* GPIOTest(void *arg){
    // GPIO Pins Definition
    int pinID[43] = {60, 50, 48, 51, 49, 15, 117, 14, 115, 113, 111, 112, 110, 20, 7,
        66, 67, 69, 68, 45, 44, 23, 26, 47, 46, 27, 65, 22, 79, 76, 77,
        86, 61, 88, 87, 89, 10, 11, 9, 81, 8, 80, 78};
    int numLED = 43;

    // Retrieve the input arguments
    struct ThreadParams *input = (struct ThreadParams *) arg;
    Time::time_point start = input->start;
    float RunTime = input->RunTime_Sec;

    // Initialize GPIO class for control. please refer to GPIO.h
    GPIO LED;

    // First, export all GPIO as output and pulled low.
    for (int i = 0; i < numLED; i++) {
        LED.exportGPIO(pinID[i], true);
        usleep(10000);
        LED.setDirection(pinID[i], GPIO_OUTPUT);
        usleep(10000);
    }

    while (true) {
        // Loop through the LEDs, toggle high for 500ms and low. 100ms interval between lightup of each LEDs
        for (int i = 0; i < numLED; i++) {
            LED.setValue(pinID[i], GPIO_HIGH);
            usleep(500000);
            LED.setValue(pinID[i], GPIO_LOW);
            usleep(1000);
        }

        // Wait till the RunTime complete. RunTime is coming from the thread arguments input from main.
        Time::time_point now = Time::now();
        chrono::duration<float> elapsed_time = chrono::duration_cast<chrono::duration<float>>(now - start);
        if (elapsed_time.count() > RunTime) {
            break;
        }
    }

    // When complete, unexport all GPIOs
    for (int i = 0; i < numLED; i++) {
        LED.exportGPIO(pinID[i], false);
    }

    return NULL;
}

// PRU Thread, this will called the C-program for PRU
void* PRUTest(void *arg){
    struct ThreadParams *input = (struct ThreadParams *) arg;
    Time::time_point start = input->start;
    int RunTime = input->RunTime_Sec;

    ADS8329 ADC;
    if (!ADC.init()) {
        printf("Error, cannot start PRU\n");
        return NULL;
    }

    ADC.begin(start, RunTime);

    return NULL;
}

// I2C Thread, this will log the temperature and pressure every seconds through BMP180.
// I2C Thread, this will also use MCP4725 as Digital-to-Analog Converter to deliver signal for PRU.
void* I2CTest(void *arg){
    struct ThreadParams *input = (struct ThreadParams *) arg;
    Time::time_point start = input->start;
    float RunTime = input->RunTime_Sec;

    // Start the file string for I2C Test Log, open as write and append.
    ofstream I2CRecord;
    char filename[80];
    sprintf(filename, "/root/SD/Data/I2CTest.txt");
    I2CRecord.open(filename, ofstream::out | ofstream::app);

    // Time at which I2C Test Started
    struct tm tm = logTime(I2CRecord);

    // Initializing variables and I2C Bus
	float temperature, pressure;
	int bus_interface;
	if ((bus_interface = open("/dev/i2c-2", O_RDWR)) < 0){
        // If I2C Bus failed to initialize, record the error and terminate the thread.
		I2CRecord << "Failed to open the bus" << endl;
        return NULL;
	}

    // Initialize the BMP180 sensor and MCP4725 DAC.
    BMP180 sensor(bus_interface);
    MCP4725 DAC(bus_interface);

    bool sensorState = sensor.init(BMP085_MODE_ULTRAHIGHRES);
    if (!sensorState) {

    }

    while (true) {

        // Log time again
        tm = logTime(I2CRecord);

        if (sensor.sensorState) {
            // Do BMP180 computation only if sensor is succesfully initialized.
            // Please refer to BMP180.h for details
            sensor.getTemperature(&temperature);
            sensor.getPressure(&pressure);
            I2CRecord << temperature << " C Degree\t" << pressure << " Pascal" << endl << endl;
        } else {
            // that I2C is failed. Loged.
            I2CRecord << "I2C Failed in Initialization" << endl;
        }

        // Flipping DAC high and low. Please refer to MCP4725.h for details
        // This is 1Hz signal with 50% duty cycles.
        DAC.setVoltage(4095 ,false);
        usleep(500000);
        DAC.setVoltage(0 ,false);
        usleep(500000);

        Time::time_point now = Time::now();
        chrono::duration<float> elapsed_time = chrono::duration_cast<chrono::duration<float>>(now - start);
        if (elapsed_time.count() > RunTime) {
            break;
        }
    }

    I2CRecord.close();
    return NULL;
}

// Stress test thread. Implemented with only CPU 1 worker and IO 1 worker.
void* Stress(void *arg) {
    struct ThreadParams *input = (struct ThreadParams *) arg;
    Time::time_point start = input->start;
    int RunTime = input->RunTime_Sec;

    char CommandLine[80];
    sprintf(CommandLine, "stress -c 1 -i 1 -t %d", RunTime);
    system(CommandLine);

    return NULL;
}
