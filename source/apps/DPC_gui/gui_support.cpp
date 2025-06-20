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

/**
 * @file gui_support.cpp
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
#include <time.h>
#include <stdint.h>
#include <cassert>
#include <Psapi.h>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"

#include "dpl_gui_configuration.h"
#include "dpc_image_writer.h"
#include "dpc_draw_lib.h"

#include "gui_support.h"

bool ClearIscControl(IscControl* isc_control)
{
	memset(&isc_control->camera_parameter, 0, sizeof(isc_control->camera_parameter));
	isc_control->camera_status = CameraStatus::kStop;
	isc_control->draw_settings.diaplay_mode = DisplayModeDisplay::kSingle;
	isc_control->draw_settings.disparity_mode = DisplayModeDepth::kDistance;
	isc_control->draw_settings.magnification = 1.0;
	isc_control->draw_settings.magnification_center = { 0,0 };
	isc_control->main_state = MainStateState::kIdle;
	isc_control->main_state_mode = MainStateMode::kLiveStreaming;
	isc_control->start_request = false;
	isc_control->stop_request = false;
	isc_control->pause_request = false;
	isc_control->resume_request = false;
	isc_control->restart_request = false;
	isc_control->one_shot_save_request = false;
	isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kParallax;
	isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
	isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
	isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kBGR;
	isc_control->isc_start_mode.isc_grab_start_mode.isc_record_mode = IscRecordMode::kRecordOff;
	isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOff;
	isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.interval = 30;
	memset(isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, 0, sizeof(isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name));

	isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = false;
	isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = false;
	isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = false;

	memset(&isc_control->isc_image_info, 0, sizeof(isc_control->isc_image_info));
	memset(&isc_control->isc_data_proc_result_data, 0, sizeof(isc_control->isc_data_proc_result_data));

	return true;
}

bool SetupIscControlToStart(const bool is_start, const bool is_record, const bool is_play, const wchar_t* play_file_name, IscFeatureRequest* isc_feature_request, IscControl* isc_control)
{
	if (is_start && (isc_control->camera_status != CameraStatus::kStop)) {
		__debugbreak();
	}

	if (is_start && is_play) {
		__debugbreak();
	}

	if (is_record && is_play) {
		__debugbreak();
	}

	if (isc_control->camera_status == CameraStatus::kStop) {
		// stop -> start
		isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode		= IscGrabMode::kParallax;
		isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode	= IscGrabColorMode::kColorOFF;
		isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode	= IscGetModeRaw::kRawOff;
		isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode	= IscGetModeColor::kBGR;
		isc_control->isc_start_mode.isc_grab_start_mode.isc_record_mode		= IscRecordMode::kRecordOff;
		isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode		= IscPlayMode::kPlayOff;
		isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.interval = 30;
		memset(isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, 0, sizeof(isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name));

		isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = false;
		isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = false;
		isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = false;

		// initialize buffer status
		isc_control->is_isc_image_info_valid = false;
		isc_control->is_data_proc_result_valid = false;

		// 0: double 1: single 2:overlap
		isc_control->draw_settings.diaplay_mode = isc_feature_request->display_mode_display;

		// 0:distance 1:disparity
		isc_control->draw_settings.disparity_mode = isc_feature_request->display_mode_depth;

		// magnification
		isc_control->draw_settings.magnification = 1.0;
		isc_control->draw_settings.magnification_center = { 0,0 };

		// wait time
		isc_control->isc_start_mode.isc_grab_start_mode.isc_get_mode.wait_time = 100;

		// mode
		const bool is_disparity				= isc_feature_request->is_disparity;
		const bool is_mono_s0_image			= isc_feature_request->is_mono_s0_image;
		const bool is_mono_s0_image_correct	= isc_feature_request->is_mono_s0_image_correct;
		const bool is_mono_s1_image			= isc_feature_request->is_mono_s1_image;
		const bool is_mono_s1_image_correct	= isc_feature_request->is_mono_s1_image_correct;
		const bool is_color_image			= isc_feature_request->is_color_image;
		const bool is_color_image_correct	= isc_feature_request->is_color_image_correct;
		const bool is_dpl_stereo_matching	= isc_feature_request->is_dpl_stereo_matching;
		const bool is_dpl_frame_decoder		= isc_feature_request->is_dpl_frame_decoder;
		const bool is_dpl_disparity_filter	= isc_feature_request->is_dpl_disparity_filter;

		if (is_dpl_stereo_matching) {
			// 補正後画像モードを必要とします
			isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kCorrect;
			isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
			isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOn;
			isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = true;
			if (is_dpl_disparity_filter) {
				isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = true;
				isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = true;
			}
			else {
				isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = false;
				isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = false;
			}
			if (is_color_image) {
				isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorON;
				if (is_color_image_correct) {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwb;
				}
				else {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwbNoCorrect;
				}
			}
		}
		else if (!is_dpl_stereo_matching && is_dpl_disparity_filter) {
			isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kParallax;
			isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
			isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOn;
			isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = false;
			isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = true;
			isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = true;
			if (is_color_image) {
				isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorON;
				if (is_color_image_correct) {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwb;
				}
				else {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwbNoCorrect;
				}
			}
		}
		else if (!is_dpl_stereo_matching && !is_dpl_disparity_filter) {
			if (is_disparity) {
				isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kParallax;
				isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
				isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
				if (is_color_image) {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorON;
					if (is_color_image_correct) {
						isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwb;
					}
					else {
						isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwbNoCorrect;
					}
				}
			}
			else if (is_mono_s0_image) {
				if (is_mono_s0_image_correct) {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kCorrect;
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
				}
				else {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kBeforeCorrect;
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
				}
			}
			else if (is_mono_s1_image) {
				if (is_mono_s1_image_correct) {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kCorrect;
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
				}
				else {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kBeforeCorrect;
					isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
				}
			}
			else if (is_color_image) {
				isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kParallax;
				isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
				isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
				isc_control->isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorON;
				if (is_color_image_correct) {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwb;
				}
				else {
					isc_control->isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwbNoCorrect;
				}
			}
		}

		if (is_record) {
			isc_control->isc_start_mode.isc_grab_start_mode.isc_record_mode = IscRecordMode::kRecordOn;
			isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOn;
		}
		else {
			isc_control->isc_start_mode.isc_grab_start_mode.isc_record_mode = IscRecordMode::kRecordOff;
		}

		if (is_play) {
			isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOn;
			isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.interval = 16;

			if (play_file_name[0] != 0) {
				swprintf_s(isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, L"%s", play_file_name);
			}
			else {
				isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOff;
			}
		}
		else {
			isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOff;
		}

		if (is_play) {
			isc_control->main_state_mode = MainStateMode::kPlay;
		}
		else {
			isc_control->main_state_mode = MainStateMode::kLiveStreaming;
		}

		isc_control->start_request = true;
		isc_control->stop_request = false;
		isc_control->pause_request = false;
		isc_control->resume_request = false;
		isc_control->restart_request = false;
		isc_control->one_shot_save_request = false;
	}
	else {
		// start -> stop
		isc_control->start_request = false;
		isc_control->stop_request = true;
		isc_control->pause_request = false;
		isc_control->resume_request = false;
		isc_control->restart_request = false;
	}

	return true;
}

bool GetGrabModeString(IscImageInfo* isc_image_info, TCHAR* grab_mmode_string, int maxkength)
{

	switch (isc_image_info->grab) {
	case IscGrabMode::kParallax:
		_stprintf_s(grab_mmode_string, maxkength, _T("Parallax"));
		break;

	case IscGrabMode::kCorrect:
		_stprintf_s(grab_mmode_string, maxkength, _T("Correct"));
		break;

	case IscGrabMode::kBeforeCorrect:
		_stprintf_s(grab_mmode_string, maxkength, _T("Before Correct"));
		break;

	case IscGrabMode::kBayerS0:
		_stprintf_s(grab_mmode_string, maxkength, _T("Bayer(0)"));
		break;

	case IscGrabMode::kBayerS1:
		_stprintf_s(grab_mmode_string, maxkength, _T("Bayer(1)"));
		break;

	default:
		_stprintf_s(grab_mmode_string, maxkength, _T("Unknown"));
		break;
	}

	return true;
}

DpcDrawLib::ImageDrawMode GetDrawMode(IscFeatureRequest* isc_feature_request, IscControl* isc_control)
{
	//enum class ImageDrawMode {
	//								/**< image_data_list	[0]					[1]				*/
	//	kMonoS0,					/**< 0:					image_mono_s0						*/
	//	kMonoS1,					/**< 1:					image_mono_s1						*/
	//	kDepth,						/**< 2:					depth								*/
	//	kColor,						/**< 3:					image_color							*/
	//	kMonoS0MonoS1,				/**< 4:					image_mono_s0,		image_mono_s1	*/
	//	kDepthMonoS0,				/**< 5:					depth_data,			image_mono_s0	*/
	//	kDepthColor,				/**< 6:					depth_data,			image_color		*/
	//	kOverlapedDepthMonoS0,		/**< 7:					depth_data,			image_mono_s0	*/

	//	kDplImage,					/**< 8:					image_dpl							*/
	//	kDplImageMonoS0,			/**< 9:					image_dpl,			image_mono_s0	*/
	//	kDplImageColor,				/**< 10:				image_dpl,			image_color		*/
	//	kDplDepth,					/**< 11:				depth_dpl							*/
	//	kDplDepthMonoS0,			/**< 12:				depth_dpl,			image_mono_s0	*/
	//	kDplDepthColor,				/**< 13:				depth_dpl,			image_color		*/
	//	kDplDepthDepth,				/**< 14:				depth_dpl,			depth			*/
	//	kOverlapedDplDepthMonoS0,	/**< 15:				depth_dpl,			image_mono_s0	*/
	//	kUnknown = 99				/**< 99:				(error case)						*/
	//};

	const bool is_disparity = isc_feature_request->is_disparity;
	const bool is_mono_s0_image = isc_feature_request->is_mono_s0_image;
	const bool is_mono_s0_image_correct = isc_feature_request->is_mono_s0_image_correct;
	const bool is_mono_s1_image = isc_feature_request->is_mono_s1_image;
	const bool is_mono_s1_image_correct = isc_feature_request->is_mono_s1_image_correct;
	const bool is_color_image = isc_feature_request->is_color_image;
	const bool is_color_image_correct = isc_feature_request->is_color_image_correct;
	const bool is_dpl_stereo_matching = isc_feature_request->is_dpl_stereo_matching;
	const bool is_dpl_frame_decoder = isc_feature_request->is_dpl_frame_decoder;
	const bool is_dpl_disparity_filter = isc_feature_request->is_dpl_disparity_filter;


	const IscGrabMode isc_grab_mode = isc_control->isc_image_info.grab;

	// set up draw mode
	DpcDrawLib::ImageDrawMode mode = DpcDrawLib::ImageDrawMode::kUnknown;

	if (isc_control->draw_settings.diaplay_mode == DisplayModeDisplay::kSingle) {
		if (isc_grab_mode == IscGrabMode::kCorrect) {
			if (is_dpl_stereo_matching) {
				mode = DpcDrawLib::ImageDrawMode::kDplDepth;
			}
			else if (is_dpl_stereo_matching && is_dpl_disparity_filter) {
				mode = DpcDrawLib::ImageDrawMode::kDplDepth;
			}
			else if (is_mono_s0_image) {
				mode = DpcDrawLib::ImageDrawMode::kMonoS0;
			}
			else if (is_mono_s1_image) {
				mode = DpcDrawLib::ImageDrawMode::kMonoS1;
			}
		}
		else if (isc_grab_mode == IscGrabMode::kParallax) {
			if (is_dpl_stereo_matching) {
				mode = DpcDrawLib::ImageDrawMode::kDplDepth;
			}
			else if (!is_dpl_stereo_matching && is_dpl_disparity_filter) {
				mode = DpcDrawLib::ImageDrawMode::kDplDepth;
			}
			else if (is_disparity) {
				mode = DpcDrawLib::ImageDrawMode::kDepth;
			}
			else if (is_mono_s0_image) {
				mode = DpcDrawLib::ImageDrawMode::kMonoS0;
			}
			else if (is_mono_s1_image) {
				mode = DpcDrawLib::ImageDrawMode::kMonoS1;
			}
			else if (is_color_image) {
				mode = DpcDrawLib::ImageDrawMode::kColor;
			}
		}
		else if (isc_grab_mode == IscGrabMode::kBeforeCorrect) {
			if (is_mono_s0_image) {
				mode = DpcDrawLib::ImageDrawMode::kMonoS0;
			}
			else if (is_mono_s1_image) {
				mode = DpcDrawLib::ImageDrawMode::kMonoS1;
			}
		}
	}
	else if (isc_control->draw_settings.diaplay_mode == DisplayModeDisplay::kDual) {
		if (isc_grab_mode == IscGrabMode::kCorrect) {
			if (is_dpl_stereo_matching) {
				if (is_mono_s0_image) {
					mode = DpcDrawLib::ImageDrawMode::kDplDepthMonoS0;
				}
				else if (is_color_image) {
					mode = DpcDrawLib::ImageDrawMode::kDplDepthColor;
				}
			}
			else if (is_dpl_stereo_matching && is_dpl_disparity_filter) {
				if (is_mono_s0_image) {
					mode = DpcDrawLib::ImageDrawMode::kDplDepthMonoS0;
				}
			}
			else if (is_mono_s0_image && is_mono_s1_image) {
				mode = DpcDrawLib::ImageDrawMode::kMonoS0MonoS1;
			}
		}
		else if (isc_grab_mode == IscGrabMode::kParallax) {
			if (is_dpl_disparity_filter) {
				if (is_color_image) {
					mode = DpcDrawLib::ImageDrawMode::kDplDepthColor;
				}
				else if (is_disparity) {
					mode = DpcDrawLib::ImageDrawMode::kDplDepthDepth;
				}
				else {
					mode = DpcDrawLib::ImageDrawMode::kDplDepthMonoS0;
				}
			}
			else if (is_disparity) {
				if (is_color_image) {
					mode = DpcDrawLib::ImageDrawMode::kDepthColor;
				}
				else {
					mode = DpcDrawLib::ImageDrawMode::kDepthMonoS0;
				}
			}
		}
		else if (isc_grab_mode == IscGrabMode::kBeforeCorrect) {
			if (is_mono_s0_image) {
				mode = DpcDrawLib::ImageDrawMode::kMonoS0MonoS1;
			}
		}
	}
	else if (isc_control->draw_settings.diaplay_mode == DisplayModeDisplay::kOverlapped) {
		if (isc_grab_mode == IscGrabMode::kCorrect) {
			if (is_dpl_stereo_matching) {
				if (is_mono_s0_image) {
					mode = DpcDrawLib::ImageDrawMode::kOverlapedDplDepthMonoS0;
				}
			}
			else if (is_dpl_stereo_matching && is_dpl_disparity_filter) {
				if (is_mono_s0_image) {
					mode = DpcDrawLib::ImageDrawMode::kOverlapedDplDepthMonoS0;
				}
			}
		}
		else if (isc_grab_mode == IscGrabMode::kParallax) {
			if (is_dpl_stereo_matching) {
				if (is_mono_s0_image) {
					mode = DpcDrawLib::ImageDrawMode::kOverlapedDplDepthMonoS0;
				}
			}
			else if (is_disparity) {
				if (is_mono_s0_image) {
					mode = DpcDrawLib::ImageDrawMode::kOverlapedDepthMonoS0;
				}
			}
		}
	}

	return mode;
}

