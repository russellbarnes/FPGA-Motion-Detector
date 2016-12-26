/*
 * I2C.c
 *
 *  Created on: Dec 4, 2016
 *      Author: Russell Barnes
 */

/******************************************************************************
*
* Russell Barnes
* ECE 253 Fall 2016 Final Project
* Configuring ArduCAM Mini for QVGA single capture
*
* Copyright (C) 2006 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
* Based on:
* @file xiic_tempsensor_example.c
*
* The XIic_MasterRecv() API is used to receive the data.
*
* This program assumes that there is an interrupt controller in the hardware
* system and the IIC device is connected to the interrupt controller.
*
* @note
*
* 7-bit addressing is used to access the camera.
*
*****************************************************************************/

/***************************** Include Files ********************************/

#include "I2C.h"
#include "ov2640_regs.h"  // Contains ArduCAM Mini configuration data

#include "xparameters.h"
#include "xiic.h"

//#define DEBUG
/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define IIC_DEVICE_ID		XPAR_IIC_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_INTC_0_DEVICE_ID
#define INTC_IIC_INTERRUPT_ID	XPAR_INTC_0_IIC_0_VEC_ID


/*
 * The following constant defines the address of the IIC
 * ArduCam Mini camera device on the IIC bus. Note that since
 * the address is only 7 bits, this  constant is the address divided by 2.
 */
#define CAMERA_ADDRESS	0x30 /* The actual address is 0x60 */


/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ****************************/

static int SetupInterruptSystem(XIic *IicPtr);

static void RecvHandler(void *CallbackRef, int ByteCount);
static void SendHandler(void *CallbackRef, int ByteCount);
static void StatusHandler(void *CallbackRef, int Status);


/************************** Variable Definitions **************************/

XIic Iic;		  /* The instance of the IIC device */

XIntc InterruptController;  /* The instance of the Interrupt controller */

/*
 * The following structure contains fields that are used with the callbacks
 * (handlers) of the IIC driver. The driver asynchronously calls handlers
 * when abnormal events occur or when data has been sent or received. This
 * structure must be volatile to work when the code is optimized.
 */
volatile struct {
	int  EventStatus;
	int  RemainingRecvBytes;
	int EventStatusUpdated;
	int RecvBytesUpdated;
	int RemainingSendBytes;
	int SendBytesUpdated;
} HandlerInfo;

int i2cIsInitialized;

/*****************************************************************************
*
* Initialize I2C interface for communication with ArduCam chip.
* This program requires interrupt support.
*
*******************************************************************************/
int I2CInit()
{
	int Status;
	XIic_Config *ConfigPtr;	/* Pointer to configuration data */
	/*
	 * Initialize the IIC driver so that it is ready to use.
	 */
	ConfigPtr = XIic_LookupConfig(IIC_DEVICE_ID);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}

	Status = XIic_CfgInitialize(&Iic, ConfigPtr,
					ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Setup handler to process the asynchronous events which occur,
	 * the driver is only interrupt driven such that this must be
	 * done prior to starting the device.
	 */
	XIic_SetRecvHandler(&Iic, (void *)&HandlerInfo, RecvHandler);
	XIic_SetSendHandler(&Iic, (void *)&HandlerInfo, SendHandler);
	XIic_SetStatusHandler(&Iic, (void *)&HandlerInfo,
					StatusHandler);

	/*
	 * Connect the ISR to the interrupt and enable interrupts.
	 */
	Status = SetupInterruptSystem(&Iic);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the IIC driver such that it is ready to send and
	 * receive messages on the IIC interface, set the address
	 * to send to which is the ArduCAM Mini camera address
	 */
	XIic_Start(&Iic);
	XIic_SetAddress(&Iic, XII_ADDR_TO_SEND_TYPE, CAMERA_ADDRESS);

	i2cIsInitialized = 1;

	return XST_SUCCESS;
}

/*****************************************************************************
*
* The function for writing to an ArduCAM Mini register
*
* Must be called after I2CInit()
*
*******************************************************************************/
int ArduCamMiniInit()
{
	int status = 0;
	if (!i2cIsInitialized) {
		xil_printf("Error: Please call I2CInit() first\n");
		return XST_FAILURE;
	}
	// Begin writing config to camera controller volatile memory

	WriteArduCamMini(0xff, 0x01);
	WriteArduCamMini(0x12, 0x80);

	xil_printf("Begin ArduCam init\n");  // Leave for delay purposes

	// Set QVGA RGB image format
	u8 reg_addr = 0;
	u8 reg_val = 0;
	const struct sensor_reg *next = OV2640_QVGA;
	while ((reg_addr != 0xff) | (reg_val != 0xff)) {
		reg_addr = next->reg;
		reg_val = next->val;
		status = WriteArduCamMini(reg_addr, reg_val);
		if (status) {
			xil_printf("Error writing ArduCAM config\n");
		}
		next++;
	}

	// Leave this log message for delay purposes
	xil_printf("ArduCam initialized\n\n");

	return XST_SUCCESS;
}

