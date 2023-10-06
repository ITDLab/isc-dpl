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
 * @file isc_disparityfilter_interface.cpp
 * @brief provides an interface for implementing disparity filter
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a interface for Disparity Filter.
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

#include "isc_disparityfilter_interface.h"

#include "DisparityFilter.h"

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
IscDisparityFilterInterface::IscDisparityFilterInterface():
    parameter_update_request_(false), 
    parameter_file_name_(),
    isc_data_proc_module_configuration_(),
    frame_decoder_parameters_(),
    work_buffers_()
{
    // default
    frame_decoder_parameters_.system_parameter.enabled_opencl_for_avedisp = false;
    frame_decoder_parameters_.system_parameter.single_threaded_execution = false;

    // defult for XC
    frame_decoder_parameters_.disparity_limitation_parameter.limit = 0;
    frame_decoder_parameters_.disparity_limitation_parameter.lower = 0;
    frame_decoder_parameters_.disparity_limitation_parameter.upper = 255;

    frame_decoder_parameters_.averaging_parameter.enb = 1;
    frame_decoder_parameters_.averaging_parameter.blkshgt = 3;
    frame_decoder_parameters_.averaging_parameter.blkswdt = 3;
    frame_decoder_parameters_.averaging_parameter.intg = 1;
    frame_decoder_parameters_.averaging_parameter.range = 2;
    frame_decoder_parameters_.averaging_parameter.dsprt = 20;  // VM:40
    frame_decoder_parameters_.averaging_parameter.vldrt = 20;  // VM:40
    frame_decoder_parameters_.averaging_parameter.reprt = 40;
 
    frame_decoder_parameters_.averaging_block_weight_parameter.cntwgt = 1;
    frame_decoder_parameters_.averaging_block_weight_parameter.nrwgt = 1;
    frame_decoder_parameters_.averaging_block_weight_parameter.rndwgt = 1;

    frame_decoder_parameters_.complement_parameter.enb = 1;
    frame_decoder_parameters_.complement_parameter.lowlmt = 5;
    frame_decoder_parameters_.complement_parameter.slplmt = 0.1;
    frame_decoder_parameters_.complement_parameter.insrt = 1;
    frame_decoder_parameters_.complement_parameter.rndrt = 0.2;
    frame_decoder_parameters_.complement_parameter.crstlmt = 40;   // VM:45
    frame_decoder_parameters_.complement_parameter.hlfil = 1;
    frame_decoder_parameters_.complement_parameter.hlsz = 8;

    frame_decoder_parameters_.edge_complement_parameter.edgcmp = 1;
    frame_decoder_parameters_.edge_complement_parameter.minblks = 20;
    frame_decoder_parameters_.edge_complement_parameter.mincoef = 20.0;
    frame_decoder_parameters_.edge_complement_parameter.cmpwdt = 1;

    frame_decoder_parameters_.hough_transferm_parameter.edgthr1 = 50;
    frame_decoder_parameters_.hough_transferm_parameter.edgthr2 = 100;
    frame_decoder_parameters_.hough_transferm_parameter.linthr = 100;
    frame_decoder_parameters_.hough_transferm_parameter.minlen = 80;
    frame_decoder_parameters_.hough_transferm_parameter.maxgap = 5;


}

/**
 * destructor
 *
 */
