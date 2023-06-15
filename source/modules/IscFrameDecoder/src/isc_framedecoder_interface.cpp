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
 * @file isc_framedecoder_interface.cpp
 * @brief provides an interface for implementing Frame Decoder
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a interface for Frame Decoder.
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

#include "isc_framedecoder_interface.h"

#include "ISCFrameDecoder.h"

#include "opencv2\opencv.hpp"

#pragma comment (lib, "shlwapi")
#pragma comment (lib, "Shell32")
#pragma comment (lib, "imagehlp")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world470d")
#else
#pragma comment (lib,"opencv_world470")
#endif


/**
 * constructor
 *
 */
IscFramedecoderInterface::IscFramedecoderInterface():
    parameter_update_request_(false), 
    parameter_file_name_(),
    isc_data_proc_module_configuration_(),
    frame_decoder_parameters_(),
    work_buffers_()
{
    // default
    frame_decoder_parameters_.system_parameter.enabled_opencl_for_avedisp = 0;

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
    frame_decoder_parameters_.complement_parameter.btmrt = 0.1;
    frame_decoder_parameters_.complement_parameter.crstlmt = 40;   // VM:45
    frame_decoder_parameters_.complement_parameter.hlfil = 1;
    frame_decoder_parameters_.complement_parameter.hlsz = 8;

    frame_decoder_parameters_.decode_parameter.crstthr = 40;

}

/**
 * destructor
 *
 */
