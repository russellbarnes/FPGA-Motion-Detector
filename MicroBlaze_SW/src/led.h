/*
 * led.h
 *
 *  Created on: Dec 8, 2016
 *      Author: russell
 */

#ifndef SRC_LED_H_
#define SRC_LED_H_

#define LED_CHANNEL		1
#define GPIO_BITWIDTH	16	/* # of LEDs to illuminate upon alarm */

int InitLED();
void LEDOn();
void LEDOff();


#endif /* SRC_LED_H_ */
