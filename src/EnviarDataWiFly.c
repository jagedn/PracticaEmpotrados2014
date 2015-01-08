/*
 * EnviarDataWiFly.c
 *
 *  Created on: 4/12/2014
 *      Author: jorge
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "EnviarDataWiFly.h"

#define MAX_QUEUE_CAPACITY 5

#define LPC_UART LPC_UART3

const char *CMD = "CMD";
const char *END_CMD = "<4.00>";
const char *ERR_CMD = "Err";
const char *FAIL_CMD = "FAILED";
const char *EXIT = "EXIT";
const char *IF_UP = "IF=UP";

const char *SSID = "ONOC523";
const char *PWD = "1848116743";

//const char *APP = "arpalab.appspot.com";
//const char *POST_URL = "rest/stats?nid=6666666&ip=127.0.0.1&app=jag&data=";

const char *APP = "cloud.puravida-software.com";
const char *POST_URL = "/get.php?id=jagedn";

xQueueHandle queueHandle;
char buffer[100];
uint8_t debug = 1;


void DelayMS(uint32_t ms) {
	uint32_t i;
	for (i = 0; i < (((uint32_t) 7140) * ms); i++)
		; // loop 7140 approximate 1 millisecond
}

void Send(const char *cmd) {
	if (debug)
		printf("wifly> %s\r\n", cmd);
	DelayMS(250);
	Chip_UART_SendBlocking(LPC_UART, cmd, strlen(cmd));
	DelayMS(250);
}

void Receive(char *buffer, uint16_t size) {
	int retry = 1;
	int pos = 0;
	int read = size-1;
	memset(buffer, 0, size);
	while (retry) {
		int readed = Chip_UART_Read(LPC_UART, &buffer[pos], read);
		if (strstr(buffer, END_CMD))
			break;
		if (strstr(buffer, CMD))
			break;
		if (strstr(buffer, ERR_CMD))
			break;
		if (strstr(buffer, FAIL_CMD))
			break;
		if (strstr(buffer, EXIT))
			break;
		pos += readed;
		if( pos >= size){
			break;
		}
	}

	if (debug) {
		// Solo para debugear mejor
		for (pos = 0; pos < size; pos++) {
			if (buffer[pos] == '\n' || buffer[pos] == '\r')
				buffer[pos] = ' ';
		}
		printf("%s\r\n", buffer);
	}
}

void SendAndReceive(const char *cmd, char *buffer, uint16_t size) {
	Send(cmd);
	Receive(buffer, size);
}

void HttpGet(DataQueue_t item) {
	char tmp[80];
	sprintf(tmp,"&channel=%d&data=%d\r\n",item.channel,item.data);
	Send(tmp);
}

void InitWiFly(void) {

	queueHandle = xQueueCreate(MAX_QUEUE_CAPACITY, sizeof(DataQueue_t));

	Chip_UART_Init(LPC_UART);
	Chip_UART_SetBaud(LPC_UART, 9600);
	Chip_UART_ConfigData(LPC_UART,
			(UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS));
	Chip_UART_TXEnable(LPC_UART);

	while ( true) {
		SendAndReceive("$$$", buffer, 20);
		if (strstr(buffer, CMD))
			break;
	}

	while ( true) {
		SendAndReceive("factory Reset\r\n", buffer, sizeof(buffer));

		sprintf(buffer,"set wlan ssid %s\r\n", SSID);
		SendAndReceive(buffer, buffer, sizeof(buffer));

		sprintf(buffer,"set wlan p %s\r\n", PWD);
		SendAndReceive(buffer, buffer, sizeof(buffer));

		SendAndReceive("set wlan join 1\r\n", buffer, sizeof(buffer));
		SendAndReceive("join\r\n", buffer, sizeof(buffer));
		if (strstr(buffer, "FAILED") == NULL)
			break;
	}

	while ( true) {
		SendAndReceive("get ip\r\n", buffer, sizeof(buffer));
		if (strstr(buffer, IF_UP) != NULL)
			break;
		DelayMS(1000);
	}

	SendAndReceive("set ip proto 18\r\n", buffer, sizeof(buffer));
	SendAndReceive("set ip host 0\r\n", buffer, sizeof(buffer));
	SendAndReceive("set ip remote 80\r\n", buffer, sizeof(buffer));

	sprintf(buffer,"set dns name %s\r\n", APP);
	SendAndReceive(buffer, buffer, sizeof(buffer));

	sprintf(buffer, "set com remote GET$http://%s/%s\r\n", APP, POST_URL);
	SendAndReceive(buffer, buffer, sizeof(buffer));

	//SendAndReceive("set comm time 5000\r\n", buffer, sizeof(buffer));
	SendAndReceive("set uart mode 2\r\n", buffer, sizeof(buffer));
	SendAndReceive("exit\r\n", buffer, sizeof(buffer));

	DataQueue_t item = { 0, 0 };
	HttpGet(item);
}

void PushDataWiFly(DataQueue_t *item) {
	xQueueSend(queueHandle, item, portMAX_DELAY);
}

void SendNextDataWiFly() {
	DataQueue_t item = { 0, 0 };
	if ( xQueueReceive(queueHandle, &item, portMAX_DELAY) == pdPASS) {
		printf("Channel %04d \t %04d\n\r", item.channel, item.data);
		HttpGet(item);
	}
}
