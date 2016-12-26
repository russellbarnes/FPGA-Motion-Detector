/*
 * lcd.h
 *
 *  Created on: Oct 31, 2016
 *      Author: russell
 */

#ifndef SRC_LCDCTRL_H_
#define SRC_LCDCTRL_H_

//#include "bsp.h"
#include "xgpio.h"
#include "xintc.h"		// Interrupt types

// LCD screen
void DrawCameraToLCD(u8 *img);

// SPI
int LCDSPIInit();

#endif /* SRC_LCDCTRL_H_ */
