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

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

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

	/** @brief exit frame decoder.
		@return none.
	 */
	static void finalize();

	/** @brief configure use of OpenCL for parallax averaging.
		@return none.
	 */
	static void setUseOpenCLForAveragingDisparity(int usecl);

	/** @brief store block parallax value.
		@return none.
	 */
	static void saveBlockDisparity();

	/** @brief set the upper and lower limits of parallax.
		@return none.
	 */
	static void setDisparityLimitation(int limit, double lower, double upper);

	/** @brief set parallax averaging parameters.
		@return none.
	 */
	static void setAveragingParameter(int enb, int blkshgt, int blkswdt,
		double intg, double range, int dsprt, int vldrt, int reprt);

	/** @brief set the weight of the parallax averaging block.
		@return none.
	 */
	static void setAveragingBlockWeight(int cntwgt, int nrwgt, int rndwgt);

	/** @brief set parallax interpolation parameters.
		@return none.
	 */
	static void setComplementParameter(int enb, double lowlmt, double slplmt,
		double insrt, double rndrt, double btmrt, int crstlmt, int hlfil, double hlsz);

	/** @brief split frame data into image data.
		@return none.
	 */
	static void decodeFrameData(int imghgt, int imgwdt, unsigned char* pfrmdat,
		unsigned char* prgtimg, unsigned char* plftimg, unsigned char* pdspenc);

	/** @brief decode the parallax data, return it to the parallax image and parallax information, and perform averaging and interpolation processing.
		@return none.
	 */
	static void decodeDisparityData(int imghgt, int imgwdt,
		unsigned char* prgtimg, int crstthr, unsigned char* pdspenc,
		unsigned char* pdspimg, float* ppxldsp, float* pblkdsp);

	/** @brief average the parallax.
		@return none.
	 */
	static bool averageDisparityData(int imghgt, int imgwdt,
		int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int dspofsx, int dspofsy, int depth, int shdwdt,
		int *pblkval, int *pblkcrst,
		unsigned char* pdspimg, float* ppxldsp, float* pblkdsp);

	/** @brief spawn parallax averaging thread.
		@return none.
	 */
	static void createAveragingThread();

	/** @brief Destroy parallax averaging thread.
		@return none.
	 */
	static void deleteAveragingThread();


private:

	/** @brief get block parallax value and mask data from parallax encoded data and expand to pixels.
		@return none.
	 */
	static void decodeDisparityDirect(int imghgt, int imgwdt,
		unsigned char* pSrcImage, unsigned char* pDispImage, float* pTempParallax, float* pBlockDepth);

	/** @brief get block disparity value and contrast from disparity encoded data.
		@return none.
	 */
	static void getDisparityData(int imghgt, int imgwdt,
		unsigned char* prgtimg, int crstthr, int crstofs, int bgtmax,
		unsigned char* pdspenc, int* pblkdsp, int* pblkcrst);

	/** @brief average disparity values.
		@return none.
	 */
	static void getAveragingDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp);

	/** @brief averaging disparity values using OpenCV.
		@return none.
	 */
	static void getAveragingDisparityOpenCV(int imghgt, int imgwdt, int* pblkval, float *pavedsp);

	/** @brief average disparity values (call OpenCL).
		@return none.
	 */
	static void getAveragingDisparityOpenCL(int imghgtblk, int imgwdtblk, int dspwdtblk,
		int depth, int dspsubrt, cv::UMat src, cv::UMat dst);

	/** @brief average disparity values.
		@return none.
	 */
	static void getWholeAveragingDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp);

	/** @brief average disparity values.
		@return none.
	 */
	static void getAveragingDisparityInBand(int imghgtblk, int imgwdtblk, int dspwdtblk,
		int* pblkval, float *pavedsp, int jstart, int jend);

	/** @brief expand block parallax to pixels.
		@return none.
	 */
	static void getDisparityImage(int imghgt, int imgwdt,
		int* pblkval, float *pavedsp, unsigned char* pDestImage, float* pTempParallax, float* pBlockDepth);

	/** @brief complement parallax.
		@return none.
	 */
	static void getComplementDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp,
		int* pblkcrst);

	/** @brief fill in the parallax.
		@return none.
	 */
	static void getHoleFillingDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp,
		int* pblkcrst);

	/** @brief complement no parallax with horizontal scanning.
		@return none.
	 */
	static void getHorizontalComplementDisparity(int imghgt, int imgwdt, bool holefill,
		int* pblkval, float *pavedsp, int* pblkcrst);

	/** @brief complement no parallax with vertical scanning.
		@return none.
	 */
	static void getVerticalComplementDisparity(int imghgt, int imgwdt, bool holefill,
		int* pblkval, float *pavedsp, int* pblkcrst);

	/** @brief complement no parallax with diagonal downward scanning.
		@return none.
	 */
	static void getDiagonalDownComplementDisparity(int imghgt, int imgwdt, bool holefill,
		int* pblkval, float *pavedsp, int* pblkcrst);

	/** @brief complement no parallax with diagonal upward scanning.
		@return none.
	 */
	static void getDiagonalUpComplementDisparity(int imghgt, int imgwdt, bool holefill,
		int* pblkval, float *pavedsp, int* pblkcrst);

	/** @brief complement the target block by ascending scanning of the parallax block array.
		@return none.
	 */
	static void complementForward(int imgblkwdt, int ii, int sti, int jd, int id,
		int* pblkval, float *pavedsp);

	/** @brief complement the block of interest by descending scanning of the parallax block array.
		@return none.
	 */
	static void complementBackward(int imgblkwdt, int ii, int sti, int jd, int id,
		int* pblkval, float *pavedsp, int *pblkcrst, bool holefill,
		double blkwdt, double midrt, double toprt, double btmrt);

	/** @brief export Block Parallax Values.
		@return none.
	 */
	static void writeBlockDisparity(int imghgt, int imgwdt, float *pavedsp);

	/** @brief parallax averaging thread.
		@return none.
	 */
	static UINT averagingBandThread(LPVOID parg);

	/** @brief average disparity values.
		@return none.
	 */
	static void getBandAveragingDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp);

};

