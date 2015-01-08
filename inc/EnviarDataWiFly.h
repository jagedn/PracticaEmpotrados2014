/*
 * EnviarDataWiFly.h
 *
 *  Created on: 4/12/2014
 *      Author: jorge
 */


#include "SensorMMA7361.h"

#ifndef ENVIARDATAWIFLY_H_
#define ENVIARDATAWIFLY_H_

typedef struct {
	channel_t channel;
	uint16_t data;
} DataQueue_t;

void InitWiFly(void);

void PushDataWiFly(DataQueue_t *item);

void SendNextDataWiFly();

#endif /* ENVIARDATAWIFLY_H_ */
