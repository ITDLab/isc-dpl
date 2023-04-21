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
 * @file isc_main_control_impl.cpp
 * @brief main control class for ISC_DPL
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a function for ISC_DPL.
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <stdint.h>
#include <process.h>
#include <mutex>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_log.h"
#include "utility.h"

#include "isc_image_info_ring_buffer.h"
#include "vm_sdk_wrapper.h"
#include "xc_sdk_wrapper.h"
#include "isc_sdk_control.h"
#include "isc_file_write_control_impl.h"
#include "isc_raw_data_decoder.h"
#include "isc_file_read_control_impl.h"
#include "isc_camera_control.h"
#include "isc_dataproc_resultdata_ring_buffer.h"
#include "isc_framedecoder_interface.h"
#include "isc_blockmatching_interface.h"
#include "isc_data_processing_control.h"

#include "isc_main_control_impl.h"

#include "opencv2\opencv.hpp"

#pragma comment (lib, "shlwapi")
#pragma comment (lib, "Shell32")
#pragma comment (lib, "imagehlp")
#pragma comment (lib, "IscCameraControl")
#pragma comment (lib, "IscDataProcessingControl")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world470d")
#else
#pragma comment (lib,"opencv_world470")
#endif

/**
 * constructor
 *
 */
IscMainControlImpl::IscMainControlImpl():
    isc_log_(nullptr),
    log_file_name_(),
    freq_for_performance_counter(),
	ipc_dpl_configuratio_(),
    isc_camera_control_(nullptr),
    isc_data_processing_control_(nullptr),
    isc_image_info_(),
    isc_image_info_ring_buffer_(nullptr),
    temp_isc_grab_start_mode_(),
    temp_isc_dataproc_start_mode_(),
    work_buffers_(),
    thread_control_camera_(),
    handle_semaphore_camera_(NULL),
    thread_handle_camera_(NULL),
    threads_critical_camera_()
{

    QueryPerformanceFrequency(&freq_for_performance_counter);

}

/**
 * destructor
 *
 */
