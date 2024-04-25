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
 * @file k4a_sdk_wrapper.cpp
 * @brief Provides an interface to 4KA SDK
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a common interface for using the SDK for ISC100XC.
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <cassert>
#include <shlwapi.h>

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"

#include "k4a_sdk_wrapper.h"

#include "utility.h"

#include "opencv2\opencv.hpp"
#include "ISCSDKLib_define.h"

#pragma comment (lib, "shlwapi")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world480d")
#else
#pragma comment (lib,"opencv_world480")
#endif

namespace ns_k4asdk_wrapper
{

	constexpr char kISC_4KA_DRV_FILE_NAME[] = "ISCSDKLib4K.dll";

	// Function typedef

	//int OpenISC();
	typedef int (WINAPI* TOpenISC)();
	TOpenISC openISC = NULL;

	//int CloseISC();
	typedef int (WINAPI* TCloseISC)();
	TCloseISC closeISC = NULL;

	//int StartGrab(int nMode);
	typedef int (WINAPI* TStartGrab)(int);
	TStartGrab startGrab = NULL;

	//int StopGrab();
	typedef int (WINAPI* TStopGrab)();
	TStopGrab stopGrab = NULL;

	// int GetImageEx(struct ISC_ImageInfo* image,int waitTime);
	typedef int (WINAPI* TGetImageEx)(struct ISCSDKLib::ISC_ImageInfo*, int);
	TGetImageEx getImageEx = NULL;

	// int GetRawImageEx(struct ISC_RawImageInfo* image, int waitTime);
	typedef int (WINAPI* TGetRawImageEx)(struct ISCSDKLib::ISC_RawImageInfo*, int);
	TGetRawImageEx getRawImageEx = NULL;

	// int GetDepthInfo(float* depth);
	typedef int (WINAPI* TGetDepthInfo)(float*);
	TGetDepthInfo getDepthInfo = NULL;

	//int GetCameraParamInfo(struct CameraParamInfo* info);
	typedef int (WINAPI* TGetCameraParamInfo)(struct ISCSDKLib::CameraParamInfo*);
	TGetCameraParamInfo getCameraParamInfo = NULL;

	//int GetImageSize(unsigned int* pnWidth, unsigned int* pnHeight);
	typedef int (WINAPI* TGetImageSize)(int*, int*);
	TGetImageSize getImageSize = NULL;

	// int GetCorrectedImageSize(int* width, int* height);
	typedef int (WINAPI* TGetCorrectedImageSize)(int*, int*);
	TGetCorrectedImageSize getCorrectedImageSize = NULL;

	//int SetAutoCalibration(int nMode);
	typedef int (WINAPI* TSetAutoCalibration)(int);
	TSetAutoCalibration setAutoCalibration = NULL;

	//int GetAutoCalibration(int* nMode);
	typedef int (WINAPI* TGetAutoCalibration)(int*);
	TGetAutoCalibration getAutoCalibration = NULL;

	//int SetShutterControlModeEx(enum ISC_ShutterMode mode);
	typedef int (WINAPI* TSetShutterControlModeEx)(int);
	TSetShutterControlModeEx setShutterControlModeEx = NULL;

	//int GetShutterControlModeEx(enum ISC_ShutterMode *mode);
	typedef int (WINAPI* TGetShutterControlModeEx)(int*);
	TGetShutterControlModeEx getShutterControlModeEx = NULL;

	//int SetExposureValue(int value);
	typedef int (WINAPI* TSetExposureValue)(int);
	TSetExposureValue setExposureValue = NULL;

	//int GetExposureValue(int* value);
	typedef int (WINAPI* TGetExposureValue)(int*);
	TGetExposureValue getExposureValue = NULL;

	//int SetExposureFineValue(int value);
	typedef int (WINAPI* TSetExposureFineValue)(int);
	TSetExposureFineValue setExposureFineValue = NULL;

	//int GetExposureFineValue(int* value);
	typedef int (WINAPI* TGetExposureFineValue)(int);
	TGetExposureFineValue getExposureFineValue = NULL;

	//int SetMedianTargetValue(int value);
	typedef int (WINAPI* TSetMedianTargetValue)(int);
	TSetMedianTargetValue setMedianTargetValue = NULL;

	//int GetMedianTargetValue(int* value);
	typedef int (WINAPI* TGetMedianTargetValue)(int);
	TGetMedianTargetValue getMedianTargetValue = NULL;

	//int SetGainValue(unsigned int nValue);
	typedef int (WINAPI* TSetGainValue)(int);
	TSetGainValue setGainValue = NULL;

	//int GetGainValue(unsigned int* pnValue);
	typedef int (WINAPI* TGetGainValue)(int*);
	TGetGainValue getGainValue = NULL;

	//int SetNoiseFilter(unsigned int nValue);
	typedef int (WINAPI* TSetNoiseFilter)(int);
	TSetNoiseFilter setNoiseFilter = NULL;

	//int GetNoiseFilter(unsigned int* pnValue);
	typedef int (WINAPI* TGetNoiseFilter)(int*);
	TGetNoiseFilter getNoiseFilter = NULL;

	//int SetMeasAreaEx(int mode, int nTop, int nLeft, int nRight, int nBottom, int nTop_Left, int nTop_Right, int nBottom_Left, int nBottom_Right);
	typedef int (WINAPI* TSetMeasAreaEx)(int, int, int, int, int, int, int, int, int);
	TSetMeasAreaEx setMeasAreaEx = NULL;

	//int GetMeasAreaEx(int* mode, int* nTop, int* nLeft, int* nRight, int* nBottom, int* nTop_Left, int* nTop_Right, int* nBottom_Left, int* nBottom_Right);
	typedef int (WINAPI* TGetMeasAreaEx)(int*, int*, int*, int*, int*, int*, int*, int*, int*);
	TGetMeasAreaEx getMeasAreaEx = NULL;

	//int GetImageFromFile(struct ISC_ImageInfo* image, char* filename);
	typedef int (WINAPI* TGetImageFromFile)(struct ISCSDKLib::ISC_ImageInfo*, char*);
	TGetImageFromFile getImageFromFile = NULL;

	//int GetRawImageFromFile(struct ISC_RawImageInfo* image, char* filename);
	typedef int (WINAPI* TGetRawImageFromFile)(struct ISCSDKLib::ISC_RawImageInfo*, char*);
	TGetRawImageFromFile getRawImageFromFile = NULL;

	//int GetFullFrameInfo(unsigned char* pBuffer, int *width, int *height);
	typedef int (WINAPI* TGetFullFrameInfo)(unsigned char*, int*, int*);
	TGetFullFrameInfo getFullFrameInfo = NULL;

	//int GetFullFrameInfo2(unsigned char* pBuffer, int *width, int *height, int waitTime);
	typedef int (WINAPI* TGetFullFrameInfo2)(unsigned char*, int*, int*, int);
	TGetFullFrameInfo2 getFullFrameInfo2 = NULL;

	//int SetCameraRegData(unsigned char* pwBuf, int wSize);
	typedef int (WINAPI* TSetCameraRegData)(unsigned char*, int);
	TSetCameraRegData setCameraRegData = NULL;

	//int GetCameraRegData(unsigned char* pwBuf, unsigned char* prBuf, int wSize, int rSize);
	typedef int (WINAPI* TGetCameraRegData)(unsigned char*, unsigned char*, int, int);
	TGetCameraRegData getCameraRegData = NULL;

	//int SetRectTable(char *rect_r_file, char *rect_l_file);
	typedef int (WINAPI* TSetRectTable)(char*, char*);
	TSetRectTable setRectTable = NULL;

