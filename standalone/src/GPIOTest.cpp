#include "GPIOTest.h"
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

/* Constructer for GPIO Test*/
GPIOTest::GPIOTest(void){

    GPIO_fid.open("/root/SD/Data/GPIOtest.txt");

    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 2; j++) {
            exportGPIO(pinID[i][j], true);
        }
    }
}

GPIOTest::~GPIOTest(void){
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 2; j++) {
            exportGPIO(pinID[i][j], false);
        }
    }

    GPIO_fid.close();

}

void GPIOTest::TogglePin(bool direction){

    if (direction) {
        for (int i = 0; i < 32; i++) {
            setDirection(pinID[i][0], GPIO_OUTPUT);
            setDirection(pinID[i][1], GPIO_INPUT);
        }
        usleep(100000);

        GPIO_fid << "Toggle LED Test: No." << counter++ << endl;
        GPIO_fid << "Set Role 0 as Output, Role 1 as Input. Flip High" << endl;
        for (int i = 0; i < 32; i++) {
            setValue(pinID[i][0], GPIO_HIGH);
        }
        for (int i = 0; i < 32; i++) {
            if (readValue(pinID[i][1]) == GPIO_LOW){
                GPIO_fid << "Error, Pin" << pinID[i][1] << " reading wrong value" << endl;
            }
        }

        GPIO_fid << "Flip Low" << endl;
        for (int i = 0; i < 32; i++) {
            setValue(pinID[i][0], GPIO_LOW);
        }
        for (int i = 0; i < 32; i++) {
            if (readValue(pinID[i][1]) == GPIO_HIGH){
                GPIO_fid << "Error, Pin" << pinID[i][1] << " reading wrong value" << endl;
            }
        }
    } else {
        for (int i = 0; i < 32; i++) {
            setDirection(pinID[i][1], GPIO_OUTPUT);
            setDirection(pinID[i][0], GPIO_INPUT);
        }
        usleep(100000);

        GPIO_fid << "Toggle LED Test: No." << counter++ << endl;
        GPIO_fid << "Set Role 1 as Output, Role 0 as Input. Flip High" << endl;
        for (int i = 0; i < 32; i++) {
            setValue(pinID[i][1], GPIO_HIGH);
        }
        for (int i = 0; i < 32; i++) {
            if (readValue(pinID[i][0]) == GPIO_LOW){
                GPIO_fid << "Error, Pin" << pinID[i][1] << " reading wrong value" << endl;
            }
        }

        GPIO_fid << "Flip Low" << endl;
        for (int i = 0; i < 32; i++) {
            setValue(pinID[i][1], GPIO_LOW);
        }
        for (int i = 0; i < 32; i++) {
            if (readValue(pinID[i][0]) == GPIO_HIGH){
                GPIO_fid << "Error, Pin" << pinID[i][1] << " reading wrong value" << endl;
            }
        }
    }

    GPIO_fid << endl << endl;
}

void GPIOTest::FlashLEDs(bool direction){
    if (direction) {
        for (int i = 0; i < 32; i++) {
            setDirection(pinID[i][0], GPIO_OUTPUT);
            setDirection(pinID[i][1], GPIO_INPUT);
        }
        usleep(100000);

        GPIO_fid << "Flash LED Test: No." << counter++ << endl;
        GPIO_fid << "Set Role 0 as Output, Role 1 as Input. Flash" << endl;
        for (int i = 0; i < 32; i++) {
            setValue(pinID[i][0], GPIO_LOW);
        }
        for (int i = 0; i < 32; i++) {
            setValue(pinID[i][0], GPIO_HIGH);
            usleep(1000);
            if (readValue(pinID[i][1]) == GPIO_LOW){
                GPIO_fid << "Error, Pin" << pinID[i][1] << " reading wrong value" << endl;
            }
            setValue(pinID[i][0], GPIO_LOW);
            usleep(1000);
            if (readValue(pinID[i][1]) == GPIO_HIGH){
                GPIO_fid << "Error, Pin" << pinID[i][1] << " reading wrong value" << endl;
            }
        }
    } else {
        for (int i = 0; i < 32; i++) {
            setDirection(pinID[i][1], GPIO_OUTPUT);
            setDirection(pinID[i][0], GPIO_INPUT);
        }
        usleep(100000);

        GPIO_fid << "Flash LED Test: No." << counter++ << endl;
        GPIO_fid << "Set Role 1 as Output, Role 0 as Input. Flash" << endl;
        for (int i = 0; i < 32; i++) {
            setValue(pinID[i][1], GPIO_LOW);
        }
        for (int i = 0; i < 32; i++) {
            setValue(pinID[i][1], GPIO_HIGH);
            usleep(1000);
            if (readValue(pinID[i][0]) == GPIO_LOW){
                GPIO_fid << "Error, Pin" << pinID[i][0] << " reading wrong value" << endl;
            }
            setValue(pinID[i][1], GPIO_LOW);
            usleep(1000);
            if (readValue(pinID[i][0]) == GPIO_HIGH){
                GPIO_fid << "Error, Pin" << pinID[i][0] << " reading wrong value" << endl;
            }
        }
    }

    GPIO_fid << endl << endl;
}
