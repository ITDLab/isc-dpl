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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"

#include "dpl_controll.h"

#include "opencv2\opencv.hpp"


DplControl::DplControl():
    start_mode_(), configuration_file_path_(), log_file_path_(), image_path_(), camera_model_(0),
    isc_image_info_(), isc_data_proc_result_data_(), camera_parameter_(), isc_dpl_configuration_(), isc_dpl_(nullptr), isc_start_mode_(),
    disp_color_map_distance_(), disp_color_map_disparity_(), max_disparity_(0.0)
{

}

DplControl::~DplControl()
{

}

bool DplControl::Initialize(const wchar_t* module_path, const int camera_model)
{
    printf("[INFO]Start library open processing\n");    

    // fix path
    swprintf_s(configuration_file_path_, L"%s", module_path);
    swprintf_s(log_file_path_, L"c:\\temp");
    swprintf_s(image_path_, L"c:\\temp");

	// open library
	isc_dpl_ = new ns_isc_dpl::IscDpl;

	swprintf_s(isc_dpl_configuration_.configuration_file_path, L"%s", configuration_file_path_);
	swprintf_s(isc_dpl_configuration_.log_file_path, L"%s", log_file_path_);
	isc_dpl_configuration_.log_level = 0;

	isc_dpl_configuration_.enabled_camera = true;

	camera_model_ = camera_model;
	IscCameraModel isc_camera_model = IscCameraModel::kUnknown;
	switch (camera_model_) {
	case 0:isc_camera_model = IscCameraModel::kVM; break;
	case 1:isc_camera_model = IscCameraModel::kXC; break;
	case 2:isc_camera_model = IscCameraModel::k4K; break;
	case 3:isc_camera_model = IscCameraModel::k4KA; break;
	case 4:isc_camera_model = IscCameraModel::k4KJ; break;
	}
	isc_dpl_configuration_.isc_camera_model = isc_camera_model;

	swprintf_s(isc_dpl_configuration_.save_image_path, L"%s", image_path_);
	swprintf_s(isc_dpl_configuration_.load_image_path, L"%s", image_path_);

	isc_dpl_configuration_.enabled_data_proc_module = true;

	// open camera for use it
	DPL_RESULT dpl_result = isc_dpl_->Initialize(&isc_dpl_configuration_);

	if (dpl_result == DPC_E_OK) {
		isc_dpl_->InitializeIscIamgeinfo(&isc_image_info_);
		isc_dpl_->InitializeIscDataProcResultData(&isc_data_proc_result_data_);

		isc_dpl_->DeviceGetOption(IscCameraInfo::kBaseLength, &camera_parameter_.b);
		isc_dpl_->DeviceGetOption(IscCameraInfo::kBF, &camera_parameter_.bf);
		isc_dpl_->DeviceGetOption(IscCameraInfo::kDINF, &camera_parameter_.dinf);
		camera_parameter_.setup_angle = 0;
	
        // information
        printf("[INFO]Library opened successfully\n");    
        char camera_str[128] = {};
        isc_dpl_->DeviceGetOption(IscCameraInfo::kSerialNumber, &camera_str[0], (int)sizeof(camera_str));
        printf("[INFO]Camera Serial Number:%s\n", camera_str);    

        uint64_t fpga_version = 0;
        isc_dpl_->DeviceGetOption(IscCameraInfo::kFpgaVersion, &fpga_version);
        printf("[INFO]Camera FPGA Version:0x%016I64X\n", fpga_version);    

        printf("[INFO]Camera Parameter:b(%.3f) bf(%.3f) dinf(%.3f)\n", camera_parameter_.b, camera_parameter_.bf,camera_parameter_.dinf);    

        // set shutter mode single
        isc_dpl_->DeviceSetOption(IscCameraParameter::kShutterMode, IscShutterMode::kSingleShutter);
        printf("[INFO]Set Shutter Control Mode:Single\n");

        // set auto calib off
        isc_dpl_->DeviceSetOption(IscCameraParameter::kAutoCalibration, false);
        printf("[INFO]Set Auto Calibration:Off\n");

        // set software selft calibration
        //isc_dpl_->DeviceSetOption(IscCameraParameter::kSelfCalibration, true);
        //printf("[INFO]Set Self-Calibration:on\n");

    }
	else {
        printf("[ERROR]Failed to open library\n");
        return false;    
 	}
    
    // display settings
    double min_distance = 0.5;
    double max_distance = 20.0;
    double max_disparity = 255.0;
    switch (isc_camera_model) {
    case IscCameraModel::kVM: max_disparity = 128.0; break;
    case IscCameraModel::kXC: max_disparity = 255.0; break;
    }
    disp_color_map_distance_.min_value = min_distance;
    disp_color_map_distance_.max_value = max_distance;
    disp_color_map_distance_.color_map_size = 0;
    disp_color_map_distance_.color_map_step = 0.01;
    disp_color_map_distance_.color_map_size = (int)(max_distance / disp_color_map_distance_.color_map_step) + 1;
    disp_color_map_distance_.color_map = new int[disp_color_map_distance_.color_map_size + sizeof(int)];
    memset(disp_color_map_distance_.color_map, 0, disp_color_map_distance_.color_map_size + sizeof(int));
    BuildColorHeatMap(&disp_color_map_distance_);

    disp_color_map_disparity_.min_value = 0;
    disp_color_map_disparity_.max_value = max_disparity;
    disp_color_map_disparity_.color_map_size = 0;
    disp_color_map_disparity_.color_map_step = 0.25;
    disp_color_map_disparity_.color_map_size = (int)(max_disparity / disp_color_map_disparity_.color_map_step) + 1;
    disp_color_map_disparity_.color_map = new int[disp_color_map_disparity_.color_map_size + sizeof(int)];
    memset(disp_color_map_disparity_.color_map, 0, disp_color_map_disparity_.color_map_size + sizeof(int));
    BuildColorHeatMapForDisparity(&disp_color_map_disparity_);
    
    printf("[INFO]Finished opening the library\n");    

    return true;
}

