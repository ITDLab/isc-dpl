﻿// Copyright 2023 ITD Lab Corp.All Rights Reserved.
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

#pragma once

class DplGuiConfiguration
{

public:

	DplGuiConfiguration();
	~DplGuiConfiguration();

	bool Load(const TCHAR* file_path);
	bool Save();
	bool SaveGuiDefault();

	bool GetLogFilePath(TCHAR* file_path, const int max_length) const;
	void SetLogFilePath(const TCHAR* file_path);
	int GetLogLevel() const;
	void SetLogLevel(const int level);

	bool IsEnabledCamera() const;
	void SetEnabledCamera(const bool enabled);
	int GetCameraModel() const;
	void SetCameraModel(const int model);
	bool GetDataRecordPath(TCHAR* path, const int max_length) const;
	void SetDataRecordPath(const TCHAR* path);
	int GetCameraMinimumWriteInterval() const;
	void SetCameraMinimumWriteInterval(const int interval_time);

	bool IsEnabledDataProcLib() const;
	void SetEnabledDataProcLib(const bool enabled);

	double GetDrawMinDistance() const;
	void SetDrawMinDistance(const double distance);
	double GetDrawMaxDistance() const;
	void SetDrawMaxDistance(const double distance);

	double GetMaxDisparity() const;

	bool IsDrawOutsideBounds() const;
	void SetDrawOutsideBounds(const bool enabled);

	int GetGuiLbDisplay() const;
	void SetGuiLbDisplay(const int mode);
	int GetGuiLbDepth() const;
	void SetGuiLbDepth(const int mode);

	bool IsGuiCbSwStereoMathing() const;
	void SetGuiCbSwStereoMathing(const bool enabled);
	bool IsGuiCbDisparityFilter() const;
	void SetGuiCbDisparityFilter(const bool enabled);
	bool IsGuiCbSwCalibration() const;
	void SetGuiCbSwCalibration(const bool enabled);
	bool IsGuiCbDisparity() const;
	void SetGuiCbDisparity(const bool enabled);
	bool IsGuiCbBaseImage() const;
	void SetGuiCbBaseImage(const bool enabled);
	bool IsGuiCbBaseImageCorrected() const;
	void SetGuiCbBaseImageCorrected(const bool enabled);
	bool IsGuiCbMatchingImage() const;
	void SetGuiCbMatchingImage(const bool enabled);
	bool IsGuiCbMatchingImageCorrected() const;
	void SetGuiCbMatchingImageCorrected(const bool enabled);
	bool IsGuiCbColorImage() const;
	void SetGuiCbColorImage(const bool enabled);
	bool IsGuiCbColorImageCorrected() const;
	void SetGuiCbColorImageCorrected(const bool enabled);
	int GetGuiCmbShutterControlMode() const;
	void SetGuiCmbShutterControlMode(const int mode);
	bool IsOptionExtendedMatching() const;
	void SetOptionExtendedMatching(const bool enabled);
	bool IsOptionSearchRange128() const;
	void SetOptionSearchRange128(const bool enabled);

private:

	bool successfully_loaded_;

	TCHAR configuration_file_path_[_MAX_PATH];	/**< 設定ファイルのパス */
	TCHAR configuration_file_name_[_MAX_PATH];	/**< 設定ファイルのフルパス */

	TCHAR log_file_path_[_MAX_PATH];			/**< ログ保存のパス */
	int log_level_;								/**< ログモード */
		
	bool enabled_camera_;						/**< カメラ有効 */
	int camera_model_;							/**< カメラ型式 0:VM 1:XC 2:4K 3:4KA 4:4KJ */
	TCHAR data_record_path_[_MAX_PATH];			/**< データ保存先 */
	int minimum_write_interval_time_;			/**< 書き込みの最小空き時間 (msec) */

	bool enabled_data_proc_library_;			/**< データ処理モジュール有効 */

	double draw_min_distance_;					/**< 表示最小距離 */
	double draw_max_distance_;					/**< 表示最大距離 */
	bool draw_outside_bounds_;					/**< 表示最小～最大外を描画する */

	double max_disparity_;						/**< 視差最大 */

	// gui default
	int lb_display_;							/**< gui list box display */
	int lb_depth_;								/**< gui list box depth */

	bool cb_sw_stereo_matching_;				/**< gui check box stereo matching */
	bool cb_disparity_filter_;					/**< gui check box disparity filter */
	bool cb_sw_calibration_;					/**< gui check box software calibration */

	bool cb_disparity_;							/**< gui check disparity */
	bool cb_base_image_;						/**< gui check box base image */
	bool cb_base_image_correted_;				/**< gui check box base image correted */
	bool cb_matching_image_;					/**< gui check box matching image */
	bool cb_matching_image_correted_;			/**< gui check box matching image correted */
	bool cb_color_image_;						/**< gui check box color image */
	bool cb_color_image_correted_;				/**< gui check box color image correted */
	int cmb_shutter_control_mode_;				/**< gui combo box shutter control mode */

	// option parameters
	bool extended_matching_;					/**< extended matching */
	bool search_range128_;						/**< search range , set to 128 (disable case:256) */
};

