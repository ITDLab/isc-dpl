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

/**
* @namespace ns_isc_dpl_c
* @brief define ISC DPL API
* @details APIs
*/
namespace ns_isc_dpl_c {

extern "C" {
	
/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
	@return 0, if successful.
*/
ISCDPLC_EXPORTS_API int Initialize(const IscDplConfiguration* ipc_dpl_configuratio, const IscCameraModel isc_camera_model);

/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
	@return 0, if successful.
 */
ISCDPLC_EXPORTS_API int Terminate();

// camera dependent paraneter
ISCDPLC_EXPORTS_API bool DeviceOptionIsImplemented(const IscCameraInfo option_name);
ISCDPLC_EXPORTS_API bool DeviceOptionIsReadable(const IscCameraInfo option_name);
ISCDPLC_EXPORTS_API bool DeviceOptionIsWritable(const IscCameraInfo option_name);
ISCDPLC_EXPORTS_API int DeviceGetOptionMin(const IscCameraInfo option_name, int* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMax(const IscCameraInfo option_name, int* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionInc(const IscCameraInfo option_name, int* value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraInfo option_name, int* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraInfo option_name, const int value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMin(const IscCameraInfo option_name, float* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMax(const IscCameraInfo option_name, float* value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraInfo option_name, float* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraInfo option_name, const float value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraInfo option_name, bool* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraInfo option_name, const bool value);
ISCDPLC_EXPORTS_API int DeviceGetStringOption(const IscCameraInfo option_name, char* value, const int max_length);
ISCDPLC_EXPORTS_API int DeviceSetStringOption(const IscCameraInfo option_name, const char value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraInfo option_name, uint64_t* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraInfo option_name, const uint64_t value);

// camera control parameter
ISCDPLC_EXPORTS_API bool DeviceOptionIsImplemented(const IscCameraParameter option_name);
ISCDPLC_EXPORTS_API bool DeviceOptionIsReadable(const IscCameraParameter option_name);
ISCDPLC_EXPORTS_API bool DeviceOptionIsWritable(const IscCameraParameter option_name);
ISCDPLC_EXPORTS_API int DeviceGetOptionMin(const IscCameraParameter option_name, int* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMax(const IscCameraParameter option_name, int* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionInc(const IscCameraParameter option_name, int* value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraParameter option_name, int* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraParameter option_name, const int value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMin(const IscCameraParameter option_name, float* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMax(const IscCameraParameter option_name, float* value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraParameter option_name, float* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraParameter option_name, const float value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraParameter option_name, bool* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraParameter option_name, const bool value);
ISCDPLC_EXPORTS_API int DeviceGetStringOption(const IscCameraParameter option_name, char* value);
ISCDPLC_EXPORTS_API int DeviceSetStringOption(const IscCameraParameter option_name, const char value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value);
ISCDPLC_EXPORTS_API int DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraParameter option_name, uint64_t* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraParameter option_name, const uint64_t value);
ISCDPLC_EXPORTS_API int DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value);
ISCDPLC_EXPORTS_API int DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value);

// grab control
ISCDPLC_EXPORTS_API int Start(const IscGrabStartMode isc_grab_start_mode);
ISCDPLC_EXPORTS_API int Stop();
ISCDPLC_EXPORTS_API int GetGrabMode(IscGrabStartMode* isc_grab_start_mode);

// image & data get 
ISCDPLC_EXPORTS_API int InitializeIscIamgeinfo(IscImageInfo* isc_image_Info);
ISCDPLC_EXPORTS_API int ReleaeIscIamgeinfo(IscImageInfo* isc_image_Info);

ISCDPLC_EXPORTS_API int GetCameraData(const IscGetMode* isc_get_mode, IscImageInfo* isc_image_Info);

// get information for depth, distance, ...
ISCDPLC_EXPORTS_API int GetDepth(const int x, const int y, float* depth);
ISCDPLC_EXPORTS_API int GetPosition3D(const int x, const int y, float* x_d, float* y_d, float* z_d);
ISCDPLC_EXPORTS_API int GetStatistics(const int x, const int y, const int width, const int height, IscDataStatistics* isc_data_statistics);

// data processing module settings
ISCDPLC_EXPORTS_API int GetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter);
ISCDPLC_EXPORTS_API int SetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter);

// data processing module result data
ISCDPLC_EXPORTS_API int GetDataProcModuleData(const int module_index, IscDataProcResultData* isc_data_proc_result_data);

} /* extern "C" { */

} /* ns_isc_dpl_c*/