IscFramedecoderInterface::~IscFramedecoderInterface()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] isc_data_proc_module_configuration 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscFramedecoderInterface::Initialize(IscDataProcModuleConfiguration* isc_data_proc_module_configuration)
{
	
    swprintf_s(isc_data_proc_module_configuration_.configuration_file_path, L"%s", isc_data_proc_module_configuration->configuration_file_path);
    swprintf_s(isc_data_proc_module_configuration_.log_file_path, L"%s", isc_data_proc_module_configuration->log_file_path);

    isc_data_proc_module_configuration_.log_level = isc_data_proc_module_configuration->log_level;
    isc_data_proc_module_configuration_.isc_camera_model = isc_data_proc_module_configuration->isc_camera_model;
    isc_data_proc_module_configuration_.max_image_width = isc_data_proc_module_configuration->max_image_width;
    isc_data_proc_module_configuration_.max_image_height = isc_data_proc_module_configuration->max_image_height;

    switch (isc_data_proc_module_configuration_.isc_camera_model) {
    case IscCameraModel::kVM:
        swprintf_s(parameter_file_name_, L"%s\\FrameDecoderParameter_VM.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::kXC:
        swprintf_s(parameter_file_name_, L"%s\\FrameDecoderParameter_XC.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4K:
        swprintf_s(parameter_file_name_, L"%s\\FrameDecoderParameter_4K.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4KA:
        swprintf_s(parameter_file_name_, L"%s\\FrameDecoderParameter_4KA.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4KJ:
        swprintf_s(parameter_file_name_, L"%s\\FrameDecoderParameter_4KJ.ini", isc_data_proc_module_configuration_.configuration_file_path);
        break;

    default:
        swprintf_s(parameter_file_name_, L"%s\\FrameDecoderParameter.ini", isc_data_proc_module_configuration_.configuration_file_path);
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

    for (int i = 0; i < 4; i++) {
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
    ISCFrameDecoder::initialize(isc_data_proc_module_configuration_.max_image_height, isc_data_proc_module_configuration_.max_image_width);
    ISCFrameDecoder::createAveragingThread();

    // Check if OpenCL is available. Enable it if it is available.
    if (frame_decoder_parameters_.system_parameter.enabled_opencl_for_avedisp && cv::ocl::haveOpenCL()) {
        // it can use openCL
        //cv::String build_info_str =  cv::getBuildInformation();
        //OutputDebugStringA(build_info_str.c_str());
        ISCFrameDecoder::setUseOpenCLForAveragingDisparity(1);
    }
    else {
        ISCFrameDecoder::setUseOpenCLForAveragingDisparity(0);
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
int IscFramedecoderInterface::LoadParameterFromFile(const wchar_t* file_name, FrameDecoderParameters* frame_decoder_parameters)
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

    GetPrivateProfileString(L"COMPLEMENT", L"btmrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.btmrt = _wtof(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"crstlmt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.crstlmt = _wtoi(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"hlfil", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.hlfil = _wtoi(returned_string);

    GetPrivateProfileString(L"COMPLEMENT", L"hlsz", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->complement_parameter.hlsz = _wtof(returned_string);

    // Decode
    GetPrivateProfileString(L"DECODE", L"crstthr", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->decode_parameter.crstthr = _wtoi(returned_string);

    return DPC_E_OK;
}

/**
 * パラメータをファイルへ書き込みます.
 *
 * @param[in] file_name パラメータファイル名です
 * @param[in] block_matching_parameters　書き込むパラメータです
 * @retval 0 成功
 * @retval other 失敗
 */int IscFramedecoderInterface::SaveParameterToFile(const wchar_t* file_name, const FrameDecoderParameters* frame_decoder_parameters)
{

    wchar_t string[1024] = {};

    // SystemParameter
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->system_parameter.enabled_opencl_for_avedisp);
    WritePrivateProfileString(L"SYSTEM", L"enabled_opencl_for_avedisp", string, file_name);

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

    swprintf_s(string, L"%.3f", frame_decoder_parameters->complement_parameter.btmrt);
    WritePrivateProfileString(L"COMPLEMENT", L"btmrt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->complement_parameter.crstlmt);
    WritePrivateProfileString(L"COMPLEMENT", L"crstlmt", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->complement_parameter.hlfil);
    WritePrivateProfileString(L"COMPLEMENT", L"hlfil", string, file_name);

    swprintf_s(string, L"%.3f", frame_decoder_parameters->complement_parameter.hlsz);
    WritePrivateProfileString(L"COMPLEMENT", L"hlsz", string, file_name);

    // Decode
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->decode_parameter.crstthr);
    WritePrivateProfileString(L"DECODE", L"crstthr", string, file_name);

    return DPC_E_OK;
}
                                                                     
 /**
  * パラメータをFrameDecoder実装へ設定します.
  *
  * @param[in] frame_decoder_parameters 設定するパラメータファイルです
  * @retval 0 成功
  * @retval other 失敗
  */
 int IscFramedecoderInterface::SetParameterToFrameDecoderModule(const FrameDecoderParameters* frame_decoder_parameters)
{

    ISCFrameDecoder::setUseOpenCLForAveragingDisparity(frame_decoder_parameters->system_parameter.enabled_opencl_for_avedisp);

    ISCFrameDecoder::setDisparityLimitation(
        frame_decoder_parameters->disparity_limitation_parameter.limit,
        frame_decoder_parameters->disparity_limitation_parameter.lower,
        frame_decoder_parameters->disparity_limitation_parameter.upper);

     ISCFrameDecoder::setAveragingParameter(
        frame_decoder_parameters->averaging_parameter.enb,
        frame_decoder_parameters->averaging_parameter.blkshgt,
        frame_decoder_parameters->averaging_parameter.blkswdt,
        frame_decoder_parameters->averaging_parameter.intg,
        frame_decoder_parameters->averaging_parameter.range,
        frame_decoder_parameters->averaging_parameter.dsprt,
        frame_decoder_parameters->averaging_parameter.vldrt,
        frame_decoder_parameters->averaging_parameter.reprt);

     ISCFrameDecoder::setAveragingBlockWeight(
         frame_decoder_parameters->averaging_block_weight_parameter.cntwgt,
         frame_decoder_parameters->averaging_block_weight_parameter.nrwgt,
         frame_decoder_parameters->averaging_block_weight_parameter.rndwgt);

     ISCFrameDecoder::setComplementParameter(
         frame_decoder_parameters->complement_parameter.enb,
         frame_decoder_parameters->complement_parameter.lowlmt,
         frame_decoder_parameters->complement_parameter.slplmt,
         frame_decoder_parameters->complement_parameter.insrt,
         frame_decoder_parameters->complement_parameter.rndrt,
         frame_decoder_parameters->complement_parameter.btmrt,
         frame_decoder_parameters->complement_parameter.crstlmt,
         frame_decoder_parameters->complement_parameter.hlfil,
         frame_decoder_parameters->complement_parameter.hlsz);

    return DPC_E_OK;
}

 /**
  * 終了処理をします.
  *
  * @retval 0 成功
  * @retval other 失敗
  */
 int IscFramedecoderInterface::Terminate()
{
	
    ISCFrameDecoder::deleteAveragingThread();
    
    // release work
    for (int i = 0; i < 4; i++) {
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
 void IscFramedecoderInterface::MakeParameterSet(const int value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
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
 void IscFramedecoderInterface::MakeParameterSet(const float value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
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
 void IscFramedecoderInterface::MakeParameterSet(const double value, const wchar_t* name, const wchar_t* category, const wchar_t* description, IscDataProcModuleParameter::ParameterSet* parameter_set)
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
 int IscFramedecoderInterface::GetParameter(IscDataProcModuleParameter* isc_data_proc_module_parameter)
{
    // *** Note the maximum number of parameter_sets ***

    isc_data_proc_module_parameter->module_index = 0;
    swprintf_s(isc_data_proc_module_parameter->module_name, L"Frame Decorder\n");

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
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.btmrt,      L"btmrt",    L"Complement", L"補完画素幅の視差値倍率（下端）", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.crstlmt,    L"crstlmt",  L"Complement", L"補完ブロックのコントラスト上限値", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.hlfil,      L"hlfil",    L"Complement", L"穴埋め処理しない：0 する：1", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.complement_parameter.hlsz,       L"hlsz",     L"Complement", L"穴埋め幅", &isc_data_proc_module_parameter->parameter_set[index++]);

    // DecodeParameter
    MakeParameterSet(frame_decoder_parameters_.decode_parameter.crstthr, L"crstthr", L"Decode", L"コントラスト閾値", &isc_data_proc_module_parameter->parameter_set[index++]);

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
 void IscFramedecoderInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, int* value)
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
 void IscFramedecoderInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, float* value)
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
 void IscFramedecoderInterface::ParseParameterSet(const IscDataProcModuleParameter::ParameterSet* parameter_set, double* value)
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
 int IscFramedecoderInterface::SetParameter(IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file)
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
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.btmrt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.crstlmt);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.hlfil);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.complement_parameter.hlsz);

    // DecodeParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.decode_parameter.crstthr);

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
 int IscFramedecoderInterface::GetParameterFileName(wchar_t* file_name, const int max_length)
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
 int IscFramedecoderInterface::ReloadParameterFromFile(const wchar_t* file_name, const bool is_valid)
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
 * 視差データをデコードして視差画像と視差情報に戻し、平均化、補完処理を行いします.
 *
 * @param[in] isc_image_Info 入力画像・データ
 * @param[out] isc_data_proc_result_data　処理結果データ
 * @retval 0 成功
 * @retval other 失敗
 */
 int IscFramedecoderInterface::GetDecodeData(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data)
{

    if (isc_image_Info->grab != IscGrabMode::kParallax) {
        return DPCPROCESS_E_INVALID_MODE;
    }
   
    if ((isc_image_Info->raw.width == 0) || (isc_image_Info->raw.height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToFrameDecoderModule(&frame_decoder_parameters_);
        parameter_update_request_ = false;
    }

    IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;
   
    // (1)
    int width = isc_image_Info->raw.width / 2;
    int height = isc_image_Info->raw.height;
    unsigned char* raw_image = isc_image_Info->raw.image;

    work_buffers_.buff_image[0].width = width;
    work_buffers_.buff_image[0].height = height;
    work_buffers_.buff_image[0].channel_count = 1;
    unsigned char* prgtimg = work_buffers_.buff_image[0].image;

    work_buffers_.buff_image[1].width = width;
    work_buffers_.buff_image[1].height = height;
    work_buffers_.buff_image[1].channel_count = 1;
    unsigned char* plftimg = work_buffers_.buff_image[1].image;

    work_buffers_.buff_image[2].width = width;
    work_buffers_.buff_image[2].height = height;
    work_buffers_.buff_image[2].channel_count = 1;
    unsigned char* pdspenc = work_buffers_.buff_image[2].image;

    ISCFrameDecoder::decodeFrameData(
        height,     // 画像の高さ
        width,      // 画像の幅
        raw_image,  // フレームデータ
        prgtimg,    // 右（基準）画像データ 右下原点
        plftimg,    // 左（比較）画像データ 右下原点
        pdspenc);   // 視差エンコードデータ

    // (2)
    int decode_width = isc_image_Info->raw.width / 2;
    int decode_height = isc_image_Info->raw.height;

    work_buffers_.buff_image[3].width = decode_width;
    work_buffers_.buff_image[3].height = decode_height;
    work_buffers_.buff_image[3].channel_count = 1;
    unsigned char* pdspimg = work_buffers_.buff_image[3].image;
    
    dst_isc_image_info->depth.width = decode_width;
    dst_isc_image_info->depth.height = decode_height;
    float* ppxldsp = dst_isc_image_info->depth.image;

    work_buffers_.buff_depth[0].width = decode_width;
    work_buffers_.buff_depth[0].height = decode_height;
    float* pblkdsp = work_buffers_.buff_depth[0].image;

    int crstthr = frame_decoder_parameters_.decode_parameter.crstthr;

    ISCFrameDecoder::decodeDisparityData(
        decode_height,  // 画像の高さ
        decode_width,   // 画像の幅
        prgtimg,        // 右（基準）画像データ 右下原点
        crstthr,        // コントラスト閾値
        pdspenc,        // 視差エンコードデータ
        pdspimg,        // 視差画像 右下原点
        ppxldsp,        // 視差データ 右下原点
        pblkdsp);       // ブロック視差データ 右下基点

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
int IscFramedecoderInterface::GetDecodeAverageDisparityData(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_block_disparity_data, IscDataProcResultData* isc_data_proc_result_data)
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
    int blkhgt = isc_block_disparity_data->pblkhgt;
    int blkwdt = isc_block_disparity_data->pblkwdt;
    int mtchgt = isc_block_disparity_data->pmtchgt;
    int mtcwdt = isc_block_disparity_data->pmtcwdt;
    int dspofsx = isc_block_disparity_data->pblkofsx;
    int dspofsy = isc_block_disparity_data->pblkofsy;
    int depth = isc_block_disparity_data->pdepth;
    int pshdwdt = isc_block_disparity_data->pshdwdt;

    int* pblkval = isc_block_disparity_data->pblkval;
    int* pblkcrst = isc_block_disparity_data->pblkcrst;


    work_buffers_.buff_image[0].width = image_width;
    work_buffers_.buff_image[0].height = image_height;
    work_buffers_.buff_image[0].channel_count = 1;
    unsigned char* pdspimg = work_buffers_.buff_image[0].image;

    dst_isc_image_info->depth.width = image_width;
    dst_isc_image_info->depth.height = image_height;
    float* ppxldsp = dst_isc_image_info->depth.image;

    work_buffers_.buff_depth[0].width = image_width;
    work_buffers_.buff_depth[0].height = image_height;
    float* pblkdsp = work_buffers_.buff_depth[0].image;

    ISCFrameDecoder::averageDisparityData(
        imghgt,     // 画像の高さ
        imgwdt,     // 画像の幅 
        blkhgt,     // 視差ブロックの高さ
        blkwdt,     // 視差ブロックの幅
        mtchgt,     // マッチングブロックの高さ 
        mtcwdt,     // マッチングブロックの幅 
        dspofsx,    // 視差ブロック横オフセット
        dspofsy,    // 視差ブロック縦オフセット
        depth,      // マッチング探索幅
        pshdwdt,    // 画像遮蔽幅
        pblkval,    // 視差ブロック視差値(1000倍サブピクセル精度整数)    
        pblkcrst,   // ブロックコントラスト 
        pdspimg,    // 視差画像 右下原点
        ppxldsp,    // 視差データ 右下原点 
        pblkdsp);   // ブロック視差データ 右下原点 

    return DPC_E_OK;
}
