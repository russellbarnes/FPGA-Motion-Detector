/*
 * lcd.c
 *
 *  Created on: Oct 21, 2015
 *      Author: atlantis
 */

/*
  UTFT.cpp - Multi-Platform library support for Color TFT LCD Boards
  Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
  
  This library is the continuation of my ITDB02_Graph, ITDB02_Graph16
  and RGB_GLCD libraries for Arduino and chipKit. As the number of 
  supported display modules and controllers started to increase I felt 
  it was time to make a single, universal library as it will be much 
  easier to maintain in the future.

  Basic functionality of this library was origianlly based on the 
  demo-code provided by ITead studio (for the ITDB02 modules) and 
  NKC Electronics (for the RGB GLCD module/shield).

  This library supports a number of 8bit, 16bit and serial graphic 
  displays, and will work with both Arduino, chipKit boards and select 
  TI LaunchPads. For a full list of tested display modules and controllers,
  see the document UTFT_Supported_display_modules_&_controllers.pdf.

  When using 8bit and 16bit display modules there are some 
  requirements you must adhere to. These requirements can be found 
  in the document UTFT_Requirements.pdf.
  There are no special requirements when using serial displays.

  You can find the latest version of the library at 
  http://www.RinkyDinkElectronics.com/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the 
  examples and tools supplied with the library.
*/

#include "lcd.h"

// Global variables
int fch;
int fcl;
int bch;
int bcl;
struct _current_font cfont;


// Read data from LCD controller
// FIXME: not work
u32 LCD_Read(char VL)
{
    u32 retval = 0;
    int index = 0;

    Xil_Out32(SPI_DC, 0x0);
    Xil_Out32(SPI_DTR, VL);
    
    //while (0 == (Xil_In32(SPI_SR) & XSP_SR_TX_EMPTY_MASK));
    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
    Xil_Out32(SPI_DC, 0x01);

    while (1 == (Xil_In32(SPI_SR) & XSP_SR_RX_EMPTY_MASK));
    xil_printf("SR = %x\n", Xil_In32(SPI_SR));


    while (0 == (Xil_In32(SPI_SR) & XSP_SR_RX_EMPTY_MASK)) {
       retval = (retval << 8) | Xil_In32(SPI_DRR);
       xil_printf("receive %dth byte\n", index++);
    }

    xil_printf("SR = %x\n", Xil_In32(SPI_SR));
    xil_printf("SR = %x\n", Xil_In32(SPI_SR));
    return retval;
}


// Write command to LCD controller
void LCD_Write_COM(char VL)  
{
    Xil_Out32(SPI_DC, 0x0);
    Xil_Out32(SPI_DTR, VL);
    
    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Write 16-bit data to LCD controller
void LCD_Write_DATA16(char VH, char VL)
{
    Xil_Out32(SPI_DC, 0x01);
    Xil_Out32(SPI_DTR, VH);
    Xil_Out32(SPI_DTR, VL);

    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Write 8-bit data to LCD controller
void LCD_Write_DATA(char VL)
{
    Xil_Out32(SPI_DC, 0x01);
    Xil_Out32(SPI_DTR, VL);

    while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK));
    Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}


// Initialize LCD controller
void initLCD(void)
{
    int i;

    // Reset
    LCD_Write_COM(0x01);
    for (i = 0; i < 500000; i++); //Must wait > 5ms


    LCD_Write_COM(0xCB);
    LCD_Write_DATA(0x39);
    LCD_Write_DATA(0x2C);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x34);
    LCD_Write_DATA(0x02);

    LCD_Write_COM(0xCF); 
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0XC1);
    LCD_Write_DATA(0X30);

    LCD_Write_COM(0xE8); 
    LCD_Write_DATA(0x85);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x78);

    LCD_Write_COM(0xEA); 
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x00);
 
    LCD_Write_COM(0xED); 
    LCD_Write_DATA(0x64);
    LCD_Write_DATA(0x03);
    LCD_Write_DATA(0X12);
    LCD_Write_DATA(0X81);

    LCD_Write_COM(0xF7); 
    LCD_Write_DATA(0x20);
  
    LCD_Write_COM(0xC0);   //Power control 
    LCD_Write_DATA(0x23);  //VRH[5:0] 
 
    LCD_Write_COM(0xC1);   //Power control 
    LCD_Write_DATA(0x10);  //SAP[2:0];BT[3:0] 

    LCD_Write_COM(0xC5);   //VCM control 
    LCD_Write_DATA(0x3e);  //Contrast
    LCD_Write_DATA(0x28);
 
    LCD_Write_COM(0xC7);   //VCM control2 
    LCD_Write_DATA(0x86);  //--
 
    LCD_Write_COM(0x36);   // Memory Access Control 
    LCD_Write_DATA(0x48);  

    LCD_Write_COM(0x3A);   
    LCD_Write_DATA(0x55);

    LCD_Write_COM(0xB1);   
    LCD_Write_DATA(0x00); 
    LCD_Write_DATA(0x18);
 
    LCD_Write_COM(0xB6);   // Display Function Control 
    LCD_Write_DATA(0x08);
    LCD_Write_DATA(0x82);
    LCD_Write_DATA(0x27); 

    LCD_Write_COM(0x11);   //Exit Sleep 
    for (i = 0; i < 100000; i++);
                
    LCD_Write_COM(0x29);   //Display on 
    LCD_Write_COM(0x2c); 

    //for (i = 0; i < 100000; i++);

    // Default color and fonts
    fch = 0xFF;
    fcl = 0xFF;
    bch = 0x00;
    bcl = 0x00;
    setFont(SmallFont);
}


