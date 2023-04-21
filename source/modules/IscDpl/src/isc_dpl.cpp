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
 * @file isc_dpl.cpp
 * @brief main control class for ISC_DPL
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a function for ISC_DPL.
 */

#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <stdint.h>
#include <process.h>
#include <mutex>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_log.h"
#include "utility.h"

#include "isc_image_info_ring_buffer.h"
#include "vm_sdk_wrapper.h"
#include "xc_sdk_wrapper.h"
#include "isc_sdk_control.h"
#include "isc_file_write_control_impl.h"
#include "isc_raw_data_decoder.h"
#include "isc_file_read_control_impl.h"
#include "isc_camera_control.h"
#include "isc_dataproc_resultdata_ring_buffer.h"
#include "isc_framedecoder_interface.h"
#include "isc_blockmatching_interface.h"
#include "isc_data_processing_control.h"
#include "isc_main_control_impl.h"
#include "isc_main_control.h"

#include "isc_dpl.h"

#pragma comment (lib, "IscDplMainControl")

namespace ns_isc_dpl {


IscMainControl* isc_main_control_ = nullptr;

/**
 * constructor
 *
 */
IscDpl::IscDpl()
{
	isc_main_control_ = nullptr;
}

/**
 * destructor
 *
 */
IscDpl::~IscDpl()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] ipc_dpl_configuratio 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::Initialize(const IscDplConfiguration* ipc_dpl_configuratio)
{

	if (isc_main_control_ != nullptr) {
		return ISCDPL_E_OPVERLAPED_OPERATION;
	}

	isc_main_control_ = new IscMainControl;
	int ret = isc_main_control_->Initialize(ipc_dpl_configuratio);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::Terminate()
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->Terminate();
	if (ret != DPC_E_OK) {
		// continue on error!
	}

	delete isc_main_control_;
	isc_main_control_ = nullptr;

	return DPC_E_OK;
}

// camera dependent paraneter

/**
 * 機能が実装されているかどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 実装されています
 * @retval false 実装されていません
 */
bool IscDpl::DeviceOptionIsImplemented(const IscCameraInfo option_name)
{

	if (isc_main_control_ == nullptr) {
		return false;
	}

	bool ret = isc_main_control_->DeviceOptionIsImplemented(option_name);

	return ret;
}

/**
 * 値を取得可能かどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 読み込み可能です
 * @retval false 読み込みはできません
 */
bool IscDpl::DeviceOptionIsReadable(const IscCameraInfo option_name)
{

	if (isc_main_control_ == nullptr) {
		return false;
	}

	bool ret = isc_main_control_->DeviceOptionIsReadable(option_name);

	return ret;
}

/**
 * 値を書き込み可能かどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 書き込み可能です
 * @retval false 書き込みはできません
 */