IscMainControlImpl::~IscMainControlImpl()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] ipc_dpl_configuratio 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::Initialize(const IscDplConfiguration* ipc_dpl_configuratio)
{

    swprintf_s(ipc_dpl_configuratio_.configuration_file_path, L"%s", ipc_dpl_configuratio->configuration_file_path);
    swprintf_s(ipc_dpl_configuratio_.log_file_path, L"%s", ipc_dpl_configuratio->log_file_path);
    ipc_dpl_configuratio_.log_level = ipc_dpl_configuratio->log_level;

    ipc_dpl_configuratio_.enabled_camera = ipc_dpl_configuratio->enabled_camera;
    ipc_dpl_configuratio_.isc_camera_model = ipc_dpl_configuratio->isc_camera_model;

    swprintf_s(ipc_dpl_configuratio_.save_image_path, L"%s", ipc_dpl_configuratio->save_image_path);
    swprintf_s(ipc_dpl_configuratio_.load_image_path, L"%s", ipc_dpl_configuratio->load_image_path);

    ipc_dpl_configuratio_.enabled_data_proc_module = ipc_dpl_configuratio->enabled_data_proc_module;

    IscCameraControlConfiguration isc_camera_control_config = {};
    swprintf_s(isc_camera_control_config.configuration_file_path, L"%s", ipc_dpl_configuratio_.configuration_file_path);
    swprintf_s(isc_camera_control_config.log_file_path, L"%s", ipc_dpl_configuratio_.log_file_path);
    isc_camera_control_config.log_level = ipc_dpl_configuratio_.log_level;
    isc_camera_control_config.enabled_camera = ipc_dpl_configuratio_.enabled_camera;
    isc_camera_control_config.isc_camera_model = ipc_dpl_configuratio_.isc_camera_model;
    swprintf_s(isc_camera_control_config.save_image_path, L"%s", ipc_dpl_configuratio_.save_image_path);
    swprintf_s(isc_camera_control_config.load_image_path, L"%s", ipc_dpl_configuratio_.load_image_path);

    // log
    swprintf_s(log_file_name_, L"%s\\IscDplLib", ipc_dpl_configuratio_.log_file_path);
    isc_log_ = new IscLog;
    isc_log_->Open(ipc_dpl_configuratio_.log_file_path, log_file_name_, ipc_dpl_configuratio_.log_level, true);
    isc_log_->LogDebug(L"IscMainControlImpl", L"---Open log---\n");

    // camera control open
    wchar_t log_msg[256] = {};
    wchar_t camera_str[16] = {};
    switch (isc_camera_control_config.isc_camera_model) {
    case IscCameraModel::kVM: swprintf_s(camera_str, L"VM\n"); break;
    case IscCameraModel::kXC: swprintf_s(camera_str, L"XC\n"); break;
    case IscCameraModel::k4K: swprintf_s(camera_str, L"4K\n"); break;
    case IscCameraModel::k4KA: swprintf_s(camera_str, L"4KA\n"); break;
    case IscCameraModel::k4KJ: swprintf_s(camera_str, L"4KJ\n"); break;
    case IscCameraModel::kUnknown: swprintf_s(camera_str, L"unknown\n"); break;
    default:swprintf_s(camera_str, L"unknown\n"); break;
    }
    swprintf_s(log_msg, L"Open Camera Enabled=%d Type=%s", isc_camera_control_config.enabled_camera, camera_str);
    isc_log_->LogInfo(L"IscMainControlImpl", log_msg);

    isc_camera_control_ = new IscCameraControl;
    int ret_camera_open = isc_camera_control_->Initialize(&isc_camera_control_config, isc_log_);
    if (ret_camera_open != DPC_E_OK) {
        isc_camera_control_->Terminate();

        swprintf_s(log_msg, L"Open Camera failed (0x%08X)\n", ret_camera_open);
        isc_log_->LogError(L"IscMainControlImpl", log_msg);

        if (isc_camera_control_config.enabled_camera) {
            // open camera as disabled
            ipc_dpl_configuratio_.enabled_camera = false;
            isc_camera_control_config.enabled_camera = ipc_dpl_configuratio_.enabled_camera;
            int ret_retry = isc_camera_control_->Initialize(&isc_camera_control_config, isc_log_);

            if (ret_retry != DPC_E_OK) {
                swprintf_s(log_msg, L"  Failed to retry with camera disabled (0x%08X)\n", ret_retry);
                isc_log_->LogError(L"IscMainControlImpl", log_msg);

                return ret_camera_open;
            }
            else {
                // Keep Camera Offline
                swprintf_s(log_msg, L"  Successfully re-challenged with the camera disabled\n");
                isc_log_->LogError(L"IscMainControlImpl", log_msg);
            }
        }
    }

    isc_camera_control_->InitializeIscIamgeinfo(&isc_image_info_);

    // Width and height are available even if the camera is disabled
    int max_width = 0, max_height = 0;
    int ret = isc_camera_control_->DeviceGetOption(IscCameraInfo::kWidthMax, &max_width);
    if (ret != DPC_E_OK) {
        return ret;
    }
    ret = isc_camera_control_->DeviceGetOption(IscCameraInfo::kHeightMax, &max_height);
    if (ret != DPC_E_OK) {
        return ret;
    }

    memset(&temp_isc_grab_start_mode_, 0, sizeof(temp_isc_grab_start_mode_));

    // get Buffer
    isc_image_info_ring_buffer_ = new IscImageInfoRingBuffer;
    constexpr int max_buffer_count = 16;
    isc_image_info_ring_buffer_->Initialize(true, true, max_buffer_count, max_width, max_height);
    isc_image_info_ring_buffer_->Clear();

    // get work
    work_buffers_.max_width = max_width;
    work_buffers_.max_height = max_height;
    size_t work_buffer_frame_size = max_width * max_height;

    for (int i = 0; i < 4; i++) {
        work_buffers_.image_buffer[i] = new unsigned char[work_buffer_frame_size * 3];
        memset(work_buffers_.image_buffer[i], 0, work_buffer_frame_size * 3);

        work_buffers_.depth_buffer[i] = new float[work_buffer_frame_size];
        memset(work_buffers_.depth_buffer[i], 0, work_buffer_frame_size * sizeof(float));
    }

    // data processing library
    swprintf_s(log_msg, L"Open Data-Processing-Library Enabled=%d\n", ipc_dpl_configuratio_.enabled_data_proc_module);
    isc_log_->LogInfo(L"IscMainControlImpl", log_msg);
    
    IscDataProcModuleConfiguration isc_data_proc_module_configuration = {};

    swprintf_s(isc_data_proc_module_configuration.configuration_file_path, L"%s", ipc_dpl_configuratio_.configuration_file_path);
    swprintf_s(isc_data_proc_module_configuration.log_file_path, L"%s", ipc_dpl_configuratio_.log_file_path);
    isc_data_proc_module_configuration.log_level = ipc_dpl_configuratio_.log_level;
    isc_data_proc_module_configuration.isc_camera_model = ipc_dpl_configuratio_.isc_camera_model;

    isc_data_proc_module_configuration.max_image_width = max_width;
    isc_data_proc_module_configuration.max_image_height = max_height;

    isc_data_proc_module_configuration.enabled_data_proc_module = ipc_dpl_configuratio_.enabled_data_proc_module;

    isc_data_processing_control_ = new IscDataProcessingControl;
    isc_data_processing_control_->Initialize(&isc_data_proc_module_configuration);

    // Create Thread for camera
    thread_control_camera_.terminate_request = 0;
    thread_control_camera_.terminate_done = 0;
    thread_control_camera_.end_code = 0;
    thread_control_camera_.stop_request = false;

    char semaphoreName[64] = {};
    sprintf_s(semaphoreName, "THREAD_SEMAPHORENAME_ISCMAINCON_%d", 0);
    handle_semaphore_camera_ = CreateSemaphoreA(NULL, 0, 1, semaphoreName);
    if (handle_semaphore_camera_ == NULL) {
        // Fail
        return CAMCONTROL_E_INVALID_DEVICEHANDLE;
    }
    InitializeCriticalSection(&threads_critical_camera_);

    if ((thread_handle_camera_ = (HANDLE)_beginthreadex(0, 0, ControlThreadCamera, (void*)this, 0, 0)) == 0) {
        // Fail
        return CAMCONTROL_E_INVALID_DEVICEHANDLE;
    }

    // THREAD_PRIORITY_TIME_CRITICAL
    // THREAD_PRIORITY_HIGHEST +2
    // THREAD_PRIORITY_ABOVE_NORMAL +1
    // THREAD_PRIORITY_NORMAL  +0
    // THREAD_PRIORITY_BELOW_NORMAL -1
    if (thread_handle_camera_ != NULL) {
        SetThreadPriority(thread_handle_camera_, THREAD_PRIORITY_NORMAL);
    }

    swprintf_s(log_msg, L"Initialize ended (0x%08X)\n", ret_camera_open);
    isc_log_->LogInfo(L"IscMainControlImpl", log_msg);

    return ret_camera_open;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::Terminate()
{

    // close thread procedure
    thread_control_camera_.stop_request = true;
    thread_control_camera_.terminate_done = 0;
    thread_control_camera_.end_code = 0;
    thread_control_camera_.terminate_request = 1;

    // release any resources
    ReleaseSemaphore(handle_semaphore_camera_, 1, NULL);

    int count = 0;
    while (thread_control_camera_.terminate_done == 0) {
        if (count > 100) {
            break;
        }
        count++;
        Sleep(10L);
    }

    if (thread_handle_camera_ != NULL) {
        CloseHandle(thread_handle_camera_);
        thread_handle_camera_ = NULL;
    }
    if (handle_semaphore_camera_ != NULL) {
        CloseHandle(handle_semaphore_camera_);
        handle_semaphore_camera_ = NULL;
    }
    DeleteCriticalSection(&threads_critical_camera_);

    for (int i = 0; i < 4; i++) {
        delete[] work_buffers_.image_buffer[i];
        delete[] work_buffers_.depth_buffer[i];
    }
    work_buffers_.max_width = 0;
    work_buffers_.max_height = 0;

    if (isc_data_processing_control_ != nullptr) {
        isc_data_processing_control_->Terminate();
        delete isc_data_processing_control_;
        isc_data_processing_control_ = nullptr;
    }

    if (isc_image_info_ring_buffer_ != nullptr) {
        isc_image_info_ring_buffer_->Terminate();
        delete isc_image_info_ring_buffer_;
        isc_image_info_ring_buffer_ = nullptr;
    }

    isc_camera_control_->ReleaeIscIamgeinfo(&isc_image_info_);

    if (isc_camera_control_ != nullptr) {
        isc_camera_control_->Terminate();
        delete isc_camera_control_;
        isc_camera_control_ = nullptr;
    }

    if (isc_log_ != nullptr) {
        isc_log_->LogDebug(L"IscMainControlImpl", L"---Close log---\n");
        isc_log_->Close();
        delete isc_log_;
        isc_log_ = nullptr;
    }

	return DPC_E_OK;
}

/**
 * データ受信Thread.
 *
 * @param[in] context Thraed入力データ
 * @retval 0 成功
 * @retval other 失敗
 */
unsigned __stdcall IscMainControlImpl::ControlThreadCamera(void* context)
{
    IscMainControlImpl* isc_main_control_impl = (IscMainControlImpl*)context;

    if (isc_main_control_impl == nullptr) {
        return -1;
    }

    int ret = isc_main_control_impl->RecieveDataProcCamera();

    return ret;
}

/**
 * データ受信Thread　処理本体.
 *
 * @retval 0 成功
 * @retval other 失敗
 * @note
 *  - 以下の処理を行います  
 *    - カメラからのデータ受信  
 *    - データ処理呼び出し
      .
    .
 */
int IscMainControlImpl::RecieveDataProcCamera()
{

    IscCameraControl* isc_camera_control = isc_camera_control_;
    if (isc_camera_control == nullptr) {
        return -1;
    }

    bool show_elaped_time = false;
    TCHAR logMsg[256] = {};
    LARGE_INTEGER elp_before = {}, elp_now = {};
    LARGE_INTEGER start_tact1 = {}, end_tac1 = {};

    QueryPerformanceCounter(&elp_before);

    while (thread_control_camera_.terminate_request < 1) {

        // Wait for start
        WaitForSingleObject(handle_semaphore_camera_, INFINITE);

        for (;;) {
            if (thread_control_camera_.stop_request) {
                thread_control_camera_.stop_request = false;
                break;
            }

            // Check if there is space in the location to save the processing result
            IscImageInfoRingBuffer::BufferData* buffer_data = nullptr;
            const ULONGLONG time = GetTickCount64();
            int put_index = isc_image_info_ring_buffer_->GetPutBuffer(&buffer_data, time);
            int image_status = 0;

            if (put_index >= 0 && buffer_data != nullptr) {
                // There is space in the place to save

                // Check if there is data from the camera
                int camera_result = isc_camera_control_->GetData(&isc_image_info_);
                if (camera_result == DPC_E_OK) {
                    // I have data from the camera
                    if (show_elaped_time) {
                        QueryPerformanceCounter(&elp_now);
                        double elaped_time = (DWORD)((elp_now.QuadPart - elp_before.QuadPart) * 1000 / freq_for_performance_counter.QuadPart);
                        TCHAR msg[128] = {};
                        _stprintf_s(msg, _T("[IscMainControlImpl::RecieveDataProcCamera][INFO]RecieveDataProcCamera tact time=%.1f(ms)\n"), elaped_time);
                        OutputDebugString(msg);
                        QueryPerformanceCounter(&elp_before);
                    }

                    // start data processing
                    int dpc_result = isc_data_processing_control_->Run(&isc_image_info_);

                    // copy image data
                    buffer_data->isc_image_info.frameNo = isc_image_info_.frameNo;
                    buffer_data->isc_image_info.gain = isc_image_info_.gain;
                    buffer_data->isc_image_info.exposure = isc_image_info_.exposure;

                    buffer_data->isc_image_info.grab = isc_image_info_.grab;
                    buffer_data->isc_image_info.color_grab_mode = isc_image_info_.color_grab_mode;
                    buffer_data->isc_image_info.shutter_mode = isc_image_info_.shutter_mode;

                    buffer_data->isc_image_info.camera_specific_parameter.d_inf = isc_image_info_.camera_specific_parameter.d_inf;
                    buffer_data->isc_image_info.camera_specific_parameter.bf = isc_image_info_.camera_specific_parameter.bf;
                    buffer_data->isc_image_info.camera_specific_parameter.base_length = isc_image_info_.camera_specific_parameter.base_length;
                    buffer_data->isc_image_info.camera_specific_parameter.dz = isc_image_info_.camera_specific_parameter.dz;

                    buffer_data->isc_image_info.camera_status.error_code = isc_image_info_.camera_status.error_code;
                    buffer_data->isc_image_info.camera_status.data_receive_tact_time = isc_image_info_.camera_status.data_receive_tact_time;

                    buffer_data->isc_image_info.p1.width = isc_image_info_.p1.width;
                    buffer_data->isc_image_info.p1.height = isc_image_info_.p1.height;
                    buffer_data->isc_image_info.p1.channel_count = isc_image_info_.p1.channel_count;
                    size_t cp_size = isc_image_info_.p1.width * isc_image_info_.p1.height * isc_image_info_.p1.channel_count;
                    if (cp_size > 0) {
                        memcpy(buffer_data->isc_image_info.p1.image, isc_image_info_.p1.image, cp_size);
                    }

                    buffer_data->isc_image_info.p2.width = isc_image_info_.p2.width;
                    buffer_data->isc_image_info.p2.height = isc_image_info_.p2.height;
                    buffer_data->isc_image_info.p2.channel_count = isc_image_info_.p2.channel_count;
                    cp_size = isc_image_info_.p2.width * isc_image_info_.p2.height * isc_image_info_.p2.channel_count;
                    if (cp_size > 0) {
                        memcpy(buffer_data->isc_image_info.p2.image, isc_image_info_.p2.image, cp_size);
                    }

                    buffer_data->isc_image_info.color.width = isc_image_info_.color.width;
                    buffer_data->isc_image_info.color.height = isc_image_info_.color.height;
                    buffer_data->isc_image_info.color.channel_count = isc_image_info_.color.channel_count;
                    cp_size = isc_image_info_.color.width * isc_image_info_.color.height * isc_image_info_.color.channel_count;
                    if (cp_size > 0) {
                        memcpy(buffer_data->isc_image_info.color.image, isc_image_info_.color.image, cp_size);
                    }

                    buffer_data->isc_image_info.depth.width = isc_image_info_.depth.width;
                    buffer_data->isc_image_info.depth.height = isc_image_info_.depth.height;
                    cp_size = isc_image_info_.depth.width * isc_image_info_.depth.height * sizeof(float);
                    if (cp_size > 0) {
                        memcpy(buffer_data->isc_image_info.depth.image, isc_image_info_.depth.image, cp_size);
                    }

                    buffer_data->isc_image_info.raw.width = isc_image_info_.raw.width;
                    buffer_data->isc_image_info.raw.height = isc_image_info_.raw.height;
                    buffer_data->isc_image_info.raw.channel_count = isc_image_info_.raw.channel_count;
                    cp_size = isc_image_info_.raw.width * isc_image_info_.raw.height * isc_image_info_.raw.channel_count;
                    if (cp_size > 0) {
                        memcpy(buffer_data->isc_image_info.raw.image, isc_image_info_.raw.image, cp_size);
                    }

                    image_status = 1;
                }// if (camera_result == DPC_E_OK) {
                else {
                    // no data from camera
                }
            }// if (put_index >= 0 && buffer_data != nullptr) {
            else {
                // There is no space in the storage location
            }

            isc_image_info_ring_buffer_->DonePutBuffer(put_index, image_status);

        }//  for (;;) {

    }// while (thread_control_camera_.terminate_request < 1) {

    thread_control_camera_.terminate_done = 1;

    return 0;
}

// camera dependent paraneter

/**
 * 機能が実装されているかどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 実装されています
 * @retval false 実装されていません
 */
bool IscMainControlImpl::DeviceOptionIsImplemented(const IscCameraInfo option_name)
{
    if (isc_camera_control_ == nullptr) {
        return false;
    }
    
    bool ret = isc_camera_control_->DeviceOptionIsImplemented(option_name);
    
    return ret;
}

/**
 * 値を取得可能かどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 読み込み可能です
 * @retval false 読み込みはできません
 */
bool IscMainControlImpl::DeviceOptionIsReadable(const IscCameraInfo option_name)
{
    if (isc_camera_control_ == nullptr) {
        return false;
    }

    bool ret = isc_camera_control_->DeviceOptionIsReadable(option_name);

    return ret;
}

/**
 * 値を書き込み可能かどうかを確認します(IscCameraInfo)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 書き込み可能です
 * @retval false 書き込みはできません
 */
bool IscMainControlImpl::DeviceOptionIsWritable(const IscCameraInfo option_name)
{
    if (isc_camera_control_ == nullptr) {
        return false;
    }

    bool ret = isc_camera_control_->DeviceOptionIsWritable(option_name);

    return ret;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMin(const IscCameraInfo option_name, int* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMax(const IscCameraInfo option_name, int* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な増減値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionInc(const IscCameraInfo option_name, int* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionInc(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraInfo option_name, int* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/int)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraInfo option_name, const int value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMin(const IscCameraInfo option_name, float* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMax(const IscCameraInfo option_name, float* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraInfo option_name, float* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/int)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraInfo option_name, const float value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/bool)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraInfo option_name, bool* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/bool)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraInfo option_name, const bool value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/char)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @param[in] max_length valueバッファーの最大文字数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value, max_length);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/char)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraInfo option_name, const char* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な増減値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionInc(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraInfo option_name, uint64_t* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraInfo/uint64_t)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraInfo option_name, const uint64_t value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}


