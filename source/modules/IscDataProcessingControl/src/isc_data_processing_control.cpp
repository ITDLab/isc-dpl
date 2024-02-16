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
 * @file isc_data_processing_control.cpp
 * @brief data processing control class for ISC_DPL
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a function for control data prosseing modules.
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <stdint.h>
#include <process.h>

#include "isc_dpl_error_def.h"
#include "isc_camera_def.h"
#include "isc_dataprocessing_def.h"

#include "utility.h"
#include "isc_image_info_ring_buffer.h"
#include "isc_dataproc_resultdata_ring_buffer.h"
#include "isc_framedecoder_interface.h"
#include "isc_stereomatching_interface.h"
#include "isc_disparityfilter_interface.h"

#include "isc_data_processing_control.h"

#include "opencv2\opencv.hpp"
#include "opencv2\core\ocl.hpp"

#pragma comment (lib, "IscFrameDecoder")
#pragma comment (lib, "IscStereoMatching")
#pragma comment (lib, "IscDisparityFilter")


#ifdef _DEBUG
#pragma comment (lib,"opencv_world480d")
#else
#pragma comment (lib,"opencv_world480")
#endif


constexpr int kISC_DPL_MODULE_COUNT = 3;
const wchar_t kISC_DPL_MODULE_NAME[kISC_DPL_MODULE_COUNT][32] = { L"S/W Stereo Matching", L"Frame Decoder", L"Disparity Filter" };

/**
 * constructor
 *
 */
IscDataProcessingControl::IscDataProcessingControl():
    measure_time_(nullptr),
	isc_data_proc_module_configuration_(),
    isc_grab_start_mode_(),
    isc_dataproc_start_mode_(),
    isc_image_info_ring_buffer_(nullptr),
    isc_dataproc_resultdata_ring_buffer_(nullptr),
    isc_block_disparity_data_(),
    isc_frame_decoder_(nullptr),
    isc_stereo_matching_(nullptr),
    isc_disparity_filter_(nullptr),
	thread_control_dataproc_(),
	handle_semaphore_dataproc_(NULL),
	thread_handle_dataproc_(NULL),
	threads_critical_dataproc_()
{
    measure_time_ = new UtilityMeasureTime;
}

/**
 * destructor
 *
 */
IscDataProcessingControl::~IscDataProcessingControl()
{
    delete measure_time_;
    measure_time_ = nullptr;
}

