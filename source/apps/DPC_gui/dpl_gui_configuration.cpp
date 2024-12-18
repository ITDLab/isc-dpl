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
 * @file dpl_gui_configuration.cpp
 * @brief support functions for GUI Dialog
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 *
 * @details This class provides a funtion for GUI.
 */

#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "dpl_gui_configuration.h"

DplGuiConfiguration::DplGuiConfiguration():
	successfully_loaded_(false),
	configuration_file_path_(),
	configuration_file_name_(),
	log_file_path_(),
	log_level_(0),
	enabled_camera_(false),
	camera_model_(0),
	data_record_path_(),
	minimum_write_interval_time_(0),
	enabled_data_proc_library_(false),
	draw_min_distance_(0),
	draw_max_distance_(10.0),
	draw_outside_bounds_(true),
	max_disparity_(255),
	lb_display_(1),
	lb_depth_(0),
	cb_sw_stereo_matching_(true),
	cb_disparity_filter_(true),
	cb_sw_calibration_(false),
	cb_disparity_(false),
	cb_base_image_(true),
	cb_base_image_correted_(false),
	cb_matching_image_(false),
	cb_matching_image_correted_(false),
	cb_color_image_(false),
	cb_color_image_correted_(false),
	cmb_shutter_control_mode_(0)
{

	

}

DplGuiConfiguration::~DplGuiConfiguration()
{

}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @return true:成功
 */
