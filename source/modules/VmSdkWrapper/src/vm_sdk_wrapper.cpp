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
 * @file vm_sdk_wrapper.cpp
 * @brief Provides an interface to VMSDK
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a common interface for using the SDK for ISC100VM.
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

#include "vm_sdk_wrapper.h"

#include "opencv2\opencv.hpp"
#include "ISCSDKLib_define.h"

#pragma comment (lib, "shlwapi")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world480d")
#else
#pragma comment (lib,"opencv_world480")
#endif

#define SDK_VERSION 2320

extern char vm_module_file_name_[1024];

namespace ns_vmsdk_wrapper
{
constexpr char kISC_VM_DRV_FILE_NAME[] = "ISCSDKLibvm200.dll";

// Function typedef

// int OpenISC();
typedef int (WINAPI* TOpenISC)();
TOpenISC openISC = NULL;

//int CloseISC();
typedef int (WINAPI* TCloseISC)();
TCloseISC closeISC = NULL;

//int SetISCRunMode(int nMode);
typedef int (WINAPI* TSetISCRunMode)(int);
TSetISCRunMode setISCRunMode = NULL;

//int GetISCRunMode(int* pMode);
typedef int (WINAPI* TGetISCRunMode)(int*);
TGetISCRunMode getISCRunMode = NULL;

//int StartGrab(int nMode);
typedef int (WINAPI* TStartGrab)(int);
TStartGrab startGrab = NULL;

//int StopGrab();
typedef int (WINAPI* TStopGrab)();
TStopGrab stopGrab = NULL;

//int GetImage(unsigned char* pBuffer1, unsigned char* pBuffer2, int nSkip);
typedef int (WINAPI* TGetImage)(unsigned char*, unsigned char*, int);
TGetImage getImage = NULL;

//int GetImageEx(unsigned char* pBuffer1, unsigned char* pBuffer2, int nSkip, int nWaitTime = 100);
typedef int (WINAPI* TGetImageEx)(unsigned char*, unsigned char*, int, int);
TGetImageEx getImageEx = NULL;

//int GetDepthInfo(float* pBuffer);
typedef int (WINAPI* TGetDepthInfo)(float*);
TGetDepthInfo getDepthInfo = NULL;

//int GetCameraParamInfo(CameraParamInfo* pParam);
typedef int (WINAPI* TGetCameraParamInfo)(CameraParamInfo*);
TGetCameraParamInfo getCameraParamInfo = NULL;

//int GetImageSize(unsigned int* pnWidth, unsigned int* pnHeight);
typedef int (WINAPI* TGetImageSize)(int*, int*);
TGetImageSize getImageSize = NULL;

//int SetAutoCalibration(int nMode);
typedef int (WINAPI* TSetAutoCalibration)(int);
TSetAutoCalibration setAutoCalibration = NULL;

//int GetAutoCalibration(int* nMode);
typedef int (WINAPI* TGetAutoCalibration)(int*);
TGetAutoCalibration getAutoCalibration = NULL;

//int SetShutterControlMode(int nMode);
typedef int (WINAPI* TSetShutterControlMode)(int);
TSetShutterControlMode setShutterControlMode = NULL;

//int GetShutterControlMode(int* nMode);
typedef int (WINAPI* TGetShutterControlMode)(int*);
TGetShutterControlMode getShutterControlMode = NULL;

//int GetExposureValue(unsigned int* pnValue);
typedef int (WINAPI* TGetExposureValue)(unsigned int*);
TGetExposureValue getExposureValue = NULL;

//int SetExposureValue(unsigned int nValue);
typedef int (WINAPI* TSetExposureValue)(unsigned int);
TSetExposureValue setExposureValue = NULL;

//int SetFineExposureValue(unsigned int nValue);
typedef int (WINAPI* TSetFineExposureValue)(unsigned int);
TSetFineExposureValue setFineExposureValue = NULL;

//int GetFineExposureValue(unsigned int* pnValue);
typedef int (WINAPI* TGetFineExposureValue)(unsigned int*);
TGetFineExposureValue getFineExposureValue = NULL;

//int GetGainValue(unsigned int* pnValue);
typedef int (WINAPI* TGetGainValue)(unsigned int*);
TGetGainValue getGainValue = NULL;

//int SetGainValue(unsigned int nValue);
typedef int (WINAPI* TSetGainValue)(unsigned int);
TSetGainValue setGainValue = NULL;

//int SetHDRMode(int nValue);
typedef int (WINAPI* TSetHDRMode)(int);
TSetHDRMode setHDRMode = NULL;

//int GetHDRMode(int* pnMode);
typedef int (WINAPI* TGetHDRMode)(int*);
TGetHDRMode getHDRMode = NULL;

//int SetHiResolutionMode(int nValue);
typedef int (WINAPI* TSetHiResolutionMode)(int);
TSetHiResolutionMode setHiResolutionMode = NULL;

//int GetHiResolutionMode(int* pnMode);
typedef int (WINAPI* TGetHiResolutionMode)(int*);
TGetHiResolutionMode getHiResolutionMode = NULL;

//int SetNoiseFilter(unsigned int nDCDX);
typedef int (WINAPI* TSetNoiseFilter)(int);
TSetNoiseFilter setNoiseFilter = NULL;

//int GetNoiseFilter(unsigned int* nDCDX);
typedef int (WINAPI* TGetNoiseFilter)(int*);
TGetNoiseFilter getNoiseFilter = NULL;

//int SetMeasArea(int mode, int nTop, int nLeft, int nRight, int nBottom, int nTop_Left, int nTop_Right, int nBottom_Left, int nBottom_Right);
typedef int (WINAPI* TSetMeasArea)(int,int,int,int,int,int,int,int,int);
TSetMeasArea setMeasArea = NULL;

//int GetMeasArea(int* mode, int* nTop, int* nLeft, int* nRight, int* nBottom, int* nTop_Left, int* nTop_Right, int* nBottom_Left, int* nBottom_Right);
typedef int (WINAPI* TGetMeasArea)(int*,int*,int*,int*,int*,int*,int*,int*,int*);
TGetMeasArea getMeasArea = NULL;

//int SetCameraFPSMode(int nMode);
typedef int (WINAPI* TSetCameraFPSMode)(int);
TSetCameraFPSMode setCameraFPSMode = NULL;

//int GetCameraFPSMode(int* pnMode, int* pnNominalFPS);
typedef int (WINAPI* TGetCameraFPSMode)(int*, int*);
TGetCameraFPSMode getCameraFPSMode = NULL;

//int GetFullFrameInfo(unsigned char* pBuffer);
typedef int (WINAPI* TGetFullFrameInfo)(unsigned char*);
TGetFullFrameInfo getFullFrameInfo = NULL;

//int GetFullFrameInfo2(unsigned char* pBuffer);
typedef int (WINAPI* TGetFullFrameInfo2)(unsigned char*);
TGetFullFrameInfo2 getFullFrameInfo2 = NULL;

//int SetCameraRegData(unsigned char* pwBuf, unsigned int wSize);
typedef int (WINAPI* TSetCameraRegData)(unsigned char*, unsigned int);
TSetCameraRegData setCameraRegData = NULL;

//int GetCameraRegData(unsigned char* pwBuf, unsigned char* prBuf, unsigned int wSize, unsigned int rSize);
typedef int (WINAPI* TGetCameraRegData)(unsigned char*,  unsigned char*, unsigned int, unsigned int);
TGetCameraRegData getCameraRegData = NULL;

/**
 * constructor
 *
 */
VmSdkWrapper::VmSdkWrapper()
	:module_path_(), file_name_of_dll_(), dll_handle_(NULL), vm_camera_param_info_(), isc_grab_start_mode_(), isc_shutter_mode_(IscShutterMode::kManualShutter), isc_image_info_(), decode_buffer_()
{
	isc_grab_start_mode_.isc_grab_mode = IscGrabMode::kParallax;
	isc_grab_start_mode_.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
}

/**
 * destructor
 *
 */
VmSdkWrapper::~VmSdkWrapper()
{

}

/**
 * Initialize the class. 
 *
 * @return 0 if successful.
 */
int VmSdkWrapper::Initialize()
{
	// get module path
	char module_path_name[MAX_PATH + 1] = {};
	GetModuleFileNameA(NULL, module_path_name, MAX_PATH);

	// DLL のエントリ ポイントより取得
	sprintf_s(module_path_, "%s", vm_module_file_name_);
	PathRemoveFileSpecA(module_path_);

	// value in specification
	constexpr int camera_width = 752;
	constexpr int camea_height = 480;

	size_t frame_size = camera_width * camea_height;

	for (int i = 0; i < 3; i++) {
		decode_buffer_.split_images[i] = new unsigned char[frame_size];
	}
	decode_buffer_.s0_image = new unsigned char[frame_size];
	decode_buffer_.s1_image = new unsigned char[frame_size];
	decode_buffer_.disparity_image = new unsigned char[frame_size];
	decode_buffer_.mask_image = new unsigned char[frame_size];
	decode_buffer_.disparity = new float[frame_size];

	// Set the specified value for initialization
	vm_camera_param_info_.image_width	= camera_width;
	vm_camera_param_info_.image_height	= camea_height;

	return ISC_OK;
}

/**
 * Terminate the class. Currently, do nothing.
 *
 * @return 0 if successful.
 */
int VmSdkWrapper::Terminate()
{
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
int VmSdkWrapper::DeviceOpen()
{

	int ret = LoadDLLFunction(module_path_);
	if (ret != DPC_E_OK) {
		return ret;
	}

	if (openISC != NULL) {
		ret = openISC();
	}

	if (ret == ISC_OK) {
		memset(&vm_camera_param_info_, 0, sizeof(vm_camera_param_info_));

		CameraParamInfo paramInfo = {};
		ret = getCameraParamInfo(&paramInfo);

		if (ret == ISC_OK) {
			vm_camera_param_info_.d_inf					= paramInfo.fD_INF;
			vm_camera_param_info_.bf					= paramInfo.fBF;
			vm_camera_param_info_.base_length			= paramInfo.fBaseLength;
			vm_camera_param_info_.dz					= paramInfo.fdZ;
			vm_camera_param_info_.view_angle			= paramInfo.fViewAngle;
			vm_camera_param_info_.image_width			= paramInfo.nImageWidth;
			vm_camera_param_info_.image_height			= paramInfo.nImageHeight;
			vm_camera_param_info_.product_number		= paramInfo.nProductNumber;
			vm_camera_param_info_.product_number2		= paramInfo.nProductNumber2;
			memcpy(vm_camera_param_info_.serial_number, paramInfo.nSerialNumber, sizeof(paramInfo.nSerialNumber));
			vm_camera_param_info_.fpga_version_major	= paramInfo.nFPGA_version_major;
			vm_camera_param_info_.fpga_version_minor	= paramInfo.nFPGA_version_minor;

			IscCameraParameter option = IscCameraParameter::kShutterMode;
			IscShutterMode shutter_mode = IscShutterMode::kManualShutter;
			int ret_temp = DeviceGetOption(option, &shutter_mode);

			if (ret_temp == DPC_E_OK) {
				isc_shutter_mode_ = shutter_mode;
			}

			InitializeIscIamgeinfo(&isc_image_info_);
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
int VmSdkWrapper::DeviceClose()
{

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
bool VmSdkWrapper::DeviceOptionIsImplemented(const IscCameraInfo option_name)
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
bool VmSdkWrapper::DeviceOptionIsReadable(const IscCameraInfo option_name)
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
bool VmSdkWrapper::DeviceOptionIsWritable(const IscCameraInfo option_name)
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
int VmSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, int* value)
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
int VmSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, int* value)
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
int VmSdkWrapper::DeviceGetOptionInc(const IscCameraInfo option_name, int* value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kWidthMax:
		*value = vm_camera_param_info_.image_width;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kHeightMax:
		*value = vm_camera_param_info_.image_height;
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
int VmSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const int value)
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
int VmSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, float* value)
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
int VmSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, float* value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, float* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kBF:
		*value = vm_camera_param_info_.bf;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kDINF:
		*value = vm_camera_param_info_.d_inf;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kDz:
		*value = vm_camera_param_info_.dz;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kBaseLength:
		*value = vm_camera_param_info_.base_length;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kViewAngle:
		*value = vm_camera_param_info_.view_angle;
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
int VmSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const float value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, bool* value)
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
int VmSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const bool value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kSerialNumber:
		sprintf_s(value, max_length, "%s", vm_camera_param_info_.serial_number);
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
int VmSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const char* value)
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
int VmSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value)
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
int VmSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value)
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
int VmSdkWrapper::DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, uint64_t* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kProductID:
		*value = vm_camera_param_info_.product_number | ((uint64_t)vm_camera_param_info_.product_number2) << 32;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kSerialNumber:
		break;

	case IscCameraInfo::kFpgaVersion:
		*value = vm_camera_param_info_.fpga_version_minor | ((uint64_t)vm_camera_param_info_.fpga_version_major) << 32;
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
int VmSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const uint64_t value)
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
bool VmSdkWrapper::DeviceOptionIsImplemented(const IscCameraParameter option_name)
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
		ret_value = true;
		break;

	case IscCameraParameter::kDoubleShutter2:
		ret_value = true;
		break;

	case IscCameraParameter::kExposure:
		ret_value = true;
		break;

	case IscCameraParameter::kFineExposure:
#if SDK_VERSION == 2400
		ret_value = true;
#else
		ret_value = false;
#endif
		break;

	case IscCameraParameter::kGain:
		ret_value = true;
		break;

	case IscCameraParameter::kHrMode:
		ret_value = true;
		break;

	case IscCameraParameter::kHdrMode:
		ret_value = true;
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
bool VmSdkWrapper::DeviceOptionIsReadable(const IscCameraParameter option_name)
{
	bool ret_value = false;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		ret_value = true;
		break;

	case IscCameraParameter::kExposure:
		ret_value = true;
		break;

	case IscCameraParameter::kFineExposure:
#if SDK_VERSION == 2400
		ret_value = true;
#else
		ret_value = false;
#endif
		break;

	case IscCameraParameter::kGain:
		ret_value = true;
		break;

	case IscCameraParameter::kHrMode:
		ret_value = true;
		break;

	case IscCameraParameter::kHdrMode:
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
bool VmSdkWrapper::DeviceOptionIsWritable(const IscCameraParameter option_name)
{
	bool ret_value = false;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		ret_value = true;
		break;

	case IscCameraParameter::kExposure:
		ret_value = true;
		break;

	case IscCameraParameter::kFineExposure:
#if SDK_VERSION == 2400
		ret_value = true;
#else
		ret_value = false;
#endif
		break;

	case IscCameraParameter::kGain:
		ret_value = true;
		break;

	case IscCameraParameter::kHrMode:
		ret_value = true;
		break;

	case IscCameraParameter::kHdrMode:
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
int VmSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
		*value = 1;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kFineExposure:
#if SDK_VERSION == 2400
		*value = 0;
		ret_value = DPC_E_OK;
#else
		*value = 0;
		ret_value = CAMCONTROL_E_INVALID_REQUEST;
#endif
		break;

	case IscCameraParameter::kGain:
		*value = 16;
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
int VmSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
		*value = 480;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kFineExposure:
#if SDK_VERSION == 2400
		*value = 828;
		ret_value = DPC_E_OK;
#else
		*value = 0;
		ret_value = CAMCONTROL_E_INVALID_REQUEST;
#endif
		break;

	case IscCameraParameter::kGain:
		*value = 64;
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
int VmSdkWrapper::DeviceGetOptionInc(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraParameter::kExposure:
		*value = 1;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kFineExposure:
#if SDK_VERSION == 2400
		*value = 1;
		ret_value = DPC_E_OK;
#else
		*value = 0;
		ret_value = CAMCONTROL_E_INVALID_REQUEST;
#endif
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
int VmSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	unsigned int get_value = 0;

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

	case IscCameraParameter::kFineExposure:
#if SDK_VERSION == 2400
		ret_value = GetFineExposureValue(&get_value);
		if (ret_value == ISC_OK) {
			*value = static_cast<int>(get_value);
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
#else
		ret_value = CAMCONTROL_E_INVALID_REQUEST;
#endif

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
		ret_value = GetStereoMatchingsOcclusionRemoval(&get_value);
		if (ret_value == ISC_OK) {
			*value = static_cast<int>(get_value);
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
int VmSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const int value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;

	const unsigned int set_value = static_cast<unsigned int>(value);

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
		ret_value = setExposureValue(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kFineExposure:
#if SDK_VERSION == 2400
		ret_value = SetFineExposureValue(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
#else
		ret_value = CAMCONTROL_E_INVALID_REQUEST;
#endif
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
int VmSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, float* value)
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
int VmSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, float* value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, float* value)
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
int VmSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const float value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, bool* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = false;

	int get_value = 0;

	switch (option_name) {
	case IscCameraParameter::kHrMode:
		ret_value = getHiResolutionMode(&get_value);
		if (ret_value == ISC_OK) {
			if (get_value == 0) {
				*value = false;
			}
			else {
				*value = true;
			}
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kHdrMode:
		ret_value = getHDRMode(&get_value);
		if (ret_value == ISC_OK) {
			if (get_value == 0) {
				*value = false;
			}
			else {
				*value = true;
			}
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

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
int VmSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const bool value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	unsigned int set_value = 0;

	switch (option_name) {
	case IscCameraParameter::kHrMode:
		if (value) {
			set_value = 1;
		}
		else {
			set_value = 0;
		}
		ret_value = setHiResolutionMode(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kHdrMode:
		if (value) {
			set_value = 1;
		}
		else {
			set_value = 0;
		}
		ret_value = setHDRMode(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kAutoCalibration:
		if (value) {
			set_value = AUTOCALIBRATION_COMMAND_AUTO_ON;
		}
		else {
			set_value = AUTOCALIBRATION_COMMAND_STOP;
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
			set_value = AUTOCALIBRATION_COMMAND_MANUAL_START;

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
int VmSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length)
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
int VmSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const char* value)
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
int VmSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value)
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
int VmSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value)
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
int VmSdkWrapper::DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, uint64_t* value)
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
int VmSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const uint64_t value)
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
int VmSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value)
{
	*value = IscShutterMode::kManualShutter;

	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	int get_value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		ret_value = getShutterControlMode(&get_value);
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
int VmSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value)
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

		ret_value = setShutterControlMode(set_value);
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
int VmSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size, unsigned char* read_value, const int read_size)
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
int VmSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size)
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
int VmSdkWrapper::Start(const IscGrabStartMode* isc_grab_start_mode)
{

	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	int start_mode = 0;

	switch (isc_grab_start_mode->isc_grab_mode) {
	case IscGrabMode::kParallax:
		start_mode = 2;
		break;

	case IscGrabMode::kCorrect:
		start_mode = 3;
		break;

	case IscGrabMode::kBeforeCorrect:
		start_mode = 4;
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
		return CAMCONTROL_E_INVALID_REQUEST;
		break;

	default:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;
	}

	int camera_ret_value = startGrab(start_mode);

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
int VmSdkWrapper::Stop()
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
int VmSdkWrapper::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
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
int VmSdkWrapper::InitializeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	int width = vm_camera_param_info_.image_width;
	int height = vm_camera_param_info_.image_height;

	isc_image_info->grab = IscGrabMode::kParallax;
	isc_image_info->color_grab_mode = IscGrabColorMode::kColorOFF;
	isc_image_info->shutter_mode = IscShutterMode::kManualShutter;
	isc_image_info->camera_specific_parameter.d_inf = vm_camera_param_info_.d_inf;
	isc_image_info->camera_specific_parameter.bf = vm_camera_param_info_.bf;
	isc_image_info->camera_specific_parameter.base_length = vm_camera_param_info_.base_length;
	isc_image_info->camera_specific_parameter.dz = vm_camera_param_info_.dz;

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
		isc_image_info->frame_data[i].color.image = nullptr;

		isc_image_info->frame_data[i].depth.width = 0;
		isc_image_info->frame_data[i].depth.height = 0;
		isc_image_info->frame_data[i].depth.image = new float[width * height];

		isc_image_info->frame_data[i].raw.width = 0;
		isc_image_info->frame_data[i].raw.height = 0;
		isc_image_info->frame_data[i].raw.channel_count = 0;
		isc_image_info->frame_data[i].raw.image = new unsigned char[width * height * 2];

		isc_image_info->frame_data[i].raw_color.width = 0;
		isc_image_info->frame_data[i].raw_color.height = 0;
		isc_image_info->frame_data[i].raw_color.channel_count = 0;
		isc_image_info->frame_data[i].raw_color.image = nullptr;

		size_t image_size = width * height;
		memset(isc_image_info->frame_data[i].p1.image, 0, image_size);
		memset(isc_image_info->frame_data[i].p2.image, 0, image_size);

		image_size = width * height * sizeof(float);
		memset(isc_image_info->frame_data[i].depth.image, 0, image_size);

		image_size = width * height * 2;
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
int VmSdkWrapper::ReleaeIscIamgeinfo(IscImageInfo* isc_image_info)
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
		assert(isc_image_info->frame_data[i].color.image == nullptr);
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
		assert(isc_image_info->frame_data[i].raw_color.image == nullptr);
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
int VmSdkWrapper::GetData(const IscGetMode* isc_get_mode, IscImageInfo* isc_image_info)
{
	isc_image_info->grab = isc_grab_start_mode_.isc_grab_mode;
	isc_image_info->color_grab_mode = isc_grab_start_mode_.isc_grab_color_mode;
	isc_image_info->shutter_mode = isc_shutter_mode_;
	isc_image_info->camera_specific_parameter.d_inf = vm_camera_param_info_.d_inf;
	isc_image_info->camera_specific_parameter.bf = vm_camera_param_info_.bf;
	isc_image_info->camera_specific_parameter.base_length = vm_camera_param_info_.base_length;
	isc_image_info->camera_specific_parameter.dz = vm_camera_param_info_.dz;

	for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {

		isc_image_info->frame_data[i].camera_status.error_code = ISC_OK;
		isc_image_info->frame_data[i].camera_status.data_receive_tact_time = 0;

		isc_image_info->frame_data[i].frameNo = -1;
		isc_image_info->frame_data[i].gain = -1;
		isc_image_info->frame_data[i].exposure = -1;

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

	constexpr int frame_data_id = kISCIMAGEINFO_FRAMEDATA_LATEST;
	int ret = getImageEx(isc_image_info_.frame_data[frame_data_id].p2.image, isc_image_info_.frame_data[frame_data_id].p1.image, 1, isc_get_mode->wait_time);
	isc_image_info->frame_data[frame_data_id].camera_status.error_code = ret;

	if (ret != ISC_OK) {
		if (ret != 0) {
			if (ret == ERR_USB_NO_IMAGE) {
				/*ERR_USB_NO_IMAGE*/
				return CAMCONTROL_E_NO_IMAGE;
			}
			else if (ret == FT_IO_ERROR) {
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

	int width = vm_camera_param_info_.image_width;
	int height = vm_camera_param_info_.image_height;

	// RAW data
	if (isc_grab_start_mode_.isc_get_raw_mode == IscGetModeRaw::kRawOn) {
		ret = getFullFrameInfo(isc_image_info->frame_data[frame_data_id].raw.image);
		if (ret != 0) {
			return CAMCONTROL_E_GET_FULL_FRAME_FAILED;
		}
		isc_image_info->frame_data[frame_data_id].raw.width = width * 2;
		isc_image_info->frame_data[frame_data_id].raw.height = height;
		isc_image_info->frame_data[frame_data_id].raw.channel_count = 1;

		// 反転不要です
	}

	// 他のカメラと画像の向きを統一させるために左右反転します
	constexpr bool is_flip_for_compatibility = true;

	if (is_flip_for_compatibility) {
		// 基準側画像
		isc_image_info->frame_data[frame_data_id].p1.width = width;
		isc_image_info->frame_data[frame_data_id].p1.height = height;
		isc_image_info->frame_data[frame_data_id].p1.channel_count = 1;

		// 他のカメラとの互換性のために左右を反転します
		cv::Mat mat_src_image_p1(isc_image_info->frame_data[frame_data_id].p1.height, isc_image_info->frame_data[frame_data_id].p1.width, CV_8UC1, isc_image_info_.frame_data[frame_data_id].p1.image);
		cv::Mat mat_dst_image_p1(isc_image_info->frame_data[frame_data_id].p1.height, isc_image_info->frame_data[frame_data_id].p1.width, CV_8UC1, isc_image_info->frame_data[frame_data_id].p1.image);
		cv::flip(mat_src_image_p1, mat_dst_image_p1, 1);

		if (isc_grab_start_mode_.isc_grab_mode == IscGrabMode::kParallax) {
			ret = getDepthInfo(isc_image_info_.frame_data[frame_data_id].depth.image);
			if (ret != ISC_OK) {
				return CAMCONTROL_E_GET_DEPTH_FAILED;
			}
			isc_image_info->frame_data[frame_data_id].depth.width = width;
			isc_image_info->frame_data[frame_data_id].depth.height = height;

			// 他のカメラとの互換性のために左右を反転します
			cv::Mat mat_src_image_depth(isc_image_info->frame_data[frame_data_id].depth.height, isc_image_info->frame_data[frame_data_id].depth.width, CV_32FC1, isc_image_info_.frame_data[frame_data_id].depth.image);
			cv::Mat mat_dst_image_depth(isc_image_info->frame_data[frame_data_id].depth.height, isc_image_info->frame_data[frame_data_id].depth.width, CV_32FC1, isc_image_info->frame_data[frame_data_id].depth.image);
			cv::flip(mat_src_image_depth, mat_dst_image_depth, 1);
		}
		else {
			// 補正後比較画像/補正前比較画像
			isc_image_info->frame_data[frame_data_id].p2.width = width;
			isc_image_info->frame_data[frame_data_id].p2.height = height;
			isc_image_info->frame_data[frame_data_id].p2.channel_count = 1;

			// 他のカメラとの互換性のために左右を反転します
			cv::Mat mat_src_image_p2(isc_image_info->frame_data[frame_data_id].p2.height, isc_image_info->frame_data[frame_data_id].p2.width, CV_8UC1, isc_image_info_.frame_data[frame_data_id].p2.image);
			cv::Mat mat_dst_image_p2(isc_image_info->frame_data[frame_data_id].p2.height, isc_image_info->frame_data[frame_data_id].p2.width, CV_8UC1, isc_image_info->frame_data[frame_data_id].p2.image);
			cv::flip(mat_src_image_p2, mat_dst_image_p2, 1);
		}
	}
	else {
		// 基準側画像
		isc_image_info->frame_data[frame_data_id].p1.width = width;
		isc_image_info->frame_data[frame_data_id].p1.height = height;
		isc_image_info->frame_data[frame_data_id].p1.channel_count = 1;

		size_t cp_size = isc_image_info->frame_data[frame_data_id].p1.width * isc_image_info->frame_data[frame_data_id].p1.height;
		memcpy(isc_image_info->frame_data[frame_data_id].p1.image, isc_image_info_.frame_data[frame_data_id].p1.image, cp_size);

		if (isc_grab_start_mode_.isc_grab_mode == IscGrabMode::kParallax) {
			ret = getDepthInfo(isc_image_info_.frame_data[frame_data_id].depth.image);
			if (ret != ISC_OK) {
				return CAMCONTROL_E_GET_DEPTH_FAILED;
			}
			isc_image_info->frame_data[frame_data_id].depth.width = width;
			isc_image_info->frame_data[frame_data_id].depth.height = height;

			cp_size = isc_image_info->frame_data[frame_data_id].depth.width * isc_image_info->frame_data[frame_data_id].depth.height * sizeof(float);
			memcpy(isc_image_info->frame_data[frame_data_id].depth.image, isc_image_info_.frame_data[frame_data_id].depth.image, cp_size);
		}
		else {
			// 補正後比較画像/補正前比較画像
			isc_image_info->frame_data[frame_data_id].p2.width = width;
			isc_image_info->frame_data[frame_data_id].p2.height = height;
			isc_image_info->frame_data[frame_data_id].p2.channel_count = 1;

			cp_size = isc_image_info->frame_data[frame_data_id].p2.width * isc_image_info->frame_data[frame_data_id].p2.height;
			memcpy(isc_image_info->frame_data[frame_data_id].p2.image, isc_image_info_.frame_data[frame_data_id].p2.image, cp_size);
		}
	}

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
int VmSdkWrapper::Decode(const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode,
	const int width, const int height, IscImageInfo* isc_image_info)
{

	constexpr int frame_data_id = kISCIMAGEINFO_FRAMEDATA_LATEST;

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
int VmSdkWrapper::SplitImage(const bool is_disparity, const int width, const int height, unsigned char* raw_data, unsigned char* image1, unsigned char* image2, unsigned char* image3)
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
int VmSdkWrapper::ReCreateParallaxImage(const int width, const int height, unsigned char* src_data, float* temp_disparity, unsigned char* dst_image, unsigned char* mask_image)
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
int VmSdkWrapper::FlipImage(const int width, const int height, unsigned char* src_image, unsigned char* dst_image)
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
int VmSdkWrapper::SetStereoMatchingsPeculiarRemoval(const int value)
{
	constexpr int USB_WRITE_CMD_SIZE = 5;
	unsigned char wbuf[USB_WRITE_CMD_SIZE] = { };

	wbuf[0] = 0xF0;
	wbuf[1] = 0x00;
	wbuf[2] = 0x12;
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
int VmSdkWrapper::GetStereoMatchingsPeculiarRemoval(int* value)
{
	constexpr int usb_write_cmd_size = 5;
	unsigned char wbuf[usb_write_cmd_size] = { };
	constexpr int usb_read_data_size = 16;
	unsigned char rbuf[usb_read_data_size] = { };

	wbuf[0] = 0xF1;
	wbuf[1] = 0x00;
	wbuf[2] = 0x12;
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
int VmSdkWrapper::SetStereoMatchingsOcclusionRemoval(const unsigned int value)
{

	constexpr int USB_WRITE_CMD_SIZE = 5;
	unsigned char wbuf[USB_WRITE_CMD_SIZE] = { };

	wbuf[0] = 0xF0;
	wbuf[1] = 0x00;
	wbuf[2] = 0x11;
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
 * Get occlusion removal regiser.
 *
 * @param[out] Value read from camera 0:off 1:on.
 * @return 0 if successful.
 */
int VmSdkWrapper::GetStereoMatchingsOcclusionRemoval(unsigned int* value)
{

	constexpr int usb_write_cmd_size = 5;
	unsigned char wbuf[usb_write_cmd_size] = { };
	constexpr int usb_read_data_size = 16;
	unsigned char rbuf[usb_read_data_size] = { };

	wbuf[0] = 0xF1;
	wbuf[1] = 0x00;
	wbuf[2] = 0x11;
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
int VmSdkWrapper::LoadDLLFunction(char* module_path)
{
	sprintf_s(file_name_of_dll_, "%s\\%s", module_path, kISC_VM_DRV_FILE_NAME);

	// Load DLL
	dll_handle_ = LoadLibraryA(file_name_of_dll_);

	if (dll_handle_ == NULL) {
		char dbg_msg[512] = {};
		sprintf_s(dbg_msg, "Failed to load DLL(%s)", file_name_of_dll_);
		MessageBoxA(NULL, dbg_msg, "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}

	// Load Function
	
	// int OpenISC();
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

	//int SetISCRunMode(int nMode);
	proc = GetProcAddress(dll_handle_, "SetISCRunMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setISCRunMode = reinterpret_cast<TSetISCRunMode>(proc);

	//int GetISCRunMode(int* pMode);
	proc = GetProcAddress(dll_handle_, "GetISCRunMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getISCRunMode = reinterpret_cast<TGetISCRunMode>(proc);

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

	//int GetImage(unsigned char* pBuffer1, unsigned char* pBuffer2, int nSkip);
	proc = GetProcAddress(dll_handle_, "GetImage");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getImage = reinterpret_cast<TGetImage>(proc);

	//int GetImageEx(unsigned char* pBuffer1, unsigned char* pBuffer2, int nSkip, int nWaitTime = 100);
	proc = GetProcAddress(dll_handle_, "GetImageEx");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getImageEx = reinterpret_cast<TGetImageEx>(proc);

	//int GetDepthInfo(float* pBuffer);
	proc = GetProcAddress(dll_handle_, "GetDepthInfo");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getDepthInfo = reinterpret_cast<TGetDepthInfo>(proc);

	//int GetCameraParamInfo(CameraParamInfo* pParam);
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

	//int SetShutterControlMode(bool nMode);
	proc = GetProcAddress(dll_handle_, "SetShutterControlMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setShutterControlMode = reinterpret_cast<TSetShutterControlMode>(proc);

	//int GetShutterControlMode(bool* nMode);
	proc = GetProcAddress(dll_handle_, "GetShutterControlMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getShutterControlMode = reinterpret_cast<TGetShutterControlMode>(proc);

	//int GetExposureValue(unsigned int* pnValue);
	proc = GetProcAddress(dll_handle_, "GetExposureValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getExposureValue = reinterpret_cast<TGetExposureValue>(proc);

	//int SetExposureValue(unsigned int nValue);
	proc = GetProcAddress(dll_handle_, "SetExposureValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setExposureValue = reinterpret_cast<TSetExposureValue>(proc);

	//int GetGainValue(unsigned int* pnValue);
	proc = GetProcAddress(dll_handle_, "GetGainValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getGainValue = reinterpret_cast<TGetGainValue>(proc);

	//int SetGainValue(unsigned int nValue);
	proc = GetProcAddress(dll_handle_, "SetGainValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setGainValue = reinterpret_cast<TSetGainValue>(proc);

	//int SetHDRMode(int nValue);
	proc = GetProcAddress(dll_handle_, "SetHDRMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setHDRMode = reinterpret_cast<TSetHDRMode>(proc);

	//int GetHDRMode(int* pnMode);
	proc = GetProcAddress(dll_handle_, "GetHDRMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getHDRMode = reinterpret_cast<TGetHDRMode>(proc);

	//int SetHiResolutionMode(int nValue);
	proc = GetProcAddress(dll_handle_, "SetHiResolutionMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setHiResolutionMode = reinterpret_cast<TSetHiResolutionMode>(proc);

	//int GetHiResolutionMode(int* pnMode);
	proc = GetProcAddress(dll_handle_, "GetHiResolutionMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getHiResolutionMode = reinterpret_cast<TGetHiResolutionMode>(proc);

	//int SetNoiseFilter(unsigned int nDCDX);
	proc = GetProcAddress(dll_handle_, "SetNoiseFilter");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setNoiseFilter = reinterpret_cast<TSetNoiseFilter>(proc);

	//int GetNoiseFilter(unsigned int* nDCDX);
	proc = GetProcAddress(dll_handle_, "GetNoiseFilter");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getNoiseFilter = reinterpret_cast<TGetNoiseFilter>(proc);

	//int SetMeasArea(int mode, int nTop, int nLeft, int nRight, int nBottom, int nTop_Left, int nTop_Right, int nBottom_Left, int nBottom_Right);
	proc = GetProcAddress(dll_handle_, "SetMeasArea");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setMeasArea = reinterpret_cast<TSetMeasArea>(proc);

	//int GetMeasArea(int* mode, int* nTop, int* nLeft, int* nRight, int* nBottom, int* nTop_Left, int* nTop_Right, int* nBottom_Left, int* nBottom_Right);
	proc = GetProcAddress(dll_handle_, "GetMeasArea");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getMeasArea = reinterpret_cast<TGetMeasArea>(proc);

	//int SetCameraFPSMode(int nMode);
	proc = GetProcAddress(dll_handle_, "SetCameraFPSMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setCameraFPSMode = reinterpret_cast<TSetCameraFPSMode>(proc);

	//int GetCameraFPSMode(int* pnMode, int* pnNominalFPS);
	proc = GetProcAddress(dll_handle_, "GetCameraFPSMode");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getCameraFPSMode = reinterpret_cast<TGetCameraFPSMode>(proc);

	//int GetFullFrameInfo(unsigned char* pBuffer);
	proc = GetProcAddress(dll_handle_, "GetFullFrameInfo");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getFullFrameInfo = reinterpret_cast<TGetFullFrameInfo>(proc);

	//int GetFullFrameInfo2(unsigned char* pBuffer);
	proc = GetProcAddress(dll_handle_, "GetFullFrameInfo2");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getFullFrameInfo2 = reinterpret_cast<TGetFullFrameInfo2>(proc);

	//int SetCameraRegData(unsigned char* pwBuf, unsigned int wSize);
	proc = GetProcAddress(dll_handle_, "SetCameraRegData");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setCameraRegData = reinterpret_cast<TSetCameraRegData>(proc);

	//int GetCameraRegData(unsigned char* pwBuf, unsigned char* prBuf, unsigned int wSize, unsigned int rSize);
	proc = GetProcAddress(dll_handle_, "GetCameraRegData");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getCameraRegData = reinterpret_cast<TGetCameraRegData>(proc);



#if SDK_VERSION == 2400
	//int SetFineExposureValue(unsigned int nValue);
	proc = GetProcAddress(dll_handle_, "SetFineExposureValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	setFineExposureValue = reinterpret_cast<TSetFineExposureValue>(proc);

	//int GetFineExposureValue(unsigned int* pnValue);
	proc = GetProcAddress(dll_handle_, "GetFineExposureValue");
	if (proc == NULL) {
		MessageBoxA(NULL, "Failed to get function address", "LoadDLLFunction", MB_OK);
		return CAMCONTROL_E_LOAD_DLL_FAILED;
	}
	getFineExposureValue = reinterpret_cast<TGetFineExposureValue>(proc);

#endif

	return DPC_E_OK;
}

/**
 * unload function address from DLL.
 *
 * @return 0 if successful.
 */
int VmSdkWrapper::UnLoadDLLFunction()
{
	// unload dll
	if (dll_handle_ != NULL) {
		FreeLibrary(dll_handle_);
		dll_handle_ = NULL;
	}

	openISC = NULL;
	closeISC = NULL;
	setISCRunMode = NULL;
	getISCRunMode = NULL;
	startGrab = NULL;
	stopGrab = NULL;
	getImage = NULL;
	getImageEx = NULL;
	getDepthInfo = NULL;
	getCameraParamInfo = NULL;
	getImageSize = NULL;
	setAutoCalibration = NULL;
	getAutoCalibration = NULL;
	setShutterControlMode = NULL;
	getShutterControlMode = NULL;
	getExposureValue = NULL;
	setExposureValue = NULL;
	setFineExposureValue = NULL;
	getFineExposureValue = NULL;
	getGainValue = NULL;
	setGainValue = NULL;
	setHDRMode = NULL;
	getHDRMode = NULL;
	setHiResolutionMode = NULL;
	getHiResolutionMode = NULL;
	setNoiseFilter = NULL;
	getNoiseFilter = NULL;
	setMeasArea = NULL;
	getMeasArea = NULL;
	setCameraFPSMode = NULL;
	getCameraFPSMode = NULL;
	getFullFrameInfo = NULL;
	getFullFrameInfo2 = NULL;
	setCameraRegData = NULL;
	getCameraRegData = NULL;

	return DPC_E_OK;
}




} /* namespace ns_vmsdk_wrapper */
