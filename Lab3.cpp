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
#include <cstdlib>
#include "ITM_write.h"

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
QueueHandle_t xQueue2 = NULL;
QueueHandle_t xDebug = NULL;


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
			if (character != 10 && character != 13 && character != 8 && numberof <= 5){
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

/* UART (or output) thread */
static void task1_2(void *pvParameters) {
	int numberof = 0;
	int character;

	while (1) {

		vTaskDelay(configTICK_RATE_HZ / (rand() % 9 + 2));

		if (character != -1){
			character = (rand() % 10 + 48);

			Board_UARTPutChar(character);
			numberof++;

			Board_UARTPutSTR ("\r\n");
			if( xQueueSendToBack( xQueue2,( void * ) &numberof,( TickType_t ) 10 ) != pdPASS )
			{
				// fail
			}
			numberof = 0;
		}
	}
}

/* UART (or output) thread */
static void task2_2(void *pvParameters) {
	int emergency = 112;
	int send = 0;

	while (1) {
		while (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 17)){
			send = 1;
		}
		if(send == 1){
			if( xQueueSendToFront( xQueue2,( void * ) &emergency,( TickType_t ) 10 ) != pdPASS )
			{
				// fail
			}
			send = 0;
			vTaskDelay(configTICK_RATE_HZ / 20);
		}
	}
}

/* UART (or output) thread */
static void task3_2(void *pvParameters) {
	int number = 0;
	char s[3];

	while (1) {
		if( xQueueReceive( xQueue2, &( number ), ( TickType_t ) 10 ) )
		{
			sprintf(s, "%d", number);
			Board_UARTPutSTR(s);

			if (number == 112){
				Board_UARTPutSTR(" Help me \r\n");
			}else{
				Board_UARTPutSTR(" \r\n");
			}
			vTaskDelay(configTICK_RATE_HZ / 3.333);
		}
	}
}

struct debugEvent {
	char *format;
	uint32_t data[3];
};


static void debug(char *format, uint32_t d1, uint32_t d2, uint32_t d3){

	debugEvent eventti;

	eventti.format = format;
	eventti.data[0] = d1;
	eventti.data[1] = d2;
	eventti.data[2] = d3;


	if( xQueueSendToBack( xDebug,( void * ) &eventti,( TickType_t ) 10 ) != pdPASS )
	{
		// fail
	}
};


/* UART (or output) thread */
static void task1_3(void *pvParameters) {
	int numberof = 0;
	int character;

	while (1) {
		character = Board_UARTGetChar();
		if (character != -1){
			if (character != 10 && character != 13 && character != 8 && character != 32){

				Board_UARTPutChar(character);
				numberof++;

			}else{
				if (character == 32 && numberof == 0){
					Board_UARTPutChar(character);
				}else{
					Board_UARTPutSTR ("\r\n");
					debug("Receiver cmd: %d at %d\n", numberof, xTaskGetTickCount(), 0);
					numberof = 0;
				}
			}
		}
		vTaskDelay(configTICK_RATE_HZ / 20);
	}
}

/* UART (or output) thread */
static void task2_3(void *pvParameters) {
	int pressLength = 0;
	while (1) {
		if (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 17)){
			while (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 17)){
				pressLength++;
			}
			debug("Length of press was: %d.%d seconds.\n", pressLength/1400000, pressLength/140000, 0);
			pressLength = 0;
		}
		vTaskDelay(configTICK_RATE_HZ / 24);
	}
}

static void debugTask(void *pvParameters)
{
	char buffer[64];
	debugEvent e;
	// this is not complete! how do we know which queue to wait on?
	while (1) {
		// read queue
		xQueueReceive(xDebug, &e, portMAX_DELAY);
		snprintf(buffer, 64, e.format, e.data[0], e.data[1], e.data[2]);
		ITM_write(buffer);
	}
}



static void test(void *pvParameters){
	while(1){
		ITM_write("Toimii");
		vTaskDelay(configTICK_RATE_HZ);
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

	ITM_init();



	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 17, IOCON_DIGMODE_EN | IOCON_MODE_PULLUP);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 17);

	//	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 11, IOCON_DIGMODE_EN | IOCON_MODE_PULLUP);
	//	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 11);
	//
	//	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 9, IOCON_DIGMODE_EN | IOCON_MODE_PULLUP);
	//	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 9);

	xQueue = xQueueCreate( 5, sizeof (int) );
	xQueue2 = xQueueCreate( 20, sizeof (int) );
	xDebug = xQueueCreate(10, sizeof(debugEvent));

	// EXERCISE 1
	//	/* UART output thread, simply counts seconds */
	//	xTaskCreate(task1, "task1",
	//			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//			(TaskHandle_t *) NULL);
	//
	//	/* UART output thread, simply counts seconds */
	//	xTaskCreate(task2, "task2",
	//			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//			(TaskHandle_t *) NULL);
	//
	//	/* UART output thread, simply counts seconds */
	//	xTaskCreate(task3, "task3",
	//			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//			(TaskHandle_t *) NULL);


	// EXERCISE 2
	//	/* UART output thread, simply counts seconds */
	//	xTaskCreate(task1_2, "task1",
	//			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//			(TaskHandle_t *) NULL);
	//
	//	/* UART output thread, simply counts seconds */
	//	xTaskCreate(task2_2, "task2",
	//			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//			(TaskHandle_t *) NULL);
	//
	//	/* UART output thread, simply counts seconds */
	//	xTaskCreate(task3_2, "task3",
	//			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//			(TaskHandle_t *) NULL);


	//		/* UART output thread, simply counts seconds */
	//		xTaskCreate(test, "test",
	//				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	//				(TaskHandle_t *) NULL);

	// EXERCISE 3
	/* UART output thread, simply counts seconds */
	xTaskCreate(task1_3, "task1",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
			(TaskHandle_t *) NULL);

	/* UART output thread, simply counts seconds */
	xTaskCreate(task2_3, "task2",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
			(TaskHandle_t *) NULL);

	/* UART output thread, simply counts seconds */
	xTaskCreate(debugTask, "debugTask",
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
