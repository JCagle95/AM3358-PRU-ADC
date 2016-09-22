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
#define ARRAY_SIZE	1000

void sleepms(int ms) {
	nanosleep((struct timespec[]){{0, ms*100000}}, NULL);
}

typedef struct {
	uint32_t run_flag;
	uint32_t data_ready;
	uint32_t data_size;
	uint32_t data1[ARRAY_SIZE];
	uint32_t data2[ARRAY_SIZE];
} pru_data;

typedef struct {
	void *pru_memory;
	pru_data* pru_data;
} app_data;

app_data info;
int workthread_running = 0;

void intHandler(int dummy) {
    printf("Setting run flag to 0\n");
    info.pru_data->run_flag = 0;
}

void* pru_ram_task(void *arg){
	
	unsigned int ret;
	// Initialize PRU0
	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	prussdrv_init();
	ret = prussdrv_open(PRU_EVTOUT_0);
	if (ret)
	{
		printf("prussdrv_open failed\n");
	}

	// Initialize Interrupt
	prussdrv_pruintc_init(&pruss_intc_initdata);

	// Initialize Memory Mapping for local memory
	prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, (void *) &info.pru_memory);
	info.pru_data = info.pru_memory;

	// Initialize Parameters
	info.pru_data->run_flag = 1;
	info.pru_data->data_ready = 0;
	info.pru_data->data_size = ARRAY_SIZE * sizeof(uint32_t);

	prussdrv_exec_program (PRU_NUM0, "./ram_writing.bin");

	int Counter = 0, pointer = 0;
	uint32_t dataPoint[100000];
	FILE * pFile;
	while(info.pru_data->run_flag){
		if (pointer == 0){
			char str[80];
			sprintf(str, "Data%d.bin",Counter);
			pFile = fopen(str,"wb");
		}
		int timer = 0;
		while (info.pru_data->data_ready == Counter && info.pru_data->run_flag){
			//printf("%d\n",info.pru_data->run_flag);
			usleep(100);
			timer++;
		}
		Counter++;
		if (Counter % 2 == 1){
			//fwrite(info.pru_data->data1,sizeof(uint32_t),ARRAY_SIZE,pFile);
			memcpy(dataPoint+pointer,info.pru_data->data1,ARRAY_SIZE*4);
			pointer += ARRAY_SIZE;
			printf("%d Data Point Reading: %4u00us, first point is %d\n",ARRAY_SIZE*2, timer, info.pru_data->data1[0]);
		} else {
			//fwrite(info.pru_data->data2,sizeof(uint32_t),ARRAY_SIZE,pFile);
			memcpy(dataPoint+pointer,info.pru_data->data2,ARRAY_SIZE*4);
			pointer += ARRAY_SIZE;
			printf("%d Data Point Reading: %4u00us, first point is %d\n",ARRAY_SIZE*2, timer, info.pru_data->data2[0]);
		}
		if (pointer==100000)
		{
			fwrite(dataPoint,sizeof(uint32_t),100000,pFile);
			fclose(pFile);
			pointer = 0;
		}
		fflush(stdout);
	}

	workthread_running = 0;

	// Wait for PRU0 to halt
	prussdrv_pru_wait_event(PRU_EVTOUT_0);
	prussdrv_pru_clear_event(PRU0_ARM_INTERRUPT);

	// Safe Quit PRUs
	prussdrv_pru_disable(PRU_NUM0);
	prussdrv_exit();

	return NULL;
}

int main (void)
{
	pthread_t tid;
	
	// This is Thread-Safe Halting
	signal (SIGQUIT, intHandler);
	signal (SIGINT, intHandler);
	
	int taskContinue = 1;
	int input;
	while (taskContinue)
	{
		printf("1: Task Start, 2: Task End, 3: QUIT\nPlease enter: ");
		scanf("%d",&input);
		printf("\n");

		switch(input){
			case 1:
				if (!workthread_running){
					workthread_running = 1;
					pthread_create(&tid, NULL, &pru_ram_task, NULL);
				} else {
					printf("The task is already running. Please enter 2 to stop PRU\n");
				}
				break;

			case 2:
				if (workthread_running){
				    info.pru_data->run_flag = 0;
				} else {
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
	
	if (workthread_running){
		info.pru_data->run_flag = 0;
	}

	printf("Finished Execution\n\n");

	return (0);
}
