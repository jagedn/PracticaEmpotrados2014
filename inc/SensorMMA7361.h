/*
 * SensorMMA7361.h
 *
 *  Created on: 28/11/2014
 *      Author: jorge
 */

#ifndef SENSORMMA7361_H_
#define SENSORMMA7361_H_

typedef enum CHANNEL_T{
	X = 0,
	Y = 1,
	Z = 2,
	G0 = 3
}channel_t;

void InitSensorMMA7361(void);
void TakeSensorMMA7361(void);
void GiveSensorMMA7361(void);
uint16_t GetSampleSensorMMA7361(channel_t channel);

#endif /* SENSORMMA7361_H_ */
