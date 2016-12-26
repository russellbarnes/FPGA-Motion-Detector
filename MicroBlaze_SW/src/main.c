/******************************************************************************
* Russell Barnes
* ECE 253 Fall 2016 Final Project
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
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

/*
 * main.c
 *
 * This program is a motion detector for the following system configuration:
 * -- Nexys4 DDR development board including a Xilinx Artix-7 FPGA
 * 		-- FPGA block design includes AXI IIC, 1-channel GPIO (x2),
 * 		 16-channel GPIO for LEDs, AXI Quad SPI (x2), MIG, interrupt
 * 		 controller, MicroBlaze soft processor, others
 * -- ArduCAM-Mini-2MP camera with both I2C and SPI interfaces connected
 * -- 240x320 2.2" TFT display w/ILI9340C by Adafruit
 *
 * -- Memory configuration
 * 		-- MicroBlaze is utilizing 128K of BRAM
 * 		-- Stack is located in BRAM.  Stack size: 0x800
 * 		-- Heap is located in DDR memory via MIG.  Heap size: 0x80000
 *
 * -- Camera SPI @ < 8 MHz, display SPI @ 5 MHz, Camera I2C @ 100 KHz
 * -- SPI chip select signal for camera is *replaced* by single-channel
 * 		"spi_dc_1" GPIO
 * -- Additional spi_dc output for display "D/C" signal is single-channel GPIO
 * -- Current camera configuration is for QVGA bitmap output format.  To save
 * 		to a file, a bitmap header must be added to the beginning (not
 * 		included - see ArduCAM example code)
 * -- This application configures UART to baud rate 9600 for communication
 * 		with the host PC.
 *
 * Camera driver is based on ArduCAM example code:
 * 		http://www.arducam.com/
 * 		https://github.com/ArduCAM/Arduino
 *
 * Also uses UTFT.cpp - Multi-Platform library support for Color TFT LCD Boards
 * Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
 *
 * Known issues:
 * -- Camera SPI occasionally drops a byte (less than 1/20 frames), resulting
 * 		in distorted color and a false alarm.
 */

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "I2C.h"
#include "SPI.h"
#include "lcdCtrl.h"
#include "led.h"
// Custom HW
#include "ximage_filter.h"
#include "xaxivdma.h"

#define ALARM_THRESHOLD 21000  // Frame absolute difference threshold, in pixels

int ImageDiff(u8 *img1, u8 *img2);
int TestSPI();
int TestI2C();
void CustomHardwareInit(u8 *mem0, u8 *mem1);
void HardwareDiff(u8 *mem0, u8 *mem1);

XAxiVdma vdma0, vdma1;
XGpio fsync;
XImage_filter motionDetector;

int main()
{
    init_platform();
    int status, average = 0;
    u8 i = 0;

    status = InitLED();
	if (status != 0) {
		print ("LED init failed\n");
		return 1;
	}

    status = SPIInit();
	if (status != 0) {
		print ("SPI init failed\n");
		return 1;
	}

	status = I2CInit();
	if (status != 0) {
		print ("I2C init failed\n");
		return 1;
	}

    print("Camera comm initialized\n");

    if (TestSPI())
    	return XST_FAILURE;

    if (TestI2C())
    	return XST_FAILURE;

    ArduCamMiniInit();  // Via I2C

	// Allocate memory space for RGB565 image (display on screen)
	u8 *imgColor = (u8 *) malloc(320 * 240 * 2 * sizeof(u8));
	// Allocate memory space for grayscale image (for motion detection)
	u8 *imgGray[2];
	imgGray[0] = (u8 *) malloc(320 * 240 * sizeof(u8));
	imgGray[1] = (u8 *) malloc(320 * 240 * sizeof(u8));
	if (imgColor == NULL || imgGray[0] == NULL || imgGray[1] == NULL) {
		xil_printf("Failure to allocate image mem\n");
		return XST_FAILURE;
	}

	CustomHardwareInit(imgGray[0], imgGray[1]);

	LCDSPIInit();

	// Take two pictures to allow the camera to adjust exposure
	for (int init = 0; init < 2; init++) {
		ArduCamMiniCapture(imgColor, imgGray[init], &average);
		DrawCameraToLCD(imgColor);
	}

	for (;;) {
		ArduCamMiniCapture(imgColor, imgGray[i], &average);

		DrawCameraToLCD(imgColor);

		int diff = ImageDiff(imgGray[0], imgGray[1]);

		// Begin VDMA stream to motion detection HW
		HardwareDiff(imgGray[0], imgGray[1]);

		// Wait for data to stream
		//while(XAxiVdma_IsBusy(&vdma0, XAXIVDMA_READ) || XAxiVdma_IsBusy(&vdma1, XAXIVDMA_READ)) {xil_printf("Waiting..\n");}
		while (!XImage_filter_IsDone(&motionDetector)) {}


		u64 diffHW =  XImage_filter_Get_result(&motionDetector);

		xil_printf("\tDiff: %d\tHW Diff: %d\tAvg: %d\n", diff, diffHW, average);

		if (diff > ALARM_THRESHOLD) {
			LEDOn();
			xil_printf("\t\tALARM!\n");
		} else {
			LEDOff();
		}

		/* Reset fsync high for the next loop */
		XGpio_DiscreteWrite(&fsync, LED_CHANNEL, 0x3);

		i = !(i);
	}

    print("\nEnd of program - flushing UART\n");
    free(imgColor);
    free(imgGray[0]);
    free(imgGray[1]);
    cleanup_platform();
    return 0;
}

