﻿// Copyright 2023 ITD Lab Corp.All Rights Reserved.
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
 * @file vm_sdk_wrapper.h
 * @brief VM camera SDK interface
 */

#pragma once

#ifdef VMSDKWRAPPER_EXPORTS
#define VMSDKWRAPPER_API __declspec(dllexport)
#else
#define VMSDKWRAPPER_API __declspec(dllimport)
#endif


/**
 * @namespace ns_vmsdk_wrapper
 */
namespace ns_vmsdk_wrapper
{

/**
 * @class   VmSdkWrapper
 * @brief   interface class
 * this class is an interface for camera sdk
 */
class VMSDKWRAPPER_API VmSdkWrapper
{

public:
	VmSdkWrapper();
	~VmSdkWrapper();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize();

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

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size, unsigned char* read_value, const int read_size);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size);

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
	int InitializeIscIamgeinfo(IscImageInfo* isc_image_info);

	/** @brief release the allocated space.
		@return 0, if successful.
	*/
	int ReleaeIscIamgeinfo(IscImageInfo* isc_image_info);

	/** @brief get captured data.
		@return 0, if successful.
	*/
	int GetData(const IscGetMode* isc_get_mode, IscImageInfo* isc_image_info);

	/** @brief Extract RAW data.
		@return 0, if successful.
	*/
	int Decode(const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode,
		const int width, const int height, IscImageInfo* isc_image_info);

private:
	char module_path_[_MAX_PATH];		/**< This is the path of dll module */
	char file_name_of_dll_[_MAX_PATH];	/**< This is the DLL file name */
	HMODULE dll_handle_;				/**< DLL handle */

	/** struct  VM_CameraParamInfo
	*  This is the structure for camera parameter. do not use nDistanceHistValue and nParallaxThreshold 
	*/
	struct VMCameraParamInfo
	{
		float d_inf;						/**< fD_INF; */
		float bf;							/**< fBF; */
		float base_length;					/**< fBaseLength; */
		float dz;							/**< fdZ; */
		float view_angle;					/**< fViewAngle; */
		unsigned int image_width;			/**< nImageWidth; */
		unsigned int image_height;			/**< nImageHeight; */
		unsigned int product_number;		/**< nProductNumber; */
		unsigned int product_number2;		/**< nProductNumber2; */
		char serial_number[32];				/**< nSerialNumber[32]; */
		unsigned int fpga_version_major;	/**< nFPGA_version_major; */
		unsigned int fpga_version_minor;	/**< nFPGA_version_minor; */
	};

	VMCameraParamInfo vm_camera_param_info_;	/**< camera parameters */

	IscGrabStartMode isc_grab_start_mode_;		/**< This is the current mode of operation */
	IscShutterMode isc_shutter_mode_;			/**< This is the current mode of shutter */

	IscImageInfo isc_image_info_;				/**< This is a temporary image buffer */	

	struct DecodeBuffer {
		unsigned char* split_images[3];		/**< image buffer */
		unsigned char* s0_image;			/**< image buffer */
		unsigned char* s1_image;			/**< image buffer */
		unsigned char* disparity_image;		/**< image buffer */
		unsigned char* mask_image;			/**< image buffer */
		float* disparity;					/**< disparity buffer */
	};
	DecodeBuffer decode_buffer_;

	/** @brief Split RAW data.
		@return 0, if successful.
	*/
	int SplitImage(const bool is_disparity, const int width, const int height, unsigned char* raw_data, unsigned char* image1, unsigned char* image2, unsigned char* image3);

	/** @brief Unpack disparity data.
		@return 0, if successful.
	*/
	int ReCreateParallaxImage(const int width, const int height, unsigned char* src_data, float* temp_disparity, unsigned char* dst_image, unsigned char* mask_image);

	/** @brief Flip the image left and right.
		@return 0, if successful.
	*/
	int FlipImage(const int width, const int height, unsigned char* src_image, unsigned char* dst_image);

	/** @brief Set peculiar removal regiser.
		@return 0, if successful.
	*/
	int SetStereoMatchingsPeculiarRemoval(const int value);

	/** @brief Get peculiar removal regiser.
		@return 0, if successful.
	*/
	int GetStereoMatchingsPeculiarRemoval(int* value);

	/** @brief Set occlusion removal regiser.
		@return 0, if successful.
	*/
	int SetStereoMatchingsOcclusionRemoval(const unsigned int value);

	/** @brief Get occlusion removal regiser.
		@return 0, if successful.
	*/
	int GetStereoMatchingsOcclusionRemoval(unsigned int* value);

	/** @brief Get fuction address.
		@return 0, if successful.
	*/
	int LoadDLLFunction(char* module_path);

	/** @brief free fuction address.
		@return 0, if successful.
	*/
	int UnLoadDLLFunction();


};// class VMSDKWRAPPER_API VmSdkWrapper

}// namespace ns_vmsdk_wrapper
