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
 * @file isc_file_read_control_imple.cpp
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

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"
#include "isc_image_info_ring_buffer.h"
#include "vm_sdk_wrapper.h"
#include "xc_sdk_wrapper.h"
#include "k4a_sdk_wrapper.h"
#include "isc_raw_data_decoder.h"

#include "isc_file_read_control_impl.h"


/**
 * constructor
 *
 */
IscFileReadControlImpl::IscFileReadControlImpl():
	isc_camera_control_config_(), isc_grab_start_mode_(), file_read_information_(), raw_read_data_(), raw_data_decoder_(nullptr)
{

}

/**
 * destructor
 *
 */
IscFileReadControlImpl::~IscFileReadControlImpl()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] isc_camera_control_configuration 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::Initialize(const IscCameraControlConfiguration* isc_camera_control_configuration)
{
	swprintf_s(isc_camera_control_config_.configuration_file_path, L"%s", isc_camera_control_configuration->configuration_file_path);
	swprintf_s(isc_camera_control_config_.log_file_path, L"%s", isc_camera_control_configuration->log_file_path);
	isc_camera_control_config_.log_level = isc_camera_control_configuration->log_level;

	isc_camera_control_config_.enabled_camera = isc_camera_control_configuration->enabled_camera;
	isc_camera_control_config_.isc_camera_model = isc_camera_control_configuration->isc_camera_model;

	swprintf_s(isc_camera_control_config_.save_image_path, L"%s", isc_camera_control_configuration->save_image_path);
	swprintf_s(isc_camera_control_config_.load_image_path, L"%s", isc_camera_control_configuration->load_image_path);

	memset(&file_read_information_, 0, sizeof(file_read_information_));
	memset(&raw_read_data_, 0, sizeof(raw_read_data_));

	raw_data_decoder_ = new RawDataDecoder;
	raw_data_decoder_->Initialize();

	return DPC_E_OK;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::Terminate()
{

	if (raw_data_decoder_ != nullptr) {
		raw_data_decoder_->Terminate();
		delete raw_data_decoder_;
		raw_data_decoder_ = nullptr;
	}

	return DPC_E_OK;
}

/**
 * ファイルのサイズを取得します
 *
 * @param[in] file_name ファイル名
 * @param[out] file_size ファイルサイズ
 * @retval 0 成功
 * @retval other 失敗
 */
bool IscFileReadControlImpl::GetDatFileSize(TCHAR* file_name, unsigned __int64* file_size)
{
	HANDLE handle_file = NULL;

	if ((handle_file = CreateFile(	file_name,
									GENERIC_READ,
									FILE_SHARE_READ,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL)) == INVALID_HANDLE_VALUE) {

		DWORD gs_error = GetLastError();
		wchar_t msg[1024] = {};
		swprintf_s(msg, L"[ERROR]Failed to open read file(%d) %s", gs_error, file_read_information_.read_file_name);
		MessageBox(NULL, msg, L"IscFileWriteControlImpl::WriteDataProc()", MB_ICONERROR);

		return false;
	}

	unsigned long file_size_hight = 0;
	unsigned long file_size_low = GetFileSize(handle_file, &file_size_hight);

	size_t size = 0;

	if (file_size_low != 0xFFFFFFFF) {
		size = (DWORD_PTR)file_size_hight << 32 | (DWORD_PTR)file_size_low;
		// printf("FileSize = %llu bytes\n", size);
	}
	else {
		CloseHandle(handle_file);
		return false;
	}

	CloseHandle(handle_file);

	*file_size = (unsigned __int64)size;

	return true;
}

/**
 * ファイルからの読み込みを開始します
 *
 * @param[in] isc_grab_start_mode 読み込みのモードを指定する
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::Start(const IscGrabStartMode* isc_grab_start_mode)
{

	file_read_information_.is_file_ready = false;

	if (isc_grab_start_mode->isc_play_mode == IscPlayMode::kPlayOff) {
		// error
		return CAMCONTROL_E_INVALID_REQUEST;
	}

	if (isc_grab_start_mode->isc_play_mode_parameter.play_file_name[0] == 0) {
		// error
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	if (::PathFileExists(isc_grab_start_mode->isc_play_mode_parameter.play_file_name) &&
		!::PathIsDirectory(isc_grab_start_mode->isc_play_mode_parameter.play_file_name)) {
		// 指定されたパスにファイルが存在、かつディレクトリでない -> OK
	}
	else {
		// NG
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	isc_grab_start_mode_.isc_grab_mode = isc_grab_start_mode->isc_grab_mode;
	isc_grab_start_mode_.isc_grab_color_mode = isc_grab_start_mode->isc_grab_color_mode;

	isc_grab_start_mode_.isc_get_mode.wait_time = isc_grab_start_mode->isc_get_mode.wait_time;
	isc_grab_start_mode_.isc_get_raw_mode = isc_grab_start_mode->isc_get_raw_mode;
	isc_grab_start_mode_.isc_get_color_mode = isc_grab_start_mode->isc_get_color_mode;

	isc_grab_start_mode_.isc_record_mode = isc_grab_start_mode->isc_record_mode;
	isc_grab_start_mode_.isc_play_mode = isc_grab_start_mode->isc_play_mode;
	isc_grab_start_mode_.isc_play_mode_parameter.interval = isc_grab_start_mode->isc_play_mode_parameter.interval;
	swprintf_s(isc_grab_start_mode_.isc_play_mode_parameter.play_file_name, L"%s", isc_grab_start_mode->isc_play_mode_parameter.play_file_name);

	swprintf_s(file_read_information_.read_file_name, L"%s", isc_grab_start_mode->isc_play_mode_parameter.play_file_name);

	// open file
	file_read_information_.total_read_size = 0;

	if (!GetDatFileSize(file_read_information_.read_file_name, &file_read_information_.file_size)) {
		return CAMCONTROL_E_OPEN_READ_FILE_FAILED;
	}

	if ((file_read_information_.handle_file = CreateFile(	file_read_information_.read_file_name,
															GENERIC_READ,
															FILE_SHARE_READ,
															NULL,
															OPEN_EXISTING,
															FILE_ATTRIBUTE_NORMAL,
															NULL)) == INVALID_HANDLE_VALUE) {


		DWORD gs_error = GetLastError();
		wchar_t msg[1024] = {};
		swprintf_s(msg, L"[ERROR]Failed to open read file(%d) %s", gs_error, file_read_information_.read_file_name);
		MessageBox(NULL, msg, L"IscFileWriteControlImpl::WriteDataProc()", MB_ICONERROR);

		return CAMCONTROL_E_OPEN_READ_FILE_FAILED;
	}


	// read header
	DWORD bytes_to_read = sizeof(file_read_information_.raw_file_header);
	DWORD readed_size = 0;
	
	if (FALSE == ReadFile(file_read_information_.handle_file, &file_read_information_.raw_file_header, bytes_to_read, &readed_size, NULL)) {
		CloseHandle(file_read_information_.handle_file);
		file_read_information_.handle_file = NULL;
		file_read_information_.is_file_ready = false;

		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	file_read_information_.total_read_size += readed_size;

	raw_read_data_.width = file_read_information_.raw_file_header.max_width;
	raw_read_data_.height = file_read_information_.raw_file_header.max_height;

	size_t buff_size = raw_read_data_.width * raw_read_data_.height * 2;
	raw_read_data_.buffer = new unsigned char[buff_size];

	file_read_information_.is_file_ready = true;

	// it check camera model
	IscCameraModel camera_model_in_file = IscCameraModel::kVM;

	// model  0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown
	switch (file_read_information_.raw_file_header.camera_model) {
	case 0:camera_model_in_file = IscCameraModel::kVM;
		break;
	case 1:camera_model_in_file = IscCameraModel::kXC;
		break;
	case 2:camera_model_in_file = IscCameraModel::k4K;
		break;
	case 3:camera_model_in_file = IscCameraModel::k4KA;
		break;
	case 4:camera_model_in_file = IscCameraModel::k4KJ;
		break;
	case 99:camera_model_in_file = IscCameraModel::kUnknown;
		break;
	default:camera_model_in_file = IscCameraModel::kUnknown;
		break;
	}

	if (isc_camera_control_config_.isc_camera_model != camera_model_in_file) {
		if (file_read_information_.handle_file != NULL) {
			CloseHandle(file_read_information_.handle_file);
			file_read_information_.handle_file = NULL;
		}

		delete[] raw_read_data_.buffer;
		raw_read_data_.buffer = nullptr;
		raw_read_data_.width = 0;
		raw_read_data_.height = 0;

		file_read_information_.is_file_ready = false;

		return CAMCONTROL_E_READ_CAMERA_MODEL;
	}

	return DPC_E_OK;
}

/**
 * ファイルからの読み込みを終了します
 *
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::Stop()
{

	if (file_read_information_.handle_file != NULL) {
		CloseHandle(file_read_information_.handle_file);
		file_read_information_.handle_file = NULL;
	}

	delete[] raw_read_data_.buffer;
	raw_read_data_.buffer = nullptr;
	raw_read_data_.width = 0;
	raw_read_data_.height = 0;

	file_read_information_.is_file_ready = false;

	return DPC_E_OK;
}

/**
 * 現在の動作モードを取得します
 *
 * @param[out] isc_grab_start_mode　動作モード
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
{
	isc_grab_start_mode->isc_grab_mode = IscGrabMode::kParallax;
	switch (file_read_information_.raw_file_header.grab_mode) {
	case 0:
		// ?
		isc_grab_start_mode->isc_grab_mode = IscGrabMode::kParallax;
		break;

	case 1:
		isc_grab_start_mode->isc_grab_mode = IscGrabMode::kParallax;
		break;

	case 2:
		isc_grab_start_mode->isc_grab_mode = IscGrabMode::kCorrect;
		break;

	case 3:
		isc_grab_start_mode->isc_grab_mode = IscGrabMode::kBeforeCorrect;
		break;

	case 4:
		isc_grab_start_mode->isc_grab_mode = IscGrabMode::kBayerS0;
		break;

	case 5:
		isc_grab_start_mode->isc_grab_mode = IscGrabMode::kBayerS1;
		break;

	default:
		isc_grab_start_mode->isc_grab_mode = IscGrabMode::kParallax;
		break;
	}

	isc_grab_start_mode->isc_grab_color_mode = IscGrabColorMode::kColorOFF;
	switch (file_read_information_.raw_file_header.color_mode) {
	case 0:
		isc_grab_start_mode->isc_grab_color_mode = IscGrabColorMode::kColorOFF;
		break;
	case 1:
		isc_grab_start_mode->isc_grab_color_mode = IscGrabColorMode::kColorON;
		break;
	default:
		isc_grab_start_mode->isc_grab_color_mode = IscGrabColorMode::kColorOFF;
		break;
	}

	isc_grab_start_mode->isc_get_mode.wait_time = isc_grab_start_mode_.isc_get_mode.wait_time;
	isc_grab_start_mode->isc_get_raw_mode = isc_grab_start_mode_.isc_get_raw_mode;
	isc_grab_start_mode->isc_get_color_mode = isc_grab_start_mode_.isc_get_color_mode;

	isc_grab_start_mode->isc_record_mode = IscRecordMode::kRecordOff;
	isc_grab_start_mode->isc_play_mode = IscPlayMode::kPlayOff;
	isc_grab_start_mode->isc_play_mode_parameter.interval = 0;
	swprintf_s(isc_grab_start_mode->isc_play_mode_parameter.play_file_name, L"%s", file_read_information_.read_file_name);

	return DPC_E_OK;
}

/**
 * 読み込みデータを取得します
 *
 * @param[out] isc_image_info　データ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::GetData(IscImageInfo* isc_image_info)
{

	if (!file_read_information_.is_file_ready) {
		return CAMCONTROL_E_NO_IMAGE;
	}

	if (file_read_information_.handle_file == NULL) {
		return CAMCONTROL_E_NO_IMAGE;
	}

	// Colorモードを確認する
	// Color ModeがOnの場合は、mono/colorをペアで読み込み

	IscGrabColorMode isc_grab_color_mode = (file_read_information_.raw_file_header.color_mode == 0) ? IscGrabColorMode::kColorOFF : IscGrabColorMode::kColorON;
	if (isc_grab_color_mode == IscGrabColorMode::kColorOFF) {
		unsigned __int64 next_to_read = file_read_information_.total_read_size + sizeof(raw_read_data_.isc_raw_data_header) + raw_read_data_.isc_raw_data_header.data_size;
		if (next_to_read >= file_read_information_.file_size) {
			return CAMCONTROL_E_NO_IMAGE;
		}
	}
	else if (isc_grab_color_mode == IscGrabColorMode::kColorON) {
		// it read 2 raw datas
		unsigned __int64 next_to_read = (file_read_information_.total_read_size + sizeof(raw_read_data_.isc_raw_data_header) + raw_read_data_.isc_raw_data_header.data_size) * 2;
		if (next_to_read >= file_read_information_.file_size) {
			return CAMCONTROL_E_NO_IMAGE;
		}

	}
	else {
		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	// setup
	IscGrabMode isc_grab_mode = IscGrabMode::kParallax;
	switch (file_read_information_.raw_file_header.grab_mode) {
	case 1:
		isc_grab_mode = IscGrabMode::kParallax;
		break;

	case 2:
		isc_grab_mode = IscGrabMode::kCorrect;
		break;

	case 3:
		isc_grab_mode = IscGrabMode::kBeforeCorrect;
		break;

	case 4:
		isc_grab_mode = IscGrabMode::kBayerS0;
		break;
	}
	isc_image_info->grab = isc_grab_mode;
	isc_image_info->color_grab_mode = isc_grab_color_mode;

	IscShutterMode isc_shutter_mode = IscShutterMode::kManualShutter;
	switch (file_read_information_.raw_file_header.shutter_mode) {
	case 0:
		isc_shutter_mode = IscShutterMode::kManualShutter;
		break;
	case 1:
		isc_shutter_mode = IscShutterMode::kSingleShutter;
		break;
	case 2:
		isc_shutter_mode = IscShutterMode::kDoubleShutter;
		break;
	}
	isc_image_info->shutter_mode = isc_shutter_mode;
	isc_image_info->camera_specific_parameter.d_inf = file_read_information_.raw_file_header.d_inf;
	isc_image_info->camera_specific_parameter.bf = file_read_information_.raw_file_header.bf;
	isc_image_info->camera_specific_parameter.base_length = file_read_information_.raw_file_header.base_length;
	isc_image_info->camera_specific_parameter.dz = file_read_information_.raw_file_header.dz;

	if (isc_grab_color_mode == IscGrabColorMode::kColorOFF) {
		const bool specify_mode = false;
		const bool init_request = true;
		const IscGrabColorMode requested_color_mode = IscGrabColorMode::kColorOFF;
		IscGrabColorMode obtained_color_mode = IscGrabColorMode::kColorOFF;

		int ret = ReadOneRawData(isc_image_info, specify_mode, init_request, requested_color_mode, &obtained_color_mode);
		
		if (ret != DPC_E_OK) {
			return ret;
		}
	}
	else if (isc_grab_color_mode == IscGrabColorMode::kColorON) {
		// 1st data
		bool specify_mode = false;
		bool init_request = true;
		IscGrabColorMode requested_color_mode = IscGrabColorMode::kColorOFF;
		IscGrabColorMode obtained_color_mode = IscGrabColorMode::kColorOFF;

		int ret = ReadOneRawData(isc_image_info, specify_mode, init_request, requested_color_mode, &obtained_color_mode);

		if (ret != DPC_E_OK) {
			return ret;
		}

		// 2nd data
		if (obtained_color_mode == IscGrabColorMode::kColorOFF) {
			requested_color_mode = IscGrabColorMode::kColorON;
		}
		else {
			requested_color_mode = IscGrabColorMode::kColorOFF;
		}
		specify_mode = true;
		init_request = false;
		obtained_color_mode = IscGrabColorMode::kColorOFF;

		ret = ReadOneRawData(isc_image_info, specify_mode, init_request, requested_color_mode, &obtained_color_mode);

		if (ret != DPC_E_OK) {
			return ret;
		}
	}
	else {
		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	// adjust the time
	Sleep(isc_grab_start_mode_.isc_play_mode_parameter.interval);

	return DPC_E_OK;
}

/**
 * RAW Dataを1個読み込みます
 *
 * @param[out] isc_image_info　データ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::ReadOneRawData(IscImageInfo* isc_image_info, const bool specify_mode, const bool init, const IscGrabColorMode requested_color_mode, IscGrabColorMode* obtained_color_mode)
{
	// read from file
	DWORD bytes_to_read = sizeof(raw_read_data_.isc_raw_data_header);
	DWORD readed_size = 0;

	// haeder
	if (FALSE == ReadFile(file_read_information_.handle_file, &raw_read_data_.isc_raw_data_header, bytes_to_read, &readed_size, NULL)) {
		CloseHandle(file_read_information_.handle_file);
		file_read_information_.handle_file = NULL;
		file_read_information_.is_file_ready = false;

		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	file_read_information_.total_read_size += readed_size;

	// data
	bytes_to_read = raw_read_data_.isc_raw_data_header.data_size;
	if (FALSE == ReadFile(file_read_information_.handle_file, raw_read_data_.buffer, bytes_to_read, &readed_size, NULL)) {

		DWORD last_err = GetLastError();

		CloseHandle(file_read_information_.handle_file);
		file_read_information_.handle_file = NULL;
		file_read_information_.is_file_ready = false;

		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	file_read_information_.total_read_size += readed_size;

	const int frame_data_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

	if (init) {
		isc_image_info->frame_data[frame_data_index].frameNo = raw_read_data_.isc_raw_data_header.frame_index;
		isc_image_info->frame_data[frame_data_index].gain = raw_read_data_.isc_raw_data_header.gain;
		isc_image_info->frame_data[frame_data_index].exposure = raw_read_data_.isc_raw_data_header.exposure;

		isc_image_info->frame_data[frame_data_index].camera_status.error_code = raw_read_data_.isc_raw_data_header.error_code;
		isc_image_info->frame_data[frame_data_index].camera_status.data_receive_tact_time = 0;

		isc_image_info->frame_data[frame_data_index].p1.width = 0;
		isc_image_info->frame_data[frame_data_index].p1.height = 0;
		isc_image_info->frame_data[frame_data_index].p1.channel_count = 0;

		isc_image_info->frame_data[frame_data_index].p2.width = 0;
		isc_image_info->frame_data[frame_data_index].p2.height = 0;
		isc_image_info->frame_data[frame_data_index].p2.channel_count = 0;

		isc_image_info->frame_data[frame_data_index].color.width = 0;
		isc_image_info->frame_data[frame_data_index].color.height = 0;
		isc_image_info->frame_data[frame_data_index].color.channel_count = 0;

		isc_image_info->frame_data[frame_data_index].depth.width = 0;
		isc_image_info->frame_data[frame_data_index].depth.height = 0;

		isc_image_info->frame_data[frame_data_index].raw.width = 0;
		isc_image_info->frame_data[frame_data_index].raw.height = 0;
		isc_image_info->frame_data[frame_data_index].raw.channel_count = 0;
	}

	// decode
	IscGrabColorMode isc_grab_color_mode = (file_read_information_.raw_file_header.color_mode == 0) ? IscGrabColorMode::kColorOFF : IscGrabColorMode::kColorON;

	*obtained_color_mode = isc_grab_color_mode;
	if (specify_mode) {
		if (isc_grab_color_mode == requested_color_mode) {
			// OK
		}
		else {
			// NG
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
	}

	// camera modesl 0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown
	if (file_read_information_.raw_file_header.camera_model == 0) {
		IscCameraModel isc_camera_model = IscCameraModel::kVM;
		IscGetModeColor isc_get_color_mode = IscGetModeColor::kAwb;

		int width = file_read_information_.raw_file_header.max_width;
		int height = file_read_information_.raw_file_header.max_height;

		isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
		isc_image_info->frame_data[frame_data_index].raw.height = height;
		isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
		size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
		memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

		IscGrabColorMode raw_color_mode = IscGrabColorMode::kColorOFF;

		int ret = raw_data_decoder_->Decode(isc_camera_model, isc_image_info->grab, raw_color_mode, isc_get_color_mode, width, height, isc_image_info);
		if (ret != DPC_E_OK) {
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
		*obtained_color_mode = IscGrabColorMode::kColorOFF;
	}
	else if (file_read_information_.raw_file_header.camera_model == 1) {
		IscCameraModel isc_camera_model = IscCameraModel::kXC;
		IscGetModeColor isc_get_color_mode = isc_grab_start_mode_.isc_get_color_mode;// IscGetModeColor::kAwbNoCorrect;

		int width = file_read_information_.raw_file_header.max_width;
		int height = file_read_information_.raw_file_header.max_height;

		IscGrabColorMode raw_color_mode = IscGrabColorMode::kColorOFF;

		if (isc_grab_color_mode == IscGrabColorMode::kColorOFF) {
			// mono
			isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw.height = height;
			isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

			*obtained_color_mode = IscGrabColorMode::kColorOFF;
		}
		else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 1)) {
			// mono
			isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw.height = height;
			isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

			*obtained_color_mode = IscGrabColorMode::kColorOFF;
		}
		else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 2)) {
			// color
			isc_image_info->frame_data[frame_data_index].raw_color.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw_color.height = height;
			isc_image_info->frame_data[frame_data_index].raw_color.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw_color.width * isc_image_info->frame_data[frame_data_index].raw_color.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw_color.image, raw_read_data_.buffer, cp_size);

			// this raw data is color;
			raw_color_mode = IscGrabColorMode::kColorON;
			*obtained_color_mode = IscGrabColorMode::kColorON;
		}
		else {
			// error
			return CAMCONTROL_E_READ_FILE_FAILED;
		}

		int ret = raw_data_decoder_->Decode(isc_camera_model, isc_image_info->grab, raw_color_mode, isc_get_color_mode, width, height, isc_image_info);
		if (ret != DPC_E_OK) {
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
	}
	else if (file_read_information_.raw_file_header.camera_model == 2) {
		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	else if (file_read_information_.raw_file_header.camera_model == 3) {
		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	else if (file_read_information_.raw_file_header.camera_model == 4) {
		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	else {
		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	return DPC_E_OK;
}

/**
 * 読み込みファイルのヘッダー部を取得します
 *
 * @param[in] play_file_name ファイル名
 * @param[out] raw_file_header　ヘッダーデータ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header)
{
	if (::PathFileExists(play_file_name) && !::PathIsDirectory(play_file_name)) {
		// 指定されたパスにファイルが存在、かつディレクトリでない -> OK
	}
	else {
		// NG
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	// open file

	unsigned __int64 file_size = 0;
	if (!GetDatFileSize(play_file_name, &file_size)) {
		return CAMCONTROL_E_OPEN_READ_FILE_FAILED;
	}

	HANDLE handle_file;
	if ((handle_file = CreateFile(	play_file_name,
									GENERIC_READ,
									FILE_SHARE_READ,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL)) == INVALID_HANDLE_VALUE) {

		DWORD gs_error = GetLastError();
		wchar_t msg[1024] = {};
		swprintf_s(msg, L"[ERROR]Failed to open read file(%d) %s", gs_error, play_file_name);
		MessageBox(NULL, msg, L"IscFileWriteControlImpl::WriteDataProc()", MB_ICONERROR);

		return CAMCONTROL_E_OPEN_READ_FILE_FAILED;
	}


	// read header
	DWORD bytes_to_read = sizeof(IscRawFileHeader);
	DWORD readed_size = 0;

	if (FALSE == ReadFile(handle_file, raw_file_header, bytes_to_read, &readed_size, NULL)) {
		CloseHandle(handle_file);
		handle_file = NULL;

		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	CloseHandle(handle_file);
	handle_file = NULL;

	return DPC_E_OK;
}