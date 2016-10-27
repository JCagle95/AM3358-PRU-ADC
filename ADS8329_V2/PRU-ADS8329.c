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
	pru_input->Delay = 20000;

	prussdrv_exec_program(PRU_NUM1, "./PRU-ADS8329.bin");

	int blockCount = 0, pointer = 0;
	uint16_t *dataPoint = malloc(100000 * 2);
	FILE *pFile;
	char str[80];
	sprintf(str, "Data%d.bin", RunCount);
	pFile = fopen(str,"wb");
	while (pru_input->RunFlag) {

		while (pru_input->DataReady == blockCount && pru_input->RunFlag){
		}
		blockCount++;

		if (blockCount % 2 == 1){
			memcpy(dataPoint+pointer, pru_input->Data, ARRAY_SIZE*2);
			pointer += ARRAY_SIZE;
		} else {
			memcpy(dataPoint+pointer, block2_data, ARRAY_SIZE*2);
			pointer += ARRAY_SIZE;
		}

		if (pointer==100000){
			fwrite(dataPoint,2,100000,pFile);
			pointer = 0;
		}
		fflush(stdout);
	}

	if (blockCount * ARRAY_SIZE < 100000){
		fwrite(dataPoint,2,100000,pFile);
	}
	fclose(pFile);
	printf("File Written Complete\n");
	thread_running = 0;
	free(dataPoint);

	// Wait for PRU0 to halt
	prussdrv_pru_wait_event(PRU_EVTOUT_1);
	prussdrv_pru_clear_event(PRU1_ARM_INTERRUPT);

	// Safe Quit PRUs
	prussdrv_pru_disable(PRU_NUM1);
	prussdrv_exit();

	return NULL;
}

int main(void) {
	pthread_t tid;
	int taskContinue = 1;
	int input;

	while (taskContinue) {
		printf("1: Task Start, 2: Task End, 3: QUIT, 4: Change Delay\nPlease enter: ");
		scanf("%d", &input);
		printf("\n");

		switch (input) {

		case 1:
			if (!thread_running) {
				thread_running = 1;
				RunCount++;
				pthread_create(&tid, NULL, &readInput, NULL);
			}
			else {
				printf("The task is already running. Please enter 2 to stop PRU\n");
			}
			break;

		case 2:
			if (thread_running) {
				pru_input->RunFlag = 0;
			}
			else {
				printf("The task has not started yet, please enter 1 to start PRU\n");
			}
			break;

		case 3:
			taskContinue = 0;
			break;

		case 4:
			printf("Enter Delay Amount: ");
			scanf("%d", &input);
			pru_input->Delay = input;
			printf("\n");
			break;

		default:
			printf("Unknown input\n");

		}
	}

	if (thread_running) {
		pru_input->RunFlag = 0;
	}

	printf("Finished Execution\n\n");

	return (0);
}
