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

void sleepms(int ms) {
	nanosleep((struct timespec[]) { {0, ms * 1000000} }, NULL);
}

typedef struct {
	uint32_t RunFlag;
	uint32_t Delay;
	uint32_t MaxSize;
	uint32_t DataReady;
	uint16_t Data[ARRAY_SIZE];
} pru_data;

pru_data *pru_input;
int thread_running = 0;
int RunCount = 0;

void* readInput(void *arg) {

	unsigned int ret;
	void *pru_memory;
	void *shared_memory;

	uint16_t *block1_data;
	uint16_t *block2_data;

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
	block2_data = shared_memory;

	pru_input->RunFlag = 1;
	pru_input->MaxSize = ARRAY_SIZE * 2;
	pru_input->DataReady = 0;
	pru_input->Delay = 551;

	printf("Let's See!");
	prussdrv_exec_program(PRU_NUM1, "/root/ADS8329/PRU-ADS8329.bin");

	int blockCount = 0, pointer = 0;
	uint16_t dataPoint[20][ARRAY_SIZE];
	//uint16_t *dataPoint = malloc(100000 * 2);
	//FILE *pFile;
	char str[80];
	sprintf(str, "Data%d.bin", RunCount);
	//pFile = fopen(str,"wb");

	hid_t file, datatype, dataspace, plist_id;
	hsize_t dimsf[2], cdims[2];
	cdims[0] = 10;
    cdims[1] = 100;
	herr_t status;
	file = H5Fcreate(str, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	dimsf[0] = 20;
	dimsf[1] = ARRAY_SIZE;
	plist_id  = H5Pcreate (H5P_DATASET_CREATE);
	status = H5Pset_chunk (plist_id, 2, cdims);
	status = H5Pset_deflate (plist_id, 6);
	dataspace = H5Screate_simple(2, dimsf, NULL);
	datatype = H5Tcopy(H5T_STD_U16LE);
	//status = H5Tset_order(datatype, H5T_ORDER_LE);

	while (pru_input->RunFlag) {

		while (pru_input->DataReady == blockCount && pru_input->RunFlag){
		}
		blockCount++;

		if (blockCount % 2 == 1){
			memcpy(dataPoint[pointer], pru_input->Data, ARRAY_SIZE*2);
			//printf("%f\n",(float)dataPoint[pointer][0]/65535.0*5.0);
			pointer += 1;
		} else {
			memcpy(dataPoint[pointer], block2_data, ARRAY_SIZE*2);
			//printf("%f\n",(float)dataPoint[pointer][0]/65535.0*5.0);
			pointer += 1;
		}

		if (pointer==20){
			printf("Hello World!\n");
			hid_t dataset;
			char str[80];
			sprintf(str, "Data%d", blockCount);
			dataset = H5Dcreate2(file, str, datatype, dataspace,
					H5P_DEFAULT, plist_id, H5P_DEFAULT);
			status = H5Dwrite(dataset, H5T_STD_U16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataPoint);
			H5Dclose(dataset);
			pointer = 0;
		}

		fflush(stdout);
	}

	if (blockCount * ARRAY_SIZE < 100000){
		//fwrite(dataPoint,2,100000,pFile);
	}

	printf("File Written Complete\n");

	H5Sclose(dataspace);
    H5Tclose(datatype);
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
	int taskContinue = 1;
	int input;

    thread_running = 1;
    RunCount++;
    pthread_create(&tid, NULL, &readInput, NULL);

    clock_t start_t, elapsed_t ;
    start_t = clock();
	while (taskContinue) {
        elapsed_t = (double)(clock() - start_t) / CLOCKS_PER_SEC ;
		if (elapsed_t > 10) {
            pru_input->RunFlag = 0;
			break;
		}
    }

	while (thread_running) {}

	printf("Finished Execution\n\n");

	return (0);
}