bool DplGuiConfiguration::Load(const TCHAR* file_path)
{
	successfully_loaded_ = false;

	if (file_path == nullptr) {
		return false;
	}

	_stprintf_s(configuration_file_path_, _T("%s"), file_path);
	_stprintf_s(configuration_file_name_, _T("%s\\DPLGuiConfig.ini"), configuration_file_path_);

	/*

		DPLGuiConfig.ini

		[SYSTEM]
		LOG_LEVEL=0
		LOG_FILE_PATH=c:\temp

		[CAMERA]
		ENABLED=0
		CAMERA_MODEL=0		;0:VM 1:XC 2:4K 3:4KA 4:4KJ
		DATA_RECORD_PATH=c:\temp
		MINIMUM_WRITE_INTERVAL=0

		[DATA_PROC_MODULES]
		COUNT=0
		ENABLED_0=0
		ENABLED_1=0

		[DRAW]
		MIN_DISTANCE=0
		MAX_DISTANCE=10
		DRAW_OUTSIDE_BOUNDS=1

		[GUI_DEFAULT]
		LB_DISPLAY=0
		LB_DEPTH=0

		CB_SW_STEREO_MATCHING=0
		CB_DISPAIRTY_FILTER=0
		CB_SW_CALIBRATION=0

		CB_DISPARITY=0
		CB_BASE_IMAGE=0
		CB_BASE_IMAGE_CORRECTED=0
		CB_MATCHING_IMAGE=0
		CB_MATCHING_IMAGE_CORRECTED=0
		CB_COLOR_IMAGE=0
		CB_COLOR_IMAGE_CORRECTED=0

	*/
	
	TCHAR returned_string[1024] = {};

	// [SYSTEM]
	GetPrivateProfileString(_T("SYSTEM"), _T("LOG_LEVEL"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	log_level_ = _tstoi(returned_string);

	GetPrivateProfileString(_T("SYSTEM"), _T("LOG_FILE_PATH"), _T("c:\\temp"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	_stprintf_s(log_file_path_, _T("%s"), returned_string);

	// [CAMERA]
	GetPrivateProfileString(_T("CAMERA"), _T("ENABLED"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	int temp_value = _tstoi(returned_string);
	enabled_camera_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("CAMERA"), _T("CAMERA_MODEL"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	camera_model_ = _tstoi(returned_string);
	if (camera_model_ < 0 || camera_model_ > 4) {
		camera_model_ = 0;
	}

	GetPrivateProfileString(_T("CAMERA"), _T("DATA_RECORD_PATH"), _T("c:\\temp"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	_stprintf_s(data_record_path_, _T("%s"), returned_string);


	GetPrivateProfileString(_T("CAMERA"), _T("MINIMUM_WRITE_INTERVAL"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	minimum_write_interval_time_ = temp_value >= 0 ? temp_value : 0;

	if (camera_model_ == 0) {
		// 0:VM
		max_disparity_ = 127.0;
	}
	else if (camera_model_ == 1) {
		// 1:XC
		max_disparity_ = 255.0;
	}
	else if (camera_model_ == 2) {
		// 2:4K
		max_disparity_ = 255.0;
	}
	else if (camera_model_ == 3) {
		// 3:4KA
		max_disparity_ = 255.0;
	}
	else if (camera_model_ == 4) {
		// 4:4KJ
		max_disparity_ = 255.0;
	}
	else {
		max_disparity_ = 255.0;
	}

	// [DATA_PROC_MODULES]
	GetPrivateProfileString(_T("DATA_PROC_MODULES"), _T("ENABLED"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	enabled_data_proc_library_ = temp_value == 1 ? true : false;

	// [DRAW]
	GetPrivateProfileString(_T("DRAW"), _T("MIN_DISTANCE"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	draw_min_distance_ = _ttof(returned_string);

	GetPrivateProfileString(_T("DRAW"), _T("MAX_DISTANCE"), _T("20"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	draw_max_distance_ = _ttof(returned_string);

	if (draw_min_distance_ >= draw_max_distance_) {
		// error
		draw_min_distance_ = 0;
		draw_max_distance_ = 20;

		TCHAR write_string[256] = {};
		_stprintf_s(write_string, _T("%.3f"), draw_min_distance_);
		WritePrivateProfileString(_T("DRAW"), _T("MIN_DISTANCE"), write_string, configuration_file_name_);

		_stprintf_s(write_string, _T("%.3f"), draw_max_distance_);
		WritePrivateProfileString(_T("DRAW"), _T("MAX_DISTANCE"), write_string, configuration_file_name_);
	}

	GetPrivateProfileString(_T("DRAW"), _T("DRAW_OUTSIDE_BOUNDS"), _T("1"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	draw_outside_bounds_ = temp_value == 1 ? true : false;

	// [GUI_DEFAULT]
	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("LB_DISPLAY"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	lb_display_ = _tstoi(returned_string);

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("LB_DEPTH"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	lb_depth_ = _tstoi(returned_string);

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_SW_STEREO_MATCHING"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_sw_stereo_matching_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_DISPAIRTY_FILTER"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_disparity_filter_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_SW_CALIBRATION"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_sw_calibration_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_DISPARITY"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_disparity_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_BASE_IMAGE"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_base_image_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_BASE_IMAGE_CORRECTED"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_base_image_correted_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_MATCHING_IMAGE"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_matching_image_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_MATCHING_IMAGE_CORRECTED"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_matching_image_correted_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_COLOR_IMAGE"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_color_image_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CB_COLOR_IMAGE_CORRECTED"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cb_color_image_correted_ = temp_value == 1 ? true : false;

	GetPrivateProfileString(_T("GUI_DEFAULT"), _T("CMB_SHUTTER_CONTROL_MODE"), _T("0"), returned_string, sizeof(returned_string) / sizeof(TCHAR), configuration_file_name_);
	temp_value = _tstoi(returned_string);
	cmb_shutter_control_mode_ = temp_value;

	// for 4K
	// 4Kカメラは、データ処理ライブラリの対象外です
	// 4K cameras are not covered by the data processing library
	if (camera_model_ == 0) {
		// 0:VM
	}
	else if (camera_model_ == 1) {
		// 1:XC
	}
	else if (camera_model_ == 2) {
		// 2:4K
		enabled_data_proc_library_ = false;
	}
	else if (camera_model_ == 3) {
		// 3:4KA
		enabled_data_proc_library_ = false;
	}
	else if (camera_model_ == 4) {
		// 4:4KJ
		enabled_data_proc_library_ = false;
	}

	successfully_loaded_ = true;

	return true;
}

/**
 * 設定ファイルへ設定を保存
 *
 * @return int 戻り値の説明
 */
bool DplGuiConfiguration::Save()
{

	if (!successfully_loaded_) {
		return false;
	}

	TCHAR write_string[256] = {};

	// [SYSTEM]
	_stprintf_s(write_string, _T("%d"), log_level_ ? 1 : 0);
	WritePrivateProfileString(_T("SYSTEM"), _T("LOG_LEVEL"), write_string, configuration_file_name_);

	WritePrivateProfileString(_T("SYSTEM"), _T("LOG_FILE_PATH"), log_file_path_, configuration_file_name_);

	// [CAMERA]
	_stprintf_s(write_string, _T("%d"), enabled_camera_ ? 1 : 0);
	WritePrivateProfileString(_T("CAMERA"), _T("ENABLED"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), camera_model_);
	WritePrivateProfileString(_T("CAMERA"), _T("CAMERA_MODEL"), write_string, configuration_file_name_);

	WritePrivateProfileString(_T("CAMERA"), _T("DATA_RECORD_PATH"), data_record_path_, configuration_file_name_);

	// [DATA_PROC_MODULES]
	_stprintf_s(write_string, _T("%d"), enabled_data_proc_library_ ? 1 : 0);
	WritePrivateProfileString(_T("DATA_PROC_MODULES"), _T("ENABLED"), write_string, configuration_file_name_);

	// [DRAW]
	_stprintf_s(write_string, _T("%.3f"), draw_min_distance_);
	WritePrivateProfileString(_T("DRAW"), _T("MIN_DISTANCE"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%.3f"), draw_max_distance_);
	WritePrivateProfileString(_T("DRAW"), _T("MAX_DISTANCE"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), (int)draw_outside_bounds_);
	WritePrivateProfileString(_T("DRAW"), _T("DRAW_OUTSIDE_BOUNDS"), write_string, configuration_file_name_);

	return true;
}

/**
 * 設定ファイルへGui設定を保存
 *
 * @return int 戻り値の説明
 */
bool DplGuiConfiguration::SaveGuiDefault()
{
	if (!successfully_loaded_) {
		return false;
	}

	TCHAR write_string[256] = {};

	// [GUI_DEFAULT]
	_stprintf_s(write_string, _T("%d"), (int)lb_display_);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("LB_DISPLAY"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), (int)lb_depth_);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("LB_DEPTH"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_sw_stereo_matching_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_SW_STEREO_MATCHING"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_disparity_filter_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_DISPAIRTY_FILTER"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_sw_calibration_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_SW_CALIBRATION"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_disparity_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_DISPARITY"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_base_image_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_BASE_IMAGE"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_base_image_correted_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_BASE_IMAGE_CORRECTED"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_matching_image_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_MATCHING_IMAGE"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_matching_image_correted_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_MATCHING_IMAGE_CORRECTED"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_color_image_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_COLOR_IMAGE"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cb_color_image_correted_ ? 1 : 0);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CB_COLOR_IMAGE_CORRECTED"), write_string, configuration_file_name_);

	_stprintf_s(write_string, _T("%d"), cmb_shutter_control_mode_);
	WritePrivateProfileString(_T("GUI_DEFAULT"), _T("CMB_SHUTTER_CONTROL_MODE"), write_string, configuration_file_name_);

	return true;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
bool DplGuiConfiguration::GetLogFilePath(TCHAR* file_path, const int max_length) const
{
	_stprintf_s(file_path, max_length, _T("%s"), log_file_path_);

	return true;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetLogFilePath(const TCHAR* file_path)
{
	_stprintf_s(log_file_path_, _T("%s"), file_path);

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
int DplGuiConfiguration::GetLogLevel() const
{
	return log_level_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetLogLevel(const int level)
{

	log_level_ = level;

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
bool DplGuiConfiguration::IsEnabledCamera() const
{
	return enabled_camera_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetEnabledCamera(const bool enabled)
{
	enabled_camera_ = enabled;

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
int DplGuiConfiguration::GetCameraModel() const
{
	return camera_model_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetCameraModel(const int model)
{
	camera_model_ = model;

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
bool DplGuiConfiguration::GetDataRecordPath(TCHAR* path, const int max_length) const
{
	_stprintf_s(path, max_length, _T("%s"), data_record_path_);

	return true;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetDataRecordPath(const TCHAR* path)
{
	_stprintf_s(data_record_path_, _T("%s"), path);

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @return int 書き込みの最小空き時間
 */
int DplGuiConfiguration::GetCameraMinimumWriteInterval() const
{
	return minimum_write_interval_time_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] interval_time 書き込みの最小空き時間
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetCameraMinimumWriteInterval(const int interval_time)
{
	if (interval_time >= 0) {
		minimum_write_interval_time_ = interval_time;
	}

	return;
}


/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
bool DplGuiConfiguration::IsEnabledDataProcLib() const
{
	return enabled_data_proc_library_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] file_path 設定ファイルのパス
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetEnabledDataProcLib(const bool enabled)
{
	enabled_data_proc_library_ = enabled;

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] 表示最小距離
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
double DplGuiConfiguration::GetDrawMinDistance() const
{
	return draw_min_distance_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] 表示最小距離
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetDrawMinDistance(const double distance)
{
	draw_min_distance_ = distance;

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] 表示最大距離
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
double DplGuiConfiguration::GetDrawMaxDistance() const
{
	return draw_max_distance_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] 表示最大距離
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetDrawMaxDistance(const double distance)
{
	draw_max_distance_ = distance;

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] 視差の最大値
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
double DplGuiConfiguration::GetMaxDisparity() const
{
	return max_disparity_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] 範囲外を描画する
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
bool DplGuiConfiguration::IsDrawOutsideBounds() const
{
	return draw_outside_bounds_;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @param[in] 範囲外を描画する
 * @param[out] paramB 第一引数の説明
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetDrawOutsideBounds(const bool enabled)
{
	draw_outside_bounds_ = enabled;

	return;
}

/**
 * 設定ファイルより設定を読み込み
 *
 * @return int 戻り値の説明
 */
int DplGuiConfiguration::GetGuiLbDisplay() const
{
	return lb_display_;
}

/**
 * 設定ファイルを更新
 *
 * @param[in] 視差の最大値
 *
 * @return int 戻り値の説明
 */
void DplGuiConfiguration::SetGuiLbDisplay(const int mode)
{
	lb_display_ = mode;
	return;
}

int DplGuiConfiguration::GetGuiLbDepth() const
{
	return lb_depth_;
}

void DplGuiConfiguration::SetGuiLbDepth(const int mode)
{
	lb_depth_ = mode;
	return;
}

bool DplGuiConfiguration::IsGuiCbSwStereoMathing() const
{
	return cb_sw_stereo_matching_;
}

void DplGuiConfiguration::SetGuiCbSwStereoMathing(const bool enabled)
{
	cb_sw_stereo_matching_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbDisparityFilter() const
{
	return cb_disparity_filter_;
}

void DplGuiConfiguration::SetGuiCbDisparityFilter(const bool enabled)
{
	cb_disparity_filter_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbSwCalibration() const
{
	return cb_sw_calibration_;
}

void DplGuiConfiguration::SetGuiCbSwCalibration(const bool enabled)
{
	cb_sw_calibration_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbDisparity() const
{
	return cb_disparity_;
}

void DplGuiConfiguration::SetGuiCbDisparity(const bool enabled)
{
	cb_disparity_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbBaseImage() const
{
	return cb_base_image_;
}

void DplGuiConfiguration::SetGuiCbBaseImage(const bool enabled)
{
	cb_base_image_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbBaseImageCorrected() const
{
	return cb_base_image_correted_;
}

void DplGuiConfiguration::SetGuiCbBaseImageCorrected(const bool enabled)
{
	cb_base_image_correted_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbMatchingImage() const
{
	return cb_matching_image_;
}

void DplGuiConfiguration::SetGuiCbMatchingImage(const bool enabled)
{
	cb_matching_image_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbMatchingImageCorrected() const
{
	return cb_matching_image_correted_;
}

void DplGuiConfiguration::SetGuiCbMatchingImageCorrected(const bool enabled)
{
	cb_matching_image_correted_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbColorImage() const
{
	return cb_color_image_;
}

void DplGuiConfiguration::SetGuiCbColorImage(const bool enabled)
{
	cb_color_image_ = enabled;
	return;
}

bool DplGuiConfiguration::IsGuiCbColorImageCorrected() const
{
	return cb_color_image_correted_;
}

void DplGuiConfiguration::SetGuiCbColorImageCorrected(const bool enabled)
{
	cb_color_image_correted_ = enabled;
	return;
}

int DplGuiConfiguration::GetGuiCmbShutterControlMode() const
{
	return cmb_shutter_control_mode_;
}

void DplGuiConfiguration::SetGuiCmbShutterControlMode(const int mode)
{
	cmb_shutter_control_mode_ = mode;
	return;
}

