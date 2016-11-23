#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>

#include "GPIOTest.h"
#include "UART.h"
#include "BMP180.h"

using namespace std;

void* PRUTest(void *arg){
    system("/root/SD/FunctionalityTest/ADS8329 24000");
}

void* ADCTest(void *arg){
    uint16_t ADC;
    for (int i = 0; i < 10; i++){
        ADC = analogRead(0);
        cout << "Reading: " << ADC << endl;
        usleep(1000000);
    }
    return NULL;
}

void* I2CTest(void *arg){
	float temperature, pressure;
	int bus_interface;

	if ((bus_interface = open("/dev/i2c-2", O_RDWR)) < 0){
		printf("Failed to open the bus\n");
        return NULL;
	}

    BMP180 sensor(bus_interface,BMP085_MODE_ULTRAHIGHRES);
    typedef chrono::high_resolution_clock Time;
    auto start = Time::now();

    for (int i = 0; i < 10; i++){
        sensor.getTemperature(&temperature);
        sensor.getPressure(&pressure);
        cout << temperature << " C Degree\t" << pressure << " Pascal" << endl;
        /*
        auto now = Time::now();
        std::chrono::duration<float> elapsed_time = now - start;
        if (elapsed_time.count() > 10) {
            break;
        }
        */
        usleep(1000000);
    }

    return NULL;
}

int main(void){
    //
    /*
    GPIOTest GPIO;
    for (int i = 0; i < 30; i++){
        GPIO.TogglePin(true);
        usleep(500000);
        GPIO.TogglePin(false);
        usleep(500000);
    }
    */
    /*
    UART Serial(4, 9600);
    clock_t start_t, elapsed_t ;
	start_t = clock();
    while (true) {
        int BytesAvailable = Serial.peek();
        if (BytesAvailable > 0) {
            Serial.receive(BytesAvailable);
            Serial.send(Serial.input, BytesAvailable);
        }
        elapsed_t = (double)(clock() - start_t) / CLOCKS_PER_SEC ;
        if (elapsed_t > 50) {
            break;
        }
    }
    */

    pthread_t I2C_tid, ADC_tid, PRU_tid;
    pthread_create(&I2C_tid, NULL, I2CTest, NULL);
    pthread_create(&ADC_tid, NULL, ADCTest, NULL);
    pthread_create(&PRU_tid, NULL, PRUTest, NULL);
    pthread_join(I2C_tid, NULL);
    pthread_join(ADC_tid, NULL);
    pthread_join(PRU_tid, NULL);
    return 0;
}
