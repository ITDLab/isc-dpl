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
 * @file StereoMatching.h
 * @brief Implementation of Stereo Matching processing
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

 /**
  * @class   StereoMatching
  * @brief   implementation class
  * this class is an implementation of Stereo Matching processing
  */
class StereoMatching
{

public:

	StereoMatching();

	~StereoMatching();

	/** @brief initialize stereo matching.
		@return none.
	 */
	static void initialize(int imghgt, int imgwdt);

	/** @brief end block matching.
		@return none.
	 */
	static void finalize();

	/** @brief configure use of OpenCL for stereo matching.
		@return none.
	 */
	static void setUseOpenCLForMatching(int usecl, int runsgcr = 0);

	/** @brief set stereo matching parameters.
		@return none.
	 */
	static void setMatchingParameter(int imghgt, int imgwdt, int depth,
		int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int blkofsx, int blkofsy, int crstthr, int grdcrct,
		int rmvdup, int minbrtrt);

	/** @brief set extention stereo matching parameters.
		@return none.
	 */
	static void setExtensionMatchingParameter(int extmtc, int extlim, int extcnf);

	/** @brief set back matching parameters.
		@return none.
	 */
	static void setBackMatchingParameter(int enb, int bkevlwdt, int bkevlrng, int bkvldrt, int bkzrrt);
	
	/** @brief set Nearest neighbor matching parameters.
		@return none.
	 */
	static void setNeighborMatchingParameter(int enb, double neibrot, double neibvsft, double neibhsft, double neibrng);

	/** @brief Record data for nearest neighbor matching..
		@return none.
	 */
	static void setRecordNeighborMatching();

	/** @brief perform stereo matching.
		@return none.
	 */
	static void matching(unsigned char* prgtimg, unsigned char* plftimg, int frmgain = 0);

	/** @brief perform stereo matching.
		@return none.
	 */
	static void matching(unsigned short* prgtimg, unsigned short* plftimg, int frmgain = 0);

	/** @brief perform double shutter stereo matching.
		@return none.
	 */
	static void matchingDouble(unsigned char* prgtimghigh, unsigned char* plftimghigh, int frmgainhigh,
		unsigned char* prgtimglow, unsigned char* plftimglow, int frmgainlow);

	/** @brief perform double shutter stereo matching.
		@return none.
	 */
	static void matchingDouble(unsigned short* prgtimghigh, unsigned short* plftimghigh, int frmgainhigh,
		unsigned short* prgtimglow, unsigned short* plftimglow, int frmgainlow);

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
	static void doMatching(unsigned char* prgtimg, unsigned char* plftimg, int frmgain, float* pblkdsp, int* pblkcrst);

	/** @brief perform block matching.
		@return none.
	 */
	static void doMatching16U(unsigned short* prgtimg, unsigned short* plftimg, int frmgain, float* pblkdsp, int* pblkcrst);

	/** @brief Composite double-shutter parallax data.
		@return none.
	 */
	static void blendDobleDisparity(int imghgt, int imgwdt, int blkhgt, int blkwdt,
		float* pblkdsp_h, int* pblkcrst_h, float* pblkdsp_l, int* pblkcrst_l);

	/** @brief Synthesize parallax of nearest neighbor matching.
		@return none.
	 */
	static void blendNeighborMatchingDisparity(int imghgt, int imgwdt, int blkhgt, int blkwdt,
		float neibrng, float* pblkdsp_n1, float* pblkdsp_n2, float* pblkdsp);

	/** @brief Generate a nearest neighbor matching image.
		@return none.
	 */
	static void makeNeighborImage(int imghgt, int imgwdt, double rotdeg, double vrtsft, double hrzsft,
		unsigned char* psrcimg, unsigned char* pdstimg);

	/** @brief Generate a nearest neighbor matching image.
		@return none.
	 */
	static void makeNeighborImage(int imghgt, int imgwdt, double rotdeg, double vrtsft,
		unsigned char* psrcimg, unsigned char* pdstimg);

	/** @brief Generate a nearest neighbor matching image.
		@return none.
	 */
	static void makeNeighborImage16U(int imghgt, int imgwdt, double rotdeg, double vrtsft, double hrzsft,
		unsigned short* psrcimg, unsigned short* pdstimg);