/**
 * クラスを初期化します.
 *
 * @param[in] isc_data_proc_module_configuration 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::Initialize(IscDataProcModuleConfiguration* isc_data_proc_module_configuration)
{

	swprintf_s(isc_data_proc_module_configuration_.configuration_file_path, L"%s", isc_data_proc_module_configuration->configuration_file_path);
	swprintf_s(isc_data_proc_module_configuration_.log_file_path, L"%s", isc_data_proc_module_configuration->log_file_path);
	
	isc_data_proc_module_configuration_.log_level = isc_data_proc_module_configuration->log_level;
	isc_data_proc_module_configuration_.isc_camera_model = isc_data_proc_module_configuration->isc_camera_model;
	isc_data_proc_module_configuration_.max_image_width = isc_data_proc_module_configuration->max_image_width;
	isc_data_proc_module_configuration_.max_image_height = isc_data_proc_module_configuration->max_image_height;

    isc_data_proc_module_configuration_.enabled_data_proc_module = isc_data_proc_module_configuration->enabled_data_proc_module;

    isc_data_proc_module_configuration_.max_buffer_count = isc_data_proc_module_configuration->max_buffer_count;

    // modules
    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {
        isc_frame_decoder_ = new IscFramedecoderInterface;
        isc_frame_decoder_->Initialize(&isc_data_proc_module_configuration_);

        isc_stereo_matching_ = new IscStereoMatchingInterface;
        isc_stereo_matching_->Initialize(&isc_data_proc_module_configuration_);

        isc_disparity_filter_ = new IscDisparityFilterInterface;
        isc_disparity_filter_->Initialize(&isc_data_proc_module_configuration_);
    }

    // get temporary buffer
    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {
        InitializeIscBlockDisparityData(&isc_block_disparity_data_);
    }

    // setup openCL
    if (cv::ocl::haveOpenCL()) {
        cv::ocl::setUseOpenCL(true);
    }

    // get Buffer
    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {
        isc_image_info_ring_buffer_ = new IscImageInfoRingBuffer;
        const int max_buffer_count = isc_data_proc_module_configuration_.max_buffer_count;
        isc_image_info_ring_buffer_->Initialize(true, true, max_buffer_count, isc_data_proc_module_configuration_.max_image_width, isc_data_proc_module_configuration_.max_image_height);
        isc_image_info_ring_buffer_->Clear();

        isc_dataproc_resultdata_ring_buffer_ = new IscDataprocResultdataRingBuffer;
        isc_dataproc_resultdata_ring_buffer_->Initialize(true, true, max_buffer_count, isc_data_proc_module_configuration_.max_image_width, isc_data_proc_module_configuration_.max_image_height, 3);
        isc_dataproc_resultdata_ring_buffer_->Clear();
    }

    // create thread for data processing
    thread_control_dataproc_.terminate_request = 0;
    thread_control_dataproc_.terminate_done = 0;
    thread_control_dataproc_.end_code = 0;
    thread_control_dataproc_.stop_request = false;

    char semaphoreName[64] = {};
    sprintf_s(semaphoreName, "THREAD_SEMAPHORENAME_ISCDP_%d", 0);
    handle_semaphore_dataproc_ = CreateSemaphoreA(NULL, 0, 1, semaphoreName);
    if (handle_semaphore_dataproc_ == NULL) {
        // fail

        return DPCCONTROL_E_INVALID_DEVICEHANDLE;
    }
    InitializeCriticalSection(&threads_critical_dataproc_);

    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {
        // it enabled process

        if ((thread_handle_dataproc_ = (HANDLE)_beginthreadex(0, 0, ControlThreadDataProc, (void*)this, 0, 0)) == 0) {
            // fail
            return DPCCONTROL_E_INVALID_DEVICEHANDLE;
        }

        // THREAD_PRIORITY_TIME_CRITICAL
        // THREAD_PRIORITY_HIGHEST +2
        // THREAD_PRIORITY_ABOVE_NORMAL +1
        // THREAD_PRIORITY_NORMAL  +0
        // THREAD_PRIORITY_BELOW_NORMAL -1
        SetThreadPriority(thread_handle_dataproc_, THREAD_PRIORITY_NORMAL);
    }

    measure_time_->Init();

	return DPC_E_OK;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::Terminate()
{
    // close thread procedure
    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {
        thread_control_dataproc_.stop_request = true;
        thread_control_dataproc_.terminate_done = 0;
        thread_control_dataproc_.end_code = 0;
        thread_control_dataproc_.terminate_request = 1;

        // release any resources
        ReleaseSemaphore(handle_semaphore_dataproc_, 1, NULL);

        int count = 0;
        while (thread_control_dataproc_.terminate_done == 0) {
            if (count > 100) {
                break;
            }
            count++;
            Sleep(10L);
        }

        if (thread_handle_dataproc_ != NULL) {
            CloseHandle(thread_handle_dataproc_);
            thread_handle_dataproc_ = NULL;
        }
    }

    if (handle_semaphore_dataproc_ != NULL) {
        CloseHandle(handle_semaphore_dataproc_);
        handle_semaphore_dataproc_ = NULL;
    }
    DeleteCriticalSection(&threads_critical_dataproc_);

    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {

        if (isc_disparity_filter_ != nullptr) {
            isc_disparity_filter_->Terminate();
            delete isc_disparity_filter_;
            isc_disparity_filter_ = nullptr;
        }

        if (isc_stereo_matching_ != nullptr) {
            isc_stereo_matching_->Terminate();
            delete isc_stereo_matching_;
            isc_stereo_matching_ = nullptr;
        }

        if (isc_frame_decoder_ != nullptr) {
            isc_frame_decoder_->Terminate();
            delete isc_frame_decoder_;
            isc_frame_decoder_ = nullptr;
        }
    }

    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {

        if (isc_dataproc_resultdata_ring_buffer_ != nullptr) {
            isc_dataproc_resultdata_ring_buffer_->Terminate();
            delete isc_dataproc_resultdata_ring_buffer_;
            isc_dataproc_resultdata_ring_buffer_ = nullptr;
        }

        if (isc_image_info_ring_buffer_ != nullptr) {
            isc_image_info_ring_buffer_->Terminate();
            delete isc_image_info_ring_buffer_;
            isc_image_info_ring_buffer_ = nullptr;
        }
    }

    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {
        ReleaeIscIscBlockDisparityData(&isc_block_disparity_data_);
    }

    return DPC_E_OK;
}

/**
 * 処理を開始するための準備を行います.
 *
 * @param[in] isc_grab_start_mode 動作モードを設定します
 * @param[in] isc_dataproc_start_mode 動作モードを設定します
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::Start(const IscGrabStartMode* isc_grab_start_mode, const IscDataProcStartMode* isc_dataproc_start_mode)
{

    isc_grab_start_mode_.isc_play_mode = isc_grab_start_mode->isc_play_mode;

    isc_dataproc_start_mode_.enabled_stereo_matching = isc_dataproc_start_mode->enabled_stereo_matching;
    isc_dataproc_start_mode_.enabled_frame_decoder = isc_dataproc_start_mode->enabled_frame_decoder;
    isc_dataproc_start_mode_.enabled_disparity_filter = isc_dataproc_start_mode->enabled_disparity_filter;

    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {
        isc_image_info_ring_buffer_->Clear();
        isc_dataproc_resultdata_ring_buffer_->Clear();
    }

    measure_time_->Init();

    return DPC_E_OK;
}

/**
 * 処理の停止処理を行います(現在はなにも行いません).
 *
 * @param[in] isc_dataproc_start_mode 動作モードを設定します
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::Stop()
{

    return DPC_E_OK;
}

/**
 * 実装されたモジュールの数を取得します.
 *
 * @param[out] total_count 動作モードを設定します
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::GetTotalModuleCount(int* total_count)
{
    *total_count = kISC_DPL_MODULE_COUNT;

    return DPC_E_OK;
}

/**
 * 指定されたIndexのモジュール名を取得します.
 *
 * @param[in] module_index 取得するモジュールのIndex
 * @param[out] module_name モジュール名
 * @param[in] max_length module_nameバッファーの最大文字数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::GetModuleNameByIndex(const int module_index, wchar_t* module_name, int max_length)
{

    if (module_index >= kISC_DPL_MODULE_COUNT) {
        return DPCCONTROL_E_INVALID_PARAMETER;
    }

    swprintf_s(module_name, max_length, L"%s", kISC_DPL_MODULE_NAME[module_index]);

    return DPC_E_OK;
}

/**
 * 指定されたIndexのモジュールのパラメータを取得します.
 *
 * @param[in] module_index 取得するモジュールのIndex
 * @param[out] isc_data_proc_module_parameter モジュールパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::GetParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter)
{
    
    switch (module_index) {
    case 0:
        if (isc_stereo_matching_ != nullptr) {
            isc_stereo_matching_->GetParameter(isc_data_proc_module_parameter);
        }
        break;

    case 1:
        if (isc_frame_decoder_ != nullptr) {
            isc_frame_decoder_->GetParameter(isc_data_proc_module_parameter);
        }
        break;

    case 2:
        if (isc_disparity_filter_ != nullptr) {
            isc_disparity_filter_->GetParameter(isc_data_proc_module_parameter);
        }
        break;
    }

    return DPC_E_OK;
}

/**
 * 指定されたIndexのモジュールへパラメータを設定します.
 *
 * @param[in] module_index 設定するモジュールのIndex
 * @param[out] isc_data_proc_module_parameter モジュールパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::SetParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file)
{
    switch (module_index) {
    case 0:
        if (isc_stereo_matching_ != nullptr) {
            isc_stereo_matching_->SetParameter(isc_data_proc_module_parameter, is_update_file);
        }
        break;

    case 1:
        if (isc_frame_decoder_ != nullptr) {
            isc_frame_decoder_->SetParameter(isc_data_proc_module_parameter, is_update_file);
        }
        break;

    case 2:
        if (isc_disparity_filter_ != nullptr) {
            isc_disparity_filter_->SetParameter(isc_data_proc_module_parameter, is_update_file);
        }
        break;
    }

    return DPC_E_OK;
}

/**
 * 指定されたIndexのモジュールのパラメータファイル名を取得します.
 *
 * @param[in] module_index 取得するモジュールのIndex
 * @param[out] file_name ファイル名
 * @param[in] max_length file_nameバッファーの最大文字数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::GetParameterFileName(const int module_index, wchar_t* file_name, const int max_length)
{
    switch (module_index) {
    case 0:
        if (isc_stereo_matching_ != nullptr) {
            isc_stereo_matching_->GetParameterFileName(file_name, max_length);
        }
        break;

    case 1:
        if (isc_frame_decoder_ != nullptr) {
            isc_frame_decoder_->GetParameterFileName(file_name, max_length);
        }
        break;

    case 2:
        if (isc_disparity_filter_ != nullptr) {
            isc_disparity_filter_->GetParameterFileName(file_name, max_length);
        }
        break;
    }

    return DPC_E_OK;
}

/**
 * 指定されたIndexのモジュールのパラメータファイルをファイルからリロードします.
 *
 * @param[in] module_index 取得するモジュールのIndex
 * @param[out] file_name ファイル名
 * @param[in] is_valid (未使用)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::ReloadParameterFromFile(const int module_index, const wchar_t* file_name, const bool is_valid)
{
    switch (module_index) {
    case 0:
        if (isc_stereo_matching_ != nullptr) {
            isc_stereo_matching_->ReloadParameterFromFile(file_name, is_valid);
        }
        break;

    case 1:
        if (isc_frame_decoder_ != nullptr) {
            isc_frame_decoder_->ReloadParameterFromFile(file_name, is_valid);
        }
        break;

    case 2:
        if (isc_disparity_filter_ != nullptr) {
            isc_disparity_filter_->ReloadParameterFromFile(file_name, is_valid);
        }
        break;
    }
    
    return DPC_E_OK;
}

/**
 * モジュールの処理結果バッファーを初期化します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::InitializeIscBlockDisparityData(IscBlockDisparityData* isc_stereo_disparity_data)
{

    size_t frame_size = isc_data_proc_module_configuration_.max_image_height * isc_data_proc_module_configuration_.max_image_width;

    isc_stereo_disparity_data->image_width = 0;
    isc_stereo_disparity_data->image_width = 0;

    isc_stereo_disparity_data->prgtimg = 0;
    isc_stereo_disparity_data->blkhgt = 0;
    isc_stereo_disparity_data->blkwdt = 0;

    isc_stereo_disparity_data->mtchgt = 0;
    isc_stereo_disparity_data->mtcwdt = 0;

    isc_stereo_disparity_data->dspofsx = 0;
    isc_stereo_disparity_data->dspofsy = 0;

    isc_stereo_disparity_data->depth = 0;
    isc_stereo_disparity_data->shdwdt = 0;

    isc_stereo_disparity_data->pblkval = new int[frame_size];
    isc_stereo_disparity_data->pblkcrst = new int[frame_size];

    isc_stereo_disparity_data->pdspimg = new unsigned char[frame_size];
    isc_stereo_disparity_data->ppxldsp = new float[frame_size];
    isc_stereo_disparity_data->pblkdsp = new float[frame_size];

    isc_stereo_disparity_data->pbldimg = new unsigned char[frame_size];

    return DPC_E_OK;
}

/**
 * モジュールの処理結果バッファーを解放します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::ReleaeIscIscBlockDisparityData(IscBlockDisparityData* isc_stereo_disparity_data)
{

    isc_stereo_disparity_data->image_width = 0;
    isc_stereo_disparity_data->image_width = 0;

    isc_stereo_disparity_data->prgtimg = 0;
    isc_stereo_disparity_data->blkhgt = 0;
    isc_stereo_disparity_data->blkwdt = 0;

    isc_stereo_disparity_data->mtchgt = 0;
    isc_stereo_disparity_data->mtcwdt = 0;

    isc_stereo_disparity_data->dspofsx = 0;
    isc_stereo_disparity_data->dspofsy = 0;

    isc_stereo_disparity_data->depth = 0;
    isc_stereo_disparity_data->shdwdt = 0;

    delete[] isc_stereo_disparity_data->pblkval;
    isc_stereo_disparity_data->pblkval = nullptr;
    delete[] isc_stereo_disparity_data->pblkcrst;
    isc_stereo_disparity_data->pblkcrst = nullptr;

    delete[] isc_stereo_disparity_data->pdspimg;
    isc_stereo_disparity_data->pdspimg = nullptr;
    delete[] isc_stereo_disparity_data->ppxldsp;
    isc_stereo_disparity_data->ppxldsp = nullptr;
    delete[] isc_stereo_disparity_data->pblkdsp;
    isc_stereo_disparity_data->pblkdsp = nullptr;

    delete[] isc_stereo_disparity_data->pbldimg;
    isc_stereo_disparity_data->pbldimg = nullptr;

    return DPC_E_OK;
}

/**
 * モジュールの処理結果バッファーを初期化します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::InitializeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{

    isc_data_proc_result_data->number_of_modules_processed = 0;
    isc_data_proc_result_data->maximum_number_of_modules = 4;
    isc_data_proc_result_data->maximum_number_of_modulename = 32;

    isc_data_proc_result_data->status.error_code = 0;
    isc_data_proc_result_data->status.proc_tact_time = 0;

    for (int i = 0; i < isc_data_proc_result_data->maximum_number_of_modules; i++) {
        sprintf_s(isc_data_proc_result_data->module_status[i].module_names, "\n");
        isc_data_proc_result_data->module_status[i].error_code = 0;
        isc_data_proc_result_data->module_status[i].processing_time = 0;
    }

    IscImageInfo* isc_image_info = &isc_data_proc_result_data->isc_image_info;
    int width = isc_data_proc_module_configuration_.max_image_width;
    int height = isc_data_proc_module_configuration_.max_image_height;

    isc_image_info->grab = IscGrabMode::kParallax;
    isc_image_info->color_grab_mode = IscGrabColorMode::kColorOFF;
    isc_image_info->shutter_mode = IscShutterMode::kManualShutter;
    isc_image_info->camera_specific_parameter.d_inf = 0;
    isc_image_info->camera_specific_parameter.bf = 0;
    isc_image_info->camera_specific_parameter.base_length = 0;
    isc_image_info->camera_specific_parameter.dz = 0;

    for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
        isc_image_info->frame_data[i].frameNo = -1;
        isc_image_info->frame_data[i].gain = -1;
        isc_image_info->frame_data[i].exposure = -1;

        isc_image_info->frame_data[i].camera_status.error_code = 0;
        isc_image_info->frame_data[i].camera_status.data_receive_tact_time = 0;

        isc_image_info->frame_data[i].p1.width = 0;
        isc_image_info->frame_data[i].p1.height = 0;
        isc_image_info->frame_data[i].p1.channel_count = 0;
        isc_image_info->frame_data[i].p1.image = new unsigned char[width * height];

        isc_image_info->frame_data[i].p2.width = 0;
        isc_image_info->frame_data[i].p2.height = 0;
        isc_image_info->frame_data[i].p2.channel_count = 0;
        isc_image_info->frame_data[i].p2.image = new unsigned char[width * height];

        isc_image_info->frame_data[i].color.width = 0;
        isc_image_info->frame_data[i].color.height = 0;
        isc_image_info->frame_data[i].color.channel_count = 0;
        isc_image_info->frame_data[i].color.image = new unsigned char[width * height * 3];

        isc_image_info->frame_data[i].depth.width = 0;
        isc_image_info->frame_data[i].depth.height = 0;
        isc_image_info->frame_data[i].depth.image = new float[width * height];

        isc_image_info->frame_data[i].raw.width = 0;
        isc_image_info->frame_data[i].raw.height = 0;
        isc_image_info->frame_data[i].raw.channel_count = 0;
        isc_image_info->frame_data[i].raw.image = new unsigned char[width * height * 2];

        isc_image_info->frame_data[i].raw_color.width = 0;
        isc_image_info->frame_data[i].raw_color.height = 0;
        isc_image_info->frame_data[i].raw_color.channel_count = 0;
        isc_image_info->frame_data[i].raw_color.image = new unsigned char[width * height * 2];

        size_t image_size = width * height;
        memset(isc_image_info->frame_data[i].p1.image, 0, image_size);
        memset(isc_image_info->frame_data[i].p2.image, 0, image_size);

        image_size = width * height * sizeof(float);
        memset(isc_image_info->frame_data[i].depth.image, 0, image_size);

        image_size = width * height * 2;
        memset(isc_image_info->frame_data[i].raw.image, 0, image_size);

        image_size = width * height * 2;
        memset(isc_image_info->frame_data[i].raw_color.image, 0, image_size);
    }

    return DPC_E_OK;
}

/**
 * モジュールの処理結果バッファーを解放します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::ReleaeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{

    isc_data_proc_result_data->number_of_modules_processed = 0;
    isc_data_proc_result_data->maximum_number_of_modules = 4;
    isc_data_proc_result_data->maximum_number_of_modulename = 32;

    isc_data_proc_result_data->status.error_code = 0;
    isc_data_proc_result_data->status.proc_tact_time = 0;

    for (int i = 0; i < isc_data_proc_result_data->maximum_number_of_modules; i++) {
        sprintf_s(isc_data_proc_result_data->module_status[i].module_names, "\n");
        isc_data_proc_result_data->module_status[i].error_code = 0;
        isc_data_proc_result_data->module_status[i].processing_time = 0;
    }

    IscImageInfo* isc_image_info = &isc_data_proc_result_data->isc_image_info;

    isc_image_info->grab = IscGrabMode::kParallax;
    isc_image_info->color_grab_mode = IscGrabColorMode::kColorOFF;
    isc_image_info->shutter_mode = IscShutterMode::kManualShutter;
    isc_image_info->camera_specific_parameter.d_inf = 0;
    isc_image_info->camera_specific_parameter.bf = 0;
    isc_image_info->camera_specific_parameter.base_length = 0;
    isc_image_info->camera_specific_parameter.dz = 0;

    for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
        isc_image_info->frame_data[i].frameNo = -1;
        isc_image_info->frame_data[i].gain = -1;
        isc_image_info->frame_data[i].exposure = -1;

        isc_image_info->frame_data[i].camera_status.error_code = 0;
        isc_image_info->frame_data[i].camera_status.data_receive_tact_time = 0;

        isc_image_info->frame_data[i].p1.width = 0;
        isc_image_info->frame_data[i].p1.height = 0;
        isc_image_info->frame_data[i].p1.channel_count = 0;
        delete[] isc_image_info->frame_data[i].p1.image;
        isc_image_info->frame_data[i].p1.image = nullptr;

        isc_image_info->frame_data[i].p2.width = 0;
        isc_image_info->frame_data[i].p2.height = 0;
        isc_image_info->frame_data[i].p2.channel_count = 0;
        delete[] isc_image_info->frame_data[i].p2.image;
        isc_image_info->frame_data[i].p2.image = nullptr;

        isc_image_info->frame_data[i].color.width = 0;
        isc_image_info->frame_data[i].color.height = 0;
        isc_image_info->frame_data[i].color.channel_count = 0;
        delete[] isc_image_info->frame_data[i].color.image;
        isc_image_info->frame_data[i].color.image = nullptr;

        isc_image_info->frame_data[i].depth.width = 0;
        isc_image_info->frame_data[i].depth.height = 0;
        delete[] isc_image_info->frame_data[i].depth.image;
        isc_image_info->frame_data[i].depth.image = nullptr;

        isc_image_info->frame_data[i].raw.width = 0;
        isc_image_info->frame_data[i].raw.height = 0;
        isc_image_info->frame_data[i].raw.channel_count = 0;
        delete[] isc_image_info->frame_data[i].raw.image;
        isc_image_info->frame_data[i].raw.image = nullptr;

        isc_image_info->frame_data[i].raw_color.width = 0;
        isc_image_info->frame_data[i].raw_color.height = 0;
        isc_image_info->frame_data[i].raw_color.channel_count = 0;
        delete[] isc_image_info->frame_data[i].raw_color.image;
        isc_image_info->frame_data[i].raw_color.image = nullptr;
    }

    return DPC_E_OK;
}

/**
 * モジュールの処理結果バッファーを初期化します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::ClearIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{

    isc_data_proc_result_data->number_of_modules_processed = 0;

    isc_data_proc_result_data->status.error_code = 0;
    isc_data_proc_result_data->status.proc_tact_time = 0;

    for (int i = 0; i < isc_data_proc_result_data->maximum_number_of_modules; i++) {
        sprintf_s(isc_data_proc_result_data->module_status[i].module_names, "\n");
        isc_data_proc_result_data->module_status[i].error_code = 0;
        isc_data_proc_result_data->module_status[i].processing_time = 0;
    }

    IscImageInfo* isc_image_info = &isc_data_proc_result_data->isc_image_info;

    isc_image_info->grab = IscGrabMode::kParallax;
    isc_image_info->color_grab_mode = IscGrabColorMode::kColorOFF;
    isc_image_info->shutter_mode = IscShutterMode::kManualShutter;
    isc_image_info->camera_specific_parameter.d_inf = 0;
    isc_image_info->camera_specific_parameter.bf = 0;
    isc_image_info->camera_specific_parameter.base_length = 0;
    isc_image_info->camera_specific_parameter.dz = 0;

    for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
        isc_image_info->frame_data[i].frameNo = -1;
        isc_image_info->frame_data[i].gain = -1;
        isc_image_info->frame_data[i].exposure = -1;

        isc_image_info->frame_data[i].camera_status.error_code = 0;
        isc_image_info->frame_data[i].camera_status.data_receive_tact_time = 0;

        isc_image_info->frame_data[i].p1.width = 0;
        isc_image_info->frame_data[i].p1.height = 0;
        isc_image_info->frame_data[i].p1.channel_count = 0;

        isc_image_info->frame_data[i].p2.width = 0;
        isc_image_info->frame_data[i].p2.height = 0;
        isc_image_info->frame_data[i].p2.channel_count = 0;

        isc_image_info->frame_data[i].color.width = 0;
        isc_image_info->frame_data[i].color.height = 0;
        isc_image_info->frame_data[i].color.channel_count = 0;

        isc_image_info->frame_data[i].depth.width = 0;
        isc_image_info->frame_data[i].depth.height = 0;

        isc_image_info->frame_data[i].raw.width = 0;
        isc_image_info->frame_data[i].raw.height = 0;
        isc_image_info->frame_data[i].raw.channel_count = 0;

        isc_image_info->frame_data[i].raw_color.width = 0;
        isc_image_info->frame_data[i].raw_color.height = 0;
        isc_image_info->frame_data[i].raw_color.channel_count = 0;
    }

    return DPC_E_OK;
}

/**
 * モジュールの処理結果を取得します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::GetDataProcModuleData(IscDataProcResultData* isc_data_proc_result_data)
{
 
    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {

        IscDataprocResultdataRingBuffer::BufferData* dataproc_result_buffer_data = nullptr;
        ULONGLONG time = 0;
        int get_index = isc_dataproc_resultdata_ring_buffer_->GetGetBuffer(&dataproc_result_buffer_data, &time);

        if (get_index >= 0) {

            isc_data_proc_result_data->number_of_modules_processed = dataproc_result_buffer_data->isc_dataproc_resultdata.number_of_modules_processed;

            isc_data_proc_result_data->status.error_code = dataproc_result_buffer_data->isc_dataproc_resultdata.status.error_code;
            isc_data_proc_result_data->status.proc_tact_time = dataproc_result_buffer_data->isc_dataproc_resultdata.status.proc_tact_time;

            for (int i = 0; i < isc_data_proc_result_data->number_of_modules_processed; i++) {
                sprintf_s(isc_data_proc_result_data->module_status[i].module_names, "%s", dataproc_result_buffer_data->isc_dataproc_resultdata.module_status[i].module_names);
                isc_data_proc_result_data->module_status[i].error_code = dataproc_result_buffer_data->isc_dataproc_resultdata.module_status[i].error_code;
                isc_data_proc_result_data->module_status[i].processing_time = dataproc_result_buffer_data->isc_dataproc_resultdata.module_status[i].processing_time;
            }

            // copy image data
            IscImageInfo* src_isc_image_info = &dataproc_result_buffer_data->isc_dataproc_resultdata.isc_image_info;

            isc_data_proc_result_data->isc_image_info.grab = src_isc_image_info->grab;
            isc_data_proc_result_data->isc_image_info.color_grab_mode = src_isc_image_info->color_grab_mode;
            isc_data_proc_result_data->isc_image_info.shutter_mode = src_isc_image_info->shutter_mode;

            isc_data_proc_result_data->isc_image_info.camera_specific_parameter.d_inf = src_isc_image_info->camera_specific_parameter.d_inf;
            isc_data_proc_result_data->isc_image_info.camera_specific_parameter.bf = src_isc_image_info->camera_specific_parameter.bf;
            isc_data_proc_result_data->isc_image_info.camera_specific_parameter.base_length = src_isc_image_info->camera_specific_parameter.base_length;
            isc_data_proc_result_data->isc_image_info.camera_specific_parameter.dz = src_isc_image_info->camera_specific_parameter.dz;

            for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
                isc_data_proc_result_data->isc_image_info.frame_data[i].frameNo = src_isc_image_info->frame_data[i].frameNo;
                isc_data_proc_result_data->isc_image_info.frame_data[i].gain = src_isc_image_info->frame_data[i].gain;
                isc_data_proc_result_data->isc_image_info.frame_data[i].exposure = src_isc_image_info->frame_data[i].exposure;

                isc_data_proc_result_data->isc_image_info.frame_data[i].camera_status.error_code = src_isc_image_info->frame_data[i].camera_status.error_code;
                isc_data_proc_result_data->isc_image_info.frame_data[i].camera_status.data_receive_tact_time = src_isc_image_info->frame_data[i].camera_status.data_receive_tact_time;

                isc_data_proc_result_data->isc_image_info.frame_data[i].p1.width = src_isc_image_info->frame_data[i].p1.width;
                isc_data_proc_result_data->isc_image_info.frame_data[i].p1.height = src_isc_image_info->frame_data[i].p1.height;
                isc_data_proc_result_data->isc_image_info.frame_data[i].p1.channel_count = src_isc_image_info->frame_data[i].p1.channel_count;
                size_t cp_size = src_isc_image_info->frame_data[i].p1.width * src_isc_image_info->frame_data[i].p1.height * src_isc_image_info->frame_data[i].p1.channel_count;
                if (cp_size > 0) {
                    memcpy(isc_data_proc_result_data->isc_image_info.frame_data[i].p1.image, src_isc_image_info->frame_data[i].p1.image, cp_size);
                }

                isc_data_proc_result_data->isc_image_info.frame_data[i].p2.width = src_isc_image_info->frame_data[i].p2.width;
                isc_data_proc_result_data->isc_image_info.frame_data[i].p2.height = src_isc_image_info->frame_data[i].p2.height;
                isc_data_proc_result_data->isc_image_info.frame_data[i].p2.channel_count = src_isc_image_info->frame_data[i].p2.channel_count;
                cp_size = src_isc_image_info->frame_data[i].p2.width * src_isc_image_info->frame_data[i].p2.height * src_isc_image_info->frame_data[i].p2.channel_count;
                if (cp_size > 0) {
                    memcpy(isc_data_proc_result_data->isc_image_info.frame_data[i].p2.image, src_isc_image_info->frame_data[i].p2.image, cp_size);
                }

                isc_data_proc_result_data->isc_image_info.frame_data[i].color.width = src_isc_image_info->frame_data[i].color.width;
                isc_data_proc_result_data->isc_image_info.frame_data[i].color.height = src_isc_image_info->frame_data[i].color.height;
                isc_data_proc_result_data->isc_image_info.frame_data[i].color.channel_count = src_isc_image_info->frame_data[i].color.channel_count;
                cp_size = src_isc_image_info->frame_data[i].color.width * src_isc_image_info->frame_data[i].color.height * src_isc_image_info->frame_data[i].color.channel_count;
                if (cp_size > 0) {
                    memcpy(isc_data_proc_result_data->isc_image_info.frame_data[i].color.image, src_isc_image_info->frame_data[i].color.image, cp_size);
                }

                isc_data_proc_result_data->isc_image_info.frame_data[i].depth.width = src_isc_image_info->frame_data[i].depth.width;
                isc_data_proc_result_data->isc_image_info.frame_data[i].depth.height = src_isc_image_info->frame_data[i].depth.height;
                cp_size = src_isc_image_info->frame_data[i].depth.width * src_isc_image_info->frame_data[i].depth.height * sizeof(float);
                if (cp_size > 0) {
                    memcpy(isc_data_proc_result_data->isc_image_info.frame_data[i].depth.image, src_isc_image_info->frame_data[i].depth.image, cp_size);
                }

                isc_data_proc_result_data->isc_image_info.frame_data[i].raw.width = src_isc_image_info->frame_data[i].raw.width;
                isc_data_proc_result_data->isc_image_info.frame_data[i].raw.height = src_isc_image_info->frame_data[i].raw.height;
                isc_data_proc_result_data->isc_image_info.frame_data[i].raw.channel_count = src_isc_image_info->frame_data[i].raw.channel_count;
                cp_size = src_isc_image_info->frame_data[i].raw.width * src_isc_image_info->frame_data[i].raw.height * src_isc_image_info->frame_data[i].raw.channel_count;
                if (cp_size > 0) {
                    memcpy(isc_data_proc_result_data->isc_image_info.frame_data[i].raw.image, src_isc_image_info->frame_data[i].raw.image, cp_size);
                }

                isc_data_proc_result_data->isc_image_info.frame_data[i].raw_color.width = src_isc_image_info->frame_data[i].raw_color.width;
                isc_data_proc_result_data->isc_image_info.frame_data[i].raw_color.height = src_isc_image_info->frame_data[i].raw_color.height;
                isc_data_proc_result_data->isc_image_info.frame_data[i].raw_color.channel_count = src_isc_image_info->frame_data[i].raw_color.channel_count;
                cp_size = src_isc_image_info->frame_data[i].raw_color.width * src_isc_image_info->frame_data[i].raw_color.height * src_isc_image_info->frame_data[i].raw_color.channel_count;
                if (cp_size > 0) {
                    memcpy(isc_data_proc_result_data->isc_image_info.frame_data[i].raw_color.image, src_isc_image_info->frame_data[i].raw_color.image, cp_size);
                }
            }

            {
                char debug_msg[256] = {};
                sprintf_s(debug_msg, "[IscDataProcessingControl::GetDataProcModuleData] get dp_proc data fn=%d\n", src_isc_image_info->frame_data[0].frameNo);
                OutputDebugStringA(debug_msg);
            }
        }

        isc_dataproc_resultdata_ring_buffer_->DoneGetBuffer(get_index);

        if (get_index < 0) {
            return CAMCONTROL_E_NO_IMAGE;
        }
    }
    else {
        return CAMCONTROL_E_NO_IMAGE;
    }

    return DPC_E_OK;
}

/**
 * データ処理を呼び出します
 *
 * @param[in] isc_image_info 入力データ
 * @retval 0 成功
 * @retval other 失敗
 * @note 
 *  - 同期型・非同期型があります  
 *  - 非同期型固定です
 */
