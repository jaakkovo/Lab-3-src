/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string>
#include "string.h"


#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

// TODO: insert other include files here
QueueHandle_t xQueue = NULL;

char word[61];
char empty[61];
int indexi = 0;
char question[] = "?";

/* UART (or output) thread */
static void task1(void *pvParameters) {
	int numberof = 0;
	int character;

	while (1) {
		character = Board_UARTGetChar();
		if (character != -1){
			if (character != 10 && character != 13 && character != 8){
				Board_UARTPutChar(character);
				numberof++;
			}else{

				Board_UARTPutSTR ("\r\n");
				if( xQueueSendToBack( xQueue,( void * ) &numberof,( TickType_t ) 10 ) != pdPASS )
				{
					// fail
				}
				numberof = 0;
			}
		}
	}
}

/* UART (or output) thread */
static void task2(void *pvParameters) {
	int minusone = -1;
	int send = 0;

	while (1) {
		while (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 17)){
			send = 1;
		}
		if(send == 1){
			if( xQueueSendToBack( xQueue,( void * ) &minusone,( TickType_t ) 10 ) != pdPASS )
			{
				// fail
			}
			send = 0;
			vTaskDelay(configTICK_RATE_HZ / 12);
		}
	}
}

/* UART (or output) thread */
static void task3(void *pvParameters) {
	int number = 0;
	int total = 0;
	char s[3];

	while (1) {
		if( xQueueReceive( xQueue, &( number ), ( TickType_t ) 10 ) )
		{
			if (number == -1){
				Board_UARTPutSTR("You have typed ");
				sprintf(s, "%d", total);

				Board_UARTPutSTR(s);

				Board_UARTPutSTR(" characters \r\n");
				total = 0;
			}else{
				total += number;
			}
		}
	}
}

int main(void) {

#if defined (__USE_LPCOPEN)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
#endif
#endif

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 17, IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 17);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 11, IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 11);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 9, IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 9);

	xQueue = xQueueCreate( 10, sizeof( int ) );

	/* UART output thread, simply counts seconds */
	xTaskCreate(task1, "task1",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* UART output thread, simply counts seconds */
	xTaskCreate(task2, "task2",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* UART output thread, simply counts seconds */
	xTaskCreate(task3, "task3",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);


	/* Start the scheduler */
	vTaskStartScheduler();

	// Force the counter to be placed into memory
	volatile static int i = 0 ;
	// Enter an infinite loop, just incrementing a counter
	while(1) {
		i++ ;
	}
	return 0 ;
}
