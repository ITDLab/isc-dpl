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

#pragma once

// parameter
struct CameraParameter {
	float b;
	float bf;
	float dinf;
	float setup_angle;
};

// status
enum class CameraStatus {
	kStop,
	kStart
};

// draw settings
enum class DisplayModeDisplay {
	kSingle,
	kDual,
	kOverlapped
};

enum class DisplayModeDepth {
	kDistance,
	kDisparity
};

struct DrawSettings {
	DisplayModeDisplay diaplay_mode;
	DisplayModeDepth disparity_mode;

	double magnification;
	POINT magnification_center;
};

// state
enum class MainStateState {
	kIdle,
	kGrabStart,
	kGrabRun,
	kGrabStop,
	kGrabEnded,
	kPlayStart,
	kPlayReadyToRun,
	kPlayRun,
	kPlayPause,
	kPlayStop,
	kPlayEnded
};

enum class MainStateMode {
	kLiveStreaming,
	kPlay
};

struct IscControl {
	CameraParameter camera_parameter;
	CameraStatus camera_status;
	DrawSettings draw_settings;
	MainStateState main_state;
	MainStateMode main_state_mode;

	ULONGLONG time_to_event;

	bool start_request;
	bool stop_request;
	bool pause_request;
	bool resume_request;
	bool restart_request;

	bool one_shot_save_request;

	IscStartMode isc_start_mode;

	bool is_isc_image_info_valid;
	IscImageInfo isc_image_info;

	bool is_data_proc_result_valid;
	IscDataProcResultData isc_data_proc_result_data;
};

struct IscFeatureRequest {
	DisplayModeDisplay display_mode_display;
	DisplayModeDepth display_mode_depth;

	bool is_disparity;
	bool is_base_image;
	bool is_base_image_correct;
	bool is_compare_image;
	bool is_compare_image_correct;
	bool is_color_image;
	bool is_color_image_correct;
	bool is_dpl_block_matching;
	bool is_dpl_frame_decoder;
};

bool ClearIscControl(IscControl* isc_control);
bool SetupIscControlToStart(const bool is_start, const bool is_record, const bool is_play, const wchar_t* play_file_name, IscFeatureRequest* isc_feature_request, IscControl* isc_control);
bool GetGrabModeString(IscImageInfo* isc_image_info, TCHAR* grab_mmode_string, int maxkength);
DpcDrawLib::ImageDrawMode GetDrawMode(IscFeatureRequest* isc_feature_request, IscControl* isc_control);
bool SetupDrawImageDataSet(const DpcDrawLib::ImageDrawMode mode, IscControl* isc_control, DpcDrawLib::ImageDataSet* image_data_set0, DpcDrawLib::ImageDataSet* image_data_set1, bool* is_dpresult_mode);

bool CheckDiskFreeSpace(const TCHAR* target_folder, const unsigned __int64 requested_size);

class GuiSupport
{

public:

	GuiSupport();
	~GuiSupport();

private:

};




