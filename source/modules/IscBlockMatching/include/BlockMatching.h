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
 * @file BlockMatching.h
 * @brief Implementation of Block Matching processing
 */
#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>


/**
 * @class   BlockMatching
 * @brief   implementation class
 * this class is an implementation of Block Matching processing
 */
class BlockMatching
{

public:

	BlockMatching();

	~BlockMatching();

	/** @brief initialize block matching.
		@return none.
	 */
	static void initialize(int imghgt, int imgwdt);

	/** @brief end block matching.
		@return none.
	 */
	static void finalize();

	/** @brief configure use of OpenCL for block matching.
		@return none.
	 */
	static void setUseOpenCLForBlockMatching(int usecl);

	/** @brief set block matching parameters.
		@return none.
	 */
	static void setMatchingParameter(int imghgt, int imgwdt, int depth,
		int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int blkofsx, int blkofsy, int crstthr);

	/** @brief set back matching parameters.
		@return none.
	 */
	static void setBackMatchingParameter(int enb, int bkevlwdt, int bkevlrng, int bkvldrt, int bkzrrt);
	
	/** @brief perform stereo matching.
		@return none.
	 */
	static void matching(unsigned char* prgtimg, unsigned char* plftimg);

	/** @brief get parallax block information.
		@return none.
	 */
	static void getBlockDisparity(int *pblkhgt, int *pblkwdt, int *pmtchgt, int *pmtcwdt,
		int *pblkofsx, int *pblkofsy, int *pdepth, int *pshdwdt, float *pblkdsp, int *pblkval, int *pblkcrst);

	/** @brief get parallax pixel information.
		@return none.
	 */
	static void getDisparity(int imghgt, int imgwdt, unsigned char *pdspimg, float *ppxldsp);

	/** @brief spawn a block matching thread.
		@return none.
	 */
	static void createMatchingThread();

	/** @brief destroy the matching thread.
		@return none.
	 */
	static void deleteMatchingThread();


private:

	/** @brief perform block matching.
		@return none.
	 */
	static void executeMatching(int imghgt, int imgwdt, int depth,
		unsigned char *pimgref, unsigned char *pimgcmp,
		float * pblkdsp, int *pblkcrst);

	/** @brief get parallax.
		@return none.
	 */
	static void getMatchingDisparity(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp, int *pblkcrst);

	/** @brief get the parallax of the whole image.
		@return none.
	 */
	static void getWholeDisparity(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp, int *pblkcrst);

	/** @brief gGet In-Band Parallax.
		@return none.
	 */
	static void getDisparityInBand(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char *pimgref, unsigned char *pimgcmp, float * pblkdsp, int *pblkcrst, int jstart, int jend);

	/** @brief get parallax by SSD.
		@return none.
	 */
	static void getDisparityBySSD(int x, int y, int imghgt, int imgwdt, int depth,
		int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char *pimgref, unsigned char *pimgcmp, float * pblkdsp, int *pblkcrst);

	/** @brief get the disparity in both directions within the band.
		@return none.
	 */
	static void getBothDisparityInBand(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp, int *pblkcrst,
		int jstart, int jend);

	/** @brief Find disparity value by block matching in both directions.
		@return none.
	*/
	static void getBothDisparityBySSD(int x, int y, int imghgt, int imgwdt, int depth,
		int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char *pimgref, unsigned char *pimgcmp, 
		float * pblkdsp, float * pblkbkdsp, int *pblkcrst);

	/** @brief synthesize Parallax Matching in Both Directions.
		@return none.
	 */
	static void blendBothMatchingDisparity(int imghgt, int imgwdt, int imghgtblk, int imgwdtblk,
		int bkevlwdt, int bkevlrng, int bkvldrt, int bkzrrt, float *pblkdsp, float *pblkbkdsp);

	/** @brief expand parallax from blocks to pixels.
		@return none.
	 */
	static void spreadDisparityImage(int imghgt, int imgwdt, int depth, int shdwdt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int dspofsx, int dspofsy,
		float * pblkdsp, unsigned char * ppxldsp, float *ppxlsub);

	/** @brief perform block matching.(OpenCL)
		@return none.
	 */
	static void executeMatchingOpenCL(int imghgt, int imgwdt, int depth,
		unsigned char *pimgref, unsigned char *pimgcmp,
		float * pblkdsp, int *pblkcrst);

	/** @brief Get parallax by SSD.
		@return none.
	 */
	static void getDisparityBySSDOpenCL(int imghgt, int imgwdt, int depth,
		int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		cv::UMat imgref, cv::UMat imgcmp, cv::UMat blkdsp, cv::UMat blkcrst);

	/** @brief get parallax by bi-directional matching.(OpenCL)
		@return none.
	 */
	static void getBothDisparityBySSDOpenCL(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		cv::UMat imgref, cv::UMat imgcmp, cv::UMat blkdsp, cv::UMat blkbkdsp, cv::UMat blkcrst);

	/** @brief get parallax by band splitting.
		@return none.
	 */
	static void getBandDisparity(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int bgtmax,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp, int *pblkcrst);

	/** @brief block matching thread.
		@return none.
	 */
	static UINT  matchingBandThread(LPVOID parg);


};

