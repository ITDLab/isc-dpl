// Copyright 2023 ITD Lab Corp.All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file ISCFrameDecoder.h
 * @brief Implementation of Frame Decoder processing
 */

#pragma once

/**
 * @class   ISCFrameDecoder
 * @brief   implementation class
 * this class is an implementation of Frame Decoder processing
 */
class ISCFrameDecoder
{

public:

	/** @brief initialize the frame decoder.
		@return none.
	 */
	static void initialize(int imghgt, int imgwdt);

	/** @brief configure use of OpenCL for parallax averaging.
		@return none.
	 */
	static void finalize();

	/** @brief set decoder parameters.
		@return none.
	 */
	static void setFrameDecoderParameter(int crstthr, int grdcrct);

	/** @brief set camera matching  parameters.
		@return none.
	 */
	static void setCameraMatchingParameter(int mtchgt, int mtcwdt);

	/** @brief set the upper and lower limits of parallax.
		@return none.
	 */
	static void setDisparityLimitation(int limit, double lower, double upper);

	/** @brief set the parameter for double shutter mode.
		@return none.
	 */
	static void setDoubleShutterOutput(int dbdout, int dbcout);

	/** @brief split frame data into image data.
		@return none.
	 */
	static void decodeFrameData(int imghgt, int imgwdt, unsigned char* pfrmdat,
		unsigned char* prgtimg, unsigned char* plftimg);

	/** @brief split frame data into image data.
		@return none.
	 */
	static void decodeFrameData(int imghgt, int imgwdt, unsigned char* pfrmdat,
		unsigned short* prgtimg, unsigned short* plftimg);

	/** @brief Decode disparity encoded data, perform disparity averaging and completion processing.
		@return none.
	 */
	static void getDisparityData(int imghgt, int imgwdt, unsigned char* prgtimg, unsigned char* pdspenc, int frmgain,
		int* pblkhgt, int* pblkwdt, int* pmtchgt, int* pmtcwdt,
		int* pblkofsx, int* pblkofsy, int* pdepth, int* pshdwdt,
		unsigned char* pdspimg, float* ppxldsp, float* pblkdsp, int *pblkval, int *pblkcrst);

	/** @brief Decode disparity encoded data, perform disparity averaging and completion processing.
		@return none.
	 */
	static void getDisparityData(int imghgt, int imgwdt,
		unsigned short* prgtimg, unsigned short* pdspenc, int frmgain,
		int* pblkhgt, int* pblkwdt, int* pmtchgt, int* pmtcwdt,
		int* pblkofsx, int* pblkofsy, int* pdepth, int* pshdwdt,
		unsigned char* pdspimg, float* ppxldsp, float* pblkdsp, int *pblkval, int *pblkcrst);

	/** @brief Decodes double-shutter disparity encoded data, performs disparity averaging and completion processing.
		@return none.
	 */
	static void getDoubleDisparityData(int imghgt, int imgwdt,
		unsigned char* pimgcur, unsigned char* penccur, int expcur, int gaincur,
		unsigned char* pimgprev, unsigned char* pencprev, int expprev, int gainprev,
		int* pblkhgt, int* pblkwdt, int* pmtchgt, int* pmtcwdt,
		int* pblkofsx, int* pblkofsy, int* pdepth, int* pshdwdt,
		unsigned char* pbldimg, unsigned char* pdspimg, float* ppxldsp, float* pblkdsp,
		int* pblkval, int* pblkcrst);


private:

	/** @brief Decode parallax encoded data.
		@return none.
	 */
	static void decodeDisparityData(int imghgt, int imgwdt, unsigned char* prgtimg, 
		unsigned char* pSrcImage, int frmgain,
		unsigned char* pDispImage, float* pTempParallax, float* pBlockDepth,
		int* pblkval, int* pblkcrst);

	/** @brief Decode parallax encoded data.
		@return none.
	 */
	static void decodeDisparityDataFor4K(int imghgt, int imgwdt, unsigned short* prgtimg,
		unsigned short* pSrcImage, int frmgain,
		unsigned char* pDispImage, float* pTempParallax, float* pBlockDepth,
		int* pblkval, int* pblkcrst);

	/** @brief Decode double shutter parallax encoded data.
		@return none.
	 */
	static void decodeDoubleDisparityData(int imghgt, int imgwdt,
		unsigned char* pimghigh, unsigned char* penchigh, int gainhigh,
		unsigned char* pimglow, unsigned char* penclow, int gainlow,
		unsigned char* pbldimg, unsigned char* pdspimg, float* ppxldsp, float* pblkdsp,
		int* pblkval, int* pblkcrst);

	/** @brief Synthesize parallax data.
		@return none.
	 */
	static void blendDisparityData(int imghgt, int imgwdt,
		unsigned char* pbldimg_h, unsigned char* pdspimg_h, float* ppxldsp_h, float* pblkdsp_h, int* pblkval_h, int* pblkcrst_h,
		unsigned char* pbldimg_l, unsigned char* pdspimg_l, float* ppxldsp_l, float* pblkdsp_l, int* pblkval_l, int* pblkcrst_l);


};

