/*
 * SensorMMA7361.c
 *
 *  Created on: 28/11/2014
 *      Author: jorge
 */
#include "board.h"
#include "iocon_17xx_40xx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "SensorMMA7361.h"


#ifdef BOARD_NXP_LPCXPRESSO_1769
#define _ADC_CHANNLE ADC_CH0
#else
#define _ADC_CHANNLE ADC_CH2
#endif

#define _LPC_ADC_ID LPC_ADC

static ADC_CLOCK_SETUP_T ADCSetup;


STATIC const PINMUX_GRP_T mmcaPinMuxing[] = {
{ 0, 23, IOCON_MODE_INACT | IOCON_FUNC1 }, /* ADC X */
{ 0, 24, IOCON_MODE_INACT | IOCON_FUNC1 }, /* ADC Y */
{ 0, 25, IOCON_MODE_INACT | IOCON_FUNC1 }, /* ADC Z */
};


xSemaphoreHandle semaphoreRead;

uint16_t GetSampleSensorMMA7361Pool(channel_t channel);

void bruteWaitTo0G(){
	uint16_t lastValueX = 0xffff;
	uint16_t lastValueY = 0xffff;
	uint16_t lastValueZ = 0xffff;

	uint16_t valueX = 0x0000;
	uint16_t valueY = 0x0000;
	uint16_t valueZ = 0x0000;

	while( true ){
		valueX = GetSampleSensorMMA7361(X);
		valueY = GetSampleSensorMMA7361(Y);
		valueZ = GetSampleSensorMMA7361(Z);
		if( abs(lastValueX-valueX) < 10 && abs(lastValueY-valueY) < 10 && abs(lastValueZ-valueZ) < 10 ){
			break;
		}
		lastValueX = valueX;
		lastValueY = valueY;
		lastValueZ = valueZ;
		printf("%04d \t %04d \t %04d\n\r",valueX, valueY, valueZ);
	}
}

void pinWaitTo0G(){
	int i=0;
	int count=0;
	while( i < 100){
		uint16_t g = GetSampleSensorMMA7361Pool(G0);
		printf("G0 %04d\r\n", g);
		count = g == 0 ? count+1 : 0;
		if( count > 5){
			break;
		}
		i++;
	}
}

void waitTo0G(){
	printf("Esperando a estabilizacion del sensor\r\n");
	pinWaitTo0G();
	printf("Sensor estabilizado\r\n");
}

void InitSensorMMA7361(void) {
	uint32_t _bitRate = 110000;

	Chip_IOCON_SetPinMuxing(LPC_IOCON, mmcaPinMuxing,
			sizeof(mmcaPinMuxing) / sizeof(PINMUX_GRP_T));

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, 1);

	Chip_ADC_Init(_LPC_ADC_ID, &ADCSetup);

	Chip_ADC_SetSampleRate(_LPC_ADC_ID, &ADCSetup, _bitRate);
	Chip_ADC_SetBurstCmd(_LPC_ADC_ID, DISABLE);

	waitTo0G();

	semaphoreRead = xSemaphoreCreateMutex();
}

void TakeSensorMMA7361(void) {

	xSemaphoreTake(semaphoreRead, portMAX_DELAY);

}
void GiveSensorMMA7361(void) {
	xSemaphoreGive(semaphoreRead);
}

uint16_t GetSampleSensorMMA7361Pool(channel_t channel) {
	uint16_t dataADC;

	Chip_ADC_EnableChannel(_LPC_ADC_ID, channel, ENABLE);

	Chip_ADC_SetStartMode(_LPC_ADC_ID, ADC_START_NOW, ADC_TRIGGERMODE_RISING);

	while (Chip_ADC_ReadStatus(_LPC_ADC_ID, channel, ADC_DR_DONE_STAT)
			!= SET) {
	}
	/* Read ADC value */
	if( Chip_ADC_ReadValue(_LPC_ADC_ID, channel, &dataADC) == ERROR ){
		DEBUGSTR("Error leyendo, dato invalido \n\r");
		return -1;
	}

	Chip_ADC_EnableChannel(_LPC_ADC_ID, channel, DISABLE);

	return dataADC;
}

uint16_t GetSampleSensorMMA7361(channel_t channel) {

	uint16_t ret1 = GetSampleSensorMMA7361Pool(channel );

	return ret1; // (ret1 * 3.3 / 4095)*100;  // VoltsRx = AdcRx * Vref / 12bits
}
