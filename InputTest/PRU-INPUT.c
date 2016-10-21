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

void sleepms(int ms) {
	nanosleep((struct timespec[]) { {0, ms * 1000000} }, NULL);
}

typedef struct {
	uint32_t run_flag;
	uint32_t data;
	uint32_t reference;
} pru_data;

pru_data *pru_input;
int thread_running = 0;

void* readInput(void *arg) {

	unsigned int ret;
	void *pru_memory;
	unsigned int data;

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
	pru_input = pru_memory;

	pru_input->run_flag = 1;
	
	prussdrv_exec_program(PRU_NUM1, "./PRU-INPUT.bin");

	while (pru_input->run_flag) {
		printf("Current Reading: %d\n", pru_input->data);
		printf("Reference: %d\n", pru_input->reference);
		usleep(1000000);
	}

	thread_running = 0;

	// Wait for PRU0 to halt
	prussdrv_pru_wait_event(PRU_EVTOUT_1);
	prussdrv_pru_clear_event(PRU0_ARM_INTERRUPT);

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
				pthread_create(&tid, NULL, &readInput, NULL);
			}
			else {
				printf("The task is already running. Please enter 2 to stop PRU\n");
			}
			break;

		case 2:
			if (thread_running) {
				pru_input->run_flag = 0;
			}
			else {
				printf("The task has not started yet, please enter 1 to start PRU\n");
			}
			break;

		case 3:
			taskContinue = 0;
			break;

		default:
			printf("Unknown input\n");

		}
	}

	if (thread_running) {
		pru_input->run_flag = 0;
	}

	printf("Finished Execution\n\n");

	return (0);
}