void DplControl::Terminate()
{
    // ended
    delete[] disp_color_map_distance_.color_map;
    disp_color_map_distance_.color_map = nullptr;

    delete[] disp_color_map_disparity_.color_map;
    disp_color_map_disparity_.color_map = nullptr;

	if (isc_dpl_ != nullptr) {
		isc_dpl_->ReleaeIscDataProcResultData(&isc_data_proc_result_data_);
		isc_dpl_->ReleaeIscIamgeinfo(&isc_image_info_);

		isc_dpl_->Terminate();
		delete isc_dpl_;
		isc_dpl_ = nullptr;
	}

    return;
}

bool DplControl::InitializeBuffers(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data)
{

    int ret = isc_dpl_->InitializeIscIamgeinfo(isc_image_Info);
    if (ret != DPC_E_OK) {
        return false;
    }

    ret = isc_dpl_->InitializeIscDataProcResultData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return false;
    }

    return true;
}

bool DplControl::ReleaseBuffers(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data)
{

    int ret = isc_dpl_->ReleaeIscIamgeinfo(isc_image_Info);
    if (ret != DPC_E_OK) {
        return false;
    }

    ret = isc_dpl_->ReleaeIscDataProcResultData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return false;
    }

    return true;
}

bool DplControl::Start(StartMode& start_mode)
{
    if (isc_dpl_ == nullptr) {
        return false;
    }

    // for (Block Matcing=ON Frame Decoder=ON)
	isc_start_mode_.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kCorrect;
	isc_start_mode_.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
	if(start_mode.enabled_color) {
        isc_start_mode_.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorON;
    }
    isc_start_mode_.isc_grab_start_mode.isc_get_mode.wait_time = 100;
	isc_start_mode_.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
	if(start_mode.enabled_color) {
        isc_start_mode_.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwb;
    }
	isc_start_mode_.isc_grab_start_mode.isc_record_mode = IscRecordMode::kRecordOff;
	isc_start_mode_.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOff;
    isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.interval = 30;
	memset(isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, 0, sizeof(isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name));

    isc_start_mode_.isc_dataproc_start_mode.enabled_stereo_matching = true;
    isc_start_mode_.isc_dataproc_start_mode.enabled_frame_decoder = true;
    isc_start_mode_.isc_dataproc_start_mode.enabled_disparity_filter = true;

    DPL_RESULT dpl_result = isc_dpl_->Start(&isc_start_mode_);
    if (dpl_result == DPC_E_OK) {
        printf("[INFO]Start successfully\n");    
    }
    else {
        printf("[ERROR]Failed to Start\n");
        return false;
    }

    return true;
}

bool DplControl::Stop()
{
    if (isc_dpl_ == nullptr) {
        return false;
    }

    DPL_RESULT dpl_result = isc_dpl_->Stop();

    if (dpl_result == DPC_E_OK) {
        printf("[INFO]Stop successfully\n");    
    }
    else {
        printf("[ERROR]Failed to Stop\n");
        return false;
    }

    return true;
}

bool DplControl::GetCameraData(IscImageInfo* isc_image_Info)
{
    if (isc_dpl_ == nullptr) {
        return false;
    }
    
    DPL_RESULT dpl_result = isc_dpl_->GetCameraData(isc_image_Info);
	if (dpl_result != DPC_E_OK) {
		return false;
	}

    return true;
}

