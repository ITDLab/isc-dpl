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
 * @file isc_stereomatching_interface.cpp
 * @brief interface class for StereoMatching module
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 *
 * @details This class provides a interface for Stereo Matching.
 * @note
 */
#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <shlwapi.h>
#include <imagehlp.h>
#include <Shlobj.h>

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"
#include "isc_dataprocessing_def.h"

#include "isc_stereomatching_interface.h"

#include "StereoMatching.h"

#include "opencv2\opencv.hpp"

#pragma comment (lib, "shlwapi")
#pragma comment (lib, "Shell32")
#pragma comment (lib, "imagehlp")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world480d")
#else
#pragma comment (lib,"opencv_world480")
#endif


/**
 * constructor
 *
 */
IscStereoMatchingInterface::IscStereoMatchingInterface():
    parameter_update_request_(false),
    parameter_file_name_(),
    isc_data_proc_module_configuration_(),
    stereo_matching_parameters_(),
    work_buffers_()
{

    // default
    stereo_matching_parameters_.system_parameter.enabled_opencl_for_avedisp = 0;

    // defult for XC
    stereo_matching_parameters_.matching_parameter.imghgt = 0;
    stereo_matching_parameters_.matching_parameter.imgwdt = 0;
    stereo_matching_parameters_.matching_parameter.depth = 256;  // VM:112
    stereo_matching_parameters_.matching_parameter.blkhgt = 3;   // VM:2
    stereo_matching_parameters_.matching_parameter.blkwdt = 3;   // VM:2
    stereo_matching_parameters_.matching_parameter.mtchgt = 7;   // VM:6
    stereo_matching_parameters_.matching_parameter.mtcwdt = 7;   // VM:6
    stereo_matching_parameters_.matching_parameter.blkofsx = 2;  // VM:2
    stereo_matching_parameters_.matching_parameter.blkofsy = 2;  // VM:2
    stereo_matching_parameters_.matching_parameter.crstthr = 50; // VM:45
	stereo_matching_parameters_.matching_parameter.grdcrct = 0;	 // VM:0
    stereo_matching_parameters_.matching_parameter.rmvdup = 0;	 // VM:0
    stereo_matching_parameters_.matching_parameter.minbrtrt = 70;	 // VM:70

    stereo_matching_parameters_.extension_matching_parameter.extmtc = 1;        
    stereo_matching_parameters_.extension_matching_parameter.extlim = 10;   // VM:10
    stereo_matching_parameters_.extension_matching_parameter.extcnf = 20;   // VM:20

    stereo_matching_parameters_.back_matching_parameter.enb = 1;
    stereo_matching_parameters_.back_matching_parameter.bkevlwdt = 1;    // VM:1
    stereo_matching_parameters_.back_matching_parameter.bkevlrng = 3;    // VM:3
    stereo_matching_parameters_.back_matching_parameter.bkvldrt = 20;    // VM:20
    stereo_matching_parameters_.back_matching_parameter.bkzrrt = 60;     // VM:80

    stereo_matching_parameters_.neighbor_matching_parameter.enb = 1;
    stereo_matching_parameters_.neighbor_matching_parameter.neibrot = 0.08; // VM:0.08
    stereo_matching_parameters_.neighbor_matching_parameter.neibvsft = 0.1; // VM:0.1
    stereo_matching_parameters_.neighbor_matching_parameter.neibhsft = 0.5; // VM:0.5
    stereo_matching_parameters_.neighbor_matching_parameter.neibrng = 8;    // VM:8

    stereo_matching_parameters_.edge_mask_filter_parameter.enabled = 1;
    stereo_matching_parameters_.edge_mask_filter_parameter.edge_filter_method = 1;
    stereo_matching_parameters_.edge_mask_filter_parameter.sobel_x_order = 1;
    stereo_matching_parameters_.edge_mask_filter_parameter.sobel_y_order = 0;
    stereo_matching_parameters_.edge_mask_filter_parameter.sobel_ksize = 3;
    stereo_matching_parameters_.edge_mask_filter_parameter.sobel_threshold = 8;
    stereo_matching_parameters_.edge_mask_filter_parameter.canny_threshold_1 = 30;
    stereo_matching_parameters_.edge_mask_filter_parameter.canny_threshold_2 = 70;
    stereo_matching_parameters_.edge_mask_filter_parameter.canny_aperture_size = 3;
    stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_ksize = 1;
    stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_scale = 1;
    stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_threshold = 8;

}	

/**
 * destructor
 *
 */