	//void FlushLog();
	typedef int (WINAPI* TFlushLog)();
	TFlushLog flushLog = NULL;

/**
 * constructor
 *
 */
K4aSdkWrapper::K4aSdkWrapper()
		:module_path_(), file_name_of_dll_(), dll_handle_(NULL), k4a_camera_param_info_(), isc_grab_start_mode_(), isc_shutter_mode_(IscShutterMode::kManualShutter), isc_image_info_(), work_buffer_(), decode_buffer_()
{
	isc_grab_start_mode_.isc_grab_mode = IscGrabMode::kParallax;
	isc_grab_start_mode_.isc_grab_color_mode = IscGrabColorMode::kColorOFF;

}

/**
 * destructor
 *
 */
K4aSdkWrapper::~K4aSdkWrapper()
{

}

/**
 * Initialize the class.
 *
 * @return 0 if successful.
 */
int K4aSdkWrapper::Initialize()
{
	// get module path
	char module_path_name[MAX_PATH + 1] = {};
	GetModuleFileNameA(NULL, module_path_name, MAX_PATH);
	sprintf_s(module_path_, "%s", module_path_name);
	PathRemoveFileSpecA(module_path_);

	// value in specification
	constexpr int camera_width = 3840;
	constexpr int camea_height = 1920;

	size_t frame_size = camera_width * camea_height;

	for (int i = 0; i < 3; i++) {
		decode_buffer_.split_images[i] = new unsigned char[frame_size];
	}
	decode_buffer_.s0_image = new unsigned char[frame_size];
	decode_buffer_.s1_image = new unsigned char[frame_size];
	decode_buffer_.disparity_image = new unsigned char[frame_size];
	decode_buffer_.mask_image = new unsigned char[frame_size];
	decode_buffer_.disparity = new float[frame_size];

	decode_buffer_.work_buffer.max_width = camera_width;
	decode_buffer_.work_buffer.max_height = camea_height;
	size_t buffer_size = decode_buffer_.work_buffer.max_width * decode_buffer_.work_buffer.max_height * 3;
	for (int i = 0; i < 4; i++) {
		decode_buffer_.work_buffer.buffer[i] = new unsigned char[buffer_size];
	}

	// Set the specified value for initialization
	k4a_camera_param_info_.image_width	= camera_width;
	k4a_camera_param_info_.image_height	= camea_height;

	return ISC_OK;
}

/**
 * Terminate the class. Currently, do nothing.
 *
 * @return 0 if successful.
 */
int K4aSdkWrapper::Terminate()
{

	decode_buffer_.work_buffer.max_width = 0;
	decode_buffer_.work_buffer.max_height = 0;
	for (int i = 0; i < 4; i++) {
		delete[] decode_buffer_.work_buffer.buffer[i];
		decode_buffer_.work_buffer.buffer[i] = nullptr;
	}

	for (int i = 0; i < 3; i++) {
		delete[] decode_buffer_.split_images[i];
		decode_buffer_.split_images[i] = nullptr;
	}
	delete[] decode_buffer_.s0_image;
	decode_buffer_.s0_image = nullptr;
	delete[] decode_buffer_.s1_image;
	decode_buffer_.s1_image = nullptr;
	delete[] decode_buffer_.disparity_image;
	decode_buffer_.disparity_image = nullptr;
	delete[] decode_buffer_.mask_image;
	decode_buffer_.mask_image = nullptr;
	delete[] decode_buffer_.disparity;
	decode_buffer_.disparity = nullptr;

	return ISC_OK;
}

/**
 * Open your camera and connect.
 *
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceOpen()
{

	int ret = LoadDLLFunction(module_path_);
	if (ret != DPC_E_OK) {
		return ret;
	}

	if (openISC != NULL) {
		ret = openISC();
	}

	if (ret == ISC_OK) {
		memset(&k4a_camera_param_info_, 0, sizeof(k4a_camera_param_info_));

		ISCSDKLib::CameraParamInfo paramInfo = {};
		ret = getCameraParamInfo(&paramInfo);

		if (ret == ISC_OK) {
			k4a_camera_param_info_.d_inf				= paramInfo.fD_INF;
			k4a_camera_param_info_.bf					= paramInfo.fBF;
			k4a_camera_param_info_.base_length			= paramInfo.fBaseLength;
			k4a_camera_param_info_.dz					= 0;	// not supported
			k4a_camera_param_info_.view_angle			= paramInfo.fViewAngle;
			k4a_camera_param_info_.image_width			= paramInfo.nImageWidth;
			k4a_camera_param_info_.image_height			= paramInfo.nImageHeight;
			k4a_camera_param_info_.product_number		= paramInfo.nProductNumber;
			sprintf_s(k4a_camera_param_info_.serial_number, "%s", paramInfo.szSerialNumber);
			k4a_camera_param_info_.fpga_version_major	= paramInfo.nFPGA_version_major;
			k4a_camera_param_info_.fpga_version_minor	= paramInfo.nFPGA_version_minor;	

			IscCameraParameter option = IscCameraParameter::kShutterMode;
			IscShutterMode shutter_mode = IscShutterMode::kManualShutter;
			int ret_temp = DeviceGetOption(option, &shutter_mode);

			if (ret_temp == DPC_E_OK) {
				isc_shutter_mode_ = shutter_mode;
			}

			InitializeIscIamgeinfo(&isc_image_info_);

			work_buffer_.max_width = k4a_camera_param_info_.image_width;
			work_buffer_.max_height = k4a_camera_param_info_.image_height;
			size_t buffer_size = work_buffer_.max_width * work_buffer_.max_height * 3;
			for (int i = 0; i < 4; i++) {
				work_buffer_.buffer[i] = new unsigned char[buffer_size];
			}
		}
	}

	int ret_value = DPC_E_OK;

	if (ret == ISC_OK) {
	}
	else {
		ret_value = CAMCONTROL_E_OPEN_DEVICE_FAILED;
	}

	return ret_value;
}

/**
 * Disconnect the camera.
 *
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceClose()
{

	work_buffer_.max_width = 0;
	work_buffer_.max_height = 0;
	for (int i = 0; i < 4; i++) {
		delete[] work_buffer_.buffer[i];
		work_buffer_.buffer[i] = nullptr;
	}

	ReleaeIscIamgeinfo(&isc_image_info_);

	int ret = ISC_OK;
	if (closeISC != NULL) {
		ret = closeISC();
	}

	int ret_value = DPC_E_OK;
	if (ret == ISC_OK) {
	}
	else {
		ret_value = CAMCONTROL_E_CLOSE_DEVICE_FAILED;
	}

	UnLoadDLLFunction();

	return ret;
}

// camera dependent paraneter
/**
 * whether or not the parameter is implemented.
 *
 * @param[in] option_name target parameter.
 * @return true, if implemented.
 */
bool K4aSdkWrapper::DeviceOptionIsImplemented(const IscCameraInfo option_name)
{

	bool ret_value = false;

	switch (option_name) {
	case IscCameraInfo::kBF:
		ret_value = true;
		break;

	case IscCameraInfo::kDINF:
		ret_value = true;
		break;

	case IscCameraInfo::kDz:
		ret_value = true;
		break;

	case IscCameraInfo::kBaseLength:
		ret_value = true;
		break;

	case IscCameraInfo::kViewAngle:
		ret_value = true;
		break;

	case IscCameraInfo::kProductID:
		ret_value = true;
		break;

	case IscCameraInfo::kSerialNumber:
		ret_value = true;
		break;

	case IscCameraInfo::kFpgaVersion:
		ret_value = true;
		break;

	case IscCameraInfo::kWidthMax:
		ret_value = true;
		break;

	case IscCameraInfo::kHeightMax:
		ret_value = true;
		break;

	}

	return ret_value;
}

/**
 * whether the parameter is readable.
 *
 * @param[in] option_name target parameter.
 * @return true, if readable.
 */
bool K4aSdkWrapper::DeviceOptionIsReadable(const IscCameraInfo option_name)
{
	bool ret_value = false;

	switch (option_name) {
	case IscCameraInfo::kBF:
		ret_value = true;
		break;

	case IscCameraInfo::kDINF:
		ret_value = true;
		break;

	case IscCameraInfo::kDz:
		ret_value = true;
		break;

	case IscCameraInfo::kBaseLength:
		ret_value = true;
		break;

	case IscCameraInfo::kViewAngle:
		ret_value = true;
		break;

	case IscCameraInfo::kProductID:
		ret_value = true;
		break;

	case IscCameraInfo::kSerialNumber:
		ret_value = true;
		break;

	case IscCameraInfo::kFpgaVersion:
		ret_value = true;
		break;

	case IscCameraInfo::kWidthMax:
		ret_value = true;
		break;

	case IscCameraInfo::kHeightMax:
		ret_value = true;
		break;

	}

	return ret_value;
}

/**
 * whether the parameter is writable.
 *
 * @param[in] option_name target parameter.
 * @return true, if writable.
 */
bool K4aSdkWrapper::DeviceOptionIsWritable(const IscCameraInfo option_name)
{
	bool ret_value = false;

	switch (option_name) {
	case IscCameraInfo::kBF:
		ret_value = false;
		break;

	case IscCameraInfo::kDINF:
		ret_value = false;
		break;

	case IscCameraInfo::kDz:
		ret_value = false;
		break;

	case IscCameraInfo::kBaseLength:
		ret_value = false;
		break;

	case IscCameraInfo::kViewAngle:
		ret_value = false;
		break;

	case IscCameraInfo::kProductID:
		ret_value = false;
		break;

	case IscCameraInfo::kSerialNumber:
		ret_value = false;
		break;

	case IscCameraInfo::kFpgaVersion:
		ret_value = false;
		break;

	case IscCameraInfo::kWidthMax:
		ret_value = false;
		break;

	case IscCameraInfo::kHeightMax:
		ret_value = false;
		break;

	}

	return ret_value;
}

/**
 * get the minimum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value minimum value. 
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, int* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get get the maximum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value maximum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, int* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * gets the unit of increment or decrement for the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value unit value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionInc(const IscCameraInfo option_name, int* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kWidthMax:
		*value = k4a_camera_param_info_.image_width;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kHeightMax:
		*value = k4a_camera_param_info_.image_height;
		ret_value = DPC_E_OK;
		break;

	}

	return ret_value;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const int value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the minimum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value minimum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, float* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get get the maximum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value maximum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, float* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, float* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kBF:
		*value = k4a_camera_param_info_.bf;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kDINF:
		*value = k4a_camera_param_info_.d_inf;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kDz:
		*value = k4a_camera_param_info_.dz;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kBaseLength:
		*value = k4a_camera_param_info_.base_length;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kViewAngle:
		*value = k4a_camera_param_info_.view_angle;
		ret_value = DPC_E_OK;
		break;
	}

	return ret_value;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const float value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}


/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, bool* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const bool value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[in] max_legth maximum size of output array.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kSerialNumber:
		sprintf_s(value, max_length, "%s", k4a_camera_param_info_.serial_number);
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kFpgaVersion:
		break;

	case IscCameraInfo::kWidthMax:
		break;

	case IscCameraInfo::kHeightMax:
		break;

	}

	return ret_value;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const char* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the minimum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value minimum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get get the maximum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value maximum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * gets the unit of increment or decrement for the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value unit value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, uint64_t* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kProductID:
		*value = (uint64_t)k4a_camera_param_info_.product_number;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kSerialNumber:
		break;

	case IscCameraInfo::kFpgaVersion:
		*value = k4a_camera_param_info_.fpga_version_minor | ((uint64_t)k4a_camera_param_info_.fpga_version_major) << 32;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kWidthMax:
		break;

	case IscCameraInfo::kHeightMax:
		break;

	}

	return ret_value;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const uint64_t value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

// camera control parameter
/**
 * whether or not the parameter is implemented.
 *
 * @param[in] option_name target parameter.
 * @return true, if implemented.
 */
bool K4aSdkWrapper::DeviceOptionIsImplemented(const IscCameraParameter option_name)
{
	bool ret_value = false;

	switch (option_name) {
	case IscCameraParameter::kMonoS0Image:
		ret_value = true;
		break;

	case IscCameraParameter::kMonoS1Image:
		ret_value = true;
		break;

	case IscCameraParameter::kDepthData:
		ret_value = true;
		break;

	case IscCameraParameter::kColorImage:
		ret_value = false;
		break;

	case IscCameraParameter::kColorImageCorrect:
		ret_value = false;
		break;

	case IscCameraParameter::kAlternatelyColorImage:
		ret_value = false;
		break;

	case IscCameraParameter::kBayerColorImage:
		ret_value = false;
		break;

	case IscCameraParameter::kShutterMode:
		ret_value = true;
		break;

	case IscCameraParameter::kManualShutter:
		ret_value = true;
		break;

	case IscCameraParameter::kSingleShutter:
		ret_value = true;
		break;

	case IscCameraParameter::kDoubleShutter:
		ret_value = false;
		break;

	case IscCameraParameter::kDoubleShutter2:
		ret_value = false;
		break;

	case IscCameraParameter::kExposure:
		ret_value = true;
		break;

	case IscCameraParameter::kFineExposure:
		ret_value = false;
		break;

	case IscCameraParameter::kGain:
		ret_value = true;
		break;

	case IscCameraParameter::kHrMode:
		ret_value = false;
		break;

	case IscCameraParameter::kHdrMode:
		ret_value = false;
		break;

	case IscCameraParameter::kAutoCalibration:
		ret_value = true;
		break;

	case IscCameraParameter::kManualCalibration:
		ret_value = true;
		break;

	case IscCameraParameter::kOcclusionRemoval:
		ret_value = true;
		break;

	case IscCameraParameter::kPeculiarRemoval:
		ret_value = true;
		break;

	}

	return ret_value;
}

/**
 * whether the parameter is readable.
 *
 * @param[in] option_name target parameter.
 * @return true, if readable.
 */
bool K4aSdkWrapper::DeviceOptionIsReadable(const IscCameraParameter option_name)
{
	bool ret_value = false;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		ret_value = true;
		break;

	case IscCameraParameter::kExposure:
		ret_value = true;
		break;

	case IscCameraParameter::kGain:
		ret_value = true;
		break;

	case IscCameraParameter::kOcclusionRemoval:
		ret_value = true;
		break;

	case IscCameraParameter::kPeculiarRemoval:
		ret_value = true;
		break;

	}

