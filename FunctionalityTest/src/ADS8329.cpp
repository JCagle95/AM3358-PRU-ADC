#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <time.h>
#include <chrono>

#include "ADS8329.h"

using namespace std;

bool ADS8329::init (void) {
    unsigned int ret;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	prussdrv_init();
	ret = prussdrv_open(PRU_EVTOUT_0);
	if (ret) {
		printf("prussdrv_open failed\n");
        return false;
	}

    // Initialize Interrupt
	prussdrv_pruintc_init(&pruss_intc_initdata);

	// Initialize Memory Mapping for local memory
	prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, (void **) &this->PRU1_Memory);
	prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, (void **) &this->Shared_Memory);

    // Arguments to pass to PRU1.
    this->PRU1_Memory->RunFlag = 1;						// Runflag = 1 or 0, Run or Stop
    this->PRU1_Memory->MaxSize = ARRAY_SIZE * 2;		// Amount of memory to allocate
    this->PRU1_Memory->DataReady = 0;

    if (this->Timer.exportPWM(0, true) == -1) {
        printf("Timer Failed to Export\n");
        return false;
    }

    if (!this->setSamplingRate(100000)) {
        printf("Cannot set SamplingRate\n");
        return false;
    }

    if (!this->Timer.setMode(0, PWM_ENABLE) == -1) {
        printf("Cannot enable PWM\n");
        return false;
    }

    return true;
}

bool ADS8329::setSamplingRate (int rate) {
    int Period;
    if (rate > 0) Period = 1000000000 / rate;
    else return false;

    if (this->Timer.setPeriod(0, Period) == -1) {
        printf("Cannot set Period\n");
        return false;
    }

    if (this->Timer.getPeriod(0) != Period) {
        printf("Failed to set Period\n");
        return false;
    }

    int DutyCycle = Period * 0.9;
    if (this->Timer.setDutyCycle(0, DutyCycle) == -1) {
        printf("Cannot set Period\n");
        return false;
    }

    if (this->Timer.getDutyCycle(0) != DutyCycle) {
        printf("Failed to set Period\n");
        return false;
    }

    return true;
}

bool ADS8329::begin (chrono::high_resolution_clock::time_point start, int duration) {
    // File Description
    char str[80];
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
	sprintf(str, "/root/SD/Data/%d_%d_%d_%d_%d_%d.bin", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    ofstream PRUData (str, ofstream::out | ofstream::app);

    sprintf(str, "/root/SD/Data/%d_%d_%d_%d_%d_%d.txt", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    ofstream PRUTime (str, ofstream::out | ofstream::app);

    // Setup timers
    int blockCount = 0, pointer = 0;
    chrono::duration<float> elapsed_time;

    // Start the compiled Assembly code
    prussdrv_exec_program(PRU_NUM1, "/root/PRUTest/ADS8329.bin");

    do {
        // Check to see if the PRU data is ready
    	while (this->PRU1_Memory->DataReady == blockCount && this->PRU1_Memory->RunFlag) {
    		usleep(20000);
    	}
    	blockCount++;

    	// This determine which block of data to retrieve.
    	// PRU1 contain two data block, so we read the one is full while PRU is writing to another.
    	if (blockCount % 2 == 1){
    		memcpy(dataPoint + pointer, this->PRU1_Memory->Data, ARRAY_SIZE*2);
    		pointer += ARRAY_SIZE;
    	} else {
    		memcpy(dataPoint + pointer, this->Shared_Memory, ARRAY_SIZE*2);
    		pointer += ARRAY_SIZE;
    	}

    	// If the pointer indicated the allocated data array is full, we write to file
    	if (pointer == MAX_CYCLE * ARRAY_SIZE){
    		// Writing to the binary file all the data then reset the pointer
    		//PRUData.write(dataPoint, MAX_CYCLE*ARRAY_SIZE);
    		pointer = 0;

    		// Show the current data, just for debug purpose.
    		cout << "Current Data: " << dataPoint[MAX_CYCLE*ARRAY_SIZE-1] << endl;;
    	}

        elapsed_time = chrono::duration_cast<chrono::duration<float>>(chrono::high_resolution_clock::now() - start);
    } while (elapsed_time.count() < duration);

    this->PRU1_Memory->RunFlag = 0;

	PRUData.close();
    PRUTime.close();

	// Wait for PRU0 to halt
	prussdrv_pru_wait_event(PRU_EVTOUT_0);
	prussdrv_pru_clear_event(PRU1_ARM_INTERRUPT);

	// Safe Quit PRUs
	prussdrv_pru_disable(PRU_NUM1);
	prussdrv_exit();

    if (!this->Timer.setMode(0, PWM_DISABLE) == -1) {
        printf("Cannot Disable PWM\n");
        return false;
    }

    return true;
}
