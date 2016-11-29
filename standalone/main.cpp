#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "GPIO.h"
#include "UART.h"
#include "SPI.h"
#include "BMP180.h"
#include "MCP4725.h"

using namespace std;
typedef chrono::high_resolution_clock Time;
int DIVISION = 1;

struct ThreadParams {
    Time::time_point start;
    float RunTime_Sec;
};

void* GPIOTest(void *arg);
void* I2CTest(void *arg);
void* PRUTest(void *arg);
void* Stress(void *arg);
void SPITest(void);
void UARTTest(void);

int main(int argc, char *argv[]){

    time_t t;
    struct tm tm;
    uint16_t ADC = 0;

    UART Serial(4, 9600);
    char debug[80];

    ofstream MainLog;
    char filename[80];
    sprintf(filename, "/root/SD/Data/mainLog.txt");
    MainLog.open(filename, ofstream::out | ofstream::app);

    for (int i = 0; i < 1800 / DIVISION; i += 5) {
        ADC = analogRead(0);
        double volt = (double)ADC * 1.8 * (2156.0 + 504.2) / (504.2 * 4096.0);
        double temp = (volt - 0.5) * 100.0;
        sprintf(debug, "Current Time: %d/%d\t Temperature Reading: %.2f\n\r", i, 1800/DIVISION, temp);
        Serial.send((uint8_t *) debug, strlen(debug));

        t = time(NULL);
        tm = *localtime(&t);
        MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
        MainLog << "Reading: " << ADC << " of 4096: "<< temp << " C." << endl;
        sleep(5);
    }



    struct ThreadParams args;
    args.start = Time::now();
    args.RunTime_Sec = 1800 / DIVISION;

    t = time(NULL);
    tm = *localtime(&t);
    pthread_t I2C_tid, GPIO_tid, PRU_tid, Stress_tid;
    MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    MainLog << "I2C Start" << endl;
    sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t I2C Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    Serial.send((uint8_t *) debug, strlen(debug));
    pthread_create(&I2C_tid, NULL, I2CTest, (void *) &args);

    t = time(NULL);
    tm = *localtime(&t);
    MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    MainLog << "GPIO Start" << endl;
    sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t GPIO Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    Serial.send((uint8_t *) debug, strlen(debug));
    pthread_create(&GPIO_tid, NULL, GPIOTest, (void *) &args);

    t = time(NULL);
    tm = *localtime(&t);
    MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    MainLog << "PRU Start" << endl;
    sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t PRU Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    Serial.send((uint8_t *) debug, strlen(debug));
    pthread_create(&PRU_tid, NULL, PRUTest, (void *) &args);

    Time::time_point start = args.start;
    float RunTime = args.RunTime_Sec;

    while(1){
        ADC = analogRead(0);
        double volt = (double)ADC * 1.8 * (2156.0 + 504.2) / (504.2 * 4096.0);
        double temp = (volt - 0.5) * 100.0;

        t = time(NULL);
        tm = *localtime(&t);
        MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
        MainLog << "Reading: " << ADC << " of 4096: "<< temp << " C." << endl;
        sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t Temperature Reading: %.2f\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, temp);
        Serial.send((uint8_t *) debug, strlen(debug));
        cout << "Reading: " << ADC << " of 4096: "<< temp << " C." << endl;
        system("/root/FunctionalityTest/spi_test");

        MainLog << "Sending UART" << endl;
        char OUTPUT = 'A';
        Serial.send((uint8_t *) &OUTPUT, 1);
        int BytesAvailable = Serial.peek();
        if (BytesAvailable > 0) {
            MainLog << "Received Message from UART" << endl;
            Serial.receive(BytesAvailable);
            Serial.send(Serial.input, BytesAvailable);
        }

        Time::time_point now = Time::now();
        chrono::duration<float> elapsed_time = chrono::duration_cast<chrono::duration<float>>(now - start);
        cout << elapsed_time.count() << endl;
        if (elapsed_time.count() > RunTime) {
            break;
        }

        sleep(5);
    }
    pthread_join(I2C_tid, NULL);
    pthread_join(GPIO_tid, NULL);
    pthread_join(PRU_tid, NULL);

    for (int i = 0; i < 1800 / DIVISION; i += 5) {
        ADC = analogRead(0);
        double volt = (double)ADC * 1.8 * (2156.0 + 504.2) / (504.2 * 4096.0);
        double temp = (volt - 0.5) * 100.0;
        sprintf(debug, "Current Time: %d/%d\t Temperature Reading: %.2f\n\r", i, 1800/DIVISION, temp);
        Serial.send((uint8_t *) debug, strlen(debug));

        t = time(NULL);
        tm = *localtime(&t);
        MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
        MainLog << "Reading: " << ADC << " of 4096: "<< temp << " C." << endl;
        sleep(5);
    }

    t = time(NULL);
    tm = *localtime(&t);
    MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    MainLog << "Stress Start" << endl;
    args.start = Time::now();
    args.RunTime_Sec = 1800 / DIVISION;
    sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t Stress Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    Serial.send((uint8_t *) debug, strlen(debug));
    pthread_create(&Stress_tid, NULL, Stress, (void *) &args);
    pthread_join(Stress_tid, NULL);

    for (int i = 0; i < 1800 / DIVISION; i += 5) {
        ADC = analogRead(0);
        double volt = (double)ADC * 1.8 * (2156.0 + 504.2) / (504.2 * 4096.0);
        double temp = (volt - 0.5) * 100.0;
        sprintf(debug, "Current Time: %d/%d\t Temperature Reading: %.2f\n\r", i, 1800/DIVISION, temp);
        Serial.send((uint8_t *) debug, strlen(debug));

        t = time(NULL);
        tm = *localtime(&t);
        MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
        MainLog << "Reading: " << ADC << " of 4096: "<< temp << " C." << endl;
        sleep(5);
    }

    args.start = Time::now();
    args.RunTime_Sec = 4200 / DIVISION;

    t = time(NULL);
    tm = *localtime(&t);
    MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    MainLog << "I2C Start" << endl;
    sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t I2C Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    Serial.send((uint8_t *) debug, strlen(debug));
    pthread_create(&I2C_tid, NULL, I2CTest, (void *) &args);
    t = time(NULL);
    tm = *localtime(&t);
    MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    MainLog << "GPIO Start" << endl;
    sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t GPIO Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    Serial.send((uint8_t *) debug, strlen(debug));
    pthread_create(&GPIO_tid, NULL, GPIOTest, (void *) &args);
    t = time(NULL);
    tm = *localtime(&t);
    MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    MainLog << "PRU Start" << endl;
    sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t PRU Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    Serial.send((uint8_t *) debug, strlen(debug));
    pthread_create(&PRU_tid, NULL, PRUTest, (void *) &args);
    t = time(NULL);
    tm = *localtime(&t);
    MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
    MainLog << "Stress Start" << endl;
    sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t Stress Start\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    Serial.send((uint8_t *) debug, strlen(debug));
    pthread_create(&Stress_tid, NULL, Stress, (void *) &args);

    start = args.start;
    RunTime = args.RunTime_Sec;

    while(1){
        ADC = analogRead(0);
        double volt = (double)ADC * 1.8 * (2156.0 + 504.2) / (504.2 * 4096.0);
        double temp = (volt - 0.5) * 100.0;

        t = time(NULL);
        tm = *localtime(&t);
        MainLog << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
        MainLog << "Reading: " << ADC << " of 4096: "<< temp << " C." << endl;
        cout << "Reading: " << ADC << " of 4096: "<< temp << " C." << endl;
        sprintf(debug, "Current Time: %d-%d-%d %d:%d:%d\t Temperature Reading: %.2f\n\r", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, temp);
        Serial.send((uint8_t *) debug, strlen(debug));

        system("/root/FunctionalityTest/spi_test");

        MainLog << "Sending UART" << endl;
        char OUTPUT = 'A';
        Serial.send((uint8_t *) &OUTPUT, 1);
        int BytesAvailable = Serial.peek();
        if (BytesAvailable > 0) {
            MainLog << "Received Message from UART" << endl;
            Serial.receive(BytesAvailable);
            Serial.send(Serial.input, BytesAvailable);
        }

        Time::time_point now = Time::now();
        chrono::duration<float> elapsed_time = chrono::duration_cast<chrono::duration<float>>(now - start);
        cout << elapsed_time.count() << endl;
        if (elapsed_time.count() > RunTime) {
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

void* GPIOTest(void *arg){
    struct ThreadParams *input = (struct ThreadParams *) arg;
    Time::time_point start = input->start;
    float RunTime = input->RunTime_Sec;

    GPIO LED;
    for (int i = 0; i < LED.numLED; i++) {
        LED.exportGPIO(LED.pinID[i], true);
        usleep(10000);
        LED.setDirection(LED.pinID[i], GPIO_OUTPUT);
        usleep(10000);
    }

    while(1){
        for (int i = 0; i < LED.numLED; i++) {
            LED.setValue(LED.pinID[i], GPIO_HIGH);
            usleep(500000);

            LED.setValue(LED.pinID[i], GPIO_LOW);
            usleep(1000);
        }

        Time::time_point now = Time::now();
        chrono::duration<float> elapsed_time = chrono::duration_cast<chrono::duration<float>>(now - start);
        if (elapsed_time.count() > RunTime) {
            break;
        }
    }

    for (int i = 0; i < LED.numLED; i++) {
        LED.exportGPIO(LED.pinID[i], false);
    }

    return NULL;
}

void* PRUTest(void *arg){
    struct ThreadParams *input = (struct ThreadParams *) arg;
    Time::time_point start = input->start;
    int RunTime = input->RunTime_Sec;

    char CommandLine[80];
    sprintf(CommandLine, "/root/FunctionalityTest/ADS8329 %d", RunTime);
    system(CommandLine);

    return NULL;
}

void* I2CTest(void *arg){
    struct ThreadParams *input = (struct ThreadParams *) arg;
    Time::time_point start = input->start;
    float RunTime = input->RunTime_Sec;

    ofstream I2CRecord;
    char filename[80];
    sprintf(filename, "/root/SD/Data/I2CTest.txt");
    I2CRecord.open(filename, ofstream::out | ofstream::app);

	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    I2CRecord << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;

	float temperature, pressure;
	int bus_interface;
	if ((bus_interface = open("/dev/i2c-2", O_RDWR)) < 0){
		I2CRecord << "Failed to open the bus" << endl;
        return NULL;
	}

    BMP180 sensor(bus_interface,BMP085_MODE_ULTRAHIGHRES);
    MCP4725 DAC(bus_interface);

    while(1){
        t = time(NULL);
        tm = *localtime(&t);
        I2CRecord << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
        sensor.getTemperature(&temperature);
        sensor.getPressure(&pressure);
        I2CRecord << temperature << " C Degree\t" << pressure << " Pascal" << endl << endl;

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

void* Stress(void *arg) {
    struct ThreadParams *input = (struct ThreadParams *) arg;
    Time::time_point start = input->start;
    int RunTime = input->RunTime_Sec;

    char CommandLine[80];
    sprintf(CommandLine, "stress -c 1 -i 1 -t %d", RunTime);
    system(CommandLine);

    return NULL;
}

void SPITest(void){
    int ret = 0;
	int fd;
    static const char *device = "/dev/spidev1.0";
    static uint8_t mode = 0;
    static uint8_t bits = 8;
    static uint32_t speed = 500000;
    static uint16_t delay = 0;

    ofstream SPIRecord;
    char filename[80];
    sprintf(filename, "/root/SD/Data/SPITest.txt");
    SPIRecord.open(filename, ofstream::out | ofstream::app);

	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    SPIRecord << "Time-stamp: " << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;

	fd = open(device, O_RDWR);
	if (fd < 0) {
        SPIRecord << "can't open device" << endl;
        return;
    }

	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
        SPIRecord << "can't set spi mode" << endl;
        return;
    }

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) {
        SPIRecord << "can't get spi mode" << endl;
        return;
    }

	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
        SPIRecord << "can't set bits per word" << endl;
        return;
    }

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {
        SPIRecord << "can't get bits per word" << endl;
        return;
    }

	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
        SPIRecord << "can't set max speed hz" << endl;
        return;
    }

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
        SPIRecord << "can't get max speed hz" << endl;
    }

	SPIRecord << "spi mode: " << mode << endl;
	SPIRecord << "bits per word: " << bits << endl;
	SPIRecord << "spi mode: " << mode << endl;
	SPIRecord << "max speed: " << speed << " Hz" << endl;

    uint8_t tx[4] = { 0xFF, 0x00, 0xAB, 0xBE };
	uint8_t rx[4] = { 0x00, 0x00, 0x00, 0x00 };
	struct spi_ioc_transfer tr;
    tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = 4;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
        SPIRecord << "can't send spi message" << endl;
        return;
    }

    bool TestTrial = true;
	for (ret = 0; ret < 4; ret++) {
        if (rx[ret] != tx[ret]) {
            SPIRecord << "ID " << ret << " not matched" << endl;
            TestTrial = false;
        }
	}
    if (TestTrial) {
        SPIRecord << "Test Passed" << endl;
    }

	close(fd);
    SPIRecord.close();

    return;
}

void UARTTest(void) {
    UART Serial(4, 9600);
    char OUTPUT = 'A';
    Serial.send((uint8_t *) &OUTPUT, 1);

    int BytesAvailable = Serial.peek();
    if (BytesAvailable > 0) {
        Serial.receive(BytesAvailable);
        Serial.send(Serial.input, BytesAvailable);
    }

    return;
}