	return ret_value;
}

/**
 * whether the parameter is writable.
 *
 * @param[in] option_name target parameter.
 * @return true, if writable.
 */
bool K4aSdkWrapper::DeviceOptionIsWritable(const IscCameraParameter option_name)
{
	bool ret_value = false;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		ret_value = true;
		break;

	case IscCameraParameter::kExposure:
		ret_value = true;
		break;

	case IscCameraParameter::kGain:
		ret_value = true;
		break;

	case IscCameraParameter::kOcclusionRemoval:
		ret_value = true;
		break;

	case IscCameraParameter::kPeculiarRemoval:
		ret_value = true;
		break;

	}

	return ret_value;
}

/**
 * get the minimum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value minimum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
		*value = 0;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kGain:
		*value = 0;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kOcclusionRemoval:
		*value = 0;
		ret_value = DPC_E_OK;
		break;

	}

	return ret_value;
}

/**
 * get get the maximum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value maximum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
		*value = 3346;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kGain:
		*value = 300;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kOcclusionRemoval:
		*value = 7;
		ret_value = DPC_E_OK;
		break;
	}

	return ret_value;
}

/**
 * gets the unit of increment or decrement for the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value unit value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionInc(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraParameter::kExposure:
		*value = 1;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kGain:
		*value = 1;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kOcclusionRemoval:
		*value = 1;
		ret_value = DPC_E_OK;
		break;
	}

	return ret_value;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	int get_value = 0;
	unsigned int get_value_un = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
		ret_value = getExposureValue(&get_value);
		if (ret_value == ISC_OK) {
			*value = static_cast<int>(get_value);
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kGain:
		ret_value = getGainValue(&get_value);
		if (ret_value == ISC_OK) {
			*value = static_cast<int>(get_value);
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kOcclusionRemoval:
		ret_value = GetStereoMatchingsOcclusionRemoval(&get_value_un);
		if (ret_value == ISC_OK) {
			*value = static_cast<int>(get_value_un);
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;


	}

	return ret_value;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const int value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;

	const unsigned int set_value = static_cast<unsigned int>(value);

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
	{
		const unsigned int exposure_value = MAX(0, set_value);
		ret_value = setExposureValue(exposure_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
	}
		break;

	case IscCameraParameter::kGain:
		ret_value = setGainValue(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kOcclusionRemoval:
		ret_value = SetStereoMatchingsOcclusionRemoval(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
		break;
	}

	return ret_value;
}

/**
 * get the minimum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value minimum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, float* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get get the maximum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value maximum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, float* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, float* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const float value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, bool* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = false;

	int get_value = 0;

	switch (option_name) {
	case IscCameraParameter::kAutoCalibration:
		ret_value = getAutoCalibration(&get_value);
		if (ret_value == ISC_OK) {
			if ((get_value & AUTOCALIBRATION_STATUS_BIT_AUTO_ON) != 0) {
				*value = true;
			}
			else {
				*value = false;
			}
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kManualCalibration:
		ret_value = getAutoCalibration(&get_value);
		if (ret_value == ISC_OK) {
			if ((get_value & AUTOCALIBRATION_STATUS_BIT_MANUAL_RUNNING) != 0) {
				*value = true;
			}
			else {
				*value = false;
			}
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kPeculiarRemoval:
		ret_value = GetStereoMatchingsPeculiarRemoval(&get_value);
		if (ret_value == ISC_OK) {
			if ((get_value & 0x00000001) != 0) {
				*value = true;
			}
			else {
				*value = false;
			}
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;
	}

	return ret_value;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const bool value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	unsigned int set_value = 0;

	switch (option_name) {
	case IscCameraParameter::kAutoCalibration:
		if (value) {
			set_value = AUTOCALIBRATION_STATUS_BIT_AUTO_ON;
		}
		else {
			set_value = 0;
		}
		ret_value = setAutoCalibration(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kManualCalibration:
		if (value) {
			set_value = AUTOCALIBRATION_STATUS_BIT_MANUAL_RUNNING;

			ret_value = setAutoCalibration(set_value);
			if (ret_value == ISC_OK) {
				ret_value = DPC_E_OK;
			}
			else {
				ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
			}
		}
		else {
			ret_value = DPC_E_OK;
		}
		break;

	case IscCameraParameter::kPeculiarRemoval:
		if (value) {
			ret_value = SetStereoMatchingsPeculiarRemoval(3);
			if (ret_value == ISC_OK) {
				ret_value = DPC_E_OK;
			}
			else {
				ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
			}
		}
		else {
			ret_value = SetStereoMatchingsPeculiarRemoval(0);
			if (ret_value == ISC_OK) {
				ret_value = DPC_E_OK;
			}
			else {
				ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
			}
		}
		break;

	}

	return ret_value;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[in] max_legth maximum size of output array.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const char* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the minimum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value minimum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get get the maximum value of a parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value maximum value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * gets the unit of increment or decrement for the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value unit value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, uint64_t* value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const uint64_t value)
{
	// This feature is not provided
	return CAMCONTROL_E_INVALID_REQUEST;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[out] value obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value)
{
	*value = IscShutterMode::kManualShutter;

	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	int get_value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		ret_value = getShutterControlModeEx(&get_value);
		if (ret_value == ISC_OK) {
			switch (get_value) {
			case 0:
				*value = IscShutterMode::kManualShutter;
				break;
			case 1:
				*value = IscShutterMode::kSingleShutter;
				break;
			case 2:
				*value = IscShutterMode::kDoubleShutter;
				break;
			case 3:
				*value = IscShutterMode::kDoubleShutter2;
				break;
			}
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;
	}

	return ret_value;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] value value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value)
{

	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	int set_value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		switch (value) {
		case IscShutterMode::kManualShutter:
			set_value = 0;
			break;

		case IscShutterMode::kSingleShutter:
			set_value = 1;
			break;

		case IscShutterMode::kDoubleShutter:
			set_value = 2;
			break;

		case IscShutterMode::kDoubleShutter2:
			set_value = 3;
			break;
		}

		ret_value = setShutterControlModeEx(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
		break;
	}

	if (ret_value == DPC_E_OK) {
		isc_shutter_mode_ = value;
	}

	return ret_value;
}

/**
 * get the value of the parameter.
 *
 * @param[in] option_name target parameter.
 * @param[int] write_value obtained value.
 * @param[int] write_size obtained value.
 * @param[out] read_value obtained value.
 * @param[int] read_size obtained value.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size, unsigned char* read_value, const int read_size)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;

	if (write_value == nullptr || read_value == nullptr) {
		return ret_value;
	}

	if (write_size == 0 || read_size == 0) {
		return ret_value;
	}

	switch (option_name) {
	case IscCameraParameter::kGenericRead:
	{
		int ret = getCameraRegData(write_value, read_value, write_size, read_size);
		if (ret == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
	}
	break;
	}

	return ret_value;
}

/**
 * set the parameters.
 *
 * @param[in] option_name target parameter.
 * @param[in] write_value value to set.
 * @param[in] write_size value to set.
 * @return 0 if successful.
 */