// camera control parameter

/**
 * 機能が実装されているかどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 実装されています
 * @retval false 実装されていません
 */
bool IscMainControlImpl::DeviceOptionIsImplemented(const IscCameraParameter option_name)
{
    if (isc_camera_control_ == nullptr) {
        return false;
    }

    bool ret = isc_camera_control_->DeviceOptionIsImplemented(option_name);

    return ret;
}

/**
 * 値を取得可能かどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 読み込み可能です
 * @retval false 読み込みはできません
 */
bool IscMainControlImpl::DeviceOptionIsReadable(const IscCameraParameter option_name)
{
    if (isc_camera_control_ == nullptr) {
        return false;
    }

    bool ret = isc_camera_control_->DeviceOptionIsReadable(option_name);

    return ret;
}

/**
 * 値を書き込み可能かどうかを確認します(IscCameraParameter)
 *
 * @param[in] option_name 確認する機能の名前
 * @retval true 書き込み可能です
 * @retval false 書き込みはできません
 */
bool IscMainControlImpl::DeviceOptionIsWritable(const IscCameraParameter option_name)
{
    if (isc_camera_control_ == nullptr) {
        return false;
    }

    bool ret = isc_camera_control_->DeviceOptionIsWritable(option_name);

    return ret;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMin(const IscCameraParameter option_name, int* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMax(const IscCameraParameter option_name, int* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な増減値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionInc(const IscCameraParameter option_name, int* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionInc(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/int)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraParameter option_name, int* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/int)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraParameter option_name, const int value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMin(const IscCameraParameter option_name, float* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMax(const IscCameraParameter option_name, float* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/float)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraParameter option_name, float* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/float)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraParameter option_name, const float value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/bool)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraParameter option_name, bool* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/bool)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraParameter option_name, const bool value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/char)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @param[in] max_length valueバッファーの最大文字数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value, max_length);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/char)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraParameter option_name, const char* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最小値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最小値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な最大値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 最大値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 設定可能な増減値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 増減値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOptionInc(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraParameter option_name, uint64_t* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/uint64_t)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraParameter option_name, const uint64_t value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を取得します(IscCameraParameter/IscShutterMode)
 *
 * @param[in] option_name 確認する機能の名前
 * @param[out] value 取得値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 値を設定します
 *
 * @param[in] option_name 確認する機能の名前(IscCameraParameter/IscShutterMode)
 * @param[out] value 設定値
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}