// Set boundary for drawing
void setXY(int x1, int y1, int x2, int y2)
{
    LCD_Write_COM(0x2A); 
    LCD_Write_DATA(x1 >> 8);
    LCD_Write_DATA(x1);
    LCD_Write_DATA(x2 >> 8);
    LCD_Write_DATA(x2);
    LCD_Write_COM(0x2B); 
    LCD_Write_DATA(y1 >> 8);
    LCD_Write_DATA(y1);
    LCD_Write_DATA(y2 >> 8);
    LCD_Write_DATA(y2);
    LCD_Write_COM(0x2C);
}


// Remove boundry
void clrXY(void)
{
    setXY(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}


// Set foreground RGB color for next drawing
void setColor(u8 r, u8 g, u8 b)
{
    // 5-bit r, 6-bit g, 5-bit b
    fch = (r & 0x0F8) | g >> 5;
    fcl = (g & 0x1C) << 3 | b >> 3;
}


// Set background RGB color for next drawing
void setColorBg(u8 r, u8 g, u8 b)
{
    // 5-bit r, 6-bit g, 5-bit b
    bch = (r & 0x0F8) | g >> 5;
    bcl = (g & 0x1C) << 3 | b >> 3;
}


// Clear display
void clrScr(void)
{
    // Black screen
    setColor(0, 0, 0);

    fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}

/*
 * Draws 1 scanline of an image
 *
 * This is the full length of the screen (320 px).
 *
 * bh and bl contain the high and low byte of an
 * rgb565 signal, respectively.
 *
 * Each bh and bl array must be 320 elements long.
 *
 */
void drawImageLine(int x, u8 *buffer)
{
    u16 i;

    setXY(x, 0, x, 320 - 1);
    for (i = 0; i < 640; i = i + 2) {
    	LCD_Write_DATA16(buffer[i], buffer[i + 1]);
    }

    //clrXY();
}

// Draw horizontal line
void drawHLine(int x, int y, int l)
{
    int i;

    if (l < 0) {
        l = -l;
        x -= l;
    }

    setXY(x, y, x + l, y);
    for (i = 0; i < l + 1; i++) {
        LCD_Write_DATA16(fch, fcl);
    }

    clrXY();
}

// Draw horizontal line
void drawVLine(int x, int y, int l)
{
    int i;

    if (l < 0) {
        l = -l;
        y -= l;
    }

    setXY(x, y, x, y + l);
    for (i = 0; i < l + 1; i++) {
        LCD_Write_DATA16(fch, fcl);
    }

    clrXY();
}

// Fill using vertical lines
void VGradients()
{
	u8 x = 20, y = 20, red = 240;
	//240, -12
	int h = 280;
	int w = 200;
	for (int i = 0; i < 20; i++) {
		setColor(red, 0, 0);
		drawVLine(x, y, h);  // Left
		drawVLine(240 - x, y, h);  // Right
		drawHLine(x, y, w);  // Top
		drawHLine(x, y + h, w);  // Bottom
		x--;
		y--;
		h = h + 2;
		w = w + 2;
		red = red - 12;
	}
}

// Fill a rectangular 
void fillRect(int x1, int y1, int x2, int y2)
{
    int i;

    if (x1 > x2)
        swap(int, x1, x2);

    if (y1 > y2)
        swap(int, y1, y2);

    setXY(x1, y1, x2, y2);
    for (i = 0; i < (x2 - x1 + 1) * (y2 - y1 + 1); i++) {
        LCD_Write_DATA16(fch, fcl);
    }

   clrXY();
}

/* Draw a right triangle (no rotation)
 * Peak @ (x0, y0), base width: width
 * Inverted: puts point at the bottom
 */
void DrawTriangle(int x0, int y0, int width, int inverted)
{
	int lineWidth = 1, x = x0, y = y0, dir;
	if (inverted)
		dir = -1;
	else
		dir = 1;

	while (lineWidth <= width) {
		drawHLine(x, y, lineWidth);
		lineWidth = lineWidth + 2;
		y = y + dir;
		x--;
	}

	return;
}

/*
 * Update the volume bar
 * This will erase rects if volume decreases
 *
 * vol (input) represents the number of blocks to draw.
 */
void UpdateVolumeBar(u8 vol)
{
	static u8 volBlocks = 0;  // Volume shown, not actual value!

	if (vol > volBlocks) {
		// Add more volume bars

		setColor(0, 255, 0);  // Green for blocks

		for (int i = volBlocks; i < vol; i++) {
			// Draw volume block
			fillRect(26 + (i * 12), 250,
					26 + (i * 12) + 8, 260);
		}
	} else if (vol < volBlocks) {
		// Erase some volume bars

		setColor(255, 0, 0);  // Red for background

		for (int i = volBlocks - 1; i > vol - 1; i--) {
			// Draw background over volume block
			fillRect(26 + (i * 12), 250,
					26 + (i * 12) + 8, 260);
		}
	}

	volBlocks = vol;
	return;
}

/*
 * Draws a smiling face for the background
 */
void DrawFace()
{
	// Head
	setColor(234, 192, 134);//skin
	DrawCircle(120, 120, 50);

	// Smile
	setColor(255, 255, 255);//mouth
	DrawCircle(120, 134, 25);
	setColor(234, 192, 134);//skin
	fillRect(120 - 25, 134 - 25, 120 + 25, 134);

	// Eyes
	setColor(255, 255, 255);//white
	DrawCircle(100, 110, 8);
	DrawCircle(140, 110, 8);
	setColor(0, 0, 255);//blue
	DrawCircle(100, 110, 4);
	DrawCircle(140, 110, 4);
	setColor(0, 0, 0);//black
	DrawCircle(100, 110, 2);
	DrawCircle(140, 110, 2);
}

/*
 * Draw a circle at (x0, y0) with radius.
 *
 * Thanks to Wikipedia, Pitteway, Van Aken, Bresenham and StackOverflow
 * user AakashM for this code:
 * https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 * http://stackoverflow.com/questions/1201200/fast-algorithm-for-drawing-filled-circles
 */
void DrawCircle(int x0, int y0, int radius)
{
	int x = radius;
	int y = 0, err = 0;

	while (x >= y) {
		drawHLine(x0 - x, y0 + y, 2 * x);

		drawHLine(x0 - y, y0 + x, 2 * y);

		drawHLine(x0 - x, y0 - y, 2 * x);

		drawHLine(x0 - y, y0 - x, 2 * y);

		y += 1;
		err += 1 + (2 * y);
		if (2 * (err - x) + 1 > 0) {
			x -= 1;
			err += 1 - (2 * x);
		}
	}
	return;
}


// Select the font used by print() and printChar()
void setFont(u8* font)
{
	cfont.font=font;
	cfont.x_size = font[0];
	cfont.y_size = font[1];
	cfont.offset = font[2];
	cfont.numchars = font[3];
}


// Print a character
void printChar(u8 c, int x, int y)
{
    u8 ch;
    int i, j, pixelIndex;


    setXY(x, y, x + cfont.x_size - 1,y + cfont.y_size - 1);

    pixelIndex =
            (c - cfont.offset) * (cfont.x_size >> 3) * cfont.y_size + 4;
    for(j = 0; j < (cfont.x_size >> 3) * cfont.y_size; j++) {
        ch = cfont.font[pixelIndex];
        for(i = 0; i < 8; i++) {   
            if ((ch & (1 << (7 - i))) != 0)   
                LCD_Write_DATA16(fch, fcl);
            else
                LCD_Write_DATA16(bch, bcl);
        }
        pixelIndex++;
    }

    clrXY();
}


// Print string
void lcdPrint(char *st, int x, int y)
{
    int i = 0;

    while(*st != '\0')
        printChar(*st++, x + cfont.x_size * i++, y);
}