int IscDataProcessingControl::Run(IscImageInfo* isc_image_info)
{
    if (isc_data_proc_module_configuration_.enabled_data_proc_module) {

        if (isc_frame_decoder_ == nullptr) {
            return DPCCONTROL_E_INVALID_DEVICEHANDLE;
        }

        int mode = 1;
        if (isc_grab_start_mode_.isc_play_mode == IscPlayMode::kPlayOn) {
            // 全てのデータを処理します
            mode = 1;
        }

        if (mode == 0) {
            // sync mode
            int ret = SyncRun(isc_image_info);
            if (ret != DPC_E_OK) {
                return ret;
            }
        }
        else {
            // async mode
            int ret = AsyncRun(isc_image_info);
            if (ret != DPC_E_OK) {
                return ret;
            }
        }
    }
    else {
        return DPC_E_OK;
    }

    return DPC_E_OK;
}

/**
 * データ処理Threadです
 *
 * @param[in] context Thread入力パラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
unsigned __stdcall IscDataProcessingControl::IscDataProcessingControl::ControlThreadDataProc(void* context)
{
    IscDataProcessingControl* isc_data_processing_control = (IscDataProcessingControl*)context;

    if (isc_data_processing_control == nullptr) {
        return -1;
    }

    int ret = isc_data_processing_control->DataProc();

    return ret;
}

/**
 * データ処理Threadの処理本体
 *
 * @param[in] context Thread入力パラメータ
 * @retval 0 成功
 * @retval other 失敗
 * @note 非同期型　処理呼び出しによって利用されます
 */
