/*
 * I2C.h
 *
 *  Created on: Dec 4, 2016
 *      Author: russell
 */

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include "xil_exception.h"
#include "xintc.h"

extern XIntc InterruptController;  /* The instance of the Interrupt controller */

int I2CInit();
int ArduCamMiniInit(); // Must be called after I2CInit()

int ReadArduCamMini(u8 reg, u8 *readBuf);
int WriteArduCamMini(u8 reg, u8 data);

int ReadBytes(u8 *recvBuf, int count);
int WriteBytes(u8 *writeBuf, int count);

#endif /* SRC_I2C_H_ */
