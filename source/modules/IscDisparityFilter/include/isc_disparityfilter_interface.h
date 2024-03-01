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
 * @file isc_disparityfilter_interface.h
 * @brief Provides an interface for implementing Disparity Filter
 */

#pragma once

#ifdef ISCDISPARITYFILTER_EXPORTS
#define  ISCDISPARITYFILTER_API __declspec(dllexport)
#else
#define ISCDISPARITYFILTER_API __declspec(dllimport)
#endif

/**
 * @class   IscDisparityFilterInterface
 * @brief   interface class
 * this class is an interface for Disparity Filter module
 */
class ISCDISPARITYFILTER_API IscDisparityFilterInterface
{

public:
	IscDisparityFilterInterface();
	~IscDisparityFilterInterface();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	 */
	int Initialize(IscDataProcModuleConfiguration* isc_data_proc_module_configuration);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	// data processing module settings

	/** @brief get the configuration of the specified module.
		@return 0, if successful.
	 */
	int GetParameter(IscDataProcModuleParameter* isc_data_proc_module_parameter);

	/** @brief sets a parameter to the specified module.
		@return 0, if successful.
	 */
	int SetParameter(IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file);

	/** @brief gets the name of the parameter file for the specified module.
		@return 0, if successful.
	 */
	int GetParameterFileName(wchar_t* file_name, const int max_length);

	/** @brief get parameters for the specified module from a file.
		@return 0, if successful.
	 */
	int ReloadParameterFromFile(const wchar_t* file_name, const bool is_valid);

	// run

	/** @brief average the parallax.
		@return 0, if successful.
	*/
	int GetAverageDisparityData(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_block_disparity_data, IscDataProcResultData* isc_data_proc_result_data);

	/** @brief average the parallax for double shutter.
		@return 0, if successful.
	*/
	int GetAverageDisparityDataDoubleShutter(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_block_disparity_data, IscDataProcResultData* isc_data_proc_result_data);

private:

	bool parameter_update_request_;

	IscDataProcModuleConfiguration isc_data_proc_module_configuration_;

	wchar_t parameter_file_name_[_MAX_PATH];

	struct SystemParameter {
		bool enabled_opencl_for_avedisp;	/**< 視差平均化処理にOpenCLの使用を設定する */
		bool single_threaded_execution;		/**< シングルスレッドで実行 0:しない 1:する */
	};

	struct DisparityLimitationParameter {
		int limit;		/**< 視差値の制限　0:しない 1:する */
		int lower;		/**< 視差値の下限 */
		int upper;		/**< 視差値の上限 */
	};

	struct AveragingParameter {
		int enb;		/**< 平均化処理しない：0 する：1 */
		int blkshgt;	/**< 平均化ブロック高さ（片側）*/
		int blkswdt;	/**< 平均化ブロック幅（片側）*/
		double intg;	/**< 平均化移動積分幅（片側）*/
		double range;	/**< 平均化分布範囲最大幅（片側）*/
		int dsprt;		/**< 平均化視差含有率 */
		int vldrt;		/**< 平均化有効比率 */
		int reprt;		/**< 平均化置換有効比率 */
	};

	struct AveragingBlockWeightParameter {
		int cntwgt;		/**< ブロックの重み（中央） */
		int nrwgt;		/**< ブロックの重み（近傍） */
		int rndwgt;		/**< ブロックの重み（周辺） */
	};

	struct InterpolateParameter {
		int enb;		/**< 補完処理しない：0 する：1 */
		double lowlmt;	/**< 視補完最小視差値 */
		double slplmt;	/**< 補完幅の最大視差勾配 */
		double insrt;	/**< 補完画素幅の視差値倍率（内側） */
		double rndrt;	/**< 補完画素幅の視差値倍率（周辺） */
		int crstlmt;	/**< 補完ブロックのコントラスト上限値 */
		int hlfil;		/**< 穴埋め処理しない：0 する：1 */
		double hlsz;	/**< 穴埋め幅 */
	};

	struct EdgeInterpolateParameter {
		int edgcmp;		/**< エッジ補完 0:しない 1:する */
		int minblks;	/**< エッジ線分上の最小視差ブロック数 */
		double mincoef;	/**< エッジ視差の最小線形性指数（回帰線の決定係数） */
		int cmpwdt;		/**< エッジ線の補完視差ブロック幅 */
	};

	struct HoughTransformParameter {
		int edgthr1;	/**< Cannyエッジ検出閾値1 */
		int edgthr2;	/**< Cannyエッジ検出閾値2 */
		int linthr;		/**< HoughLinesP投票閾値 */
		int minlen;		/**< HoughLinesP最小線分長 */
		int maxgap;		/**< HoughLinesP最大ギャップ長 */
	};

	struct FrameDecoderParameters {
		SystemParameter system_parameter;
		DisparityLimitationParameter disparity_limitation_parameter;
		AveragingParameter averaging_parameter;
		AveragingBlockWeightParameter averaging_block_weight_parameter;
		InterpolateParameter interpolate_parameter;
		EdgeInterpolateParameter edge_interpolate_parameter;
		HoughTransformParameter hough_transferm_parameter;
	};

	FrameDecoderParameters frame_decoder_parameters_;

	struct WorkBuffers {
		IscImageInfo::ImageType buff_image[4];
		IscImageInfo::DepthType buff_depth[4];
	};
	WorkBuffers work_buffers_;

	int LoadParameterFromFile(const wchar_t* file_name, FrameDecoderParameters* frame_decoder_parameters);
	int SaveParameterToFile(const wchar_t* file_name, const FrameDecoderParameters* frame_decoder_parameters);
	int SetParameterToFrameDecoderModule(const FrameDecoderParameters* frame_decoder_parameters);

	void MakeParameterSet(const int value,		const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set);
	void MakeParameterSet(const float value,	const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set);
	void MakeParameterSet(const double value,	const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set);

	void ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, int* value);
	void ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, float* value);
	void ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, double* value);

};