int IscDataProcessingControl::DataProc()
{

    constexpr DWORD wait_milli_seconds = 10;

    while (thread_control_dataproc_.terminate_request < 1) {

        // Wait for start
        DWORD wait_result = WaitForSingleObject(handle_semaphore_dataproc_, wait_milli_seconds /*INFINITE*/);

        if (thread_control_dataproc_.stop_request) {
            thread_control_dataproc_.stop_request = false;
            break;
        }

        if (wait_result == WAIT_OBJECT_0) {
            // get data
            IscImageInfoRingBuffer::BufferData* image_info_buffer_data = nullptr;
            ULONGLONG time = 0;
            int get_index = isc_image_info_ring_buffer_->GetGetBuffer(&image_info_buffer_data, &time);

            if (get_index >= 0) {
                // there is an image

                // get buffer for proc
                IscDataprocResultdataRingBuffer::BufferData* dataproc_result_buffer_data = nullptr;
                const ULONGLONG time = GetTickCount64();
                int put_index = isc_dataproc_resultdata_ring_buffer_->GetPutBuffer(&dataproc_result_buffer_data, time);
                int image_status = 0;

                if (put_index >= 0 && dataproc_result_buffer_data != nullptr) {
                    // call data processing
                    int dp_ret = RunDataProcModules(&image_info_buffer_data->isc_image_info, &dataproc_result_buffer_data->isc_dataproc_resultdata);

                    // ended
                    if (dp_ret == DPC_E_OK) {
                        image_status = 1;
                    }

                }
                isc_dataproc_resultdata_ring_buffer_->DonePutBuffer(put_index, image_status);
            }

            // ended
            isc_image_info_ring_buffer_->DoneGetBuffer(get_index);
        }
        else if (wait_result == WAIT_TIMEOUT) {
            // The semaphore was nonsignaled, so a time-out occurred.
        }
        else if (wait_result == WAIT_FAILED) {
            // error, abort
            // The thread got ownership of an abandoned mutex
            // The database is in an indeterminate state
            break;
        }
    }

    thread_control_dataproc_.terminate_done = 1;

    return 0;
}

