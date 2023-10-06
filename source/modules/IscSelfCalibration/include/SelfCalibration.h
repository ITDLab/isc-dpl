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
 * @file SelfCalibration.h
 * @brief Implementation of self calibration
 */

#pragma once

/**
 * @class   SelfCalibration
 * @brief   implementation class
 * this class is an implementation of self calibration
 */
class SelfCalibration
{
public:
	/// <summary>
	/// オブジェクトを生成する
	/// </summary>
	SelfCalibration();

	/// <summary>
	/// オブジェクトを破棄する
	/// </summary>
	~SelfCalibration();

	/** @brief initialize the disparity filter.
		@return none.
	 */
	static void initialize(int imghgt, int imgwdt);

	/** @brief terminate the disparity filter.
		@return none.
	 */
	static void finalize();

	/** @brief Set mesh parameters.
		@return none.
	 */
	static void setMeshParameter(int imghgt, int imgwdt,
		int mshhgt, int mshwdt, int mshcntx, int mshcnty,
		int mshnrgt, int mshnlft, int mshnupr, int mshnlwr,
		int rgntop, int rgnbtm, int rgnlft, int rgnrgt, int srchhgt, int srchwdt);

	/** @brief Set tone correction mode.
		@return none.
	 */
	static void setOperationMode(int grdcrct);

	/** @brief Set mesh threshold.
		@return none.
	 */
	static void setMeshThreshold(int minbrgt, int maxbrgt, int mincrst,
		double minedgrt, int maxdphgt, int maxdpwdt, double minmtcrt);

	/** @brief Set average amount of error.
		@return none.
	 */
	static void setAveragingParameter(int minmshn, double maxdifdev, int avefrmn);

	/** @brief Set criterion of error.
		@return none.
	 */
	static void setCriteria(int calccnt, double crtrdiff, double crtrrot, double crtrstd, int crctrot, int crctsv);

	/** @brief Obtain the coordinates of the mesh.
		@return none.
	 */
	static int getMeshCoordinate(int **mshrgnx, int **mshrgny, int **srchrgnx, int **srchrgny);

	/** @brief Obtain the pattern intensity of the mesh.
		@return none.
	 */
	static void getMeshTextureStrength(bool **mshstrgt, int **mshbrgt, int **mshcrst,
		double **mshedgx, double **mshedgd);

	/** @brief Obtain sub-pixel coordinates of the corresponding point area.
		@return none.
	 */
	static int getMatchSubpixelCoordinate(bool **mshmtc, double **mtcrt, double **mtcsubx, double **mtcsuby);

	/** @brief Clear the result of the rectification process for the current frame.
		@return none.
	 */
	static void clearCurrentMeshDifference();

	/** @brief Returns the result of the rectification process for the current frame.
		@return result of the rectification process.
	 */
	static int getCurrentMeshDifference(double *difvrt, double *difrot, double *difvstd);

	/** @brief Obtain the amount of mesh misalignment of the left and right images of the latest frame.
		@return none.
	 */
	static void getMeshDifference(double **mshdif, double *difvrt, double *difrot, double *difvstd);

	/** @brief Obtain the frame average displacement of the left and right images.
		@return none.
	 */
	static int getAverageDifference(double *avedvrt, double *avedrot, double *avedvstd);

	/** @brief Obtain the amount of displacement correction for the current left and right images.
		@return none.
	 */
	static void  getCurrentCorrectValue(double *vrtdif, int *vrtval, double *rotdif, int *rotval);

	/** @brief Save the latest misalignment correction amount in the camera.
		@return none.
	 */
	static void saveLatestCorrectValue();

	/** @brief Parallel left and right images.
		@return none.
	 */
	static void parallelize(unsigned char *pimgref, unsigned char *pimgcmp);

	/** @brief Start the parallelization process.
		@return none.
	 */
	static void start();

	/** @brief Stops the parallelization process.
		@return none.
	 */
	static void stop();

	/** @brief Request to stop the parallelization process.
		@return none.
	 */
	static void requestStop();

	/** @brief Obtains the status of the parallelization process.
		@return none.
	 */
	static bool getStatus(bool *pprcstat);

	/** @brief Setup call-back function.
		@return none.
	 */
	static void SetCallbackFunc(std::function<int(unsigned char*, unsigned char*, int, int)> func_get_camera_reg, std::function<int(unsigned char*, int)> func_set_camera_reg);

private:

	/** @brief Parallelize epipolar lines.
		@return none.
	 */
	static void parallelizeEpipolarLine(unsigned char *pimgref, unsigned char *pimgcmp);

	/** @brief Set epipolar mesh coordinates.
		@return none.
	 */
	static void setEpipolarMeshCoodinate();

	/** @brief Obtaining the contrast of an epipolar mesh.
		@return none.
	 */
	static void getEpipolarMeshContrast(unsigned char *pimgbuf);

	/** @brief Explore mesh patterns.
		@return none.
	 */
	static void searchEpipolarMesh(unsigned char *pimgref, unsigned char *pimgcmp);

	/** @brief Calculate the amount of mesh misalignment between the left and right images.
		@return none.
	 */
	static void calculateEpipolarMeshDifference();

