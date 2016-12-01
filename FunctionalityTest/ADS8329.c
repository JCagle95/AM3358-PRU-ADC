#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#include "prussdrv.h"
#include <pruss_intc_mapping.h>

#define AM33XX
#define PRU_NUM0	0
#define PRU_NUM1	1
#define ARRAY_SIZE	4000
#define MAX_CYCLE	50

// Struct design for PRU data sharing. This matches the beginning of ADS8329.p
typedef struct {
	uint32_t RunFlag;
	uint32_t Delay;
	uint32_t MaxSize;
	uint32_t DataReady;
	uint16_t Data[ARRAY_SIZE];
} pru_data;

uint32_t getTime(void);

int main(int argc, char *argv[]) {

	// Receive the first argument in Command Line Interface. This is the time to run PRU
	int TimeLimit = atoi(argv[1]);
	printf("The Current Timelimit is %d sec.\n", TimeLimit);
	unsigned int ret;

	// Intialize pointers to hold the PRU data
	void *pru_memory;
	void *shared_memory;
	uint16_t *shared_data;
	pru_data *pru_input;

	// Initialize PRU1
	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	prussdrv_init();
	ret = prussdrv_open(PRU_EVTOUT_0);
	if (ret) {
		printf("prussdrv_open failed\n");
	}

	// Initialize Interrupt
	prussdrv_pruintc_init(&pruss_intc_initdata);

	// Initialize Memory Mapping for local memory
	prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, (void *)&pru_memory);
	prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, (void *)&shared_memory);
	pru_input = pru_memory;
	shared_data = shared_memory;

	// Arguments to pass to PRU1.
	pru_input->RunFlag = 1;						// Runflag = 1 or 0, Run or Stop
	pru_input->MaxSize = ARRAY_SIZE * 2;		// Amount of memory to allocate
	pru_input->DataReady = 0;
	pru_input->Delay = 555;

	// Start the compiled Assembly code
	prussdrv_exec_program(PRU_NUM1, "/root/FunctionalityTest/ADS8329.bin");

	// Initializing the data array to store all the data before writing to a file
	int blockCount = 0, pointer = 0;
	uint16_t dataPoint[ARRAY_SIZE*MAX_CYCLE];

	// Initialize the PRU binary file handler (.bin)
	char str[80];
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
	sprintf(str, "/root/SD/Data/%d_%d_%d_%d_%d_%d.bin", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	FILE *pFile;
	pFile = fopen(str, "wb");

	// Initialize the PRU time file handler (.txt)
	sprintf(str, "/root/SD/Data/%d_%d_%d_%d_%d_%d.txt", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	FILE *logFile;
	logFile = fopen(str, "w");

	// Initialize the timer
	uint32_t start, now;
	start = getTime();

	while (pru_input->RunFlag) {

		// Check to see if the PRU data is ready
		while (pru_input->DataReady == blockCount && pru_input->RunFlag) {
			usleep(20000);
		}
		blockCount++;

		// This determine which block of data to retrieve.
		// PRU1 contain two data block, so we read the one is full while PRU is writing to another.
		if (blockCount % 2 == 1){
			memcpy(dataPoint + pointer, pru_input->Data, ARRAY_SIZE*2);
			pointer += ARRAY_SIZE;
		} else {
			memcpy(dataPoint + pointer, shared_data, ARRAY_SIZE*2);
			pointer += ARRAY_SIZE;
		}

		// If the pointer indicated the allocated data array is full, we write to file
		if (pointer==MAX_CYCLE*ARRAY_SIZE){
			// Writing to the binary file all the data then reset the pointer
			fwrite(dataPoint,2,MAX_CYCLE*ARRAY_SIZE,pFile);
			pointer = 0;

			// Logging the current time of execution (in milliseconds);
			fprintf(logFile, "Current Time: %.2f / %d sec\n", (now - start)/1000.0, TimeLimit);

			// Show the current data, just for debug purpose.
			printf("Current Data: %d\n", dataPoint[MAX_CYCLE*ARRAY_SIZE-1]);
		}

		// Check to see if the program already finished executing the desinated time
		now = getTime();
		if (now - start > TimeLimit * 1000) {
			fprintf(logFile, "Current Time: %.4f / %d sec\n", (now - start)/1000.0, TimeLimit);
			printf("Stopped!\n");
			pru_input->RunFlag = 0;
			fclose(pFile);
			fclose(logFile);
		}

		fflush(stdout);
	}

	printf("File Written Complete\n");

	// Wait for PRU0 to halt
	prussdrv_pru_wait_event(PRU_EVTOUT_0);
	prussdrv_pru_clear_event(PRU1_ARM_INTERRUPT);

	// Safe Quit PRUs
	prussdrv_pru_disable(PRU_NUM1);
	prussdrv_exit();
	printf("Finished Execution\n\n");

	return (0);
}

// This method will use Time.h functions to acquire amount of time that this program had run in milliseconds.
uint32_t getTime (void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint32_t milliseconds = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
	return milliseconds;
}
