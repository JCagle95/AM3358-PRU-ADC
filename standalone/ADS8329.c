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
#define ARRAY_SIZE	5000
#define MAX_CYCLE	50

typedef struct {
	uint32_t RunFlag;
	uint32_t Delay;
	uint32_t MaxSize;
	uint32_t DataReady;
	uint16_t Data[ARRAY_SIZE];
} pru_data;

uint32_t getTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000 + tv.tv_usec);
}

pru_data *pru_input;

int main(int argc, char *argv[]) {

	int TimeLimit = atoi(argv[1]);
	printf("The Current Timelimit is %d sec.\n", TimeLimit);
	unsigned int ret;
	void *pru_memory;
	void *shared_memory;
	uint16_t *shared_data;

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
	pru_input->Delay = 555;

	prussdrv_exec_program(PRU_NUM1, "/root/PRU/ADS8329.bin");
	int blockCount = 0, pointer = 0;
	uint16_t dataPoint[ARRAY_SIZE*MAX_CYCLE];
	char str[80];
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
	sprintf(str, "/root/SD/Data/%d_%d_%d_%d_%d_%d.bin", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	FILE *pFile;

	uint32_t start, now;
	start = getTime();
	while (pru_input->RunFlag) {

		while (pru_input->DataReady == blockCount && pru_input->RunFlag) {
			usleep(20000);
		}
		blockCount++;

		if (blockCount % 2 == 1){
			memcpy(dataPoint + pointer, pru_input->Data, ARRAY_SIZE*2);
			pointer += ARRAY_SIZE;
		} else {
			memcpy(dataPoint + pointer, shared_data, ARRAY_SIZE*2);
			pointer += ARRAY_SIZE;
		}

		if (pointer==MAX_CYCLE*ARRAY_SIZE){
			pFile = fopen(str, "wb");
			fwrite(dataPoint,2,MAX_CYCLE*ARRAY_SIZE,pFile);
			fclose(pFile);
			pointer = 0;
			printf("Write Dataset\t");
			printf("Current Time: %d / %d sec\n", getTime()*1000000, TimeLimit);
		}

		now = getTime();

		if (now - start > TimeLimit * 1000000) {
            pru_input->RunFlag = 0;
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
