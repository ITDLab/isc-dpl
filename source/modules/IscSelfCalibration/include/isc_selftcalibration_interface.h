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
 * @file isc_selftcalibration_interface.h
 * @brief Provides an interface for implementing self calibration
 */

#pragma once

#ifdef ISCSELFCALIBRATION_EXPORTS
#define ISCSELFCALIBRATION_API __declspec(dllexport)
#else
#define ISCSELFCALIBRATION_API __declspec(dllimport)
#endif

/**
 * @class   IscSelftCalibrationInterface
 * @brief   interface class
 * this class is an interface for self calibration module
 */
class ISCSELFCALIBRATION_API IscSelftCalibrationInterface
{

public:
	IscSelftCalibrationInterface();
	~IscSelftCalibrationInterface();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	 */
	int Initialize(const IscCameraControlConfiguration* isc_camera_control_configuration, const int max_image_width, const int max_image_height);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	/** @brief gets the name of the parameter file for the specified module.
		@return 0, if successful.
	 */
	int GetParameterFileName(wchar_t* file_name, const int max_length);

	/** @brief get parameters for the specified module from a file.
		@return 0, if successful.
	 */
	int ReloadParameterFromFile(const wchar_t* file_name, const bool is_valid);

	/** @brief starts auto-adjustment.
		@return 0, if successful.
	 */
	int StartSelfCalibration();

	/** @brief stops automatic adjustment.
		@return 0, if successful.
	 */
	int StoptSelfCalibration();

	/** @brief make the left and right images parallel.
		@return 0, if successful.
	 */
	int ParallelizeSelfCalibration(IscImageInfo* isc_image_info);

	/** @brief Setup call-back function.
		@return none.
	 */
	void SetCallbackFunc(std::function<int(unsigned char*, unsigned char*, int, int)> func_get_camera_reg, std::function<int(unsigned char*, int)> func_set_camera_reg);


private:

	bool parameter_update_request_;

	IscCameraControlConfiguration isc_camera_control_configuration_;
	int max_image_width_, max_image_height_;

	wchar_t parameter_file_name_[_MAX_PATH];

	struct MeshParameter {
		int imghgt;		/**< 補正画像の高さ */
		int imgwdt;		/**< 補正画像の幅 */
		int mshhgt;		/**< メッシュサイズ高さ */
		int mshwdt;		/**< メッシュサイズ幅 */
		int mshcntx;	/**< メッシュ中心x座標 */
		int mshcnty;	/**< メッシュ中心y座標 */
		int mshnrgt;	/**< メッシュ右個数 */
		int mshnlft;	/**< メッシュ左個数 */
		int mshnupr;	/**< メッシュ上個数 */
		int mshnlwr;	/**< メッシュ下個数 */
		int rgntop;		/**< 領域座標Top */
		int rgnbtm;		/**< 領域座標Bottom */
		int rgnlft;		/**< 領域座標Left */
		int rgnrgt;		/**< 領域座標Right */
		int srchhgt;	/**< 対応点探索領域の高さ */
		int srchwdt;	/**< 対応点探索領域の幅 */
	};

	struct MeshThreshold {
		int minbrgt;		/**< 最小平均輝度 */
		int maxbrgt;		/**< 最大平均輝度 */
		int mincrst;		/**< 最小コントラスト */
		double minedgrt;	/**< 最小エッジ比率 */
		int maxdphgt;		/**< 最大変位高さ */
		int maxdpwdt;		/**< 最大変位幅 */
		double minmtcrt;	/**< 最小一致率 */
	};

	struct OperationMode {
		int grdcrct;	/**< 階調補正モードステータス 0:オフ 1:オン */
	};

	struct AveragingParameter {
		int minmshn;		/**< 最小メッシュ特徴点数 */
		double maxdifstd;	/**< 最大垂直ズレ標準偏差 */
		int avefrmn;		/**< ズレ量平均フレーム数 */
	};

	struct Criteria {
		int calccnt;		/**< ズレ量算出回数 */
		double crtrdiff;	/**< 垂直ズレ量の判定基準 */
		double crtrrot;		/**< 回転ズレ量の判定基準 */
		double crtrdev;		/**< 標準偏差の判定基準 */
		int crctrot;		/**< 回転ズレ補正 0:しない 1:する */
		int crctsv;			/**< 補正量の自動保存 0:しない 1:する */
	};

	struct SelefCalibrationparameter {
		MeshParameter mesh_parameter;
		MeshThreshold mesh_threshold;
		OperationMode operation_mode;
		AveragingParameter averaging_parameter;
		Criteria criteria;
	};

	SelefCalibrationparameter selft_calibration_parameters_;


	int LoadParameterFromFile(const wchar_t* file_name, SelefCalibrationparameter* selfcalibration_parameters);
	int SaveParameterToFile(const wchar_t* file_name, const SelefCalibrationparameter* selfcalibration_parameters);
	int SetParameterToSelftCalibrationModule(const SelefCalibrationparameter* selfcalibration_parameters);

	static int GetCameraRegData(unsigned char* wbuf, unsigned char* rbuf, int write_size, int read_size);
	static int SetCameraRegData(unsigned char* wbuf, int write_size);

};