IscStereoMatchingInterface::~IscStereoMatchingInterface()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] isc_data_proc_module_configuration 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::Initialize(IscDataProcModuleConfiguration* isc_data_proc_module_configuration)
{
    swprintf_s(isc_data_proc_module_configuration_.configuration_file_path, L"%s", isc_data_proc_module_configuration->configuration_file_path);
    swprintf_s(isc_data_proc_module_configuration_.log_file_path, L"%s", isc_data_proc_module_configuration->log_file_path);

    isc_data_proc_module_configuration_.log_level = isc_data_proc_module_configuration->log_level;
    isc_data_proc_module_configuration_.isc_camera_model = isc_data_proc_module_configuration->isc_camera_model;
    isc_data_proc_module_configuration_.max_image_width = isc_data_proc_module_configuration->max_image_width;
    isc_data_proc_module_configuration_.max_image_height = isc_data_proc_module_configuration->max_image_height;

    switch (isc_data_proc_module_configuration_.isc_camera_model) {
    case IscCameraModel::kVM:
        swprintf_s(parameter_file_name_, L"%s\\StereoMatcingParameter_VM.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::kXC:
        swprintf_s(parameter_file_name_, L"%s\\StereoMatcingParameter_XC.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4K:
        swprintf_s(parameter_file_name_, L"%s\\StereoMatcingParameter_4K.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4KA:
        swprintf_s(parameter_file_name_, L"%s\\StereoMatcingParameter_4KA.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4KJ:
        swprintf_s(parameter_file_name_, L"%s\\StereoMatcingParameter_4KJ.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    default:
        swprintf_s(parameter_file_name_, L"%s\\StereoMatcingParameter.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;
    }

    int ret = LoadParameterFromFile(parameter_file_name_, &stereo_matching_parameters_);
    if (ret != DPC_E_OK) {
        if (ret == DPCPROCESS_E_FILE_NOT_FOUND) {
            // Since the file does not exist, it will be created by default and continue
            SaveParameterToFile(parameter_file_name_, &stereo_matching_parameters_);
        }
        else {
            return ret;
        }
    }

    stereo_matching_parameters_.matching_parameter.imghgt = isc_data_proc_module_configuration_.max_image_height;
    stereo_matching_parameters_.matching_parameter.imgwdt = isc_data_proc_module_configuration_.max_image_width;

    ret = SetParameterToStereoMatchingModule(&stereo_matching_parameters_);
    if (ret != DPC_E_OK) {
        return ret;
    }
 
    // get work
    size_t image_size = isc_data_proc_module_configuration_.max_image_width * isc_data_proc_module_configuration_.max_image_height;
    size_t depth_size = isc_data_proc_module_configuration_.max_image_width * isc_data_proc_module_configuration_.max_image_height;

    for (int i = 0; i < 2; i++) {
        work_buffers_.buff_image[i].width = 0;
        work_buffers_.buff_image[i].height = 0;
        work_buffers_.buff_image[i].channel_count = 0;
        work_buffers_.buff_image[i].image = new unsigned char[image_size];
        memset(work_buffers_.buff_image[i].image, 0, image_size);
    }

    for (int i = 0; i < 4; i++) {
        work_buffers_.buff_depth[i].width = 0;
        work_buffers_.buff_depth[i].height = 0;
        work_buffers_.buff_depth[i].image = new float[depth_size];
    }

    // initialize FrameDecoder
    StereoMatching::initialize(isc_data_proc_module_configuration_.max_image_height, isc_data_proc_module_configuration_.max_image_width);
    StereoMatching::createMatchingThread();

    // Check if OpenCL is available. Enable it if it is available.
    if (stereo_matching_parameters_.system_parameter.enabled_opencl_for_avedisp && cv::ocl::haveOpenCL()) {
        // it can use openCL
        //cv::String build_info_str =  cv::getBuildInformation();
        //OutputDebugStringA(build_info_str.c_str());
        StereoMatching::setUseOpenCLForMatching(1);
    }
    else {
        StereoMatching::setUseOpenCLForMatching(0);
    }

    return DPC_E_OK;
}

/**
 * パラメータをファイルから読み込みます.
 *
 * @param[in] file_name パラメータファイル名です
 * @param[out] stereo_matching_parameters　読み込んだパラメータです
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::LoadParameterFromFile(const wchar_t* file_name, StereoMatchingParameters* stereo_matching_parameters)
{
    bool is_exists = PathFileExists(file_name) == TRUE ? true : false;
    if (!is_exists) {
        return DPCPROCESS_E_FILE_NOT_FOUND;
    }

    wchar_t returned_string[1024] = {};

    // SystemParameter
    GetPrivateProfileString(L"SYSTEM", L"enabled_opencl_for_avedisp", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    int temp_value = _wtoi(returned_string);
    stereo_matching_parameters->system_parameter.enabled_opencl_for_avedisp = temp_value == 1 ? true : false;

    // MatchingParameter 
    //GetPrivateProfileString(L"MATCHING", L"imghgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    //stereo_matching_parameters->matching_parameter.imghgt = _wtoi(returned_string);

    //GetPrivateProfileString(L"MATCHING", L"imgwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    //stereo_matching_parameters->matching_parameter.imgwdt = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"depth", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.depth = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"blkhgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.blkhgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"blkwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.blkwdt = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"mtchgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.mtchgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"mtcwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.mtcwdt = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"blkofsx", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.blkofsx = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"blkofsy", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.blkofsy = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"crstthr", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.crstthr = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"grdcrct", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.grdcrct = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"rmvdup", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.rmvdup = _wtoi(returned_string);

    GetPrivateProfileString(L"MATCHING", L"minbrtrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->matching_parameter.minbrtrt = _wtoi(returned_string);

    // ExtensionMatchingParameter
    GetPrivateProfileString(L"EXT_MATCHING", L"extmtc", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->extension_matching_parameter.extmtc = _wtoi(returned_string);

    GetPrivateProfileString(L"EXT_MATCHING", L"extlim", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->extension_matching_parameter.extlim = _wtoi(returned_string);

    GetPrivateProfileString(L"EXT_MATCHING", L"extcnf", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->extension_matching_parameter.extcnf = _wtoi(returned_string);

    // BackMatchingParameter
    GetPrivateProfileString(L"BACKMATCHING", L"enb", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->back_matching_parameter.enb = _wtoi(returned_string);

    GetPrivateProfileString(L"BACKMATCHING", L"bkevlwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->back_matching_parameter.bkevlwdt = _wtoi(returned_string);

    GetPrivateProfileString(L"BACKMATCHING", L"bkevlrng", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->back_matching_parameter.bkevlrng = _wtoi(returned_string);

    GetPrivateProfileString(L"BACKMATCHING", L"bkvldrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->back_matching_parameter.bkvldrt = _wtoi(returned_string);

    GetPrivateProfileString(L"BACKMATCHING", L"bkzrrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->back_matching_parameter.bkzrrt = _wtoi(returned_string);

    // NeighborMatchingParameter
    GetPrivateProfileString(L"NEIGHBOR_MATCHING", L"enb", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->neighbor_matching_parameter.enb = _wtoi(returned_string);

    GetPrivateProfileString(L"NEIGHBOR_MATCHING", L"neibrot", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->neighbor_matching_parameter.neibrot = _wtof(returned_string);

    GetPrivateProfileString(L"NEIGHBOR_MATCHING", L"neibvsft", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->neighbor_matching_parameter.neibvsft = _wtof(returned_string);

    GetPrivateProfileString(L"NEIGHBOR_MATCHING", L"neibhsft", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->neighbor_matching_parameter.neibhsft = _wtof(returned_string);

    GetPrivateProfileString(L"NEIGHBOR_MATCHING", L"neibrng", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->neighbor_matching_parameter.neibrng = _wtof(returned_string);

    // EdgeMaskFilterParameter
    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"enabled", L"1", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.enabled = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"edge_filter_method", L"1", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.edge_filter_method = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"sobel_x_order", L"1", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.sobel_x_order = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"sobel_y_order", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.sobel_y_order = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"sobel_ksize", L"3", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.sobel_ksize = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"sobel_threshold", L"8", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.sobel_threshold = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"canny_threshold_1", L"30", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.canny_threshold_1 = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"canny_threshold_2", L"70", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.canny_threshold_2 = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"canny_aperture_size", L"3", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.canny_aperture_size = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"laplacian_ksize", L"1", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.laplacian_ksize = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"laplacian_scale", L"1", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.laplacian_scale = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_MASK_FILTER", L"laplacian_threshold", L"8", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    stereo_matching_parameters->edge_mask_filter_parameter.laplacian_threshold = _wtoi(returned_string);

    {
        // edge_mask_filter_parameter_　を一時的な保存へコピー
        edge_mask_filter_temporary_parameter_.enabled = stereo_matching_parameters->edge_mask_filter_parameter.enabled;
        edge_mask_filter_temporary_parameter_.edge_filter_method = stereo_matching_parameters->edge_mask_filter_parameter.edge_filter_method;
        edge_mask_filter_temporary_parameter_.sobel_x_order = stereo_matching_parameters->edge_mask_filter_parameter.sobel_x_order;
        edge_mask_filter_temporary_parameter_.sobel_y_order = stereo_matching_parameters->edge_mask_filter_parameter.sobel_y_order;
        edge_mask_filter_temporary_parameter_.sobel_ksize = stereo_matching_parameters->edge_mask_filter_parameter.sobel_ksize;
        edge_mask_filter_temporary_parameter_.sobel_threshold = stereo_matching_parameters->edge_mask_filter_parameter.sobel_threshold;
        edge_mask_filter_temporary_parameter_.canny_threshold_1 = stereo_matching_parameters->edge_mask_filter_parameter.canny_threshold_1;
        edge_mask_filter_temporary_parameter_.canny_threshold_2 = stereo_matching_parameters->edge_mask_filter_parameter.canny_threshold_2;
        edge_mask_filter_temporary_parameter_.canny_aperture_size = stereo_matching_parameters->edge_mask_filter_parameter.canny_aperture_size;
        edge_mask_filter_temporary_parameter_.laplacian_ksize = stereo_matching_parameters->edge_mask_filter_parameter.laplacian_ksize;
        edge_mask_filter_temporary_parameter_.laplacian_scale = stereo_matching_parameters->edge_mask_filter_parameter.laplacian_scale;
        edge_mask_filter_temporary_parameter_.laplacian_threshold = stereo_matching_parameters->edge_mask_filter_parameter.laplacian_threshold;
    }

    return DPC_E_OK;
}

/**
 * パラメータをファイルへ書き込みます.
 *
 * @param[in] file_name パラメータファイル名です
 * @param[in] stereo_matching_parameters　書き込むパラメータです
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::SaveParameterToFile(const wchar_t* file_name, const StereoMatchingParameters* stereo_matching_parameters)
{
    wchar_t string[1024] = {};

    // SystemParameter
    swprintf_s(string, L"%d", (int)stereo_matching_parameters->system_parameter.enabled_opencl_for_avedisp);
    WritePrivateProfileString(L"SYSTEM", L"enabled_opencl_for_avedisp", string, file_name);

    // MatchingParameter 
    //swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.imghgt);
    //WritePrivateProfileString(L"MATCHING", L"imghgt", string, file_name);

    //swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.imgwdt);
    //WritePrivateProfileString(L"MATCHING", L"imgwdt", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.depth);
    WritePrivateProfileString(L"MATCHING", L"depth", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.blkhgt);
    WritePrivateProfileString(L"MATCHING", L"blkhgt", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.blkwdt);
    WritePrivateProfileString(L"MATCHING", L"blkwdt", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.mtchgt);
    WritePrivateProfileString(L"MATCHING", L"mtchgt", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.mtcwdt);
    WritePrivateProfileString(L"MATCHING", L"mtcwdt", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.blkofsx);
    WritePrivateProfileString(L"MATCHING", L"blkofsx", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.blkofsy);
    WritePrivateProfileString(L"MATCHING", L"blkofsy", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.crstthr);
    WritePrivateProfileString(L"MATCHING", L"crstthr", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.grdcrct);
    WritePrivateProfileString(L"MATCHING", L"grdcrct", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.rmvdup);
    WritePrivateProfileString(L"MATCHING", L"rmvdup", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->matching_parameter.minbrtrt);
    WritePrivateProfileString(L"MATCHING", L"minbrtrt", string, file_name);

    // ExtensionMatchingParameter
    swprintf_s(string, L"%d", (int)stereo_matching_parameters->extension_matching_parameter.extmtc);
    WritePrivateProfileString(L"EXT_MATCHING", L"extmtc", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->extension_matching_parameter.extlim);
    WritePrivateProfileString(L"EXT_MATCHING", L"extlim", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->extension_matching_parameter.extcnf);
    WritePrivateProfileString(L"EXT_MATCHING", L"extcnf", string, file_name);

    // BackMatchingParameter
    swprintf_s(string, L"%d", (int)stereo_matching_parameters->back_matching_parameter.enb);
    WritePrivateProfileString(L"BACKMATCHING", L"enb", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->back_matching_parameter.bkevlwdt);
    WritePrivateProfileString(L"BACKMATCHING", L"bkevlwdt", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->back_matching_parameter.bkevlrng);
    WritePrivateProfileString(L"BACKMATCHING", L"bkevlrng", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->back_matching_parameter.bkvldrt);
    WritePrivateProfileString(L"BACKMATCHING", L"bkvldrt", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->back_matching_parameter.bkzrrt);
    WritePrivateProfileString(L"BACKMATCHING", L"bkzrrt", string, file_name);

    // NeighborMatchingParameter
    swprintf_s(string, L"%d", (int)stereo_matching_parameters->neighbor_matching_parameter.enb);
    WritePrivateProfileString(L"NEIGHBOR_MATCHING", L"enb", string, file_name);

    swprintf_s(string, L"%.3f", stereo_matching_parameters->neighbor_matching_parameter.neibrot);
    WritePrivateProfileString(L"NEIGHBOR_MATCHING", L"neibrot", string, file_name);

    swprintf_s(string, L"%.3f", stereo_matching_parameters->neighbor_matching_parameter.neibvsft);
    WritePrivateProfileString(L"NEIGHBOR_MATCHING", L"neibvsft", string, file_name);

    swprintf_s(string, L"%.3f", stereo_matching_parameters->neighbor_matching_parameter.neibhsft);
    WritePrivateProfileString(L"NEIGHBOR_MATCHING", L"neibhsft", string, file_name);

    swprintf_s(string, L"%.3f", stereo_matching_parameters->neighbor_matching_parameter.neibrng);
    WritePrivateProfileString(L"NEIGHBOR_MATCHING", L"neibrng", string, file_name);

    // EdgeMaskFilterParameter
    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.enabled);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"enabled", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.edge_filter_method);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"edge_filter_method", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.sobel_x_order);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"sobel_x_order", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.sobel_y_order);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"sobel_y_order", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.sobel_ksize);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"sobel_ksize", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.sobel_threshold);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"sobel_threshold", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.canny_threshold_1);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"canny_threshold_1", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.canny_threshold_2);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"canny_threshold_2", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.canny_aperture_size);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"canny_aperture_size", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.laplacian_ksize);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"laplacian_ksize", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.laplacian_scale);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"laplacian_scale", string, file_name);

    swprintf_s(string, L"%d", (int)stereo_matching_parameters->edge_mask_filter_parameter.laplacian_threshold);
    WritePrivateProfileString(L"EDGE_MASK_FILTER", L"laplacian_threshold", string, file_name);

    return DPC_E_OK;
}

/**
 * パラメータをStereoMatchingの実装へ設定します.
 *
 * @param[in] stereo_matching_parameters 設定するパラメータファイルです
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::SetParameterToStereoMatchingModule(const StereoMatchingParameters* stereo_matching_parameters)
{
    StereoMatching::setUseOpenCLForMatching(stereo_matching_parameters->system_parameter.enabled_opencl_for_avedisp);

    StereoMatching::setMatchingParameter(
        stereo_matching_parameters->matching_parameter.imghgt,
        stereo_matching_parameters->matching_parameter.imgwdt,
        stereo_matching_parameters->matching_parameter.depth,
        stereo_matching_parameters->matching_parameter.blkhgt,
        stereo_matching_parameters->matching_parameter.blkwdt,
        stereo_matching_parameters->matching_parameter.mtchgt,
        stereo_matching_parameters->matching_parameter.mtcwdt,
        stereo_matching_parameters->matching_parameter.blkofsx,
        stereo_matching_parameters->matching_parameter.blkofsy,
        stereo_matching_parameters->matching_parameter.crstthr,
        stereo_matching_parameters->matching_parameter.grdcrct,
        stereo_matching_parameters->matching_parameter.rmvdup,
        stereo_matching_parameters->matching_parameter.minbrtrt
    );

    StereoMatching::setExtensionMatchingParameter(
        stereo_matching_parameters->extension_matching_parameter.extmtc,
        stereo_matching_parameters->extension_matching_parameter.extlim,
        stereo_matching_parameters->extension_matching_parameter.extcnf
    );

    StereoMatching::setBackMatchingParameter(
        stereo_matching_parameters->back_matching_parameter.enb, 
        stereo_matching_parameters->back_matching_parameter.bkevlwdt, 
        stereo_matching_parameters->back_matching_parameter.bkevlrng,
        stereo_matching_parameters->back_matching_parameter.bkvldrt,
        stereo_matching_parameters->back_matching_parameter.bkzrrt
    );

    StereoMatching::setNeighborMatchingParameter(
        stereo_matching_parameters->neighbor_matching_parameter.enb,
        stereo_matching_parameters->neighbor_matching_parameter.neibrot,
        stereo_matching_parameters->neighbor_matching_parameter.neibvsft,
        stereo_matching_parameters->neighbor_matching_parameter.neibhsft,
        stereo_matching_parameters->neighbor_matching_parameter.neibrng
    );

    // Edge Mask
    stereo_matching_parameters_.edge_mask_filter_parameter.enabled = edge_mask_filter_temporary_parameter_.enabled;
    stereo_matching_parameters_.edge_mask_filter_parameter.edge_filter_method = edge_mask_filter_temporary_parameter_.edge_filter_method;
    stereo_matching_parameters_.edge_mask_filter_parameter.sobel_x_order = edge_mask_filter_temporary_parameter_.sobel_x_order;
    stereo_matching_parameters_.edge_mask_filter_parameter.sobel_y_order = edge_mask_filter_temporary_parameter_.sobel_y_order;
    stereo_matching_parameters_.edge_mask_filter_parameter.sobel_ksize = edge_mask_filter_temporary_parameter_.sobel_ksize;
    stereo_matching_parameters_.edge_mask_filter_parameter.sobel_threshold = edge_mask_filter_temporary_parameter_.sobel_threshold;
    stereo_matching_parameters_.edge_mask_filter_parameter.canny_threshold_1 = edge_mask_filter_temporary_parameter_.canny_threshold_1;
    stereo_matching_parameters_.edge_mask_filter_parameter.canny_threshold_2 = edge_mask_filter_temporary_parameter_.canny_threshold_2;
    stereo_matching_parameters_.edge_mask_filter_parameter.canny_aperture_size = edge_mask_filter_temporary_parameter_.canny_aperture_size;
    stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_ksize = edge_mask_filter_temporary_parameter_.laplacian_ksize;
    stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_scale = edge_mask_filter_temporary_parameter_.laplacian_scale;
    stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_threshold = edge_mask_filter_temporary_parameter_.laplacian_threshold;

    return DPC_E_OK;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::Terminate()
{
    StereoMatching::deleteMatchingThread();

    // release work
    for (int i = 0; i < 4; i++) {
        work_buffers_.buff_depth[i].width = 0;
        work_buffers_.buff_depth[i].height = 0;
        delete[] work_buffers_.buff_depth[i].image;
        work_buffers_.buff_depth[i].image = nullptr;
    }

    for (int i = 0; i < 2; i++) {
        work_buffers_.buff_image[i].width = 0;
        work_buffers_.buff_image[i].height = 0;
        work_buffers_.buff_image[i].channel_count = 0;
        delete[] work_buffers_.buff_image[i].image;
        work_buffers_.buff_image[i].image = nullptr;
    }

    return DPC_E_OK;

}

/**
 * ParameterSetへデータを設定します(int).
 *
 * @param[in] value　パラメータ値です
 * @param[in] name　パラメータの名称です
 * @param[in] category　パラメータの分類です
 * @param[in] description　パラメータの説明です
 * @param[out] parameter_set　パラメータの設定先です
 * @retval 0 成功
 * @retval other 失敗
 */
void IscStereoMatchingInterface::MakeParameterSet(const int value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
{
    parameter_set->value_type = 0;
    parameter_set->value_int = value;
    parameter_set->value_float = 0;
    parameter_set->value_double = 0;

    swprintf_s(parameter_set->category, L"%s", category);
    swprintf_s(parameter_set->name, L"%s", name);

    if (description != nullptr) {
        swprintf_s(parameter_set->description, L"%s", description);
    }
    else {
        swprintf_s(parameter_set->description, L"\n");
    }

    return;
}

/**
 * ParameterSetへデータを設定します(float).
 *
 * @param[in] value　パラメータ値です
 * @param[in] name　パラメータの名称です
 * @param[in] category　パラメータの分類です
 * @param[in] description　パラメータの説明です
 * @param[out] parameter_set　パラメータの設定先です
 * @retval 0 成功
 * @retval other 失敗
 */
void IscStereoMatchingInterface::MakeParameterSet(const float value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
{
    parameter_set->value_type = 1;
    parameter_set->value_int = 0;
    parameter_set->value_float = value;
    parameter_set->value_double = 0;

    swprintf_s(parameter_set->category, L"%s", category);
    swprintf_s(parameter_set->name, L"%s", name);

    if (description != nullptr) {
        swprintf_s(parameter_set->description, L"%s", description);
    }
    else {
        swprintf_s(parameter_set->description, L"\n");
    }

    return;
}

/**
 * ParameterSetへデータを設定します(double).
 *
 * @param[in] value　パラメータ値です
 * @param[in] name　パラメータの名称です
 * @param[in] category　パラメータの分類です
 * @param[in] description　パラメータの説明です
 * @param[out] parameter_set　パラメータの設定先です
 * @retval 0 成功
 * @retval other 失敗
 */
void IscStereoMatchingInterface::MakeParameterSet(const double value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
{
    parameter_set->value_type = 2;
    parameter_set->value_int = 0;
    parameter_set->value_float = 0;
    parameter_set->value_double = value;

    swprintf_s(parameter_set->category, L"%s", category);
    swprintf_s(parameter_set->name, L"%s", name);

    if (description != nullptr) {
        swprintf_s(parameter_set->description, L"%s", description);
    }
    else {
        swprintf_s(parameter_set->description, L"\n");
    }

    return;
}

/**
 * モジュールのパラメータを取得します.
 *
 * @param[out] isc_data_proc_module_parameter モジュールパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::GetParameter(IscDataProcModuleParameter* isc_data_proc_module_parameter)
{
    // *** Note the maximum number of parameter_sets ***

    isc_data_proc_module_parameter->module_index = 0;
    swprintf_s(isc_data_proc_module_parameter->module_name, L"S/W Stereo Matching\n");

    int index = 0;

    // MatchingParameter
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.depth,   L"depth",    L"Matching", L"マッチング探索幅", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.blkhgt,  L"blkhgt",   L"Matching", L"視差ブロック高",  & isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.blkwdt,  L"blkwdt",   L"Matching", L"視差ブロック幅", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.mtchgt,  L"mtchgt",   L"Matching", L"マッチングブロック高さ", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.mtcwdt,  L"mtcwdt",   L"Matching", L"マッチングブロック幅", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.blkofsx, L"blkofsx",  L"Matching", L"視差ブロック横オフセット", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.blkofsy, L"blkofsy",  L"Matching", L"視差ブロック縦オフセット", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.crstthr, L"crstthr",  L"Matching", L"コントラスト閾値", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.grdcrct, L"grdcrct",  L"Matching", L"階調補正モードステータス 0:オフ 1:オン", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.rmvdup,     L"rmvdup",      L"Matching", L"重複マッチング除去：0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.matching_parameter.minbrtrt,   L"minbrtrt",    L"Matching", L"マッチングブロック最低輝度比率(%)", &isc_data_proc_module_parameter->parameter_set[index++]);

    // ExtensionMatchingParameter
    MakeParameterSet(stereo_matching_parameters_.extension_matching_parameter.extmtc, L"extmtc", L"ExtensionMatching", L"拡張マッチング 0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.extension_matching_parameter.extlim, L"extlim", L"ExtensionMatching", L"拡張マッチング探索制限幅", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.extension_matching_parameter.extcnf, L"extcnf", L"ExtensionMatching", L"拡張マッチング信頼限界", &isc_data_proc_module_parameter->parameter_set[index++]);

    // BackMatchingParameter
    MakeParameterSet(stereo_matching_parameters_.back_matching_parameter.enb,        L"enb",         L"BackMatching", L"バックマッチング 0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.back_matching_parameter.bkevlwdt,   L"bkevlwdt",    L"BackMatching", L"バックマッチング視差評価領域幅（片側）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.back_matching_parameter.bkevlrng,   L"bkevlrng",    L"BackMatching", L"バックマッチング視差評価視差値幅", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.back_matching_parameter.bkvldrt,    L"bkvldrt",     L"BackMatching", L"バックマッチング評価視差正当率（％）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.back_matching_parameter.bkzrrt,     L"bkzrrt",      L"BackMatching", L"バックマッチング評価視差ゼロ率（％）", &isc_data_proc_module_parameter->parameter_set[index++]);

    // NeighborMatchingParameter
    MakeParameterSet(stereo_matching_parameters_.neighbor_matching_parameter.enb,       L"enb",         L"NeighborMatching", L"近傍マッチング 0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.neighbor_matching_parameter.neibrot,   L"neibrot",     L"NeighborMatching", L"近傍マッチング回転角(度)", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.neighbor_matching_parameter.neibvsft,  L"neibvsft",    L"NeighborMatching", L"近傍マッチング垂直シフト", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.neighbor_matching_parameter.neibhsft,  L"neibhsft",    L"NeighborMatching", L"近傍マッチング水平シフト", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.neighbor_matching_parameter.neibrng,   L"neibrng",     L"NeighborMatching", L"近傍マッチング視差変化範囲", &isc_data_proc_module_parameter->parameter_set[index++]);

    // EdgeMaskFilterParameter
    MakeParameterSet(stereo_matching_parameters_.edge_mask_filter_parameter.enabled,            L"enb",         L"EdgeMaskFilter", L"EdgeMask 0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.edge_mask_filter_parameter.edge_filter_method, L"method",      L"EdgeMaskFilter", L"Filetrの手法 0:無し 1:Sobel", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.edge_mask_filter_parameter.sobel_x_order,      L"x_order",     L"EdgeMaskFilter", L"xに関する微分の次数", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.edge_mask_filter_parameter.sobel_y_order,      L"y_order",     L"EdgeMaskFilter", L"yに関する微分の次数", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.edge_mask_filter_parameter.sobel_ksize,        L"ksize",       L"EdgeMaskFilter", L"拡張Sobelカーネルのサイズ", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(stereo_matching_parameters_.edge_mask_filter_parameter.sobel_threshold,    L"threshold",   L"EdgeMaskFilter", L"閾値以上の場合にエッジと見なす", &isc_data_proc_module_parameter->parameter_set[index++]);

    isc_data_proc_module_parameter->parameter_count = index;

    return DPC_E_OK;
}

/**
 * ParameterSetより値を取得します(int).
 *
 * @param[in] parameter_set　パラメータを含んだ構造体です
 * @param[out] value　パラメータです
 * @retval 0 成功
 * @retval other 失敗
 */
void IscStereoMatchingInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, int* value)
{
    if (parameter_set->value_type != 0) {
        // error
        *value = 0;
    }
    *value = parameter_set->value_int;

    return;
}

/**
 * ParameterSetより値を取得します(float).
 *
 * @param[in] parameter_set　パラメータを含んだ構造体です
 * @param[out] value　パラメータです
 * @retval 0 成功
 * @retval other 失敗
 */
void IscStereoMatchingInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, float* value)
{
    if (parameter_set->value_type != 1) {
        // error
        *value = 0.0F;
    }
    *value = parameter_set->value_float;

    return;
}

/**
 * ParameterSetより値を取得します(double).
 *
 * @param[in] parameter_set　パラメータを含んだ構造体です
 * @param[out] value　パラメータです
 * @retval 0 成功
 * @retval other 失敗
 */
void IscStereoMatchingInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, double* value)
{
    if (parameter_set->value_type != 2) {
        // error
        *value = 0.0;
    }
    *value = parameter_set->value_double;

    return;
}

/**
 * モジュールへパラメータを設定します.
 *
 * @param[in] isc_data_proc_module_parameter モジュールパラメータ
 * @param[in] is_update_file (未使用)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::SetParameter(IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file)
{
    int index = 0;

    // MatchingParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.depth);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.blkhgt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.blkwdt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.mtchgt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.mtcwdt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.blkofsx);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.blkofsy);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.crstthr);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.grdcrct);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.rmvdup);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.matching_parameter.minbrtrt);

    // ExtensionMatchingParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.extension_matching_parameter.extmtc);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.extension_matching_parameter.extlim);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.extension_matching_parameter.extcnf);

    // BackMatchingParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.back_matching_parameter.enb);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.back_matching_parameter.bkevlwdt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.back_matching_parameter.bkevlrng);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.back_matching_parameter.bkvldrt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.back_matching_parameter.bkzrrt);

    // NeighborMatchingParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.neighbor_matching_parameter.enb);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.neighbor_matching_parameter.neibrot);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.neighbor_matching_parameter.neibvsft);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.neighbor_matching_parameter.neibhsft);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &stereo_matching_parameters_.neighbor_matching_parameter.neibrng);

    // EdgeMaskFilterParameter
    // 処理中の変更を防止するために一時変数へ保存
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &edge_mask_filter_temporary_parameter_.enabled);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &edge_mask_filter_temporary_parameter_.edge_filter_method);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &edge_mask_filter_temporary_parameter_.sobel_x_order);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &edge_mask_filter_temporary_parameter_.sobel_y_order);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &edge_mask_filter_temporary_parameter_.sobel_ksize);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &edge_mask_filter_temporary_parameter_.sobel_threshold);

    parameter_update_request_ = true;

    // file
    if (is_update_file) {
        // Update the file with current parameters
        SaveParameterToFile(parameter_file_name_, &stereo_matching_parameters_);
    }

    return DPC_E_OK;
}

/**
 * モジュールのパラメータファイル名を取得します.
 *
 * @param[out] file_name ファイル名
 * @param[in] max_length file_nameバッファーの最大文字数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::GetParameterFileName(wchar_t* file_name, const int max_length)
{
    if (max_length <= wcslen(parameter_file_name_)) {
        return DPCPROCESS_E_INVALID_PARAMETER;
    }

    swprintf_s(file_name, max_length, L"%s", parameter_file_name_);

    return DPC_E_OK;
}

/**
 * モジュールのパラメータファイルをファイルからリロードします.
 *
 * @param[out] file_name ファイル名
 * @param[in] is_valid (未使用)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::ReloadParameterFromFile(const wchar_t* file_name, const bool is_valid)
{

    wchar_t parameter_file_name[_MAX_PATH] = {};
    swprintf_s(parameter_file_name, L"%s", file_name);

    int ret = LoadParameterFromFile(parameter_file_name, &stereo_matching_parameters_);
    if (ret != DPC_E_OK) {
        return ret;
    }

    ret = SetParameterToStereoMatchingModule(&stereo_matching_parameters_);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 視差画素情報を取得します
 *
 * @param[in] isc_image_Info 入力画像・データ
 * @param[out] isc_data_proc_result_data　処理結果画素情報
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::GetDisparity(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_image_Info->grab != IscGrabMode::kCorrect) {
        return DPCPROCESS_E_INVALID_MODE;
    }

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    if ((isc_image_Info->frame_data[fd_index].p1.width == 0) || (isc_image_Info->frame_data[fd_index].p1.height == 0)) {
        return DPC_E_OK;
    }

    if ((isc_image_Info->frame_data[fd_index].p2.width == 0) || (isc_image_Info->frame_data[fd_index].p2.height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToStereoMatchingModule(&stereo_matching_parameters_);
        parameter_update_request_ = false;
    }

    IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

    // (1) matching
    unsigned char* s0_image = isc_image_Info->frame_data[fd_index].p1.image;
    unsigned char* s1_image = isc_image_Info->frame_data[fd_index].p2.image;

    StereoMatching::matching(s0_image, s1_image);

    // (2) get disparity

    int width = isc_image_Info->frame_data[fd_index].p1.width;
    int height = isc_image_Info->frame_data[fd_index].p1.height;

    unsigned char* display_image = work_buffers_.buff_image[0].image;

    dst_isc_image_info->frame_data[fd_index].depth.width = width;
    dst_isc_image_info->frame_data[fd_index].depth.height = height;
    float* disparity = dst_isc_image_info->frame_data[fd_index].depth.image;
    
    StereoMatching::getDisparity(height, width, display_image, disparity);

    return DPC_E_OK;
}

/**
 * 視差画素情報を取得します(Matchingをしない関数)
 *
 * @param[in] isc_image_Info 入力画像・データ
 * @param[out] isc_data_proc_result_data　処理結果画素情報
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::GetDecodeDisparity(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_image_Info->grab != IscGrabMode::kCorrect) {
        return DPCPROCESS_E_INVALID_MODE;
    }

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    if ((isc_image_Info->frame_data[fd_index].p1.width == 0) || (isc_image_Info->frame_data[fd_index].p1.height == 0)) {
        return DPC_E_OK;
    }

    if ((isc_image_Info->frame_data[fd_index].p2.width == 0) || (isc_image_Info->frame_data[fd_index].p2.height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToStereoMatchingModule(&stereo_matching_parameters_);
        parameter_update_request_ = false;
    }

    IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

    // (1) get disparity
    int width = isc_image_Info->frame_data[fd_index].p1.width;
    int height = isc_image_Info->frame_data[fd_index].p1.height;

    unsigned char* display_image = work_buffers_.buff_image[0].image;

    dst_isc_image_info->frame_data[fd_index].depth.width = width;
    dst_isc_image_info->frame_data[fd_index].depth.height = height;
    float* disparity = dst_isc_image_info->frame_data[fd_index].depth.image;

    memset(disparity, 0, width * height * sizeof(float));

    StereoMatching::getDisparity(height, width, display_image, disparity);

    return DPC_E_OK;
}

/**
 * 視差ブロック情報を取得するを取得します
 *
 * @param[in] isc_image_Info 入力画像・データ
 * @param[out] isc_stereo_disparity_data　処理結果視差ブロック情報
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::GetBlockDisparity(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_stereo_disparity_data)
{
    if (isc_image_Info->grab != IscGrabMode::kCorrect) {
        return DPCPROCESS_E_INVALID_MODE;
    }

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    if ((isc_image_Info->frame_data[fd_index].p1.width == 0) || (isc_image_Info->frame_data[fd_index].p1.height == 0)) {
        return DPC_E_OK;
    }

    if ((isc_image_Info->frame_data[fd_index].p2.width == 0) || (isc_image_Info->frame_data[fd_index].p2.height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToStereoMatchingModule(&stereo_matching_parameters_);
        parameter_update_request_ = false;
    }

    // (1) matching
    unsigned char* s0_image = isc_image_Info->frame_data[fd_index].p1.image;
    unsigned char* s1_image = isc_image_Info->frame_data[fd_index].p2.image;

    StereoMatching::matching(s0_image, s1_image);

    // (2) get stereo disparity

    isc_stereo_disparity_data->image_width = isc_image_Info->frame_data[fd_index].p1.width;
    isc_stereo_disparity_data->image_height = isc_image_Info->frame_data[fd_index].p1.height;

    int* stereo_width = &isc_stereo_disparity_data->blkhgt;
    int* stereo_height = &isc_stereo_disparity_data->blkwdt;

    int* pmtchgt = &isc_stereo_disparity_data->mtchgt;
    int* pmtcwdt = &isc_stereo_disparity_data->mtcwdt;
    int* pblkofsx = &isc_stereo_disparity_data->dspofsx;
    int* pblkofsy = &isc_stereo_disparity_data->dspofsy;
    int* pdepth = &isc_stereo_disparity_data->depth;
    int* pshdwdt = &isc_stereo_disparity_data->shdwdt;

    float* pblkdsp = isc_stereo_disparity_data->pblkdsp;
    int* pblkval = isc_stereo_disparity_data->pblkval;
    int* pblkcrst = isc_stereo_disparity_data->pblkcrst;

    StereoMatching::getBlockDisparity(stereo_height, stereo_width, pmtchgt, pmtcwdt, pblkofsx, pblkofsy, pdepth, pshdwdt, pblkdsp, pblkval, pblkcrst);
    
    return DPC_E_OK;
}

/**
 * 視差データをEdgeでMaskします
 *
 * @param[in] isc_image_Info 入力画像・データ
 * @param[out] isc_stereo_disparity_data　処理結果視差情報
 * @retval 0 成功
 * @retval other 失敗
 */
int IscStereoMatchingInterface::GetEdgeMaskDisparity(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data)
{

    if (isc_image_Info->grab != IscGrabMode::kCorrect) {
        return DPCPROCESS_E_INVALID_MODE;
    }

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    if ((isc_image_Info->frame_data[fd_index].p1.width == 0) || (isc_image_Info->frame_data[fd_index].p1.height == 0)) {
        return DPC_E_OK;
    }

    if ((isc_image_Info->frame_data[fd_index].p2.width == 0) || (isc_image_Info->frame_data[fd_index].p2.height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToStereoMatchingModule(&stereo_matching_parameters_);

        parameter_update_request_ = false;
    }

    if (stereo_matching_parameters_.edge_mask_filter_parameter.enabled == 0) {
        return DPC_E_OK;
    }

    IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

    // (1) edge detection
    unsigned char* src_image = isc_image_Info->frame_data[fd_index].p1.image;
    int src_image_width = isc_image_Info->frame_data[fd_index].p1.width;
    int src_image_height = isc_image_Info->frame_data[fd_index].p1.height;
    cv::Mat src_image_mat(src_image_height, src_image_width, CV_8UC1, src_image);

    float* edge_image = work_buffers_.buff_depth[1].image;
    cv::Mat edge_image_mat(src_image_height, src_image_width, CV_32F, edge_image);

    unsigned char* abs_edge_image = work_buffers_.buff_image[1].image;
    cv::Mat abs_edge_image_mat(src_image_height, src_image_width, CV_8UC1, abs_edge_image);

    int edge_filter_method = stereo_matching_parameters_.edge_mask_filter_parameter.edge_filter_method;

    if (edge_filter_method == 1) {
        // sobel
        int x_order = stereo_matching_parameters_.edge_mask_filter_parameter.sobel_x_order;
        int y_order = stereo_matching_parameters_.edge_mask_filter_parameter.sobel_y_order;
        int ksize = stereo_matching_parameters_.edge_mask_filter_parameter.sobel_ksize;
        double threshold = (double)stereo_matching_parameters_.edge_mask_filter_parameter.sobel_threshold;
        
        // Sobelフィルタの処理(入力画像,出力画像,出力タイプ,x方向の微分次数,y方向の微分次数,フィルタサイズ)
        cv::Sobel(src_image_mat, edge_image_mat, CV_32F, x_order, y_order, ksize);

        // convertScaleAbs（＝スケーリング後に絶対値を計算し，結果を8ビットに変換）
        cv::convertScaleAbs(edge_image_mat, abs_edge_image_mat, 1, 0);

        // 閾値以上の場合にエッジ（＝白）と見なす(入力画像,出力画像,閾値,最大値,閾値タイプ)
        cv::threshold(abs_edge_image_mat, abs_edge_image_mat, threshold, 255, cv::THRESH_BINARY);
    }
    else if (edge_filter_method == 2) {
        // cany
        double threshold1 = (double)stereo_matching_parameters_.edge_mask_filter_parameter.canny_threshold_1;
        double threshold2 = (double)stereo_matching_parameters_.edge_mask_filter_parameter.canny_threshold_2;
        int aperture_size = stereo_matching_parameters_.edge_mask_filter_parameter.canny_aperture_size;

        cv::Canny(src_image_mat, edge_image_mat, threshold1, threshold2, aperture_size, false);

        // convertScaleAbs（＝スケーリング後に絶対値を計算し，結果を8ビットに変換）
        cv::convertScaleAbs(edge_image_mat, abs_edge_image_mat, 1, 0);
    }
    else if (edge_filter_method == 3) {
        int ksize = stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_ksize;
        double scale = stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_scale;
        double threshold = (double)stereo_matching_parameters_.edge_mask_filter_parameter.laplacian_threshold;

        // Laplacian
        cv::Laplacian(src_image_mat, edge_image_mat, CV_32F, ksize, scale);

        // convertScaleAbs（＝スケーリング後に絶対値を計算し，結果を8ビットに変換）
        cv::convertScaleAbs(edge_image_mat, abs_edge_image_mat, 1, 0);

        // 閾値以上の場合にエッジ（＝白）と見なす(入力画像,出力画像,閾値,最大値,閾値タイプ)
        cv::threshold(abs_edge_image_mat, abs_edge_image_mat, threshold, 255, cv::THRESH_BINARY);
    }
    else {
        return DPC_E_OK;
    }

    // (2)mask
    int disparity_width = dst_isc_image_info->frame_data[fd_index].depth.width;
    int disparity_height = dst_isc_image_info->frame_data[fd_index].depth.height;
    float* disparity = dst_isc_image_info->frame_data[fd_index].depth.image;
    
    size_t depth_size = disparity_width * disparity_height * sizeof(float);
    memcpy(work_buffers_.buff_depth[0].image, disparity, depth_size);

    cv::Mat src_disparity_mat(disparity_height, disparity_width, CV_32FC1, work_buffers_.buff_depth[0].image);
    cv::Mat mask_result_mat(disparity_height, disparity_width, CV_32FC1, disparity);

    for (int i = 0; i < disparity_height; i++) {

        float* src = work_buffers_.buff_depth[0].image + (i * disparity_width);
        unsigned char* mask_src = abs_edge_image + (i * disparity_width);

        float* dst = disparity + (i * disparity_width);

        for (int j = 0; j < disparity_width; j++) {
            if (*mask_src == 255) {
                if (*src > 1) {
                    *dst = *src;
                }
                else {
                    *dst = 0;
                }
            }
            else {
                *dst = 0;
            }
            src++;
            dst++;
            mask_src++;
        }
    }

    return DPC_E_OK;
}
