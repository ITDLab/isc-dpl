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
 * @file isc_file_write_control_imple.cpp
 * @brief implementation class of camera cntrol
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides reading and writing of files.
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
#include <mutex>

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"
#include "isc_log.h"
#include "utility.h"

#include "isc_image_info_ring_buffer.h"

#include "isc_file_write_control_impl.h"


/**
 * constructor
 *
 */
IscFileWriteControlImpl::IscFileWriteControlImpl():
	isc_camera_control_config_(), isc_save_data_configuration_(), camera_width_(0), camera_height_(0), isc_image_info_ring_buffer_(nullptr), isc_log_(nullptr), utility_measure_time_(nullptr),
	file_write_information_(), thread_control_(), handle_semaphore_(NULL), thread_handle_(NULL), threads_critical_()
{

}

/**
 * destructor
 *
 */
IscFileWriteControlImpl::~IscFileWriteControlImpl()
{

}

/**
 * クラスを初期化し書き込み先を確保します.
 *
 * @param[in] isc_camera_control_configuration 初期化パラメータ構造体
 * @param[in] isc_save_data_configuration 保存設定
 * @param[in] width データ幅
 * @param[in] height データ高さ
 * @param[in] isc_log ログオブジェクト
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::Initialize(const IscCameraControlConfiguration* isc_camera_control_configuration, const IscSaveDataConfiguration* isc_save_data_configuration, const int width, const int height, IscLog* isc_log)
{

	wchar_t logMag[128] = {};

	swprintf_s(isc_camera_control_config_.configuration_file_path, L"%s", isc_camera_control_configuration->configuration_file_path);
	swprintf_s(isc_camera_control_config_.log_file_path, L"%s", isc_camera_control_configuration->log_file_path);
	isc_camera_control_config_.log_level = isc_camera_control_configuration->log_level;

	isc_camera_control_config_.enabled_camera = isc_camera_control_configuration->enabled_camera;
	isc_camera_control_config_.isc_camera_model = isc_camera_control_configuration->isc_camera_model;

	swprintf_s(isc_camera_control_config_.save_image_path, L"%s", isc_camera_control_configuration->save_image_path);
	swprintf_s(isc_camera_control_config_.load_image_path, L"%s", isc_camera_control_configuration->load_image_path);

	isc_save_data_configuration_.max_save_folder_count = isc_save_data_configuration->max_save_folder_count;
	isc_save_data_configuration_.save_folder_count = isc_save_data_configuration->save_folder_count;
	for (int i = 0; i < isc_save_data_configuration_.save_folder_count; i++) {
		swprintf_s(isc_save_data_configuration_.save_folders[i], L"%s", isc_save_data_configuration->save_folders[i]);
	}
	isc_save_data_configuration_.minimum_capacity_required = isc_save_data_configuration->minimum_capacity_required;
	isc_save_data_configuration_.save_time_for_one_file = isc_save_data_configuration->save_time_for_one_file;

	camera_width_ = width;
	camera_height_ = height;

	// log
	isc_log_ = isc_log;

	// 保存先の確認、フォルダー作成

	// 空きディスクを確認して、OKなら保存対象とする
	// OKが０ならエラー
	
	int ok_count = 0;
	int disk_index = 0;
	for (int i = 0; i < isc_save_data_configuration_.save_folder_count; i++) {

		bool is_ok = CheckDiskFreeSpace(isc_save_data_configuration_.save_folders[i], isc_save_data_configuration_.minimum_capacity_required);

		if (is_ok) {
			ok_count++;

			swprintf_s(file_write_information_.root_folder[disk_index], L"%s", isc_save_data_configuration_.save_folders[i]);
			swprintf_s(file_write_information_.write_folder[disk_index], L"%s", isc_save_data_configuration_.save_folders[i]);
			disk_index++;
		}
		else {
			swprintf_s(logMag, L"This disk cannot be used due to lack of free space!\n");
			isc_log_->LogError(L"IscFileWriteControlImpl", logMag);
		}
	}

	if (ok_count == 0) {
		// エラー
		swprintf_s(logMag, L"There is not enough free space in the save destination!\n");
		isc_log_->LogError(L"IscFileWriteControlImpl", logMag);

		return CAMCONTROL_E_NOT_ENOUGH_FREE_SPACE;
	}

	// フォルダー作成
	file_write_information_.target_folder_count = ok_count;
	file_write_information_.current_folder_index = 0;
	
	for (int i = 0; i < file_write_information_.target_folder_count; i++) {

		wchar_t save_folder_name[_MAX_PATH] = {};
		swprintf_s(save_folder_name, L"%s", file_write_information_.write_folder[i]);
		
		if (::PathFileExists(save_folder_name) && !::PathIsDirectory(save_folder_name)) {
			// 指定されたパスにファイルが存在、かつディレクトリでない -> NG
			swprintf_s(logMag, L"The save destination is invalid! There is a file with the same name\n");
			isc_log_->LogError(L"IscFileWriteControlImpl", logMag);

			return CAMCONTROL_E_INVALID_SAVE_FOLDER;
		}
		else if (::PathFileExists(save_folder_name)) {
			// 指定されたパスがディレクトリである -> OK
		}
		else {
			// 作成する
			int ret = SHCreateDirectoryEx(NULL, save_folder_name, NULL);
			if (ret == ERROR_SUCCESS) {
				// OK
			}
			else {
				// Failed
				swprintf_s(logMag, L"Unable to create destination folder\n");
				isc_log_->LogError(L"IscFileWriteControlImpl", logMag);

				return CAMCONTROL_E_INVALID_SAVE_FOLDER;
			}
		}
	}

	file_write_information_.minimum_capacity_required = isc_save_data_configuration_.minimum_capacity_required;

	file_write_information_.start_time_of_current_file_msec = 0i64;
	file_write_information_.save_time_for_one_file_sec = isc_save_data_configuration_.save_time_for_one_file * 60;	// minutes -> second

	file_write_information_.previous_time_free_space_monitoring = 0i64;
	file_write_information_.free_space_monitoring_cycle_sec = 1 * 60;	// minutes -> second

	/*
		pre-allocated
			予めDiskに一定の大きさのファイルを作成します
			暫定で2GBとします
			Create a file of a certain size on disk in advance
			Temporarily set it to 2GB

		VM：
		752x480x2 -> 1 raw data size
		752x480x2x60=42MB/sec

		XC:
		1280x720x2 -> 1 raw data size
		1280x720x2x60=106MB/sec

		4K:
		3840x11920x2 -> 1 raw data size
		3840x11920x2x60=844MB/sec
	*/
	file_write_information_.initial_size = 2i64 * 1024i64 * 1024i64 * 1024i64;	

	file_write_information_.handle_file = NULL;
	file_write_information_.is_file_ready = false;
	file_write_information_.frame_index = 0;
	memset(&file_write_information_.raw_file_hedaer, 0, sizeof(file_write_information_.raw_file_hedaer));

	// get buffer
	isc_image_info_ring_buffer_ = new IscImageInfoRingBuffer;
	constexpr int max_buffer_count = 16;
	isc_image_info_ring_buffer_->Initialize(true, true, max_buffer_count, camera_width_, camera_height_);
	isc_image_info_ring_buffer_->Clear();

	// utility
	utility_measure_time_ = new UtilityMeasureTime;
	utility_measure_time_->Init();

	// clear thread control
	thread_control_.terminate_request = 0;
	thread_control_.terminate_done = 0;
	thread_control_.end_code = 0;
	thread_control_.stop_request = false;

	// initalize semaphore
	char semaphoreName[64] = {};
	sprintf_s(semaphoreName, "THREAD_SEMAPHORENAME_ISCCAMERAERITW%d", 0);
	handle_semaphore_ = CreateSemaphoreA(NULL, 0, 8, semaphoreName);
	if (handle_semaphore_ == NULL) {
		// Fail
		swprintf_s(logMag, L"failed to create semaphore\n");
		isc_log_->LogError(L"IscFileWriteControlImpl", logMag);

		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	InitializeCriticalSection(&threads_critical_);

	return DPC_E_OK;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::Terminate()
{

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

	delete utility_measure_time_;
	utility_measure_time_ = nullptr;

	isc_log_ = nullptr;

	return DPC_E_OK;
}

/**
 * ファイルへの書き込みを開始します
 *
 * @param[in] camera_specific_parameter カメラの固有パラメータ
 * @param[in] isc_grab_start_mode カメラの取り込みモード
 * @param[in] shutter_mode 現在のシャッター制御モード
 *
 * @retval 0 成功
 * @retval other 失敗
 * @note 入力は、ファイルのヘッダー情報に保存されます
 */
int IscFileWriteControlImpl::Start(const IscCameraSpecificParameter* camera_specific_parameter, const IscGrabStartMode* isc_grab_start_mode, const IscShutterMode shutter_mode)
{

	if (file_write_information_.is_file_ready == true) {
		return CAMCONTROL_E_SART_SAVE_FAILED;
	}

	if (thread_handle_ != NULL) {
		return CAMCONTROL_E_SART_SAVE_FAILED;
	}
	
	// 保存File準備
	SYSTEMTIME st = {};
	wchar_t date_time_name[_MAX_PATH] = {};
	GetLocalTime(&st);

	// YYYYMMDD_HHMMSS
	swprintf_s(date_time_name, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	// make file name
	int index = file_write_information_.current_folder_index;
	swprintf_s(file_write_information_.write_file_name, L"%s\\%s.dat", file_write_information_.write_folder[index], date_time_name);

	// prepare write header to file
	memset(&file_write_information_.raw_file_hedaer, 0, sizeof(file_write_information_.raw_file_hedaer));

	switch (isc_camera_control_config_.isc_camera_model) {
	case::IscCameraModel::kVM:
		sprintf_s(file_write_information_.raw_file_hedaer.mark, "VM RAW DATA");
		file_write_information_.raw_file_hedaer.camera_model = 0;
		break;
	case::IscCameraModel::kXC:
		sprintf_s(file_write_information_.raw_file_hedaer.mark, "XC RAW DATA");
		file_write_information_.raw_file_hedaer.camera_model = 1;
		break;
	case::IscCameraModel::k4K:
		sprintf_s(file_write_information_.raw_file_hedaer.mark, "4K RAW DATA");
		file_write_information_.raw_file_hedaer.camera_model = 2;
		break;
	case::IscCameraModel::k4KA:
		sprintf_s(file_write_information_.raw_file_hedaer.mark, "4KA RAW DATA");
		file_write_information_.raw_file_hedaer.camera_model = 3;
		break;
	case::IscCameraModel::k4KJ:
		sprintf_s(file_write_information_.raw_file_hedaer.mark, "4KJ RAW DATA");
		file_write_information_.raw_file_hedaer.camera_model = 4;
		break;
	case::IscCameraModel::kUnknown:
		sprintf_s(file_write_information_.raw_file_hedaer.mark, "Unknown RAW DATA");
		file_write_information_.raw_file_hedaer.camera_model = 99;
		break;
	default:
		sprintf_s(file_write_information_.raw_file_hedaer.mark, "Unknown RAW DAT");
		file_write_information_.raw_file_hedaer.camera_model = 99;
		break;
	}

	file_write_information_.raw_file_hedaer.version = ISC_ROW_FILE_HEADER_VERSION;
	file_write_information_.raw_file_hedaer.hedaer_size = sizeof(IscRawFileHeader);
	file_write_information_.raw_file_hedaer.max_width = camera_width_;
	file_write_information_.raw_file_hedaer.max_height = camera_height_;
	file_write_information_.raw_file_hedaer.d_inf = camera_specific_parameter->d_inf;
	file_write_information_.raw_file_hedaer.bf = camera_specific_parameter->bf;
	file_write_information_.raw_file_hedaer.dz = camera_specific_parameter->dz;
	file_write_information_.raw_file_hedaer.base_length = camera_specific_parameter->base_length;

	switch(isc_grab_start_mode->isc_grab_mode) {
	case IscGrabMode::kParallax:
		file_write_information_.raw_file_hedaer.grab_mode = 1;
		break;
	case IscGrabMode::kCorrect:
		file_write_information_.raw_file_hedaer.grab_mode = 2;
		break;
	case IscGrabMode::kBeforeCorrect:
		file_write_information_.raw_file_hedaer.grab_mode = 3;
		break;
	case IscGrabMode::kBayerBase:
		file_write_information_.raw_file_hedaer.grab_mode = 4;
		break;
	case IscGrabMode::kBayerCompare:
		file_write_information_.raw_file_hedaer.grab_mode = 5;
		break;
	default:
		file_write_information_.raw_file_hedaer.grab_mode = -1;
		break;
	}

	switch (shutter_mode) {
	case IscShutterMode::kManualShutter:
		file_write_information_.raw_file_hedaer.shutter_mode = 0;
		break;
	case IscShutterMode::kSingleShutter:
		file_write_information_.raw_file_hedaer.shutter_mode = 1;
		break;
	case IscShutterMode::kDoubleShutter:
		file_write_information_.raw_file_hedaer.shutter_mode = 2;
		break;
	case IscShutterMode::kDoubleShutter2:
		file_write_information_.raw_file_hedaer.shutter_mode = 3;
		break;
	default:
		file_write_information_.raw_file_hedaer.shutter_mode = -1;
		break;
	}

	switch (isc_grab_start_mode->isc_grab_color_mode) {
	case IscGrabColorMode::kColorOFF:
		file_write_information_.raw_file_hedaer.color_mode = 0;
		break;
	case IscGrabColorMode::kColorON:
		file_write_information_.raw_file_hedaer.color_mode = 1;
		break;
	default:
		file_write_information_.raw_file_hedaer.color_mode = 0;
		break;
	}

	file_write_information_.start_time_of_current_file_msec = GetTickCount64();
	file_write_information_.previous_time_free_space_monitoring = GetTickCount64();

	// clear thread control
	thread_control_.terminate_request = 0;
	thread_control_.terminate_done = 0;
	thread_control_.end_code = 0;
	thread_control_.stop_request = false;

	// it start write thread
	if ((thread_handle_ = (HANDLE)_beginthreadex(0, 0, ControlThread, (void*)this, 0, 0)) == 0) {
		// Fail
		return CAMCONTROL_E_INVALID_DEVICEHANDLE;
	}

	// THREAD_PRIORITY_TIME_CRITICAL
	// THREAD_PRIORITY_HIGHEST +2
	// THREAD_PRIORITY_ABOVE_NORMAL +1
	// THREAD_PRIORITY_NORMAL  +0
	// THREAD_PRIORITY_BELOW_NORMAL -1
	SetThreadPriority(thread_handle_, THREAD_PRIORITY_BELOW_NORMAL);

	return DPC_E_OK;
}

/**
 * ファイルへの書き込みデータを追加します
 *
 * @param[in] isc_image_info 書き込みデータ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::Add(IscImageInfo* isc_image_info)
{
	if (!file_write_information_.is_file_ready) {
		return DPC_E_OK;
	}

	if (isc_image_info_ring_buffer_ == nullptr) {
		return DPC_E_OK;
	}

	//wchar_t msg[128] = {};
	//swprintf_s(msg, L"[INFO]Add frame(%d)\n"), isc_image_info->frameNo);
	//OutputDebugString(msg);

	// Only RAW data is copied here.
	IscImageInfoRingBuffer::BufferData* buffer_data = nullptr;
	const ULONGLONG time = GetTickCount64();
	int put_index = isc_image_info_ring_buffer_->GetPutBuffer(&buffer_data, time);
	int image_status = 0;

	if (put_index >= 0 && buffer_data != nullptr) {

		buffer_data->isc_image_info.frameNo = isc_image_info->frameNo;
		buffer_data->isc_image_info.gain = isc_image_info->gain;
		buffer_data->isc_image_info.exposure = isc_image_info->exposure;

		buffer_data->isc_image_info.grab = isc_image_info->grab;
		buffer_data->isc_image_info.color_grab_mode = isc_image_info->color_grab_mode;
		buffer_data->isc_image_info.shutter_mode = isc_image_info->shutter_mode;
		buffer_data->isc_image_info.camera_specific_parameter.d_inf = isc_image_info->camera_specific_parameter.d_inf;
		buffer_data->isc_image_info.camera_specific_parameter.bf = isc_image_info->camera_specific_parameter.bf;
		buffer_data->isc_image_info.camera_specific_parameter.base_length = isc_image_info->camera_specific_parameter.base_length;
		buffer_data->isc_image_info.camera_specific_parameter.dz = isc_image_info->camera_specific_parameter.dz;

		buffer_data->isc_image_info.camera_status.error_code = isc_image_info->camera_status.error_code;
		buffer_data->isc_image_info.camera_status.data_receive_tact_time = isc_image_info->camera_status.data_receive_tact_time;

		buffer_data->isc_image_info.p1.width = 0;
		buffer_data->isc_image_info.p1.height = 0;
		buffer_data->isc_image_info.p1.channel_count = 0;

		buffer_data->isc_image_info.p2.width = 0;
		buffer_data->isc_image_info.p2.height = 0;
		buffer_data->isc_image_info.p2.channel_count = 0;

		buffer_data->isc_image_info.color.width = 0;
		buffer_data->isc_image_info.color.height = 0;
		buffer_data->isc_image_info.color.channel_count = 0;

		buffer_data->isc_image_info.depth.width = 0;
		buffer_data->isc_image_info.depth.height = 0;

		buffer_data->isc_image_info.raw.width = isc_image_info->raw.width;
		buffer_data->isc_image_info.raw.height = isc_image_info->raw.height;
		buffer_data->isc_image_info.raw.channel_count = isc_image_info->raw.channel_count;
		size_t cp_size = isc_image_info->raw.width * isc_image_info->raw.height * isc_image_info->raw.channel_count;
		if (cp_size > 0) {
			memcpy(buffer_data->isc_image_info.raw.image, isc_image_info->raw.image, cp_size);
		}

		image_status = 1;

		// start processing thread
		BOOL result_release = ReleaseSemaphore(handle_semaphore_, 1, NULL);
		if (!result_release) {
			// over flow 
			
			//DWORD last_error = GetLastError();
			//char error_msg[256] = {};
			//sprintf_s(error_msg, "[ERROR] IscFileWriteControlImpl::Add ReleaseSemaphore failed(%d)\n", last_error);
			//OutputDebugStringA(error_msg);
		}
	}

	isc_image_info_ring_buffer_->DonePutBuffer(put_index, image_status);

	return DPC_E_OK;
}

/**
 * ファイルへの書き込みを終了します
 *
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::Stop()
{

	file_write_information_.is_file_ready = false;

	if (thread_handle_ != NULL) {
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
	}

	return DPC_E_OK;
}

/**
 * threadの動作状態を取得します
 *
 * @param[out] is_running true:Threadは動作中です
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::QueryThreadStatus(bool* is_running)
{

	if (thread_control_.terminate_done == 0) {
		*is_running = true;
	}
	else {
		*is_running = false;
	}

	return thread_control_.end_code;
}

/**
 * 書き込みファイルを準備します(初回)
 *
 * @param[in] file_write_information 書き込みファイルの情報です
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::PrepareFileforWriting(FileWriteInformation* file_write_information)
{
	wchar_t logMag[128] = {};

	int ret = CreateWriteFile(file_write_information);
	if (ret != DPC_E_OK) {
		swprintf_s(logMag, L"CreateWriteFile() error(0X%08X)\n", ret);
		isc_log_->LogError(L"IscFileWriteControlImpl::PrepareFileforWriting", logMag);

		return ret;
	}

	__int64 number_of_write_to_file = sizeof(IscRawFileHeader);
	__int64 number_of_byteswritten = 0i64;
	ret = WriteDataToFile(file_write_information->handle_file, (LPCVOID)&file_write_information->raw_file_hedaer, number_of_write_to_file, &number_of_byteswritten, NULL);
	if (ret != DPC_E_OK) {
		swprintf_s(logMag, L"WriteDataToFile() error(%d)\n", ret);
		isc_log_->LogError(L"IscFileWriteControlImpl::PrepareFileforWriting", logMag);

		return ret;
	}

	file_write_information->frame_index = 0;
	file_write_information->is_file_ready = true;

	return DPC_E_OK;
}

/**
 * ファイルの特権昇格処理を行います
 *
 * @param[in] privilege_name 特権名を取得するシステムの名前
 * @param[in] enabled LUID の属性を指定します true:SE_PRIVILEGE_ENABLED(特権が有効)
 * @retval 0 成功
 * @retval other 失敗
 */
HRESULT IscFileWriteControlImpl::EnablePrivilege(const wchar_t* privilege_name, const BOOL enabled)
{
	BOOL ret = FALSE;

	HANDLE handle_token;

	/*
		open the access token associated with the process
	*/
	ret = ::OpenProcessToken(
								GetCurrentProcess()
								, 0
								//          | TOKEN_ASSIGN_PRIMARY
								//          | TOKEN_DUPLICATE
								//          | TOKEN_IMPERSONATE
								| TOKEN_QUERY
								//          | TOKEN_QUERY_SOURCE
								| TOKEN_ADJUST_PRIVILEGES
								//          | TOKEN_ADJUST_GROUPS
								//          | TOKEN_ADJUST_DEFAULT
								//          | TOKEN_ADJUST_SESSIONID
								, &handle_token);

	if (0 == ret) {
		// error
		return(::HRESULT_FROM_WIN32(::GetLastError()));
	}

	LUID tLuid;

	/*
		Get the Locally Unique Identifier (LUID) used by the specified system
	*/
	ret = LookupPrivilegeValue(NULL, privilege_name, &tLuid);
	if (0 == ret) {
		// error
		DWORD last_error = ::GetLastError();
		::CloseHandle(handle_token);
		return(::HRESULT_FROM_WIN32(last_error));
	}

	TOKEN_PRIVILEGES token_privileges;
	token_privileges.PrivilegeCount = 1;
	token_privileges.Privileges[0].Luid = tLuid;
	token_privileges.Privileges[0].Attributes = (FALSE != enabled) ? SE_PRIVILEGE_ENABLED : 0;

	/*
		Set privileges within the specified access token
	*/
	ret = ::AdjustTokenPrivileges(
									handle_token
									, FALSE
									, &token_privileges
									, sizeof(token_privileges)
									, NULL
									, NULL
	);
	if (0 == ret) {
		// error
		DWORD last_error = ::GetLastError();
		::CloseHandle(handle_token);
		return(::HRESULT_FROM_WIN32(last_error));
	}

	// close the token handle
	::CloseHandle(handle_token);

	return(S_OK);
}

/**
 * 書き込みファイルを実際に作成します(初回)
 *
 * @param[in] file_write_information 書き込みファイルの情報です
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::CreateWriteFile(FileWriteInformation* file_write_information)
{
	wchar_t logMag[128] = {};

	// create file
	if ((file_write_information->handle_file = CreateFile(	file_write_information->write_file_name,
															GENERIC_WRITE,
															FILE_SHARE_READ,
															NULL,
															CREATE_ALWAYS,
															FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
															NULL)) == INVALID_HANDLE_VALUE) {
		DWORD gs_error = GetLastError();
		//wchar_t msg[64] = {};
		//swprintf_s(msg, L"[ERROR]CreateWriteFile() error(%d) %s", gs_error, file_write_information->write_file_name);
		//MessageBox(NULL, msg, L"IscFileWriteControlImpl::CreateWriteFile()", MB_ICONERROR);

		swprintf_s(logMag, L"CreateWriteFile() error(%d) %s\n", gs_error, file_write_information->write_file_name);
		isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

		return CAMCONTROL_E_CREATE_SAVE_FILE;
	}


	// pre allocate file
	if (file_write_information->initial_size > 0) {
		LARGE_INTEGER new_pointer = {};

		LARGE_INTEGER  distance_to_move = {};
		distance_to_move.QuadPart = (LONGLONG)file_write_information->initial_size;
		DWORD move_method = FILE_BEGIN;

		// 先までファイルポインタを進める
		if (!SetFilePointerEx(file_write_information->handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}
		
		// EOFを設定
		if (!SetEndOfFile(file_write_information->handle_file)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetEndOfFile error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() SetEndOfFile error(%d)\n", gs_error);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE; 
		}

		// Enable privilege ( SE_MANAGE_VOLUME_NAME )
		DWORD hr_result = EnablePrivilege(SE_MANAGE_VOLUME_NAME, TRUE);
		if (hr_result != S_OK) {
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() EnablePrivilege error(%d)\n", hr_result);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() EnablePrivilege error(%d)\n", hr_result);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}

		// 先頭までファイルポインタを移動
		distance_to_move.QuadPart = (LONGLONG)0;
		if (!SetFilePointerEx(file_write_information->handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}

		// EOFの位置までを有効なデータとする
		/*
			私は、SetFileValidDataで発生するエラー（1314)を解決できません。
			そのため、初めの書き込みでパフォーマンスが低下します。
			将来において、これを解決する予定です。
			I can't resolve the error (1314) I get with SetFileValidData.
			So the first write will have poor performance.
			We plan to resolve this in the future.
		*/
		/*
		if (!SetFileValidData(file_write_information->handle_file, (LONGLONG)file_write_information->initial_size)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}
		*/

		// 先頭までファイルポインタを移動
		distance_to_move.QuadPart = (LONGLONG)0;
		if (!SetFilePointerEx(file_write_information->handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}
	}

	return DPC_E_OK;
}

/**
 * 書き込みファイルを準備します(更新)
 *
 * @param[in] file_write_information 書き込みファイルの情報です
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::PrepareNewFileforWriting(FileWriteInformation* file_write_information)
{
	wchar_t logMag[128] = {};

	int ret = CreateNewWriteFile(file_write_information);
	if (ret != DPC_E_OK) {
		swprintf_s(logMag, L"CreateWriteFile() error(0X%08X)\n", ret);
		isc_log_->LogError(L"IscFileWriteControlImpl::PrepareNewFileforWriting", logMag);

		return ret;
	}

	__int64 number_of_write_to_file = sizeof(IscRawFileHeader);
	__int64 number_of_byteswritten = 0i64;
	ret = WriteDataToFile(file_write_information->handle_file, (LPCVOID)&file_write_information->raw_file_hedaer, number_of_write_to_file, &number_of_byteswritten, NULL);
	if (ret != DPC_E_OK) {
		swprintf_s(logMag, L"WriteDataToFile() error(%d)\n", ret);
		isc_log_->LogError(L"IscFileWriteControlImpl::PrepareNewFileforWriting", logMag);

		return ret;
	}

	file_write_information->frame_index = 0;
	//file_write_information->is_file_ready = true;

	return DPC_E_OK;
}

/**
 * 書き込みファイルを実際に作成します(更新)
 *
 * @param[in] file_write_information 書き込みファイルの情報です
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::CreateNewWriteFile(FileWriteInformation* file_write_information)
{
	// 現在のファイルを閉じて新しいファイルにする
	wchar_t logMag[128] = {};

	// close file to write
	if (file_write_information->handle_file != NULL) {
		// truncate Pre-Allocated disk space
		SetEndOfFile(file_write_information->handle_file);

		CloseHandle(file_write_information->handle_file);
		file_write_information->handle_file = NULL;
	}

	// new file

	// 空き容量の確認
	int index = file_write_information->current_folder_index;
	bool is_ok = CheckDiskFreeSpace(file_write_information->write_folder[index], file_write_information->minimum_capacity_required);

	if (!is_ok) {
		swprintf_s(logMag, L"Your current disk is out of free space. %s\n", file_write_information->write_folder[index]);
		isc_log_->LogError(L"IscFileWriteControlImpl::CreateNewWriteFile", logMag);

		// 次のフォルダ
		if (file_write_information->current_folder_index < (file_write_information->target_folder_count - 1)) {
			file_write_information->current_folder_index++;
			index = file_write_information->current_folder_index;

			swprintf_s(logMag, L"Check the free space on the following disks. %s\n", file_write_information->write_folder[index]);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateNewWriteFile", logMag);

			bool is_ok2 = CheckDiskFreeSpace(file_write_information->write_folder[index], file_write_information->minimum_capacity_required);
			if (!is_ok2) {
				// 保存できない
				swprintf_s(logMag, L"Insufficient free space. %s\n", file_write_information->write_folder[index]);
				isc_log_->LogError(L"IscFileWriteControlImpl::CreateNewWriteFile", logMag);

				return CAMCONTROL_E_NOT_ENOUGH_FREE_SPACE;
			}
		}
		else {
			// 保存できない
			swprintf_s(logMag, L"There is no next destination.\n");
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateNewWriteFile", logMag);

			return CAMCONTROL_E_NOT_ENOUGH_FREE_SPACE;
		}
	}

	// 保存Fil
	SYSTEMTIME st = {};
	wchar_t date_time_name[_MAX_PATH] = {};
	GetLocalTime(&st);

	// YYYYMMDD_HHMMSS
	swprintf_s(date_time_name, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	// make file name
	index = file_write_information->current_folder_index;
	swprintf_s(file_write_information->write_file_name, L"%s\\%s.dat", file_write_information->write_folder[index], date_time_name);

	// create file
	if ((file_write_information->handle_file = CreateFile(	file_write_information->write_file_name,
															GENERIC_WRITE,
															FILE_SHARE_READ,
															NULL,
															CREATE_ALWAYS,
															FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
															NULL)) == INVALID_HANDLE_VALUE) {
		DWORD gs_error = GetLastError();
		//wchar_t msg[64] = {};
		//swprintf_s(msg, L"[ERROR]CreateWriteFile() error(%d) %s", gs_error, file_write_information->write_file_name);
		//MessageBox(NULL, msg, L"IscFileWriteControlImpl::CreateNewWriteFile()", MB_ICONERROR);

		swprintf_s(logMag, L"CreateWriteFile() error(%d) %s\n", gs_error, file_write_information->write_file_name);
		isc_log_->LogError(L"IscFileWriteControlImpl::CreateNewWriteFile", logMag);

		return CAMCONTROL_E_CREATE_SAVE_FILE;
	}


	// pre allocate file
	if (file_write_information->initial_size > 0) {
		LARGE_INTEGER new_pointer = {};

		LARGE_INTEGER  distance_to_move = {};
		distance_to_move.QuadPart = (LONGLONG)file_write_information->initial_size;
		DWORD move_method = FILE_BEGIN;

		// 先までファイルポインタを進める
		if (!SetFilePointerEx(file_write_information->handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}

		// EOFを設定
		if (!SetEndOfFile(file_write_information->handle_file)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetEndOfFile error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() SetEndOfFile error(%d)\n", gs_error);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}

		// Enable privilege ( SE_MANAGE_VOLUME_NAME )
		DWORD hr_result = EnablePrivilege(SE_MANAGE_VOLUME_NAME, TRUE);
		if (hr_result != S_OK) {
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() EnablePrivilege error(%d)\n", hr_result);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() EnablePrivilege error(%d)\n", hr_result);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}

		// 先頭までファイルポインタを移動
		distance_to_move.QuadPart = (LONGLONG)0;
		if (!SetFilePointerEx(file_write_information->handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}

		// EOFの位置までを有効なデータとする
		/*
			私は、SetFileValidDataで発生するエラー（1314)を解決できません。
			そのため、初めの書き込みでパフォーマンスが低下します。
			将来において、これを解決する予定です。
			I can't resolve the error (1314) I get with SetFileValidData.
			So the first write will have poor performance.
			We plan to resolve this in the future.
		*/
		/*
		if (!SetFileValidData(file_write_information->handle_file, (LONGLONG)file_write_information->initial_size)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}
		*/

		// 先頭までファイルポインタを移動
		distance_to_move.QuadPart = (LONGLONG)0;
		if (!SetFilePointerEx(file_write_information->handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			swprintf_s(logMag, L"CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			isc_log_->LogError(L"IscFileWriteControlImpl::CreateWriteFile", logMag);

			CloseHandle(file_write_information->handle_file);
			file_write_information->handle_file = NULL;
			return CAMCONTROL_E_CREATE_SAVE_FILE;
		}
	}

	return DPC_E_OK;
}

/**
 * ファイル書き込みを行います
 *
 * @param[in] handle_file 書き込みファイルハンドル
 * @param[in] buffer 書き込みデータバッファー
 * @param[in] number_of_write_to_file 書き込みバイト数
 * @param[out] number_of_byteswritten 実際に書き込んだバイト数
 * @param[in] overlapped Overlap構造体(=NULLで呼び出すこと)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::WriteDataToFile(HANDLE handle_file, LPCVOID buffer, __int64 number_of_write_to_file, __int64* number_of_byteswritten, LPOVERLAPPED overlapped)
{
	if (handle_file == NULL) {
		return CAMCONTROL_E_WRITE_FAILED;
	}
	
	DWORD number_of_write = (DWORD)number_of_write_to_file;
	DWORD number_of_written = 0;

	BOOL ret = WriteFile(handle_file, buffer, number_of_write, &number_of_written, overlapped);
	if (!ret) {
		return CAMCONTROL_E_WRITE_FAILED;
	}
	
	if (number_of_written != 0) {
		*number_of_byteswritten = (__int64)number_of_written;
	}
	else {
		return CAMCONTROL_E_WRITE_FAILED;
	}

	return DPC_E_OK;
}

/**
 * ディスクの空き容量を確認します
 *
 * @param[in] file_write_information 確認用のパラメータ
 * @retval 0 成功
 * @retval other 失敗/保存できない
 */
int IscFileWriteControlImpl::CheckFreeSpace(FileWriteInformation* file_write_information)
{
	// 空き容量の確認
	int index = file_write_information->current_folder_index;
	bool is_ok = CheckDiskFreeSpace(file_write_information->write_folder[index], file_write_information->minimum_capacity_required);

	if (!is_ok) {
		// 次のフォルダ
		if (file_write_information->current_folder_index < (file_write_information->target_folder_count - 1)) {
			file_write_information->current_folder_index++;

			index = file_write_information->current_folder_index;
			bool is_ok2 = CheckDiskFreeSpace(file_write_information->write_folder[index], file_write_information->minimum_capacity_required);
			if (!is_ok) {
				// 保存できない
				return CAMCONTROL_E_NOT_ENOUGH_FREE_SPACE;
			}
		}
		else {
			// 保存できない
			return CAMCONTROL_E_NOT_ENOUGH_FREE_SPACE;
		}
	}

	return DPC_E_OK;
}

/**
 * 保存Thread
 *
 * @param[in] context Threadパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
unsigned __stdcall IscFileWriteControlImpl::ControlThread(void* context)
{
	IscFileWriteControlImpl* isc_file_write_Control = (IscFileWriteControlImpl*)context;

	if (isc_file_write_Control == nullptr) {
		return -1;
	}

	int ret = isc_file_write_Control->WriteDataProc(isc_file_write_Control);

	return ret;
}

/**
 * 保存Thread　処理本体
 *
 * @param[in] isc_file_write_Control Threadパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileWriteControlImpl::WriteDataProc(IscFileWriteControlImpl* isc_file_write_Control)
{
	wchar_t logMag[128] = {};
	isc_file_write_Control->thread_control_.end_code = 0;

	// prepare the file for writing
	swprintf_s(logMag, L"prepare file for writing file=%s\n", isc_file_write_Control->file_write_information_.write_file_name);
	isc_log_->LogInfo(L"IscFileWriteControlImpl::WriteDataProc", logMag);

	int ret = PrepareFileforWriting(&isc_file_write_Control->file_write_information_);
	if (ret != DPC_E_OK) {
		//wchar_t msg[1024] = {};
		//swprintf_s(msg, L"[ERROR]Failed to create save file code=0X%08X %s", ret, isc_file_write_Control->file_write_information_.write_file_name);
		//MessageBox(NULL, msg, L"IscFileWriteControlImpl::WriteDataProc()", MB_ICONERROR);

		swprintf_s(logMag, L"Failed to create save file code=0X%08X %s\n", ret, isc_file_write_Control->file_write_information_.write_file_name);
		isc_log_->LogError(L"IscFileWriteControlImpl::WriteDataProc", logMag);

		isc_file_write_Control->thread_control_.end_code = ret;
		return -1;
	}

	IscRawDataHeader isc_raw_data_header = {};
	isc_raw_data_header.version = ISC_ROW_DATA_HEADER_VERSION;
	isc_raw_data_header.hedaer_size = sizeof(IscRawDataHeader);

	__int64 number_of_write_to_file = 0;
	__int64 number_of_byteswritten = 0;

	while (isc_file_write_Control->thread_control_.terminate_request < 1) {

		// Wait for start
		DWORD wait_result = WaitForSingleObject(isc_file_write_Control->handle_semaphore_, INFINITE);

		if (isc_file_write_Control->thread_control_.stop_request) {
			isc_file_write_Control->thread_control_.stop_request = false;
			break;
		}

		if (wait_result == WAIT_OBJECT_0) {
			isc_file_write_Control->utility_measure_time_->Start();

			// 時間経過を確認し、ファイルを変更する
			__int64 now_time_msec = GetTickCount64();
			__int64 elapsed_time_msec = now_time_msec - (isc_file_write_Control->file_write_information_.start_time_of_current_file_msec);
			__int64 time_limit_msec = isc_file_write_Control->file_write_information_.save_time_for_one_file_sec * 1000;
			if (elapsed_time_msec > time_limit_msec) {
				swprintf_s(logMag, L"change files over time\n");
				isc_log_->LogInfo(L"IscFileWriteControlImpl::WriteDataProc", logMag);

				// ファイル変更
				int ret_new = PrepareNewFileforWriting(&isc_file_write_Control->file_write_information_);
				if (ret_new != DPC_E_OK) {
					//wchar_t msg[1024] = {};
					//swprintf_s(msg, L"[ERROR]Failed to create new save file code=0X%08X %s", ret_new, isc_file_write_Control->file_write_information_.write_file_name);
					//MessageBox(NULL, msg, L"IscFileWriteControlImpl::WriteDataProc()", MB_ICONERROR);

					swprintf_s(logMag, L"Failed to create new save file code=0X%08X %s\n", ret_new, isc_file_write_Control->file_write_information_.write_file_name);
					isc_log_->LogError(L"IscFileWriteControlImpl::WriteDataProc", logMag);

					isc_file_write_Control->thread_control_.end_code = ret_new;
					break;
				}
				else {
					swprintf_s(logMag, L"Successfully changed the save destination %s\n", isc_file_write_Control->file_write_information_.write_file_name);
					isc_log_->LogInfo(L"IscFileWriteControlImpl::WriteDataProc", logMag);
				}

				file_write_information_.start_time_of_current_file_msec = GetTickCount64();
			}

			elapsed_time_msec = now_time_msec - isc_file_write_Control->file_write_information_.previous_time_free_space_monitoring;
			time_limit_msec = isc_file_write_Control->file_write_information_.free_space_monitoring_cycle_sec * 1000;
			if (elapsed_time_msec > time_limit_msec) {
				// 空き容量の監視
				int check_ret = CheckFreeSpace(&isc_file_write_Control->file_write_information_);
				if (check_ret != DPC_E_OK) {
					//wchar_t msg[1024] = {};
					//swprintf_s(msg, L"[ERROR]Insufficient free space. Cannot save. code=0X%08X %s", check_ret, isc_file_write_Control->file_write_information_.write_file_name);
					//MessageBox(NULL, msg, L"IscFileWriteControlImpl::WriteDataProc()", MB_ICONERROR);

					swprintf_s(logMag, L"Insufficient free space. Cannot save. code=0X%08X %s\n", check_ret, isc_file_write_Control->file_write_information_.write_file_name);
					isc_log_->LogError(L"IscFileWriteControlImpl::WriteDataProc", logMag);

					swprintf_s(logMag, L"Search for another storage location\n");
					isc_log_->LogError(L"IscFileWriteControlImpl::WriteDataProc", logMag);

					// ファイル変更
					int ret_new = PrepareNewFileforWriting(&isc_file_write_Control->file_write_information_);
					if (ret_new != DPC_E_OK) {
						swprintf_s(logMag, L"Failed to create new save file code=0X%08X %s\n", ret_new, isc_file_write_Control->file_write_information_.write_file_name);
						isc_log_->LogError(L"IscFileWriteControlImpl::WriteDataProc", logMag);

						isc_file_write_Control->thread_control_.end_code = ret_new;
						break;

					}
					else {
						swprintf_s(logMag, L"Successfully changed the save destination %s\n", isc_file_write_Control->file_write_information_.write_file_name);
						isc_log_->LogInfo(L"IscFileWriteControlImpl::WriteDataProc", logMag);

						file_write_information_.start_time_of_current_file_msec = GetTickCount64();
					}
				}

				file_write_information_.previous_time_free_space_monitoring = GetTickCount64();
			}

			// get data
			IscImageInfoRingBuffer::BufferData* image_info_buffer_data = nullptr;
			ULONGLONG time = 0;
			int get_index = isc_file_write_Control->isc_image_info_ring_buffer_->GetGetBuffer(&image_info_buffer_data, &time);

			if (get_index >= 0) {
				// there is an image
				int data_size = image_info_buffer_data->isc_image_info.raw.width * image_info_buffer_data->isc_image_info.raw.height * image_info_buffer_data->isc_image_info.raw.channel_count;
				isc_raw_data_header.data_size = data_size;
				isc_raw_data_header.compressed = 0;
				isc_raw_data_header.frame_index = (image_info_buffer_data->isc_image_info.frameNo == -1)? isc_file_write_Control->file_write_information_.frame_index: image_info_buffer_data->isc_image_info.frameNo;
				isc_raw_data_header.type = (image_info_buffer_data->isc_image_info.color_grab_mode == IscGrabColorMode::kColorON)? 2:1;
				isc_raw_data_header.status = 0;
				isc_raw_data_header.error_code = image_info_buffer_data->isc_image_info.camera_status.error_code;
				isc_raw_data_header.exposure = image_info_buffer_data->isc_image_info.exposure;
				isc_raw_data_header.gain = image_info_buffer_data->isc_image_info.gain;

				number_of_write_to_file = isc_raw_data_header.hedaer_size;
				number_of_byteswritten = 0;
				 
				int write_ret = WriteDataToFile(isc_file_write_Control->file_write_information_.handle_file,
												(LPCVOID*)&isc_raw_data_header,
												number_of_write_to_file,
												&number_of_byteswritten,
												NULL);
				if ((write_ret != DPC_E_OK) || (number_of_byteswritten == 0)) {
					//wchar_t msg[1024] = {};
					//swprintf_s(msg, L"[ERROR]Failed to write file code=0X%08X %s", write_ret, isc_file_write_Control->file_write_information_.write_file_name);
					//MessageBox(NULL, msg, L"IscFileWriteControlImpl::WriteDataProc()", MB_ICONERROR);
					
					swprintf_s(logMag, L"Failed to write file code=0X%08X %s\n", write_ret, isc_file_write_Control->file_write_information_.write_file_name);
					isc_log_->LogError(L"IscFileWriteControlImpl::WriteDataProc", logMag);

					isc_file_write_Control->thread_control_.end_code = CAMCONTROL_E_WRITE_FAILED;
					break;
				}

				number_of_write_to_file = data_size;
				number_of_byteswritten = 0;

				write_ret = WriteDataToFile(isc_file_write_Control->file_write_information_.handle_file,
											(LPCVOID*)image_info_buffer_data->isc_image_info.raw.image,
											number_of_write_to_file,
											&number_of_byteswritten,
											NULL);
				if ((write_ret != DPC_E_OK) || (number_of_byteswritten == 0)) {
					//wchar_t msg[1024] = {};
					//swprintf_s(msg, L"[ERROR]Failed to write file code=0X%08X %s", write_ret, isc_file_write_Control->file_write_information_.write_file_name);
					//MessageBox(NULL, msg, L"IscFileWriteControlImpl::WriteDataProc()", MB_ICONERROR);

					swprintf_s(logMag, L"Failed to write file code=0X%08X %s\n", write_ret, isc_file_write_Control->file_write_information_.write_file_name);
					isc_log_->LogError(L"IscFileWriteControlImpl::WriteDataProc", logMag);

					isc_file_write_Control->thread_control_.end_code = CAMCONTROL_E_WRITE_FAILED;
					break;
				}

				isc_file_write_Control->file_write_information_.frame_index++;
			}

			// ended
			isc_file_write_Control->isc_image_info_ring_buffer_->DoneGetBuffer(get_index);

			// debug
			double elapsed_time = isc_file_write_Control->utility_measure_time_->Stop();
			if (elapsed_time > 100.0) {
				swprintf_s(logMag, L"[WARN]elapsed_time > 100 %.2f\n", elapsed_time);
				OutputDebugString(logMag);
			}

		}
		else if (wait_result == WAIT_TIMEOUT) {
			// The semaphore was nonsignaled, so a time-out occurred.
		}
		else if (wait_result == WAIT_FAILED) {
			// error, abort
			// The thread got ownership of an abandoned mutex
			// The database is in an indeterminate state
			isc_file_write_Control->thread_control_.end_code = CAMCONTROL_E_WRITE_FAILED;
			break;
		}
	}


	// close file to write
	if (file_write_information_.handle_file != NULL) {
		// truncate Pre-Allocated disk space
		SetEndOfFile(file_write_information_.handle_file);

		CloseHandle(file_write_information_.handle_file);
		file_write_information_.handle_file = NULL;
	}

	isc_file_write_Control->thread_control_.terminate_done = 1;

	return 0;
}

/**
 * ディスクの空き容量を取得します
 *
 * @param[in] folder 取得先
 * @param[out] free_disk_space 空き容量
 * @retval 0 成功
 * @retval other 失敗
 */
bool IscFileWriteControlImpl::GetFreeDiskSpace(const wchar_t* folder, unsigned __int64* free_disk_space)
{
	// Decompose Path
	wchar_t drive[8] = {};
	wchar_t dir[_MAX_DIR] = {};
	wchar_t file[_MAX_DIR] = {};
	wchar_t ext[8] = {};

	_wsplitpath_s(folder, drive, dir, file, ext);

	wchar_t root_path_name[_MAX_DIR] = {};
	wchar_t volume_name_buffer[_MAX_DIR + 1] = {};
	DWORD volume_name_size = _MAX_DIR + 1;
	DWORD volume_serial_number = 0;
	DWORD maximum_component_length = 0;
	DWORD file_system_flags = 0;
	wchar_t file_system_name_buffer[_MAX_DIR + 1] = {};
	DWORD file_system_name_size = _MAX_DIR + 1;

	swprintf_s(root_path_name, L"%s\\", drive);

	// Error handle to application
	UINT oldErMode = SetErrorMode(SEM_FAILCRITICALERRORS);

	if (GetVolumeInformationW(
							root_path_name,
							volume_name_buffer,
							volume_name_size,
							&volume_serial_number,
							&maximum_component_length,
							&file_system_flags,
							file_system_name_buffer,
							file_system_name_size) != 0) {

							// OK
							// this drive exists

							// return
							SetErrorMode(oldErMode);
	}
	else {
		// error
		// I can not use it
		//wchar_t msg[256] = {};
		//swprintf_s(msg, L"[ERROR]Unable to access target drive(%s)", root_path_name);
		//MessageBoxW(NULL, msg, L"GetFreeDiskSpace()", MB_ICONERROR);

		// return
		SetErrorMode(oldErMode);
		return false;
	}

	ULARGE_INTEGER freeBytesAvailable = {}, totalNumberOfBytes = {}, totalNumberOfFreeBytes = {};
	if (GetDiskFreeSpaceExW(
							root_path_name,
							&freeBytesAvailable,
							&totalNumberOfBytes,
							&totalNumberOfFreeBytes) != 0) {

		// OK
		*free_disk_space = totalNumberOfFreeBytes.QuadPart;
	}
	else {
		// error
		// I can not use it
		return false;
	}

	return true;
}

/**
 * ディスクの空き容量を確認します
 *
 * @param[in] target_folder 取得先
 * @param[out] requested_size 最低空き容量(GB)
 * @retval true 空き容量は確保できる
 * @retval false 空き容量不足
 */
bool IscFileWriteControlImpl::CheckDiskFreeSpace(const wchar_t* target_folder, const unsigned __int64 requested_size)
{
	// free space check
	wchar_t logMag[128] = {};

	unsigned __int64 free_disk_space = 0;
	bool ret = GetFreeDiskSpace(target_folder, &free_disk_space);
	if (!ret) {
		swprintf_s(logMag, L"Failed to get free disk space %s\n", target_folder);
		isc_log_->LogError(L"IscFileWriteControlImpl", logMag);

		return false;
	}

	unsigned __int64 freeSpace = free_disk_space / (unsigned __int64)1024 / (unsigned __int64)1024;

	swprintf_s(logMag, L"free space %s=%I64d MB\n", target_folder, freeSpace);
	isc_log_->LogInfo(L"IscFileWriteControlImpl", logMag);

	unsigned __int64 freeSpaceGB = free_disk_space / (unsigned __int64)1024 / (unsigned __int64)1024 / (unsigned __int64)1024;
	if (freeSpaceGB < requested_size) {
		// ERROR

		//wchar_t msg[256] = {};
		//swprintf_s(msg, L"[ERROR]There is not enough free space in the save destination.(%I64d MB)"), freeSpace);
		//MessageBox(NULL, msg, L"CheckDiskFreeSpace()"), MB_ICONERROR);

		return false;
	}

	return true;
}