/*
 * int ImageDiff(u8 *img1, u8 *img2)
 *
 * This function takes the absolute, per-pixel difference of the two images
 * and returns the result.  Both images are expected to be in grayscale QVGA
 * 8-bit format.
 */
int ImageDiff(u8 *img1, u8 *img2)
{
	int diff = 0;
	u8 buffer1[320];
	u8 buffer2[320];
	for (int y = 0; y < 239; y++) {//debug 240
		memcpy(buffer1, (void *) (img1 + (320 * y)), 320 * sizeof(u8));
		memcpy(buffer2, (void *) (img2 + (320 * y)), 320 * sizeof(u8));
		for (int x = 0; x < 320; x++)
			diff = diff + abs(buffer1[x] - buffer2[x]);
	}
	return diff;
}

int TestSPI()
{
	u8 data;
	// Test SPI
	CameraSend(0, 55);  // Write 55 to register 0
	CameraRead(0, &data);  // We expect to read 55 back from reg 0
	//xil_printf("Res: %d\n", data);
	if (data != 55) {
		xil_printf("Error - cannot connect to camera via SPI\n");
		print("\nEnd of program - flushing UART\n");
		cleanup_platform();
		return XST_FAILURE;
	} else {
		return XST_SUCCESS;
	}
}

int TestI2C()
{
	// Test I2C
	u8 vid, pid;
	int status;
	status = WriteArduCamMini((u8) 0xFF, (u8) 0x01);
	if (status) {
		xil_printf("Error writing to camera\n");
	}
	//  OV2640_CHIPID_HIGH = 0x0A, OV2640_CHIPID_LOW = 0x0B
	status = ReadArduCamMini((u8) 0x0A, &vid);
	if (status) {
		xil_printf("Error reading from camera (1st)\n");
	}
	status = ReadArduCamMini((u8) 0x0B, &pid);
	if (status) {
		xil_printf("Error reading from camera (2nd)\n");
	}
	//  We expect them to be 0x26 and 0x42 for ArduCAM Mini
	if ((vid != 0x26) || (pid != 0x42)) {
		xil_printf("Error - cannot connect to camera via I2C\n");
		print("\nEnd of program - flushing UART\n");
		cleanup_platform();
		return XST_FAILURE;
	} else {
		return XST_SUCCESS;
	}
}

