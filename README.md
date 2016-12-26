# FPGA-Motion-Detector

This was my final project for ECE 253 Embedded System Design at UCSB.

![Project hardware](https://github.com/russellbarnes/FPGA-Motion-Detector/raw/master/project_hw.jpeg "Project hardware")

The FPGA takes pictures with the camera and performs motion detection in both software and in [hardware-accelerated OpenCV](http://www.wiki.xilinx.com/HLS+Video+Library).

![Block design](https://github.com/russellbarnes/FPGA-Motion-Detector/raw/master/block_design.png "Block design")

## System Configuration
- [__Nexys4 DDR__](http://store.digilentinc.com/nexys-4-ddr-artix-7-fpga-trainer-board-recommended-for-ece-curriculum/) development board including a Xilinx Artix-7 FPGA
	- FPGA block design includes AXI IIC, 1-channel GPIO (x2), 16-channel GPIO for LEDs, AXI Quad SPI (x2), AXI VDMA (x2), custom HLS block (included), MIG, interrupt controller, MicroBlaze soft processor, others
- [__ArduCAM-Mini-2MP__](http://www.arducam.com/arducam-mini-released/) camera with both I2C and SPI interfaces connected
- [__240x320 2.2" TFT display w/ILI9340C__](https://www.adafruit.com/product/1480) by Adafruit
- Memory configuration
	- MicroBlaze is utilizing 128K of BRAM
	- Stack is located in BRAM.  Stack size: 0x800
	- Heap is located in DDR memory via MIG.  Heap size: 0x80000
- Camera SPI @ < 8 MHz, display SPI @ 5 MHz, Camera I2C @ 100 KHz
- SPI chip select signal for camera is *replaced* by single-channel "spi_dc_1" GPIO
- Additional spi_dc output for display "D/C" signal is single-channel GPIO
- Current camera configuration is for QVGA bitmap output format.  To save to a file, a bitmap header must be added to the beginning (not included - see ArduCAM example code)
- This application configures UART to baud rate 9600 for communication with the host PC.
- Camera driver is based on ArduCAM example code:
	http://www.arducam.com/
	https://github.com/ArduCAM/Arduino

_Also uses UTFT.cpp - Multi-Platform library support for Color TFT LCD Boards.  Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved._

The OpenCV HLS block should be created with the Xilinx HLS program using the code in __Vivado_HLS_MotionDetect__.  Update the code for desired resolution (QVGA for this project) and validate with corresponding input images.  

![HLS connections](https://github.com/russellbarnes/FPGA-Motion-Detector/raw/master/HLS_IP_block.png "HLS connections")

After exporting the hardware design, use Vivado to create the above FPGA block design.  The HLS block requires two AXI VDMA controllers in order to receive the input image streams.  Run the program in __MicroBlaze_SW__ on the FPGA's MicroBlaze processor using Xilinx SDK.

## Known issues:
- Camera SPI occasionally drops a byte (less than 1/20 frames), resulting in distorted color and a false alarm.
- In the program, the hardware motion detection block's VDMA streams are not initialized to supply the whole frame, resulting in a numerical discrepancy between the software and hardware motion detections.  The hardware block can still be thresholded to perform motion detection, but part of the image may be ignored.

## Next Step
The motion detection can be improved by performing edge detection, reducing luminance sensitivity.  This would be more easily implemented in the [hardware block](https://github.com/Xilinx/HLx_Examples/tree/master/Vision/video_edge) than in software.