// grab control

/**
 * 取り込みを開始します
 *
 * @param[in] isc_start_mode 開始のためのパラメータ
 *
 * @retval 0 成功
 * @retval other 失敗
 * @details
 *  - カメラ又はファイルから取得可能です
 *  - 詳細は IscStartMode　を参照します
 */
int IscMainControlImpl::Start(const IscStartMode* isc_start_mode)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    // setup data processing
    const IscDataProcStartMode* isc_dataproc_start_mode = &isc_start_mode->isc_dataproc_start_mode;
    temp_isc_dataproc_start_mode_.enabled_block_matching = isc_dataproc_start_mode->enabled_block_matching;
    temp_isc_dataproc_start_mode_.enabled_frame_decoder = isc_dataproc_start_mode->enabled_frame_decoder;

    int ret = isc_data_processing_control_->Start(isc_dataproc_start_mode);
    if (ret != DPC_E_OK) {
        return ret;
    }

    // setup camera
    const IscGrabStartMode* isc_grab_start_mode = &isc_start_mode->isc_grab_start_mode;
    temp_isc_grab_start_mode_.isc_grab_mode = isc_grab_start_mode->isc_grab_mode;
    temp_isc_grab_start_mode_.isc_grab_color_mode = isc_grab_start_mode->isc_grab_color_mode;
    temp_isc_grab_start_mode_.isc_get_mode.wait_time = isc_grab_start_mode->isc_get_mode.wait_time;
    // Always enable RAW data for data processing modules
    temp_isc_grab_start_mode_.isc_get_raw_mode = IscGetModeRaw::kRawOn;  // isc_grab_start_mode->isc_get_raw_mode;
    temp_isc_grab_start_mode_.isc_get_color_mode = isc_grab_start_mode->isc_get_color_mode;

    temp_isc_grab_start_mode_.isc_record_mode = isc_grab_start_mode->isc_record_mode;
    temp_isc_grab_start_mode_.isc_play_mode = isc_grab_start_mode->isc_play_mode;
    temp_isc_grab_start_mode_.isc_play_mode_parameter.interval = isc_grab_start_mode->isc_play_mode_parameter.interval;
    swprintf_s(temp_isc_grab_start_mode_.isc_play_mode_parameter.play_file_name, L"%s", isc_grab_start_mode->isc_play_mode_parameter.play_file_name);

    isc_image_info_ring_buffer_->Clear();
    if (temp_isc_grab_start_mode_.isc_play_mode == IscPlayMode::kPlayOn) {
        // process all data in order
        isc_image_info_ring_buffer_->SetMode(false, false);
    }
    else {
        isc_image_info_ring_buffer_->SetMode(true, true);
    }

    // setup Occlusion, Peculiar
    if (ipc_dpl_configuratio_.enabled_camera) {
        if (temp_isc_dataproc_start_mode_.enabled_block_matching || temp_isc_dataproc_start_mode_.enabled_frame_decoder) {
            ret = isc_camera_control_->DeviceSetOption(IscCameraParameter::kOcclusionRemoval, (int)0);
            if (ret != DPC_E_OK) {
                return ret;
            }

            ret = isc_camera_control_->DeviceSetOption(IscCameraParameter::kPeculiarRemoval, (bool)false);
            if (ret != DPC_E_OK) {
                return ret;
            }
        }
        else {
            ret = isc_camera_control_->DeviceSetOption(IscCameraParameter::kOcclusionRemoval, (int)7);
            if (ret != DPC_E_OK) {
                return ret;
            }

            ret = isc_camera_control_->DeviceSetOption(IscCameraParameter::kPeculiarRemoval, (bool)true);
            if (ret != DPC_E_OK) {
                return ret;
            }
        }
    }

    ret = isc_camera_control_->Start(&temp_isc_grab_start_mode_);
    if (ret != DPC_E_OK) {
        return ret;
    }

    // it start main process thread
    ReleaseSemaphore(handle_semaphore_camera_, 1, NULL);

    return DPC_E_OK;
}

