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
 * @file isc_framedecoder_interface.h
 * @brief Provides an interface for implementing Frame Decoder
 */

#pragma once

#ifdef ISCFRAMEDECODER_EXPORTS
#define ISCFRAMEDECODER_API __declspec(dllexport)
#else
#define ISCFRAMEDECODER_API __declspec(dllimport)
#endif

/**
 * @class   IscFramedecoderInterface
 * @brief   interface class
 * this class is an interface for Frame decoder module
 */
class ISCFRAMEDECODER_API IscFramedecoderInterface
{

public:
	IscFramedecoderInterface();
	~IscFramedecoderInterface();

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

	/** @brief Decode the parallax data, return it to the parallax image and parallax information, and perform averaging and interpolation processing.
		@return 0, if successful.
	*/
	int GetDecodeData(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_block_disparity_data);

	/** @brief Decode the parallax data in Double-Shutter mode, return it to the parallax image and parallax information, and perform averaging and interpolation processing.
		@return 0, if successful.
	*/
	int GetDecodeDataDoubleShutter(IscImageInfo* isc_image_info_in, IscBlockDisparityData* isc_block_disparity_data, IscImageInfo* isc_image_info_out);

private:

	bool parameter_update_request_;

	IscDataProcModuleConfiguration isc_data_proc_module_configuration_;

	wchar_t parameter_file_name_[_MAX_PATH];

	struct DecodeParameter {
		int crstthr;	/**< コントラスト閾値 */
		int grdcrct;	/**< 階調補正モードステータス 0:オフ 1:オン */
	};

	struct CameraMatchingParameter {
		int mtchgt;		/**< マッチングブロック高さ */
		int mtcwdt;		/**< マッチングブロック幅 */
	};

	struct DisparityLimitationParameter {
		int limit;		/**< 視差値の制限　0:しない 1:する */
		int lower;		/**< 視差値の下限 */
		int upper;		/**< 視差値の上限 */
	};

	struct FrameDecoderParameters {
		DisparityLimitationParameter disparity_limitation_parameter;
		CameraMatchingParameter camera_matching_parameter;
		DecodeParameter decode_parameter;
	};

	FrameDecoderParameters frame_decoder_parameters_;

	struct ImageIntType {
		int width;              /**< width */
		int height;             /**< height */
		int channel_count;      /**< number of channels */
		int* image;				/**< data */
	};

	struct WorkBuffers {
		IscImageInfo::ImageType buff_image[8];
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
