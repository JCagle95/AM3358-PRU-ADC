/*
 * ADS8329.c
 *
 *  Created on: Sep 13, 2016
 *      Author: Jackson
 */

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "mcp3301.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define ADCTASKSTACKSIZE 1024

static void
ADCTask(void *pvParameters)
{
	// Input is the default configuration 1111b
	int input[16] = {GPIO_PIN_3,GPIO_PIN_3,GPIO_PIN_3,GPIO_PIN_3,0,0,0,0,0,0,0,0,0,0,0,0};

	// readInput is the readData 1101b
	int readinput[16] = {GPIO_PIN_3,GPIO_PIN_3,0,GPIO_PIN_3,0,0,0,0,0,0,0,0,0,0,0,0};

	int Round = 1;
	int i;
	uint16_t value;

	while(1)
	{
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 0);  	// CONVST LOW
		SysCtlDelay(10);								// Delay
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0);	// Chip Select LOW

		value = 0;										// Reset Value
		for (i = 0; i < 16; i++){

			if (Round == 1) GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, input[i]);
			else GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, readinput[i]);

			GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_PIN_2);

			if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_4) & GPIO_PIN_4) >> 4 == 0x01){
				value |= 1;
				value = value << 1;
			} else {
				value = value << 1;
			}

			GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
		}

		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_PIN_5);
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0);

		UARTprintf("%d\n",value & 0x0FFF);

		Round += 1;
		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7);
		SysCtlDelay(1);
	}
}

uint32_t
ADCTaskInit(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_2);
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_4);
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_5);
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_6);
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_7);

	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_PIN_5);
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0);
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7);
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0);

	UARTprintf("Pass\n");

	if(xTaskCreate(ADCTask, (const portCHAR *)"ADC", ADCTASKSTACKSIZE, NULL,
				   tskIDLE_PRIORITY + PRIORITY_LED_TASK, NULL) != pdTRUE)
	{
		UARTprintf("Fail\n");
		return(1);
	}

	return (0);
}


