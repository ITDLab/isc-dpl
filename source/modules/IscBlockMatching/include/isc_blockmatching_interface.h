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
 * @file isc_blockmatching_interface.h
 * @brief Provides an interface for implementing Block Matching
 */
 
#pragma once

#ifdef ISCBLOCKMATCHING_EXPORTS
#define ISCBLOCKMATCHING_API __declspec(dllexport)
#else
#define ISCBLOCKMATCHING_API __declspec(dllimport)
#endif

/**
 * @class   IscBlockMatchingInterface
 * @brief   interface class
 * this class is an interface for Block Matching module
 */
class ISCBLOCKMATCHING_API IscBlockMatchingInterface
{
public:

	IscBlockMatchingInterface();
	~IscBlockMatchingInterface();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	 */
	int Initialize(IscDataProcModuleConfiguration* isc_data_proc_module_configuration);

	/** @brief ... Shut down the system. Don't call any method after calling Terminate().
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

	// for result data initialize

	/** @brief initialize IscBlockDisparityData. Allocate the required space.
		@return 0, if successful.
	*/
	int InitializeIscBlockDisparityData(IscBlockDisparityData* isc_block_disparity_data);

	/** @brief release the allocated space.
		@return 0, if successful.
	*/
	int ReleaeIscIscBlockDisparityData(IscBlockDisparityData* isc_block_disparity_data);

	// run

	/** @brief get parallax pixel information.
		@return 0, if successful.
	*/
	int GetDisparity(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data);

	/** @brief get parallax block information.
		@return 0, if successful.
	*/
	int GetBlockDisparity(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_block_disparity_data);

private:

	bool parameter_update_request_;

	IscDataProcModuleConfiguration isc_data_proc_module_configuration_;

	wchar_t parameter_file_name_[_MAX_PATH];

	struct SystemParameter {
		bool enabled_opencl_for_avedisp;	/**< 視差平均化処理にOpenCLの使用を設定する */
	};

	struct MatchingParameter {
		int imghgt;		/**< 補正画像の高さ */
		int imgwdt;		/**< 補正画像の幅 */
		int depth;		/**< マッチング探索幅 */
		int blkhgt;		/**< 視差ブロック高さ */
		int blkwdt;		/**< 視差ブロック幅 */
		int mtchgt;		/**< マッチングブロック高さ */
		int mtcwdt;		/**< マッチングブロック幅 */
		int blkofsx;	/**< 視差ブロック横オフセット */
		int blkofsy;	/**< 視差ブロック縦オフセット */
		int crstthr;	/**< コントラスト閾値 */
	};

	struct BackMatchingParameter {
		int enb;		/**< バックマッチング 0:しない 1:する */
		int bkevlwdt;	/**< バックマッチング視差評価領域幅（片側） */
		int bkevlrng;	/**< バックマッチング視差評価視差値幅 */
		int bkvldrt;	/**< バックマッチング評価視差正当率（％） */
		int bkzrrt;		/**< バックマッチング評価視差ゼロ率（％） */
	};

	struct BlockMatchingParameters {
		SystemParameter system_parameter;
		MatchingParameter matching_parameter;
		BackMatchingParameter back_matching_parameter;
	};

	BlockMatchingParameters block_matching_parameters_;

	struct WorkBuffers {
		IscImageInfo::ImageType buff_image[4];
		IscImageInfo::DepthType buff_depth[4];
	};
	WorkBuffers work_buffers_;

	int LoadParameterFromFile(const wchar_t* file_name, BlockMatchingParameters* block_matching_parameters);
	int SaveParameterToFile(const wchar_t* file_name, const BlockMatchingParameters* block_matching_parameters);
	int SetParameterToBlockMatchingModule(const BlockMatchingParameters* block_matching_parameters);

	void MakeParameterSet(const int value,		const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set);
	void MakeParameterSet(const float value,	const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set);
	void MakeParameterSet(const double value,	const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set);

	void ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, int* value);
	void ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, float* value);
	void ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, double* value);

};

