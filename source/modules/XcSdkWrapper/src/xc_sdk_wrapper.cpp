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
 * @file xc_sdk_wrapper.cpp
 * @brief Provides an interface to XCSDK
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

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"

#include "xc_sdk_wrapper.h"

#include "opencv2\opencv.hpp"
#include "ISCSDKLib.h"
#include "utility.h"

#pragma comment (lib, "ISCSDKLibxc")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world470d")
#else
#pragma comment (lib,"opencv_world470")
#endif

namespace ns_xcsdk_wrapper
{

/**
 * constructor
 *
 */
XcSdkWrapper::XcSdkWrapper()
		:xc_camera_param_info_(), isc_grab_start_mode_(), isc_shutter_mode_(IscShutterMode::kManualShutter), isc_image_info_(), work_buffer_(), decode_buffer_()
{
	isc_grab_start_mode_.isc_grab_mode = IscGrabMode::kParallax;
	isc_grab_start_mode_.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
}

/**
 * destructor
 *
 */
XcSdkWrapper::~XcSdkWrapper()
{

}

/**
 * Initialize the class.
 *
 * @return 0 if successful.
 */
int XcSdkWrapper::Initialize()
{
	// value in specification
	constexpr int camera_width = 1280;
	constexpr int camea_height = 720;

	size_t frame_size = camera_width * camea_height;

	for (int i = 0; i < 3; i++) {
		decode_buffer_.split_images[i] = new unsigned char[frame_size];
	}
	decode_buffer_.base_image = new unsigned char[frame_size];
	decode_buffer_.compare_image = new unsigned char[frame_size];
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
	xc_camera_param_info_.image_width	= camera_width;
	xc_camera_param_info_.image_height	= camea_height;

	return ISC_OK;
}

/**
 * Terminate the class. Currently, do nothing.
 *
 * @return 0 if successful.
 */
int XcSdkWrapper::Terminate()
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
	delete[] decode_buffer_.base_image;
	decode_buffer_.base_image = nullptr;
	delete[] decode_buffer_.compare_image;
	decode_buffer_.compare_image = nullptr;
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
int XcSdkWrapper::DeviceOpen()
{

	int ret = OpenISC();

	if (ret == ISC_OK) {
		memset(&xc_camera_param_info_, 0, sizeof(xc_camera_param_info_));

		CameraParamInfo paramInfo = {};
		ret = GetCameraParamInfo(&paramInfo);

		if (ret == ISC_OK) {
			xc_camera_param_info_.d_inf					= paramInfo.fD_INF;
			xc_camera_param_info_.bf					= paramInfo.fBF;
			xc_camera_param_info_.base_length			= paramInfo.fBaseLength;
			xc_camera_param_info_.dz					= 0;	// xc not supported
			xc_camera_param_info_.view_angle			= paramInfo.fViewAngle;
			xc_camera_param_info_.image_width			= paramInfo.nImageWidth;
			xc_camera_param_info_.image_height			= paramInfo.nImageHeight;
			xc_camera_param_info_.product_number		= paramInfo.nProductNumber;
			xc_camera_param_info_.product_number2		= 0;	// xc not supported
			sprintf_s(xc_camera_param_info_.serial_number, "%d", paramInfo.nSerialNumber);
			xc_camera_param_info_.fpga_version_major	= paramInfo.nFPGA_version;
			xc_camera_param_info_.fpga_version_minor	= 0;	// xc not supported

			IscCameraParameter option = IscCameraParameter::kShutterMode;
			IscShutterMode shutter_mode = IscShutterMode::kManualShutter;
			int ret_temp = DeviceGetOption(option, &shutter_mode);

			if (ret_temp == DPC_E_OK) {
				isc_shutter_mode_ = shutter_mode;
			}

			InitializeIscIamgeinfo(&isc_image_info_);

			work_buffer_.max_width = xc_camera_param_info_.image_width;
			work_buffer_.max_height = xc_camera_param_info_.image_height;
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
int XcSdkWrapper::DeviceClose()
{

	work_buffer_.max_width = 0;
	work_buffer_.max_height = 0;
	for (int i = 0; i < 4; i++) {
		delete[] work_buffer_.buffer[i];
		work_buffer_.buffer[i] = nullptr;
	}

	ReleaeIscIamgeinfo(&isc_image_info_);

	int ret = CloseISC();

	int ret_value = DPC_E_OK;
	if (ret == ISC_OK) {
	}
	else {
		ret_value = CAMCONTROL_E_CLOSE_DEVICE_FAILED;
	}

	return ret;
}

// camera dependent paraneter
/**
 * whether or not the parameter is implemented.
 *
 * @param[in] option_name target parameter.
 * @return true, if implemented.
 */
bool XcSdkWrapper::DeviceOptionIsImplemented(const IscCameraInfo option_name)
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
bool XcSdkWrapper::DeviceOptionIsReadable(const IscCameraInfo option_name)
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
bool XcSdkWrapper::DeviceOptionIsWritable(const IscCameraInfo option_name)
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
int XcSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, int* value)
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
int XcSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, int* value)
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
int XcSdkWrapper::DeviceGetOptionInc(const IscCameraInfo option_name, int* value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kWidthMax:
		*value = xc_camera_param_info_.image_width;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kHeightMax:
		*value = xc_camera_param_info_.image_height;
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
int XcSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const int value)
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
int XcSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, float* value)
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
int XcSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, float* value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, float* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kBF:
		*value = xc_camera_param_info_.bf;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kDINF:
		*value = xc_camera_param_info_.d_inf;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kDz:
		*value = xc_camera_param_info_.dz;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kBaseLength:
		*value = xc_camera_param_info_.base_length;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kViewAngle:
		*value = xc_camera_param_info_.view_angle;
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
int XcSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const float value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, bool* value)
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
int XcSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const bool value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kSerialNumber:
		sprintf_s(value, max_length, "%s", xc_camera_param_info_.serial_number);
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
int XcSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const char* value)
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
int XcSdkWrapper::DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value)
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
int XcSdkWrapper::DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value)
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
int XcSdkWrapper::DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraInfo option_name, uint64_t* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraInfo::kProductID:
		*value = xc_camera_param_info_.product_number | ((uint64_t)xc_camera_param_info_.product_number2) << 32;
		ret_value = DPC_E_OK;
		break;

	case IscCameraInfo::kSerialNumber:
		break;

	case IscCameraInfo::kFpgaVersion:
		*value = xc_camera_param_info_.fpga_version_minor | ((uint64_t)xc_camera_param_info_.fpga_version_major) << 32;
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
int XcSdkWrapper::DeviceSetOption(const IscCameraInfo option_name, const uint64_t value)
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
bool XcSdkWrapper::DeviceOptionIsImplemented(const IscCameraParameter option_name)
{
	bool ret_value = false;

	switch (option_name) {
	case IscCameraParameter::kBaseImage:
		ret_value = true;
		break;

	case IscCameraParameter::kCompareImage:
		ret_value = true;
		break;

	case IscCameraParameter::kDepthData:
		ret_value = true;
		break;

	case IscCameraParameter::kColorImage:
		ret_value = true;
		break;

	case IscCameraParameter::kColorImageCorrect:
		ret_value = true;
		break;

	case IscCameraParameter::kAlternatelyColorImage:
		ret_value = true;
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

	case IscCameraParameter::kAdjustAuto:
		ret_value = true;
		break;

	case IscCameraParameter::kAdjustForce:
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
bool XcSdkWrapper::DeviceOptionIsReadable(const IscCameraParameter option_name)
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
bool XcSdkWrapper::DeviceOptionIsWritable(const IscCameraParameter option_name)
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
int XcSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, int* value)
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
int XcSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
		*value = 746;
		ret_value = DPC_E_OK;
		break;

	case IscCameraParameter::kGain:
		*value = 720;
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
int XcSdkWrapper::DeviceGetOptionInc(const IscCameraParameter option_name, int* value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, int* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = 0;

	unsigned int get_value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
		ret_value = GetExposureValue(&get_value);
		if (ret_value == ISC_OK) {
			*value = 748 - static_cast<int>(get_value);
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kGain:
		ret_value = GetGainValue(&get_value);
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
int XcSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const int value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;

	const unsigned int set_value = static_cast<unsigned int>(value);

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		break;

	case IscCameraParameter::kExposure:
	{
		const unsigned int exposure_value = MAX(2, (748 - set_value));
		ret_value = SetExposureValue(exposure_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_SET_FETURE_FAILED;
		}
	}
		break;

	case IscCameraParameter::kGain:
		ret_value = SetGainValue(set_value);
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
int XcSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, float* value)
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
int XcSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, float* value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, float* value)
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
int XcSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const float value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, bool* value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	*value = false;

	int get_value = 0;

	switch (option_name) {
	case IscCameraParameter::kAdjustAuto:
		ret_value = GetAutoCalibration(&get_value);
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

	case IscCameraParameter::kAdjustForce:
		ret_value = GetAutoCalibration(&get_value);
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
int XcSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const bool value)
{
	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	unsigned int set_value = 0;

	switch (option_name) {
	case IscCameraParameter::kAdjustAuto:
		if (value) {
			set_value = AUTOCALIBRATION_COMMAND_AUTO_ON;
		}
		else {
			set_value = AUTOCALIBRATION_COMMAND_STOP;
		}
		ret_value = SetAutoCalibration(set_value);
		if (ret_value == ISC_OK) {
			ret_value = DPC_E_OK;
		}
		else {
			ret_value = CAMCONTROL_E_GET_FETURE_FAILED;
		}
		break;

	case IscCameraParameter::kAdjustForce:
		if (value) {
			set_value = AUTOCALIBRATION_COMMAND_MANUAL_START;

			ret_value = SetAutoCalibration(set_value);
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
int XcSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length)
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
int XcSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const char* value)
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
int XcSdkWrapper::DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value)
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
int XcSdkWrapper::DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value)
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
int XcSdkWrapper::DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, uint64_t* value)
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
int XcSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const uint64_t value)
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
int XcSdkWrapper::DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value)
{
	*value = IscShutterMode::kManualShutter;

	int ret_value = CAMCONTROL_E_INVALID_REQUEST;
	int get_value = 0;

	switch (option_name) {
	case IscCameraParameter::kShutterMode:
		ret_value = GetShutterControlModeEx(&get_value);
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
int XcSdkWrapper::DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value)
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

		ret_value = SetShutterControlModeEx(set_value);
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

// grab control
/**
 * start image acquisition.
 *
 * @param[in] isc_grab_start_mode sets the operating mode.
 * @return 0 if successful.
 */
int XcSdkWrapper::Start(const IscGrabStartMode* isc_grab_start_mode)
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

	case IscGrabMode::kBayerBase:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;

	case IscGrabMode::kBayerCompare:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;

	default:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;
	}

	switch (isc_grab_start_mode->isc_grab_color_mode) {
	case IscGrabColorMode::kColorOFF:
		SetRGBEnabled(0);
		break;

	case IscGrabColorMode::kColorON:
		SetRGBEnabled(1);
		break;

	default:
		return CAMCONTROL_E_INVALID_REQUEST;
		break;
	}

	int camera_ret_value = StartGrab(start_mode);

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
int XcSdkWrapper::Stop()
{

	int ret_value = DPC_E_OK;

	int camera_ret_value = StopGrab();
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
int XcSdkWrapper::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
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
int XcSdkWrapper::InitializeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	int width = xc_camera_param_info_.image_width;
	int height = xc_camera_param_info_.image_height;

	isc_image_info->frameNo = -1;
	isc_image_info->gain = -1;
	isc_image_info->exposure = -1;

	isc_image_info->grab = IscGrabMode::kParallax;
	isc_image_info->color_grab_mode = IscGrabColorMode::kColorOFF;
	isc_image_info->shutter_mode = IscShutterMode::kManualShutter;
	isc_image_info->camera_specific_parameter.d_inf = xc_camera_param_info_.d_inf;
	isc_image_info->camera_specific_parameter.bf = xc_camera_param_info_.bf;
	isc_image_info->camera_specific_parameter.base_length = xc_camera_param_info_.base_length;
	isc_image_info->camera_specific_parameter.dz = xc_camera_param_info_.dz;

	isc_image_info->camera_status.error_code = ISC_OK;
	isc_image_info->camera_status.data_receive_tact_time = 0;

	isc_image_info->p1.width = 0;
	isc_image_info->p1.height = 0;
	isc_image_info->p1.channel_count = 0;
	isc_image_info->p1.image = new unsigned char[width * height];

	isc_image_info->p2.width = 0;
	isc_image_info->p2.height = 0;
	isc_image_info->p2.channel_count = 0;
	isc_image_info->p2.image = new unsigned char[width * height];

	isc_image_info->color.width = 0;
	isc_image_info->color.height = 0;
	isc_image_info->color.channel_count = 0;
	isc_image_info->color.image = new unsigned char[width * height * 4];

	isc_image_info->depth.width = 0;
	isc_image_info->depth.height = 0;
	isc_image_info->depth.image = new float[width * height];

	isc_image_info->raw.width = 0;
	isc_image_info->raw.height = 0;
	isc_image_info->raw.channel_count = 0;
	isc_image_info->raw.image = new unsigned char[width * height * 2];

	isc_image_info->bayer_base.width = 0;
	isc_image_info->bayer_base.height = 0;
	isc_image_info->bayer_base.channel_count = 0;
	isc_image_info->bayer_base.image = nullptr;

	isc_image_info->bayer_compare.width = 0;
	isc_image_info->bayer_compare.height = 0;
	isc_image_info->bayer_compare.channel_count = 0;
	isc_image_info->bayer_compare.image = nullptr;


	size_t image_size = width * height;
	memset(isc_image_info->p1.image, 0, image_size);
	memset(isc_image_info->p2.image, 0, image_size);

	image_size = width * height * sizeof(float);
	memset(isc_image_info->depth.image, 0, image_size);

	image_size = width * height * 2;
	memset(isc_image_info->raw.image, 0, image_size);

	return DPC_E_OK;
}

