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
 * @file isc_sdk_control.h
 * @brief SDK interface
 */
 
#pragma once

/**
 * @class   IscSdkControl
 * @brief   implementation class
 * this class is an implementation for isc sdk wrapper control
 */
class IscSdkControl
{

public:

	IscSdkControl();
	~IscSdkControl();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize(const IscCameraModel isc_camera_model);

	/** @brief ... Shut down the system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	/** @brief Open the device.
		@return 0, if successful.
	*/
	int DeviceOpen();

	/** @brief Close the device.
		@return 0, if successful.
	*/
	int DeviceClose();

	// camera dependent paraneter

	/** @brief whether or not the parameter is implemented.
		@return true, if implemented.
	*/
	bool DeviceOptionIsImplemented(const IscCameraInfo option_name);

	/** @brief whether the parameter is readable.
		@return true, if readable.
	*/
	bool DeviceOptionIsReadable(const IscCameraInfo option_name);

	/** @brief whether the parameter is writable.
		@return true, if writable.
	*/
	bool DeviceOptionIsWritable(const IscCameraInfo option_name);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraInfo option_name, int* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraInfo option_name, int* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionInc(const IscCameraInfo option_name, int* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, int* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const int value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraInfo option_name, float* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraInfo option_name, float* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, float* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const float value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, bool* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const bool value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const char* value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, uint64_t* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const uint64_t value);

	// camera control parameter

	/** @brief whether or not the parameter is implemented.
		@return true, if implemented.
	*/
	bool DeviceOptionIsImplemented(const IscCameraParameter option_name);

	/** @brief whether the parameter is readable.
		@return true, if readable.
	*/
	bool DeviceOptionIsReadable(const IscCameraParameter option_name);

	/** @brief whether the parameter is writable.
		@return true, if writable.
	*/
	bool DeviceOptionIsWritable(const IscCameraParameter option_name);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraParameter option_name, int* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraParameter option_name, int* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionInc(const IscCameraParameter option_name, int* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, int* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const int value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraParameter option_name, float* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraParameter option_name, float* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, float* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const float value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, bool* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const bool value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const char* value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, uint64_t* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const uint64_t value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value);

	// grab control
	/** @brief start image acquisition.
		@return 0, if successful.
	*/
	int Start(const IscGrabStartMode* isc_grab_start_mode);

	/** @brief stop image capture.
		@return 0, if successful.
	*/
	int Stop();

	/** @brief get the current capture mode.
		@return 0, if successful.
	*/
	int GetGrabMode(IscGrabStartMode* isc_grab_start_mode);

	// image & data get 
	/** @brief initialize IscImageInfo. Allocate the required space.
		@return 0, if successful.
	*/
	int InitializeIscIamgeinfo(IscImageInfo* isc_image_Info);

	/** @brief release the allocated space.
		@return 0, if successful.
	*/
	int ReleaeIscIamgeinfo(IscImageInfo* isc_image_Info);

	/** @brief get captured data.
		@return 0, if successful.
	*/
	int GetData(const IscGetMode* isc_get_mode, IscImageInfo* isc_image_info);

private:

	IscCameraModel isc_camera_model_;

	ns_vmsdk_wrapper::VmSdkWrapper* vm_sdk_wrapper_;
	ns_xcsdk_wrapper::XcSdkWrapper* xc_sdk_wrapper_;

};
