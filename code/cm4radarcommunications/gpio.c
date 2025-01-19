#include <stdio.h>
#include <unistd.h>
#include "gpio.h"

void gpioSetup(void){
    wiringPiSetupGpio();

    // Setup RADAR1 GPIOs
    pinMode(RAD_EN, OUTPUT);
    pinMode(RAD1_NRST, OUTPUT);
    pinMode(RAD1_HOSTINT, INPUT);

    pullUpDnControl(RAD_EN, PUD_DOWN);
    pullUpDnControl(RAD1_HOSTINT, PUD_DOWN);
}

void resetRadar(void){
        digitalWrite(RAD1_NRST, LOW);
        sleep(2);
        digitalWrite(RAD1_NRST, HIGH);
}
void turnOffRadar(void){
        digitalWrite(RAD1_NRST, LOW);
        digitalWrite(RAD_EN, LOW);
        printf("Radar1 is OFF\n");
}

void turnOnRadar(void){
        digitalWrite(RAD_EN, HIGH);
        printf("Radar Power Enabled\n");
        sleep(1);
        digitalWrite(RAD1_NRST, HIGH);
        printf("Radar1 RESET\n");
}

int getHostInt(void){
        return digitalRead(RAD1_HOSTINT);
}