/**
 * release the allocated space.
 *
 * @param[in,out] isc_image_info is a structure to release.
 * @return 0 if successful.
 */
int XcSdkWrapper::ReleaeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	isc_image_info->frameNo = -1;
	isc_image_info->gain = -1;
	isc_image_info->exposure = -1;

	isc_image_info->grab = IscGrabMode::kParallax;
	isc_image_info->color_grab_mode = IscGrabColorMode::kColorOFF;
	isc_image_info->shutter_mode = IscShutterMode::kManualShutter;
	isc_image_info->camera_specific_parameter.d_inf = 0;
	isc_image_info->camera_specific_parameter.bf = 0;
	isc_image_info->camera_specific_parameter.base_length = 0;
	isc_image_info->camera_specific_parameter.dz = 0;

	isc_image_info->camera_status.error_code = ISC_OK;
	isc_image_info->camera_status.data_receive_tact_time = 0;

	isc_image_info->p1.width = 0;
	isc_image_info->p1.height = 0;
	isc_image_info->p1.channel_count = 0;
	delete[] isc_image_info->p1.image;
	isc_image_info->p1.image = nullptr;

	isc_image_info->p2.width = 0;
	isc_image_info->p2.height = 0;
	isc_image_info->p2.channel_count = 0;
	delete[] isc_image_info->p2.image;
	isc_image_info->p2.image = nullptr;

	isc_image_info->color.width = 0;
	isc_image_info->color.height = 0;
	isc_image_info->color.channel_count = 0;
	delete[] isc_image_info->color.image;
	isc_image_info->color.image = nullptr;

	isc_image_info->depth.width = 0;
	isc_image_info->depth.height = 0;
	delete[] isc_image_info->depth.image;
	isc_image_info->depth.image = nullptr;

	isc_image_info->raw.width = 0;
	isc_image_info->raw.height = 0;
	isc_image_info->raw.channel_count = 0;
	delete[] isc_image_info->raw.image;
	isc_image_info->raw.image = nullptr;

	isc_image_info->bayer_base.width = 0;
	isc_image_info->bayer_base.height = 0;
	isc_image_info->bayer_base.channel_count = 0;
	assert(isc_image_info->bayer_base.image == nullptr);
	isc_image_info->bayer_base.image = nullptr;

	isc_image_info->bayer_compare.width = 0;
	isc_image_info->bayer_compare.height = 0;
	isc_image_info->bayer_compare.channel_count = 0;
	assert(isc_image_info->bayer_compare.image == nullptr);
	isc_image_info->bayer_compare.image = nullptr;

	return DPC_E_OK;
}