int K4aSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;

	if (write_value == nullptr) {
		return ret_value;
	}

	if (write_size == 0) {
		return ret_value;
	}

	switch (option_name) {
	case IscCameraParameter::kGenericWrite:
	{
		int ret = setCameraRegData(write_value, write_size);
		if (ret == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
	}
	break;
	}

	return ret_value;
}

// grab control
/**
 * start image acquisition.
 *
 * @param[in] isc_grab_start_mode sets the operating mode.
 * @return 0 if successful.
 */
int K4aSdkWrapper::Start(const IscGrabStartMode* isc_grab_start_mode)
{

	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	ISCSDKLib::ISC_GrabMode start_mode = ISCSDKLib::ParallaxImage;

	switch (isc_grab_start_mode->isc_grab_mode) {
	case IscGrabMode::kParallax:
		start_mode = ISCSDKLib::ParallaxImage;
		break;

	case IscGrabMode::kCorrect:
		start_mode = ISCSDKLib::CorrectedImage;
		break;

	case IscGrabMode::kBeforeCorrect:
		start_mode = ISCSDKLib::OriginalImage;
		break;

	case IscGrabMode::kBayerS0:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;

	case IscGrabMode::kBayerS1:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;

	default:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;
	}

	switch (isc_grab_start_mode->isc_grab_color_mode) {
	case IscGrabColorMode::kColorOFF:
		break;

	case IscGrabColorMode::kColorON:
		break;

	default:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;
	}

	int camera_ret_value = startGrab((int)start_mode);

	if (camera_ret_value == ISC_OK) {
		isc_grab_start_mode_.isc_grab_mode = isc_grab_start_mode->isc_grab_mode;
		isc_grab_start_mode_.isc_grab_color_mode = isc_grab_start_mode->isc_grab_color_mode;

		isc_grab_start_mode_.isc_get_mode.wait_time = isc_grab_start_mode->isc_get_mode.wait_time;
		isc_grab_start_mode_.isc_get_raw_mode = isc_grab_start_mode->isc_get_raw_mode;
		isc_grab_start_mode_.isc_get_color_mode = isc_grab_start_mode->isc_get_color_mode;

		isc_grab_start_mode_.isc_record_mode = isc_grab_start_mode->isc_record_mode;
		isc_grab_start_mode_.isc_play_mode = isc_grab_start_mode->isc_play_mode;
		isc_grab_start_mode_.isc_play_mode_parameter.interval = isc_grab_start_mode->isc_play_mode_parameter.interval;
		swprintf_s(isc_grab_start_mode_.isc_play_mode_parameter.play_file_name, L"%s", isc_grab_start_mode->isc_play_mode_parameter.play_file_name);
	}
	else {
		return CAMCONTROL_E_GRAB_START_FAILED;
	}

	return DPC_E_OK;
}

/**
 * stop image capture.
 *
 * @return 0 if successful.
 */
int K4aSdkWrapper::Stop()
{

	int ret_value = DPC_E_OK;

	int camera_ret_value = stopGrab();
	if (camera_ret_value == ISC_OK) {
	}
	else {
		return CAMCONTROL_E_GRAB_STOP_FAILED;
	}

	return ret_value;
}

/**
 * Get the current capture mode.
 *
 * @param[in] isc_grab_start_mode is the current mode of operation.
 * @return 0 if successful.
 */
int K4aSdkWrapper::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
{
	isc_grab_start_mode->isc_grab_mode = isc_grab_start_mode_.isc_grab_mode;
	isc_grab_start_mode->isc_grab_color_mode = isc_grab_start_mode_.isc_grab_color_mode;

	isc_grab_start_mode->isc_get_mode.wait_time = isc_grab_start_mode_.isc_get_mode.wait_time;
	isc_grab_start_mode->isc_get_raw_mode = isc_grab_start_mode_.isc_get_raw_mode;
	isc_grab_start_mode->isc_get_color_mode = isc_grab_start_mode_.isc_get_color_mode;

	isc_grab_start_mode->isc_record_mode = isc_grab_start_mode_.isc_record_mode;
	isc_grab_start_mode->isc_play_mode = isc_grab_start_mode_.isc_play_mode;
	isc_grab_start_mode->isc_play_mode_parameter.interval = isc_grab_start_mode_.isc_play_mode_parameter.interval;
	swprintf_s(isc_grab_start_mode->isc_play_mode_parameter.play_file_name, L"%s", isc_grab_start_mode_.isc_play_mode_parameter.play_file_name);

	return DPC_E_OK;
}

// image & data get 
/**
 * initialize IscImageInfo. Allocate the required space.
 *
 * @param[in,out] isc_image_info is a structure to initialize.
 * @return 0 if successful.
 */
int K4aSdkWrapper::InitializeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	int width = k4a_camera_param_info_.image_width;
	int height = k4a_camera_param_info_.image_height;

	isc_image_info->grab = IscGrabMode::kParallax;
	isc_image_info->color_grab_mode = IscGrabColorMode::kColorOFF;
	isc_image_info->shutter_mode = IscShutterMode::kManualShutter;
	isc_image_info->camera_specific_parameter.d_inf = k4a_camera_param_info_.d_inf;
	isc_image_info->camera_specific_parameter.bf = k4a_camera_param_info_.bf;
	isc_image_info->camera_specific_parameter.base_length = k4a_camera_param_info_.base_length;
	isc_image_info->camera_specific_parameter.dz = k4a_camera_param_info_.dz;

	for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
		isc_image_info->frame_data[i].camera_status.error_code = ISC_OK;
		isc_image_info->frame_data[i].camera_status.data_receive_tact_time = 0;

		isc_image_info->frame_data[i].frame_time = 0;

		isc_image_info->frame_data[i].frameNo = -1;
		isc_image_info->frame_data[i].gain = -1;
		isc_image_info->frame_data[i].exposure = -1;

		isc_image_info->frame_data[i].p1.width = 0;
		isc_image_info->frame_data[i].p1.height = 0;
		isc_image_info->frame_data[i].p1.channel_count = 0;
		isc_image_info->frame_data[i].p1.image = new unsigned char[width * height];

		isc_image_info->frame_data[i].p2.width = 0;
		isc_image_info->frame_data[i].p2.height = 0;
		isc_image_info->frame_data[i].p2.channel_count = 0;
		isc_image_info->frame_data[i].p2.image = new unsigned char[width * height];

		isc_image_info->frame_data[i].color.width = 0;
		isc_image_info->frame_data[i].color.height = 0;
		isc_image_info->frame_data[i].color.channel_count = 0;
		isc_image_info->frame_data[i].color.image = new unsigned char[width * height * 4];

		isc_image_info->frame_data[i].depth.width = 0;
		isc_image_info->frame_data[i].depth.height = 0;
		isc_image_info->frame_data[i].depth.image = new float[width * height];

		isc_image_info->frame_data[i].raw.width = 0;
		isc_image_info->frame_data[i].raw.height = 0;
		isc_image_info->frame_data[i].raw.channel_count = 0;
		isc_image_info->frame_data[i].raw.image = new unsigned char[width * height * 4];

		isc_image_info->frame_data[i].raw_color.width = 0;
		isc_image_info->frame_data[i].raw_color.height = 0;
		isc_image_info->frame_data[i].raw_color.channel_count = 0;
		isc_image_info->frame_data[i].raw_color.image = new unsigned char[width * height * 2];

		size_t image_size = width * height;
		memset(isc_image_info->frame_data[i].p1.image, 0, image_size);
		memset(isc_image_info->frame_data[i].p2.image, 0, image_size);

		image_size = width * height * sizeof(float);
		memset(isc_image_info->frame_data[i].depth.image, 0, image_size);

		image_size = width * height * 4;
		memset(isc_image_info->frame_data[i].raw.image, 0, image_size);

		image_size = width * height * 2;
		memset(isc_image_info->frame_data[i].raw_color.image, 0, image_size);
	}

	return DPC_E_OK;
}

/**
 * release the allocated space.
 *
 * @param[in,out] isc_image_info is a structure to release.
 * @return 0 if successful.
 */
