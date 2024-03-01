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
 * @file DisparityFilter.h
 * @brief Implementation of Disparity Filter
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

/**
 * @class   DisparityFilter
 * @brief   implementation class
 * this class is an implementation of Disparity Filter
 */
class DisparityFilter
{

public:

	/** @brief initialize the disparity filter.
		@return none.
	 */
	static void initialize(int imghgt, int imgwdt);

	/** @brief terminate the disparity filter.
		@return none.
	 */
	static void finalize();

	/** @brief Configure the use of OpenCL for the disparity averaging process.
		@return none.
	 */
	static void setUseOpenCLForAveragingDisparity(int usecl, int runsgcr = 0);

	/** @brief Set the upper and lower parallax limits.
		@return none.
	 */
	static void setDisparityLimitation(int limit, double lower, double upper);

	/** @brief Set parallax averaging parameters.
		@return none.
	 */
	static void setAveragingParameter(int enb, int blkshgt, int blkswdt,
		double intg, double range, int dsprt, int vldrt, int reprt);

	/** @brief Set the weights of the disparity averaging blocks.
		@return none.
	 */
	static void setAveragingBlockWeight(int cntwgt, int nrwgt, int rndwgt);

	/** @brief Set parallax completion parameters.
		@return none.
	 */
	static void setInterpolateParameter(int enb, double lowlmt, double slplmt,
		double insrt, double rndrt, int crstlmt, int hlfil, double hlsz);

	/** @brief Set edge completion parameters.
		@return none.
	 */
	static void setEdgeInterpolateParameter(int edgcmp, int minblks, double mincoef, int cmpwdt);

	/** @brief Set Hough transform parameters.
		@return none.
	 */
	static void setHoughTransformParameter(int edgthr1, int edgthr2, int linthr, int minlen, int maxgap);

	/** @brief Parallax averaging.
		@return none.
	 */
	static bool averageDisparityData(int imghgt, int imgwdt, unsigned char* prgtimg,
		int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int dspofsx, int dspofsy, int depth, int shdwdt,
		int *pblkval, int *pblkcrst,
		unsigned char* pdspimg, float* ppxldsp, float* pblkdsp);

	/** @brief Generate parallax averaging thread.
		@return none.
	 */
	static void createAveragingThread();

	/** @brief Discard parallax averaging threads.
		@return none.
	 */
	static void deleteAveragingThread();

	/** @brief Sharpen parallax on straight edges.
		@return none.
	 */
	static void sharpenLinearEdge(int imghgt, int imgwdt, unsigned char* prgtimg,
		int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int dspofsx, int dspofsy, int depth, int shdwdt,
		int *pblkval);

private:

	/** @brief Averages disparity values.
		@return none.
	 */
	static void getAveragingDisparity(int imghgt, int imgwdt, int* pblkval);

	/** @brief Averages disparity values(OpenCV).
		@return none.
	 */
	static void getAveragingDisparityOpenCV(int imghgt, int imgwdt, int* pblkval, float *pavedsp);

	/** @brief Averages disparity values(OpenCL).
		@return none.
	 */
	static void getAveragingDisparityOpenCL(int imghgtblk, int imgwdtblk, int dspwdtblk,
		int depth, int dspsubrt, cv::UMat src, cv::UMat dst);

	/** @brief Averages disparity values.
		@return none.
	 */
	static void getWholeAveragingDisparity(int imghgt, int imgwdt, int* pblkval);

	/** @brief Averages disparity values.
		@return none.
	 */
	static void getAveragingDisparityInBand(int imghgtblk, int imgwdtblk, int dspwdtblk,
		int* pblkval, int jstart, int jend);

	/** @brief Expand parallax of a block to a pixel.
		@return none.
	 */
	static void getDisparityImage(int imghgt, int imgwdt,
		int* pblkval, unsigned char* pDestImage, float* pTempParallax, float* pBlockDepth);

	/** @brief Complementary parallax.
		@return none.
	 */
	static void getInterpolateDisparity(int imghgt, int imgwdt, int* pblkval,
		int* pblkcrst);

	/** @brief Fill in the parallax.
		@return none.
	 */
	static void getHoleFillingDisparity(int imghgt, int imgwdt, int* pblkval,
		int* pblkcrst);

	/** @brief Horizontal scanning completes areas with no parallax.
		@return none.
	 */
	static void getHorizontalInterpolateDisparity(int imghgt, int imgwdt, bool holefill,
		int* pblkval, int* pblkcrst);

	/** @brief Vertical scanning completes areas with no parallax.
		@return none.
	 */
	static void getVerticalInterpolateDisparity(int imghgt, int imgwdt, bool holefill,
		int* pblkval, int* pblkcrst);

	/** @brief Diagonal downward to complement parallax-free areas.
		@return none.
	 */
	static void getDiagonalDownInterpolateDisparity(int imghgt, int imgwdt, bool holefill,
		int* pblkval, int* pblkcrst);

	/** @brief Diagonally upward to complement parallax-free areas.
		@return none.
	 */
	static void getDiagonalUpInterpolateDisparity(int imghgt, int imgwdt, bool holefill,
		int* pblkval, int* pblkcrst);

	/** @brief Ascending scan of the disparity block array to complement the block of interest.
		@return none.
	 */
	static void interpolateForward(int imgblkwdt, int ii, int sti, int jd, int id,
		int* pblkval);

	/** @brief Descending scan of the disparity block array to complement the block of interest.
		@return none.
	 */
	static void interpolateBackward(int imgblkwdt, int ii, int sti, int jd, int id,
		int* pblkval, int *pblkcrst, bool holefill,
		double blkwdt, double midrt, double toprt, double btmrt);

	/** @brief Parallax Averaging Thread.
		@return none.
	 */
	static UINT averagingBandThread(LPVOID parg);

	/** @brief Averages disparity values.
		@return none.
	 */
	static void getBandAveragingDisparity(int imghgt, int imgwdt, int* pblkval);

	/** @brief Obtaining edge line segments from an image.
		@return none.
	 */
	static int getEdgeLineSegment(int imghgt, int imgwdt, unsigned char *prgtimg,
		int linnum, int linseg[][4], int *plinall);

	/** @brief Obtain a parallax block on a line segment.
		@return none.
	 */
	static void getLineSegmentBlocks(int imghgt, int imgwdt, unsigned char *prgtimg,
		int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int dspofsx, int dspofsy, int depth, int shdwdt,
		int *pblkval, int linnum, int linseg[][4], int linblk[][2], int *dspval, int *dspwgt, int *dspcmp);

	/** @brief Complementary parallax.
		@return none.
	 */
	static void setInterpolateDisparity(int blknum, int *dspval, int *dspwgt, int *dspcmp, double slope);

	/** @brief Find the regression line of disparity on a line segment.
		@return none.
	 */
	static int calculateRegressionLine(int blknum, int *dspval, double *pslope, double *pintcpt, double *pcoefdet);

	/** @brief Remove outlier parallax.
		@return none.
	 */
	static int removeOutsideDisparity(int blknum, int *dspval, int depth);


};