/**
 * get captured data.
 *
 * @param[in] isc_get_mode a parameter that specifies the acquisition behavior.
 * @param[in,out] isc_image_info it is a structure to write the acquired data.
 * @return 0 if successful.
 */
int XcSdkWrapper::GetData(const IscGetMode* isc_get_mode, IscImageInfo* isc_image_info)
{
	isc_image_info->frameNo = -1;
	isc_image_info->gain = -1;
	isc_image_info->exposure = -1;

	isc_image_info->grab = isc_grab_start_mode_.isc_grab_mode;
	isc_image_info->color_grab_mode = isc_grab_start_mode_.isc_grab_color_mode;
	isc_image_info->shutter_mode = isc_shutter_mode_;
	isc_image_info->camera_specific_parameter.d_inf = xc_camera_param_info_.d_inf;
	isc_image_info->camera_specific_parameter.bf = xc_camera_param_info_.bf;
	isc_image_info->camera_specific_parameter.base_length = xc_camera_param_info_.base_length;
	isc_image_info->camera_specific_parameter.dz = xc_camera_param_info_.dz;

	isc_image_info->camera_status.error_code = ISC_OK;
	isc_image_info->camera_status.data_receive_tact_time = 0;

	isc_image_info->p1.width = 0;
	isc_image_info->p1.height = 0;
	isc_image_info->p1.channel_count = 0;

	isc_image_info->p2.width = 0;
	isc_image_info->p2.height = 0;
	isc_image_info->p2.channel_count = 0;

	isc_image_info->color.width = 0;
	isc_image_info->color.height = 0;
	isc_image_info->color.channel_count = 0;

	isc_image_info->depth.width = 0;
	isc_image_info->depth.height = 0;

	isc_image_info->raw.width = 0;
	isc_image_info->raw.height = 0;
	isc_image_info->raw.channel_count = 0;

	/*
		視差やモノクロ画像と、カラー画像は、本来は独立で取得できるケースがある。
		が、単純化のために　GetImageが成功した場合のみカラー画像を取得する

		モノクロ画像があって、カラーが無い場合は、前回の画像がそのままになる
		初回は不定になってる
	*/

	UtilityMeasureTime utility_measure_time_total;
	utility_measure_time_total.Init();

	UtilityMeasureTime utility_measure_time;
	utility_measure_time.Init();
	double elp_time[8] = {};
	char msg[128] = {};

	utility_measure_time_total.Start();
	if (isc_grab_start_mode_.isc_grab_color_mode == IscGrabColorMode::kColorON) {
		utility_measure_time.Start();
		int ret = GetYUVImageEx(work_buffer_.buffer[0], 0, isc_get_mode->wait_time);
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
		elp_time[7] = utility_measure_time.Stop();

		//sprintf_s(msg, "[GetData]GetYUVImage successful(%.1f)\n", elp_time[7]);
		//OutputDebugStringA(msg);

	}


	int ret = GetImageEx(isc_image_info_.p2.image, isc_image_info_.p1.image, 1, isc_get_mode->wait_time);
	isc_image_info->camera_status.error_code = ret;

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

	int width = xc_camera_param_info_.image_width;
	int height = xc_camera_param_info_.image_height;

	// RAW data
	// ToDo:このままだと、モノクロのみとなっている
	if (isc_grab_start_mode_.isc_get_raw_mode == IscGetModeRaw::kRawOn) {

		utility_measure_time.Start();

		ret = GetFullFrameInfo(isc_image_info->raw.image);
		if (ret != 0) {
			return CAMCONTROL_E_GET_FULL_FRAME_FAILED;
		}
		isc_image_info->raw.width = width * 2;
		isc_image_info->raw.height = height;
		isc_image_info->raw.channel_count = 1;

		// 反転不要です

		elp_time[0] = utility_measure_time.Stop();
	}

	// 他のカメラと画像の向きを統一させるために左右反転します
	constexpr bool is_flip_for_compatibility = true;

	if (is_flip_for_compatibility) {

		utility_measure_time.Start();
		// 基準側画像
		isc_image_info->p1.width = width;
		isc_image_info->p1.height = height;
		isc_image_info->p1.channel_count = 1;

		// 他のカメラとの互換性のために左右を反転します
		cv::Mat mat_src_image_p1(isc_image_info->p1.height, isc_image_info->p1.width, CV_8UC1, isc_image_info_.p1.image);
		cv::Mat mat_dst_image_p1(isc_image_info->p1.height, isc_image_info->p1.width, CV_8UC1, isc_image_info->p1.image);
		cv::flip(mat_src_image_p1, mat_dst_image_p1, 1);

		elp_time[1] = utility_measure_time.Stop();

		if (isc_grab_start_mode_.isc_grab_mode == IscGrabMode::kParallax) {
			utility_measure_time.Start();

			ret = GetDepthInfo(isc_image_info_.depth.image);
			if (ret != ISC_OK) {
				return CAMCONTROL_E_GET_DEPTH_FAILED;
			}
			isc_image_info->depth.width = width;
			isc_image_info->depth.height = height;

			// 他のカメラとの互換性のために左右を反転します
			cv::Mat mat_src_image_depth(isc_image_info->depth.height, isc_image_info->depth.width, CV_32FC1, isc_image_info_.depth.image);
			cv::Mat mat_dst_image_depth(isc_image_info->depth.height, isc_image_info->depth.width, CV_32FC1, isc_image_info->depth.image);
			cv::flip(mat_src_image_depth, mat_dst_image_depth, 1);

			elp_time[2] = utility_measure_time.Stop();
		}
		else {
			// 補正後比較画像/補正前比較画像
			isc_image_info->p2.width = width;
			isc_image_info->p2.height = height;
			isc_image_info->p2.channel_count = 1;

			// 他のカメラとの互換性のために左右を反転します
			cv::Mat mat_src_image_p2(isc_image_info->p2.height, isc_image_info->p2.width, CV_8UC1, isc_image_info_.p2.image);
			cv::Mat mat_dst_image_p2(isc_image_info->p2.height, isc_image_info->p2.width, CV_8UC1, isc_image_info->p2.image);
			cv::flip(mat_src_image_p2, mat_dst_image_p2, 1);
		}
	}
	else {
		// 基準側画像
		isc_image_info->p1.width = width;
		isc_image_info->p1.height = height;
		isc_image_info->p1.channel_count = 1;

		size_t cp_size = isc_image_info->p1.width * isc_image_info->p1.height;
		memcpy(isc_image_info->p1.image, isc_image_info_.p1.image, cp_size);

		if (isc_grab_start_mode_.isc_grab_mode == IscGrabMode::kParallax) {
			ret = GetDepthInfo(isc_image_info_.depth.image);
			if (ret != ISC_OK) {
				OutputDebugStringA("[GetData]GetDepthInfo Failed\n");
				return CAMCONTROL_E_GET_DEPTH_FAILED;
			}
			isc_image_info->depth.width = width;
			isc_image_info->depth.height = height;

			cp_size = isc_image_info->depth.width * isc_image_info->depth.height * sizeof(float);
			memcpy(isc_image_info->depth.image, isc_image_info_.depth.image, cp_size);
		}
		else {
			// 補正後比較画像/補正前比較画像
			isc_image_info->p2.width = width;
			isc_image_info->p2.height = height;
			isc_image_info->p2.channel_count = 1;

			cp_size = isc_image_info->p2.width * isc_image_info->p2.height;
			memcpy(isc_image_info->p2.image, isc_image_info_.p2.image, cp_size);
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

		if (isc_grab_start_mode_.isc_get_color_mode == IscGetModeColor::kYuv) {

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 1;

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				//size_t cp_size = isc_image_info->color.width * isc_image_info->color.height * isc_image_info->color.channel_count * 2;
				//memcpy(isc_image_info->color.image, work_buffer_.buffer[0], cp_size);

				unsigned char temp_yuv[4] = {};
				int line_step = width * 2;

				for (int y = 0; y < height; y++) {
					unsigned char* src = work_buffer_.buffer[0] + (y * line_step) + (line_step - 4);
					unsigned char* dst = isc_image_info->color.image + (y * line_step);
					for (int x = 0; x < width / 2; x++) {
						//temp_yuv[0] = *src++;
						//temp_yuv[1] = *src++;
						//temp_yuv[2] = *src++;
						//temp_yuv[3] = *src++;
						//src -= 8;
						*((unsigned int*)&temp_yuv[0]) = *((unsigned int*)src);
						src -= 4;

						unsigned int temp_Value = (unsigned int)(temp_yuv[3] << 24 | temp_yuv[0] << 16 | temp_yuv[1] << 8 | temp_yuv[2]);
						*((unsigned int*)dst) = temp_Value;
						dst += 4;
					}
				}
			}
			else {
				size_t cp_size = isc_image_info->color.width * isc_image_info->color.height * isc_image_info->color.channel_count * 2;
				memcpy(isc_image_info->color.image, work_buffer_.buffer[0], cp_size);
			}
		}
		else if (isc_grab_start_mode_.isc_get_color_mode == IscGetModeColor::kBGR) {

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 3;

			int size = width * height * 2;
			ConvertYUVToRGB(work_buffer_.buffer[0], work_buffer_.buffer[1], width* height * 2);

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				cv::Mat mat_src_color_image(height, width, CV_8UC3, work_buffer_.buffer[1]);
				cv::Mat mat_dst_color_image(height, width, CV_8UC3, isc_image_info->color.image);
				cv::flip(mat_src_color_image, mat_dst_color_image, 1);
			}
			else {
				size_t cp_size = width * height * isc_image_info->color.channel_count;
				memcpy(isc_image_info->color.image, work_buffer_.buffer[1], cp_size);
			}
		}
		else if (isc_grab_start_mode_.isc_get_color_mode == IscGetModeColor::kCorrect) {
			ret = GetYUVImage(work_buffer_.buffer[0], 0);
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

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 3;

			int size = width * height * 2;
			ConvertYUVToRGB(work_buffer_.buffer[0], work_buffer_.buffer[1], size);

			// correct
			CorrectRGBImage(work_buffer_.buffer[1], work_buffer_.buffer[2]);

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				cv::Mat mat_src_color_image(height, width, CV_8UC3, work_buffer_.buffer[2]);
				cv::Mat mat_dst_color_image(height, width, CV_8UC3, isc_image_info->color.image);
				cv::flip(mat_src_color_image, mat_dst_color_image, 1);
			}
			else {
				size_t cp_size = width * height * isc_image_info->color.channel_count;
				memcpy(isc_image_info->color.image, work_buffer_.buffer[2], cp_size);
			}
		}
		else if (isc_grab_start_mode_.isc_get_color_mode == IscGetModeColor::kAwb) {

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 3;

			int size = width * height * 2;
			ConvertYUVToRGB(work_buffer_.buffer[0], work_buffer_.buffer[1], size);

			// correct
			CorrectRGBImage(work_buffer_.buffer[1], work_buffer_.buffer[2]);

			// auto white balance
			ApplyAutoWhiteBalance(work_buffer_.buffer[2], work_buffer_.buffer[3]);

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				cv::Mat mat_src_color_image(height, width, CV_8UC3, work_buffer_.buffer[3]);
				cv::Mat mat_dst_color_image(height, width, CV_8UC3, isc_image_info->color.image);
				cv::flip(mat_src_color_image, mat_dst_color_image, 1);
			}
			else {
				size_t cp_size = width * height * isc_image_info->color.channel_count;
				memcpy(isc_image_info->color.image, work_buffer_.buffer[3], cp_size);
			}
		}
		else if (isc_grab_start_mode_.isc_get_color_mode == IscGetModeColor::kAwbNoCorrect) {

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 3;

			int size = width * height * 2;
			ConvertYUVToRGB(work_buffer_.buffer[0], work_buffer_.buffer[1], size);

			// auto white balance
			ApplyAutoWhiteBalance(work_buffer_.buffer[1], work_buffer_.buffer[2]);

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				cv::Mat mat_src_color_image(height, width, CV_8UC3, work_buffer_.buffer[2]);
				cv::Mat mat_dst_color_image(height, width, CV_8UC3, isc_image_info->color.image);
				cv::flip(mat_src_color_image, mat_dst_color_image, 1);
			}
			else {
				size_t cp_size = width * height * isc_image_info->color.channel_count;
				memcpy(isc_image_info->color.image, work_buffer_.buffer[2], cp_size);
			}
		}

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
int XcSdkWrapper::Decode(const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode, const IscGetModeColor isc_get_color_mode,
	const int width, const int height, IscImageInfo* isc_image_info)
{

	constexpr bool is_flip_for_compatibility = true;
	int ret = DPC_E_OK;

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

		size_t cp_size = width * height * 2;
		memcpy(decode_buffer_.work_buffer.buffer[0], isc_image_info->raw.image, cp_size);

		if (isc_get_color_mode == IscGetModeColor::kYuv) {

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 1;

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				//size_t cp_size = isc_image_info->color.width * isc_image_info->color.height * isc_image_info->color.channel_count * 2;
				//memcpy(isc_image_info->color.image, decode_buffer_.work_buffer.buffer[0], cp_size);

				unsigned char temp_yuv[4] = {};
				int line_step = width * 2;

				for (int y = 0; y < height; y++) {
					unsigned char* src = decode_buffer_.work_buffer.buffer[0] + (y * line_step) + (line_step - 4);
					unsigned char* dst = isc_image_info->color.image + (y * line_step);
					for (int x = 0; x < width / 2; x++) {
						//temp_yuv[0] = *src++;
						//temp_yuv[1] = *src++;
						//temp_yuv[2] = *src++;
						//temp_yuv[3] = *src++;
						//src -= 8;
						*((unsigned int*)&temp_yuv[0]) = *((unsigned int*)src);
						src -= 4;

						unsigned int temp_Value = (unsigned int)(temp_yuv[3] << 24 | temp_yuv[0] << 16 | temp_yuv[1] << 8 | temp_yuv[2]);
						*((unsigned int*)dst) = temp_Value;
						dst += 4;
					}
				}
			}
			else {
				size_t cp_size = isc_image_info->color.width * isc_image_info->color.height * isc_image_info->color.channel_count * 2;
				memcpy(isc_image_info->color.image, decode_buffer_.work_buffer.buffer[0], cp_size);
			}
		}
		else if (isc_get_color_mode == IscGetModeColor::kBGR) {

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 3;

			int size = width * height * 2;
			ConvertYUVToRGB(decode_buffer_.work_buffer.buffer[0], decode_buffer_.work_buffer.buffer[1], width * height * 2);

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				cv::Mat mat_src_color_image(height, width, CV_8UC3, decode_buffer_.work_buffer.buffer[1]);
				cv::Mat mat_dst_color_image(height, width, CV_8UC3, isc_image_info->color.image);
				cv::flip(mat_src_color_image, mat_dst_color_image, 1);
			}
			else {
				size_t cp_size = width * height * isc_image_info->color.channel_count;
				memcpy(isc_image_info->color.image, decode_buffer_.work_buffer.buffer[1], cp_size);
			}
		}
		else if (isc_get_color_mode == IscGetModeColor::kCorrect) {
			ret = GetYUVImage(decode_buffer_.work_buffer.buffer[0], 0);
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

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 3;

			int size = width * height * 2;
			ConvertYUVToRGB(decode_buffer_.work_buffer.buffer[0], decode_buffer_.work_buffer.buffer[1], size);

			// correct
			CorrectRGBImage(decode_buffer_.work_buffer.buffer[1], decode_buffer_.work_buffer.buffer[2]);

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				cv::Mat mat_src_color_image(height, width, CV_8UC3, decode_buffer_.work_buffer.buffer[2]);
				cv::Mat mat_dst_color_image(height, width, CV_8UC3, isc_image_info->color.image);
				cv::flip(mat_src_color_image, mat_dst_color_image, 1);
			}
			else {
				size_t cp_size = width * height * isc_image_info->color.channel_count;
				memcpy(isc_image_info->color.image, decode_buffer_.work_buffer.buffer[2], cp_size);
			}
		}
		else if (isc_get_color_mode == IscGetModeColor::kAwb) {

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 3;

			int size = width * height * 2;
			ConvertYUVToRGB(decode_buffer_.work_buffer.buffer[0], decode_buffer_.work_buffer.buffer[1], size);

			// correct
			CorrectRGBImage(decode_buffer_.work_buffer.buffer[1], decode_buffer_.work_buffer.buffer[2]);

			// auto white balance
			ApplyAutoWhiteBalance(decode_buffer_.work_buffer.buffer[2], decode_buffer_.work_buffer.buffer[3]);

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				cv::Mat mat_src_color_image(height, width, CV_8UC3, decode_buffer_.work_buffer.buffer[3]);
				cv::Mat mat_dst_color_image(height, width, CV_8UC3, isc_image_info->color.image);
				cv::flip(mat_src_color_image, mat_dst_color_image, 1);
			}
			else {
				size_t cp_size = width * height * isc_image_info->color.channel_count;
				memcpy(isc_image_info->color.image, decode_buffer_.work_buffer.buffer[3], cp_size);
			}
		}
		else if (isc_get_color_mode == IscGetModeColor::kAwbNoCorrect) {

			isc_image_info->color.width = width;
			isc_image_info->color.height = height;
			isc_image_info->color.channel_count = 3;

			int size = width * height * 2;
			ConvertYUVToRGB(decode_buffer_.work_buffer.buffer[0], decode_buffer_.work_buffer.buffer[1], size);

			// auto white balance
			ApplyAutoWhiteBalance(decode_buffer_.work_buffer.buffer[1], decode_buffer_.work_buffer.buffer[2]);

			if (is_flip_for_compatibility) {
				// 他のカメラとの互換性のために左右を反転します
				cv::Mat mat_src_color_image(height, width, CV_8UC3, decode_buffer_.work_buffer.buffer[2]);
				cv::Mat mat_dst_color_image(height, width, CV_8UC3, isc_image_info->color.image);
				cv::flip(mat_src_color_image, mat_dst_color_image, 1);
			}
			else {
				size_t cp_size = width * height * isc_image_info->color.channel_count;
				memcpy(isc_image_info->color.image, decode_buffer_.work_buffer.buffer[2], cp_size);
			}
		}
	}
	else {
		// 画像分割処理を行います
		const bool is_disparity = (isc_grab_mode == IscGrabMode::kParallax) ? true : false;
		int ret = SplitImage(is_disparity, width, height, isc_image_info->raw.image, decode_buffer_.split_images[0], decode_buffer_.split_images[1], decode_buffer_.split_images[2]);
		if (ret != DPC_E_OK) {
			return ret;
		}

		switch (isc_grab_mode) {
		case IscGrabMode::kParallax:
			// mask情報を使った視差情報にします
			ReCreateParallaxImage(width, height, decode_buffer_.split_images[2], decode_buffer_.disparity, decode_buffer_.disparity_image, decode_buffer_.mask_image);
			//memcpy(isc_image_info->depth.image, decode_buffer_.disparity, width * height * sizeof(float));
			// 他のカメラとの互換性のために左右を反転します
			isc_image_info->depth.width = width;
			isc_image_info->depth.height = height;
			{
				cv::Mat mat_src_image_depth(isc_image_info->depth.height, isc_image_info->depth.width, CV_32FC1, decode_buffer_.disparity);
				cv::Mat mat_dst_image_depth(isc_image_info->depth.height, isc_image_info->depth.width, CV_32FC1, isc_image_info->depth.image);
				cv::flip(mat_src_image_depth, mat_dst_image_depth, 1);
			}

			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[1], isc_image_info->p1.image);
			memcpy(isc_image_info->p1.image, decode_buffer_.split_images[1], width * height);
			isc_image_info->p1.width = width;
			isc_image_info->p1.height = height;
			isc_image_info->p1.channel_count = 1;
			break;

		case IscGrabMode::kCorrect:
			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[1], isc_image_info->p1.image);
			memcpy(isc_image_info->p1.image, decode_buffer_.split_images[1], width * height);
			isc_image_info->p1.width = width;
			isc_image_info->p1.height = height;
			isc_image_info->p1.channel_count = 1;

			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[0], isc_image_info->p2.image);
			memcpy(isc_image_info->p2.image, decode_buffer_.split_images[0], width * height);
			isc_image_info->p2.width = width;
			isc_image_info->p2.height = height;
			isc_image_info->p2.channel_count = 1;

			break;

		case IscGrabMode::kBeforeCorrect:
			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[1], isc_image_info->p1.image);
			memcpy(isc_image_info->p1.image, decode_buffer_.split_images[1], width * height);
			isc_image_info->p1.width = width;
			isc_image_info->p1.height = height;
			isc_image_info->p1.channel_count = 1;

			// 画像を左右を反転します -> このライブラリでは反転しない
			//FlipImage(width, height, decode_buffer_.split_images[0], isc_image_info->p2.image);
			memcpy(isc_image_info->p2.image, decode_buffer_.split_images[0], width * height);
			isc_image_info->p2.width = width;
			isc_image_info->p2.height = height;
			isc_image_info->p2.channel_count = 1;

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
 * @param[in,out] image1 base image / disparity.
 * @param[in,out] image2 compare image / mask data.
 * @param[in,out] image3 disparity.
 * @return 0 if successful.
 */
