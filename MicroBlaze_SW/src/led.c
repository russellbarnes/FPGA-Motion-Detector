/*
 * led.c
 *
 *  Created on: Dec 8, 2016
 *      Author: russell
 */

#include "led.h"
#include "xparameters.h"
#include "xgpio.h"
#include "stdio.h"
#include "xstatus.h"

XGpio leds;

int InitLED()
{
	int Status;

	/*
	 * Initialize the GPIO driver so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h
	 * (expecting LEDs to be GPIO dev 0 - change the value if needed)
	 */
	Status = XGpio_Initialize(&leds, XPAR_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS)  {
		xil_printf("LED failure\n");
		return XST_FAILURE;
	}

	/* Set the direction for all signals to be outputs */
	XGpio_SetDataDirection(&leds, LED_CHANNEL, 0x0);


	/* Set the GPIO outputs to low */
	XGpio_DiscreteWrite(&leds, LED_CHANNEL, 0x0);

	return XST_SUCCESS;
}

void LEDOn()
{
	// Turn on all LEDs
	XGpio_DiscreteWrite(&leds, LED_CHANNEL,	0xFFFF);
}

void LEDOff()
{
	// Turn on all LEDs
	XGpio_DiscreteWrite(&leds, LED_CHANNEL,	0x0000);
}