int K4aSdkWrapper::ReleaeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	isc_image_info->grab = IscGrabMode::kParallax;
	isc_image_info->color_grab_mode = IscGrabColorMode::kColorOFF;
	isc_image_info->shutter_mode = IscShutterMode::kManualShutter;
	isc_image_info->camera_specific_parameter.d_inf = 0;
	isc_image_info->camera_specific_parameter.bf = 0;
	isc_image_info->camera_specific_parameter.base_length = 0;
	isc_image_info->camera_specific_parameter.dz = 0;

	for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
		isc_image_info->frame_data[i].camera_status.error_code = ISC_OK;
		isc_image_info->frame_data[i].camera_status.data_receive_tact_time = 0;

		isc_image_info->frame_data[i].frame_time = 0;

		isc_image_info->frame_data[i].frameNo = -1;
		isc_image_info->frame_data[i].gain = -1;
		isc_image_info->frame_data[i].exposure = -1;

		isc_image_info->frame_data[i].p1.width = 0;
		isc_image_info->frame_data[i].p1.height = 0;
		isc_image_info->frame_data[i].p1.channel_count = 0;
		delete[] isc_image_info->frame_data[i].p1.image;
		isc_image_info->frame_data[i].p1.image = nullptr;

		isc_image_info->frame_data[i].p2.width = 0;
		isc_image_info->frame_data[i].p2.height = 0;
		isc_image_info->frame_data[i].p2.channel_count = 0;
		delete[] isc_image_info->frame_data[i].p2.image;
		isc_image_info->frame_data[i].p2.image = nullptr;

		isc_image_info->frame_data[i].color.width = 0;
		isc_image_info->frame_data[i].color.height = 0;
		isc_image_info->frame_data[i].color.channel_count = 0;
		delete[] isc_image_info->frame_data[i].color.image;
		isc_image_info->frame_data[i].color.image = nullptr;

		isc_image_info->frame_data[i].depth.width = 0;
		isc_image_info->frame_data[i].depth.height = 0;
		delete[] isc_image_info->frame_data[i].depth.image;
		isc_image_info->frame_data[i].depth.image = nullptr;

		isc_image_info->frame_data[i].raw.width = 0;
		isc_image_info->frame_data[i].raw.height = 0;
		isc_image_info->frame_data[i].raw.channel_count = 0;
		delete[] isc_image_info->frame_data[i].raw.image;
		isc_image_info->frame_data[i].raw.image = nullptr;

		isc_image_info->frame_data[i].raw_color.width = 0;
		isc_image_info->frame_data[i].raw_color.height = 0;
		isc_image_info->frame_data[i].raw_color.channel_count = 0;
		delete[] isc_image_info->frame_data[i].raw_color.image;
		isc_image_info->frame_data[i].raw_color.image = nullptr;
	}

	return DPC_E_OK;
}

/**
 * get captured data.
 *
 * @param[in] isc_get_mode a parameter that specifies the acquisition behavior.
 * @param[in,out] isc_image_info it is a structure to write the acquired data.
 * @return 0 if successful.
 */
int K4aSdkWrapper::GetData(const IscGetMode* isc_get_mode, IscImageInfo* isc_image_info)
{
	int ret = GetDataModeNormal(isc_get_mode, isc_image_info);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * get captured data in normal mode.
 *
 * @param[in] isc_get_mode a parameter that specifies the acquisition behavior.
 * @param[in,out] isc_image_info it is a structure to write the acquired data.
 * @return 0 if successful.
 */
int K4aSdkWrapper::GetDataModeNormal(const IscGetMode* isc_get_mode, IscImageInfo* isc_image_info)
{

	isc_image_info->grab = isc_grab_start_mode_.isc_grab_mode;
	isc_image_info->color_grab_mode = isc_grab_start_mode_.isc_grab_color_mode;
	isc_image_info->shutter_mode = isc_shutter_mode_;
	isc_image_info->camera_specific_parameter.d_inf = k4a_camera_param_info_.d_inf;
	isc_image_info->camera_specific_parameter.bf = k4a_camera_param_info_.bf;
	isc_image_info->camera_specific_parameter.base_length = k4a_camera_param_info_.base_length;
	isc_image_info->camera_specific_parameter.dz = k4a_camera_param_info_.dz;

	for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
		isc_image_info->frame_data[i].frameNo = -1;
		isc_image_info->frame_data[i].gain = -1;
		isc_image_info->frame_data[i].exposure = -1;

		isc_image_info->frame_data[i].camera_status.error_code = ISC_OK;
		isc_image_info->frame_data[i].camera_status.data_receive_tact_time = 0;

		isc_image_info->frame_data[i].p1.width = 0;
		isc_image_info->frame_data[i].p1.height = 0;
		isc_image_info->frame_data[i].p1.channel_count = 0;

		isc_image_info->frame_data[i].p2.width = 0;
		isc_image_info->frame_data[i].p2.height = 0;
		isc_image_info->frame_data[i].p2.channel_count = 0;

		isc_image_info->frame_data[i].color.width = 0;
		isc_image_info->frame_data[i].color.height = 0;
		isc_image_info->frame_data[i].color.channel_count = 0;

		isc_image_info->frame_data[i].depth.width = 0;
		isc_image_info->frame_data[i].depth.height = 0;

		isc_image_info->frame_data[i].raw.width = 0;
		isc_image_info->frame_data[i].raw.height = 0;
		isc_image_info->frame_data[i].raw.channel_count = 0;

		isc_image_info->frame_data[i].raw_color.width = 0;
		isc_image_info->frame_data[i].raw_color.height = 0;
		isc_image_info->frame_data[i].raw_color.channel_count = 0;
	}

	UtilityMeasureTime utility_measure_time_total;
	utility_measure_time_total.Init();

	UtilityMeasureTime utility_measure_time;
	utility_measure_time.Init();
	double elp_time[8] = {};
	char msg[128] = {};

	utility_measure_time_total.Start();

	constexpr int frame_data_id = kISCIMAGEINFO_FRAMEDATA_LATEST;

	ISCSDKLib::ISC_ImageInfo isc_img_info_4k = {};

	isc_img_info_4k.frameNo = 0;
	isc_img_info_4k.gain = 0;
	isc_img_info_4k.exposure = 0;
	isc_img_info_4k.grab = ISCSDKLib::ParallaxImage;
	isc_img_info_4k.shutter = ISCSDKLib::Manual;
	isc_img_info_4k.p1_width = 0;
	isc_img_info_4k.p1_height = 0;
	isc_img_info_4k.p1 = isc_image_info_.frame_data[frame_data_id].p1.image;
	isc_img_info_4k.p2_width = 0;
	isc_img_info_4k.p2_height = 0;
	isc_img_info_4k.p2 = isc_image_info_.frame_data[frame_data_id].p2.image;

	int ret = getImageEx(&isc_img_info_4k, isc_get_mode->wait_time);
	isc_image_info->frame_data[frame_data_id].camera_status.error_code = ret;

	if (ret != ISC_OK) {
		if (ret != 0) {
			if (ret == ERR_USB_NO_IMAGE) {
				/*ERR_USB_NO_IMAGE*/
				return CAMCONTROL_E_NO_IMAGE;
			}
			else if (ret == 4 /*FT_IO_ERROR*/) {
				/*FT_IO_ERROR*/
				return CAMCONTROL_E_FTDI_ERROR;
			}
			else if (ret == ERR_NO_VALID_IMAGES_CALIBRATING) {
				/*ERR_NO_VALID_IMAGES_CALIBRATING*/
				return CAMCONTROL_E_CAMERA_UNDER_CARIBRATION;
			}
			else {
				// 画像を確認するため継続
			}
		}
	}

	// RAW data
	if (isc_grab_start_mode_.isc_get_raw_mode == IscGetModeRaw::kRawOn) {

		utility_measure_time.Start();

		int full_frame_width = 0, full_frame_height = 0;
		ret = getFullFrameInfo(isc_image_info->frame_data[frame_data_id].raw.image, &full_frame_width, &full_frame_height);
		if (ret != 0) {
			return CAMCONTROL_E_GET_FULL_FRAME_FAILED;
		}
		isc_image_info->frame_data[frame_data_id].raw.width = full_frame_width;
		isc_image_info->frame_data[frame_data_id].raw.height = full_frame_height;
		isc_image_info->frame_data[frame_data_id].raw.channel_count = 1;

		// 反転不要です

		elp_time[0] = utility_measure_time.Stop();
	}

	// 反転不要です
	constexpr bool is_flip = false;

	if (is_flip) {

		utility_measure_time.Start();
		// 基準側画像
		isc_image_info->frame_data[frame_data_id].p1.width = isc_img_info_4k.p1_width;
		isc_image_info->frame_data[frame_data_id].p1.height = isc_img_info_4k.p1_height;
		isc_image_info->frame_data[frame_data_id].p1.channel_count = 1;

		// 他のカメラとの互換性のために左右を反転します
		cv::Mat mat_src_image_p1(isc_image_info->frame_data[frame_data_id].p1.height, isc_image_info->frame_data[frame_data_id].p1.width, CV_8UC1, isc_image_info_.frame_data[frame_data_id].p1.image);
		cv::Mat mat_dst_image_p1(isc_image_info->frame_data[frame_data_id].p1.height, isc_image_info->frame_data[frame_data_id].p1.width, CV_8UC1, isc_image_info->frame_data[frame_data_id].p1.image);
		cv::flip(mat_src_image_p1, mat_dst_image_p1, 1);

		elp_time[1] = utility_measure_time.Stop();

		if (isc_grab_start_mode_.isc_grab_mode == IscGrabMode::kParallax) {
			utility_measure_time.Start();

			ret = getDepthInfo(isc_image_info_.frame_data[frame_data_id].depth.image);
			if (ret != ISC_OK) {
				return CAMCONTROL_E_GET_DEPTH_FAILED;
			}
			isc_image_info->frame_data[frame_data_id].depth.width = k4a_camera_param_info_.image_width;
			isc_image_info->frame_data[frame_data_id].depth.height = k4a_camera_param_info_.image_height;

			// 他のカメラとの互換性のために左右を反転します
			cv::Mat mat_src_image_depth(isc_image_info->frame_data[frame_data_id].depth.height, isc_image_info->frame_data[frame_data_id].depth.width, CV_32FC1, isc_image_info_.frame_data[frame_data_id].depth.image);
			cv::Mat mat_dst_image_depth(isc_image_info->frame_data[frame_data_id].depth.height, isc_image_info->frame_data[frame_data_id].depth.width, CV_32FC1, isc_image_info->frame_data[frame_data_id].depth.image);
			cv::flip(mat_src_image_depth, mat_dst_image_depth, 1);

			elp_time[2] = utility_measure_time.Stop();
		}
		else {
			// 補正後比較画像/補正前比較画像
			isc_image_info->frame_data[frame_data_id].p2.width = isc_img_info_4k.p2_width;
			isc_image_info->frame_data[frame_data_id].p2.height = isc_img_info_4k.p2_height;
			isc_image_info->frame_data[frame_data_id].p2.channel_count = 1;

			// 他のカメラとの互換性のために左右を反転します
			cv::Mat mat_src_image_p2(isc_image_info->frame_data[frame_data_id].p2.height, isc_image_info->frame_data[frame_data_id].p2.width, CV_8UC1, isc_image_info_.frame_data[frame_data_id].p2.image);
			cv::Mat mat_dst_image_p2(isc_image_info->frame_data[frame_data_id].p2.height, isc_image_info->frame_data[frame_data_id].p2.width, CV_8UC1, isc_image_info->frame_data[frame_data_id].p2.image);
			cv::flip(mat_src_image_p2, mat_dst_image_p2, 1);
		}
	}
	else {
		// 基準側画像
		isc_image_info->frame_data[frame_data_id].p1.width = isc_img_info_4k.p1_width;
		isc_image_info->frame_data[frame_data_id].p1.height = isc_img_info_4k.p1_height;
		isc_image_info->frame_data[frame_data_id].p1.channel_count = 1;

		size_t cp_size = isc_image_info->frame_data[frame_data_id].p1.width * isc_image_info->frame_data[frame_data_id].p1.height;
		memcpy(isc_image_info->frame_data[frame_data_id].p1.image, isc_image_info_.frame_data[frame_data_id].p1.image, cp_size);

		if (isc_grab_start_mode_.isc_grab_mode == IscGrabMode::kParallax) {
			ret = getDepthInfo(isc_image_info_.frame_data[frame_data_id].depth.image);
			if (ret != ISC_OK) {
				OutputDebugStringA("[GetData]GetDepthInfo Failed\n");
				return CAMCONTROL_E_GET_DEPTH_FAILED;
			}
			isc_image_info->frame_data[frame_data_id].depth.width = k4a_camera_param_info_.image_width;
			isc_image_info->frame_data[frame_data_id].depth.height = k4a_camera_param_info_.image_height;

			cp_size = isc_image_info->frame_data[frame_data_id].depth.width * isc_image_info->frame_data[frame_data_id].depth.height * sizeof(float);
			memcpy(isc_image_info->frame_data[frame_data_id].depth.image, isc_image_info_.frame_data[frame_data_id].depth.image, cp_size);
		}
		else {
			// 補正後比較画像/補正前比較画像
			isc_image_info->frame_data[frame_data_id].p2.width = isc_img_info_4k.p2_width;
			isc_image_info->frame_data[frame_data_id].p2.height = isc_img_info_4k.p2_height;
			isc_image_info->frame_data[frame_data_id].p2.channel_count = 1;

			cp_size = isc_image_info->frame_data[frame_data_id].p2.width * isc_image_info->frame_data[frame_data_id].p2.height;
			memcpy(isc_image_info->frame_data[frame_data_id].p2.image, isc_image_info_.frame_data[frame_data_id].p2.image, cp_size);
		}
	}

	// Color
	if (isc_grab_start_mode_.isc_grab_color_mode == IscGrabColorMode::kColorON) {
		utility_measure_time.Start();
		//enum class IscGetModeColor {
		//	kYuv,               /**< output yuv */
		//	kBayer,             /**< bayer output */
		//	kBGR,               /**< yuv(bayer) -> bgr */
		//	kCorrect,           /**< yuv(bayer) -> bgr -> correct */
		//	kAwb,               /**< yuv(bayer) -> bgr -> correct -> auto white balance */
		//	kAwbNoCorrect       /**< yuv(bayer) -> bgr -> auto white balance */
		//};

		/*
			4KAでは、Bayerデータの取得モードのみハードウェアで実装されている
			Bayerデータは、専用の取得モードでサポートされるが、用途がない
			このアプリケーションでは、Colorをサポートしない
		*/
		
		elp_time[3] = utility_measure_time.Stop();

	} // if (isc_grab_start_mode_.isc_grab_color_mode == IscGrabColorMode::kColorON) {

	double total_time = utility_measure_time_total.Stop();

	//sprintf_s(msg, "[GetImage]Total=%.1f Raw=%.1f Base=%.1f Depth=%.1f Color=%.1f\n", total_time, elp_time[0], elp_time[1], elp_time[2], elp_time[3]);
	//OutputDebugStringA(msg);

	return DPC_E_OK;
}