IscDisparityFilterInterface::~IscDisparityFilterInterface()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] isc_data_proc_module_configuration 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDisparityFilterInterface::Initialize(IscDataProcModuleConfiguration* isc_data_proc_module_configuration)
{
	
    swprintf_s(isc_data_proc_module_configuration_.configuration_file_path, L"%s", isc_data_proc_module_configuration->configuration_file_path);
    swprintf_s(isc_data_proc_module_configuration_.log_file_path, L"%s", isc_data_proc_module_configuration->log_file_path);

    isc_data_proc_module_configuration_.log_level = isc_data_proc_module_configuration->log_level;
    isc_data_proc_module_configuration_.isc_camera_model = isc_data_proc_module_configuration->isc_camera_model;
    isc_data_proc_module_configuration_.max_image_width = isc_data_proc_module_configuration->max_image_width;
    isc_data_proc_module_configuration_.max_image_height = isc_data_proc_module_configuration->max_image_height;

    switch (isc_data_proc_module_configuration_.isc_camera_model) {
    case IscCameraModel::kVM:
        swprintf_s(parameter_file_name_, L"%s\\DisparityFilterParameter_VM.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::kXC:
        swprintf_s(parameter_file_name_, L"%s\\DisparityFilterParameter_XC.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4K:
        swprintf_s(parameter_file_name_, L"%s\\DisparityFilterParameter_4K.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4KA:
        swprintf_s(parameter_file_name_, L"%s\\DisparityFilterParameter_4KA.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4KJ:
        swprintf_s(parameter_file_name_, L"%s\\DisparityFilterParameter_4KJ.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    default:
        swprintf_s(parameter_file_name_, L"%s\\DisparityFilterParameter.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;
    }

    int ret = LoadParameterFromFile(parameter_file_name_, &frame_decoder_parameters_);
    if (ret != DPC_E_OK) {
        if (ret == DPCPROCESS_E_FILE_NOT_FOUND) {
            // Since the file does not exist, it will be created by default and continue
            SaveParameterToFile(parameter_file_name_, &frame_decoder_parameters_);
        }
        else {
            return ret;
        }
    }

    ret = SetParameterToFrameDecoderModule(&frame_decoder_parameters_);
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

        work_buffers_.buff_depth[i].width = 0;
        work_buffers_.buff_depth[i].height = 0;
        work_buffers_.buff_depth[i].image = new float[depth_size];
        memset(work_buffers_.buff_depth[i].image, 0, depth_size * sizeof(float));
    }
    
    // initialize Framedecoder
    DisparityFilter::initialize(isc_data_proc_module_configuration_.max_image_height, isc_data_proc_module_configuration_.max_image_width);
    DisparityFilter::createAveragingThread();

    // Check if OpenCL is available. Enable it if it is available.
    if (frame_decoder_parameters_.system_parameter.enabled_opencl_for_avedisp && cv::ocl::haveOpenCL()) {
        // it can use openCL
        //cv::String build_info_str =  cv::getBuildInformation();
        //OutputDebugStringA(build_info_str.c_str());
        DisparityFilter::setUseOpenCLForAveragingDisparity(1);
    }
    else {
        DisparityFilter::setUseOpenCLForAveragingDisparity(0);
    }

    return DPC_E_OK;
}

/**
 * パラメータをファイルから読み込みます.
 *
 * @param[in] file_name パラメータファイル名です
 * @param[out] block_matching_parameters　読み込んだパラメータです
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDisparityFilterInterface::LoadParameterFromFile(const wchar_t* file_name, FrameDecoderParameters* frame_decoder_parameters)
{

    bool is_exists = PathFileExists(file_name) == TRUE ? true : false;
    if (!is_exists) {
        return DPCPROCESS_E_FILE_NOT_FOUND;
    }

    wchar_t returned_string[1024] = {};

    // SystemParameter
    GetPrivateProfileString(L"SYSTEM", L"enabled_opencl_for_avedisp", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    int temp_value = _wtoi(returned_string);
    frame_decoder_parameters->system_parameter.enabled_opencl_for_avedisp = temp_value == 1 ? true : false;

    GetPrivateProfileString(L"SYSTEM", L"single_threaded_execution", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    temp_value = _wtoi(returned_string);
    frame_decoder_parameters->system_parameter.single_threaded_execution = temp_value == 1 ? true : false;

    // DisparityLimitationParameter
    GetPrivateProfileString(L"DISPARITY_LIMITATION", L"limit", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->disparity_limitation_parameter.limit = _wtoi(returned_string);

    GetPrivateProfileString(L"DISPARITY_LIMITATION", L"lower", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->disparity_limitation_parameter.lower = _wtoi(returned_string);

    GetPrivateProfileString(L"DISPARITY_LIMITATION", L"upper", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->disparity_limitation_parameter.upper = _wtoi(returned_string);

    // AveragingParameter
    GetPrivateProfileString(L"AVERAGE", L"enb", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_parameter.enb = _wtoi(returned_string);

    GetPrivateProfileString(L"AVERAGE", L"blkshgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_parameter.blkshgt = _wtoi(returned_string);

    GetPrivateProfileString(L"AVERAGE", L"blkswdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_parameter.blkswdt = _wtoi(returned_string);

    GetPrivateProfileString(L"AVERAGE", L"intg", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_parameter.intg = _wtof(returned_string);

    GetPrivateProfileString(L"AVERAGE", L"range", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_parameter.range = _wtof(returned_string);

    GetPrivateProfileString(L"AVERAGE", L"dsprt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_parameter.dsprt = _wtoi(returned_string);

    GetPrivateProfileString(L"AVERAGE", L"vldrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_parameter.vldrt = _wtoi(returned_string);

    GetPrivateProfileString(L"AVERAGE", L"reprt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_parameter.reprt = _wtoi(returned_string);

    // AveragingBlockWeightParameter
    GetPrivateProfileString(L"AVERAGE_BLOCK_WEIGHT", L"cntwgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_block_weight_parameter.cntwgt = _wtoi(returned_string);

    GetPrivateProfileString(L"AVERAGE_BLOCK_WEIGHT", L"nrwgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_block_weight_parameter.nrwgt = _wtoi(returned_string);

    GetPrivateProfileString(L"AVERAGE_BLOCK_WEIGHT", L"rndwgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->averaging_block_weight_parameter.rndwgt = _wtoi(returned_string);

    // ComplementParameter
    GetPrivateProfileString(L"COMPLEMENT", L"enb", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.enb = _wtoi(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"lowlmt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.lowlmt = _wtof(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"slplmt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.slplmt = _wtof(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"insrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.insrt = _wtof(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"rndrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.rndrt = _wtof(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"crstlmt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.crstlmt = _wtoi(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"hlfil", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.hlfil = _wtoi(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"hlsz", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.hlsz = _wtof(returned_string);

    // Edge Complement
    GetPrivateProfileString(L"EDGE_COMPLEMENT", L"edgcmp", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->edge_complement_parameter.edgcmp = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_COMPLEMENT", L"minblks", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->edge_complement_parameter.minblks = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_COMPLEMENT", L"mincoef", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->edge_complement_parameter.mincoef = _wtoi(returned_string);

    GetPrivateProfileString(L"EDGE_COMPLEMENT", L"cmpwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->edge_complement_parameter.cmpwdt = _wtoi(returned_string);

    // Hough Transform
    GetPrivateProfileString(L"HOUGH_TRANSFORM", L"edgthr1", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->hough_transferm_parameter.edgthr1 = _wtoi(returned_string);

    GetPrivateProfileString(L"HOUGH_TRANSFORM", L"edgthr2", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->hough_transferm_parameter.edgthr2 = _wtoi(returned_string);

    GetPrivateProfileString(L"HOUGH_TRANSFORM", L"linthr", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->hough_transferm_parameter.linthr = _wtoi(returned_string);

    GetPrivateProfileString(L"HOUGH_TRANSFORM", L"minlen", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->hough_transferm_parameter.minlen = _wtoi(returned_string);

    GetPrivateProfileString(L"HOUGH_TRANSFORM", L"maxgap", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->hough_transferm_parameter.maxgap = _wtoi(returned_string);

    return DPC_E_OK;
}

/**
 * パラメータをファイルへ書き込みます.
 *
 * @param[in] file_name パラメータファイル名です
 * @param[in] block_matching_parameters　書き込むパラメータです
 * @retval 0 成功
 * @retval other 失敗
 */int IscDisparityFilterInterface::SaveParameterToFile(const wchar_t* file_name, const FrameDecoderParameters* frame_decoder_parameters)
{

    wchar_t string[1024] = {};

    // SystemParameter
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->system_parameter.enabled_opencl_for_avedisp);
    WritePrivateProfileString(L"SYSTEM", L"enabled_opencl_for_avedisp", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->system_parameter.single_threaded_execution);
    WritePrivateProfileString(L"SYSTEM", L"single_threaded_execution", string, file_name);

    // DisparityLimitationParameter
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->disparity_limitation_parameter.limit);
    WritePrivateProfileString(L"DISPARITY_LIMITATION", L"limit", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->disparity_limitation_parameter.lower);
    WritePrivateProfileString(L"DISPARITY_LIMITATION", L"lower", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->disparity_limitation_parameter.upper);
    WritePrivateProfileString(L"DISPARITY_LIMITATION", L"upper", string, file_name);

    // AveragingParameter
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_parameter.enb);
    WritePrivateProfileString(L"AVERAGE", L"enb", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_parameter.blkshgt);
    WritePrivateProfileString(L"AVERAGE", L"blkshgt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_parameter.blkswdt);
    WritePrivateProfileString(L"AVERAGE", L"blkswdt", string, file_name);

    swprintf_s(string, L"%.3f", frame_decoder_parameters->averaging_parameter.intg);
    WritePrivateProfileString(L"AVERAGE", L"intg", string, file_name);

    swprintf_s(string, L"%.3f", frame_decoder_parameters->averaging_parameter.range);
    WritePrivateProfileString(L"AVERAGE", L"range", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_parameter.dsprt);
    WritePrivateProfileString(L"AVERAGE", L"dsprt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_parameter.vldrt);
    WritePrivateProfileString(L"AVERAGE", L"vldrt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_parameter.reprt);
    WritePrivateProfileString(L"AVERAGE", L"reprt", string, file_name);

    // AveragingBlockWeightParameter
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_block_weight_parameter.cntwgt);
    WritePrivateProfileString(L"AVERAGE_BLOCK_WEIGHT", L"cntwgt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_block_weight_parameter.nrwgt);
    WritePrivateProfileString(L"AVERAGE_BLOCK_WEIGHT", L"nrwgt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->averaging_block_weight_parameter.rndwgt);
    WritePrivateProfileString(L"AVERAGE_BLOCK_WEIGHT", L"rndwgt", string, file_name);

    // ComplementParameter
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->complement_parameter.enb);
    WritePrivateProfileString(L"COMPLEMENT", L"enb", string, file_name);

    swprintf_s(string, L"%.3f", frame_decoder_parameters->complement_parameter.lowlmt);
    WritePrivateProfileString(L"COMPLEMENT", L"lowlmt", string, file_name);

    swprintf_s(string, L"%.3f", frame_decoder_parameters->complement_parameter.slplmt);
    WritePrivateProfileString(L"COMPLEMENT", L"slplmt", string, file_name);

    swprintf_s(string, L"%.3f", frame_decoder_parameters->complement_parameter.insrt);
    WritePrivateProfileString(L"COMPLEMENT", L"insrt", string, file_name);

    swprintf_s(string, L"%.3f", frame_decoder_parameters->complement_parameter.rndrt);
    WritePrivateProfileString(L"COMPLEMENT", L"rndrt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->complement_parameter.crstlmt);
    WritePrivateProfileString(L"COMPLEMENT", L"crstlmt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->complement_parameter.hlfil);
    WritePrivateProfileString(L"COMPLEMENT", L"hlfil", string, file_name);

    swprintf_s(string, L"%.3f", frame_decoder_parameters->complement_parameter.hlsz);
    WritePrivateProfileString(L"COMPLEMENT", L"hlsz", string, file_name);

    // Edge Complement
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->edge_complement_parameter.edgcmp);
    WritePrivateProfileString(L"EDGE_COMPLEMENT", L"edgcmp", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->edge_complement_parameter.minblks);
    WritePrivateProfileString(L"EDGE_COMPLEMENT", L"minblks", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->edge_complement_parameter.mincoef);
    WritePrivateProfileString(L"EDGE_COMPLEMENT", L"mincoef", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->edge_complement_parameter.cmpwdt);
    WritePrivateProfileString(L"EDGE_COMPLEMENT", L"cmpwdt", string, file_name);

    // Hough Transform
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->hough_transferm_parameter.edgthr1);
    WritePrivateProfileString(L"HOUGH_TRANSFORM", L"edgthr1", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->hough_transferm_parameter.edgthr2);
    WritePrivateProfileString(L"HOUGH_TRANSFORM", L"edgthr2", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->hough_transferm_parameter.linthr);
    WritePrivateProfileString(L"HOUGH_TRANSFORM", L"linthr", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->hough_transferm_parameter.minlen);
    WritePrivateProfileString(L"HOUGH_TRANSFORM", L"minlen", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->hough_transferm_parameter.maxgap);
    WritePrivateProfileString(L"HOUGH_TRANSFORM", L"maxgap", string, file_name);


    return DPC_E_OK;
}
                                                                     
 /**
  * パラメータをFrameDecoder実装へ設定します.
  *
  * @param[in] frame_decoder_parameters 設定するパラメータファイルです
  * @retval 0 成功
  * @retval other 失敗
  */
 int IscDisparityFilterInterface::SetParameterToFrameDecoderModule(const FrameDecoderParameters* frame_decoder_parameters)
{

    DisparityFilter::setUseOpenCLForAveragingDisparity(
        frame_decoder_parameters->system_parameter.enabled_opencl_for_avedisp,
        frame_decoder_parameters->system_parameter.single_threaded_execution);

    DisparityFilter::setDisparityLimitation(
        frame_decoder_parameters->disparity_limitation_parameter.limit,
        frame_decoder_parameters->disparity_limitation_parameter.lower,
        frame_decoder_parameters->disparity_limitation_parameter.upper);

    DisparityFilter::setAveragingParameter(
        frame_decoder_parameters->averaging_parameter.enb,
        frame_decoder_parameters->averaging_parameter.blkshgt,
        frame_decoder_parameters->averaging_parameter.blkswdt,
        frame_decoder_parameters->averaging_parameter.intg,
        frame_decoder_parameters->averaging_parameter.range,
        frame_decoder_parameters->averaging_parameter.dsprt,
        frame_decoder_parameters->averaging_parameter.vldrt,
        frame_decoder_parameters->averaging_parameter.reprt);

    DisparityFilter::setAveragingBlockWeight(
        frame_decoder_parameters->averaging_block_weight_parameter.cntwgt,
        frame_decoder_parameters->averaging_block_weight_parameter.nrwgt,
        frame_decoder_parameters->averaging_block_weight_parameter.rndwgt);

    DisparityFilter::setComplementParameter(
        frame_decoder_parameters->complement_parameter.enb,
        frame_decoder_parameters->complement_parameter.lowlmt,
        frame_decoder_parameters->complement_parameter.slplmt,
        frame_decoder_parameters->complement_parameter.insrt,
        frame_decoder_parameters->complement_parameter.rndrt,
        frame_decoder_parameters->complement_parameter.crstlmt,
        frame_decoder_parameters->complement_parameter.hlfil,
        frame_decoder_parameters->complement_parameter.hlsz);

    DisparityFilter::setEdgeComplementParameter(
        frame_decoder_parameters->edge_complement_parameter.edgcmp,
        frame_decoder_parameters->edge_complement_parameter.minblks,
        frame_decoder_parameters->edge_complement_parameter.mincoef,
        frame_decoder_parameters->edge_complement_parameter.cmpwdt);

    DisparityFilter::setHoughTransformParameter(
        frame_decoder_parameters->hough_transferm_parameter.edgthr1,
        frame_decoder_parameters->hough_transferm_parameter.edgthr2,
        frame_decoder_parameters->hough_transferm_parameter.linthr,
        frame_decoder_parameters->hough_transferm_parameter.minlen,
        frame_decoder_parameters->hough_transferm_parameter.maxgap);

    return DPC_E_OK;
}

 /**
  * 終了処理をします.
  *
  * @retval 0 成功
  * @retval other 失敗
  */
 int IscDisparityFilterInterface::Terminate()
{
	
     DisparityFilter::deleteAveragingThread();
    
    // release work
    for (int i = 0; i < 2; i++) {
        work_buffers_.buff_image[i].width = 0;
        work_buffers_.buff_image[i].height = 0;
        work_buffers_.buff_image[i].channel_count = 0;
        delete[] work_buffers_.buff_image[i].image;
        work_buffers_.buff_image[i].image = nullptr;

        work_buffers_.buff_depth[i].width = 0;
        work_buffers_.buff_depth[i].height = 0;
        delete[] work_buffers_.buff_depth[i].image;
        work_buffers_.buff_depth[i].image = nullptr;
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
 void IscDisparityFilterInterface::MakeParameterSet(const int value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
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
 void IscDisparityFilterInterface::MakeParameterSet(const float value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
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
 void IscDisparityFilterInterface::MakeParameterSet(const double value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
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
 int IscDisparityFilterInterface::GetParameter(IscDataProcModuleParameter* isc_data_proc_module_parameter)
{
    // *** Note the maximum number of parameter_sets ***

    isc_data_proc_module_parameter->module_index = 0;
    swprintf_s(isc_data_proc_module_parameter->module_name, L"Disparity Filter\n");

    int index = 0;

    // DisparityLimitationParameter
    MakeParameterSet(frame_decoder_parameters_.disparity_limitation_parameter.limit, L"limit", L"DisparityLimitation", L"視差値の制限　0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.disparity_limitation_parameter.lower, L"lower", L"DisparityLimitation", L"視差値の下限", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.disparity_limitation_parameter.upper, L"upper", L"DisparityLimitation", L"視差値の上限", &isc_data_proc_module_parameter->parameter_set[index++]);

    // AveragingParameter
    MakeParameterSet(frame_decoder_parameters_.averaging_parameter.enb,     L"enb",      L"Averaging", L"平均化処理しない：0 する：1", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_parameter.blkshgt, L"blkshgt",  L"Averaging", L"平均化ブロック高さ（片側）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_parameter.blkswdt, L"blkswdt",  L"Averaging", L"平均化ブロック幅（片側）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_parameter.intg,    L"intg",     L"Averaging", L"平均化移動積分幅（片側）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_parameter.range,   L"range",    L"Averaging", L"平均化分布範囲最大幅（片側）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_parameter.dsprt,   L"dsprt",    L"Averaging", L"平均化視差含有率", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_parameter.vldrt,   L"vldrt",    L"Averaging", L"平均化有効比率", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_parameter.reprt,   L"reprt",    L"Averaging", L"平均化置換有効比率", &isc_data_proc_module_parameter->parameter_set[index++]);

    // AveragingBlockWeightParameter
    MakeParameterSet(frame_decoder_parameters_.averaging_block_weight_parameter.cntwgt, L"cntwgt",   L"AveragingBlockWeight", L"ブロックの重み（中央）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_block_weight_parameter.nrwgt,  L"nrwgt",    L"AveragingBlockWeight", L"ブロックの重み（近傍）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.averaging_block_weight_parameter.rndwgt, L"rndwgt",   L"AveragingBlockWeight", L"ブロックの重み（周辺）", &isc_data_proc_module_parameter->parameter_set[index++]);

    // ComplementParameter
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.enb,        L"enb",      L"Complement", L"補完処理しない：0 する：1", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.lowlmt,     L"lowlmt",   L"Complement", L"視補完最小視差値", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.slplmt,     L"slplmt",   L"Complement", L"補完幅の最大視差勾配", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.insrt,      L"insrt",    L"Complement", L"補完画素幅の視差値倍率（内側）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.rndrt,      L"rndrt",    L"Complement", L"補完画素幅の視差値倍率（周辺）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.crstlmt,    L"crstlmt",  L"Complement", L"補完ブロックのコントラスト上限値", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.hlfil,      L"hlfil",    L"Complement", L"穴埋め処理しない：0 する：1", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.hlsz,       L"hlsz",     L"Complement", L"穴埋め幅", &isc_data_proc_module_parameter->parameter_set[index++]);

    // EdgeComplementParameter
    MakeParameterSet(frame_decoder_parameters_.edge_complement_parameter.edgcmp,    L"edgcmp",  L"EdgeComplement", L"エッジ補完 0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.edge_complement_parameter.minblks,   L"minblks", L"EdgeComplement", L"エッジ線分上の最小視差ブロック数", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.edge_complement_parameter.mincoef,   L"mincoef", L"EdgeComplement", L"エッジ視差の最小線形性指数", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.edge_complement_parameter.cmpwdt,    L"cmpwdt",  L"EdgeComplement", L"エッジ線の補完視差ブロック幅", &isc_data_proc_module_parameter->parameter_set[index++]);

    // HoughTransformParameter
    MakeParameterSet(frame_decoder_parameters_.hough_transferm_parameter.edgthr1,   L"edgthr1", L"HoughTransform", L"Cannyエッジ検出閾値1", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.hough_transferm_parameter.edgthr2,   L"edgthr2", L"HoughTransform", L"Cannyエッジ検出閾値2", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.hough_transferm_parameter.linthr,    L"linthr",  L"HoughTransform", L"HoughLinesP投票閾値", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.hough_transferm_parameter.minlen,    L"minlen",  L"HoughTransform", L"HoughLinesP最小線分長", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.hough_transferm_parameter.maxgap,    L"maxgap",  L"HoughTransform", L"HoughLinesP最大ギャップ長", &isc_data_proc_module_parameter->parameter_set[index++]);

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
 void IscDisparityFilterInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, int* value)
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
 void IscDisparityFilterInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, float* value)
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
 void IscDisparityFilterInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, double* value)
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
 int IscDisparityFilterInterface::SetParameter(IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file)
{

    int index = 0;

    // DisparityLimitationParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.disparity_limitation_parameter.limit);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.disparity_limitation_parameter.lower);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.disparity_limitation_parameter.upper);

    // AveragingParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_parameter.enb);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_parameter.blkshgt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_parameter.blkswdt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_parameter.intg);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_parameter.range);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_parameter.dsprt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_parameter.vldrt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_parameter.reprt);

    // AveragingBlockWeightParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_block_weight_parameter.cntwgt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_block_weight_parameter.nrwgt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.averaging_block_weight_parameter.rndwgt);

    // ComplementParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.enb);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.lowlmt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.slplmt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.insrt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.rndrt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.crstlmt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.hlfil);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.hlsz);

    // EdgeComplementParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.edge_complement_parameter.edgcmp);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.edge_complement_parameter.minblks);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.edge_complement_parameter.mincoef);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.edge_complement_parameter.cmpwdt);

    // HoughTransformParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.hough_transferm_parameter.edgthr1);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.hough_transferm_parameter.edgthr2);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.hough_transferm_parameter.linthr);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.hough_transferm_parameter.minlen);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.hough_transferm_parameter.maxgap);

    parameter_update_request_ = true;

    // file
    if (is_update_file) {
        // Update the file with current parameters
        SaveParameterToFile(parameter_file_name_, &frame_decoder_parameters_);
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
 int IscDisparityFilterInterface::GetParameterFileName(wchar_t* file_name, const int max_length)
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
 int IscDisparityFilterInterface::ReloadParameterFromFile(const wchar_t* file_name, const bool is_valid)
{

    wchar_t parameter_file_name[_MAX_PATH] = {};
    swprintf_s(parameter_file_name, L"%s", file_name);

    int ret = LoadParameterFromFile(parameter_file_name, &frame_decoder_parameters_);
    if (ret != DPC_E_OK) {
        return ret;
    }

    ret = SetParameterToFrameDecoderModule(&frame_decoder_parameters_);
    if (ret != DPC_E_OK) {
        return ret;
    }
    
    return DPC_E_OK;
}

/**
 * 視差を平均化します.
 *
 * @param[in] isc_image_Info 入力画像・データ
 * @param[out] isc_data_proc_result_data　処理結果データ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDisparityFilterInterface::GetAverageDisparityData(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_block_disparity_data, IscDataProcResultData* isc_data_proc_result_data)
{

    if ((isc_block_disparity_data->image_width == 0) || (isc_block_disparity_data->image_height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToFrameDecoderModule(&frame_decoder_parameters_);
        parameter_update_request_ = false;
    }

    IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

    int image_width = isc_block_disparity_data->image_width;
    int image_height = isc_block_disparity_data->image_height;

    int imghgt      = image_height;
    int imgwdt      = image_width;
    unsigned char* prgtimg = dst_isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_LATEST].p1.image;
    int blkhgt      = isc_block_disparity_data->blkhgt;
    int blkwdt      = isc_block_disparity_data->blkwdt;
    int mtchgt      = isc_block_disparity_data->mtchgt;
    int mtcwdt      = isc_block_disparity_data->mtcwdt;
    int dspofsx     = isc_block_disparity_data->dspofsx;
    int dspofsy     = isc_block_disparity_data->dspofsy;
    int depth       = isc_block_disparity_data->depth;;
    int shdwdt      = isc_block_disparity_data->shdwdt;;
    int* pblkval    = isc_block_disparity_data->pblkval;
    int* pblkcrst   = isc_block_disparity_data->pblkcrst;

    work_buffers_.buff_image[0].width = image_width;
    work_buffers_.buff_image[0].height = image_height;
    work_buffers_.buff_image[0].channel_count = 1;
    unsigned char* pdspimg = work_buffers_.buff_image[0].image;

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    dst_isc_image_info->frame_data[fd_index].depth.width = image_width;
    dst_isc_image_info->frame_data[fd_index].depth.height = image_height;
    float* ppxldsp = dst_isc_image_info->frame_data[fd_index].depth.image;

    work_buffers_.buff_depth[0].width = image_width;
    work_buffers_.buff_depth[0].height = image_height;
    float* pblkdsp = work_buffers_.buff_depth[0].image;

    bool ret = DisparityFilter::averageDisparityData(
        imghgt,     // 画像の高さ
        imgwdt,     // 画像の幅 
        prgtimg,    // 右（基準）画像データ 右下原点
        blkhgt,     // 視差ブロックの高さ 
        blkwdt,     // 視差ブロックの幅 
        mtchgt,     // マッチングブロックの高さ 
        mtcwdt,     // マッチングブロックの幅
        dspofsx,    // 視差ブロック横オフセット
        dspofsy,    // 視差ブロック縦オフセット
        depth,      // マッチング探索幅 
        shdwdt,     // 遮蔽領域幅
        pblkval,    // 視差ブロック視差値(1000倍サブピクセル精度整数)
        pblkcrst,   // ブロックコントラスト
        pdspimg,    // 視差画像 右下原点 
        ppxldsp,    // 視差データ 右下原点 
        pblkdsp);   // ブロック視差データ 右下原点

    return DPC_E_OK;
}

/**
 * 視差を平均化します.(Doube Shutter 用)
 *
 * @param[in] isc_image_Info 入力画像・データ
 * @param[out] isc_data_proc_result_data　処理結果データ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDisparityFilterInterface::GetAverageDisparityDataDoubleShutter(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_block_disparity_data, IscDataProcResultData* isc_data_proc_result_data)
{

    if ((isc_block_disparity_data->image_width == 0) || (isc_block_disparity_data->image_height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToFrameDecoderModule(&frame_decoder_parameters_);
        parameter_update_request_ = false;
    }

    IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

    int image_width = isc_block_disparity_data->image_width;
    int image_height = isc_block_disparity_data->image_height;

    int imghgt = image_height;
    int imgwdt = image_width;
    unsigned char* prgtimg = isc_block_disparity_data->pbldimg;
    int blkhgt = isc_block_disparity_data->blkhgt;
    int blkwdt = isc_block_disparity_data->blkwdt;
    int mtchgt = isc_block_disparity_data->mtchgt;
    int mtcwdt = isc_block_disparity_data->mtcwdt;
    int dspofsx = isc_block_disparity_data->dspofsx;
    int dspofsy = isc_block_disparity_data->dspofsy;
    int depth = isc_block_disparity_data->depth;;
    int shdwdt = isc_block_disparity_data->shdwdt;;
    int* pblkval = isc_block_disparity_data->pblkval;
    int* pblkcrst = isc_block_disparity_data->pblkcrst;

    work_buffers_.buff_image[0].width = image_width;
    work_buffers_.buff_image[0].height = image_height;
    work_buffers_.buff_image[0].channel_count = 1;
    unsigned char* pdspimg = work_buffers_.buff_image[0].image;

    int fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;

    dst_isc_image_info->frame_data[fd_index].depth.width = image_width;
    dst_isc_image_info->frame_data[fd_index].depth.height = image_height;
    float* ppxldsp = dst_isc_image_info->frame_data[fd_index].depth.image;

    work_buffers_.buff_depth[0].width = image_width;
    work_buffers_.buff_depth[0].height = image_height;
    float* pblkdsp = work_buffers_.buff_depth[0].image;

    bool ret = DisparityFilter::averageDisparityData(
        imghgt,     // 画像の高さ
        imgwdt,     // 画像の幅 
        prgtimg,    // 右（基準）画像データ 右下原点
        blkhgt,     // 視差ブロックの高さ 
        blkwdt,     // 視差ブロックの幅 
        mtchgt,     // マッチングブロックの高さ 
        mtcwdt,     // マッチングブロックの幅
        dspofsx,    // 視差ブロック横オフセット
        dspofsy,    // 視差ブロック縦オフセット
        depth,      // マッチング探索幅 
        shdwdt,     // 遮蔽領域幅
        pblkval,    // 視差ブロック視差値(1000倍サブピクセル精度整数)
        pblkcrst,   // ブロックコントラスト
        pdspimg,    // 視差画像 右下原点 
        ppxldsp,    // 視差データ 右下原点 
        pblkdsp);   // ブロック視差データ 右下原点

    return DPC_E_OK;
}

