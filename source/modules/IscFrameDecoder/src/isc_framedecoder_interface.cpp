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
    // defult for XC
    frame_decoder_parameters_.decode_parameter.crstthr = 40;
    frame_decoder_parameters_.decode_parameter.crsthrm = 0;

    frame_decoder_parameters_.disparity_limitation_parameter.limit = 0;
    frame_decoder_parameters_.disparity_limitation_parameter.lower = 0;
    frame_decoder_parameters_.disparity_limitation_parameter.upper = 255;

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

    for (int i = 0; i < 8; i++) {
        work_buffers_.buff_image[i].width = 0;
        work_buffers_.buff_image[i].height = 0;
        work_buffers_.buff_image[i].channel_count = 0;
        work_buffers_.buff_image[i].image = new unsigned char[image_size];
        memset(work_buffers_.buff_image[i].image, 0, image_size);

        work_buffers_.buff_depth[i].width = 0;
        work_buffers_.buff_depth[i].height = 0;
        work_buffers_.buff_depth[i].image = new float[depth_size];
        memset(work_buffers_.buff_depth[i].image, 0, depth_size * sizeof(float));

        work_buffers_.buff_int_image[i].width = 0;
        work_buffers_.buff_int_image[i].height = 0;
        work_buffers_.buff_int_image[i].image = new int[image_size];
        memset(work_buffers_.buff_int_image[i].image, 0, image_size * sizeof(int));
    }
    
    // initialize Framedecoder
    ISCFrameDecoder::initialize(isc_data_proc_module_configuration_.max_image_height, isc_data_proc_module_configuration_.max_image_width);

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

    // Decode
    GetPrivateProfileString(L"DECODE", L"crstthr", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->decode_parameter.crstthr = _wtoi(returned_string);

    GetPrivateProfileString(L"DECODE", L"crsthrm", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->decode_parameter.crsthrm = _wtoi(returned_string);

    // DisparityLimitationParameter
    GetPrivateProfileString(L"DISPARITY_LIMITATION", L"limit", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->disparity_limitation_parameter.limit = _wtoi(returned_string);

    GetPrivateProfileString(L"DISPARITY_LIMITATION", L"lower", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->disparity_limitation_parameter.lower = _wtoi(returned_string);

    GetPrivateProfileString(L"DISPARITY_LIMITATION", L"upper", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    frame_decoder_parameters->disparity_limitation_parameter.upper = _wtoi(returned_string);


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

    // Decode
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->decode_parameter.crstthr);
    WritePrivateProfileString(L"DECODE", L"crstthr", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->decode_parameter.crsthrm);
    WritePrivateProfileString(L"DECODE", L"crsthrm", string, file_name);

    // DisparityLimitationParameter
    swprintf_s(string, L"%d", (int)frame_decoder_parameters->disparity_limitation_parameter.limit);
    WritePrivateProfileString(L"DISPARITY_LIMITATION", L"limit", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->disparity_limitation_parameter.lower);
    WritePrivateProfileString(L"DISPARITY_LIMITATION", L"lower", string, file_name);

    swprintf_s(string, L"%d", (int)frame_decoder_parameters->disparity_limitation_parameter.upper);
    WritePrivateProfileString(L"DISPARITY_LIMITATION", L"upper", string, file_name);

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

     ISCFrameDecoder::setFrameDecoderParameter(
         frame_decoder_parameters->decode_parameter.crstthr,
         frame_decoder_parameters->decode_parameter.crsthrm);

    ISCFrameDecoder::setDisparityLimitation(
        frame_decoder_parameters->disparity_limitation_parameter.limit,
        frame_decoder_parameters->disparity_limitation_parameter.lower,
        frame_decoder_parameters->disparity_limitation_parameter.upper);

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
	
    ISCFrameDecoder::finalize();
    
    // release work
    for (int i = 0; i < 8; i++) {
        work_buffers_.buff_image[i].width = 0;
        work_buffers_.buff_image[i].height = 0;
        work_buffers_.buff_image[i].channel_count = 0;
        delete[] work_buffers_.buff_image[i].image;
        work_buffers_.buff_image[i].image = nullptr;

        work_buffers_.buff_depth[i].width = 0;
        work_buffers_.buff_depth[i].height = 0;
        delete[] work_buffers_.buff_depth[i].image;
        work_buffers_.buff_depth[i].image = nullptr;

        work_buffers_.buff_int_image[i].width = 0;
        work_buffers_.buff_int_image[i].height = 0;
        delete[] work_buffers_.buff_int_image[i].image;
        work_buffers_.buff_int_image[i].image = nullptr;
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

    // DecodeParameter
    MakeParameterSet(frame_decoder_parameters_.decode_parameter.crstthr, L"crstthr", L"Decode", L"コントラスト閾値", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.decode_parameter.crsthrm, L"crsthrm", L"Decode", L"センサー輝度高解像度モード 0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);

    // DisparityLimitationParameter
    MakeParameterSet(frame_decoder_parameters_.disparity_limitation_parameter.limit, L"limit", L"DisparityLimitation", L"視差値の制限　0:しない 1:する", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.disparity_limitation_parameter.lower, L"lower", L"DisparityLimitation", L"視差値の下限", &isc_data_proc_module_parameter->parameter_set[index++]);
    MakeParameterSet(frame_decoder_parameters_.disparity_limitation_parameter.upper, L"upper", L"DisparityLimitation", L"視差値の上限", &isc_data_proc_module_parameter->parameter_set[index++]);

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

    // DecodeParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.decode_parameter.crstthr);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.decode_parameter.crsthrm);

    // DisparityLimitationParameter
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.disparity_limitation_parameter.limit);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.disparity_limitation_parameter.lower);
    ParseParameterSet(&isc_data_proc_module_parameter->parameter_set[index++], &frame_decoder_parameters_.disparity_limitation_parameter.upper);

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
 int IscFramedecoderInterface::GetDecodeData(IscImageInfo* isc_image_Info, IscBlockDisparityData* isc_block_disparity_data)
{

    if (isc_image_Info->grab != IscGrabMode::kParallax) {
        return DPCPROCESS_E_INVALID_MODE;
    }
   
    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    if ((isc_image_Info->frame_data[fd_index].raw.width == 0) || (isc_image_Info->frame_data[fd_index].raw.height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToFrameDecoderModule(&frame_decoder_parameters_);
        parameter_update_request_ = false;
    }
  
    // (1)
    int width = isc_image_Info->frame_data[fd_index].raw.width / 2;
    int height = isc_image_Info->frame_data[fd_index].raw.height;
    unsigned char* raw_image = isc_image_Info->frame_data[fd_index].raw.image;

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

    ISCFrameDecoder::decodeFrameData(
        height,     // 画像の高さ
        width,      // 画像の幅
        raw_image,  // フレームデータ
        prgtimg,    // 右（基準）画像データ 右下原点
        plftimg);   // 左（比較）画像データまたは視差エンコードデータ 右下原点


    // (2)
    int decode_width = isc_image_Info->frame_data[fd_index].raw.width / 2;
    int decode_height = isc_image_Info->frame_data[fd_index].raw.height;

    work_buffers_.buff_image[3].width = decode_width;
    work_buffers_.buff_image[3].height = decode_height;
    work_buffers_.buff_image[3].channel_count = 1;
    unsigned char* pdspimg = work_buffers_.buff_image[3].image;

    int* pblkhgt = &isc_block_disparity_data->blkhgt;
    int* pblkwdt = &isc_block_disparity_data->blkwdt;
    int* pmtchgt = &isc_block_disparity_data->mtchgt;
    int* pmtcwdt = &isc_block_disparity_data->mtcwdt;
    int* pblkofsx = &isc_block_disparity_data->dspofsx;
    int* pblkofsy = &isc_block_disparity_data->dspofsy;
    int* pdepth = &isc_block_disparity_data->depth;
    int* pshdwdt = &isc_block_disparity_data->shdwdt;

    float* ppxldsp = isc_block_disparity_data->ppxldsp;
    float* pblkdsp = isc_block_disparity_data->pblkdsp;
    int* pblkval = isc_block_disparity_data->pblkval;
    int* pblkcrst = isc_block_disparity_data->pblkcrst;

    ISCFrameDecoder::getDisparityData(
        decode_height,  // 画像の高さ
        decode_width,   // 画像の幅
        prgtimg,        // 右（基準）画像データ 右下原点
        plftimg,        // 視差エンコードデータ
        pblkhgt,        // 視差ブロック高さ
        pblkwdt,        // 視差ブロック幅
        pmtchgt,        // マッチングブロック高さ
        pmtcwdt,        // マッチングブロック幅
        pblkofsx,       // 視差ブロック横オフセット
        pblkofsy,       // 視差ブロック縦オフセット
        pdepth,         // マッチング探索幅
        pshdwdt,        // 画像遮蔽幅
        pdspimg,        // 視差画像 右下原点
        ppxldsp,        // 視差データ 右下原点
        pblkdsp,        // ブロック視差データ 右下基点 
        pblkval,        // 視差ブロック視差値(1000倍サブピクセル精度整数)
        pblkcrst);      // ブロックコントラスト

    isc_block_disparity_data->image_width = isc_image_Info->frame_data[fd_index].p1.width;
    isc_block_disparity_data->image_height = isc_image_Info->frame_data[fd_index].p1.height;

    return DPC_E_OK;
}

 /**
  * Double Shutterモード使用時に、視差データをデコードして視差画像と視差情報に戻し、平均化、補完処理を行いします.
  *
  * @param[in] isc_image_Info 入力画像・データ
  * @param[out] isc_data_proc_result_data　処理結果データ
  * @retval 0 成功
  * @retval other 失敗
  */
int IscFramedecoderInterface::GetDecodeDataDoubleShutter(IscImageInfo* isc_image_info_in, IscBlockDisparityData* isc_block_disparity_data, IscImageInfo* isc_image_info_out)
{

    if (isc_image_info_in->grab != IscGrabMode::kParallax) {
        return DPCPROCESS_E_INVALID_MODE;
    }

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    if ((isc_image_info_in->frame_data[fd_index].raw.width == 0) || (isc_image_info_in->frame_data[fd_index].raw.height == 0)) {
        return DPC_E_OK;
    }

    fd_index = kISCIMAGEINFO_FRAMEDATA_PREVIOUS;
    if ((isc_image_info_in->frame_data[fd_index].raw.width == 0) || (isc_image_info_in->frame_data[fd_index].raw.height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToFrameDecoderModule(&frame_decoder_parameters_);
        parameter_update_request_ = false;
    }

    // (1)
    fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;
    int width_latest = isc_image_info_in->frame_data[fd_index].raw.width / 2;
    int height_latest = isc_image_info_in->frame_data[fd_index].raw.height;
    unsigned char* raw_image_latest = isc_image_info_in->frame_data[fd_index].raw.image;

    isc_image_info_out->frame_data[fd_index].p1.width = width_latest;
    isc_image_info_out->frame_data[fd_index].p1.height = height_latest;
    isc_image_info_out->frame_data[fd_index].p1.channel_count = 1;
    unsigned char* prgtimg_latest = isc_image_info_out->frame_data[fd_index].p1.image;

    isc_image_info_out->frame_data[fd_index].p2.width = width_latest;
    isc_image_info_out->frame_data[fd_index].p2.height = height_latest;
    isc_image_info_out->frame_data[fd_index].p2.channel_count = 1;
    unsigned char* plftimg_latest = isc_image_info_out->frame_data[fd_index].p2.image;

    ISCFrameDecoder::decodeFrameData(
        height_latest,     // 画像の高さ
        width_latest,      // 画像の幅
        raw_image_latest,  // フレームデータ
        prgtimg_latest,    // 右（基準）画像データ 右下原点
        plftimg_latest);   // 左（比較）画像データまたは視差エンコードデータ 右下原点

    // (2)
    fd_index = kISCIMAGEINFO_FRAMEDATA_PREVIOUS;

    int width_previous = isc_image_info_in->frame_data[fd_index].raw.width / 2;
    int height_previous = isc_image_info_in->frame_data[fd_index].raw.height;
    unsigned char* raw_image_previous = isc_image_info_in->frame_data[fd_index].raw.image;

    isc_image_info_out->frame_data[fd_index].p1.width = width_previous;
    isc_image_info_out->frame_data[fd_index].p1.height = height_previous;
    isc_image_info_out->frame_data[fd_index].p1.channel_count = 1;
    unsigned char* prgtimg_previous = isc_image_info_out->frame_data[fd_index].p1.image;

    isc_image_info_out->frame_data[fd_index].p2.width = width_previous;
    isc_image_info_out->frame_data[fd_index].p2.height = height_previous;
    isc_image_info_out->frame_data[fd_index].p2.channel_count = 1;
    unsigned char* plftimg_previous = isc_image_info_out->frame_data[fd_index].p2.image;

    ISCFrameDecoder::decodeFrameData(
        height_previous,    // 画像の高さ
        width_previous,     // 画像の幅
        raw_image_previous, // フレームデータ
        prgtimg_previous,   // 右（基準）画像データ 右下原点
        plftimg_previous);  // 左（比較）画像データまたは視差エンコードデータ 右下原点

    // (3)
    int decode_width = width_latest;
    int decode_height = height_latest;

    double gain_latest = isc_image_info_in->frame_data[kISCIMAGEINFO_FRAMEDATA_LATEST].gain;
    double exposure_latest = isc_image_info_in->frame_data[kISCIMAGEINFO_FRAMEDATA_LATEST].exposure;

    double gain_previous = isc_image_info_in->frame_data[kISCIMAGEINFO_FRAMEDATA_PREVIOUS].gain;
    double exposure_previous = isc_image_info_in->frame_data[kISCIMAGEINFO_FRAMEDATA_PREVIOUS].exposure;

    work_buffers_.buff_image[5].width = decode_width;
    work_buffers_.buff_image[5].height = decode_height;
    work_buffers_.buff_image[5].channel_count = 1;
    unsigned char* pdspimg = work_buffers_.buff_image[5].image;

    int* pblkhgt = &isc_block_disparity_data->blkhgt;
    int* pblkwdt = &isc_block_disparity_data->blkwdt;
    int* pmtchgt = &isc_block_disparity_data->mtchgt;
    int* pmtcwdt = &isc_block_disparity_data->mtcwdt;
    int* pblkofsx = &isc_block_disparity_data->dspofsx;
    int* pblkofsy = &isc_block_disparity_data->dspofsy;
    int* pdepth = &isc_block_disparity_data->depth;
    int* pshdwdt = &isc_block_disparity_data->shdwdt;

    float* ppxldsp = isc_block_disparity_data->ppxldsp;
    float* pblkdsp = isc_block_disparity_data->pblkdsp;
    int* pblkval = isc_block_disparity_data->pblkval;
    int* pblkcrst = isc_block_disparity_data->pblkcrst;
    unsigned char* pbldimg = isc_block_disparity_data->pbldimg;

    ISCFrameDecoder::getDoubleDisparityData(
        decode_height,      // 画像の高さ
        decode_width,       // 画像の幅
        prgtimg_latest,     // 現フレーム画像データ
        plftimg_latest,     // 現フレーム視差エンコードデータ
        exposure_latest,    // 現フレームシャッター露光値
        gain_latest,        // 現フレームシャッターゲイン値
        prgtimg_previous,   // 前フレーム画像データ
        plftimg_previous,   // 前フレーム視差エンコードデータ
        exposure_previous,  // 前フレームシャッター露光値
        gain_previous,      // 前フレームシャッターゲイン値
        pblkhgt,            // 視差ブロック高さ
        pblkwdt,            // 視差ブロック幅
        pmtchgt,            // マッチングブロック高さ
        pmtcwdt,            // マッチングブロック幅
        pblkofsx,           // 視差ブロック横オフセット
        pblkofsy,           // 視差ブロック縦オフセット 
        pdepth,             // マッチング探索幅 
        pshdwdt,            // 画像遮蔽幅
        pbldimg,            // 合成画像 右下基点
        pdspimg,            // 視差画像 右下基点 
        ppxldsp,            // 視差情報 右下基点 
        pblkdsp,            // ブロック視差情報 右下基点
        pblkval,            // ブロック視差値(1000倍サブピクセル精度の整数)
        pblkcrst);          // ブロックコントラスト

    isc_block_disparity_data->image_width = decode_width;
    isc_block_disparity_data->image_height = decode_height;

    return DPC_E_OK;
}

