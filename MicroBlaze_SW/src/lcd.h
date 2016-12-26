/*
 * lcd.h
 *
 *  Created on: Oct 21, 2015
 *      Author: atlantis
 */

/*
  UTFT.h - Multi-Platform library support for Color TFT LCD Boards
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

#ifndef LCD_H_
#define LCD_H_

#include "xparameters.h"
#include "xil_io.h"
#include "xil_types.h"
#include "xspi_l.h"
#include "xil_printf.h"

#define SPI_DC          XPAR_SPI_DC_BASEADDR
#define B_RS            0x00000001

#define SPI_DTR         XPAR_SPI_BASEADDR + XSP_DTR_OFFSET
#define SPI_DRR         XPAR_SPI_BASEADDR + XSP_DRR_OFFSET
#define SPI_IISR        XPAR_SPI_BASEADDR + XSP_IISR_OFFSET
#define SPI_SR          XPAR_SPI_BASEADDR + XSP_SR_OFFSET

#define cbi(reg, bitmask)       Xil_Out32(reg, Xil_In32(reg) & ~(u32)bitmask)
#define sbi(reg, bitmask)       Xil_Out32(reg, Xil_In32(reg) |= (u32)bitmask)
#define swap(type, i, j)        {type t = i; i = j; j = t;}

#define DISP_X_SIZE     239
#define DISP_Y_SIZE     329

struct _current_font
{
    u8* font;
    u8 x_size;
    u8 y_size;
    u8 offset;
    u8 numchars;
};

extern int fch; // Foreground color upper byte
extern int fcl; // Foreground color lower byte
extern int bch; // Background color upper byte
extern int bcl; // Background color lower byte

extern struct _current_font cfont;
extern u8 SmallFont[];
extern u8 BigFont[];
extern u8 SevenSegNumFont[];

u32 LCD_Read(char VL);
void LCD_Write_COM(char VL);  
void LCD_Write_DATA(char VL);
void LCD_Write_DATA16(char VH, char VL);
//void LCD_Write_DATA_(char VH, char VL);

void initLCD(void);
void setXY(int x1, int y1, int x2, int y2);
void setColor(u8 r, u8 g, u8 b);
void setColorBg(u8 r, u8 g, u8 b);
void clrXY(void);
void clrScr(void);

void drawHLine(int x, int y, int l);
void fillRect(int x1, int y1, int x2, int y2);
void drawImageLine(int x, u8 *buffer);  // Russell's addition (for displaying RGB565 image)
void DrawCircle(int x0, int y0, int radius);  // Russell's addition
void DrawTriangle(int x0, int y0, int width, int inverted);  // Russell's addition
void UpdateVolumeBar(u8 vol);  // Russell's addition
void DrawFace();  // Russell's addition
void VGradients();  // Russell's addition

void setFont(u8* font);
void printChar(u8 c, int x, int y);
void lcdPrint(char *st, int x, int y);

#endif /* LCD_H_ */
