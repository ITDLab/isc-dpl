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
 * @file isc_camera_control.cpp
 * @brief camera control class interface
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a interface for using camera.
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <process.h>
#include <mutex>
#include <functional>

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"
#include "isc_log.h"
#include "utility.h"

#include "vm_sdk_wrapper.h"
#include "xc_sdk_wrapper.h"
#include "k4a_sdk_wrapper.h"
#include "isc_sdk_control.h"
#include "isc_image_info_ring_buffer.h"
#include "isc_file_write_control_impl.h"
#include "isc_raw_data_decoder.h"
#include "isc_file_read_control_impl.h"
#include "isc_selftcalibration_interface.h"

#include "isc_camera_control.h"

#include "opencv2\opencv.hpp"

#pragma comment (lib, "shlwapi")
#pragma comment (lib, "Shell32")
#pragma comment (lib, "imagehlp")
#pragma comment (lib, "VmSdkWrapper")
#pragma comment (lib, "XcSdkWrapper")
#pragma comment (lib, "XcSdkWrapper")
#pragma comment (lib, "K4aSdkWrapper")
#pragma comment (lib, "IscSelfCalibration")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world480d")
#else
#pragma comment (lib,"opencv_world480")
#endif

// Callback Control
/*
	IscSelfCalibrationにおいてカメラのRegistへ直接R/Wするための関数を提供する
*/
int CallbackGetCameraRegData(unsigned char* wbuf, unsigned char* rbuf, int write_size, int read_size);
int CallbackSetCameraRegData(unsigned char* wbuf, int write_size);

class CallbackIscSdkControlControl {
public:
	IscSdkControl* isc_sdk_control_;

	CallbackIscSdkControlControl():isc_sdk_control_(nullptr)
	{
		
	}

	~CallbackIscSdkControlControl()
	{
		isc_sdk_control_ = nullptr;
	}


	void SetSdkControl(IscSdkControl* isc_sdk_control)
	{
		isc_sdk_control_ = isc_sdk_control;
	}

