/*
 ===============================================================================
 Name        : Practica.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
 ===============================================================================
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

// TODO: insert other include files here

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "SensorMMA7361.h"
#include "EnviarDataWiFly.h"

// TODO: insert other definitions and declarations here

#define MAX_AVERAGE_COUNT 10

typedef struct {
	uint16_t 	value;
	uint16_t 	lastValue;
} Diference_t;

typedef struct {
	uint8_t 	count;
	uint16_t	values[MAX_AVERAGE_COUNT];
} Average_t;


Diference_t valueY = {0, 0};

Average_t valueZ;


//  Funciones utiles
void InitAverage( Average_t *valueZ){
	int i;
	valueZ->count=0;
	for(i=0; i<MAX_AVERAGE_COUNT; i++){
		valueZ->values[i]=0;
	}
}

uint16_t GetAverage(Average_t *valueZ){
	int i;
	uint16_t acum=0;
	for(i=0; i<valueZ->count; i++){
		acum += valueZ->values[i];
	}
	return acum / valueZ->count;
}

void PushAverage(Average_t *valueZ, uint16_t add){
	if( valueZ->count == MAX_AVERAGE_COUNT ){
		int i;
		for(i=1; i<valueZ->count; i++){
			valueZ->values[i-1]=valueZ->values[i];
		}
		valueZ->count--;
	}
	valueZ->values[valueZ->count++] = add;
}


// Tareas principales
void prvReadSensorTask(void *pvParameters) {
	portTickType xNextWakeTime;

	channel_t *channel = (channel_t *) pvParameters;
	uint32_t delay = 0;
	switch (*channel) {
	case X:
		delay = 1000;
		break;
	case Y:
		delay = 500;
		break;
	default:
		delay = 100;
	}

	xNextWakeTime = xTaskGetTickCount();
	for (;;) {
		DataQueue_t item = { *channel, 0};

		TakeSensorMMA7361();

		item.data = GetSampleSensorMMA7361(*channel);

		switch (*channel) {
		case X:
			PushDataWiFly(&item);
			break;
		case Y:
			valueY.value = item.data;
			item.data = valueY.lastValue - valueY.value;
			PushDataWiFly(&item);
			valueY.lastValue = valueY.value;
			break;
		case Z:
			PushAverage(&valueZ,item.data);
			if( valueZ.count == MAX_AVERAGE_COUNT){
				item.data = GetAverage(&valueZ);
				PushDataWiFly(&item);
			}
			break;
		default:
			break;
		}

		GiveSensorMMA7361();

		vTaskDelayUntil(&xNextWakeTime, delay*10); // multiplicamos por 10 para ver mejor las trazas
	}
}

void prvReadQueueTask(void *pvParameters) {
	for (;;) {
		SendNextDataWiFly();
	}
}


static channel_t x = X, y = Y, z = Z;
int main(void) {

#if defined (__USE_LPCOPEN)
#if !defined(NO_BOARD_LIB)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
	Board_LED_Set(0, true);
#endif
#endif

	InitAverage(&valueZ);

	InitSensorMMA7361();

	InitWiFly();

	printf("Starting practica\n");

	if (xTaskCreate(prvReadSensorTask, (signed char *) "prvReadSensorTaskX", configMINIMAL_STACK_SIZE+250,
			&x, (tskIDLE_PRIORITY + 3UL), (xTaskHandle *) NULL) != pdPASS) {
		printf("Error creando la tarea X\n");
		return -1;
	}

	if (xTaskCreate(prvReadSensorTask, (signed char *) "prvReadSensorTaskY", configMINIMAL_STACK_SIZE+250,
			&y, (tskIDLE_PRIORITY + 2UL), (xTaskHandle *) NULL) != pdPASS) {
		printf("Error creando la tarea Y\n");
		return -1;
	}

	if (xTaskCreate(prvReadSensorTask, (signed char *) "prvReadSensorTaskZ", configMINIMAL_STACK_SIZE+250,
			&z, (tskIDLE_PRIORITY + 1UL), (xTaskHandle *) NULL) != pdPASS) {
		printf("Error creando la tarea Z\n");
		return -1;
	}

	if (xTaskCreate(prvReadQueueTask, (signed char *) "prvReadQueue", configMINIMAL_STACK_SIZE+250,
			NULL, (tskIDLE_PRIORITY + 1UL), (xTaskHandle *) NULL) != pdPASS) {
		printf("Error creando la tarea Queue\n");
		return -1;
	}

	vTaskStartScheduler();

	for (;;)
		;
	return 0;
}

