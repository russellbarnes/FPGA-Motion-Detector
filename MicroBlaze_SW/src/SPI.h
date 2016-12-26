/*
 * SPI.h
 *
 *  Created on: Dec 4, 2016
 *      Author: russell
 */

#ifndef SRC_SPI_H_
#define SRC_SPI_H_
#include "xspi.h"

// ArduCAM Mini values
#define ARDUCHIP_FIFO		0x04  // FIFO and I2C control
#define FIFO_CLEAR_MASK		0x01  // Flush FIFO
#define FIFO_START_MASK		0x02  // Start capture
#define ARDUCHIP_TRIG		0x41  // Register
#define CAP_DONE_MASK		0x08  // Bit, indicates finished capture
#define BURST_FIFO_READ		0x3C  // Burst FIFO read operation
#define SINGLE_FIFO_READ	0x3D  // Single FIFO read operation

int SPIInit();

int ArduCamMiniCapture(u8 *img, u8 *imgGray, int *avg);

int CameraSend(u8 addr, u8 data);  // Send byte to ArduCAM
int CameraRead(u8 addr, u8 *dataBuf);  // Read byte from ArduCAM
int CameraBurstRead(u8 *dataBuf);  // Only for image transfer

int SPIWrite8(u8 *sendBuf);
int SPIRead8(u8 *recvBuf);

int ErrorCheck(int status);

#endif /* SRC_SPI_H_ */