/**
 * 取り込みを停止します
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::Stop()
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->Stop();
    if (ret != DPC_E_OK) {
        return ret;
    }

    ret = isc_data_processing_control_->Stop();
    if (ret != DPC_E_OK) {
        return ret;
    }

    // it stop main process thread, then wait for Start()
    thread_control_camera_.stop_request = true;

    return DPC_E_OK;
}

/**
 * 現在の動作モードを取得します
 *
 * @param[in] isc_start_mode 現在のパラメータ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->GetGrabMode(isc_grab_start_mode);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}


// image & data get

/**
 * データ取得のためのバッファーを初期化します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::InitializeIscIamgeinfo(IscImageInfo* isc_image_Info)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->InitializeIscIamgeinfo(isc_image_Info);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * データ取得のためのバッファーを解放します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::ReleaeIscIamgeinfo(IscImageInfo* isc_image_Info)
{
    if (isc_camera_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_camera_control_->ReleaeIscIamgeinfo(isc_image_Info);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * データを取得します
 *
 * @param[in] isc_image_Info バッファー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetCameraData(IscImageInfo* isc_image_Info)
{
    if (isc_image_Info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    // get data
    IscImageInfoRingBuffer::BufferData* buffer_data = nullptr;
    ULONGLONG time = 0;
    int get_index = isc_image_info_ring_buffer_->GetGetBuffer(&buffer_data, &time);
 
    if (get_index < 0) {
        return CAMCONTROL_E_NO_IMAGE;
    }

    // copy data to result
    int ret = CopyIscImageInfo(isc_image_Info, &buffer_data->isc_image_info);

    isc_image_info_ring_buffer_->DoneGetBuffer(get_index);

    return DPC_E_OK;
}

/**
 * ファイルよりデータを取得する場合に、ヘッダーを取得します
 *
 * @param[in] play_file_name ファイル名
 * @param[out] raw_file_header ヘッダー構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header)
{
    if (play_file_name == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (raw_file_header == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_camera_control_->GetFileInformation(play_file_name, raw_file_header);
    if (ret != DPC_E_OK) {
        return ret;
    }
    
    return DPC_E_OK;
}

/**
 * IscImageInfoの内容をコピーします
 *
 * @param[out] dst_isc_image_Info コピー先
 * @param[in] src_isc_image_Info コピー元
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::CopyIscImageInfo(IscImageInfo* dst_isc_image_Info, IscImageInfo* src_isc_image_Info)
{
    // copy data to dst
    dst_isc_image_Info->frameNo = src_isc_image_Info->frameNo;
    dst_isc_image_Info->gain = src_isc_image_Info->gain;
    dst_isc_image_Info->exposure = src_isc_image_Info->exposure;
    dst_isc_image_Info->grab = src_isc_image_Info->grab;
    dst_isc_image_Info->color_grab_mode = src_isc_image_Info->color_grab_mode;
    dst_isc_image_Info->shutter_mode = src_isc_image_Info->shutter_mode;
    dst_isc_image_Info->camera_specific_parameter.d_inf = src_isc_image_Info->camera_specific_parameter.d_inf;
    dst_isc_image_Info->camera_specific_parameter.bf = src_isc_image_Info->camera_specific_parameter.bf;
    dst_isc_image_Info->camera_specific_parameter.base_length = src_isc_image_Info->camera_specific_parameter.base_length;
    dst_isc_image_Info->camera_specific_parameter.dz = src_isc_image_Info->camera_specific_parameter.dz;

    dst_isc_image_Info->camera_status.error_code = src_isc_image_Info->camera_status.error_code;
    dst_isc_image_Info->camera_status.data_receive_tact_time = src_isc_image_Info->camera_status.data_receive_tact_time;

    dst_isc_image_Info->p1.width = 0;
    dst_isc_image_Info->p1.height = 0;
    dst_isc_image_Info->p1.channel_count = 0;

    dst_isc_image_Info->p2.width = 0;
    dst_isc_image_Info->p2.height = 0;
    dst_isc_image_Info->p2.channel_count = 0;

    dst_isc_image_Info->color.width = 0;
    dst_isc_image_Info->color.height = 0;
    dst_isc_image_Info->color.channel_count = 0;

    dst_isc_image_Info->depth.width = 0;
    dst_isc_image_Info->depth.height = 0;

    dst_isc_image_Info->raw.width = 0;
    dst_isc_image_Info->raw.height = 0;
    dst_isc_image_Info->raw.channel_count = 0;

    dst_isc_image_Info->bayer_base.width = 0;
    dst_isc_image_Info->bayer_base.height = 0;
    dst_isc_image_Info->bayer_base.channel_count = 0;

    dst_isc_image_Info->bayer_compare.width = 0;
    dst_isc_image_Info->bayer_compare.height = 0;
    dst_isc_image_Info->bayer_compare.channel_count = 0;

    // p1
    dst_isc_image_Info->p1.width = src_isc_image_Info->p1.width;
    dst_isc_image_Info->p1.height = src_isc_image_Info->p1.height;
    dst_isc_image_Info->p1.channel_count = src_isc_image_Info->p1.channel_count;

    size_t copy_size = src_isc_image_Info->p1.width * src_isc_image_Info->p1.height * src_isc_image_Info->p1.channel_count;
    memcpy(dst_isc_image_Info->p1.image, src_isc_image_Info->p1.image, copy_size);

    // p2
    if (src_isc_image_Info->grab == IscGrabMode::kCorrect ||
        src_isc_image_Info->grab == IscGrabMode::kBeforeCorrect) {

        dst_isc_image_Info->p2.width = src_isc_image_Info->p2.width;
        dst_isc_image_Info->p2.height = src_isc_image_Info->p2.height;
        dst_isc_image_Info->p2.channel_count = src_isc_image_Info->p2.channel_count;

        copy_size = src_isc_image_Info->p2.width * src_isc_image_Info->p2.height * src_isc_image_Info->p2.channel_count;
        memcpy(dst_isc_image_Info->p2.image, src_isc_image_Info->p2.image, copy_size);
    }

    // color
    if (dst_isc_image_Info->color_grab_mode == IscGrabColorMode::kColorON) {
        if (src_isc_image_Info->color.width != 0 && src_isc_image_Info->color.height != 0 &&
            src_isc_image_Info->color.channel_count == 3) {

            dst_isc_image_Info->color.width = src_isc_image_Info->color.width;
            dst_isc_image_Info->color.height = src_isc_image_Info->color.height;
            dst_isc_image_Info->color.channel_count = src_isc_image_Info->color.channel_count;

            copy_size = src_isc_image_Info->color.width * src_isc_image_Info->color.height * src_isc_image_Info->color.channel_count;
            memcpy(dst_isc_image_Info->color.image, src_isc_image_Info->color.image, copy_size);
        }
    }

    // depth
    if (src_isc_image_Info->grab == IscGrabMode::kParallax) {
        if (src_isc_image_Info->depth.width != 0 && src_isc_image_Info->depth.height != 0) {

            dst_isc_image_Info->depth.width = src_isc_image_Info->depth.width;
            dst_isc_image_Info->depth.height = src_isc_image_Info->depth.height;

            copy_size = src_isc_image_Info->depth.width * src_isc_image_Info->depth.height * sizeof(float);
            memcpy(dst_isc_image_Info->depth.image, src_isc_image_Info->depth.image, copy_size);
        }
    }

    //raw
    if (src_isc_image_Info->raw.width != 0 && src_isc_image_Info->raw.height != 0) {

        dst_isc_image_Info->raw.width = src_isc_image_Info->raw.width;
        dst_isc_image_Info->raw.height = src_isc_image_Info->raw.height;
        dst_isc_image_Info->raw.channel_count = src_isc_image_Info->raw.channel_count;

        copy_size = src_isc_image_Info->raw.width * src_isc_image_Info->raw.height;
        memcpy(dst_isc_image_Info->raw.image, src_isc_image_Info->raw.image, copy_size);
    }

    // bayer
    if (src_isc_image_Info->grab == IscGrabMode::kBayerBase) {
        // bayer_base
        if (src_isc_image_Info->bayer_base.width != 0 && src_isc_image_Info->bayer_base.height != 0) {

            dst_isc_image_Info->bayer_base.width = src_isc_image_Info->bayer_base.width;
            dst_isc_image_Info->bayer_base.height = src_isc_image_Info->bayer_base.height;
            dst_isc_image_Info->bayer_base.channel_count = src_isc_image_Info->bayer_base.channel_count;

            copy_size = src_isc_image_Info->bayer_base.width * src_isc_image_Info->bayer_base.height;
            memcpy(dst_isc_image_Info->bayer_base.image, src_isc_image_Info->bayer_base.image, copy_size);
        }
    }
    else if (src_isc_image_Info->grab == IscGrabMode::kBayerCompare) {
        // bayer_compare
        if (src_isc_image_Info->bayer_compare.width != 0 && src_isc_image_Info->bayer_compare.height != 0) {

            dst_isc_image_Info->bayer_compare.width = src_isc_image_Info->bayer_compare.width;
            dst_isc_image_Info->bayer_compare.height = src_isc_image_Info->bayer_compare.height;
            dst_isc_image_Info->bayer_compare.channel_count = src_isc_image_Info->bayer_compare.channel_count;

            copy_size = src_isc_image_Info->bayer_compare.width * src_isc_image_Info->bayer_compare.height;
            memcpy(dst_isc_image_Info->bayer_compare.image, src_isc_image_Info->bayer_compare.image, copy_size);
        }
    }

    return DPC_E_OK;
}

// get information for depth, distance, ...

/**
 * 指定位置の視差と距離を取得します
 *
 * @param[in] x 画像内座標(X)
 * @param[in] y 画像内座標(Y)
 * @param[in] isc_image_info データ構造体
 * @param[out] disparity 視差
 * @param[out] depth 距離(m)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetPositionDepth(const int x, const int y, const IscImageInfo* isc_image_info, float* disparity, float* depth)
{

    if (isc_image_info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (disparity == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (depth == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int width = isc_image_info->depth.width;
    int height = isc_image_info->depth.height;

    if ((x < 0) || (x >= width)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if ((y < 0) || (y >= height)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    float disp = *(isc_image_info->depth.image + ((y * width) + x));

    if (disp > isc_image_info->camera_specific_parameter.d_inf) {
        *disparity = disp;
        *depth = isc_image_info->camera_specific_parameter.bf / (disp - isc_image_info->camera_specific_parameter.d_inf);
    }
    else {
        *disparity = 0;
        *depth = 0;
    }

    return DPC_E_OK;
}

/**
 * 指定位置の3D位置を取得します
 *
 * @param[in] x 画像内座標(X)
 * @param[in] y 画像内座標(Y)
 * @param[in] isc_image_info データ構造体
 * @param[out] x_d 画面中央からの距離(m)
 * @param[out] y_d 画面中央からの距離(m)
 * @param[out] z_d 距離(m)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetPosition3D(const int x, const int y, const IscImageInfo* isc_image_info, float* x_d, float* y_d, float* z_d)
{
    if (isc_image_info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (x_d == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (y_d == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (z_d == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int width = isc_image_info->depth.width;
    int height = isc_image_info->depth.height;

    if ((x < 0) || (x >= width)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if ((y < 0) || (y >= height)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    float disp = *(isc_image_info->depth.image + ((y * width) + x));

    if (disp > isc_image_info->camera_specific_parameter.d_inf) {
        float bd = isc_image_info->camera_specific_parameter.base_length / disp;

        *x_d = (float)(x - width / 2) * bd;
        *y_d = (float)(height / 2 - y) * bd;
        *z_d = isc_image_info->camera_specific_parameter.bf / (disp - isc_image_info->camera_specific_parameter.d_inf);

    }
    else {
        *x_d = 0;
        *y_d = 0;
        *z_d = 0;
    }

    return DPC_E_OK;
}

/**
 * 中央値を取得します
 *
 * @param[in] input_org 入力配列
 * @retval 中央値
 * @note 失敗時には、0を戻します
 */
