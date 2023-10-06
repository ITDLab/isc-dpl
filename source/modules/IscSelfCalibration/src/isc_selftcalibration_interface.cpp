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
 * @file isc_selftcalibration_interface.cpp
 * @brief provides an interface for implementing selft calibration
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a interface for selft calibration.
 * @note 
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <shlwapi.h>
#include <imagehlp.h>
#include <Shlobj.h>
#include <process.h>
#include <mutex>
#include <functional>

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"
#include "isc_log.h"
#include "utility.h"

#include "isc_selftcalibration_interface.h"

#include "SelfCalibration.h"

#include "opencv2\opencv.hpp"

#pragma comment (lib, "shlwapi")
#pragma comment (lib, "Shell32")
#pragma comment (lib, "imagehlp")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world480d")
#else
#pragma comment (lib,"opencv_world480")
#endif

using GetCameraRegDataMethod = std::function<int(unsigned char*, unsigned char*, int, int)>;
using SetCameraRegDataMethod = std::function<int(unsigned char*, int)>;

GetCameraRegDataMethod GetCameraRegDataCbFunc = NULL;
SetCameraRegDataMethod SetCameraRegDataCbFunc = NULL;

/**
 * constructor
 *
 */
IscSelftCalibrationInterface::IscSelftCalibrationInterface():
    parameter_update_request_(false), isc_camera_control_configuration_(), max_image_width_(0), max_image_height_(0), parameter_file_name_(), selft_calibration_parameters_()
{

    selft_calibration_parameters_.mesh_parameter.imghgt = 0;
    selft_calibration_parameters_.mesh_parameter.imgwdt = 0;
    selft_calibration_parameters_.mesh_parameter.mshhgt = 25;   // VM:20
    selft_calibration_parameters_.mesh_parameter.mshwdt = 25;   // VM:20
    selft_calibration_parameters_.mesh_parameter.mshcntx = 480; // VM:376 
    selft_calibration_parameters_.mesh_parameter.mshcnty = 360; // VM:240
    selft_calibration_parameters_.mesh_parameter.mshnrgt = 20;  
    selft_calibration_parameters_.mesh_parameter.mshnlft = 20;  
    selft_calibration_parameters_.mesh_parameter.mshnupr = 20;
    selft_calibration_parameters_.mesh_parameter.mshnlwr = 20;  
    selft_calibration_parameters_.mesh_parameter.rgntop = 16;   // VM:16
    selft_calibration_parameters_.mesh_parameter.rgnbtm = 703;  // VM:460
    selft_calibration_parameters_.mesh_parameter.rgnlft = 16;   // VM:16
    selft_calibration_parameters_.mesh_parameter.rgnrgt = 1024; // VM:640
    selft_calibration_parameters_.mesh_parameter.srchhgt = 10;  // VM:10
    selft_calibration_parameters_.mesh_parameter.srchwdt = 180; // VM:100

    selft_calibration_parameters_.mesh_threshold.minbrgt = 40;      // VM:20
    selft_calibration_parameters_.mesh_threshold.maxbrgt = 160;     // VM:200
    selft_calibration_parameters_.mesh_threshold.mincrst = 1000;    // VM:1000
    selft_calibration_parameters_.mesh_threshold.minedgrt = 30.0;   // VM:50.0  
    selft_calibration_parameters_.mesh_threshold.maxdphgt = 5;      // VM:5
    selft_calibration_parameters_.mesh_threshold.maxdpwdt = 160;    // VM:90
    selft_calibration_parameters_.mesh_threshold.minmtcrt = 97.0;   // VM:96.0;

    selft_calibration_parameters_.operation_mode.grdcrct = 0;

    selft_calibration_parameters_.averaging_parameter.minmshn = 10;     // VM:10
    selft_calibration_parameters_.averaging_parameter.maxdifstd = 1.0;  // VM:0.5
    selft_calibration_parameters_.averaging_parameter.avefrmn = 50;     // VM:20

    selft_calibration_parameters_.criteria.calccnt = 100;
    selft_calibration_parameters_.criteria.crtrdiff = 0.1;
    selft_calibration_parameters_.criteria.crtrrot = 0.001;
    selft_calibration_parameters_.criteria.crtrdev = 0.25;
    selft_calibration_parameters_.criteria.crctrot = 0;  
    selft_calibration_parameters_.criteria.crctsv = 1;


}

/**
 * destructor
 *
 */