/**
 * 同期型　処理呼び出し
 *
 * @param[in] isc_image_info 入力データ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::SyncRun(IscImageInfo* isc_image_info)
{

    // get buffer for proc
    IscDataprocResultdataRingBuffer::BufferData* dataproc_result_buffer_data = nullptr;
    const ULONGLONG time = GetTickCount64();
    int put_index = isc_dataproc_resultdata_ring_buffer_->GetPutBuffer(&dataproc_result_buffer_data, time);
    int image_status = 0;

    if (put_index >= 0 && dataproc_result_buffer_data != nullptr) {
        // call data processing
        int dp_ret = RunDataProcModules(isc_image_info, &dataproc_result_buffer_data->isc_dataproc_resultdata);

        if (dp_ret != DPC_E_OK) {
            isc_dataproc_resultdata_ring_buffer_->DonePutBuffer(put_index, image_status);
            return dp_ret;
        }

        image_status = 1;
    }

    isc_dataproc_resultdata_ring_buffer_->DonePutBuffer(put_index, image_status);
    
	return DPC_E_OK;
}

/**
 * 非同期型　処理呼び出し
 *
 * @param[in] isc_image_info 入力データ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::AsyncRun(IscImageInfo* isc_image_info)
{

    if (isc_frame_decoder_ == nullptr) {
        return DPCCONTROL_E_INVALID_DEVICEHANDLE;
    }

    IscImageInfoRingBuffer::BufferData* buffer_data = nullptr;
    const ULONGLONG time = GetTickCount64();
    int put_index = isc_image_info_ring_buffer_->GetPutBuffer(&buffer_data, time);
    int image_status = 0;

    if (put_index >= 0 && buffer_data != nullptr) {

        buffer_data->isc_image_info.grab = isc_image_info->grab;
        buffer_data->isc_image_info.color_grab_mode = isc_image_info->color_grab_mode;
        buffer_data->isc_image_info.shutter_mode = isc_image_info->shutter_mode;
        buffer_data->isc_image_info.camera_specific_parameter.d_inf = isc_image_info->camera_specific_parameter.d_inf;
        buffer_data->isc_image_info.camera_specific_parameter.bf = isc_image_info->camera_specific_parameter.bf;
        buffer_data->isc_image_info.camera_specific_parameter.base_length = isc_image_info->camera_specific_parameter.base_length;
        buffer_data->isc_image_info.camera_specific_parameter.dz = isc_image_info->camera_specific_parameter.dz;

        for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
            buffer_data->isc_image_info.frame_data[i].frameNo = isc_image_info->frame_data[i].frameNo;
            buffer_data->isc_image_info.frame_data[i].gain = isc_image_info->frame_data[i].gain;
            buffer_data->isc_image_info.frame_data[i].exposure = isc_image_info->frame_data[i].exposure;

            buffer_data->isc_image_info.frame_data[i].camera_status.error_code = isc_image_info->frame_data[i].camera_status.error_code;
            buffer_data->isc_image_info.frame_data[i].camera_status.data_receive_tact_time = isc_image_info->frame_data[i].camera_status.data_receive_tact_time;

            buffer_data->isc_image_info.frame_data[i].p1.width = isc_image_info->frame_data[i].p1.width;
            buffer_data->isc_image_info.frame_data[i].p1.height = isc_image_info->frame_data[i].p1.height;
            buffer_data->isc_image_info.frame_data[i].p1.channel_count = isc_image_info->frame_data[i].p1.channel_count;
            size_t cp_size = isc_image_info->frame_data[i].p1.width * isc_image_info->frame_data[i].p1.height * isc_image_info->frame_data[i].p1.channel_count;
            if (cp_size > 0) {
                memcpy(buffer_data->isc_image_info.frame_data[i].p1.image, isc_image_info->frame_data[i].p1.image, cp_size);
            }

            buffer_data->isc_image_info.frame_data[i].p2.width = isc_image_info->frame_data[i].p2.width;
            buffer_data->isc_image_info.frame_data[i].p2.height = isc_image_info->frame_data[i].p2.height;
            buffer_data->isc_image_info.frame_data[i].p2.channel_count = isc_image_info->frame_data[i].p2.channel_count;
            cp_size = isc_image_info->frame_data[i].p2.width * isc_image_info->frame_data[i].p2.height * isc_image_info->frame_data[i].p2.channel_count;
            if (cp_size > 0) {
                memcpy(buffer_data->isc_image_info.frame_data[i].p2.image, isc_image_info->frame_data[i].p2.image, cp_size);
            }

            buffer_data->isc_image_info.frame_data[i].color.width = isc_image_info->frame_data[i].color.width;
            buffer_data->isc_image_info.frame_data[i].color.height = isc_image_info->frame_data[i].color.height;
            buffer_data->isc_image_info.frame_data[i].color.channel_count = isc_image_info->frame_data[i].color.channel_count;
            cp_size = isc_image_info->frame_data[i].color.width * isc_image_info->frame_data[i].color.height * isc_image_info->frame_data[i].color.channel_count;
            if (cp_size > 0) {
                memcpy(buffer_data->isc_image_info.frame_data[i].color.image, isc_image_info->frame_data[i].color.image, cp_size);
            }

            buffer_data->isc_image_info.frame_data[i].depth.width = isc_image_info->frame_data[i].depth.width;
            buffer_data->isc_image_info.frame_data[i].depth.height = isc_image_info->frame_data[i].depth.height;
            cp_size = isc_image_info->frame_data[i].depth.width * isc_image_info->frame_data[i].depth.height * sizeof(float);
            if (cp_size > 0) {
                memcpy(buffer_data->isc_image_info.frame_data[i].depth.image, isc_image_info->frame_data[i].depth.image, cp_size);
            }

            buffer_data->isc_image_info.frame_data[i].raw.width = isc_image_info->frame_data[i].raw.width;
            buffer_data->isc_image_info.frame_data[i].raw.height = isc_image_info->frame_data[i].raw.height;
            buffer_data->isc_image_info.frame_data[i].raw.channel_count = isc_image_info->frame_data[i].raw.channel_count;
            cp_size = isc_image_info->frame_data[i].raw.width * isc_image_info->frame_data[i].raw.height * isc_image_info->frame_data[i].raw.channel_count;
            if (cp_size > 0) {
                memcpy(buffer_data->isc_image_info.frame_data[i].raw.image, isc_image_info->frame_data[i].raw.image, cp_size);
            }

            buffer_data->isc_image_info.frame_data[i].raw_color.width = isc_image_info->frame_data[i].raw_color.width;
            buffer_data->isc_image_info.frame_data[i].raw_color.height = isc_image_info->frame_data[i].raw_color.height;
            buffer_data->isc_image_info.frame_data[i].raw_color.channel_count = isc_image_info->frame_data[i].raw_color.channel_count;
            cp_size = isc_image_info->frame_data[i].raw_color.width * isc_image_info->frame_data[i].raw_color.height * isc_image_info->frame_data[i].raw_color.channel_count;
            if (cp_size > 0) {
                memcpy(buffer_data->isc_image_info.frame_data[i].raw_color.image, isc_image_info->frame_data[i].raw_color.image, cp_size);
            }
        }

        image_status = 1;

        // start processing thread
        BOOL result_release = ReleaseSemaphore(handle_semaphore_dataproc_, 1, NULL);
        if (!result_release) {
            // This usually happens when data processing is slow. So you can ignore this error.
            //  
            //DWORD last_error = GetLastError();
            //char error_msg[256] = {};
            //sprintf_s(error_msg, "[ERROR] IscDataProcessingControl::AsyncRun ReleaseSemaphore failed(%d)\n", last_error);
            //OutputDebugStringA(error_msg);
        }

        {
            char debug_msg[256] = {};
            sprintf_s(debug_msg, "[IscDataProcessingControl::AsyncRun] start dp_proc fn=%d\n", isc_image_info->frame_data[0].frameNo);
            OutputDebugStringA(debug_msg);
        }
    }

    isc_image_info_ring_buffer_->DonePutBuffer(put_index, image_status);

	return DPC_E_OK;
}

/**
 * データ処理モジュールを呼び出します
 *
 * @param[in] isc_image_info 入力データ
 * @param[out] isc_data_proc_result_data 処理結果データ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::RunDataProcModules(IscImageInfo* isc_image_info, IscDataProcResultData* isc_data_proc_result_data)
{
    
    // clear
    ClearIscDataProcResultData(isc_data_proc_result_data);

    if( (isc_stereo_matching_ == nullptr)   ||
        (isc_frame_decoder_ == nullptr)     ||
        (isc_disparity_filter_ == nullptr) ) {

        return DPC_E_OK;
    }

    /*
        --- check mode ---

        enabled_stereo_matching     input:after correct     double shutter:not support
        enabled_frame_decoder       input:disparity         double shutter:support
        enabled_disparity_filter    -                       -
    
        ---bouble shutter ---
        
        kDoubleShutter      Combine light and dark images
        kDoubleShutter2     Displays light and dark images independently.     

    */

    int dp_ret = DPC_E_OK;

    if (isc_dataproc_start_mode_.enabled_stereo_matching) {
        dp_ret = RunDataProcStereoMatching(isc_image_info, isc_data_proc_result_data);

        isc_data_proc_result_data->status.proc_tact_time = measure_time_->GetTaktTime();
    }
    else if (isc_dataproc_start_mode_.enabled_frame_decoder) {
        // double shutter mode or other mode
        if (isc_image_info->shutter_mode == IscShutterMode::kDoubleShutter) {
            dp_ret = RunDataProcFrameDecoderInDoubleShutter(isc_image_info, isc_data_proc_result_data);
        }
        else {
            dp_ret = RunDataProcFrameDecoder(isc_image_info, isc_data_proc_result_data);
        }

        isc_data_proc_result_data->status.proc_tact_time = measure_time_->GetTaktTime();
    }

    return dp_ret;
}

