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
	file_read_information_.request_fo_designated_number = false;
	file_read_information_.designated_number = 0;

	file_read_information_.total_read_size = 0;
	file_read_information_.current_frame_number = 0;

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
	memset(raw_read_data_.buffer, 0, buff_size);

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

	{
		// deug informatin
		char msg[256] = {};

		OutputDebugStringA("[INFO]IscFileReadControlImpl::Start() \n");
		
		OutputDebugStringA("    ");
		OutputDebugStringW(file_read_information_.read_file_name);
		OutputDebugStringA("\n");

		__int64 one_fr_size = sizeof(raw_read_data_.isc_raw_data_header) + buff_size;
		__int64 total_frame_count = (file_read_information_.file_size - sizeof(file_read_information_.raw_file_header)) / one_fr_size;

		sprintf_s(msg, "    Total Frame Count=%lld\n", total_frame_count);
		OutputDebugStringA(msg);

		OutputDebugStringA("[INFO]IscFileReadControlImpl::Start() end of message -- \n");
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
		case 3:
			isc_shutter_mode = IscShutterMode::kDoubleShutter2;
			break;
		}

		if (isc_shutter_mode == IscShutterMode::kDoubleShutter) {
			int width = file_read_information_.raw_file_header.max_width;
			int height = file_read_information_.raw_file_header.max_height;

			size_t frame_size = width * height * 2;
			__int64 one_data_size = sizeof(raw_read_data_.isc_raw_data_header) + frame_size;

			unsigned __int64 next_to_read = file_read_information_.total_read_size + one_data_size;
			// Double Shutterの場合は、2Frame必要
			if (next_to_read >= file_read_information_.file_size) {
				return CAMCONTROL_E_NO_IMAGE;
			}
		}
		else {
			int width = file_read_information_.raw_file_header.max_width;
			int height = file_read_information_.raw_file_header.max_height;

			size_t frame_size = width * height * 2;
			__int64 one_data_size = sizeof(raw_read_data_.isc_raw_data_header) + frame_size;

			unsigned __int64 next_to_read = file_read_information_.total_read_size + one_data_size;
			if (next_to_read > file_read_information_.file_size) {
				return CAMCONTROL_E_NO_IMAGE;
			}
		}
	}
	else if (isc_grab_color_mode == IscGrabColorMode::kColorON) {
		// it read 2 raw datas
		int width = file_read_information_.raw_file_header.max_width;
		int height = file_read_information_.raw_file_header.max_height;

		size_t frame_size = width * height * 2;
		__int64 one_data_size = sizeof(raw_read_data_.isc_raw_data_header) + frame_size;

		unsigned __int64 next_to_read = file_read_information_.total_read_size + one_data_size * 2;
		if (next_to_read > file_read_information_.file_size) {
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
	case 3:
		isc_shutter_mode = IscShutterMode::kDoubleShutter2;
		break;
	}
	isc_image_info->shutter_mode = isc_shutter_mode;
	isc_image_info->camera_specific_parameter.d_inf = file_read_information_.raw_file_header.d_inf;
	isc_image_info->camera_specific_parameter.bf = file_read_information_.raw_file_header.bf;
	isc_image_info->camera_specific_parameter.base_length = file_read_information_.raw_file_header.base_length;
	isc_image_info->camera_specific_parameter.dz = file_read_information_.raw_file_header.dz;

	// 指定Frameへの移動要求の確認
	if (file_read_information_.request_fo_designated_number) {
		file_read_information_.request_fo_designated_number = false;

		__int64 request_frame = file_read_information_.designated_number;

		if ((request_frame >= 0) && (request_frame < file_read_information_.play_file_information.total_frame_count)) {
			// 移動
			int move_ret = MoveToSpecifyFrameNumber(request_frame);

			if (move_ret != DPC_E_OK) {
				return CAMCONTROL_E_READ_FILE_FAILED;
			}
		}
	}

	if ((isc_shutter_mode == IscShutterMode::kManualShutter) ||
		(isc_shutter_mode == IscShutterMode::kSingleShutter) ||
		(isc_shutter_mode == IscShutterMode::kDoubleShutter2)) {

		if (isc_grab_color_mode == IscGrabColorMode::kColorOFF) {
			int ret = ReadOneRawData(isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else if (isc_grab_color_mode == IscGrabColorMode::kColorON) {
			int ret = ReadColorRawData(isc_image_info);
			if (ret != DPC_E_OK) {
				return ret;
			}
		}
		else {
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
	}
	else if (isc_shutter_mode == IscShutterMode::kDoubleShutter) {
		// isc_shutter_mode = IscShutterMode::kDoubleShutter
		// 合成を行います

		if (isc_grab_color_mode == IscGrabColorMode::kColorOFF) {

			// Frame番号が連続していないデータは、不採用とします

			DPL_RESULT read_ret = DPC_E_OK;
			while(true) {
				read_ret = ReadDoubleShutterRawData(isc_image_info);
				
				if (read_ret == DPC_E_OK) {
					break;
				}
				else if (read_ret == CAMCONTROL_E_READ_FILE_FAILED_RETRY) {
					// continue
					int width = file_read_information_.raw_file_header.max_width;
					int height = file_read_information_.raw_file_header.max_height;

					size_t frame_size = width * height * 2;
					__int64 one_data_size = sizeof(raw_read_data_.isc_raw_data_header) + frame_size;

					unsigned __int64 next_to_read = file_read_information_.total_read_size + one_data_size;
					// Double Shutterの場合は、2Frame必要
					if (next_to_read >= file_read_information_.file_size) {
						return CAMCONTROL_E_NO_IMAGE;
					}
				}
				else {
					break;
				}
			}
		}
		else {
			return CAMCONTROL_E_READ_FILE_FAILED;
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
int IscFileReadControlImpl::ReadOneRawData(IscImageInfo* isc_image_info)
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

	// 初期化
	{
		isc_image_info->frame_data[frame_data_index].camera_status.error_code = raw_read_data_.isc_raw_data_header.error_code;
		isc_image_info->frame_data[frame_data_index].camera_status.data_receive_tact_time = 0;

		if (raw_read_data_.isc_raw_data_header.version >= 300) {

			ULARGE_INTEGER ul_int = {};
			ul_int.LowPart = raw_read_data_.isc_raw_data_header.frame_time_low;
			ul_int.HighPart = raw_read_data_.isc_raw_data_header.frame_time_high;

			isc_image_info->frame_data[frame_data_index].frame_time = ul_int.QuadPart;

			// debug
			if (0) {
				struct timespec tm = {};
				tm.tv_sec = ul_int.QuadPart / 1000LL;
				tm.tv_nsec = static_cast<long>((ul_int.QuadPart - (tm.tv_sec * 1000LL)) * 1000000);

				struct tm ltm;
				localtime_s(&ltm, &tm.tv_sec);
				long millisecond = tm.tv_nsec / 1000000LL;

				char time_str[256] = {};
				char dayofweek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

				sprintf_s(time_str, "%d: %d/%d/%d %s %02d:%02d:%02d.%03d\n",
					raw_read_data_.isc_raw_data_header.frame_index,
					ltm.tm_year + 1900,
					ltm.tm_mon + 1,
					ltm.tm_mday,
					dayofweek[ltm.tm_wday],
					ltm.tm_hour,
					ltm.tm_min,
					ltm.tm_sec,
					millisecond);

				OutputDebugStringA(time_str);
			}
		}
		else {
			isc_image_info->frame_data[frame_data_index].frame_time = 0;
		}

		isc_image_info->frame_data[frame_data_index].data_index = file_read_information_.current_frame_number;
		isc_image_info->frame_data[frame_data_index].frameNo = raw_read_data_.isc_raw_data_header.frame_index;
		isc_image_info->frame_data[frame_data_index].gain = raw_read_data_.isc_raw_data_header.gain;
		isc_image_info->frame_data[frame_data_index].exposure = raw_read_data_.isc_raw_data_header.exposure;

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
	IscGrabColorMode obtained_color_mode = isc_grab_color_mode;

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

		int ret = raw_data_decoder_->Decode(isc_camera_model, isc_image_info->grab, raw_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
		if (ret != DPC_E_OK) {
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
		obtained_color_mode = IscGrabColorMode::kColorOFF;
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

			obtained_color_mode = IscGrabColorMode::kColorOFF;
		}
		else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 1)) {
			// mono
			isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw.height = height;
			isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

			obtained_color_mode = IscGrabColorMode::kColorOFF;
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
			obtained_color_mode = IscGrabColorMode::kColorON;
		}
		else {
			// error
			return CAMCONTROL_E_READ_FILE_FAILED;
		}

		int ret = raw_data_decoder_->Decode(isc_camera_model, isc_image_info->grab, raw_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
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

	file_read_information_.current_frame_number++;

	return DPC_E_OK;
}

/**
 * Color を含んだRAW Dataを2個読み込みます
 *
 * @param[out] isc_image_info　データ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::ReadColorRawData(IscImageInfo* isc_image_info)
{
	// read from file
	DWORD bytes_to_read = sizeof(raw_read_data_.isc_raw_data_header);
	DWORD readed_size = 0;

	// (1) first data
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
	readed_size = 0;
	if (FALSE == ReadFile(file_read_information_.handle_file, raw_read_data_.buffer, bytes_to_read, &readed_size, NULL)) {

		DWORD last_err = GetLastError();

		CloseHandle(file_read_information_.handle_file);
		file_read_information_.handle_file = NULL;
		file_read_information_.is_file_ready = false;

		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	file_read_information_.total_read_size += readed_size;

	const int frame_data_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

	// 初期化
	{
		isc_image_info->frame_data[frame_data_index].camera_status.error_code = raw_read_data_.isc_raw_data_header.error_code;
		isc_image_info->frame_data[frame_data_index].camera_status.data_receive_tact_time = 0;

		if (raw_read_data_.isc_raw_data_header.version >= 300) {

			ULARGE_INTEGER ul_int = {};
			ul_int.LowPart = raw_read_data_.isc_raw_data_header.frame_time_low;
			ul_int.HighPart = raw_read_data_.isc_raw_data_header.frame_time_high;

			isc_image_info->frame_data[frame_data_index].frame_time = ul_int.QuadPart;

			// debug
			if (0) {
				struct timespec tm = {};
				tm.tv_sec = ul_int.QuadPart / 1000LL;
				tm.tv_nsec = static_cast<long>((ul_int.QuadPart - (tm.tv_sec * 1000LL)) * 1000000);

				struct tm ltm;
				localtime_s(&ltm, &tm.tv_sec);
				long millisecond = tm.tv_nsec / 1000000LL;

				char time_str[256] = {};
				char dayofweek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

				sprintf_s(time_str, "%d: %d/%d/%d %s %02d:%02d:%02d.%03d\n",
					raw_read_data_.isc_raw_data_header.frame_index,
					ltm.tm_year + 1900,
					ltm.tm_mon + 1,
					ltm.tm_mday,
					dayofweek[ltm.tm_wday],
					ltm.tm_hour,
					ltm.tm_min,
					ltm.tm_sec,
					millisecond);

				OutputDebugStringA(time_str);
			}
		}
		else {
			isc_image_info->frame_data[frame_data_index].frame_time = 0;
		}

		isc_image_info->frame_data[frame_data_index].data_index = file_read_information_.current_frame_number;
		isc_image_info->frame_data[frame_data_index].frameNo = raw_read_data_.isc_raw_data_header.frame_index;
		isc_image_info->frame_data[frame_data_index].gain = raw_read_data_.isc_raw_data_header.gain;
		isc_image_info->frame_data[frame_data_index].exposure = raw_read_data_.isc_raw_data_header.exposure;

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
	IscGrabColorMode obtained_color_mode = isc_grab_color_mode;

	// camera modesl 0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown
	if (file_read_information_.raw_file_header.camera_model == 0) {
		// Colorはサポートされていない
		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	else if (file_read_information_.raw_file_header.camera_model == 1) {
		IscCameraModel isc_camera_model = IscCameraModel::kXC;
		IscGetModeColor isc_get_color_mode = isc_grab_start_mode_.isc_get_color_mode;// IscGetModeColor::kAwbNoCorrect;

		int width = file_read_information_.raw_file_header.max_width;
		int height = file_read_information_.raw_file_header.max_height;

		IscGrabColorMode raw_color_mode = IscGrabColorMode::kColorOFF;

		if (isc_grab_color_mode == IscGrabColorMode::kColorOFF) {
			// mono
			// Color要求に対してColorのデータではない

			// error
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
		else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 1)) {
			// mono
			isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw.height = height;
			isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

			obtained_color_mode = IscGrabColorMode::kColorOFF;
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
			obtained_color_mode = IscGrabColorMode::kColorON;
		}
		else {
			// error
			return CAMCONTROL_E_READ_FILE_FAILED;
		}

		int ret = raw_data_decoder_->Decode(isc_camera_model, isc_image_info->grab, raw_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
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

	file_read_information_.current_frame_number++;

	// (2) second data
	IscGrabColorMode requested_color_mode = IscGrabColorMode::kColorOFF;

	if (obtained_color_mode == IscGrabColorMode::kColorOFF) {
		// 最初のデータがmonochroなら次はColorが必要
		requested_color_mode = IscGrabColorMode::kColorON;
	}
	else {
		// 最初のデータがColorなら次はmonochroが必要
		requested_color_mode = IscGrabColorMode::kColorOFF;
	}

	bytes_to_read = sizeof(raw_read_data_.isc_raw_data_header);
	readed_size = 0;
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
	readed_size = 0;
	if (FALSE == ReadFile(file_read_information_.handle_file, raw_read_data_.buffer, bytes_to_read, &readed_size, NULL)) {

		DWORD last_err = GetLastError();

		CloseHandle(file_read_information_.handle_file);
		file_read_information_.handle_file = NULL;
		file_read_information_.is_file_ready = false;

		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	file_read_information_.total_read_size += readed_size;

	// check mode
	isc_grab_color_mode = (file_read_information_.raw_file_header.color_mode == 0) ? IscGrabColorMode::kColorOFF : IscGrabColorMode::kColorON;

	if ((isc_grab_color_mode == IscGrabColorMode::kColorOFF) && (raw_read_data_.isc_raw_data_header.type == 1)) {
		// color off
		// NG
		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 1)) {
		// mono
		if (requested_color_mode == IscGrabColorMode::kColorOFF) {
			// OK
		}
		else {
			// NG
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
	}
	else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 2)) {
		// color
		if (requested_color_mode == IscGrabColorMode::kColorON) {
			// OK
		}
		else {
			// NG
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
	}
	else {
		// NG
		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	// camera modesl 0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown
	if (file_read_information_.raw_file_header.camera_model == 0) {
		// Colorはサポートされていない
		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	else if (file_read_information_.raw_file_header.camera_model == 1) {
		IscCameraModel isc_camera_model = IscCameraModel::kXC;
		IscGetModeColor isc_get_color_mode = isc_grab_start_mode_.isc_get_color_mode;// IscGetModeColor::kAwbNoCorrect;

		int width = file_read_information_.raw_file_header.max_width;
		int height = file_read_information_.raw_file_header.max_height;

		IscGrabColorMode raw_color_mode = IscGrabColorMode::kColorOFF;

		if ((isc_grab_color_mode == IscGrabColorMode::kColorOFF) && (raw_read_data_.isc_raw_data_header.type == 1)) {
			// mono
			isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw.height = height;
			isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

			obtained_color_mode = IscGrabColorMode::kColorOFF;
		}
		else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 1)) {
			// mono
			isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw.height = height;
			isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

			obtained_color_mode = IscGrabColorMode::kColorOFF;
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
			obtained_color_mode = IscGrabColorMode::kColorON;
		}
		else {
			// error
			return CAMCONTROL_E_READ_FILE_FAILED;
		}

		int ret = raw_data_decoder_->Decode(isc_camera_model, isc_image_info->grab, raw_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
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

	file_read_information_.current_frame_number++;

	return DPC_E_OK;
}

/**
 * Double Shutter を含んだRAW Dataを2個読み込みます
 *
 * @param[out] isc_image_info　データ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::ReadDoubleShutterRawData(IscImageInfo* isc_image_info)
{
	// read from file
	DWORD bytes_to_read = sizeof(raw_read_data_.isc_raw_data_header);
	DWORD readed_size = 0;

	// (1) first data
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
	readed_size = 0;
	if (FALSE == ReadFile(file_read_information_.handle_file, raw_read_data_.buffer, bytes_to_read, &readed_size, NULL)) {

		DWORD last_err = GetLastError();

		CloseHandle(file_read_information_.handle_file);
		file_read_information_.handle_file = NULL;
		file_read_information_.is_file_ready = false;

		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	file_read_information_.total_read_size += readed_size;

	int frame_data_index = kISCIMAGEINFO_FRAMEDATA_PREVIOUS;

	// 初期化
	{
		isc_image_info->frame_data[frame_data_index].camera_status.error_code = raw_read_data_.isc_raw_data_header.error_code;
		isc_image_info->frame_data[frame_data_index].camera_status.data_receive_tact_time = 0;

		if (raw_read_data_.isc_raw_data_header.version >= 300) {

			ULARGE_INTEGER ul_int = {};
			ul_int.LowPart = raw_read_data_.isc_raw_data_header.frame_time_low;
			ul_int.HighPart = raw_read_data_.isc_raw_data_header.frame_time_high;

			isc_image_info->frame_data[frame_data_index].frame_time = ul_int.QuadPart;

			// debug
			if (0) {
				struct timespec tm = {};
				tm.tv_sec = ul_int.QuadPart / 1000LL;
				tm.tv_nsec = static_cast<long>((ul_int.QuadPart - (tm.tv_sec * 1000LL)) * 1000000);

				struct tm ltm;
				localtime_s(&ltm, &tm.tv_sec);
				long millisecond = tm.tv_nsec / 1000000LL;

				char time_str[256] = {};
				char dayofweek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

				sprintf_s(time_str, "%d: %d/%d/%d %s %02d:%02d:%02d.%03d\n",
					raw_read_data_.isc_raw_data_header.frame_index,
					ltm.tm_year + 1900,
					ltm.tm_mon + 1,
					ltm.tm_mday,
					dayofweek[ltm.tm_wday],
					ltm.tm_hour,
					ltm.tm_min,
					ltm.tm_sec,
					millisecond);

				OutputDebugStringA(time_str);
			}
		}
		else {
			isc_image_info->frame_data[frame_data_index].frame_time = 0;
		}

		isc_image_info->frame_data[frame_data_index].data_index = file_read_information_.current_frame_number;
		isc_image_info->frame_data[frame_data_index].frameNo = raw_read_data_.isc_raw_data_header.frame_index;
		isc_image_info->frame_data[frame_data_index].gain = raw_read_data_.isc_raw_data_header.gain;
		isc_image_info->frame_data[frame_data_index].exposure = raw_read_data_.isc_raw_data_header.exposure;

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
	IscGrabColorMode obtained_color_mode = isc_grab_color_mode;

	// camera modesl 0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown
	if (file_read_information_.raw_file_header.camera_model == 0) {
		// サポートされていない
		return CAMCONTROL_E_READ_FILE_FAILED;
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

			obtained_color_mode = IscGrabColorMode::kColorOFF;
		}
		else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 1)) {
			// mono
			isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw.height = height;
			isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

			obtained_color_mode = IscGrabColorMode::kColorOFF;
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
			obtained_color_mode = IscGrabColorMode::kColorON;
		}
		else {
			// error
			return CAMCONTROL_E_READ_FILE_FAILED;
		}

		int ret = raw_data_decoder_->Decode(isc_camera_model, isc_image_info->grab, raw_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
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

	// (2) second data
	bytes_to_read = sizeof(raw_read_data_.isc_raw_data_header);
	readed_size = 0;
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
	readed_size = 0;
	if (FALSE == ReadFile(file_read_information_.handle_file, raw_read_data_.buffer, bytes_to_read, &readed_size, NULL)) {

		DWORD last_err = GetLastError();

		CloseHandle(file_read_information_.handle_file);
		file_read_information_.handle_file = NULL;
		file_read_information_.is_file_ready = false;

		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	file_read_information_.total_read_size += readed_size;

	frame_data_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

	// 次の読み込みのために1データ分　戻す
	LARGE_INTEGER new_pointer = {};

	LARGE_INTEGER  distance_to_move = {};
	distance_to_move.QuadPart = (LONGLONG)(sizeof(raw_read_data_.isc_raw_data_header) + raw_read_data_.isc_raw_data_header.data_size) * -1;
	DWORD move_method = FILE_CURRENT;

	if (!SetFilePointerEx(file_read_information_.handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
		DWORD gs_error = GetLastError();
		char msg[64] = {};
		sprintf_s(msg, "[ERROR]ReadDoubleShutterRawData() SetFilePointerEx error(%d)\n", gs_error);
		OutputDebugStringA(msg);

		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	file_read_information_.total_read_size += distance_to_move.QuadPart;

	// 初期化
	{
		isc_image_info->frame_data[frame_data_index].camera_status.error_code = raw_read_data_.isc_raw_data_header.error_code;
		isc_image_info->frame_data[frame_data_index].camera_status.data_receive_tact_time = 0;

		if (raw_read_data_.isc_raw_data_header.version >= 300) {

			ULARGE_INTEGER ul_int = {};
			ul_int.LowPart = raw_read_data_.isc_raw_data_header.frame_time_low;
			ul_int.HighPart = raw_read_data_.isc_raw_data_header.frame_time_high;

			isc_image_info->frame_data[frame_data_index].frame_time = ul_int.QuadPart;

			// debug
			if (0) {
				struct timespec tm = {};
				tm.tv_sec = ul_int.QuadPart / 1000LL;
				tm.tv_nsec = static_cast<long>((ul_int.QuadPart - (tm.tv_sec * 1000LL)) * 1000000);

				struct tm ltm;
				localtime_s(&ltm, &tm.tv_sec);
				long millisecond = tm.tv_nsec / 1000000LL;

				char time_str[256] = {};
				char dayofweek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

				sprintf_s(time_str, "%d: %d/%d/%d %s %02d:%02d:%02d.%03d\n",
					raw_read_data_.isc_raw_data_header.frame_index,
					ltm.tm_year + 1900,
					ltm.tm_mon + 1,
					ltm.tm_mday,
					dayofweek[ltm.tm_wday],
					ltm.tm_hour,
					ltm.tm_min,
					ltm.tm_sec,
					millisecond);

				OutputDebugStringA(time_str);
			}
		}
		else {
			isc_image_info->frame_data[frame_data_index].frame_time = 0;
		}

		isc_image_info->frame_data[frame_data_index].data_index = file_read_information_.current_frame_number + 1;
		isc_image_info->frame_data[frame_data_index].frameNo = raw_read_data_.isc_raw_data_header.frame_index;
		isc_image_info->frame_data[frame_data_index].gain = raw_read_data_.isc_raw_data_header.gain;
		isc_image_info->frame_data[frame_data_index].exposure = raw_read_data_.isc_raw_data_header.exposure;

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
	isc_grab_color_mode = (file_read_information_.raw_file_header.color_mode == 0) ? IscGrabColorMode::kColorOFF : IscGrabColorMode::kColorON;
	obtained_color_mode = isc_grab_color_mode;

	// camera modesl 0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown
	if (file_read_information_.raw_file_header.camera_model == 0) {
		// サポートされていない
		return CAMCONTROL_E_READ_FILE_FAILED;
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

			obtained_color_mode = IscGrabColorMode::kColorOFF;
		}
		else if ((isc_grab_color_mode == IscGrabColorMode::kColorON) && (raw_read_data_.isc_raw_data_header.type == 1)) {
			// mono
			isc_image_info->frame_data[frame_data_index].raw.width = width * 2;
			isc_image_info->frame_data[frame_data_index].raw.height = height;
			isc_image_info->frame_data[frame_data_index].raw.channel_count = 1;
			size_t cp_size = isc_image_info->frame_data[frame_data_index].raw.width * isc_image_info->frame_data[frame_data_index].raw.height;
			memcpy(isc_image_info->frame_data[frame_data_index].raw.image, raw_read_data_.buffer, cp_size);

			obtained_color_mode = IscGrabColorMode::kColorOFF;
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
			obtained_color_mode = IscGrabColorMode::kColorON;
		}
		else {
			// error
			return CAMCONTROL_E_READ_FILE_FAILED;
		}

		int ret = raw_data_decoder_->Decode(isc_camera_model, isc_image_info->grab, raw_color_mode, isc_get_color_mode, width, height, isc_image_info, frame_data_index);
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

	// 合成
	// camera modesl 0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown
	if (file_read_information_.raw_file_header.camera_model == 0) {
		// サポートされていない
		return CAMCONTROL_E_READ_FILE_FAILED;
	}
	else if (file_read_information_.raw_file_header.camera_model == 1) {
		IscCameraModel isc_camera_model = IscCameraModel::kXC;

		int width = file_read_information_.raw_file_header.max_width;
		int height = file_read_information_.raw_file_header.max_height;

		// check frame number
		int latest_frame_no = isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_LATEST].frameNo;
		int previous_frame_no = isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_PREVIOUS].frameNo;

		bool is_frame_ok = true;
		if (latest_frame_no == 255) {
			int diff = abs(latest_frame_no - previous_frame_no);
			if (diff != 1) {
				is_frame_ok = false;
			}
		}
		else if (previous_frame_no == 255) {
			int diff = abs((latest_frame_no + 256) - previous_frame_no);
			if (diff != 1) {
				is_frame_ok = false;
			}
		}
		else {
			int diff = abs(latest_frame_no - previous_frame_no);
			if (diff != 1) {
				is_frame_ok = false;
			}
		}

		if (!is_frame_ok) {
			// Frameが連続していない
			//char msg[256] = {};
			//sprintf_s(msg, "[ERROR]ReadDoubleShutterRawData() Frame numbers are not consecutive.(%d,%d)\n", latest_frame_no, previous_frame_no);
			//OutputDebugStringA(msg);

			file_read_information_.current_frame_number++;
			return CAMCONTROL_E_READ_FILE_FAILED_RETRY;
		}

		int ret = raw_data_decoder_->CombineImagesForDoubleShutter(isc_camera_model, width, height, isc_image_info);
		if (ret != DPC_E_OK) {
			return CAMCONTROL_E_READ_FILE_FAILED;
		}
	}

	file_read_information_.current_frame_number++;

	return DPC_E_OK;
}

/**
 * 読み込みファイルのヘッダー部を取得します
 *
 * @param[in] play_file_name ファイル名
 * @param[out] raw_file_header　ヘッダーデータ
 * @param[out] play_file_information 再生ファイル情報
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header, IscPlayFileInformation* play_file_information)
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

	int width = raw_file_header->max_width;
	int height = raw_file_header->max_height;

	size_t buff_size = width * height * 2;
	__int64 one_fr_size = sizeof(raw_read_data_.isc_raw_data_header) + buff_size;
	__int64 total_frame_count = (file_size - sizeof(file_read_information_.raw_file_header)) / one_fr_size;

	// read first data

	// haeder
	bytes_to_read = sizeof(raw_read_data_.isc_raw_data_header);
	readed_size = 0;
	IscRawDataHeader isc_raw_data_header_first = {};

	if (FALSE == ReadFile(handle_file, &isc_raw_data_header_first, bytes_to_read, &readed_size, NULL)) {
		CloseHandle(handle_file);

		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	ULARGE_INTEGER ul_int_first = {};
	ul_int_first.LowPart = isc_raw_data_header_first.frame_time_low;
	ul_int_first.HighPart = isc_raw_data_header_first.frame_time_high;

	/*
	struct timespec tm_start = {};
	tm_start.tv_sec = ul_int_first.QuadPart / 1000LL;
	tm_start.tv_nsec = static_cast<long>((ul_int_first.QuadPart - (tm_start.tv_sec * 1000LL)) * 1000000);
	*/

	// 保存間隔を確認するために、100Frame先のフレームの時間を見る
	__int64 test_read_frame_count = 100;

	__int64 frame_interval = 16;

	if (total_frame_count > test_read_frame_count) {

		LARGE_INTEGER new_pointer = {};

		LARGE_INTEGER  distance_to_move = {};
		distance_to_move.QuadPart = (LONGLONG)(sizeof(IscRawFileHeader) + one_fr_size * (test_read_frame_count - 1));
		DWORD move_method = FILE_BEGIN;

		if (!SetFilePointerEx(handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
			DWORD gs_error = GetLastError();
			char msg[64] = {};
			sprintf_s(msg, "[ERROR]CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
			OutputDebugStringA(msg);

			CloseHandle(handle_file);

			return CAMCONTROL_E_READ_FILE_FAILED;
		}

		bytes_to_read = sizeof(raw_read_data_.isc_raw_data_header);
		readed_size = 0;
		IscRawDataHeader isc_raw_data_header_test = {};

		if (FALSE == ReadFile(handle_file, &isc_raw_data_header_test, bytes_to_read, &readed_size, NULL)) {
			CloseHandle(handle_file);

			return CAMCONTROL_E_READ_FILE_FAILED;
		}

		ULARGE_INTEGER ul_int_test = {};
		ul_int_test.LowPart = isc_raw_data_header_test.frame_time_low;
		ul_int_test.HighPart = isc_raw_data_header_test.frame_time_high;

		__int64 elapsed_time_msec = ul_int_test.QuadPart - ul_int_first.QuadPart;
		frame_interval = elapsed_time_msec / test_read_frame_count;

		// 現在のISCシリーズは、最大60FPSなので、小さすぎる値は補正する
		if (frame_interval < 15) {
			frame_interval = 16;
		}
	}
	else {
		frame_interval = 16;
	}

	// 最後のデータまでファイルポインタを進める
	LARGE_INTEGER new_pointer = {};
	LARGE_INTEGER  distance_to_move = {};
	memset(&new_pointer, 0, sizeof(LARGE_INTEGER));
	memset(&distance_to_move, 0, sizeof(LARGE_INTEGER));

	distance_to_move.QuadPart = (LONGLONG)(sizeof(IscRawFileHeader) + one_fr_size * (total_frame_count - 1));
	DWORD move_method = FILE_BEGIN;

	if (!SetFilePointerEx(handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
		DWORD gs_error = GetLastError();
		char msg[64] = {};
		sprintf_s(msg, "[ERROR]CreateWriteFile() SetFilePointerEx error(%d)\n", gs_error);
		OutputDebugStringA(msg);

		CloseHandle(handle_file);

		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	bytes_to_read = sizeof(raw_read_data_.isc_raw_data_header);
	readed_size = 0;
	IscRawDataHeader isc_raw_data_header_last = {};

	if (FALSE == ReadFile(handle_file, &isc_raw_data_header_last, bytes_to_read, &readed_size, NULL)) {
		CloseHandle(handle_file);

		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	ULARGE_INTEGER ul_int_last = {};
	ul_int_last.LowPart = isc_raw_data_header_last.frame_time_low;
	ul_int_last.HighPart = isc_raw_data_header_last.frame_time_high;

	__int64 total_elapsed_time_msec = ul_int_last.QuadPart - ul_int_first.QuadPart;

	// file information
	file_read_information_.play_file_information.total_frame_count = total_frame_count;
	file_read_information_.play_file_information.total_time_sec = (__int64)(total_elapsed_time_msec / (long long)1000);
	file_read_information_.play_file_information.frame_interval = (int)frame_interval;
	file_read_information_.play_file_information.start_time = (__int64)ul_int_first.QuadPart;
	file_read_information_.play_file_information.end_time = (__int64)ul_int_last.QuadPart;

	// ended
	CloseHandle(handle_file);
	handle_file = NULL;

	// copy data
	play_file_information->total_frame_count	= file_read_information_.play_file_information.total_frame_count;
	play_file_information->total_time_sec		= file_read_information_.play_file_information.total_time_sec;
	play_file_information->frame_interval		= file_read_information_.play_file_information.frame_interval;
	play_file_information->start_time			= file_read_information_.play_file_information.start_time;
	play_file_information->end_time				= file_read_information_.play_file_information.end_time;

	return DPC_E_OK;
}

/**
 * 読み込みFrameを指定番号とします
 *
 * @param[in] frame_number 指定番号
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::SetReadFrameNumber(const __int64 frame_number)
{
	if (!file_read_information_.request_fo_designated_number) {
		file_read_information_.request_fo_designated_number = true;
		file_read_information_.designated_number = frame_number;
	}

	return DPC_E_OK;
}

/**
 * 指定Frameまで移動します
 *
 * @param[in] specify_frame_number　指定フレーム
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFileReadControlImpl::MoveToSpecifyFrameNumber(const __int64 specify_frame_number)
{

	if (file_read_information_.handle_file == NULL) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	if ((specify_frame_number < 0) && (specify_frame_number >= file_read_information_.play_file_information.total_frame_count)) {
		return CAMCONTROL_E_INVALID_PARAMETER;
	}

	// Header
	__int64 headr_size = sizeof(IscRawFileHeader);

	int width = file_read_information_.raw_file_header.max_width;
	int height = file_read_information_.raw_file_header.max_height;

	size_t frame_size = width * height * 2;
	__int64 one_data_size = sizeof(raw_read_data_.isc_raw_data_header) + frame_size;

	// move
	LARGE_INTEGER new_pointer = {};

	LARGE_INTEGER  distance_to_move = {};
	distance_to_move.QuadPart = (LONGLONG)(headr_size + (one_data_size * specify_frame_number));
	DWORD move_method = FILE_BEGIN;

	if (!SetFilePointerEx(file_read_information_.handle_file, distance_to_move, (PLARGE_INTEGER)&new_pointer, move_method)) {
		DWORD gs_error = GetLastError();
		char msg[64] = {};
		sprintf_s(msg, "[ERROR]MoveToSpecifyFrameNumber() SetFilePointerEx error(%d)\n", gs_error);
		OutputDebugStringA(msg);

		return CAMCONTROL_E_READ_FILE_FAILED;
	}

	file_read_information_.total_read_size = distance_to_move.QuadPart;
	file_read_information_.current_frame_number = specify_frame_number;

	return DPC_E_OK;
}