/*****************************************************************************
*
* The function for writing to an ArduCAM Mini register
*
* Port of wrSensorReg8_8() function in ArduCAM driver
*
*******************************************************************************/
int WriteArduCamMini(u8 reg, u8 data)
{
	u8 writeBuf[2];
	int status;

	writeBuf[0] = reg;
	writeBuf[1] = data;

	// Wait until bus is free
	while(XIic_IsIicBusy(&Iic) == TRUE) {}

	status = WriteBytes(writeBuf, 2);

	//print("Delay\n");	 // Add Delay?

	if (!status) {
		return XST_SUCCESS;
	} else {
		xil_printf("ERROR writing to ArduCAM register\n");
		return XST_FAILURE;
	}
}

/*****************************************************************************
*
* The function for reading from an ArduCAM Mini register
*
*******************************************************************************/
int ReadArduCamMini(u8 reg, u8 *readBuf)
{
	int status;
	/*
	 * Start the IIC device.
	 */
	/*Status = XIic_Start(&Iic);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}*/

	// Wait until bus is free
	while(XIic_IsIicBusy(&Iic) == TRUE) {}

	status = WriteBytes(&reg, 1);
	if (status) {
		xil_printf("ERROR writing to ArduCAM register (for reading)\n");
		return XST_FAILURE;
	}

	// Wait until bus is free again
	while(XIic_IsIicBusy(&Iic) == TRUE) {}

	status = ReadBytes(readBuf, 1);
	if (!status) {
		return XST_SUCCESS;
	} else {
		xil_printf("ERROR reading from ArduCAM register\n");
		return XST_FAILURE;
	}
	xil_printf("Delay\n");
}

/*****************************************************************************/
/**
* The function reads one byte from the camera
*
* @param	recvBuf is a pointer to the data byte(s) read from the camera
*
* @return	XST_SUCCESS to indicate success, else XST_FAILURE to indicate
*		a Failure.
*
* @note		None.
*
*******************************************************************************/
int ReadBytes(u8 *recvBuf, int count)
{
	int Status;

	/*
	 * Clear updated flags such that they can be polled to indicate
	 * when the handler information has changed asynchronously and
	 * initialize the status which will be returned to a default value
	 */
	HandlerInfo.EventStatusUpdated = FALSE;
	HandlerInfo.RecvBytesUpdated = FALSE;
	Status = XST_FAILURE;

	/*
	 * Attempt to receive a byte of data from the camera
	 * on the IIC interface, ignore the return value since this example is
	 * a single master system such that the IIC bus should not ever be busy
	 *
	 * Format: Instance, readBuffer, byte count
	 */
	XIic_MasterRecv(&Iic, recvBuf, count);

	/*
	 * The message is being received from the camera;
	 * wait for it to complete by polling the information that is
	 * updated asynchronously by interrupt processing
	 */
	while(1) {
		if(HandlerInfo.RecvBytesUpdated == TRUE) {  // && (XIic_IsIicBusy(&IicInstance) == FALSE)) {
			/*
			 * The device information has been updated for receive
			 * processing, if all bytes received, indicate
			 * success
			 */
			if (HandlerInfo.RemainingRecvBytes == 0) {
				Status = XST_SUCCESS;
				break;
			}
			//break;
		}

		/*
		 * Any event status which occurs indicates there was an error,
		 * so return unsuccessful, for this example there should be no
		 * status events since there is a single master on the bus
		 */
		if (HandlerInfo.EventStatusUpdated == TRUE) {
			break;
		}
	}

	return Status;
}


/*****************************************************************************/
/**
* The function writes one byte to the camera
*
* @param	recvBuf is a pointer to the data byte(s) written to the camera
*
* @return	XST_SUCCESS to indicate success, else XST_FAILURE to indicate
*		a Failure.
*
* @note		None.
*
*******************************************************************************/
int WriteBytes(u8 *writeBuf, int count)
{
	int Status;

	/*
	 * Clear updated flags such that they can be polled to indicate
	 * when the handler information has changed asynchronously and
	 * initialize the status which will be returned to a default value
	 */
	HandlerInfo.EventStatusUpdated = FALSE;
	HandlerInfo.SendBytesUpdated = FALSE;
	Status = XST_FAILURE;

	/*
	 * Attempt to write a byte of data to the camera
	 * on the IIC interface, ignore the return value since this example is
	 * a single master system such that the IIC bus should not ever be busy
	 */
	//print("about to write\n");
	XIic_MasterSend(&Iic, writeBuf, count);
	//print("just wrote\n");

	/*
	 * The message is being received from the camera;
	 * wait for it to complete by polling the information that is
	 * updated asynchronously by interrupt processing
	 */
	while(1) {
		if((HandlerInfo.SendBytesUpdated == TRUE)) {  // && (XIic_IsIicBusy(&IicInstance) == FALSE)) {
			/*
			 * The device information has been updated for receive
			 * processing,if all bytes written, indicate
			 * success
			 */
			if (HandlerInfo.RemainingSendBytes == 0) {
				Status = XST_SUCCESS;
				break;
			}
			//break;
		}

		/*
		 * Any event status which occurs indicates there was an error,
		 * so return unsuccessful, for this example there should be no
		 * status events since there is a single master on the bus
		 */
		if (HandlerInfo.EventStatusUpdated == TRUE) {

			break;
		}
	}

	return Status;
}


