#ifndef GPIO_H_
#define GPIO_H_

#include <wiringPi.h>

#define RAD_EN 7
#define RAD1_NRST 27
#define RAD1_HOSTINT 26

void gpioSetup(void);

void resetRadar(void);
void turnOffRadar(void);
void turnOnRadar(void);

int getHostInt(void);

#endif