int XcSdkWrapper::SplitImage(const bool is_disparity, const int width, const int height, unsigned char* raw_data, unsigned char* image1, unsigned char* image2, unsigned char* image3)
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
int XcSdkWrapper::ReCreateParallaxImage(const int width, const int height, unsigned char* src_data, float* temp_disparity, unsigned char* dst_image, unsigned char* mask_image)
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
int XcSdkWrapper::FlipImage(const int width, const int height, unsigned char* src_image, unsigned char* dst_image)
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
int XcSdkWrapper::SetStereoMatchingsPeculiarRemoval(const int value)
{
	constexpr int USB_WRITE_CMD_SIZE = 8;
	unsigned char wbuf[USB_WRITE_CMD_SIZE] = { };

	wbuf[0] = 0xF0;
	wbuf[1] = 0x00;
	wbuf[2] = 0x82;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	if (value == 0) {
		wbuf[4] = 0x00;
	}
	else {
		wbuf[4] = 0x01;
	}

	// コマンドを送ります
	int ret = SetCameraRegData(wbuf, USB_WRITE_CMD_SIZE);

	return ret;
}

/**
 * Get peculiar removal regiser.
 *
 * @param[out] Value read from camera .
 * @return 0 if successful.
 */
int XcSdkWrapper::GetStereoMatchingsPeculiarRemoval(int* value)
{
	constexpr int usb_write_cmd_size = 8;
	unsigned char wbuf[usb_write_cmd_size] = { };
	constexpr int usb_read_data_size = 8;
	unsigned char rbuf[usb_read_data_size] = { };

	wbuf[0] = 0xF1;
	wbuf[1] = 0x00;
	wbuf[2] = 0x82;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	int ret = GetCameraRegData(wbuf, rbuf, usb_write_cmd_size, usb_read_data_size);

	*value = (int)rbuf[7];

	return ret;
}