/*****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur
* for IIC. This function is application specific since the actual system may
* or may not have an interrupt controller. The IIC device could be directly
* connected to a processor without an interrupt controller. The user should
* modify this function to fit the application.
*
* @param	IicPtr contains a pointer to the instance of the IIC component
*		which is going to be connected to the interrupt controller.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @notes	None.
*
****************************************************************************/
static int SetupInterruptSystem(XIic *IicPtr)
{
	int Status;

	/*
	 * Initialize the interrupt controller driver so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h
	 */
	Status = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Connect a device driver handler that will be called when an interrupt
	 * for the device occurs, the device driver handler performs the
	 * specific interrupt processing for the device
	 */
	Status = XIntc_Connect(&InterruptController, INTC_IIC_INTERRUPT_ID,
					XIic_InterruptHandler, IicPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the interrupt controller such that interrupts are recognized
	 * and handled by the processor.
	 */
	Status = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable the interrupts for the IIC device.
	 */
	XIntc_Enable(&InterruptController, INTC_IIC_INTERRUPT_ID);

	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				 (Xil_ExceptionHandler) XIntc_InterruptHandler,
				 &InterruptController);

	/*
	 * Enable non-critical exceptions.
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;

}


/*****************************************************************************/
/**
* This receive handler is called asynchronously from an interrupt context and
* and indicates that data in the specified buffer was received. The byte count
* should equal the byte count of the buffer if all the buffer was filled.
*
* @param	CallBackRef is a pointer to the IIC device driver instance which
*		the handler is being called for.
* @param	ByteCount indicates the number of bytes remaining to be received of
*		the requested byte count. A value of zero indicates all requested
*		bytes were received.
*
* @return	None.
*
* @notes	None.
*
****************************************************************************/
static void RecvHandler(void *CallbackRef, int ByteCount)
{
	HandlerInfo.RemainingRecvBytes = ByteCount;
	HandlerInfo.RecvBytesUpdated = TRUE;
}

/*****************************************************************************/
/**
* This is called asynchronously upon transmission
*
* @param	CallBackRef is a pointer to the IIC device driver instance which
*		the handler is being called for.
* @param	ByteCount indicates the number of bytes remaining to be sent of
*		the requested byte count.  A value of zero indicates all requested
*		bytes were sent.
*
****************************************************************************/
static void SendHandler(void *CallbackRef, int ByteCount)
{
	HandlerInfo.RemainingSendBytes = ByteCount;
	HandlerInfo.SendBytesUpdated = TRUE;
}

/*****************************************************************************/
/**
* This status handler is called asynchronously from an interrupt context and
* indicates that the conditions of the IIC bus changed. This  handler should
* not be called for the application unless an error occurs.
*
* @param	CallBackRef is a pointer to the IIC device driver instance which the
*		handler is being called for.
* @param	Status contains the status of the IIC bus which changed.
*
* @return	None.
*
* @notes	None.
*
****************************************************************************/
static void StatusHandler(void *CallbackRef, int Status)
{
	HandlerInfo.EventStatus |= Status;
	HandlerInfo.EventStatusUpdated = TRUE;
#ifdef DEBUG
	switch(Status) {
	case (XII_BUS_NOT_BUSY_EVENT):
		xil_printf("\tI2C Status ERROR: Bus not busy\n");
		break;
	case (XII_ARB_LOST_EVENT):
		xil_printf("\tI2C Status ERROR: Arbitration was lost\n");
		break;
	case (XII_SLAVE_NO_ACK_EVENT):
		xil_printf("\tI2C Status ERROR: ERROR, slave did not ack\n");
		break;
	case (XII_MASTER_READ_EVENT):
		xil_printf("\tI2C Status ERROR: Master reading from slave\n");
		break;
	case (XII_MASTER_WRITE_EVENT):
		xil_printf("\tI2C Status ERROR: Master writing to slave\n");
		break;
	case (XII_GENERAL_CALL_EVENT):
		xil_printf("\tI2C Status (error?): General call to all slaves\n");
		break;
	default:
		xil_printf("I2C StatusHandler(): Unknown error occurred\n");
	}
#else
	xil_printf("I2C: Error occurred\n");
#endif
	return;
}