/**
 * Unpack parallax data.
 *
 * @param[in] IscGrabMode mode for grabing.
 * @param[in] isc_grab_color_mode color mode on/off.
 * @param[in] width image width.
 * @param[in] height image height.
 * @param[in,out] isc_image_info buffer for image.
 * @return 0 if successful.
 */
int K4aSdkWrapper::Decode(const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode, const IscGetModeColor isc_get_color_mode,
	const int width, const int height, IscImageInfo* isc_image_info, int frame_data_index)
{

	constexpr bool is_flip_for_compatibility = true;
	int ret = DPC_E_OK;

	const int frame_data_id = frame_data_index;
	if (isc_grab_color_mode == IscGrabColorMode::kColorON) {

		// color

		//enum class IscGetModeColor {
		//	kYuv,               /**< output yuv */
		//	kBayer,             /**< bayer output */
		//	kBGR,               /**< yuv(bayer) -> bgr */
		//	kCorrect,           /**< yuv(bayer) -> bgr -> correct */
		//	kAwb,               /**< yuv(bayer) -> bgr -> correct -> auto white balance */
		//	kAwbNoCorrect       /**< yuv(bayer) -> bgr -> auto white balance */
		//};

	}
	else {
		// 画像分割処理を行います
		const bool is_disparity = (isc_grab_mode == IscGrabMode::kParallax) ? true : false;
		int ret = SplitImage(is_disparity, width, height, isc_image_info->frame_data[frame_data_id].raw.image, decode_buffer_.split_images[0], decode_buffer_.split_images[1], decode_buffer_.split_images[2]);
		if (ret != DPC_E_OK) {
			return ret;
		}

		switch (isc_grab_mode) {
		case IscGrabMode::kParallax:
			// mask情報を使った視差情報にします
			ReCreateParallaxImage(width, height, decode_buffer_.split_images[2], decode_buffer_.disparity, decode_buffer_.disparity_image, decode_buffer_.mask_image);
			//memcpy(isc_image_info->depth.image, decode_buffer_.disparity, width * height * sizeof(float));
			// 他のカメラとの互換性のために左右を反転します
			isc_image_info->frame_data[frame_data_id].depth.width = width;
			isc_image_info->frame_data[frame_data_id].depth.height = height;
			{
				cv::Mat mat_src_image_depth(isc_image_info->frame_data[frame_data_id].depth.height, isc_image_info->frame_data[frame_data_id].depth.width, CV_32FC1, decode_buffer_.disparity);
				cv::Mat mat_dst_image_depth(isc_image_info->frame_data[frame_data_id].depth.height, isc_image_info->frame_data[frame_data_id].depth.width, CV_32FC1, isc_image_info->frame_data[frame_data_id].depth.image);
				cv::flip(mat_src_image_depth, mat_dst_image_depth, 1);
			}

			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[1], isc_image_info->p1.image);
			memcpy(isc_image_info->frame_data[frame_data_id].p1.image, decode_buffer_.split_images[1], width * height);
			isc_image_info->frame_data[frame_data_id].p1.width = width;
			isc_image_info->frame_data[frame_data_id].p1.height = height;
			isc_image_info->frame_data[frame_data_id].p1.channel_count = 1;
			break;

		case IscGrabMode::kCorrect:
			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[1], isc_image_info->p1.image);
			memcpy(isc_image_info->frame_data[frame_data_id].p1.image, decode_buffer_.split_images[1], width * height);
			isc_image_info->frame_data[frame_data_id].p1.width = width;
			isc_image_info->frame_data[frame_data_id].p1.height = height;
			isc_image_info->frame_data[frame_data_id].p1.channel_count = 1;

			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[0], isc_image_info->p2.image);
			memcpy(isc_image_info->frame_data[frame_data_id].p2.image, decode_buffer_.split_images[0], width * height);
			isc_image_info->frame_data[frame_data_id].p2.width = width;
			isc_image_info->frame_data[frame_data_id].p2.height = height;
			isc_image_info->frame_data[frame_data_id].p2.channel_count = 1;

			break;

		case IscGrabMode::kBeforeCorrect:
			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[1], isc_image_info->p1.image);
			memcpy(isc_image_info->frame_data[frame_data_id].p1.image, decode_buffer_.split_images[1], width * height);
			isc_image_info->frame_data[frame_data_id].p1.width = width;
			isc_image_info->frame_data[frame_data_id].p1.height = height;
			isc_image_info->frame_data[frame_data_id].p1.channel_count = 1;

			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[0], isc_image_info->p2.image);
			memcpy(isc_image_info->frame_data[frame_data_id].p2.image, decode_buffer_.split_images[0], width * height);
			isc_image_info->frame_data[frame_data_id].p2.width = width;
			isc_image_info->frame_data[frame_data_id].p2.height = height;
			isc_image_info->frame_data[frame_data_id].p2.channel_count = 1;

			break;
		}
	}

	return DPC_E_OK;
}

