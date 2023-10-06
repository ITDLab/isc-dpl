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
 * @file isc_sdk_control.cpp
 * @brief interface class for SDK Wrapper
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides an interface with the SDK Wrapper Class.
 * @note ISC100VM SDK 2.0.0~
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"

#include "vm_sdk_wrapper.h"
#include "xc_sdk_wrapper.h"
#include "k4a_sdk_wrapper.h"

#include "isc_sdk_control.h"


/**
 * constructor
 *
 */
IscSdkControl::IscSdkControl():
	isc_camera_model_(IscCameraModel::kUnknown),
	vm_sdk_wrapper_(nullptr), xc_sdk_wrapper_(nullptr), k4a_sdk_wrapper_(nullptr)
{

}

/**
 * destructor
 *
 */
IscSdkControl::~IscSdkControl()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] isc_camera_model 指定カメラモデル
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::Initialize(const IscCameraModel isc_camera_model)
{

	isc_camera_model_ = isc_camera_model;

	int ret = DPC_E_OK;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		vm_sdk_wrapper_ = new ns_vmsdk_wrapper::VmSdkWrapper;
		ret = vm_sdk_wrapper_->Initialize();
		break;

	case IscCameraModel::kXC:
		xc_sdk_wrapper_ = new ns_xcsdk_wrapper::XcSdkWrapper;
		ret = xc_sdk_wrapper_->Initialize();
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		k4a_sdk_wrapper_ = new ns_k4asdk_wrapper::K4aSdkWrapper;
		ret = k4a_sdk_wrapper_->Initialize();
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return ret;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::Terminate()
{
	int ret = DPC_E_OK;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			ret = vm_sdk_wrapper_->Terminate();
			delete vm_sdk_wrapper_;
			vm_sdk_wrapper_ = nullptr;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			ret = xc_sdk_wrapper_->Terminate();
			delete xc_sdk_wrapper_;
			xc_sdk_wrapper_ = nullptr;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			ret = k4a_sdk_wrapper_->Terminate();
			delete k4a_sdk_wrapper_;
			k4a_sdk_wrapper_ = nullptr;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return ret;
}

/**
 * カメラとの接続を行います.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceOpen()
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceOpen();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceOpen();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceOpen();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * カメラとの接続を切断します
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceClose()
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceClose();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceClose();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceClose();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

// camera dependent paraneter

/**
 * Work Bufferの推奨数を戻します
 *
 * @param[out] buffer_count Buffer数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::GetRecommendedBufferCount(int* buffer_count)
{
	*buffer_count = 8;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		*buffer_count = 16;
		break;

	case IscCameraModel::kXC:
		*buffer_count = 16;
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		*buffer_count = 4;
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 機能が実装されているかどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 実装されています
 * @retval false 実装されていません
 */
bool IscSdkControl::DeviceOptionIsImplemented(const IscCameraInfo option_name)
{

	bool ret_value = false;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			ret_value = vm_sdk_wrapper_->DeviceOptionIsImplemented(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			ret_value = xc_sdk_wrapper_->DeviceOptionIsImplemented(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4K:
		return false;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			ret_value = k4a_sdk_wrapper_->DeviceOptionIsImplemented(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4KJ:
		return false;
		//break;

	default:
		return false;
		//break;
	}

	return ret_value;
}

/**
 * 値を取得可能かどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 読み込み可能です
 * @retval false 読み込みはできません
 */
bool IscSdkControl::DeviceOptionIsReadable(const IscCameraInfo option_name)
{
	bool ret_value = false;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			ret_value = vm_sdk_wrapper_->DeviceOptionIsReadable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			ret_value = xc_sdk_wrapper_->DeviceOptionIsReadable(option_name);
		}
		else {
			return false;
		}
		break;


	case IscCameraModel::k4K:
		return false;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			ret_value = k4a_sdk_wrapper_->DeviceOptionIsReadable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4KJ:
		return false;
		//break;

	default:
		return false;
		//break;
	}

	return ret_value;
}

/**
 * 値を書き込み可能かどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 書き込み可能です
 * @retval false 書き込みはできません
 */
bool IscSdkControl::DeviceOptionIsWritable(const IscCameraInfo option_name)
{
	bool ret_value = false;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			ret_value = vm_sdk_wrapper_->DeviceOptionIsWritable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			ret_value = xc_sdk_wrapper_->DeviceOptionIsWritable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4K:
		return false;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			ret_value = k4a_sdk_wrapper_->DeviceOptionIsWritable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4KJ:
		return false;
		//break;

	default:
		return false;
		//break;
	}

	return ret_value;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMin(const IscCameraInfo option_name, int* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMax(const IscCameraInfo option_name, int* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な増減値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionInc(const IscCameraInfo option_name, int* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraInfo option_name, int* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/int)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraInfo option_name, const int value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMin(const IscCameraInfo option_name, float* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMax(const IscCameraInfo option_name, float* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraInfo option_name, float* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/float)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraInfo option_name, const float value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/bool)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraInfo option_name, bool* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/bool)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraInfo option_name, const bool value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/char)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[in] max_length valueバッファーの最大文字数
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value, max_length);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value, max_length);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value, max_length);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/char)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraInfo option_name, const char* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な増減値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraInfo option_name, uint64_t* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/uint64_t)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraInfo option_name, const uint64_t value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}


// camera control parameter

/**
 * 機能が実装されているかどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 実装されています
 * @retval false 実装されていません
 */