bool SetupDrawImageDataSet(const DpcDrawLib::ImageDrawMode mode, IscControl* isc_control, DpcDrawLib::ImageDataSet* image_data_set0, DpcDrawLib::ImageDataSet* image_data_set1, bool* is_dpresult_mode)
{
	// claer
	DpcDrawLib::ImageDataSet* image_data_set[2] = { image_data_set0, image_data_set1 };

	for (int i = 0; i < 2; i++) {
		image_data_set[0]->valid = false;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kMonoS0;

		for (int j = 0; j < 2; j++) {
			image_data_set[0]->image_data_list[j].image_mono_s0.width = 0;
			image_data_set[0]->image_data_list[j].image_mono_s0.height = 0;
			image_data_set[0]->image_data_list[j].image_mono_s0.channel_count = 0;

			image_data_set[0]->image_data_list[j].image_mono_s1.width = 0;
			image_data_set[0]->image_data_list[j].image_mono_s1.height = 0;
			image_data_set[0]->image_data_list[j].image_mono_s1.channel_count = 0;

			image_data_set[0]->image_data_list[j].depth.width = 0;
			image_data_set[0]->image_data_list[j].depth.height = 0;

			image_data_set[0]->image_data_list[j].image_color.width = 0;
			image_data_set[0]->image_data_list[j].image_color.height = 0;
			image_data_set[0]->image_data_list[j].image_color.channel_count = 0;

			image_data_set[0]->image_data_list[j].image_dpl.width = 0;
			image_data_set[0]->image_data_list[j].image_dpl.height = 0;
			image_data_set[0]->image_data_list[j].image_dpl.channel_count = 0;

			image_data_set[0]->image_data_list[j].depth_dpl.width = 0;
			image_data_set[0]->image_data_list[j].depth_dpl.height = 0;
		}
	}

	size_t cp_size = 0;
	*is_dpresult_mode = false;

	IscShutterMode shutter_mode = isc_control->isc_image_info.shutter_mode;
	bool is_raw_on = isc_control->isc_start_mode.isc_grab_start_mode.isc_get_raw_mode == IscGetModeRaw::kRawOn ? true : false;
	int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;
	bool is_play_mode = isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode == IscPlayMode::kPlayOn ? true : false;

	switch (mode) {
	case DpcDrawLib::ImageDrawMode::kUnknown:
		// mode error, display a temporary image
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kUnknown;

		image_data_set[0]->image_data_list[0].image_mono_s0.width = isc_control->isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[0].image_mono_s0.height = isc_control->isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[0].image_mono_s0.channel_count = isc_control->isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[0].image_mono_s0.width * image_data_set[0]->image_data_list[0].image_mono_s0.height;
		memset(image_data_set[0]->image_data_list[0].image_mono_s0.buffer, 64, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kMonoS0:
		// mono sensor-0 image
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kMonoS0;

		if ((shutter_mode == IscShutterMode::kDoubleShutter && is_raw_on) ||
			(shutter_mode == IscShutterMode::kDoubleShutter && is_play_mode)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}

		image_data_set[0]->image_data_list[0].image_mono_s0.width = isc_control->isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[0].image_mono_s0.height = isc_control->isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[0].image_mono_s0.channel_count = isc_control->isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[0].image_mono_s0.width * image_data_set[0]->image_data_list[0].image_mono_s0.height;
		memcpy(image_data_set[0]->image_data_list[0].image_mono_s0.buffer, isc_control->isc_image_info.frame_data[fd_index].p1.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kMonoS1:
		// mono sensor-1 image
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kMonoS1;

		image_data_set[0]->image_data_list[0].image_mono_s1.width = isc_control->isc_image_info.frame_data[fd_index].p2.width;
		image_data_set[0]->image_data_list[0].image_mono_s1.height = isc_control->isc_image_info.frame_data[fd_index].p2.height;
		image_data_set[0]->image_data_list[0].image_mono_s1.channel_count = isc_control->isc_image_info.frame_data[fd_index].p2.channel_count;
		cp_size = image_data_set[0]->image_data_list[0].image_mono_s1.width * image_data_set[0]->image_data_list[0].image_mono_s1.height;
		memcpy(image_data_set[0]->image_data_list[0].image_mono_s1.buffer, isc_control->isc_image_info.frame_data[fd_index].p2.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDepth:
		// depth data
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDepth;

		if ((shutter_mode == IscShutterMode::kDoubleShutter && is_raw_on) ||
			(shutter_mode == IscShutterMode::kDoubleShutter && is_play_mode)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}

		image_data_set[0]->image_data_list[0].depth.width = isc_control->isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth.height = isc_control->isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth.width * image_data_set[0]->image_data_list[0].depth.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth.buffer, isc_control->isc_image_info.frame_data[fd_index].depth.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kColor:
		// color image
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kColor;

		image_data_set[0]->image_data_list[0].image_color.width = isc_control->isc_image_info.frame_data[fd_index].color.width;
		image_data_set[0]->image_data_list[0].image_color.height = isc_control->isc_image_info.frame_data[fd_index].color.height;
		image_data_set[0]->image_data_list[0].image_color.channel_count = isc_control->isc_image_info.frame_data[fd_index].color.channel_count;
		cp_size = image_data_set[0]->image_data_list[0].image_color.width * image_data_set[0]->image_data_list[0].image_color.height * image_data_set[0]->image_data_list[0].image_color.channel_count;
		memcpy(image_data_set[0]->image_data_list[0].image_color.buffer, isc_control->isc_image_info.frame_data[fd_index].color.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kMonoS0MonoS1:
		// mono sensor-0 image,mono sensor-1 image
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kMonoS0MonoS1;

		image_data_set[0]->image_data_list[0].image_mono_s0.width = isc_control->isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[0].image_mono_s0.height = isc_control->isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[0].image_mono_s0.channel_count = isc_control->isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[0].image_mono_s0.width * image_data_set[0]->image_data_list[0].image_mono_s0.height;
		memcpy(image_data_set[0]->image_data_list[0].image_mono_s0.buffer, isc_control->isc_image_info.frame_data[fd_index].p1.image, cp_size);

		image_data_set[0]->image_data_list[1].image_mono_s1.width = isc_control->isc_image_info.frame_data[fd_index].p2.width;
		image_data_set[0]->image_data_list[1].image_mono_s1.height = isc_control->isc_image_info.frame_data[fd_index].p2.height;
		image_data_set[0]->image_data_list[1].image_mono_s1.channel_count = isc_control->isc_image_info.frame_data[fd_index].p2.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_mono_s1.width * image_data_set[0]->image_data_list[1].image_mono_s1.height;
		memcpy(image_data_set[0]->image_data_list[1].image_mono_s1.buffer, isc_control->isc_image_info.frame_data[fd_index].p2.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDepthMonoS0:
		// depth data,mono sensor-0 image
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDepthMonoS0;

		if ((shutter_mode == IscShutterMode::kDoubleShutter && is_raw_on) ||
			(shutter_mode == IscShutterMode::kDoubleShutter && is_play_mode)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}

		image_data_set[0]->image_data_list[0].depth.width = isc_control->isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth.height = isc_control->isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth.width * image_data_set[0]->image_data_list[0].depth.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth.buffer, isc_control->isc_image_info.frame_data[fd_index].depth.image, cp_size);

		image_data_set[0]->image_data_list[1].image_mono_s0.width = isc_control->isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[1].image_mono_s0.height = isc_control->isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[1].image_mono_s0.channel_count = isc_control->isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_mono_s0.width * image_data_set[0]->image_data_list[1].image_mono_s0.height;
		memcpy(image_data_set[0]->image_data_list[1].image_mono_s0.buffer, isc_control->isc_image_info.frame_data[fd_index].p1.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDepthColor:
		// depth data,color image
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDepthColor;
		
		if ((shutter_mode == IscShutterMode::kDoubleShutter && is_raw_on) ||
			(shutter_mode == IscShutterMode::kDoubleShutter && is_play_mode)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}

		image_data_set[0]->image_data_list[0].depth.width = isc_control->isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth.height = isc_control->isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth.width * image_data_set[0]->image_data_list[0].depth.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth.buffer, isc_control->isc_image_info.frame_data[fd_index].depth.image, cp_size);

		image_data_set[0]->image_data_list[1].image_color.width = isc_control->isc_image_info.frame_data[fd_index].color.width;
		image_data_set[0]->image_data_list[1].image_color.height = isc_control->isc_image_info.frame_data[fd_index].color.height;
		image_data_set[0]->image_data_list[1].image_color.channel_count = isc_control->isc_image_info.frame_data[fd_index].color.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_color.width * image_data_set[0]->image_data_list[1].image_color.height * image_data_set[0]->image_data_list[1].image_color.channel_count;
		memcpy(image_data_set[0]->image_data_list[1].image_color.buffer, isc_control->isc_image_info.frame_data[fd_index].color.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kOverlapedDepthMonoS0:
		// over lapped depth data,mono sensor-0 image
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kOverlapedDepthMonoS0;

		if ((shutter_mode == IscShutterMode::kDoubleShutter && is_raw_on) ||
			(shutter_mode == IscShutterMode::kDoubleShutter && is_play_mode)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}

		image_data_set[0]->image_data_list[0].depth.width = isc_control->isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth.height = isc_control->isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth.width * image_data_set[0]->image_data_list[0].depth.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth.buffer, isc_control->isc_image_info.frame_data[fd_index].depth.image, cp_size);

		image_data_set[0]->image_data_list[1].image_mono_s0.width = isc_control->isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[1].image_mono_s0.height = isc_control->isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[1].image_mono_s0.channel_count = isc_control->isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_mono_s0.width * image_data_set[0]->image_data_list[1].image_mono_s0.height;
		memcpy(image_data_set[0]->image_data_list[1].image_mono_s0.buffer, isc_control->isc_image_info.frame_data[fd_index].p1.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDplImage:
		// data processing result image

		*is_dpresult_mode = true;
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDplImage;

		if ((shutter_mode == IscShutterMode::kDoubleShutter)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}
		image_data_set[0]->image_data_list[0].image_dpl.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[0].image_dpl.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[0].image_dpl.channel_count = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[0].image_dpl.width * image_data_set[0]->image_data_list[0].image_dpl.height * image_data_set[0]->image_data_list[0].image_dpl.channel_count;
		memcpy(image_data_set[0]->image_data_list[0].image_dpl.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDplImageMonoS0:
		// data processing result image,mono sensor-0 image
		*is_dpresult_mode = true;
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDplImageMonoS0;

		if ((shutter_mode == IscShutterMode::kDoubleShutter)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}
		image_data_set[0]->image_data_list[0].image_dpl.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[0].image_dpl.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[0].image_dpl.channel_count = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[0].image_dpl.width * image_data_set[0]->image_data_list[0].image_dpl.height * image_data_set[0]->image_data_list[0].image_dpl.channel_count;
		memcpy(image_data_set[0]->image_data_list[0].image_dpl.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.image, cp_size);

		fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;
		image_data_set[0]->image_data_list[1].image_mono_s0.width = isc_control->isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[1].image_mono_s0.height = isc_control->isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[1].image_mono_s0.channel_count = isc_control->isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_mono_s0.width * image_data_set[0]->image_data_list[1].image_mono_s0.height;
		memcpy(image_data_set[0]->image_data_list[1].image_mono_s0.buffer, isc_control->isc_image_info.frame_data[fd_index].p1.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDplImageColor:
		// data processing result image,color image

		*is_dpresult_mode = true;
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDplImageColor;

		if ((shutter_mode == IscShutterMode::kDoubleShutter)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}
		image_data_set[0]->image_data_list[0].image_dpl.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[0].image_dpl.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[0].image_dpl.channel_count = isc_control->isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[0].image_dpl.width * image_data_set[0]->image_data_list[0].image_dpl.height * image_data_set[0]->image_data_list[0].image_dpl.channel_count;
		memcpy(image_data_set[0]->image_data_list[0].image_dpl.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.image, cp_size);

		image_data_set[0]->image_data_list[1].image_color.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.width;
		image_data_set[0]->image_data_list[1].image_color.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.height;
		image_data_set[0]->image_data_list[1].image_color.channel_count = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_color.width * image_data_set[0]->image_data_list[1].image_color.height * image_data_set[0]->image_data_list[1].image_color.channel_count;
		memcpy(image_data_set[0]->image_data_list[1].image_color.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDplDepth:
		// data processing result depth
		*is_dpresult_mode = true;
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDplDepth;

		if ((shutter_mode == IscShutterMode::kDoubleShutter)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}
		image_data_set[0]->image_data_list[0].depth_dpl.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth_dpl.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth_dpl.width * image_data_set[0]->image_data_list[0].depth_dpl.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth_dpl.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.image, cp_size);

		break;

	case DpcDrawLib::ImageDrawMode::kDplDepthMonoS0:
		//  data processing result depth, mono sensor-0 image
		*is_dpresult_mode = true;
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDplDepthMonoS0;

		if ((shutter_mode == IscShutterMode::kDoubleShutter)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}
		image_data_set[0]->image_data_list[0].depth_dpl.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth_dpl.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth_dpl.width * image_data_set[0]->image_data_list[0].depth_dpl.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth_dpl.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.image, cp_size);

		image_data_set[0]->image_data_list[1].image_mono_s0.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[1].image_mono_s0.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[1].image_mono_s0.channel_count = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_mono_s0.width * image_data_set[0]->image_data_list[1].image_mono_s0.height;
		memcpy(image_data_set[0]->image_data_list[1].image_mono_s0.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDplDepthColor:
		// data processing result depth, kDplDepthColor
		*is_dpresult_mode = true;
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDplDepthColor;

		if ((shutter_mode == IscShutterMode::kDoubleShutter)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}
		image_data_set[0]->image_data_list[0].depth_dpl.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth_dpl.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth_dpl.width * image_data_set[0]->image_data_list[0].depth_dpl.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth_dpl.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.image, cp_size);

		image_data_set[0]->image_data_list[1].image_color.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.width;
		image_data_set[0]->image_data_list[1].image_color.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.height;
		image_data_set[0]->image_data_list[1].image_color.channel_count = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_color.width * image_data_set[0]->image_data_list[1].image_color.height * image_data_set[0]->image_data_list[1].image_color.channel_count;
		memcpy(image_data_set[0]->image_data_list[1].image_color.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kDplDepthDepth:
		// data processing result depth,depth data
		*is_dpresult_mode = true;
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kDplDepthDepth;

		if ((shutter_mode == IscShutterMode::kDoubleShutter)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}
		image_data_set[0]->image_data_list[0].depth_dpl.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth_dpl.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth_dpl.width * image_data_set[0]->image_data_list[0].depth_dpl.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth_dpl.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.image, cp_size);

		fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

		if ((shutter_mode == IscShutterMode::kDoubleShutter && is_raw_on) ||
			(shutter_mode == IscShutterMode::kDoubleShutter && is_play_mode)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}

		image_data_set[0]->image_data_list[1].depth.width = isc_control->isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[1].depth.height = isc_control->isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[1].depth.width * image_data_set[0]->image_data_list[1].depth.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[1].depth.buffer, isc_control->isc_image_info.frame_data[fd_index].depth.image, cp_size);
		break;

	case DpcDrawLib::ImageDrawMode::kOverlapedDplDepthMonoS0:
		// over lapped data processing result depth
		*is_dpresult_mode = true;
		image_data_set[0]->valid = true;
		image_data_set[0]->mode = DpcDrawLib::ImageDrawMode::kOverlapedDplDepthMonoS0;

		if ((shutter_mode == IscShutterMode::kDoubleShutter)) {
			fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
		}
		image_data_set[0]->image_data_list[0].depth_dpl.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width;
		image_data_set[0]->image_data_list[0].depth_dpl.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height;
		cp_size = image_data_set[0]->image_data_list[0].depth_dpl.width * image_data_set[0]->image_data_list[0].depth_dpl.height * sizeof(float);
		memcpy(image_data_set[0]->image_data_list[0].depth_dpl.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.image, cp_size);

		image_data_set[0]->image_data_list[1].image_mono_s0.width = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.width;
		image_data_set[0]->image_data_list[1].image_mono_s0.height = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.height;
		image_data_set[0]->image_data_list[1].image_mono_s0.channel_count = isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.channel_count;
		cp_size = image_data_set[0]->image_data_list[1].image_mono_s0.width * image_data_set[0]->image_data_list[1].image_mono_s0.height;
		memcpy(image_data_set[0]->image_data_list[1].image_mono_s0.buffer, isc_control->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.image, cp_size);
		break;

	}

	return true;
}

/**
 * @brief Show error message
 * @param errorcode
 * @retuen {@code 0} success , {@code >0} Failure
 * @throws never
 */
void SystemFormatMessage(const DWORD errorcode)
{
	LPVOID lp_msg_buf = nullptr;

	// Flags
	/*
		request memory allocation for text
		use the error message provided by Windows
		Ignore the next argument and create an error message for the error code
		テキストのメモリ割り当てを要求する
		エラーメッセージはWindowsが用意しているものを使用
		次の引数を無視してエラーコードに対するエラーメッセージを作成する
	*/
	FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER
					| FORMAT_MESSAGE_FROM_SYSTEM
					| FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					errorcode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	
					(LPTSTR)&lp_msg_buf,						
					0,
					NULL);

	TCHAR msg[256] = {};
	_stprintf_s(msg, _T("[ERROR]%s"), (LPTSTR)lp_msg_buf);
	MessageBox(NULL, msg, _T("SystemFormatMessage()"), MB_OK | MB_ICONINFORMATION);
	LocalFree(lp_msg_buf);

	return;
}

/**
 * @brief Get free disk space
 * @param[IN] folder
 * @param[OUT] free_disk_space
 * @retuen {@code 0} success , {@code >0} Failure
 * @throws never
 */
bool GetFreeDiskSpace(const TCHAR* folder, unsigned __int64* free_disk_space)
{
	// Decompose Path
	TCHAR drive[8] = {};
	TCHAR dir[_MAX_DIR] = {};
	TCHAR file[_MAX_DIR] = {};
	TCHAR ext[8] = {};

	_tsplitpath_s(folder, drive, dir, file, ext);

	TCHAR root_path_name[_MAX_DIR] = {};
	TCHAR volume_name_buffer[_MAX_DIR + 1] = {};
	DWORD volume_name_size = _MAX_DIR + 1;
	DWORD volume_serial_number = 0;
	DWORD maximum_component_length = 0;
	DWORD file_system_flags = 0;
	TCHAR file_system_name_buffer[_MAX_DIR + 1] = {};
	DWORD file_system_name_size = _MAX_DIR + 1;

	_stprintf_s(root_path_name, _T("%s\\"), drive);

	// Error handle to application
	UINT oldErMode = SetErrorMode(SEM_FAILCRITICALERRORS);

	if (GetVolumeInformation(
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
		TCHAR msg[256] = {};
		_stprintf_s(msg, _T("[ERROR]Unable to access target drive(%s)"), root_path_name);
		MessageBox(NULL, msg, _T("GetFreeDiskSpace()"), MB_ICONERROR);

		// return
		SetErrorMode(oldErMode);
		return false;
	}

	ULARGE_INTEGER freeBytesAvailable = {}, totalNumberOfBytes = {}, totalNumberOfFreeBytes = {};
	if (GetDiskFreeSpaceEx(
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

bool CheckDiskFreeSpace(const TCHAR* target_folder, const unsigned __int64 requested_size)
{
	// free space check
	unsigned __int64 free_disk_space = 0;
	bool ret = GetFreeDiskSpace(target_folder, &free_disk_space);
	if (!ret) {
		return false;
	}

	if (free_disk_space < requested_size) {
		// ERROR
		unsigned __int64 freeSpace = free_disk_space / (unsigned __int64)1024 / (unsigned __int64)1024;

		TCHAR msg[256] = {};
		_stprintf_s(msg, _T("[ERROR]There is not enough free space in the save destination.(%I64d MB)"), freeSpace);
		MessageBox(NULL, msg, _T("CheckDiskFreeSpace()"), MB_ICONERROR);

		return false;
	}

	return true;
}

/**
 * @brief Get process memory use
 * @param[OUT] process_id Proecss ID
 * @param[OUT] peak_working_set_size_mb Peak Working Memory Use(MB)
 * @param[OUT] working_set_size_mb Working Memory Use(MB)
 * @retuen {@code 0} success , {@code >0} Failure
 * @throws never
 */
bool GetProcessMemoryUse(DWORD* process_id, SIZE_T* peak_working_set_size_mb, SIZE_T* working_set_size_mb)
{

	*process_id = 0;
	*peak_working_set_size_mb	= 0;
	*working_set_size_mb		= 0;

	PROCESS_MEMORY_COUNTERS pmc = { 0 };
	DWORD dwProcessID = GetCurrentProcessId();
	HANDLE hProcess;

	TCHAR logMsg[512] = { 0 };
	constexpr int kb = 1024;

	if ((hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessID)) != NULL) {
		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {

			*process_id = dwProcessID;
			*peak_working_set_size_mb	= pmc.PeakWorkingSetSize / kb / kb;
			*working_set_size_mb		= pmc.WorkingSetSize / kb / kb;
		}
		else {
		}
		CloseHandle(hProcess);

		return true;
	}

	return false;
}

/**
 * @brief Get memory status
 * @param[OUT] total_physical_memory_mb total MB of physical memory
 * @param[OUT] total_installed_physical_memory_mb total MB of physical memory installed
 * @retuen {@code 0} success , {@code >0} Failure
 * @throws never
 */
bool GetGlobalMemoryStatus(unsigned long long* total_physical_memory_mb, unsigned long long* total_installed_physical_memory_mb)
{
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof(statex);

	BOOL ret = GlobalMemoryStatusEx(&statex);

	if (ret == FALSE) {
		return false;
	}

	constexpr int kb = 1024;

	*total_physical_memory_mb = statex.ullTotalPhys / kb / kb;

	unsigned long long totalMemoryInKilobytes = (unsigned long long)0;
	ret = GetPhysicallyInstalledSystemMemory(&totalMemoryInKilobytes);

	if (ret == FALSE) {
		return false;
	}
	*total_installed_physical_memory_mb = totalMemoryInKilobytes / kb;
	
	return true;
}

GuiSupport::GuiSupport()
{

}

GuiSupport::~GuiSupport()
{

}