IscSelftCalibrationInterface::~IscSelftCalibrationInterface()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] isc_data_proc_module_configuration 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::Initialize(const IscCameraControlConfiguration* isc_camera_control_configuration, const int max_image_width, const int max_image_height)
{

    swprintf_s(isc_camera_control_configuration_.configuration_file_path, L"%s", isc_camera_control_configuration->configuration_file_path);
    swprintf_s(isc_camera_control_configuration_.log_file_path, L"%s", isc_camera_control_configuration->log_file_path);

    isc_camera_control_configuration_.log_level = isc_camera_control_configuration->log_level;
    isc_camera_control_configuration_.isc_camera_model = isc_camera_control_configuration->isc_camera_model;

    max_image_width_ = max_image_width;
    max_image_height_ = max_image_height;

    switch (isc_camera_control_configuration_.isc_camera_model) {
    case IscCameraModel::kVM:
        swprintf_s(parameter_file_name_, L"%s\\SelfCalibrationParameter_VM.ini", isc_camera_control_configuration_.configuration_file_path);
        break;

    case IscCameraModel::kXC:
        swprintf_s(parameter_file_name_, L"%s\\SelfCalibrationParameter_XC.ini", isc_camera_control_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4K:
        swprintf_s(parameter_file_name_, L"%s\\SelfCalibrationParameter_4K.ini", isc_camera_control_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4KA:
        swprintf_s(parameter_file_name_, L"%s\\SelfCalibrationParameter_4KA.ini", isc_camera_control_configuration_.configuration_file_path);
        break;

    case IscCameraModel::k4KJ:
        swprintf_s(parameter_file_name_, L"%s\\SelfCalibrationParameter_4KJ.ini", isc_camera_control_configuration_.configuration_file_path);
        break;

    default:
        swprintf_s(parameter_file_name_, L"%s\\SelfCalibrationParameter.ini", isc_camera_control_configuration_.configuration_file_path);
        break;
    }

    int ret = LoadParameterFromFile(parameter_file_name_, &selft_calibration_parameters_);
    if (ret != DPC_E_OK) {
        if (ret == DPCPROCESS_E_FILE_NOT_FOUND) {
            // Since the file does not exist, it will be created by default and continue
            SaveParameterToFile(parameter_file_name_, &selft_calibration_parameters_);
        }
        else {
            return ret;
        }
    }

    selft_calibration_parameters_.mesh_parameter.imghgt = max_image_height_;
    selft_calibration_parameters_.mesh_parameter.imgwdt = max_image_width_;

    // initialize SelfCalibration
    SelfCalibration::initialize(max_image_width_, max_image_height_);

    ret = SetParameterToSelftCalibrationModule(&selft_calibration_parameters_);
    if (ret != DPC_E_OK) {
        return ret;
    }

    // set call-back function
    SelfCalibration::SetCallbackFunc(GetCameraRegData, SetCameraRegData);

    return DPC_E_OK;
}

/**
 * パラメータをファイルから読み込みます.
 *
 * @param[in] file_name パラメータファイル名です
 * @param[out] selfcalibration_parameters　読み込んだパラメータです
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::LoadParameterFromFile(const wchar_t* file_name, SelefCalibrationparameter* selfcalibration_parameters)
{
    bool is_exists = PathFileExists(file_name) == TRUE ? true : false;
    if (!is_exists) {
        return DPCPROCESS_E_FILE_NOT_FOUND;
    }

    wchar_t returned_string[1024] = {};

    // MeshParameter
    GetPrivateProfileString(L"MeshParameter", L"imghgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.imghgt = _wtoi(returned_string);
    
    GetPrivateProfileString(L"MeshParameter", L"imgwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.imgwdt = _wtoi(returned_string);
    
    GetPrivateProfileString(L"MeshParameter", L"mshhgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.mshhgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"mshwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.mshwdt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"mshcntx", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.mshcntx = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"mshcnty", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.mshcnty = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"mshnrgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.mshnrgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"mshnlft", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.mshnlft = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"mshnupr", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.mshnupr = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"mshnlwr", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.mshnlwr = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"rgntop", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.rgntop = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"rgnbtm", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.rgnbtm = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"rgnlft", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.rgnlft = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"rgnrgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.rgnrgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"srchhgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.srchhgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshParameter", L"srchwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_parameter.srchwdt = _wtoi(returned_string);

    // MeshThreshold
    GetPrivateProfileString(L"MeshThreshold", L"minbrgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_threshold.minbrgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshThreshold", L"maxbrgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_threshold.maxbrgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshThreshold", L"mincrst", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_threshold.mincrst = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshThreshold", L"minedgrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_threshold.minedgrt = _wtof(returned_string);

    GetPrivateProfileString(L"MeshThreshold", L"maxdphgt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_threshold.maxdphgt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshThreshold", L"maxdpwdt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_threshold.maxdpwdt = _wtoi(returned_string);

    GetPrivateProfileString(L"MeshThreshold", L"minmtcrt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->mesh_threshold.minmtcrt = _wtof(returned_string);

    // OperationMode
    GetPrivateProfileString(L"OperationMode", L"grdcrct", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->operation_mode.grdcrct = _wtoi(returned_string);

    // AveragingParameter
    GetPrivateProfileString(L"AveragingParameter", L"minmshn", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->averaging_parameter.minmshn = _wtoi(returned_string);

    GetPrivateProfileString(L"AveragingParameter", L"maxdifstd", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->averaging_parameter.maxdifstd = _wtof(returned_string);

    GetPrivateProfileString(L"AveragingParameter", L"avefrmn", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->averaging_parameter.avefrmn = _wtoi(returned_string);

    // Criteria
    GetPrivateProfileString(L"Criteria", L"calccnt", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->criteria.calccnt = _wtoi(returned_string);

    GetPrivateProfileString(L"Criteria", L"crtrdiff", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->criteria.crtrdiff = _wtof(returned_string);

    GetPrivateProfileString(L"Criteria", L"crtrrot", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->criteria.crtrrot = _wtof(returned_string);

    GetPrivateProfileString(L"Criteria", L"crtrdev", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->criteria.crtrdev = _wtof(returned_string);

    GetPrivateProfileString(L"Criteria", L"crctrot", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->criteria.crctrot = _wtoi(returned_string);

    GetPrivateProfileString(L"Criteria", L"crctsv", L"0", returned_string, sizeof(returned_string) / sizeof(wchar_t), file_name);
    selfcalibration_parameters->criteria.crctsv = _wtoi(returned_string);


    return DPC_E_OK;
}

/**
 * パラメータをファイルへ書き込みます.
 *
 * @param[in] file_name パラメータファイル名です
 * @param[in] selfcalibration_parameters　書き込むパラメータです
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::SaveParameterToFile(const wchar_t* file_name, const SelefCalibrationparameter* selfcalibration_parameters)
{
    wchar_t string[1024] = {};

    // MeshParameter
    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.imghgt);
    WritePrivateProfileString(L"MeshParameter", L"imghgt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.imgwdt);
    WritePrivateProfileString(L"MeshParameter", L"imgwdt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.mshhgt);
    WritePrivateProfileString(L"MeshParameter", L"mshhgt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.mshwdt);
    WritePrivateProfileString(L"MeshParameter", L"mshwdt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.mshcntx);
    WritePrivateProfileString(L"MeshParameter", L"mshcntx", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.mshcnty);
    WritePrivateProfileString(L"MeshParameter", L"mshcnty", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.mshnrgt);
    WritePrivateProfileString(L"MeshParameter", L"mshnrgt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.mshnlft);
    WritePrivateProfileString(L"MeshParameter", L"mshnlft", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.mshnupr);
    WritePrivateProfileString(L"MeshParameter", L"mshnupr", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.mshnlwr);
    WritePrivateProfileString(L"MeshParameter", L"mshnlwr", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.rgntop);
    WritePrivateProfileString(L"MeshParameter", L"rgntop", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.rgnbtm);
    WritePrivateProfileString(L"MeshParameter", L"rgnbtm", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.rgnlft);
    WritePrivateProfileString(L"MeshParameter", L"rgnlft", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.rgnrgt);
    WritePrivateProfileString(L"MeshParameter", L"rgnrgt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.srchhgt);
    WritePrivateProfileString(L"MeshParameter", L"srchhgt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_parameter.srchwdt);
    WritePrivateProfileString(L"MeshParameter", L"srchwdt", string, file_name);

    // MeshThreshold
    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_threshold.minbrgt);
    WritePrivateProfileString(L"MeshThreshold", L"minbrgt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_threshold.maxbrgt);
    WritePrivateProfileString(L"MeshThreshold", L"maxbrgt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_threshold.mincrst);
    WritePrivateProfileString(L"MeshThreshold", L"mincrst", string, file_name);

    swprintf_s(string, L"%.3f", (double)selfcalibration_parameters->mesh_threshold.minedgrt);
    WritePrivateProfileString(L"MeshThreshold", L"minedgrt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_threshold.maxdphgt);
    WritePrivateProfileString(L"MeshThreshold", L"maxdphgt", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->mesh_threshold.maxdpwdt);
    WritePrivateProfileString(L"MeshThreshold", L"maxdpwdt", string, file_name);

    swprintf_s(string, L"%.3f", (double)selfcalibration_parameters->mesh_threshold.minmtcrt);
    WritePrivateProfileString(L"MeshThreshold", L"minmtcrt", string, file_name);

    // OperationMode
    swprintf_s(string, L"%d", (int)selfcalibration_parameters->operation_mode.grdcrct);
    WritePrivateProfileString(L"OperationMode", L"grdcrct", string, file_name);

    // AveragingParameter
    swprintf_s(string, L"%d", (int)selfcalibration_parameters->averaging_parameter.minmshn);
    WritePrivateProfileString(L"AveragingParameter", L"minmshn", string, file_name);

    swprintf_s(string, L"%.3f", selfcalibration_parameters->averaging_parameter.maxdifstd);
    WritePrivateProfileString(L"AveragingParameter", L"maxdifstd", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->averaging_parameter.avefrmn);
    WritePrivateProfileString(L"AveragingParameter", L"avefrmn", string, file_name);

    // Criteria
    swprintf_s(string, L"%d", (int)selfcalibration_parameters->criteria.calccnt);
    WritePrivateProfileString(L"Criteria", L"calccnt", string, file_name);

    swprintf_s(string, L"%.3f", (double)selfcalibration_parameters->criteria.crtrdiff);
    WritePrivateProfileString(L"Criteria", L"crtrdiff", string, file_name);

    swprintf_s(string, L"%.3f", (double)selfcalibration_parameters->criteria.crtrrot);
    WritePrivateProfileString(L"Criteria", L"crtrrot", string, file_name);

    swprintf_s(string, L"%.3f", (double)selfcalibration_parameters->criteria.crtrdev);
    WritePrivateProfileString(L"Criteria", L"crtrdev", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->criteria.crctrot);
    WritePrivateProfileString(L"Criteria", L"crctrot", string, file_name);

    swprintf_s(string, L"%d", (int)selfcalibration_parameters->criteria.crctsv);
    WritePrivateProfileString(L"Criteria", L"crctsv", string, file_name);

    return DPC_E_OK;
}

/**
 * パラメータをSelfCalibration実装へ設定します.
 *
 * @param[in] selfcalibration_parameters 設定するパラメータファイルです
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::SetParameterToSelftCalibrationModule(const SelefCalibrationparameter* selfcalibration_parameters)
{

    SelfCalibration::setMeshParameter(
        selfcalibration_parameters->mesh_parameter.imghgt,
        selfcalibration_parameters->mesh_parameter.imgwdt,
        selfcalibration_parameters->mesh_parameter.mshhgt,
        selfcalibration_parameters->mesh_parameter.mshwdt,
        selfcalibration_parameters->mesh_parameter.mshcntx,
        selfcalibration_parameters->mesh_parameter.mshcnty,
        selfcalibration_parameters->mesh_parameter.mshnrgt,
        selfcalibration_parameters->mesh_parameter.mshnlft,
        selfcalibration_parameters->mesh_parameter.mshnupr,
        selfcalibration_parameters->mesh_parameter.mshnlwr,
        selfcalibration_parameters->mesh_parameter.rgntop,
        selfcalibration_parameters->mesh_parameter.rgnbtm,
        selfcalibration_parameters->mesh_parameter.rgnlft,
        selfcalibration_parameters->mesh_parameter.rgnrgt,
        selfcalibration_parameters->mesh_parameter.srchhgt,
        selfcalibration_parameters->mesh_parameter.srchwdt
    );

    SelfCalibration::setMeshThreshold(
        selfcalibration_parameters->mesh_threshold.minbrgt,
        selfcalibration_parameters->mesh_threshold.maxbrgt,
        selfcalibration_parameters->mesh_threshold.mincrst,
        selfcalibration_parameters->mesh_threshold.minedgrt,
        selfcalibration_parameters->mesh_threshold.maxdphgt,
        selfcalibration_parameters->mesh_threshold.maxdpwdt,
        selfcalibration_parameters->mesh_threshold.minmtcrt
    );

    SelfCalibration::setOperationMode(selfcalibration_parameters->operation_mode.grdcrct);

    SelfCalibration::setAveragingParameter(
        selfcalibration_parameters->averaging_parameter.minmshn,
        selfcalibration_parameters->averaging_parameter.maxdifstd,
        selfcalibration_parameters->averaging_parameter.avefrmn
    );

    SelfCalibration::setCriteria(
        selfcalibration_parameters->criteria.calccnt,
        selfcalibration_parameters->criteria.crtrdiff,
        selfcalibration_parameters->criteria.crtrrot,
        selfcalibration_parameters->criteria.crtrdev,
        selfcalibration_parameters->criteria.crctrot,
        selfcalibration_parameters->criteria.crctsv
    );

    return DPC_E_OK;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::Terminate()
{
    SelfCalibration::finalize();

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
int IscSelftCalibrationInterface::GetParameterFileName(wchar_t* file_name, const int max_length)
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
int IscSelftCalibrationInterface::ReloadParameterFromFile(const wchar_t* file_name, const bool is_valid)
{

    wchar_t parameter_file_name[_MAX_PATH] = {};
    swprintf_s(parameter_file_name, L"%s", file_name);

    int ret = LoadParameterFromFile(parameter_file_name, &selft_calibration_parameters_);
    if (ret != DPC_E_OK) {
        return ret;
    }

    ret = SetParameterToSelftCalibrationModule(&selft_calibration_parameters_);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 自動調整を開始します.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::StartSelfCalibration()
{

    SelfCalibration::start();

    return DPC_E_OK;
}

/**
 * 自動調整を停止します.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::StoptSelfCalibration()
{

    SelfCalibration::stop();

    return DPC_E_OK;
}

/**
 * 左右画像を平行にする.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::ParallelizeSelfCalibration(IscImageInfo* isc_image_info)
{

    if (isc_image_info->grab != IscGrabMode::kCorrect) {
        return DPCPROCESS_E_INVALID_MODE;
    }

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    if ((isc_image_info->frame_data[fd_index].p1.width == 0) || (isc_image_info->frame_data[fd_index].p1.height == 0)) {
        return DPC_E_OK;
    }

    if ((isc_image_info->frame_data[fd_index].p2.width == 0) || (isc_image_info->frame_data[fd_index].p2.height == 0)) {
        return DPC_E_OK;
    }

    if (parameter_update_request_) {
        int ret = SetParameterToSelftCalibrationModule(&selft_calibration_parameters_);
        parameter_update_request_ = false;
    }

    unsigned char* pimgref = isc_image_info->frame_data[fd_index].p1.image;
    unsigned char* pimgcmp = isc_image_info->frame_data[fd_index].p2.image;

    SelfCalibration::parallelize(pimgref, pimgcmp);

    return DPC_E_OK;
}

/// <summary>
/// Call Back関数を設定する
/// </summary>
/// <param name="func_get_camera_reg">読み込み関数</param>
/// <param name="func_set_camera_reg">書き込み関数</param>
/// <returns>処理結果を返す</returns>
void IscSelftCalibrationInterface::SetCallbackFunc(std::function<int(unsigned char*, unsigned char*, int, int)> func_get_camera_reg, std::function<int(unsigned char*, int)> func_set_camera_reg)
{
    GetCameraRegDataCbFunc = func_get_camera_reg;
    SetCameraRegDataCbFunc = func_set_camera_reg;

    return;
}

/**
 * CallBack用　レジスタアクセス関数
 *
 * @param[in] wbuf バッファー構造体
 * @param[in] rbuf バッファー構造体
 * @param[in] write_size 書き込み数
 * @param[in] read_size 読み込み数
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::GetCameraRegData(unsigned char* wbuf, unsigned char* rbuf, int write_size, int read_size)
{
    int ret = GetCameraRegDataCbFunc(wbuf, rbuf, write_size, read_size);
    
    return ret;
}

/**
 * CallBack用　レジスタアクセス関数
 *
 * @param[in] wbuf バッファー構造体
 * @param[in] write_size 書き込み数
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscSelftCalibrationInterface::SetCameraRegData(unsigned char* wbuf, int write_size)
{
    int ret = SetCameraRegDataCbFunc(wbuf, write_size);

    return ret;
}
