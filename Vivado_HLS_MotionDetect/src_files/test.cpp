/***************************************************************************
Modified by Russell Barnes for ECE 253 Final Project, Fall 2016

Specify the input test images in the main funciton.  The resolution is
defined in top.h.

When different input images are used, some nonzero difference result is
expected.  Using the same image for both inputs should result in no
difference.

*   © Copyright 2013 Xilinx, Inc. All rights reserved.

*   This file contains confidential and proprietary information of Xilinx,
*   Inc. and is protected under U.S. and international copyright and other
*   intellectual property laws.

*   DISCLAIMER
*   This disclaimer is not a license and does not grant any rights to the
*   materials distributed herewith. Except as otherwise provided in a valid
*   license issued to you by Xilinx, and to the maximum extent permitted by
*   applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH
*   ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS,
*   EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES
*   OF MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR
*   PURPOSE; and (2) Xilinx shall not be liable (whether in contract or
*   tort, including negligence, or under any other theory of liability)
*   for any loss or damage of any kind or nature related to, arising under
*   or in connection with these materials, including for any direct, or any
*   indirect, special, incidental, or consequential loss or damage (including
*   loss of data, profits, goodwill, or any type of loss or damage suffered
*   as a result of any action brought by a third party) even if such damage
*   or loss was reasonably foreseeable or Xilinx had been advised of the
*   possibility of the same.

*   CRITICAL APPLICATIONS
*   Xilinx products are not designed or intended to be fail-safe, or for use
*   in any application requiring fail-safe performance, such as life-support
*   or safety devices or systems, Class III medical devices, nuclear facilities,
*   applications related to the deployment of airbags, or any other applications
*   that could lead to death, personal injury, or severe property or environmental
*   damage (individually and collectively, "Critical Applications"). Customer
*   assumes the sole risk and liability of any use of Xilinx products in Critical
*   Applications, subject only to applicable laws and regulations governing
*   limitations on product liability.

*   THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
*   ALL TIMES.

***************************************************************************/
#include "hls_opencv.h"
#include "top.h"
#include "opencv_top.h"
#include "image_utils.h"
#include <iostream>

using namespace cv;


int main (int argc, char** argv) {

    Mat src_rgb1 = imread("../../../../src_files/img1.bmp");
    Mat src_rgb2 = imread("../../../../src_files/img2.bmp");
    if (!src_rgb1.data || !src_rgb2.data) {
        printf("ERROR: could not open or find the input image(s)!\n");
        return -1;
    }
    unsigned long long result, resultGolden;

    // HLS test
    cv::Mat src_gray1(src_rgb1.rows, src_rgb1.cols, CV_8UC1);
    cv::Mat src_gray2(src_rgb2.rows, src_rgb2.cols, CV_8UC1);

    cvtColor(src_rgb1, src_gray1, CV_RGB2GRAY);
    cvtColor(src_rgb2, src_gray2, CV_RGB2GRAY);

    IplImage src1 = src_gray1;
    IplImage src2 = src_gray2;

    hls_image_filter(&src1, &src2, &result);


    // CPU OpenCV test
    cv::Mat src_gray1_cv(src_rgb1.rows, src_rgb1.cols, CV_8UC1);
    cv::Mat src_gray2_cv(src_rgb2.rows, src_rgb2.cols, CV_8UC1);

    cvtColor(src_rgb1, src_gray1_cv, CV_RGB2GRAY);
    cvtColor(src_rgb2, src_gray2_cv, CV_RGB2GRAY);

    IplImage src1b = src_gray1_cv;
    IplImage src2b = src_gray2_cv;

    opencv_image_filter(&src1b, &src2b, &resultGolden);

    std::cout << "Result: " << result << "\nResultGolden: " << resultGolden;
    unsigned int src1b_pix = CV_IMAGE_ELEM(&src1b, uchar, 80, (120 * 1) + 1);
    unsigned int src2b_pix = CV_IMAGE_ELEM(&src2b, uchar, 80, (120 * 1) + 1);
    std::cout << "\n\nImg1 pix: " << src1b_pix << "\nImg2 pix: " << src2b_pix << "\n";

    imwrite("../../../../src_files/img1_gray.bmp", src_gray1);
    imwrite("../../../../src_files/img2_gray.bmp", src_gray2);

    if (resultGolden == result)
    	return 0;
    else
    	return 1;
}
