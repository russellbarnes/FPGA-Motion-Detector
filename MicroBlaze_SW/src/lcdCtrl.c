/*
 * lcd.c
 *
 *  Created on: Oct 31, 2016
 *      Author: russell
 */
#include "lcdCtrl.h"

#include "xspi.h"
#include "xspi_l.h"
#include "lcd.h"		// LCD driver
/**** Interrupt section ****/
#include "I2C.h" // Contains interrupt controller

XSpi_Config *spiConfig;	/* Pointer to Configuration data */
static XSpi spi;
static XGpio lcdGpio;	/* The GPIO driver for LCD SPI */

/*
 * Accepts a u8* buffer in RGB565 format
 * Converts this to RGB and then displays
 */
void DrawCameraToLCD(u8 *img)
{
	u16 y = 0;
	u8 buffer[320 * 2];  // One scan line in RGB565

	initLCD();
	//clrScr();  // Takes some time

	for (y = 0; y < 240; y++) {
		// Load 320 px (one row) from memory
		memcpy(buffer, (void *) (img + (320 * 2 * y)), 320 * 2 * sizeof(u8));
		// Data is already in RGB565 format, so no need to convert it.
		// Draw to display
		drawImageLine((240 - y) - 1, buffer);
	}
}

/*
 * SPI
 */
int LCDSPIInit()
{
	u32 status;
	u32 controlReg;

	// Initialize the GPIO driver
	status = XGpio_Initialize(&lcdGpio, XPAR_SPI_DC_DEVICE_ID);
	if (status != XST_SUCCESS)  {
		xil_printf("Initialize GPIO SPI dc FAIL!\n");
		return XST_FAILURE;
	}

	// Set the direction for all signals to be outputs
	XGpio_SetDataDirection(&lcdGpio, 1, 0x0);

	// Initialize the SPI driver
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	if (spiConfig == NULL) {
		xil_printf("Can't find SPI device!\n");
		return XST_DEVICE_NOT_FOUND;
	}

	status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (status != XST_SUCCESS) {
		xil_printf("Initialize spi fail!\n");
		return XST_FAILURE;
	}

	/*
	 * Reset the SPI device to leave it in a known good state.
	 */
	XSpi_Reset(&spi);

	/*
	 * Setup the control register to enable master mode
	 */
	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
		(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
		(~XSP_CR_TRANS_INHIBIT_MASK));

	// Select 1st slave device
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	return status;
}