/**
 * Split RAW data.
 *
 * @param[in] is_disparity whether in disparity mode.
 * @param[in] width image width.
 * @param[in] height image height.
 * @param[in] raw_data Raw data from camera.
 * @param[in,out] image1 monochrome sensor-0 image / disparity.
 * @param[in,out] image2 monochrome sensor-1 image / mask data.
 * @param[in,out] image3 disparity.
 * @return 0 if successful.
 */
int K4aSdkWrapper::SplitImage(const bool is_disparity, const int width, const int height, unsigned char* raw_data, unsigned char* image1, unsigned char* image2, unsigned char* image3)
{
	unsigned char* buff_mix = raw_data;
	unsigned char* buff_img1 = image1;
	unsigned char* buff_img2 = image2;
	unsigned char* buff_img3 = image3;

	// Image Data
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			// 視差モード?
			if (is_disparity) {
				// 視差情報
				*buff_img1 = (unsigned char)(*buff_mix);

				// 生の視差情報を保存します
				*buff_img3 = *buff_mix;

			}
			else {
				// 視差モードでなければ、普通の画像
				*(buff_img1) = *buff_mix;
			}

			// mask1(画像)
			*(buff_img2) = *(buff_mix + 1);

			buff_mix += 2;
			buff_img1++;
			buff_img2++;

			// 視差情報エリア
			buff_img3++;
		}
	}

	return DPC_E_OK;
}

/**
 * Unpack disparity data.
 *
 * @param[in] width image width.
 * @param[in] height image height.
 * @param[in] src_data Parallax data before unfolding.
 * @param[in,out] temp_disparity Parallax data after unfolding.
 * @param[in,out] dst_image Parallax image.
 * @param[in,out] mask_image mask image for reference.
 * @return 0 if successful.
 */
int K4aSdkWrapper::ReCreateParallaxImage(const int width, const int height, unsigned char* src_data, float* temp_disparity, unsigned char* dst_image, unsigned char* mask_image)
{
	unsigned char* buff_mix = src_data;
	unsigned char* buff_img1 = dst_image;
	unsigned char* buff_mask = mask_image;

	unsigned char store_disparity = 0;
	unsigned char mask1 = 0, mask2 = 0, mask3 = 0, mask4 = 0;
	unsigned char shift = 0;
	float temp_value;
	int d_tmp;
	float* disparity = temp_disparity;

	constexpr unsigned char max_disparity_value = 95;
	constexpr float disparity_step = 0.0625F;

	// Image Data
	for (int j = 0; j < height; j += 4) {
		for (int i = 0; i < width; i += 4) {
			// 視差整数部
			if (i % 4 == 0) {
				store_disparity = *(buff_mix + j * width + i);

				if (store_disparity > max_disparity_value) {
					store_disparity = 0;
					temp_value = 0.00;

					mask1 = 0;
					mask2 = 0;
				}
				else {
					//　視差小数部
					temp_value = store_disparity;
					d_tmp = ((*(buff_mix + j * width + i + 1)) & 0xF0) >> 4;
					temp_value += (float)(d_tmp * disparity_step);

					mask1 = *(buff_mix + j * width + i + 2);
					mask2 = *(buff_mix + j * width + i + 3);
				}
			}

			shift = 0x01;

			// 1段目
			for (int q = 0; q < 4; q++) {
				if (mask2 & shift) {
					*(buff_img1 + (width - (i + q) - 1)) = store_disparity;
					*(disparity + (width - (i + q) - 1)) = temp_value;
					*(buff_mask + (width - (i + q) - 1)) = 255;
				}
				else {
					*(buff_img1 + width - (i + q) - 1) = 0x00;
					*(disparity + width - (i + q) - 1) = 0x00;
					*(buff_mask + (width - (i + q) - 1)) = 0;
				}

				shift = shift << 0x01;
			}

			// 2段目
			for (int q = 0; q < 4; q++) {
				if (mask2 & shift) {
					*(buff_img1 + (width + width - (i + q) - 1)) = store_disparity;
					*(disparity + (width + width - (i + q) - 1)) = temp_value;
					*(buff_mask + (width + width - (i + q) - 1)) = 255;
				}
				else {
					*(buff_img1 + (width + width - (i + q) - 1)) = 0x00;
					*(disparity + (width + width - (i + q) - 1)) = 0x00;
					*(buff_mask + (width + width - (i + q) - 1)) = 0;
				}

				shift = shift << 0x01;
			}

			shift = 0x01;
			// 3段目
			for (int q = 0; q < 4; q++) {
				if (mask1 & shift) {
					*(buff_img1 + (width * 2 + width - (i + q) - 1)) = store_disparity;
					*(disparity + (width * 2 + width - (i + q) - 1)) = temp_value;
					*(buff_mask + (width * 2 + width - (i + q) - 1)) = 255;
				}
				else {
					*(buff_img1 + (width * 2 + width - (i + q) - 1)) = 0x00;
					*(disparity + (width * 2 + width - (i + q) - 1)) = 0x00;
					*(buff_mask + (width * 2 + width - (i + q) - 1)) = 0;
				}
				shift = shift << 0x01;
			}


			// 4段目
			for (int q = 0; q < 4; q++) {
				if (mask1 & shift) {
					*(buff_img1 + (width * 3 + width - (i + q) - 1)) = store_disparity;
					*(disparity + (width * 3 + width - (i + q) - 1)) = temp_value;
					*(buff_mask + (width * 3 + width - (i + q) - 1)) = 255;
				}
				else {
					*(buff_img1 + (width * 3 + width - (i + q) - 1)) = 0x00;
					*(disparity + (width * 3 + width - (i + q) - 1)) = 0x00;
					*(buff_mask + (width * 3 + width - (i + q) - 1)) = 0;
				}
				shift = shift << 0x01;
			}
		}

		// 書き込み位置を移動します
		buff_img1 += width * 4;
		disparity += width * 4;
		buff_mask += width * 4;

	}

	return DPC_E_OK;
}

/**
 * Flip the image left and right.
 *
 * @param[in] width image width.
 * @param[in] height image height.
 * @param[in] src_image input image.
 * @param[out] dst_image output image.
 * @return 0 if successful.
 */
int K4aSdkWrapper::FlipImage(const int width, const int height, unsigned char* src_image, unsigned char* dst_image)
{

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			dst_image[j * width + i] = src_image[j * width + width - i - 1];
		}
	}

	return DPC_E_OK;
}

/**
 * Set peculiar removal regiser.
 *
 * @param[in] value input value 0~7.
 * @return 0 if successful.
 */
int K4aSdkWrapper::SetStereoMatchingsPeculiarRemoval(const int value)
{
	constexpr int USB_WRITE_CMD_SIZE = 8;
	unsigned char wbuf[USB_WRITE_CMD_SIZE] = { };

	wbuf[0] = 0xF0;
	wbuf[1] = 0x80;
	wbuf[2] = 0x62;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	if (value == 0) {
		wbuf[4] = 0x00;
	}
	else {
		wbuf[4] = 0x01;
	}

	// コマンドを送ります
	int ret = setCameraRegData(wbuf, USB_WRITE_CMD_SIZE);

	return ret;
}

/**
 * Get peculiar removal regiser.
 *
 * @param[out] Value read from camera .
 * @return 0 if successful.
 */
int K4aSdkWrapper::GetStereoMatchingsPeculiarRemoval(int* value)
{
	constexpr int usb_write_cmd_size = 8;
	unsigned char wbuf[usb_write_cmd_size] = { };
	constexpr int usb_read_data_size = 8;
	unsigned char rbuf[usb_read_data_size] = { };

	wbuf[0] = 0xF1;
	wbuf[1] = 0x80;
	wbuf[2] = 0x62;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	int ret = getCameraRegData(wbuf, rbuf, usb_write_cmd_size, usb_read_data_size);

	*value = (int)rbuf[7];

	return ret;
}

/**
 * Set occlusion removal on/off.
 *
 * @param[in] value 0:off 1:on.
 * @return 0 if successful.
 */
int K4aSdkWrapper::SetStereoMatchingsOcclusionRemoval(const unsigned int value)
{

	constexpr int USB_WRITE_CMD_SIZE = 8;
	unsigned char wbuf[USB_WRITE_CMD_SIZE] = { };

	wbuf[0] = 0xF0;
	wbuf[1] = 0x80;
	wbuf[2] = 0x61;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	if (value == 0) {
		wbuf[4] = 0x00;
	}
	else {
		wbuf[4] = value;
	}

	// コマンドを送ります
	int ret = setCameraRegData(wbuf, USB_WRITE_CMD_SIZE);

	return ret;
}

/**
 * Get occlusion removal regiser.
 *
 * @param[out] Value read from camera 0:off 1:on.
 * @return 0 if successful.
 */
int K4aSdkWrapper::GetStereoMatchingsOcclusionRemoval(unsigned int* value)
{

	constexpr int usb_write_cmd_size = 8;
	unsigned char wbuf[usb_write_cmd_size] = { };
	constexpr int usb_read_data_size = 8;
	unsigned char rbuf[usb_read_data_size] = { };

	wbuf[0] = 0xF1;
	wbuf[1] = 0x80;
	wbuf[2] = 0x61;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	int ret = getCameraRegData(wbuf, rbuf, usb_write_cmd_size, usb_read_data_size);

	*value = rbuf[7];

	return ret;
}

/**
 * load function address from DLL.
 *
 * @return 0 if successful.
 */