/**
 * Stereo Matchingを呼び出します
 *
 * @param[in] isc_image_info 入力データ
 * @param[out] isc_data_proc_result_data 処理結果データ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::RunDataProcStereoMatching(IscImageInfo* isc_image_info, IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_dataproc_start_mode_.enabled_disparity_filter) {
        // stereo matching -> disparity filter

        // (1) block matching
        measure_time_->Start();

        int module_index = isc_data_proc_result_data->number_of_modules_processed;
        sprintf_s(isc_data_proc_result_data->module_status[module_index].module_names,
            isc_data_proc_result_data->maximum_number_of_modulename,
            ("Stereo Matching\n"));

        int dp_ret = isc_stereo_matching_->GetBlockDisparity(isc_image_info, &isc_block_disparity_data_);

        isc_data_proc_result_data->module_status[module_index].error_code = dp_ret;
        isc_data_proc_result_data->module_status[module_index].processing_time = measure_time_->Stop();

        isc_data_proc_result_data->number_of_modules_processed++;

        // (2) disparity filter
        measure_time_->Start();

        module_index = isc_data_proc_result_data->number_of_modules_processed;
        sprintf_s(isc_data_proc_result_data->module_status[module_index].module_names,
            isc_data_proc_result_data->maximum_number_of_modulename,
            ("Disparity Filter\n"));

        dp_ret = isc_disparity_filter_->GetAverageDisparityData(isc_image_info, &isc_block_disparity_data_, isc_data_proc_result_data);

        isc_data_proc_result_data->module_status[module_index].error_code = dp_ret;
        isc_data_proc_result_data->module_status[module_index].processing_time = measure_time_->Stop();

        isc_data_proc_result_data->number_of_modules_processed++;
    }
    else {
        // block matching only
        measure_time_->Start();

        int module_index = isc_data_proc_result_data->number_of_modules_processed;
        sprintf_s(isc_data_proc_result_data->module_status[module_index].module_names,
            isc_data_proc_result_data->maximum_number_of_modulename,
            ("Stereo Matching\n"));

        int dp_ret = isc_stereo_matching_->GetDisparity(isc_image_info, isc_data_proc_result_data);

        isc_data_proc_result_data->module_status[module_index].error_code = dp_ret;
        isc_data_proc_result_data->module_status[module_index].processing_time = measure_time_->Stop();

        isc_data_proc_result_data->number_of_modules_processed++;
    }

    // copy additional data
    IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

    dst_isc_image_info->grab = isc_image_info->grab;
    dst_isc_image_info->color_grab_mode = isc_image_info->color_grab_mode;
    dst_isc_image_info->shutter_mode = isc_image_info->shutter_mode;
    dst_isc_image_info->camera_specific_parameter.d_inf = isc_image_info->camera_specific_parameter.d_inf;
    dst_isc_image_info->camera_specific_parameter.bf = isc_image_info->camera_specific_parameter.bf;
    dst_isc_image_info->camera_specific_parameter.base_length = isc_image_info->camera_specific_parameter.base_length;
    dst_isc_image_info->camera_specific_parameter.dz = isc_image_info->camera_specific_parameter.dz;

    const int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    dst_isc_image_info->frame_data[fd_index].frameNo = isc_image_info->frame_data[fd_index].frameNo;
    dst_isc_image_info->frame_data[fd_index].gain = isc_image_info->frame_data[fd_index].gain;
    dst_isc_image_info->frame_data[fd_index].exposure = isc_image_info->frame_data[fd_index].exposure;

    dst_isc_image_info->frame_data[fd_index].camera_status.error_code = isc_image_info->frame_data[fd_index].camera_status.error_code;
    dst_isc_image_info->frame_data[fd_index].camera_status.data_receive_tact_time = isc_image_info->frame_data[fd_index].camera_status.data_receive_tact_time;

    dst_isc_image_info->frame_data[fd_index].p1.width = isc_image_info->frame_data[fd_index].p1.width;
    dst_isc_image_info->frame_data[fd_index].p1.height = isc_image_info->frame_data[fd_index].p1.height;
    dst_isc_image_info->frame_data[fd_index].p1.channel_count = isc_image_info->frame_data[fd_index].p1.channel_count;
    size_t cp_size = isc_image_info->frame_data[fd_index].p1.width * isc_image_info->frame_data[fd_index].p1.height * isc_image_info->frame_data[fd_index].p1.channel_count;
    if (cp_size > 0) {
        memcpy(dst_isc_image_info->frame_data[fd_index].p1.image, isc_image_info->frame_data[fd_index].p1.image, cp_size);
    }

    dst_isc_image_info->frame_data[fd_index].p2.width = isc_image_info->frame_data[fd_index].p2.width;
    dst_isc_image_info->frame_data[fd_index].p2.height = isc_image_info->frame_data[fd_index].p2.height;
    dst_isc_image_info->frame_data[fd_index].p2.channel_count = isc_image_info->frame_data[fd_index].p2.channel_count;
    cp_size = isc_image_info->frame_data[fd_index].p2.width * isc_image_info->frame_data[fd_index].p2.height * isc_image_info->frame_data[fd_index].p2.channel_count;
    if (cp_size > 0) {
        memcpy(dst_isc_image_info->frame_data[fd_index].p2.image, isc_image_info->frame_data[fd_index].p2.image, cp_size);
    }

    dst_isc_image_info->frame_data[fd_index].color.width = isc_image_info->frame_data[fd_index].color.width;
    dst_isc_image_info->frame_data[fd_index].color.height = isc_image_info->frame_data[fd_index].color.height;
    dst_isc_image_info->frame_data[fd_index].color.channel_count = isc_image_info->frame_data[fd_index].color.channel_count;
    cp_size = isc_image_info->frame_data[fd_index].color.width * isc_image_info->frame_data[fd_index].color.height * isc_image_info->frame_data[fd_index].color.channel_count;
    if (cp_size > 0) {
        memcpy(dst_isc_image_info->frame_data[fd_index].color.image, isc_image_info->frame_data[fd_index].color.image, cp_size);
    }

    // Ended
    isc_data_proc_result_data->status.error_code = DPC_E_OK;

    return DPC_E_OK;
}

/**
 * Frame Decoderを呼び出します
 *
 * @param[in] isc_image_info 入力データ
 * @param[out] isc_data_proc_result_data 処理結果データ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::RunDataProcFrameDecoder(IscImageInfo* isc_image_info, IscDataProcResultData* isc_data_proc_result_data)
{

    measure_time_->Start();

    // manual, single
    int module_index = isc_data_proc_result_data->number_of_modules_processed;
    sprintf_s(isc_data_proc_result_data->module_status[module_index].module_names,
        isc_data_proc_result_data->maximum_number_of_modulename,
        ("Frame Decoder\n"));

    int dp_ret = isc_frame_decoder_->GetDecodeData(isc_image_info, &isc_block_disparity_data_);

    isc_data_proc_result_data->module_status[module_index].error_code = dp_ret;
    isc_data_proc_result_data->module_status[module_index].processing_time = measure_time_->Stop();

    isc_data_proc_result_data->number_of_modules_processed++;

    if (isc_dataproc_start_mode_.enabled_disparity_filter) {
        // stereo matching -> disparity filter

        measure_time_->Start();

        int  module_index = isc_data_proc_result_data->number_of_modules_processed;
        sprintf_s(isc_data_proc_result_data->module_status[module_index].module_names,
            isc_data_proc_result_data->maximum_number_of_modulename,
            ("Disparity Filter\n"));

        int dp_ret = isc_disparity_filter_->GetAverageDisparityData(isc_image_info, &isc_block_disparity_data_, isc_data_proc_result_data);

        isc_data_proc_result_data->module_status[module_index].error_code = dp_ret;
        isc_data_proc_result_data->module_status[module_index].processing_time = measure_time_->Stop();

        isc_data_proc_result_data->number_of_modules_processed++;
    }
    else {
        IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

        int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

        dst_isc_image_info->frame_data[fd_index].depth.width = isc_block_disparity_data_.image_width;
        dst_isc_image_info->frame_data[fd_index].depth.height = isc_block_disparity_data_.image_height;

        size_t cp_size = isc_block_disparity_data_.image_width * isc_block_disparity_data_.image_height * sizeof(float);
        memcpy(dst_isc_image_info->frame_data[fd_index].depth.image, isc_block_disparity_data_.ppxldsp, cp_size);;
    }

    // copy additional data
    IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

    dst_isc_image_info->grab = isc_image_info->grab;
    dst_isc_image_info->color_grab_mode = isc_image_info->color_grab_mode;
    dst_isc_image_info->shutter_mode = isc_image_info->shutter_mode;
    dst_isc_image_info->camera_specific_parameter.d_inf = isc_image_info->camera_specific_parameter.d_inf;
    dst_isc_image_info->camera_specific_parameter.bf = isc_image_info->camera_specific_parameter.bf;
    dst_isc_image_info->camera_specific_parameter.base_length = isc_image_info->camera_specific_parameter.base_length;
    dst_isc_image_info->camera_specific_parameter.dz = isc_image_info->camera_specific_parameter.dz;

    const int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    dst_isc_image_info->frame_data[fd_index].frameNo = isc_image_info->frame_data[fd_index].frameNo;
    dst_isc_image_info->frame_data[fd_index].gain = isc_image_info->frame_data[fd_index].gain;
    dst_isc_image_info->frame_data[fd_index].exposure = isc_image_info->frame_data[fd_index].exposure;

    dst_isc_image_info->frame_data[fd_index].camera_status.error_code = isc_image_info->frame_data[fd_index].camera_status.error_code;
    dst_isc_image_info->frame_data[fd_index].camera_status.data_receive_tact_time = isc_image_info->frame_data[fd_index].camera_status.data_receive_tact_time;

    dst_isc_image_info->frame_data[fd_index].p1.width = isc_image_info->frame_data[fd_index].p1.width;
    dst_isc_image_info->frame_data[fd_index].p1.height = isc_image_info->frame_data[fd_index].p1.height;
    dst_isc_image_info->frame_data[fd_index].p1.channel_count = isc_image_info->frame_data[fd_index].p1.channel_count;
    size_t cp_size = isc_image_info->frame_data[fd_index].p1.width * isc_image_info->frame_data[fd_index].p1.height * isc_image_info->frame_data[fd_index].p1.channel_count;
    if (cp_size > 0) {
        memcpy(dst_isc_image_info->frame_data[fd_index].p1.image, isc_image_info->frame_data[fd_index].p1.image, cp_size);
    }

    dst_isc_image_info->frame_data[fd_index].p2.width = isc_image_info->frame_data[fd_index].p2.width;
    dst_isc_image_info->frame_data[fd_index].p2.height = isc_image_info->frame_data[fd_index].p2.height;
    dst_isc_image_info->frame_data[fd_index].p2.channel_count = isc_image_info->frame_data[fd_index].p2.channel_count;
    cp_size = isc_image_info->frame_data[fd_index].p2.width * isc_image_info->frame_data[fd_index].p2.height * isc_image_info->frame_data[fd_index].p2.channel_count;
    if (cp_size > 0) {
        memcpy(dst_isc_image_info->frame_data[fd_index].p2.image, isc_image_info->frame_data[fd_index].p2.image, cp_size);
    }

    dst_isc_image_info->frame_data[fd_index].color.width = isc_image_info->frame_data[fd_index].color.width;
    dst_isc_image_info->frame_data[fd_index].color.height = isc_image_info->frame_data[fd_index].color.height;
    dst_isc_image_info->frame_data[fd_index].color.channel_count = isc_image_info->frame_data[fd_index].color.channel_count;
    cp_size = isc_image_info->frame_data[fd_index].color.width * isc_image_info->frame_data[fd_index].color.height * isc_image_info->frame_data[fd_index].color.channel_count;
    if (cp_size > 0) {
        memcpy(dst_isc_image_info->frame_data[fd_index].color.image, isc_image_info->frame_data[fd_index].color.image, cp_size);
    }

    // Ended
    isc_data_proc_result_data->status.error_code = DPC_E_OK;

    return DPC_E_OK;
}

/**
 * Frame DecoderをDouble Shutter モードで呼び出します
 *
 * @param[in] isc_image_info 入力データ
 * @param[out] isc_data_proc_result_data 処理結果データ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscDataProcessingControl::RunDataProcFrameDecoderInDoubleShutter(IscImageInfo* isc_image_info, IscDataProcResultData* isc_data_proc_result_data)
{
    measure_time_->Start();

    // double shutter mode or other mode
    int module_index = isc_data_proc_result_data->number_of_modules_processed;
    sprintf_s(isc_data_proc_result_data->module_status[module_index].module_names,
        isc_data_proc_result_data->maximum_number_of_modulename,
        ("Frame Decoder\n"));

    int dp_ret = isc_frame_decoder_->GetDecodeDataDoubleShutter(isc_image_info, &isc_block_disparity_data_, &isc_data_proc_result_data->isc_image_info);

    isc_data_proc_result_data->module_status[module_index].error_code = dp_ret;
    isc_data_proc_result_data->module_status[module_index].processing_time = measure_time_->Stop();

    isc_data_proc_result_data->number_of_modules_processed++;

    if (isc_dataproc_start_mode_.enabled_disparity_filter) {
        // stereo matching -> disparity filter

        measure_time_->Start();

        int  module_index = isc_data_proc_result_data->number_of_modules_processed;
        sprintf_s(isc_data_proc_result_data->module_status[module_index].module_names,
            isc_data_proc_result_data->maximum_number_of_modulename,
            ("Disparity Filter\n"));

        int dp_ret = isc_disparity_filter_->GetAverageDisparityDataDoubleShutter(isc_image_info, &isc_block_disparity_data_, isc_data_proc_result_data);

        // copy additional data
        IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;
        int fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;

        dst_isc_image_info->frame_data[fd_index].p1.width = isc_block_disparity_data_.image_width;
        dst_isc_image_info->frame_data[fd_index].p1.height = isc_block_disparity_data_.image_height;
        dst_isc_image_info->frame_data[fd_index].p1.channel_count = 1;
        size_t cp_size = dst_isc_image_info->frame_data[fd_index].p1.width * dst_isc_image_info->frame_data[fd_index].p1.height * dst_isc_image_info->frame_data[fd_index].p1.channel_count;
        if (cp_size > 0) {
            memcpy(dst_isc_image_info->frame_data[fd_index].p1.image, isc_block_disparity_data_.pbldimg, cp_size);
        }

        // copy non merged data
        int fd_index_src = kISCIMAGEINFO_FRAMEDATA_LATEST;
        dst_isc_image_info->frame_data[fd_index].p2.width = isc_image_info->frame_data[fd_index_src].p2.width;
        dst_isc_image_info->frame_data[fd_index].p2.height = isc_image_info->frame_data[fd_index_src].p2.height;
        dst_isc_image_info->frame_data[fd_index].p2.channel_count = isc_image_info->frame_data[fd_index_src].p2.channel_count;
        cp_size = isc_image_info->frame_data[fd_index].p2.width * isc_image_info->frame_data[fd_index_src].p2.height * isc_image_info->frame_data[fd_index_src].p2.channel_count;
        if (cp_size > 0) {
            memcpy(dst_isc_image_info->frame_data[fd_index].p2.image, isc_image_info->frame_data[fd_index_src].p2.image, cp_size);
        }

        dst_isc_image_info->frame_data[fd_index].color.width = isc_image_info->frame_data[fd_index_src].color.width;
        dst_isc_image_info->frame_data[fd_index].color.height = isc_image_info->frame_data[fd_index_src].color.height;
        dst_isc_image_info->frame_data[fd_index].color.channel_count = isc_image_info->frame_data[fd_index_src].color.channel_count;
        cp_size = isc_image_info->frame_data[fd_index].color.width * isc_image_info->frame_data[fd_index_src].color.height * isc_image_info->frame_data[fd_index_src].color.channel_count;
        if (cp_size > 0) {
            memcpy(dst_isc_image_info->frame_data[fd_index].color.image, isc_image_info->frame_data[fd_index_src].color.image, cp_size);
        }

        isc_data_proc_result_data->module_status[module_index].error_code = dp_ret;
        isc_data_proc_result_data->module_status[module_index].processing_time = measure_time_->Stop();

        isc_data_proc_result_data->number_of_modules_processed++;
    }
    else {
        IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

        int fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;

        dst_isc_image_info->frame_data[fd_index].depth.width = isc_block_disparity_data_.image_width;
        dst_isc_image_info->frame_data[fd_index].depth.height = isc_block_disparity_data_.image_height;

        size_t cp_size = isc_block_disparity_data_.image_width * isc_block_disparity_data_.image_height * sizeof(float);
        memcpy(dst_isc_image_info->frame_data[fd_index].depth.image, isc_block_disparity_data_.ppxldsp, cp_size);;

        // copy additional data
        dst_isc_image_info->frame_data[fd_index].p1.width = isc_block_disparity_data_.image_width;
        dst_isc_image_info->frame_data[fd_index].p1.height = isc_block_disparity_data_.image_height;
        dst_isc_image_info->frame_data[fd_index].p1.channel_count = 1;
        cp_size = dst_isc_image_info->frame_data[fd_index].p1.width * dst_isc_image_info->frame_data[fd_index].p1.height * dst_isc_image_info->frame_data[fd_index].p1.channel_count;
        if (cp_size > 0) {
            memcpy(dst_isc_image_info->frame_data[fd_index].p1.image, isc_block_disparity_data_.pbldimg, cp_size);
        }

        // copy non merged data
        int fd_index_src = kISCIMAGEINFO_FRAMEDATA_LATEST;
        dst_isc_image_info->frame_data[fd_index].p2.width = isc_image_info->frame_data[fd_index_src].p2.width;
        dst_isc_image_info->frame_data[fd_index].p2.height = isc_image_info->frame_data[fd_index_src].p2.height;
        dst_isc_image_info->frame_data[fd_index].p2.channel_count = isc_image_info->frame_data[fd_index_src].p2.channel_count;
        cp_size = isc_image_info->frame_data[fd_index].p2.width * isc_image_info->frame_data[fd_index_src].p2.height * isc_image_info->frame_data[fd_index_src].p2.channel_count;
        if (cp_size > 0) {
            memcpy(dst_isc_image_info->frame_data[fd_index].p2.image, isc_image_info->frame_data[fd_index_src].p2.image, cp_size);
        }

        dst_isc_image_info->frame_data[fd_index].color.width = isc_image_info->frame_data[fd_index_src].color.width;
        dst_isc_image_info->frame_data[fd_index].color.height = isc_image_info->frame_data[fd_index_src].color.height;
        dst_isc_image_info->frame_data[fd_index].color.channel_count = isc_image_info->frame_data[fd_index_src].color.channel_count;
        cp_size = isc_image_info->frame_data[fd_index].color.width * isc_image_info->frame_data[fd_index_src].color.height * isc_image_info->frame_data[fd_index_src].color.channel_count;
        if (cp_size > 0) {
            memcpy(dst_isc_image_info->frame_data[fd_index].color.image, isc_image_info->frame_data[fd_index_src].color.image, cp_size);
        }
    }

    // copy additional data
    if ((isc_dataproc_start_mode_.enabled_stereo_matching) ||
        (isc_dataproc_start_mode_.enabled_frame_decoder)) {

        IscImageInfo* dst_isc_image_info = &isc_data_proc_result_data->isc_image_info;

        dst_isc_image_info->grab = isc_image_info->grab;
        dst_isc_image_info->color_grab_mode = isc_image_info->color_grab_mode;
        dst_isc_image_info->shutter_mode = isc_image_info->shutter_mode;
        dst_isc_image_info->camera_specific_parameter.d_inf = isc_image_info->camera_specific_parameter.d_inf;
        dst_isc_image_info->camera_specific_parameter.bf = isc_image_info->camera_specific_parameter.bf;
        dst_isc_image_info->camera_specific_parameter.base_length = isc_image_info->camera_specific_parameter.base_length;
        dst_isc_image_info->camera_specific_parameter.dz = isc_image_info->camera_specific_parameter.dz;

        for (int i = 0; i < kISCIMAGEINFO_FRAMEDATA_MAX_COUNT; i++) {
            dst_isc_image_info->frame_data[i].frameNo = isc_image_info->frame_data[i].frameNo;
            dst_isc_image_info->frame_data[i].gain = isc_image_info->frame_data[i].gain;
            dst_isc_image_info->frame_data[i].exposure = isc_image_info->frame_data[i].exposure;

            dst_isc_image_info->frame_data[i].camera_status.error_code = isc_image_info->frame_data[i].camera_status.error_code;
            dst_isc_image_info->frame_data[i].camera_status.data_receive_tact_time = isc_image_info->frame_data[i].camera_status.data_receive_tact_time;
        }
    }

    // Ended
    isc_data_proc_result_data->status.error_code = DPC_E_OK;

    return DPC_E_OK;
}