bool IscSdkControl::DeviceOptionIsImplemented(const IscCameraParameter option_name)
{

	bool ret_value = false;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			ret_value = vm_sdk_wrapper_->DeviceOptionIsImplemented(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			ret_value = xc_sdk_wrapper_->DeviceOptionIsImplemented(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4K:
		return false;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			ret_value = k4a_sdk_wrapper_->DeviceOptionIsImplemented(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4KJ:
		return false;
		//break;

	default:
		return false;
		//break;
	}

	return ret_value;
}

/**
 * 値を取得可能かどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 読み込み可能です
 * @retval false 読み込みはできません
 */
bool IscSdkControl::DeviceOptionIsReadable(const IscCameraParameter option_name)
{

	bool ret_value = false;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			ret_value = vm_sdk_wrapper_->DeviceOptionIsReadable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			ret_value = xc_sdk_wrapper_->DeviceOptionIsReadable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4K:
		return false;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			ret_value = k4a_sdk_wrapper_->DeviceOptionIsReadable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4KJ:
		return false;
		//break;

	default:
		return false;
		//break;
	}

	return ret_value;
}

/**
 * 値を書き込み可能かどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 書き込み可能です
 * @retval false 書き込みはできません
 */
bool IscSdkControl::DeviceOptionIsWritable(const IscCameraParameter option_name)
{

	bool ret_value = false;

	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			ret_value = vm_sdk_wrapper_->DeviceOptionIsWritable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			ret_value = xc_sdk_wrapper_->DeviceOptionIsWritable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4K:
		return false;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			ret_value = k4a_sdk_wrapper_->DeviceOptionIsWritable(option_name);
		}
		else {
			return false;
		}
		break;

	case IscCameraModel::k4KJ:
		return false;
		//break;

	default:
		return false;
		//break;
	}

	return ret_value;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMin(const IscCameraParameter option_name, int* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMax(const IscCameraParameter option_name, int* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な増減値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionInc(const IscCameraParameter option_name, int* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraParameter option_name, int* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/int)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraParameter option_name, const int value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMin(const IscCameraParameter option_name, float* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMax(const IscCameraParameter option_name, float* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraParameter option_name, float* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/float)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraParameter option_name, const float value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/bool)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraParameter option_name, bool* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/bool)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraParameter option_name, const bool value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/char)
 * 
 * @param[in] option_name 確認する機能の名前
 * @param[in] max_length valueバッファーの最大文字数
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value, max_length);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value, max_length);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value, max_length);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/char)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraParameter option_name, const char* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMin(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionMax(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な増減値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOptionInc(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraParameter option_name, uint64_t* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		return CAMCONTROL_E_INVALID_PARAMETER;
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;
	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/uint64_t)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraParameter option_name, const uint64_t value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/IscShutterMode)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/IscShutterMode)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, value);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(汎用アクセス)
 *
 * @param[in] option_name target parameter.
 * @param[int] write_value obtained value.
 * @param[int] write_size obtained value.
 * @param[out] read_value obtained value.
 * @param[int] read_size obtained value.
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceGetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size, unsigned char* read_value, const int read_size)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceGetOption(option_name, write_value, write_size, read_value, read_size);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceGetOption(option_name, write_value, write_size, read_value, read_size);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceGetOption(option_name, write_value, write_size, read_value, read_size);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(汎用アクセス)
 *
 * @param[in] option_name target parameter.
 * @param[in] write_value value to set.
 * @param[in] write_size value to set.
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::DeviceSetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->DeviceSetOption(option_name, write_value, write_size);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->DeviceSetOption(option_name, write_value, write_size);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->DeviceSetOption(option_name, write_value, write_size);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

// grab control

/**
 * 取り込みを開始します
 *
 * @param[in] isc_grab_start_mode 開始のためのパラメータ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::Start(const IscGrabStartMode* isc_grab_start_mode)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->Start(isc_grab_start_mode);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->Start(isc_grab_start_mode);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->Start(isc_grab_start_mode);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 取り込みを停止します
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::Stop()
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->Stop();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->Stop();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->Stop();
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * 現在の動作モードを取得します
 *
 * @param[in] isc_grab_start_mode 現在のパラメータ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->GetGrabMode(isc_grab_start_mode);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->GetGrabMode(isc_grab_start_mode);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->GetGrabMode(isc_grab_start_mode);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}


// image & data get

/**
 * データ取得のためのバッファーを初期化します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::InitializeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->InitializeIscIamgeinfo(isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->InitializeIscIamgeinfo(isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->InitializeIscIamgeinfo(isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * データ取得のためのバッファーを解放します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::ReleaeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->ReleaeIscIamgeinfo(isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->ReleaeIscIamgeinfo(isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->ReleaeIscIamgeinfo(isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}

/**
 * カメラ又はファイルよりデータを取得します
 *
 * @param[in] isc_get_mode データ取得モード設定
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSdkControl::GetData(const IscGetMode* isc_get_mode, IscImageInfo* isc_image_info)
{
	switch (isc_camera_model_) {
	case IscCameraModel::kVM:
		if (vm_sdk_wrapper_ != nullptr) {
			int ret = vm_sdk_wrapper_->GetData(isc_get_mode, isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::kXC:
		if (xc_sdk_wrapper_ != nullptr) {
			int ret = xc_sdk_wrapper_->GetData(isc_get_mode, isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4K:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	case IscCameraModel::k4KA:
		if (k4a_sdk_wrapper_ != nullptr) {
			int ret = k4a_sdk_wrapper_->GetData(isc_get_mode, isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		break;

	case IscCameraModel::k4KJ:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;

	default:
		return CAMCONTROL_E_INVALID_PARAMETER;
		//break;
	}

	return DPC_E_OK;
}
