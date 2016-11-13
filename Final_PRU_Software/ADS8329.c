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

#include "hdf5.h"

#define AM33XX
#define PRU_NUM0	0
#define PRU_NUM1	1
#define ARRAY_SIZE	2000
#define MAX_CYCLE	20

typedef struct {
	uint32_t RunFlag;
	uint32_t Delay;
	uint32_t MaxSize;
	uint32_t DataReady;
	uint16_t Data[ARRAY_SIZE];
} pru_data;

typedef struct {
	char title[40];
} thread_args;

pru_data *pru_input;
int thread_running = 0;
int RunCount = 0;

uint64_t getCurrentUTC(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long millisecondsSinceEpoch =
	    (unsigned long long)(tv.tv_sec) * 1000 +
	    (unsigned long long)(tv.tv_usec) / 1000;
	return millisecondsSinceEpoch;
}

void readCurrentTime(thread_args *args){
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
	sprintf(args->title, "/root/Data/%d_%d_%d_%d_%d_%d.hdf5", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void waitInitTime(uint32_t goalTime){
	time_t currentTime = time(NULL);
	localtime(&currentTime);
	printf("Current Time: %d\n", currentTime);
	printf("Target Time: %d\n", goalTime);
	printf("Time to Wait: %d\n", (goalTime - currentTime));
	while (currentTime < goalTime){
		printf("Time to Wait: %d\n", (goalTime - currentTime));
		usleep((goalTime - currentTime) * 1000000);
		localtime(&currentTime);
	}
}

void* readInput(void *arg) {

	thread_args *input = arg;
	unsigned int ret;
	void *pru_memory;
	void *shared_memory;
	uint16_t *shared_data;
	uint64_t timer[100][10];

	// Initialize PRU0
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

	pru_input->RunFlag = 1;
	pru_input->MaxSize = ARRAY_SIZE * 2;
	pru_input->DataReady = 0;
	pru_input->Delay = 551;

	prussdrv_exec_program(PRU_NUM1, "/root/Final/ADS8329.bin");

	int blockCount = 0, pointer = 0, datasetCounter = 0, pageCounter = 0;
	uint16_t dataPoint[MAX_CYCLE][ARRAY_SIZE];

	hid_t file, ADC_dataspace, Timer_dataspace, plist_id;
	hsize_t dimsf[2] = {MAX_CYCLE,ARRAY_SIZE};
	hsize_t cdims[2] = {10, 100};
	herr_t status;
	file = H5Fcreate(input->title, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	plist_id  = H5Pcreate (H5P_DATASET_CREATE);
	status = H5Pset_chunk (plist_id, 2, cdims);
	status = H5Pset_deflate (plist_id, 6);
	ADC_dataspace = H5Screate_simple(2, dimsf, NULL);

	while (pru_input->RunFlag) {

		while (pru_input->DataReady == blockCount && pru_input->RunFlag) {}
		blockCount++;

		if (blockCount % 2 == 1){
			memcpy(dataPoint[pointer++], pru_input->Data, ARRAY_SIZE*2);
		} else {
			memcpy(dataPoint[pointer++], shared_data, ARRAY_SIZE*2);
		}

		if (pointer==MAX_CYCLE){
			timer[pageCounter][datasetCounter%10] = getCurrentUTC();
			if (datasetCounter % 10 == 9)
				pageCounter++;
			hid_t dataset;
			char str[80];
			sprintf(str, "Data%d", datasetCounter++);
			dataset = H5Dcreate2(file, str, H5T_STD_U16LE, ADC_dataspace, H5P_DEFAULT, plist_id, H5P_DEFAULT);
			status = H5Dwrite(dataset, H5T_STD_U16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataPoint);
			H5Dclose(dataset);
			pointer = 0;
		}

		fflush(stdout);
	}

	hid_t dataset;
	dimsf[0] = 100;
	dimsf[1] = 10;
	Timer_dataspace = H5Screate_simple(2, dimsf, NULL);
	dataset = H5Dcreate2(file, "Timer", H5T_STD_U64LE, Timer_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	status = H5Dwrite(dataset, H5T_STD_U64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &timer);
	H5Dclose(dataset);

	printf("File Written Complete\n");

	H5Sclose(Timer_dataspace);
	H5Sclose(ADC_dataspace);
    H5Fclose(file);

	// Wait for PRU0 to halt
	prussdrv_pru_wait_event(PRU_EVTOUT_0);
	prussdrv_pru_clear_event(PRU1_ARM_INTERRUPT);

	// Safe Quit PRUs
	prussdrv_pru_disable(PRU_NUM1);
	prussdrv_exit();

	thread_running = 0;
	return NULL;
}

int main(void) {
	pthread_t tid;
	thread_args *args = malloc(sizeof *args);
	//uint32_t targetTime = 1478783400;
	//waitInitTime(targetTime);
	readCurrentTime(args);
    pthread_create(&tid, NULL, &readInput, args);

    clock_t start_t, elapsed_t ;
    start_t = clock();
	while (1) {
        elapsed_t = (double)(clock() - start_t) / CLOCKS_PER_SEC ;
		if (elapsed_t > 10) {
            pru_input->RunFlag = 0;
			break;
		}
    }

	// Wait for Thread to safely exit
	while (thread_running) {}
	printf("Finished Execution\n\n");

	return (0);
}