void HardwareDiff(u8 *mem0, u8 *mem1)
{
	int status;
	XAxiVdma_DmaSetup dmaOp0, dmaOp1;

    dmaOp0.VertSizeInput = 240;      /**< Vertical size input */
    dmaOp0.HoriSizeInput = 320;      /**< Horizontal size input */
    dmaOp0.Stride = 8;             /**< Stride */
    dmaOp0.FrameDelay = 0;         /**< Frame Delay */

    dmaOp0.EnableCircularBuf = 0;  /**< Circular Buffer Mode? */
    dmaOp0.EnableSync = 0;         /**< Gen-Lock Mode? */
    dmaOp0.PointNum = 0;           /**< Master we synchronize with */
    dmaOp0.EnableFrameCounter = 0; /**< Frame Counter Enable */
    dmaOp0.FixedFrameStoreAddr = 0;/**< Fixed Frame Store Address index */
    dmaOp0.GenLockRepeat = 0;      /**< Gen-Lock Repeat? */
    dmaOp0.FrameStoreStartAddr[0] = mem0;
                            /**< Start Addresses of Frame Store Buffers. (uintptr) */

	status = XAxiVdma_StartReadFrame(&vdma0, &dmaOp0);
	if (status != XST_SUCCESS)  {
		xil_printf("VDMA read failure\n");
		return XST_FAILURE;
	}

    dmaOp1.VertSizeInput = 240;      /**< Vertical size input */
    dmaOp1.HoriSizeInput = 320;      /**< Horizontal size input */
    dmaOp1.Stride = 8;             /**< Stride */
    dmaOp1.FrameDelay = 0;         /**< Frame Delay */

    dmaOp1.EnableCircularBuf = 0;  /**< Circular Buffer Mode? */
    dmaOp1.EnableSync = 0;         /**< Gen-Lock Mode? */
    dmaOp1.PointNum = 0;           /**< Master we synchronize with */
    dmaOp1.EnableFrameCounter = 0; /**< Frame Counter Enable */
    dmaOp1.FixedFrameStoreAddr = 0;/**< Fixed Frame Store Address index */
    dmaOp1.GenLockRepeat = 0;      /**< Gen-Lock Repeat? */
    dmaOp1.FrameStoreStartAddr[0] = mem1;

	status = XAxiVdma_StartReadFrame(&vdma1, &dmaOp1);
	if (status != XST_SUCCESS)  {
		xil_printf("VDMA read failure\n");
		return XST_FAILURE;
	}

	/* Set fsync low to start both (falling edge is activation) */
	XGpio_DiscreteWrite(&fsync, 1, 0x0);

	// Start the motion detection hardware
	XImage_filter_Set_rows(&motionDetector, 240);
	XImage_filter_Set_cols(&motionDetector, 320);
	XImage_filter_Start(&motionDetector);

}

void CustomHardwareInit(u8 *mem0, u8 *mem1)
{
	int status;


	XImage_filter_Initialize(&motionDetector, XPAR_XIMAGE_FILTER_0_DEVICE_ID);

	// Initialize GPIO for fsync signal
	status = XGpio_Initialize(&fsync, XPAR_FSYNC_DEVICE_ID);
	if (status != XST_SUCCESS)  {
		xil_printf("Fsync init failure\n");
		return XST_FAILURE;
	}
	/* Set the direction for all signals to be outputs */
	XGpio_SetDataDirection(&fsync, 1, 0x0);
	/* Set the GPIO outputs to high (falling edge is activation) */
	XGpio_DiscreteWrite(&fsync, 1, 0xF);

	XAxiVdma_Config *config0 = XAxiVdma_LookupConfig(XPAR_AXI_VDMA_0_DEVICE_ID);
	XAxiVdma_Config *config1 = XAxiVdma_LookupConfig(XPAR_AXI_VDMA_1_DEVICE_ID);

	config0->UseFsync = 0;
	config1->UseFsync = 0;

	status = XAxiVdma_CfgInitialize(&vdma0, config0, XPAR_AXI_VDMA_0_BASEADDR);
	if (status != XST_SUCCESS)  {
		xil_printf("vdma0 init failure\n");
		return XST_FAILURE;
	}

	status = XAxiVdma_CfgInitialize(&vdma1, config1, XPAR_AXI_VDMA_1_BASEADDR);
	if (status != XST_SUCCESS)  {
		xil_printf("vdma1 init failure\n");
		return XST_FAILURE;
	}

	XAxiVdma_SetFrmStore(&vdma0, 1, XAXIVDMA_READ);
	XAxiVdma_SetFrmStore(&vdma1, 1, XAXIVDMA_READ);

	//XAXIVDMA_READ,
	//XAxiVdma_FsyncSrcSelect(vdma0, XAXIVDMA_CHAN_FSYNC, XAXIVDMA_READ);
	//XAxiVdma_FsyncSrcSelect(vdma1, XAXIVDMA_CHAN_FSYNC, XAXIVDMA_READ);

	UINTPTR frame0[1];
	frame0[0] = mem0;
	UINTPTR frame1[1];
	frame1[0] = mem1;
	status = XAxiVdma_DmaSetBufferAddr(&vdma0, XAXIVDMA_READ, frame0);
	if (status != XST_SUCCESS)  {
		xil_printf("VDMA0 setbuffer failure\n");
		return XST_FAILURE;
	}
	XAxiVdma_DmaSetBufferAddr(&vdma1, XAXIVDMA_READ, frame1);
	if (status != XST_SUCCESS)  {
		xil_printf("VDMA1 setbuffer failure\n");
		return XST_FAILURE;
	}

	// Doesn't actually start the VDMA - need to wait for
	// falling edge of fsync signal
	XAxiVdma_DmaStart(&vdma0, XAXIVDMA_READ);
	XAxiVdma_DmaStart(&vdma1, XAXIVDMA_READ);
}