bool DplControl::GetDataProcessingData(IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_dpl_ == nullptr) {
        return false;
    }
    
    DPL_RESULT dpl_result = isc_dpl_->GetDataProcModuleData(isc_data_proc_result_data);
	if (dpl_result != DPC_E_OK) {
		return false;
	}

    return true;
}

bool DplControl::GetCameraParameter(float* b, float* bf, float* dinf, int* width, int* height)
{
    if (isc_dpl_ == nullptr) {
        return false;
    }

    *b = 0;
    *bf = 0;
    *dinf = 0;
    *width = 0;
    *height = 0;

    DPL_RESULT ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kBaseLength, b);
    if (ret != DPC_E_OK) {
        return false;
    }

    ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kBF, bf);
    if (ret != DPC_E_OK) {
        return false;
    }

    ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kDINF, dinf);
    if (ret != DPC_E_OK) {
        return false;
    }

    ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kWidthMax, width);
    if (ret != DPC_E_OK) {
        return false;
    }

    ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kHeightMax, height);
    if (ret != DPC_E_OK) {
        return false;
    }

    return true;
}

bool DplControl::ConvertDisparityToImage(double b, const double angle, const double bf, const double dinf, 
                                            const int width, const int height, float* depth, unsigned char* bgra_image)
{
    
    constexpr bool is_color_by_distance = true;
    constexpr bool is_draw_outside_bounds = true;
    const double min_length = disp_color_map_distance_.min_value;
    const double max_length = disp_color_map_distance_.max_value;
    DispColorMap* disp_color_map = &disp_color_map_distance_;

    bool ret = MakeDepthColorImage( is_color_by_distance, is_draw_outside_bounds, min_length, max_length,
                                    disp_color_map, b, angle, bf, dinf,
                                    width, height, depth, bgra_image);

    return ret;
}

int DplControl::ColorScaleBCGYR(const double min_value, const double max_value, const double in_value, int* bo, int* go, int* ro)
{
    int ret = 0;
    int r = 0, g = 0, b = 0;

    // 0.0～1.0 の範囲の値をサーモグラフィみたいな色にする
    // 0.0                    1.0
    // 青    水    緑    黄    赤
    // 最小値以下 = 青
    // 最大値以上 = 赤

    if (in_value <= min_value) {
        // red
        r = 255;
        g = 0;
        b = 0;
    }
    else if (in_value >= max_value) {
        // blue
        r = 0;
        g = 0;
        b = 255;
    }
    else {
        double temp_in_value = in_value - min_value;
        double range = max_value - min_value;

        double  value = 1.0 - (temp_in_value / range);
        double  tmp_val = cos(4 * 3.141592653589793 * value);
        int     col_val = (int)((-tmp_val / 2 + 0.5) * 255);

        if (value >= (4.0 / 4.0)) { r = 255;     g = 0;       b = 0; }		        // 赤
        else if (value >= (3.0 / 4.0)) { r = 255;     g = col_val; b = 0; }		    // 黄～赤
        else if (value >= (2.0 / 4.0)) { r = col_val; g = 255;     b = 0; }		    // 緑～黄
        else if (value >= (1.0 / 4.0)) { r = 0;       g = 255;     b = col_val; }	// 水～緑
        else if (value >= (0.0 / 4.0)) { r = 0;       g = col_val; b = 255; }		// 青～水
        else { r = 0;       g = 0;       b = 255; }		// 青
    }

    *bo = b;
    *go = g;
    *ro = r;

    return ret;
}

int DplControl::BuildColorHeatMap(DispColorMap* disp_color_map)
{
    const double min_value = disp_color_map->min_value;
    const double max_value = disp_color_map->max_value;
    const double step = disp_color_map->color_map_step;
    int* color_map = disp_color_map->color_map;

    int start = 0;
    int end = (int)(max_value / step);
    double length = 0;

    int* p_color_map = color_map;

    for (int i = start; i <= end; i++) {

        int ro = 0, go = 0, bo = 0;
        ColorScaleBCGYR(min_value, max_value, length, &bo, &go, &ro);

        *p_color_map = (0xff000000) | (ro << 16) | (go << 8) | (bo);
        p_color_map++;

        length += step;
    }

    return 0;
}