float MedianMat(cv::Mat& input_org) {

    std::vector<float> vec_from_mat;

    for (int i = 0; i < input_org.rows; i++) {
        float* src = input_org.ptr<float>(i);

        for (int j = 0; j < input_org.cols; j++) {
            if (*src > 1) {
                vec_from_mat.push_back(*src);
            }
            src++;
        }
    }

    if (vec_from_mat.size() == 0) {
        return 0.0F;
    }

    //Input = Input.reshape(0, 1); // spread Input Mat to single row
    //Input.copyTo(vec_from_mat); // Copy Input Mat to vector vec_from_mat    

    // sort vec_from_mat
    std::sort(vec_from_mat.begin(), vec_from_mat.end());

    // in case of even-numbered matrix
    if (vec_from_mat.size() % 2 == 0) { 
        return (vec_from_mat[vec_from_mat.size() / 2 - 1] + vec_from_mat[vec_from_mat.size() / 2]) / 2; 
    } 
    
    // odd-number of elements in matrix
    return vec_from_mat[(vec_from_mat.size() - 1) / 2]; 
}

/**
 * 指定領域の情報を取得します
 *
 * @param[in] x 画像内座標左上(X)
 * @param[in] y 画像内座標左上(Y)
 * @param[in] width 幅
 * @param[in] height 高さ
 * @param[in] isc_image_info データ構造体
 * @param[out] isc_data_statistics 領域の情報
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetAreaStatistics(const int x, const int y, const int width, const int height, const IscImageInfo* isc_image_info, IscAreaDataStatistics* isc_data_statistics)
{
    
    if (isc_image_info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (isc_data_statistics == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    memset(isc_data_statistics, 0, sizeof(IscAreaDataStatistics));

    int image_width = isc_image_info->depth.width;
    int image_height = isc_image_info->depth.height;

    if ((x < 0) || (x >= image_width)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if ((y < 0) || (y >= image_height)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    cv::Rect roi = {};
    roi.x = x;
    roi.y = y;
    roi.width = x + width < image_width ? width : image_width - (x + width) - 1;
    roi.height = y + height < image_height ? height : image_height - (y + height) - 1;

    if (roi.width < 0) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (roi.height < 0) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    cv::Mat src_disparity(image_height, image_width, CV_32F, isc_image_info->depth.image);
    cv::Mat roi_depth = src_disparity(roi);

    // 視差の平均を計算

    const float valid_minimum = isc_image_info->camera_specific_parameter.d_inf;

    float sum_of_depth = 0;
    float max_of_depth = 0, min_of_depth = 999;

    unsigned int sum_of_depth_count = 0;
    for (int i = 0; i < roi_depth.rows; i++) {
        float* src = roi_depth.ptr<float>(i);

        for (int j = 0; j < roi_depth.cols; j++) {
            if (*src > valid_minimum) {
                sum_of_depth += *src;
                sum_of_depth_count++;

                if (*src > max_of_depth) {
                    max_of_depth = *src;
                }
                if (*src < min_of_depth) {
                    min_of_depth = *src;
                }
            }
            src++;
        }
    }

    float average_of_depth = 0;
    if (sum_of_depth_count != 0) {
        average_of_depth = sum_of_depth / (float)sum_of_depth_count;
    }

    // 中央値を計算
    float median_of_depth = 0;
    if (sum_of_depth_count != 0) {
        median_of_depth = MedianMat(roi_depth);
    }

    // 視差の標準偏差を計算
    float std_dev_of_depth = 0;
    if (sum_of_depth_count != 0) {

        float sum_of_mean_diff = 0;
        unsigned int sum_of_mean_diff_count = 0;
        for (int i = 0; i < roi_depth.rows; i++) {
            float* src = roi_depth.ptr<float>(i);

            for (int j = 0; j < roi_depth.cols; j++) {
                if (*src > valid_minimum) {
                    sum_of_mean_diff += ((*src - average_of_depth) * (*src - average_of_depth));
                    sum_of_mean_diff_count++;
                }
                src++;
            }
        }
        std_dev_of_depth = sqrt(sum_of_mean_diff / sum_of_mean_diff_count);
    }

    isc_data_statistics->x = x;
    isc_data_statistics->y = x;
    isc_data_statistics->width = roi_depth.cols;
    isc_data_statistics->height = roi_depth.rows;

    isc_data_statistics->statistics_depth.max_value = max_of_depth;
    isc_data_statistics->statistics_depth.min_value = min_of_depth;
    isc_data_statistics->statistics_depth.std_dev = std_dev_of_depth;
    isc_data_statistics->statistics_depth.average = average_of_depth;
    isc_data_statistics->statistics_depth.median = median_of_depth;

    float d_inf = isc_image_info->camera_specific_parameter.d_inf;
    float bf = isc_image_info->camera_specific_parameter.bf;
    float base_length = isc_image_info->camera_specific_parameter.base_length;
    if (average_of_depth > d_inf) {
        float bd = base_length / (average_of_depth - d_inf);
        isc_data_statistics->roi_3d.width = bd * isc_data_statistics->width;
        isc_data_statistics->roi_3d.height = bd * isc_data_statistics->height;
        isc_data_statistics->roi_3d.distance = bf / (average_of_depth - d_inf);
    }
    else {
        isc_data_statistics->roi_3d.width = 0;
        isc_data_statistics->roi_3d.height = 0;
        isc_data_statistics->roi_3d.distance = 0;
    }

    // convert to distance
    float sum_of_distance = 0;
    float max_of_distance = 0, min_of_distance = 99999;
    unsigned int sum_of_distance_count = 0;

    cv::Mat src_distance(roi_depth.rows, roi_depth.cols, CV_32F, work_buffers_.depth_buffer[0]);

    for (int i = 0; i < roi_depth.rows; i++) {
        float* src = roi_depth.ptr<float>(i);
        float* dst = src_distance.ptr<float>(i);

        for (int j = 0; j < roi_depth.cols; j++) {
            if (*src > valid_minimum) {

                float distance = bf / (*src - d_inf);
                *dst = distance;

                sum_of_distance += distance;
                sum_of_distance_count++;

                if (distance > max_of_distance) {
                    max_of_distance = distance;
                }
                if (distance < min_of_distance) {
                    min_of_distance = *src;
                }
            }
            else {
                *dst = 0;
            }
            src++;
            dst++;
        }
    }

    float average_of_distance = 0;
    if (sum_of_distance_count != 0) {
        average_of_distance = sum_of_distance / (float)sum_of_distance_count;
    }

    float median_of_distance = 0;
    if (sum_of_distance_count != 0) {
        median_of_distance = MedianMat(src_distance);
    }

    float std_dev_of_distance = 0;
    if (sum_of_distance_count != 0) {

        float sum_of_mean_diff = 0;
        unsigned int sum_of_mean_diff_count = 0;
        for (int i = 0; i < src_distance.rows; i++) {
            float* src = src_distance.ptr<float>(i);

            for (int j = 0; j < src_distance.cols; j++) {
                if (*src > 0) {
                    sum_of_mean_diff += ((*src - average_of_distance) * (*src - average_of_distance));
                    sum_of_mean_diff_count++;
                }
                src++;
            }
        }
        std_dev_of_distance = sqrt(sum_of_mean_diff / sum_of_mean_diff_count);
    }

    isc_data_statistics->statistics_distance.max_value = max_of_distance;
    isc_data_statistics->statistics_distance.min_value = min_of_distance;
    isc_data_statistics->statistics_distance.std_dev = std_dev_of_distance;
    isc_data_statistics->statistics_distance.average = average_of_distance;
    isc_data_statistics->statistics_distance.median = median_of_distance;

    return DPC_E_OK;
}

// data processing module settings

/**
 * 利用可能なデータ処理モジュールの数を取得します
 *
 * @param[out] total_count モジュール数
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetTotalModuleCount(int* total_count)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (total_count == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->GetTotalModuleCount(total_count);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 利用可能なデータ処理モジュールの数を取得します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[in] max_length バッファーの最大文字数
 * @param[out] module_name モジュール名
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetModuleNameByIndex(const int module_index, wchar_t* module_name, int max_length)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (module_name == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (max_length == 0) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->GetModuleNameByIndex(module_index, module_name, max_length);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 指定したモジュールのパラメータを取得します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[out] isc_data_proc_module_parameter 取得したパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_module_parameter == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->GetParameter(module_index, isc_data_proc_module_parameter);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 指定したモジュールへパラメータを設定します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[out] isc_data_proc_module_parameter 設定するパラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::SetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_module_parameter == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->SetParameter(module_index, isc_data_proc_module_parameter, is_update_file);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 指定したモジュールのパラメータファイルのファイル名を取得します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[in] max_length バッファーの最大文字数
 * @param[out] file_name パラメータファイル名
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::GetParameterFileName(const int module_index, wchar_t* file_name, const int max_length)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (file_name == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->GetParameterFileName(module_index, file_name, max_length);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * 指定したモジュールへファイルからパラメータの読み込みを指示します
 *
 * @param[in] module_index 取得するモジュールの番号
 * @param[in] file_name パラメータファイル名
 * @param[in] is_valid パラメータを反映させるかどうかを指定します true:即時反映
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::ReloadParameterFromFile(const int module_index, const wchar_t* file_name, const bool is_valid)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (file_name == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->ReloadParameterFromFile(module_index, file_name, is_valid);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

// data processing module result data

/**
 * モジュールの処理結果バッファーを初期化します
 *
 * @param[in] isc_data_proc_result_data 処理結果バッファー
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMainControlImpl::InitializeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_result_data == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->InitializeIscDataProcResultData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return ret;
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
int IscMainControlImpl::ReleaeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_result_data == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->ReleaeIscDataProcResultData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return ret;
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
int IscMainControlImpl::GetDataProcModuleData(IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_data_processing_control_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_result_data == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_data_processing_control_->GetDataProcModuleData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}