/**
 * Set occlusion removal on/off.
 *
 * @param[in] value 0:off 1:on.
 * @return 0 if successful.
 */
int XcSdkWrapper::SetStereoMatchingsOcclusionRemoval(const unsigned int value)
{

	constexpr int USB_WRITE_CMD_SIZE = 8;
	unsigned char wbuf[USB_WRITE_CMD_SIZE] = { };

	wbuf[0] = 0xF0;
	wbuf[1] = 0x00;
	wbuf[2] = 0x81;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	if (value == 0) {
		wbuf[4] = 0x00;
	}
	else {
		wbuf[4] = 0x01;
	}

	// コマンドを送ります
	int ret = SetCameraRegData(wbuf, USB_WRITE_CMD_SIZE);

	return ret;
}

/**
 * Get occlusion removal regiser.
 *
 * @param[out] Value read from camera 0:off 1:on.
 * @return 0 if successful.
 */
int XcSdkWrapper::GetStereoMatchingsOcclusionRemoval(unsigned int* value)
{

	constexpr int usb_write_cmd_size = 8;
	unsigned char wbuf[usb_write_cmd_size] = { };
	constexpr int usb_read_data_size = 8;
	unsigned char rbuf[usb_read_data_size] = { };

	wbuf[0] = 0xF1;
	wbuf[1] = 0x00;
	wbuf[2] = 0x81;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	int ret = GetCameraRegData(wbuf, rbuf, usb_write_cmd_size, usb_read_data_size);

	*value = rbuf[7];

	return ret;
}


} /* namespace ns_vmsdk_wrapper */