int DplControl::BuildColorHeatMapForDisparity(DispColorMap* disp_color_map)
{
    const double min_value = disp_color_map->min_value;
    const double max_value = disp_color_map->max_value;
    const double step = disp_color_map->color_map_step;
    int* color_map = disp_color_map->color_map;

    int start = 0;
    int end = (int)(max_value / step);
    double length = 0;

    int* p_color_map = color_map;

    double* gamma_lut = new double[end + 1];
    memset(gamma_lut, 0, sizeof(double) * (end + 1));

    double gamma = 0.7;	// fix it, good for 4020
    for (int i = 0; i <= end; i++) {
        gamma_lut[i] = (int)(pow((double)i / 255.0, 1.0 / gamma) * 255.0);
    }

    for (int i = start; i <= end; i++) {

        int ro = 0, go = 0, bo = 0;
        double value = (double)(gamma_lut[(int)length]);

        ColorScaleBCGYR(min_value, max_value, value, &bo, &go, &ro);

        *p_color_map = (0xff000000) | (ro << 16) | (go << 8) | (bo);
        p_color_map++;

        length += step;
    }

    delete[] gamma_lut;

    return 0;
}

bool DplControl::MakeDepthColorImage(   const bool is_color_by_distance, const bool is_draw_outside_bounds, const double min_length_i, const double max_length_i,
                                        DispColorMap* disp_color_map, double b_i, const double angle_i, const double bf_i, const double dinf_i,
                                        const int width, const int height, float* depth, unsigned char* bgra_image)
{
    if (disp_color_map == nullptr) {
        return false;
    }

    if (depth == nullptr) {
        return false;
    }

    if (bgra_image == nullptr) {
        return false;
    }

    constexpr double pi = 3.1415926535;
    const double bf = bf_i;
    const double b = b_i;
    const double dinf = dinf_i;
    const double rad = angle_i * pi / 180.0;
    const double color_map_step_mag = 1.0 / disp_color_map->color_map_step;

    if (is_color_by_distance) {
        // 距離変換

        for (int i = 0; i < height; i++) {
            float* src = depth + (i * width);
            unsigned char* dst = bgra_image + (i * width * 4);

            for (int j = 0; j < width; j++) {
                int r = 0, g = 0, b = 0;
                if (*src <= dinf) {
                    r = 0;
                    g = 0;
                    b = 0;
                }
                else {
                    double d = (*src - dinf);
                    double za = max_length_i;
                    if (d > 0) {
#if 0
                        double yh = (b * (i - (height / 2))) / d;
                        double z = bf / d;
                        za = -1 * yh * sin(rad) + z * cos(rad);
#else
                        za = bf / d;
#endif
                    }

                    if (is_draw_outside_bounds) {
                        int map_index = (int)(za * color_map_step_mag);
                        if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
                            int map_value = disp_color_map->color_map[map_index];

                            r = (unsigned char)(map_value >> 16);
                            g = (unsigned char)(map_value >> 8);
                            b = (unsigned char)(map_value);
                        }
                        else {
                            // it's blue
                            r = 0;
                            g = 0;
                            b = 255;
                        }
                    }
                    else {
                        if (za > max_length_i) {
                            r = 0;
                            g = 0;
                            b = 0;
                        }
                        else if (za < min_length_i) {
                            r = 0;
                            g = 0;
                            b = 0;
                        }
                        else {
                            int map_index = (int)(za * color_map_step_mag);
                            if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
                                int map_value = disp_color_map->color_map[map_index];

                                r = (unsigned char)(map_value >> 16);
                                g = (unsigned char)(map_value >> 8);
                                b = (unsigned char)(map_value);
                            }
                            else {
                                // it's black
                                r = 0;
                                g = 0;
                                b = 0;
                            }
                        }
                    }
                }

                *dst++ = b;
                *dst++ = g;
                *dst++ = r;
                *dst++ = 255;

                src++;
            }
        }
    }
    else {
        // 視差
        const double max_value = max_disparity_;
        const double dinf = dinf_i;

        for (int i = 0; i < height; i++) {
            float* src = depth + (i * width);
            unsigned char* dst = bgra_image + (i * width * 4);

            for (int j = 0; j < width; j++) {
                int r = 0, g = 0, b = 0;
                if (*src <= dinf) {
                    r = 0;
                    g = 0;
                    b = 0;
                }
                else {
                    double d = MAX(0, (max_value - *src - dinf));

                    int map_index = (int)(d * color_map_step_mag);
                    if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
                        int map_value = disp_color_map->color_map[map_index];

                        r = (unsigned char)(map_value >> 16);
                        g = (unsigned char)(map_value >> 8);
                        b = (unsigned char)(map_value);
                    }
                    else {
                        // it's black
                        r = 0;
                        g = 0;
                        b = 0;
                    }
                }

                *dst++ = b;
                *dst++ = g;
                *dst++ = r;
                *dst++ = 255;

                src++;
            }
        }
    }

    return true;
}