	/** @brief Generate a nearest neighbor matching image.
		@return none.
	 */
	static void makeNeighborImage16U(int imghgt, int imgwdt, double rotdeg, double vrtsft,
		unsigned short* psrcimg, unsigned short* pdstimg);

	/** @brief Perform stereo matching.
		@return none.
	 */
	static void executeMatching(int imghgt, int imgwdt, int depth,
		unsigned char *pimgref, unsigned char *pimgcmp, int frmgain,
		float * pblkdsp, int *pblkcrst);

	/** @brief Perform stereo matching.
		@return none.
	 */
	static void executeMatching16U(int imghgt, int imgwdt, int depth,
		unsigned short *pimgref, unsigned short *pimgcmp, int frmgain,
		float * pblkdsp, int *pblkcrst);

	/** @brief Obtain disparity.
		@return none.
	 */
	static void getMatchingDisparity(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char* pimgref, unsigned char* pimgcmp, float* pblkdsp, float* pblkbkdsp,
		int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst);

	/** @brief Obtain disparity.
		@return none.
	 */
	static void getMatchingDisparity16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned short* pimgref, unsigned short* pimgcmp, float* pblkdsp, float* pblkbkdsp,
		int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst);

	/** @brief Remove duplicate blocks.
		@return none.
	 */
	static void removeDuplicateBlock(int imghgt, int imgwdt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		int* pimgrefbrt, int* pimgcmpbrt, float* pblkdsp, int* pdspposi);

	/** @brief Obtain block luminance and contrast for the entire image.
		@return none.
	 */
	static void getWholeBlockBrightnessContrast(int imghgt, int imgwdt, int stphgt, int stpwdt,
		int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
		unsigned char* pimgref, unsigned char* pimgcmp,
		int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst);

	/** @brief Obtain block luminance and contrast for the entire image.
		@return none.
	 */
	static void getWholeBlockBrightnessContrast16U(int imghgt, int imgwdt, int stphgt, int stpwdt,
		int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
		unsigned short* pimgref, unsigned short* pimgcmp,
		int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst);

	/** @brief Obtain disparity for the entire image.
		@return none.
	 */
	static void getWholeDisparity(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp, float* pblkbkdsp);

	/** @brief Obtain disparity for the entire image.
		@return none.
	 */
	static void getWholeDisparity16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp, float* pblkbkdsp);

	/** @brief Obtain the block luminance and contrast of the compared images in the band.
		@return none.
	 */
	static void getBlockBrightnessContrastInBand(int imghgt, int imgwdt, int stphgt, int stpwdt,
		int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst,
		int jstart, int jend);
	
	/** @brief Obtain the block luminance and contrast of the compared images in the band.
		@return none.
	 */
	static void getBlockBrightnessContrastInBand16U(int imghgt, int imgwdt, int stphgt, int stpwdt,
		int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst,
		int jstart, int jend);

	/** @brief Obtain parallax within a band.
		@return none.
	 */
	static void getDisparityInBand(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp,
		int jstart, int jend);


	/** @brief Obtain parallax within a band.
		@return none.
	 */
	static void getDisparityInBand16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp,
		int jstart, int jend);

	/** @brief Obtain the block luminance and contrast of an image.
		@return none.
	 */
	static void getBlockBrightnessContrast(int x, int y, int imghgt, int imgwdt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		int crstthr, int crstofs, int grdcrct,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst);

	/** @brief Obtain the block luminance and contrast of an image.
		@return none.
	 */
	static void getBlockBrightnessContrast16U(int x, int y, int imghgt, int imgwdt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		int crstthr, int crstofs, int grdcrct,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst);

	/** @brief Obtain parallax by SSD.
		@return none.
	 */
	static void getDisparityBySSD(int x, int y, int imghgt, int imgwdt,
		int depth, int extcnf, int crstthr, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst,
		float* pblkdsp);

	/** @brief Obtain parallax by SSD.
		@return none.
	 */
	static void getDisparityBySSD16U(int x, int y, int imghgt, int imgwdt,
		int depth, int extcnf, int crstthr, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst,
		float* pblkdsp);

	/** @brief Obtain parallax in both directions within a band.
		@return none.
	 */
	static void getBothDisparityInBand(int imghgt, int imgwdt, int depth,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp, float* pblkbkdsp,
		int jstart, int jend);

	/** @brief Obtain parallax in both directions within a band.
		@return none.
	 */
	static void getBothDisparityInBand16U(int imghgt, int imgwdt, int depth,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp, float* pblkbkdsp,
		int jstart, int jend);

	/** @brief Stereo matching in both directions to obtain disparity values.
		@return none.
	 */
	static void getBothDisparityBySSD(int x, int y, int imghgt, int imgwdt, int depth,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp, float* pblkbkdsp);

	/** @brief Stereo matching in both directions to obtain disparity values.
		@return none.
	 */
	static void getBothDisparityBySSD16U(int x, int y, int imghgt, int imgwdt, int depth,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp, float* pblkbkdsp);

	/** @brief Combine parallaxes of matching in both directions.
		@return none.
	 */
	static void blendBothMatchingDisparity(int imghgt, int imgwdt, int imghgtblk, int imgwdtblk,
		int bkevlwdt, int bkevlrng, int bkvldrt, int bkzrrt, float *pblkdsp, float *pblkbkdsp);

	/** @brief Expand parallax from block to pixel.
		@return none.
	 */
	static void spreadDisparityImage(int imghgt, int imgwdt, int depth, int shdwdt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int dspofsx, int dspofsy,
		float * pblkdsp, unsigned char * ppxldsp, float *ppxlsub);

	/** @brief Perform stereo matching.
		@return none.
	 */
	static void executeMatchingOpenCL(int imghgt, int imgwdt, int depth,
		unsigned char *pimgref, unsigned char *pimgcmp, int frmgain,
		float * pblkdsp, int *pblkcrst);

	/** @brief Perform stereo matching.
		@return none.
	 */
	static void executeMatchingOpenCL16U(int imghgt, int imgwdt, int depth,
		unsigned short *pimgref, unsigned short *pimgcmp, int frmgain,
		float * pblkdsp, int *pblkcrst);

	/** @brief Obtain the block luminance and contrast of an image.
		@return none.
	 */
	static void getBlockBrightnessContrastOpenCL(int imghgt, int imgwdt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
		cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst);

	/** @brief Obtain the block luminance and contrast of an image.
		@return none.
	 */
	static void getBlockBrightnessContrastOpenCL16U(int imghgt, int imgwdt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
		cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst);

	/** @brief Obtain parallax by SSD.
		@return none.
	 */
	static void getDisparityBySSDOpenCL(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst,
		cv::UMat blkdsp);

	/** @brief Obtain parallax by SSD.
		@return none.
	 */
	static void getDisparityBySSDOpenCL16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst,
		cv::UMat blkdsp);

	/** @brief Obtain disparity by bidirectional matching.
		@return none.
	 */
	static void getBothDisparityBySSDOpenCL(int imghgt, int imgwdt, int depth,
		int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst,
		cv::UMat blkdsp, cv::UMat blkbkdsp);

	/** @brief Obtain disparity by bidirectional matching.
		@return none.
	 */
	static void getBothDisparityBySSDOpenCL16U(int imghgt, int imgwdt, int depth,
		int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst,
		cv::UMat blkdsp, cv::UMat blkbkdsp);

	/** @brief Obtain block luminance and contrast by band splitting.
		@return none.
	 */
	static void getBandBlockBrightnessContrast(int imghgt, int imgwdt, int stphgt, int stpwdt,
		int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst);

	/** @brief Obtain block luminance and contrast by band splitting.
		@return none.
	 */
	static void getBandBlockBrightnessContrast16U(int imghgt, int imgwdt, int stphgt, int stpwdt,
		int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst);

	/** @brief Obtain parallax by band splitting.
		@return none.
	 */
	static void getBandDisparity(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp, float* pblkbkdsp);

	/** @brief Obtain parallax by band splitting.
		@return none.
	 */
	static void getBandDisparity16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
		int crstthr, int crstofs, int grdcrct, int minbrtrt,
		int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
		unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
		int* pblkrefcrst, int* pblkcmpcrst, float* pblkdsp, float* pblkbkdsp);

	/** @brief Block luminance calculation thread.
		@return none.
	 */
	static UINT blockBandThread(LPVOID parg);

	/** @brief Stereo matching thread.
		@return none.
	 */
	static UINT matchingBandThread(LPVOID parg);


};