int K4aSdkWrapper::LoadDLLFunction(char* module_path)
{
	sprintf_s(file_name_of_dll_, "%s\\%s", module_path, kISC_4KA_DRV_FILE_NAME);

	// Load DLL
	dll_handle_ = LoadLibraryA(file_name_of_dll_);

	if (dll_handle_ == NULL) {
		char dbg_msg[512] = {};
		sprintf_s(dbg_msg, "Failed to load DLL(%s)", file_name_of_dll_);
		MessageBoxA(NULL, dbg_msg, "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}

	// Load Function

	//int OpenISC();
	FARPROC proc = GetProcAddress(dll_handle_, "OpenISC");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	openISC = reinterpret_cast<TOpenISC>(proc);

	//int CloseISC();
	proc = GetProcAddress(dll_handle_, "CloseISC");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	closeISC = reinterpret_cast<TCloseISC>(proc);

	//int StartGrab(int nMode);
	proc = GetProcAddress(dll_handle_, "StartGrab");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	startGrab = reinterpret_cast<TStartGrab>(proc);

	//int StopGrab();
	proc = GetProcAddress(dll_handle_, "StopGrab");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	stopGrab = reinterpret_cast<TStopGrab>(proc);

	// int GetImageEx(struct ISC_ImageInfo* image,int waitTime);
	proc = GetProcAddress(dll_handle_, "GetImageEx");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getImageEx = reinterpret_cast<TGetImageEx>(proc);

	// int GetRawImageEx(struct ISC_RawImageInfo* image, int waitTime);
	proc = GetProcAddress(dll_handle_, "GetRawImageEx");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getRawImageEx = reinterpret_cast<TGetRawImageEx>(proc);

	// int GetDepthInfo(float* depth);
	proc = GetProcAddress(dll_handle_, "GetDepthInfo");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getDepthInfo = reinterpret_cast<TGetDepthInfo>(proc);

	//int GetCameraParamInfo(struct CameraParamInfo* info);
	proc = GetProcAddress(dll_handle_, "GetCameraParamInfo");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getCameraParamInfo = reinterpret_cast<TGetCameraParamInfo>(proc);

	//int GetImageSize(unsigned int* pnWidth, unsigned int* pnHeight);
	proc = GetProcAddress(dll_handle_, "GetImageSize");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getImageSize = reinterpret_cast<TGetImageSize>(proc);

	// int GetCorrectedImageSize(int* width, int* height);
	proc = GetProcAddress(dll_handle_, "GetCorrectedImageSize");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getCorrectedImageSize = reinterpret_cast<TGetCorrectedImageSize>(proc);

	//int SetAutoCalibration(int nMode);
	proc = GetProcAddress(dll_handle_, "SetAutoCalibration");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setAutoCalibration = reinterpret_cast<TSetAutoCalibration>(proc);

	//int GetAutoCalibration(int* nMode);
	proc = GetProcAddress(dll_handle_, "GetAutoCalibration");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getAutoCalibration = reinterpret_cast<TGetAutoCalibration>(proc);

	//int SetShutterControlModeEx(enum ISC_ShutterMode mode);
	proc = GetProcAddress(dll_handle_, "SetShutterControlModeEx");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setShutterControlModeEx = reinterpret_cast<TSetShutterControlModeEx>(proc);

	//int GetShutterControlModeEx(enum ISC_ShutterMode *mode);
	proc = GetProcAddress(dll_handle_, "GetShutterControlModeEx");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getShutterControlModeEx = reinterpret_cast<TGetShutterControlModeEx>(proc);

	//int SetExposureValue(int value);
	proc = GetProcAddress(dll_handle_, "SetExposureValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setExposureValue = reinterpret_cast<TSetExposureValue>(proc);

	//int GetExposureValue(int* value);
	proc = GetProcAddress(dll_handle_, "GetExposureValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getExposureValue = reinterpret_cast<TGetExposureValue>(proc);

	//int SetExposureFineValue(int value);
	proc = GetProcAddress(dll_handle_, "SetExposureFineValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setExposureFineValue = reinterpret_cast<TSetExposureFineValue>(proc);

	//int GetExposureFineValue(int* value);
	proc = GetProcAddress(dll_handle_, "GetExposureFineValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getExposureFineValue = reinterpret_cast<TGetExposureFineValue>(proc);

	//int SetMedianTargetValue(int value);
	proc = GetProcAddress(dll_handle_, "SetMedianTargetValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setMedianTargetValue = reinterpret_cast<TSetMedianTargetValue>(proc);

	//int GetMedianTargetValue(int* value);
	proc = GetProcAddress(dll_handle_, "GetMedianTargetValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getMedianTargetValue = reinterpret_cast<TGetMedianTargetValue>(proc);

	//int SetGainValue(unsigned int nValue);
	proc = GetProcAddress(dll_handle_, "SetGainValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setGainValue = reinterpret_cast<TSetGainValue>(proc);

	//int GetGainValue(unsigned int* pnValue);
	proc = GetProcAddress(dll_handle_, "GetGainValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getGainValue = reinterpret_cast<TGetGainValue>(proc);

	//int SetNoiseFilter(unsigned int nValue);
	proc = GetProcAddress(dll_handle_, "SetNoiseFilter");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setNoiseFilter = reinterpret_cast<TSetNoiseFilter>(proc);

	//int GetNoiseFilter(unsigned int* pnValue);
	proc = GetProcAddress(dll_handle_, "GetNoiseFilter");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getNoiseFilter = reinterpret_cast<TGetNoiseFilter>(proc);

	//int SetMeasAreaEx(int mode, int nTop, int nLeft, int nRight, int nBottom, int nTop_Left, int nTop_Right, int nBottom_Left, int nBottom_Right);
	proc = GetProcAddress(dll_handle_, "SetMeasAreaEx");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setMeasAreaEx = reinterpret_cast<TSetMeasAreaEx>(proc);

	//int GetMeasAreaEx(int* mode, int* nTop, int* nLeft, int* nRight, int* nBottom, int* nTop_Left, int* nTop_Right, int* nBottom_Left, int* nBottom_Right);
	proc = GetProcAddress(dll_handle_, "GetMeasAreaEx");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getMeasAreaEx = reinterpret_cast<TGetMeasAreaEx>(proc);

	//int GetImageFromFile(struct ISC_ImageInfo* image, char* filename);
	proc = GetProcAddress(dll_handle_, "GetImageFromFile");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getImageFromFile = reinterpret_cast<TGetImageFromFile>(proc);

	//int GetRawImageFromFile(struct ISC_RawImageInfo* image, char* filename);
	proc = GetProcAddress(dll_handle_, "GetRawImageFromFile");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getRawImageFromFile = reinterpret_cast<TGetRawImageFromFile>(proc);

	//int GetFullFrameInfo(unsigned char* pBuffer, int *width, int *height);
	proc = GetProcAddress(dll_handle_, "GetFullFrameInfo");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getFullFrameInfo = reinterpret_cast<TGetFullFrameInfo>(proc);

	//int GetFullFrameInfo2(unsigned char* pBuffer, int *width, int *height, int waitTime);
	proc = GetProcAddress(dll_handle_, "GetFullFrameInfo2");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getFullFrameInfo2 = reinterpret_cast<TGetFullFrameInfo2>(proc);

	//int SetCameraRegData(unsigned char* pwBuf, int wSize);
	proc = GetProcAddress(dll_handle_, "SetCameraRegData");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setCameraRegData = reinterpret_cast<TSetCameraRegData>(proc);

	//int GetCameraRegData(unsigned char* pwBuf, unsigned char* prBuf, int wSize, int rSize);
	proc = GetProcAddress(dll_handle_, "GetCameraRegData");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getCameraRegData = reinterpret_cast<TGetCameraRegData>(proc);

	//int SetRectTable(char *rect_r_file, char *rect_l_file);
	proc = GetProcAddress(dll_handle_, "SetRectTable");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setRectTable = reinterpret_cast<TSetRectTable>(proc);

	//void FlushLog();
	proc = GetProcAddress(dll_handle_, "FlushLog");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setRectTable = reinterpret_cast<TSetRectTable>(proc);

	typedef int (WINAPI* TFlushLog)();
	TFlushLog flushLog = NULL;

	return DPC_E_OK;
}

/**
 * unload function address from DLL.
 *
 * @return 0 if successful.
 */
int K4aSdkWrapper::UnLoadDLLFunction()
{
	// unload dll
	if (dll_handle_ != NULL) {
		FreeLibrary(dll_handle_);
		dll_handle_ = NULL;
	}

	openISC = nullptr;
	closeISC = nullptr;
	startGrab = nullptr;
	stopGrab = nullptr;
	getImageEx = nullptr;
	getRawImageEx = nullptr;
	getDepthInfo = nullptr;
	getCameraParamInfo = nullptr;
	getImageSize = nullptr;
	getCorrectedImageSize = nullptr;
	setAutoCalibration = nullptr;
	getAutoCalibration = nullptr;
	setShutterControlModeEx = nullptr;
	getShutterControlModeEx = nullptr;
	setExposureValue = nullptr;
	getExposureValue = nullptr;
	setExposureFineValue = nullptr;
	getExposureFineValue = nullptr;
	setMedianTargetValue = nullptr;
	getMedianTargetValue = nullptr;
	setGainValue = nullptr;
	getGainValue = nullptr;
	setNoiseFilter = nullptr;
	getNoiseFilter = nullptr;
	setMeasAreaEx = nullptr;
	getMeasAreaEx = nullptr;
	getImageFromFile = nullptr;
	getRawImageFromFile = nullptr;
	getFullFrameInfo = nullptr;
	getFullFrameInfo2 = nullptr;
	setCameraRegData = nullptr;
	getCameraRegData = nullptr;
	setRectTable = nullptr;
	setRectTable = nullptr;

	return DPC_E_OK;
}

} /* namespace ns_vmsdk_wrapper */