	/** @brief Obtain the variance of the vertical misalignment difference between left and right.
		@return none.
	 */
	static void getVarianceVerticalDifference(double raddiff, double *paveydiff, double *pvarydiff);

	/** @brief Calibrate epipolar lines.
		@return none.
	 */
	static void  correctDifference(int frmcnt, double dvrt, double drot, double stdev);

	/** @brief Obtain the coordinates of the matched region.
		@return none.
	 */
	static void getEpipolarMatchPosition(unsigned char *pimgref, unsigned char *pimgcmp,
		int topMshPos, int btmMshPos, int lftMshPos, int rgtMshPos,
		int topSrchPos, int btmSrchPos, int lftSrchPos, int rgtSrchPos,
		double *pMtchRt, double *ptopMtchSubPos, double *pbtmMtchSubPos, double *plftMtchSubPos, double *prgtMtchSubPos);

	/** @brief Obtain sub-pixel coordinates of the matched region.
		@return none.
	 */
	static void getSubPixelMatchPosition(unsigned char *pimgref, unsigned char *pimgcmp,
		int topMshPos, int btmMshPos, int lftMshPos, int rgtMshPos,
		int topMtchPos, int btmMtchPos, int lftMtchPos, int rgtMtchPos,
		double *mtchRt, double *ptopMtchSubPos, double *pbtmMtchSubPos, double *plftMtchSubPos, double *prgtMtchSubPos);

	/** @brief Get the current adjustment amount from the camera.
		@return none.
	 */
	static void  getCurrentDifference();

	// ////////////////////////////////////////
	// エピポーラ線平行化バックグラウンド処理
	// ////////////////////////////////////////

	/** @brief Generate epipolar line parallelization threads.
		@return none.
	 */
	static void createParallelizingThread();

	/** @brief Delete epipolar line parallelization thread.
		@return none.
	 */
	static void deleteParallelizingThread();

	/** @brief Interrupts the epipolar line parallelization process.
		@return none.
	 */
	static bool suspendParallelizing();

	/** @brief Restart the epipolar line parallelization process.
		@return none.
	 */
	static void resumeParallelizing(bool stflg);

	/** @brief Epipolar line parallelization thread function.
		@return none.
	 */
	static unsigned int parallelizingThreadFunction(void* parg);

	/** @brief Background execution of epipolar line parallelization process.
		@return none.
	 */
	static void epipolarParallelizingBackground(unsigned char *pimgref, unsigned char *pimgcmp);

	// ////////////////////////////////////////
	// 移動平均キュー処理
	// ////////////////////////////////////////

	/** @brief Reset the average queue.
		@return none.
	 */
	static void resetAverageQueue();

	/** @brief write to the mean queue.
		@return none.
	 */
	static void putAverageQueue(double diffval, double rotval);

	/** @brief Calculate the average from the average queue.
		@return none.
	 */
	static int getAverageQueue(int avewdt, double *diffave, double *rotave, double *diffstdev);

	/** @brief Clear the moving average calculation queue.
		@return none.
	 */
	static void clearQueue(int *pidx, bool *pwarp, int *pcnt);

	/** @brief Writes a value to the moving average calculation queue.
		@return none.
	 */
	static void putQueue(int *pidx, bool *pwarp, int *pcnt, double *pqueue, int qsize, double value);

	/** @brief Retrieves a specified number of averages from a moving average calculation queue.
		@return none.
	 */
	static int getMovingAverageQueue(int idx, bool warp, int cnt, double *pqueue, int qsize,
		int width, double *pave, double *pmin, double *pmax, double *pstddev);

	// ////////////////////////////////////////
	// カメラレジスタ読み出し書き込み
	// ////////////////////////////////////////

	/** @brief Set register read/write functions.
		@return none.
	 */
	static void  setRegisterFunction(int type);

	/** @brief Sets the amount of vertical shift for the left and right images.
		@return none.
	 */
	static void  setVerticalDifference(int adjvsft);

	/** @brief Set the amount of rotation for the left and right images.
		@return none.
	 */
	static void setRotationalDifference(int adjrot);

	/** @brief Obtain the vertical misalignment difference between left and right.
		@return none.
	 */
	static void  getVerticalDifference(int *pvsft, double *pdvrt);

	/** @brief Obtain the difference between left and right misalignment.
		@return none.
	 */
	static void getRotationalDifference(int *prot, double *drot);

	/** @brief Store register values in EEPROM.
		@return none.
	 */
	static int saveEEPROMCalib();

	/** @brief Stores epipolar line calibration values in the camera.
		@return none.
	 */
	static void saveAdjustmentValue(int vsft, int rot);

	/** @brief Reads a value from a register.
		@return none.
	 */
	static int readRegisterForVM(int addridx, int *pval);

	/** @brief Writes values to registers.
		@return none.
	 */
	static int writeRegisterForVM(int addridx, int val);

	/** @brief Reads a value from a register.
		@return none.
	 */
	static int readRegisterForXC(int addridx, int *pval);

	/** @brief Writes values to registers.
		@return none.
	 */
	static int writeRegisterForXC(int addridx, int val);


};