	int GetCameraRegData(unsigned char* wbuf, unsigned char* rbuf, int write_size, int read_size)
	{
		if (isc_sdk_control_ == nullptr) {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		
		int ret = isc_sdk_control_->DeviceGetOption(IscCameraParameter::kGenericRead, wbuf, write_size, rbuf, read_size);
		
		return ret;
	}

	int SetCameraRegData(unsigned char* wbuf, int write_size)
	{
		if (isc_sdk_control_ == nullptr) {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}

		int ret = isc_sdk_control_->DeviceSetOption(IscCameraParameter::kGenericWrite, wbuf, write_size);

		return ret;
	}

};
static CallbackIscSdkControlControl callback_iscsdkcontrol_control_;

/**
 * constructor
 *
 */
IscCameraControl::IscCameraControl() :
	isc_log_(nullptr),
	measure_tackt_time_(nullptr),
	isc_camera_control_config_(),
	camera_specific_parameter_(),
	isc_sdk_control_(nullptr), 
	isc_file_write_control_impl_(nullptr),
	isc_file_read_control_impl_(nullptr),
	enabled_isc_selfcalibration_(false),
	isc_selfcalibration_interface_(nullptr),
	isc_image_info_ring_buffer_(nullptr),
	isc_run_status_(),
	thread_control_(), 
	handle_semaphore_(NULL), 
	thread_handle_(NULL), 
	threads_critical_()
{

	measure_tackt_time_ = new UtilityMeasureTime;

	memset(&thread_control_, 0, sizeof(thread_control_));

	isc_run_status_.isc_run_state = IscRunState::stop;
	isc_run_status_.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kParallax;
	isc_run_status_.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
	isc_run_status_.isc_grab_start_mode.isc_get_mode.wait_time = 100;
	isc_run_status_.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
	isc_run_status_.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kBGR;

}

/**
 * destructor
 *
 */
IscCameraControl::~IscCameraControl()
{
	delete measure_tackt_time_;
	measure_tackt_time_ = nullptr;
}

/**
 * クラスを初期化します.
 *
 * @param[in] isc_camera_control_configuration 初期化パラメータ構造体
 * @param[in] isc_log ログオブジェクト
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::Initialize(const IscCameraControlConfiguration* isc_camera_control_configuration, IscLog* isc_log)
{

	swprintf_s(isc_camera_control_config_.configuration_file_path, L"%s", isc_camera_control_configuration->configuration_file_path);
	swprintf_s(isc_camera_control_config_.log_file_path, L"%s", isc_camera_control_configuration->log_file_path);
	isc_camera_control_config_.log_level = isc_camera_control_configuration->log_level;

	isc_camera_control_config_.enabled_camera = isc_camera_control_configuration->enabled_camera;
	isc_camera_control_config_.isc_camera_model = isc_camera_control_configuration->isc_camera_model;

	swprintf_s(isc_camera_control_config_.save_image_path, L"%s", isc_camera_control_configuration->save_image_path);
	swprintf_s(isc_camera_control_config_.load_image_path, L"%s", isc_camera_control_configuration->load_image_path);

	measure_tackt_time_->Init();

	// log
	isc_log_ = isc_log;

	if (isc_camera_control_config_.enabled_camera) {
		// it use camera 
		isc_sdk_control_ = new IscSdkControl;
		int ret = isc_sdk_control_->Initialize(isc_camera_control_config_.isc_camera_model);
		if (ret != DPC_E_OK) {
			return ret;
		}

		ret = isc_sdk_control_->DeviceOpen();
		if (ret != DPC_E_OK) {
			return ret;
		}

		int width = 0, height = 0;
		ret = isc_sdk_control_->DeviceGetOption(IscCameraInfo::kWidthMax, &width);
		if (ret != DPC_E_OK) {
			return ret;
		}

		ret = isc_sdk_control_->DeviceGetOption(IscCameraInfo::kHeightMax, &height);
		if (ret != DPC_E_OK) {
			return ret;
		}

		ret = isc_sdk_control_->DeviceGetOption(IscCameraInfo::kDINF, &camera_specific_parameter_.d_inf);
		if (ret != DPC_E_OK) {
			return ret;
		}

		ret = isc_sdk_control_->DeviceGetOption(IscCameraInfo::kBF, &camera_specific_parameter_.bf);
		if (ret != DPC_E_OK) {
			return ret;
		}

		ret = isc_sdk_control_->DeviceGetOption(IscCameraInfo::kBaseLength, &camera_specific_parameter_.base_length);
		if (ret != DPC_E_OK) {
			return ret;
		}

		ret = isc_sdk_control_->DeviceGetOption(IscCameraInfo::kDz, &camera_specific_parameter_.dz);
		if (ret != DPC_E_OK) {
			return ret;
		}

		// writer
		int max_buffer_count = 0;
		ret = isc_sdk_control_->GetRecommendedBufferCount(&max_buffer_count);
		if (ret != DPC_E_OK) {
			return ret;
		}

		IscSaveDataConfiguration save_data_configration = {};
		save_data_configration.max_save_folder_count = 1;
		save_data_configration.save_folder_count = 1;
		swprintf_s(save_data_configration.save_folders[0], L"%s", isc_camera_control_configuration->save_image_path);
		save_data_configration.minimum_capacity_required = 20;	// 20GB
		save_data_configration.save_time_for_one_file = 60;		// 60分
		save_data_configration.max_buffer_count = max_buffer_count;
		
		isc_file_write_control_impl_ = new IscFileWriteControlImpl;
		ret = isc_file_write_control_impl_->Initialize(isc_camera_control_configuration, &save_data_configration, width, height, isc_log_);
		if (ret != DPC_E_OK) {
			return ret;
		}

		// reader
		isc_file_read_control_impl_ = new IscFileReadControlImpl;
		isc_file_read_control_impl_->Initialize(isc_camera_control_configuration);

		// selft calibration
		callback_iscsdkcontrol_control_.SetSdkControl(isc_sdk_control_);

		isc_selfcalibration_interface_ = new IscSelftCalibrationInterface;
		isc_selfcalibration_interface_->Initialize(isc_camera_control_configuration, width, height);
		isc_selfcalibration_interface_->SetCallbackFunc(CallbackGetCameraRegData, CallbackSetCameraRegData);

		// Get Buffer
		isc_image_info_ring_buffer_ = new IscImageInfoRingBuffer;
		isc_image_info_ring_buffer_->Initialize(true, true, max_buffer_count, width, height);
		isc_image_info_ring_buffer_->Clear();

		// Create Thread
		thread_control_.terminate_request = 0;
		thread_control_.terminate_done = 0;
		thread_control_.end_code = 0;
		thread_control_.stop_request = false;

		char semaphoreName[64] = {};
		sprintf_s(semaphoreName, "THREAD_SEMAPHORENAME_ISCCAMERACONT_%d", 0);
		handle_semaphore_ = CreateSemaphoreA(NULL, 0, 1, semaphoreName);
		if (handle_semaphore_ == NULL) {
			// Fail
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}
		InitializeCriticalSection(&threads_critical_);

		if ((thread_handle_ = (HANDLE)_beginthreadex(0, 0, ControlThread, (void*)this, 0, 0)) == 0) {
			// Fail
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}

		// THREAD_PRIORITY_TIME_CRITICAL
		// THREAD_PRIORITY_HIGHEST +2
		// THREAD_PRIORITY_ABOVE_NORMAL +1
		// THREAD_PRIORITY_NORMAL  +0
		// THREAD_PRIORITY_BELOW_NORMAL -1
		SetThreadPriority(thread_handle_, THREAD_PRIORITY_NORMAL);

	}
	else {
		// Operation from file is possible

		// it use camera 
		isc_sdk_control_ = new IscSdkControl;
		int ret = isc_sdk_control_->Initialize(isc_camera_control_config_.isc_camera_model);
		if (ret != DPC_E_OK) {
			return ret;
		}

		// reader
		isc_file_read_control_impl_ = new IscFileReadControlImpl;
		isc_file_read_control_impl_->Initialize(isc_camera_control_configuration);

		// Specify the maximum size that corresponds
		int width = 3840;
		int height = 1920;

		// Get Buffer
		isc_image_info_ring_buffer_ = new IscImageInfoRingBuffer;
		constexpr int max_buffer_count = 4;	// 16;
		isc_image_info_ring_buffer_->Initialize(true, true, max_buffer_count, width, height);
		isc_image_info_ring_buffer_->Clear();
	}

	return DPC_E_OK;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::Terminate()
{

	if (isc_camera_control_config_.enabled_camera) {
		// close thread procedure
		thread_control_.stop_request = true;
		thread_control_.terminate_done = 0;
		thread_control_.end_code = 0;
		thread_control_.terminate_request = 1;

		// release any resources
		ReleaseSemaphore(handle_semaphore_, 1, NULL);

		int count = 0;
		while (thread_control_.terminate_done == 0) {
			if (count > 100) {
				break;
			}
			count++;
			Sleep(10L);
		}

		if (thread_handle_ != NULL) {
			CloseHandle(thread_handle_);
			thread_handle_ = NULL;
		}
		if (handle_semaphore_ != NULL) {
			CloseHandle(handle_semaphore_);
			handle_semaphore_ = NULL;
		}
		DeleteCriticalSection(&threads_critical_);

		if (isc_image_info_ring_buffer_ != nullptr) {
			isc_image_info_ring_buffer_->Terminate();
			delete isc_image_info_ring_buffer_;
			isc_image_info_ring_buffer_ = nullptr;
		}

		if (isc_selfcalibration_interface_ != nullptr) {
			isc_selfcalibration_interface_->Terminate();
			delete isc_selfcalibration_interface_;
			isc_selfcalibration_interface_ = nullptr;
		}

		if (isc_file_read_control_impl_ != nullptr) {
			isc_file_read_control_impl_->Terminate();
			delete isc_file_read_control_impl_;
			isc_file_read_control_impl_ = nullptr;
		}

		if (isc_file_write_control_impl_ != nullptr) {
			isc_file_write_control_impl_->Terminate();
			delete isc_file_write_control_impl_;
			isc_file_write_control_impl_ = nullptr;
		}

		if (isc_sdk_control_ != nullptr) {
			int ret = isc_sdk_control_->DeviceClose();
			if (ret != DPC_E_OK) {
				return ret;
			}

			isc_sdk_control_->Terminate();
			delete isc_sdk_control_;
			isc_sdk_control_ = nullptr;
		}
	}
	else {
		if (isc_image_info_ring_buffer_ != nullptr) {
			isc_image_info_ring_buffer_->Terminate();
			delete isc_image_info_ring_buffer_;
			isc_image_info_ring_buffer_ = nullptr;
		}

		if (isc_file_read_control_impl_ != nullptr) {
			isc_file_read_control_impl_->Terminate();
			delete isc_file_read_control_impl_;
			isc_file_read_control_impl_ = nullptr;
		}

		if (isc_sdk_control_ != nullptr) {
			isc_sdk_control_->Terminate();
			delete isc_sdk_control_;
			isc_sdk_control_ = nullptr;
		}
	}

	isc_log_ = nullptr;

	return DPC_E_OK;
}

/**
 * Work Bufferの推奨数を戻します
 *
 * @param[out] buffer_count Buffer数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::GetRecommendedBufferCount(int* buffer_count)
{
	if (isc_sdk_control_ == nullptr) {
		return false;
	}

	bool ret = isc_sdk_control_->GetRecommendedBufferCount(buffer_count);

	return ret;
}

/**
* @brief データ受信Thread
* @param[in] contextThread変数
* @retuen {@code 0} 成功 , {@code >0} 失敗
* @throws never
*/
unsigned __stdcall IscCameraControl::ControlThread(void* context)
{
	IscCameraControl* isc_camera_control = (IscCameraControl*)context;

	if (isc_camera_control == nullptr) {
		return -1;
	}

	int ret = isc_camera_control->RecieveDataProc();

	return ret;
}

/**
* @brief データ受信処理
* @retuen {@code 0} 成功 , {@code >0} 失敗
* @throws never
*/
int IscCameraControl::RecieveDataProc()
{

	while (thread_control_.terminate_request < 1) {

		// Wait for start
		DWORD wait_result = WaitForSingleObject(handle_semaphore_, INFINITE);

		if (wait_result == WAIT_FAILED) {
			// error, abort
			// The thread got ownership of an abandoned mutex
			// The database is in an indeterminate state
			break;
		}

		// get image from sdk and copy to buffer
		for (;;) {
			if (thread_control_.stop_request) {
				thread_control_.stop_request = false;
				break;
			}

			// get buffer for proc
			IscImageInfoRingBuffer::BufferData* buffer_data = nullptr;
			const ULONGLONG time = GetTickCount64();
			int put_index = isc_image_info_ring_buffer_->GetPutBuffer(&buffer_data, time);

			int image_status = 0;
			if (put_index >= 0 && buffer_data != nullptr) {
				int ret = ImageHandler(buffer_data);

				if (ret == 0) {
					// OK
					image_status = 1;
				}
			}
			isc_image_info_ring_buffer_->DonePutBuffer(put_index, image_status);
		}
	}

	thread_control_.terminate_done = 1;

	return 0;
}

/**
* @brief SDKからのデータを取得し、要求があれば保存処理を行います
* @retuen {@code 0} 成功 , {@code >0} 失敗
* @throws never
*/
int IscCameraControl::ImageHandler(IscImageInfoRingBuffer::BufferData* buffer_data)
{
	// Get data from camera
	IscGetMode isc_get_mode;
	isc_get_mode.wait_time = isc_run_status_.isc_grab_start_mode.isc_get_mode.wait_time;	// wait for 100msec

	int ret = isc_sdk_control_->GetData(&isc_get_mode, &buffer_data->isc_image_info);

	if (ret != DPC_E_OK) {
		if (ret == CAMCONTROL_E_INVALID_DEVICEHANDLE) {
			// device open error
			Sleep(10);
			return -1;
		}

		if (ret == CAMCONTROL_E_FTDI_ERROR) {
			// USB Error
			Sleep(10);
			return -1;
		}
		else if (ret == CAMCONTROL_E_NO_IMAGE) {
			// image not ready
			return -1;
		}
		else if (ret == CAMCONTROL_E_CAMERA_UNDER_CARIBRATION) {
			// invalid image
			return -1;
		}
		else {
			// Continue to confirm image
		}
	}

	// data write
	if (isc_run_status_.isc_grab_start_mode.isc_record_mode == IscRecordMode::kRecordOn) {
		isc_file_write_control_impl_->Add(&buffer_data->isc_image_info);
	}

	// self calibration
	if (enabled_isc_selfcalibration_) {
		isc_selfcalibration_interface_->ParallelizeSelfCalibration(&buffer_data->isc_image_info);
	}

	// set takt time
	buffer_data->isc_image_info.frame_data[kISCIMAGEINFO_FRAMEDATA_LATEST].camera_status.data_receive_tact_time = measure_tackt_time_->GetTaktTime();

	return 0;
}

// camera dependent paraneter

/**
 * 機能が実装されているかどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 実装されています
 * @retval false 実装されていません
 */
bool IscCameraControl::DeviceOptionIsImplemented(const IscCameraInfo option_name)
{
	if (isc_sdk_control_ == nullptr) {
		return false;
	}

	bool ret = isc_sdk_control_->DeviceOptionIsImplemented(option_name);

	return ret;
}

/**
 * 値を取得可能かどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 読み込み可能です
 * @retval false 読み込みはできません
 */
bool IscCameraControl::DeviceOptionIsReadable(const IscCameraInfo option_name)
{
	if (isc_sdk_control_ == nullptr) {
		return false;
	}

	bool ret = isc_sdk_control_->DeviceOptionIsReadable(option_name);

	return ret;
}

/**
 * 値を書き込み可能かどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 書き込み可能です
 * @retval false 書き込みはできません
 */
bool IscCameraControl::DeviceOptionIsWritable(const IscCameraInfo option_name)
{
	if (isc_sdk_control_ == nullptr) {
		return false;
	}

	bool ret = isc_sdk_control_->DeviceOptionIsWritable(option_name);

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
int IscCameraControl::IscCameraControl::DeviceGetOptionMin(const IscCameraInfo option_name, int* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMax(const IscCameraInfo option_name, int* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な増減値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionInc(const IscCameraInfo option_name, int* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionInc(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraInfo option_name, int* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/int)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraInfo option_name, const int value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMin(const IscCameraInfo option_name, float* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMax(const IscCameraInfo option_name, float* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraInfo option_name, float* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/float)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraInfo option_name, const float value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraInfo/bool)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraInfo option_name, bool* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/bool)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraInfo option_name, const bool value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
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
int IscCameraControl::DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value, max_length);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/char)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraInfo option_name, const char* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/uint64t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な増減値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionInc(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraInfo option_name, uint64_t* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/uint64_t)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraInfo option_name, const uint64_t value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}


// camera control parameter

/**
 * 機能が実装されているかどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 実装されています
 * @retval false 実装されていません
 */
bool IscCameraControl::DeviceOptionIsImplemented(const IscCameraParameter option_name)
{
	if (isc_sdk_control_ == nullptr) {
		return false;
	}

	bool ret = isc_sdk_control_->DeviceOptionIsImplemented(option_name);

	return ret;
}

/**
 * 値を取得可能かどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 読み込み可能です
 * @retval false 読み込みはできません
 */
bool IscCameraControl::DeviceOptionIsReadable(const IscCameraParameter option_name)
{
	if (isc_sdk_control_ == nullptr) {
		return false;
	}

	bool ret = isc_sdk_control_->DeviceOptionIsReadable(option_name);

	return ret;
}

/**
 * 値を書き込み可能かどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 書き込み可能です
 * @retval false 書き込みはできません
 */
bool IscCameraControl::DeviceOptionIsWritable(const IscCameraParameter option_name)
{
	if (isc_sdk_control_ == nullptr) {
		return false;
	}

	bool ret = isc_sdk_control_->DeviceOptionIsWritable(option_name);

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
int IscCameraControl::DeviceGetOptionMin(const IscCameraParameter option_name, int* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMax(const IscCameraParameter option_name, int* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な増減値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionInc(const IscCameraParameter option_name, int* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionInc(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraParameter option_name, int* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/int)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraParameter option_name, const int value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMin(const IscCameraParameter option_name, float* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMax(const IscCameraParameter option_name, float* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraParameter option_name, float* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/float)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraParameter option_name, const float value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraParameter/bool)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraParameter option_name, bool* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = DPC_E_OK;

	switch (option_name) {
	case IscCameraParameter::kSelfCalibration:
		*value = enabled_isc_selfcalibration_;
		ret = DPC_E_OK;
		break;

	default:
		ret = isc_sdk_control_->DeviceGetOption(option_name, value);
		if (ret != DPC_E_OK) {
			return ret;
		}
		break;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/bool)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraParameter option_name, const bool value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = DPC_E_OK;

	switch (option_name) {
	case IscCameraParameter::kSelfCalibration:

		if (value) {
			isc_selfcalibration_interface_->StartSelfCalibration();
			enabled_isc_selfcalibration_ = true;
		}
		else {
			isc_selfcalibration_interface_->StoptSelfCalibration();
			enabled_isc_selfcalibration_ = false;
		}
		ret = DPC_E_OK;
		break;

	default:
		ret = isc_sdk_control_->DeviceSetOption(option_name, value);
		if (ret != DPC_E_OK) {
			return ret;
		}
		break;
	}

	return ret;
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
int IscCameraControl::DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value, max_length);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/char)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraParameter option_name, const char* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMin(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionMax(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 設定可能な増減値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOptionInc(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraParameter option_name, uint64_t* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/uint64_t)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraParameter option_name, const uint64_t value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を取得します(IscCameraParameter/IscShutterMode)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/IscShutterMode)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, value);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
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
int IscCameraControl::DeviceGetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size, unsigned char* read_value, const int read_size)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceGetOption(option_name, write_value, write_size, read_value, read_size);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
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
int IscCameraControl::DeviceSetOption(const IscCameraParameter option_name, unsigned char* write_value, const int write_size)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->DeviceSetOption(option_name, write_value, write_size);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
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
int IscCameraControl::Start(const IscGrabStartMode* isc_grab_start_mode)
{
	int ret = DPC_E_OK;

	if (isc_grab_start_mode->isc_play_mode == IscPlayMode::kPlayOn) {
		// it read from file
		isc_run_status_.isc_run_state = IscRunState::run;

		isc_run_status_.isc_grab_start_mode.isc_grab_mode = isc_grab_start_mode->isc_grab_mode;
		isc_run_status_.isc_grab_start_mode.isc_grab_color_mode = isc_grab_start_mode->isc_grab_color_mode;

		isc_run_status_.isc_grab_start_mode.isc_get_mode.wait_time = isc_grab_start_mode->isc_get_mode.wait_time;
		isc_run_status_.isc_grab_start_mode.isc_get_raw_mode = isc_grab_start_mode->isc_get_raw_mode;
		isc_run_status_.isc_grab_start_mode.isc_get_color_mode = isc_grab_start_mode->isc_get_color_mode;

		isc_run_status_.isc_grab_start_mode.isc_record_mode = isc_grab_start_mode->isc_record_mode;
		isc_run_status_.isc_grab_start_mode.isc_play_mode = isc_grab_start_mode->isc_play_mode;
		isc_run_status_.isc_grab_start_mode.isc_play_mode_parameter.interval = isc_grab_start_mode->isc_play_mode_parameter.interval;
		swprintf_s(isc_run_status_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, L"%s", isc_grab_start_mode->isc_play_mode_parameter.play_file_name);

		ret = isc_file_read_control_impl_->Start(&isc_run_status_.isc_grab_start_mode);
	}
	else {
		// get data from camera
		if (isc_sdk_control_ == nullptr) {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}

		isc_image_info_ring_buffer_->Clear();
		measure_tackt_time_->Init();

		isc_run_status_.isc_run_state = IscRunState::run;

		isc_run_status_.isc_grab_start_mode.isc_grab_mode = isc_grab_start_mode->isc_grab_mode;
		isc_run_status_.isc_grab_start_mode.isc_grab_color_mode = isc_grab_start_mode->isc_grab_color_mode;

		isc_run_status_.isc_grab_start_mode.isc_get_mode.wait_time = isc_grab_start_mode->isc_get_mode.wait_time;
		isc_run_status_.isc_grab_start_mode.isc_get_raw_mode = isc_grab_start_mode->isc_get_raw_mode;
		isc_run_status_.isc_grab_start_mode.isc_get_color_mode = isc_grab_start_mode->isc_get_color_mode;

		isc_run_status_.isc_grab_start_mode.isc_record_mode = isc_grab_start_mode->isc_record_mode;
		isc_run_status_.isc_grab_start_mode.isc_play_mode = isc_grab_start_mode->isc_play_mode;
		isc_run_status_.isc_grab_start_mode.isc_play_mode_parameter.interval = isc_grab_start_mode->isc_play_mode_parameter.interval;
		swprintf_s(isc_run_status_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, L"%s", isc_grab_start_mode->isc_play_mode_parameter.play_file_name);

		if (isc_run_status_.isc_grab_start_mode.isc_record_mode == IscRecordMode::kRecordOn) {
			IscShutterMode shutter_mode = IscShutterMode::kManualShutter;
			int ret = isc_sdk_control_->DeviceGetOption(IscCameraParameter::kShutterMode, &shutter_mode);

			isc_file_write_control_impl_->Start(&camera_specific_parameter_, isc_grab_start_mode, shutter_mode);
		}

		// it start recieve thread
		ReleaseSemaphore(handle_semaphore_, 1, NULL);

		// start
		ret = isc_sdk_control_->Start(isc_grab_start_mode);
		if (ret != DPC_E_OK) {
			return ret;
		}
	}

	return ret;
}

/**
 * 取り込みを停止します
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::Stop()
{
	int ret = DPC_E_OK;

	if (isc_run_status_.isc_grab_start_mode.isc_play_mode == IscPlayMode::kPlayOn) {
		// it read from file
		isc_run_status_.isc_run_state = IscRunState::stop;

		ret = isc_file_read_control_impl_->Stop();
	}
	else {
		// get data from camera
		if (isc_sdk_control_ == nullptr) {
			return CAMCONTROL_E_INVALID_DEVICEHANDLE;
		}

		isc_run_status_.isc_run_state = IscRunState::stop;

		// stop writer
		if (isc_run_status_.isc_grab_start_mode.isc_record_mode == IscRecordMode::kRecordOn) {
			isc_file_write_control_impl_->Stop();
		}

		// stop grab
		ret = isc_sdk_control_->Stop();
		if (ret != DPC_E_OK) {
			return ret;
		}

		// it stop recieve loop
		thread_control_.stop_request = true;
	}

	return ret;
}

/**
 * 現在の動作モードを取得します
 *
 * @param[in] isc_grab_start_mode 現在のパラメータ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	int ret = isc_sdk_control_->GetGrabMode(isc_grab_start_mode);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
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
int IscCameraControl::InitializeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	int ret = isc_sdk_control_->InitializeIscIamgeinfo(isc_image_info);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * データ取得のためのバッファーを解放します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::ReleaeIscIamgeinfo(IscImageInfo* isc_image_info)
{
	if (isc_sdk_control_ == nullptr) {
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	int ret = isc_sdk_control_->ReleaeIscIamgeinfo(isc_image_info);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * カメラ又はファイルよりデータを取得します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::GetData(IscImageInfo* isc_image_info)
{

	int ret = DPC_E_OK;

	if (isc_run_status_.isc_grab_start_mode.isc_play_mode == IscPlayMode::kPlayOn) {
		// it read from file
		ret = GetDataReadFile(isc_image_info);
	}
	else {
		// get data from camera
		ret = GetDataLiveCamera(isc_image_info);
	}

	return ret;
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
int IscCameraControl::GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header)
{
	int ret = DPC_E_OK;

	ret = isc_file_read_control_impl_->GetFileInformation(play_file_name, raw_file_header);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return ret;
}

/**
 * カメラよりデータを取得します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::GetDataLiveCamera(IscImageInfo* isc_image_info)
{

	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	// check stop
	if (isc_run_status_.isc_run_state != IscRunState::run) {
		// already stop
		return CAMCONTROL_E_NO_IMAGE;
	}

	// get data
	IscImageInfoRingBuffer::BufferData* buffer_data = nullptr;
	ULONGLONG time = 0;
	int get_index = isc_image_info_ring_buffer_->GetGetBuffer(&buffer_data, &time);

	if (get_index < 0) {
		return CAMCONTROL_E_NO_IMAGE;
	}

	isc_image_info->grab = buffer_data->isc_image_info.grab;
	isc_image_info->color_grab_mode = buffer_data->isc_image_info.color_grab_mode;
	isc_image_info->shutter_mode = buffer_data->isc_image_info.shutter_mode;
	isc_image_info->camera_specific_parameter.d_inf = buffer_data->isc_image_info.camera_specific_parameter.d_inf;
	isc_image_info->camera_specific_parameter.bf = buffer_data->isc_image_info.camera_specific_parameter.bf;
	isc_image_info->camera_specific_parameter.base_length = buffer_data->isc_image_info.camera_specific_parameter.base_length;
	isc_image_info->camera_specific_parameter.dz = buffer_data->isc_image_info.camera_specific_parameter.dz;

	for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
		isc_image_info->frame_data[i].frameNo = buffer_data->isc_image_info.frame_data[i].frameNo;
		isc_image_info->frame_data[i].gain = buffer_data->isc_image_info.frame_data[i].gain;
		isc_image_info->frame_data[i].exposure = buffer_data->isc_image_info.frame_data[i].exposure;

		isc_image_info->frame_data[i].camera_status.error_code = buffer_data->isc_image_info.frame_data[i].camera_status.error_code;
		isc_image_info->frame_data[i].camera_status.data_receive_tact_time = buffer_data->isc_image_info.frame_data[i].camera_status.data_receive_tact_time;

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

		// p1
		isc_image_info->frame_data[i].p1.width = buffer_data->isc_image_info.frame_data[i].p1.width;
		isc_image_info->frame_data[i].p1.height = buffer_data->isc_image_info.frame_data[i].p1.height;
		isc_image_info->frame_data[i].p1.channel_count = buffer_data->isc_image_info.frame_data[i].p1.channel_count;

		size_t copy_size = buffer_data->isc_image_info.frame_data[i].p1.width * buffer_data->isc_image_info.frame_data[i].p1.height * buffer_data->isc_image_info.frame_data[i].p1.channel_count;
		if (copy_size > 0) {
			memcpy(isc_image_info->frame_data[i].p1.image, buffer_data->isc_image_info.frame_data[i].p1.image, copy_size);
		}

		// p2
		if (isc_run_status_.isc_grab_start_mode.isc_grab_mode == IscGrabMode::kCorrect ||
			isc_run_status_.isc_grab_start_mode.isc_grab_mode == IscGrabMode::kBeforeCorrect) {

			isc_image_info->frame_data[i].p2.width = buffer_data->isc_image_info.frame_data[i].p2.width;
			isc_image_info->frame_data[i].p2.height = buffer_data->isc_image_info.frame_data[i].p2.height;
			isc_image_info->frame_data[i].p2.channel_count = buffer_data->isc_image_info.frame_data[i].p2.channel_count;

			copy_size = buffer_data->isc_image_info.frame_data[i].p2.width * buffer_data->isc_image_info.frame_data[i].p2.height * buffer_data->isc_image_info.frame_data[i].p2.channel_count;
			if (copy_size > 0) {
				memcpy(isc_image_info->frame_data[i].p2.image, buffer_data->isc_image_info.frame_data[i].p2.image, copy_size);
			}
		}

		// color
		if (isc_run_status_.isc_grab_start_mode.isc_grab_color_mode == IscGrabColorMode::kColorON) {
			if (buffer_data->isc_image_info.frame_data[i].color.width != 0 && buffer_data->isc_image_info.frame_data[i].color.height != 0 &&
				buffer_data->isc_image_info.frame_data[i].color.channel_count == 3) {

				isc_image_info->frame_data[i].color.width = buffer_data->isc_image_info.frame_data[i].color.width;
				isc_image_info->frame_data[i].color.height = buffer_data->isc_image_info.frame_data[i].color.height;
				isc_image_info->frame_data[i].color.channel_count = buffer_data->isc_image_info.frame_data[i].color.channel_count;

				copy_size = buffer_data->isc_image_info.frame_data[i].color.width * buffer_data->isc_image_info.frame_data[i].color.height * buffer_data->isc_image_info.frame_data[i].color.channel_count;
				if (copy_size > 0) {
					memcpy(isc_image_info->frame_data[i].color.image, buffer_data->isc_image_info.frame_data[i].color.image, copy_size);
				}
			}
		}

		// depth
		if (isc_run_status_.isc_grab_start_mode.isc_grab_mode == IscGrabMode::kParallax) {
			if (buffer_data->isc_image_info.frame_data[i].depth.width != 0 && buffer_data->isc_image_info.frame_data[i].depth.height != 0) {

				isc_image_info->frame_data[i].depth.width = buffer_data->isc_image_info.frame_data[i].depth.width;
				isc_image_info->frame_data[i].depth.height = buffer_data->isc_image_info.frame_data[i].depth.height;

				copy_size = buffer_data->isc_image_info.frame_data[i].depth.width * buffer_data->isc_image_info.frame_data[i].depth.height * sizeof(float);
				if (copy_size > 0) {
					memcpy(isc_image_info->frame_data[i].depth.image, buffer_data->isc_image_info.frame_data[i].depth.image, copy_size);
				}
			}
		}

		//raw
		if (isc_run_status_.isc_grab_start_mode.isc_get_raw_mode == IscGetModeRaw::kRawOn) {
			if (buffer_data->isc_image_info.frame_data[i].raw.width != 0 && buffer_data->isc_image_info.frame_data[i].raw.height != 0) {

				isc_image_info->frame_data[i].raw.width = buffer_data->isc_image_info.frame_data[i].raw.width;
				isc_image_info->frame_data[i].raw.height = buffer_data->isc_image_info.frame_data[i].raw.height;
				isc_image_info->frame_data[i].raw.channel_count = buffer_data->isc_image_info.frame_data[i].raw.channel_count;

				copy_size = buffer_data->isc_image_info.frame_data[i].raw.width * buffer_data->isc_image_info.frame_data[i].raw.height;
				if (copy_size > 0) {
					memcpy(isc_image_info->frame_data[i].raw.image, buffer_data->isc_image_info.frame_data[i].raw.image, copy_size);
				}
			}
		}

		//raw color
		if ((isc_run_status_.isc_grab_start_mode.isc_get_raw_mode == IscGetModeRaw::kRawOn) &&
			(isc_run_status_.isc_grab_start_mode.isc_grab_color_mode == IscGrabColorMode::kColorON)) {
			if (buffer_data->isc_image_info.frame_data[i].raw_color.width != 0 && buffer_data->isc_image_info.frame_data[i].raw_color.height != 0) {

				isc_image_info->frame_data[i].raw_color.width = buffer_data->isc_image_info.frame_data[i].raw_color.width;
				isc_image_info->frame_data[i].raw_color.height = buffer_data->isc_image_info.frame_data[i].raw_color.height;
				isc_image_info->frame_data[i].raw_color.channel_count = buffer_data->isc_image_info.frame_data[i].raw_color.channel_count;

				copy_size = buffer_data->isc_image_info.frame_data[i].raw_color.width * buffer_data->isc_image_info.frame_data[i].raw_color.height;
				if (copy_size > 0) {
					memcpy(isc_image_info->frame_data[i].raw_color.image, buffer_data->isc_image_info.frame_data[i].raw_color.image, copy_size);
				}
			}
		}

	}

	isc_image_info_ring_buffer_->DoneGetBuffer(get_index);

	return DPC_E_OK;
}

/**
 * ファイルよりデータを取得します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscCameraControl::GetDataReadFile(IscImageInfo* isc_image_info)
{
	if (isc_image_info == nullptr) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	// check stop
	if (isc_run_status_.isc_run_state != IscRunState::run) {
		// already stop
		return CAMCONTROL_E_NO_IMAGE;
	}

	int ret = isc_file_read_control_impl_->GetData(isc_image_info);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * CallBack用　レジスタアクセス関数
 *
 * @param[in] wbuf バッファー構造体
 * @param[in] rbuf バッファー構造体
 * @param[in] write_size 書き込み数
 * @param[in] read_size 読み込み数
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int CallbackGetCameraRegData(unsigned char* wbuf, unsigned char* rbuf, int write_size, int read_size)
{
	int ret = callback_iscsdkcontrol_control_.GetCameraRegData(wbuf, rbuf, write_size, read_size);

	return ret;
}

/**
 * CallBack用　レジスタアクセス関数
 *
 * @param[in] wbuf バッファー構造体
 * @param[in] write_size 書き込み数
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int CallbackSetCameraRegData(unsigned char* wbuf, int write_size)
{
	int ret = callback_iscsdkcontrol_control_.SetCameraRegData(wbuf, write_size);

	return ret;
}
