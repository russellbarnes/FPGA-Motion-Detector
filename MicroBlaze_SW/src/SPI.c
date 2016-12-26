/*
 * SPI.c
 *
 *  Created on: Dec 4, 2016
 *      Author: russell
 */

#include "SPI.h"
#include "xil_printf.h"
#include "xgpio.h"

XSpi SPI;  // The camera SPI controller
XGpio csel;  // The chip select signal (must be controlled manually)
#define CSEL_CHANNEL 1

int ArduCamMiniCapture(u8 *img, u8 *imgGray, int *avg)
{
	u8 done, burstCmd, temp, prev, k = 1;
	u16 j = 0, m = 0;
	u8 buffer[256];  // BRAM buffer
	u8 grayBuffer[128];  // BRAM buffer
	int i = 0, h = 0, average = 0;

	// Flush the ArduCAM FIFO
	CameraSend(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
	//  Clear the capture done flag (not sure why twice, just following example)
	CameraSend(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
	//  Start capture
	CameraSend(ARDUCHIP_FIFO, FIFO_START_MASK);

	// Wait for capture
	do {
		CameraRead(ARDUCHIP_TRIG, &done);
		done = done & CAP_DONE_MASK;
	} while (!done);

	// If saving to a file instead, must write bmp_header data first (see ArduCAM source)

	// Begin burst read from FIFO (exclusive to ArduCAM Mini)
	// This effectively doubles transfer speed - no need to send new request per byte
	XGpio_DiscreteWrite(&csel, CSEL_CHANNEL, 0);  // Activate chip select
	burstCmd = BURST_FIFO_READ;
	ErrorCheck(SPIWrite8(&burstCmd));  // Send burst read command
	ErrorCheck(CameraBurstRead(&temp));  // Throw away first dummy byte
	while (i < (320 * 240 * 2)) {  // i < 153,600
		/*if (!((i) % 38400)) {
			xil_printf("Loading... (%d/4)\n", k++);
		}*/
		prev = temp;
		// Read 8 bytes of BMP image
		CameraBurstRead(&temp);
		// Use a buffer in BRAM for better performance
		buffer[j] = temp;

		j = j + 1;
		i = i + 1;
		if (j > 255) {
			// Copy buffer to DRAM
			memcpy((void *) (img + (i - 256)), buffer, 256 * sizeof(u8));
			j = 0;
		}

		if (!(i % 2)) {
			// Convert to grayscale.
			// Use simple averaging because it isn't being displayed to humans.
			// The values remain normalized to 5 bits instead of 8 for the same reason.
			u8 r = (prev & 0xF8) >> 3;
			u8 g = ((prev & 0x07) << 2) | ((temp & 0xE0) >> 6);  // Eliminate LSB
			u8 b = (temp & 0x1F);
			u8 grayPix = (r + g + b) / 3;

			// Accumulate average
			average = average + grayPix;

			// Store pixel
			grayBuffer[m] = grayPix;
			m = m + 1;
			h = h + 1;

			if (m > 127) {
				// Copy buffer to DRAM
				memcpy((void *) (imgGray + (h - 128)), grayBuffer, 128 * sizeof(u8));
				m = 0;
			}
		}
	}
	XGpio_DiscreteWrite(&csel, CSEL_CHANNEL, 1);

	// Save the last of the data
	memcpy((void *) (img + (i - j)),		buffer,		j * sizeof(u8));
	memcpy((void *) (imgGray + (h - m)),	grayBuffer,	m * sizeof(u8));

	// Find average pixel value (normalized to 5 bits)
	*avg = average / 76800;  // 320 * 240 = 76800

	xil_printf("Capture complete\n");

	return XST_SUCCESS;
}

int SPIWrite8(u8 *sendBuf)
{
	u8 recvBuf;
	int result;
	// Format:
	// int XSpi_Transfer(XSpi *InstancePtr, u8 *SendBufPtr, u8 *RecvBufPtr, unsigned int ByteCount);
	result = XSpi_Transfer(&SPI, sendBuf, &recvBuf, (unsigned int) 1);
	//xil_printf("recvBuf: %d\n", recvBuf);
	return result;
}

int SPIRead8(u8 *recvBuf)
{
	u8 junk = 0;
	// Format:
	// int XSpi_Transfer(XSpi *InstancePtr, u8 *SendBufPtr, u8 *RecvBufPtr, unsigned int ByteCount);
	return XSpi_Transfer(&SPI, &junk, recvBuf, (unsigned int) 1);
}


// Port of ArduCAM::write_reg
int CameraSend(u8 addr, u8 data)
{
	addr = addr | 0x80;
	u8 busy;

	do {
		busy = SPI.IsBusy;
	} while (busy);

	XGpio_DiscreteWrite(&csel, CSEL_CHANNEL, 0);

	if (ErrorCheck(SPIWrite8(&addr))) {
		xil_printf("\t(during CameraSend(), addr send portion)\n");
		return 1;
	}

	do {
		busy = SPI.IsBusy;
	} while (busy);

	if (ErrorCheck(SPIWrite8(&data))) {
		xil_printf("\t(during CameraSend(), data send portion)\n");
		return 1;
	}

	do {
		busy = SPI.IsBusy;
	} while (busy);


	XGpio_DiscreteWrite(&csel, CSEL_CHANNEL, 1);

	return XST_SUCCESS;  // = 0
}

// Port of ArduCAM::read_reg
int CameraRead(u8 addr, u8 *dataBuf)
{
	addr = addr & 0x7F;
	u8 busy;

	do {
		busy = SPI.IsBusy;
	} while (busy);

	XGpio_DiscreteWrite(&csel, CSEL_CHANNEL, 0);

	if (ErrorCheck(SPIWrite8(&addr))) {
		xil_printf("\t(error during CameraRead(), addr send portion)\n");
		return 1;
	}

	do {
		busy = SPI.IsBusy;
	} while (busy);

	if (ErrorCheck(SPIRead8(dataBuf))) {
		xil_printf("\t(error during CameraRead(), data recv portion)\n");
		return 1;
	}

	do {
		busy = SPI.IsBusy;
	} while (busy);

	XGpio_DiscreteWrite(&csel, CSEL_CHANNEL, 1);

	return XST_SUCCESS;  // = 0
}

/*
 * Port of SPI.transfer
 * Expects burst to be initialized and CS signal to be
 * handled externally.
 */
int CameraBurstRead(u8 *dataBuf)
{
	u8 busy;

	do {
		busy = SPI.IsBusy;
	} while (busy);

	if (ErrorCheck(SPIRead8(dataBuf))) {
		xil_printf("\t(error during CameraBurstRead())\n");
		return 1;
	}

	return XST_SUCCESS;  // = 0
}

int SPIInit()
{
	int Status;
	u16 DeviceId = XPAR_SPI_CAMERA_DEVICE_ID;  // Defined in xparameters.h
	XSpi_Config *spiConfig;	/* Pointer to Configuration data */
	u32 controlReg;


	/*
	 * Initialize the GPIO to make our own chip select signal
	 * (recommend to set the GPIO's default value to 1 in block design)
	 */
	Status = XGpio_Initialize(&csel, XPAR_SPI_DC_1_DEVICE_ID);
	if (Status != XST_SUCCESS)  {
		xil_printf("Initialize GPIO chip select failure!\n");
		return XST_FAILURE;
	}
	XGpio_SetDataDirection(&csel, 1, 0x0);  // Set as output
	XGpio_DiscreteWrite(&csel, CSEL_CHANNEL, 1);  // Deselect chip for now

	/*
	 * Initialize the SPI driver so that it is  ready to use.
	 */
	spiConfig = XSpi_LookupConfig(DeviceId);
	if (spiConfig == NULL) {
		xil_printf("Can't find SPI device!\n");
		return XST_DEVICE_NOT_FOUND;
	}

	Status = XSpi_CfgInitialize(&SPI, spiConfig, spiConfig->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialize SPI - Failure!\n");
		return XST_FAILURE;
	}

	/*
	 * Reset the SPI device to leave it in a known good state.
	 */
	XSpi_Reset(&SPI);

	/*
	 * Setup the control register to enable master mode
	 */
	controlReg = XSpi_GetControlReg(&SPI);
	XSpi_SetControlReg(&SPI, (controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK | XSP_CR_MANUAL_SS_MASK) &
							 (~XSP_CR_TRANS_INHIBIT_MASK));

	// Select 1st slave device (there can only be ONE)
	XSpi_SetSlaveSelect(&SPI, 0x0001);

	// Start the device

	Xil_AssertNonvoid(SPI.IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * If it is already started, return a status indicating so.
	 */
	if (SPI.IsStarted == XIL_COMPONENT_IS_STARTED) {
		return XST_DEVICE_IS_STARTED;
	}

	/*
	 * Indicate that the device is started before we enable the transmitter
	 * or receiver or interrupts.
	 */
	SPI.IsStarted = XIL_COMPONENT_IS_STARTED;

	return XST_SUCCESS;
}

int ErrorCheck(int status)
{
	if (status == XST_DEVICE_IS_STOPPED) {
		xil_printf("\nSPI ERROR: Device stopped\n");
		return 1;
	} else if (status == XST_DEVICE_BUSY) {
		xil_printf("\nSPI ERROR: Device busy\n");
		return 1;
	} else if (status == XST_SPI_NO_SLAVE) {
		xil_printf("\nSPI ERROR: Ya didn't select a slave!)\n");
		return 1;
	} else if (status != XST_SUCCESS) {
		xil_printf("\nSPI ERROR: Unknown error (result != XST_SUCCESS!\n");
		return 1;
	}
	return 0;
}
