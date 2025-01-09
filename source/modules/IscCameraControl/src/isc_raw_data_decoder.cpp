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
 * @file isc_raw_data_decoder.cpp
 * @brief implementation class of camera cntrol
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 *
 * @details This class provides decode function.
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <process.h>
#include <DbgHelp.h>
#include <Shlwapi.h>
#include <Shlobj.h>

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"
#include "vm_sdk_wrapper.h"
#include "xc_sdk_wrapper.h"
#include "k4a_sdk_wrapper.h"

#include "isc_raw_data_decoder.h"


/**
 * constructor
 *
 */
RawDataDecoder::RawDataDecoder():
	vm_sdk_wrapper_(nullptr), xc_sdk_wrapper_(nullptr)
{

}

/**
 * destructor
 *
 */
RawDataDecoder::~RawDataDecoder()
{

}

/**
 * クラスを初期化し書き込み先を確保します.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int RawDataDecoder::Initialize()
{
	vm_sdk_wrapper_ = new ns_vmsdk_wrapper::VmSdkWrapper;
	vm_sdk_wrapper_->Initialize();

	xc_sdk_wrapper_ = new ns_xcsdk_wrapper::XcSdkWrapper;
	xc_sdk_wrapper_->Initialize();

	return DPC_E_OK;
}

 /**
  * 終了処理をします.
  *
  * @retval 0 成功
  * @retval other 失敗
  */
int RawDataDecoder::Terminate()
{

	if (xc_sdk_wrapper_ != nullptr) {
		xc_sdk_wrapper_->Terminate();
		delete xc_sdk_wrapper_;
		xc_sdk_wrapper_ = nullptr;
	}

	if (vm_sdk_wrapper_ != nullptr) {
		vm_sdk_wrapper_->Terminate();
		delete vm_sdk_wrapper_;
		vm_sdk_wrapper_ = nullptr;
	}

	return DPC_E_OK;
}

 /**
  * Decode処理を呼び出します.
  *
  * param[in] isc_camera_model カメラｎモデルを指定します
  * param[in] isc_grab_mode 現在の取り込みモードを指定します
  * param[in] isc_grab_color_mode カラーモードのOn/Offを指定します
  * param[in] isc_get_color_mode カラーデータを取得するかどうかを指定します
  * param[in] width データの幅を指定します
  * param[in] height データの高さを指定します
  * param[in/out] isc_image_info RAWデータを受け取り、結果を書き込みます
  * @retval 0 成功
  * @retval other 失敗
  */
int RawDataDecoder::Decode(const IscCameraModel isc_camera_model, const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode, const IscGetModeColor isc_get_color_mode,
							const int width, const int height, IscImageInfo* isc_image_info, int frame_data_index)
{

	int ret = DPC_E_OK;

	switch (isc_camera_model) {
	case IscCameraModel::kVM:
		ret = DecodeVM(isc_grab_mode, isc_grab_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
		break;

	case IscCameraModel::kXC:
		ret = DecodeXC(isc_grab_mode, isc_grab_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::kUnknown:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return ret;
}

/**
 * Double Shutter用の「画像合成を呼び出します.
 *
 * param[in] isc_camera_model カメラｎモデルを指定します
 * param[in] width データの幅を指定します
 * param[in] height データの高さを指定します
 * param[in/out] isc_image_info RAWデータを受け取り、結果を書き込みます
 * @retval 0 成功
 * @retval other 失敗
 */
int RawDataDecoder::CombineImagesForDoubleShutter(const IscCameraModel isc_camera_model, const int width, const int height, IscImageInfo* isc_image_info)
{
	int ret = DPC_E_OK;

	switch (isc_camera_model) {
	case IscCameraModel::kVM:
		break;

	case IscCameraModel::kXC:
		ret = CombineImagesForDoubleShutterXC(width, height, isc_image_info);
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::kUnknown:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return ret;
}

/**
  * VMのDecode処理を呼び出します.
  *
  * param[in] isc_camera_model カメラｎモデルを指定します
  * param[in] isc_grab_mode 現在の取り込みモードを指定します
  * param[in] isc_grab_color_mode カラーモードのOn/Offを指定します
  * param[in] isc_get_color_mode カラーデータを取得するかどうかを指定します
  * param[in] width データの幅を指定します
  * param[in] height データの高さを指定します
  * param[in/out] isc_image_info RAWデータを受け取り、結果を書き込みます
  * @retval 0 成功
  * @retval other 失敗
  */
int RawDataDecoder::DecodeVM(const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode, const IscGetModeColor isc_get_color_mode,
							const int width, const int height, IscImageInfo* isc_image_info, int frame_data_index)
{

	if (vm_sdk_wrapper_ == nullptr) {
		return CAMCONTROL_E_INVALID_REQUEST;
	}

	int ret = vm_sdk_wrapper_->Decode(isc_grab_mode, isc_grab_color_mode, width, height, isc_image_info);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

 /**
   * XCのDecode処理を呼び出します.
   *
   * param[in] isc_camera_model カメラｎモデルを指定します
   * param[in] isc_grab_mode 現在の取り込みモードを指定します
   * param[in] isc_grab_color_mode カラーモードのOn/Offを指定します
   * param[in] isc_get_color_mode カラーデータを取得するかどうかを指定します
   * param[in] width データの幅を指定します
   * param[in] height データの高さを指定します
   * param[in/out] isc_image_info RAWデータを受け取り、結果を書き込みます
   * @retval 0 成功
   * @retval other 失敗
   */
int RawDataDecoder::DecodeXC(const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode, const IscGetModeColor isc_get_color_mode,
							const int width, const int height, IscImageInfo* isc_image_info, int frame_data_index)
{
	if (xc_sdk_wrapper_ == nullptr) {
		return CAMCONTROL_E_INVALID_REQUEST;
	}

	int ret = xc_sdk_wrapper_->Decode(isc_grab_mode, isc_grab_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * XCのDouble Shutter用の「画像合成を呼び出します.
 *
 * param[in] isc_camera_model カメラｎモデルを指定します
 * param[in] width データの幅を指定します
 * param[in] height データの高さを指定します
 * param[in/out] isc_image_info RAWデータを受け取り、結果を書き込みます
 * @retval 0 成功
 * @retval other 失敗
 */
int RawDataDecoder::CombineImagesForDoubleShutterXC(const int width, const int height, IscImageInfo* isc_image_info)
{
	if (xc_sdk_wrapper_ == nullptr) {
		return CAMCONTROL_E_INVALID_REQUEST;
	}

	int ret = xc_sdk_wrapper_->RunComposition(width, height, isc_image_info);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}
