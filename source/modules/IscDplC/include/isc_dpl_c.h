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
 * @file isc_dpl_c.h
 * @brief main control class for ISC_DPL (C interface)
 */

#pragma once

#ifdef ISCDPLC_EXPORTS
#define ISCDPLC_EXPORTS_API __declspec(dllexport)
#else
#define ISCDPLC_EXPORTS_API __declspec(dllimport)
#endif

extern "C" {
	
	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplInitialize(const IscDplConfiguration* ipc_dpl_configuration);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplTerminate();

	// camera dependent paraneter

	/** @brief whether or not the parameter is implemented.
		@return true, if implemented.
	*/
	ISCDPLC_EXPORTS_API bool DplDeviceOptionIsImplementedInfo(const IscCameraInfo option_name);

	/** @brief whether the parameter is readable.
		@return true, if readable.
	*/
	ISCDPLC_EXPORTS_API bool DplDeviceOptionIsReadableInfo(const IscCameraInfo option_name);

	/** @brief whether the parameter is writable.
		@return true, if writable.
	*/
	ISCDPLC_EXPORTS_API bool DplDeviceOptionIsWritableInfo(const IscCameraInfo option_name);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinInfoInt(const IscCameraInfo option_name, int* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxInfoInt(const IscCameraInfo option_name, int* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionIncInfoInt(const IscCameraInfo option_name, int* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoInt(const IscCameraInfo option_name, int* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoInt(const IscCameraInfo option_name, const int value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinInfoFloat(const IscCameraInfo option_name, float* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxInfoFloat(const IscCameraInfo option_name, float* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoFloat(const IscCameraInfo option_name, float* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoFloat(const IscCameraInfo option_name, const float value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoBool(const IscCameraInfo option_name, bool* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoBool(const IscCameraInfo option_name, const bool value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoChar(const IscCameraInfo option_name, char* value, const int max_length);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoChar(const IscCameraInfo option_name, const char* value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinInfoInt64(const IscCameraInfo option_name, uint64_t* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxInfoInt64(const IscCameraInfo option_name, uint64_t* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionIncInfoInt64(const IscCameraInfo option_name, uint64_t* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoInt64(const IscCameraInfo option_name, uint64_t* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoInt64(const IscCameraInfo option_name, const uint64_t value);

	// camera control parameter

	/** @brief whether or not the parameter is implemented.
		@return true, if implemented.
	*/
	ISCDPLC_EXPORTS_API bool DplDeviceOptionIsImplementedPara(const IscCameraParameter option_name);

	/** @brief whether the parameter is readable.
		@return true, if readable.
	*/
	ISCDPLC_EXPORTS_API bool DplDeviceOptionIsReadablePara(const IscCameraParameter option_name);

	/** @brief whether the parameter is writable.
		@return true, if writable.
	*/
	ISCDPLC_EXPORTS_API bool DplDeviceOptionIsWritablePara(const IscCameraParameter option_name);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinParaInt(const IscCameraParameter option_name, int* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxParaInt(const IscCameraParameter option_name, int* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionIncParaInt(const IscCameraParameter option_name, int* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaInt(const IscCameraParameter option_name, int* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaInt(const IscCameraParameter option_name, const int value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinParaFloat(const IscCameraParameter option_name, float* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxParaFloat(const IscCameraParameter option_name, float* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaFloat(const IscCameraParameter option_name, float* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaFloat(const IscCameraParameter option_name, const float value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaBool(const IscCameraParameter option_name, bool* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaBool(const IscCameraParameter option_name, const bool value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaChar(const IscCameraParameter option_name, char* value, const int max_length);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaChar(const IscCameraParameter option_name, const char* value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinParaInt64(const IscCameraParameter option_name, uint64_t* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxParaInt64(const IscCameraParameter option_name, uint64_t* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionIncParaInt64(const IscCameraParameter option_name, uint64_t* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaInt64(const IscCameraParameter option_name, uint64_t* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaInt64(const IscCameraParameter option_name, const uint64_t value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaShMode(const IscCameraParameter option_name, IscShutterMode* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaShMode(const IscCameraParameter option_name, const IscShutterMode value);

	// grab control

	/** @brief start image acquisition.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplStart(const IscStartMode* isc_start_mode);

	/** @brief stop image capture.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplStop();

	/** @brief get the current capture mode.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetGrabMode(IscGrabStartMode* isc_grab_start_mode);

	// image & data get 

	/** @brief initialize IscImageInfo. Allocate the required space.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplInitializeIscIamgeinfo(IscImageInfo* isc_image_Info);

	/** @brief release the allocated space.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplReleaeIscIamgeinfo(IscImageInfo* isc_image_Info);

	/** @brief get captured data.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetCameraData(IscImageInfo* isc_image_Info);

	/** @brief get the information of the file header.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header, IscPlayFileInformation* play_file_information);

	/** @brief Set the read Frame to the specified number.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplSetReadFrameNumber(const __int64 frame_number);

	/** @brief Get the status.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetFileReadStatus(__int64* frame_number, IscFileReadStatus* file_read_status);

	// get information for depth, distance, ...

	/** @brief gets the distance of the given coordinates.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetPositionDepth(const int x, const int y, const IscImageInfo* isc_image_info, float* disparity, float* depth);

	/** @brief gets the 3D position of the given coordinates.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetPosition3D(const int x, const int y, const IscImageInfo* isc_image_info, float* x_d, float* y_d, float* z_d);

	/** @brief get information for the specified region.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetAreaStatistics(const int x, const int y, const int width, const int height, const IscImageInfo* isc_image_info, IscAreaDataStatistics* isc_data_statistics);

	// data processing module settings

	/** @brief get the number of installed modules.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetTotalModuleCount(int* total_count);

	/** @brief get the name of the specified module.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetModuleNameByIndex(const int module_index, wchar_t* module_name, int max_length);

	/** @brief get the parameters of the specified module.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter);

	/** @brief sets a parameter to the specified module.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplSetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file);

	/** @brief gets the name of the configuration file for the specified module.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetParameterFileName(const int module_index, wchar_t* file_name, const int max_length);

	/** @brief requests the specified module to reload its parameters.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplReloadParameterFromFile(const int module_index, const wchar_t* file_name, const bool is_valid);

	// data processing module result data

	/** @brief initialize IscDataProcResultData. Allocate the required space.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplInitializeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data);

	/** @brief release the allocated space.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplReleaeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data);

	/** @brief get module processing result.
		@return 0, if successful.
	*/
	ISCDPLC_EXPORTS_API int DplGetDataProcModuleData(IscDataProcResultData* isc_data_proc_result_data);

} /* extern "C" { */