bool IscDpl::DeviceOptionIsWritable(const IscCameraInfo option_name)
{

	if (isc_main_control_ == nullptr) {
		return false;
	}

	bool ret = isc_main_control_->DeviceOptionIsWritable(option_name);

	return ret;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceGetOptionMin(const IscCameraInfo option_name, int* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionMax(const IscCameraInfo option_name, int* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionInc(const IscCameraInfo option_name, int* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionInc(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOption(const IscCameraInfo option_name, int* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceSetOption(const IscCameraInfo option_name, const int value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/flaot)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceGetOptionMin(const IscCameraInfo option_name, float* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/flaot)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceGetOptionMax(const IscCameraInfo option_name, float* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/flaot)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceGetOption(const IscCameraInfo option_name, float* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraInfo/flaot)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraInfo option_name, const float value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOption(const IscCameraInfo option_name, bool* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraInfo/bool)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraInfo option_name, const bool value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/char)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[in] max_length valueの最大文字数
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value, max_length);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraInfo/char)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraInfo option_name, const char* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionInc(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOption(const IscCameraInfo option_name, uint64_t* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraInfo option_name, const uint64_t value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
bool IscDpl::DeviceOptionIsImplemented(const IscCameraParameter option_name)
{

	if (isc_main_control_ == nullptr) {
		return false;
	}

	bool ret = isc_main_control_->DeviceOptionIsImplemented(option_name);

	return ret;
}

/**
 * 値を取得可能かどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 読み込み可能です
 * @retval false 読み込みはできません
 */
bool IscDpl::DeviceOptionIsReadable(const IscCameraParameter option_name)
{

	if (isc_main_control_ == nullptr) {
		return false;
	}

	bool ret = isc_main_control_->DeviceOptionIsReadable(option_name);

	return ret;
}

/**
 * 値を書き込み可能かどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 書き込み可能です
 * @retval false 書き込みはできません
 */
bool IscDpl::DeviceOptionIsWritable(const IscCameraParameter option_name)
{

	if (isc_main_control_ == nullptr) {
		return false;
	}

	bool ret = isc_main_control_->DeviceOptionIsWritable(option_name);

	return ret;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceGetOptionMin(const IscCameraParameter option_name, int* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionMax(const IscCameraParameter option_name, int* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionInc(const IscCameraParameter option_name, int* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionInc(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOption(const IscCameraParameter option_name, int* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraParameter option_name, const int value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionMin(const IscCameraParameter option_name, float* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionMax(const IscCameraParameter option_name, float* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOption(const IscCameraParameter option_name, float* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraParameter option_name, const float value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOption(const IscCameraParameter option_name, bool* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraParameter/bool)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraParameter option_name, const bool value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/char)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[in] max_length valueの最大文字数
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value, max_length);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraParameter/char)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraParameter option_name, const char* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOptionInc(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOption(const IscCameraParameter option_name, uint64_t* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraParameter option_name, const uint64_t value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 値を設定します(IscCameraParameter/IscShutterMode)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}


// grab control

/**
 * 取り込みを開始します
 *
 * @param[in] isc_start_mode 開始のためのパラメータ
 * 
 * @retval 0 成功
 * @retval other 失敗
 * @details
 *  - カメラ又はファイルから取得可能です
 *  - 詳細は IscStartMode　を参照します
 */
int IscDpl::Start(const IscStartMode* isc_start_mode)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->Start(isc_start_mode);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 取り込みを停止します
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::Stop()
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->Stop();
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 現在の動作モードを取得します
 *
 * @param[in] isc_start_mode 現在のパラメータ
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetGrabMode(isc_grab_start_mode);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::InitializeIscIamgeinfo(IscImageInfo* isc_image_Info)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->InitializeIscIamgeinfo(isc_image_Info);
	if (ret != DPC_E_OK) {
		return ret;
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
int IscDpl::ReleaeIscIamgeinfo(IscImageInfo* isc_image_Info)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->ReleaeIscIamgeinfo(isc_image_Info);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * データを取得します
 *
 * @param[in] isc_image_Info バッファー構造体
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetCameraData(IscImageInfo* isc_image_Info)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetCameraData(isc_image_Info);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * ファイルよりデータを取得する場合に、ヘッダーを取得します
 *
 * @param[in] play_file_name ファイル名
 * @param[out] raw_file_header ヘッダー構造体
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetFileInformation(play_file_name, raw_file_header);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

// get information for depth, distance, ...

/**
 * 指定位置の視差と距離を取得します
 *
 * @param[in] x 画像内座標(X)
 * @param[in] y 画像内座標(Y)
 * @param[in] isc_image_info データ構造体
 * @param[out] disparity 視差
 * @param[out] depth 距離(m)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetPositionDepth(const int x, const int y, const IscImageInfo* isc_image_info, float* disparity, float* depth)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetPositionDepth(x, y, isc_image_info, disparity, depth);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 指定位置の3D位置を取得します
 *
 * @param[in] x 画像内座標(X)
 * @param[in] y 画像内座標(Y)
 * @param[in] isc_image_info データ構造体
 * @param[out] x_d 画面中央からの距離(m)
 * @param[out] y_d 画面中央からの距離(m)
 * @param[out] z_d 距離(m)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetPosition3D(const int x, const int y, const IscImageInfo* isc_image_info, float* x_d, float* y_d, float* z_d)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetPosition3D(x, y, isc_image_info, x_d, y_d, z_d);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 指定領域の情報を取得します
 *
 * @param[in] x 画像内座標左上(X)
 * @param[in] y 画像内座標左上(Y)
 * @param[in] width 幅
 * @param[in] height 高さ 
 * @param[in] isc_image_info データ構造体
 * @param[out] isc_data_statistics 領域の情報
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetAreaStatistics(const int x, const int y, const int width, const int height, const IscImageInfo* isc_image_info, IscAreaDataStatistics* isc_data_statistics)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetAreaStatistics(x, y, width, height, isc_image_info, isc_data_statistics);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}


// data processing module settings

/**
 * 利用可能なデータ処理モジュールの数を取得します
 *
 * @param[out] total_count モジュール数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetTotalModuleCount(int* total_count)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetTotalModuleCount(total_count);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 利用可能なデータ処理モジュールの数を取得します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[in] max_length バッファーの最大文字数
 * @param[out] module_name モジュール名
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetModuleNameByIndex(const int module_index, wchar_t* module_name, int max_length)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetModuleNameByIndex(module_index, module_name, max_length);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 指定したモジュールのパラメータを取得します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[out] isc_data_proc_module_parameter 取得したパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetDataProcModuleParameter(module_index, isc_data_proc_module_parameter);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 指定したモジュールへパラメータを設定します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[out] isc_data_proc_module_parameter 設定するパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::SetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->SetDataProcModuleParameter(module_index, isc_data_proc_module_parameter, is_update_file);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 指定したモジュールのパラメータファイルのファイル名を取得します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[in] max_length バッファーの最大文字数
 * @param[out] file_name パラメータファイル名
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetParameterFileName(const int module_index, wchar_t* file_name, const int max_length)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetParameterFileName(module_index, file_name, max_length);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * 指定したモジュールへファイルからパラメータの読み込みを指示します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[in] file_name パラメータファイル名
 * @param[in] is_valid パラメータを反映させるかどうかを指定します true:即時反映
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::ReloadParameterFromFile(const int module_index, const wchar_t* file_name, const bool is_valid)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->ReloadParameterFromFile(module_index, file_name, is_valid);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}


// data processing module result data

/**
 * モジュールの処理結果バッファーを初期化します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::InitializeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{
	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->InitializeIscDataProcResultData(isc_data_proc_result_data);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * モジュールの処理結果バッファーを解放します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::ReleaeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{
	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->ReleaeIscDataProcResultData(isc_data_proc_result_data);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * モジュールの処理結果を取得します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDpl::GetDataProcModuleData(IscDataProcResultData* isc_data_proc_result_data)
{

	if (isc_main_control_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

	int ret = isc_main_control_->GetDataProcModuleData(isc_data_proc_result_data);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}



} /* namespace ns_isc_dpl { */