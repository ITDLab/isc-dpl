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
 * @file isc_main_control.cpp
 * @brief main interface control class for ISC_DPL
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a interface function for ISC_DPL.
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

#include "isc_main_control.h"

/**
 * constructor
 *
 */
IscMainControl::IscMainControl():
	isc_main_control_impl_(nullptr)
{

}

/**
 * destructor
 *
 */
IscMainControl::~IscMainControl()
{

}

/**
 * �N���X�����������܂�.
 *
 * @param[in] ipc_dpl_configuratio �������p�����[�^�\����
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::Initialize(const IscDplConfiguration* ipc_dpl_configuratio)
{
	if (isc_main_control_impl_ != nullptr) {
		return ISCDPL_E_OPVERLAPED_OPERATION;
	}

	isc_main_control_impl_ = new IscMainControlImpl;
	int ret = isc_main_control_impl_->Initialize(ipc_dpl_configuratio);
	if (ret != DPC_E_OK) {
		return ret;
	}

	return DPC_E_OK;
}

/**
 * �I�����������܂�.
 *
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::Terminate()
{
	if (isc_main_control_impl_ == nullptr) {
		return ISCDPL_E_INVALID_HANDLE;
	}

    isc_main_control_impl_->Terminate();
	delete isc_main_control_impl_;
	isc_main_control_impl_ = nullptr;

	return DPC_E_OK;
}

// camera dependent paraneter

/**
 * �@�\����������Ă��邩�ǂ������m�F���܂�(IscCameraInfo)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @retval true ��������Ă��܂�
 * @retval false ��������Ă��܂���
 */
bool IscMainControl::DeviceOptionIsImplemented(const IscCameraInfo option_name)
{
    if (isc_main_control_impl_ == nullptr) {
        return false;
    }

    bool ret = isc_main_control_impl_->DeviceOptionIsImplemented(option_name);

    return ret;
}

/**
 * �l���擾�\���ǂ������m�F���܂�(IscCameraInfo)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @retval true �ǂݍ��݉\�ł�
 * @retval false �ǂݍ��݂͂ł��܂���
 */
bool IscMainControl::DeviceOptionIsReadable(const IscCameraInfo option_name)
{
    if (isc_main_control_impl_ == nullptr) {
        return false;
    }

    bool ret = isc_main_control_impl_->DeviceOptionIsReadable(option_name);

    return ret;
}

/**
 * �l���������݉\���ǂ������m�F���܂�(IscCameraInfo)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @retval true �������݉\�ł�
 * @retval false �������݂͂ł��܂���
 */
bool IscMainControl::DeviceOptionIsWritable(const IscCameraInfo option_name)
{
    if (isc_main_control_impl_ == nullptr) {
        return false;
    }

    bool ret = isc_main_control_impl_->DeviceOptionIsWritable(option_name);

    return ret;
}

/**
 * �ݒ�\�ȍŏ��l���擾���܂�(IscCameraInfo/int)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ŏ��l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMin(const IscCameraInfo option_name, int* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍő�l���擾���܂�(IscCameraInfo/int)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ő�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMax(const IscCameraInfo option_name, int* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȑ����l���擾���܂�(IscCameraInfo/int)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �����l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionInc(const IscCameraInfo option_name, int* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionInc(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraInfo/int)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraInfo option_name, int* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraInfo/int)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraInfo option_name, const int value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍŏ��l���擾���܂�(IscCameraInfo/float)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ŏ��l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMin(const IscCameraInfo option_name, float* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍő�l���擾���܂�(IscCameraInfo/float)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ő�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMax(const IscCameraInfo option_name, float* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraInfo/float)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraInfo option_name, float* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraInfo/float)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraInfo option_name, const float value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraInfo/bool)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraInfo option_name, bool* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraInfo/bool)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraInfo option_name, const bool value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraInfo/char)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @param[in] max_length value�o�b�t�@�[�̍ő啶����
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value, max_length);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraInfo/char)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraInfo option_name, const char* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍŏ��l���擾���܂�(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ŏ��l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍő�l���擾���܂�(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ő�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȑ����l���擾���܂�(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �����l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionInc(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraInfo/uint64_t)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraInfo option_name, uint64_t* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraInfo/uint64_t)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraInfo option_name, const uint64_t value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}


// camera control parameter

/**
 * �@�\����������Ă��邩�ǂ������m�F���܂�(IscCameraInfo)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @retval true ��������Ă��܂�
 * @retval false ��������Ă��܂���
 */
bool IscMainControl::DeviceOptionIsImplemented(const IscCameraParameter option_name)
{
    if (isc_main_control_impl_ == nullptr) {
        return false;
    }

    bool ret = isc_main_control_impl_->DeviceOptionIsImplemented(option_name);

    return ret;
}

/**
 * �l���擾�\���ǂ������m�F���܂�(IscCameraInfo)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @retval true �ǂݍ��݉\�ł�
 * @retval false �ǂݍ��݂͂ł��܂���
 */
bool IscMainControl::DeviceOptionIsReadable(const IscCameraParameter option_name)
{
    if (isc_main_control_impl_ == nullptr) {
        return false;
    }

    bool ret = isc_main_control_impl_->DeviceOptionIsReadable(option_name);

    return ret;
}

/**
 * �l���������݉\���ǂ������m�F���܂�(IscCameraInfo)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @retval true �������݉\�ł�
 * @retval false �������݂͂ł��܂���
 */
bool IscMainControl::DeviceOptionIsWritable(const IscCameraParameter option_name)
{
    if (isc_main_control_impl_ == nullptr) {
        return false;
    }

    bool ret = isc_main_control_impl_->DeviceOptionIsWritable(option_name);

    return ret;
}

/**
 * �ݒ�\�ȍŏ��l���擾���܂�(IscCameraParameter/int)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ŏ��l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMin(const IscCameraParameter option_name, int* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍő�l���擾���܂�(IscCameraParameter/int)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ő�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMax(const IscCameraParameter option_name, int* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȑ����l���擾���܂�(IscCameraParameter/int)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �����l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionInc(const IscCameraParameter option_name, int* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionInc(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraParameter/int)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraParameter option_name, int* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraParameter/int)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraParameter option_name, const int value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍŏ��l���擾���܂�(IscCameraParameter/float)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ŏ��l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMin(const IscCameraParameter option_name, float* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍő�l���擾���܂�(IscCameraParameter/float)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ő�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMax(const IscCameraParameter option_name, float* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraParameter/float)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraParameter option_name, float* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraParameter/float)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraParameter option_name, const float value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraParameter/bool)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraParameter option_name, bool* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraParameter/bool)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraParameter option_name, const bool value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraParameter/char)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @param[in] max_length value�o�b�t�@�[�̍ő啶����
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value, max_length);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraParameter/char)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraParameter option_name, const char* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍŏ��l���擾���܂�(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ŏ��l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMin(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȍő�l���擾���܂�(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �ő�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionMax(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �ݒ�\�ȑ����l���擾���܂�(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �����l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOptionInc(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraParameter/uint64_t)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraParameter option_name, uint64_t* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraParameter/uint64_t)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraParameter option_name, const uint64_t value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l���擾���܂�(IscCameraParameter/IscShutterMode)
 *
 * @param[in] option_name �m�F����@�\�̖��O
 * @param[out] value �擾�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceGetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �l��ݒ肵�܂�
 *
 * @param[in] option_name �m�F����@�\�̖��O(IscCameraParameter/IscShutterMode)
 * @param[out] value �ݒ�l
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->DeviceSetOption(option_name, value);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}


// grab control

/**
 * ��荞�݂��J�n���܂�
 *
 * @param[in] isc_start_mode �J�n�̂��߂̃p�����[�^
 *
 * @retval 0 ����
 * @retval other ���s
 * @details
 *  - �J�������̓t�@�C������擾�\�ł�
 *  - �ڍׂ� IscStartMode�@���Q�Ƃ��܂�
 */
int IscMainControl::Start(const IscStartMode* isc_start_mode)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->Start(isc_start_mode);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * ��荞�݂��~���܂�
 *
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::Stop()
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->Stop();
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * ���݂̓��샂�[�h���擾���܂�
 *
 * @param[in] isc_start_mode ���݂̃p�����[�^
 *
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetGrabMode(IscGrabStartMode* isc_grab_start_mode)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->GetGrabMode(isc_grab_start_mode);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}


// image & data get

/**
 * �f�[�^�擾�̂��߂̃o�b�t�@�[�����������܂�
 *
 * @param[in] isc_image_Info �o�b�t�@�[�\����
 *
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::InitializeIscIamgeinfo(IscImageInfo* isc_image_Info)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->InitializeIscIamgeinfo(isc_image_Info);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �f�[�^�擾�̂��߂̃o�b�t�@�[��������܂�
 *
 * @param[in] isc_image_Info �o�b�t�@�[�\����
 *
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::ReleaeIscIamgeinfo(IscImageInfo* isc_image_Info)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    int ret = isc_main_control_impl_->ReleaeIscIamgeinfo(isc_image_Info);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �f�[�^���擾���܂�
 *
 * @param[in] isc_image_Info �o�b�t�@�[�\����
 *
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetCameraData(IscImageInfo* isc_image_Info)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_image_Info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->GetCameraData(isc_image_Info);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �t�@�C�����f�[�^���擾����ꍇ�ɁA�w�b�_�[���擾���܂�
 *
 * @param[in] play_file_name �t�@�C����
 * @param[out] raw_file_header �w�b�_�[�\����
 *
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (play_file_name == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (raw_file_header == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->GetFileInformation(play_file_name, raw_file_header);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

// get information for depth, distance, ...

/**
 * �w��ʒu�̎����Ƌ������擾���܂�
 *
 * @param[in] x �摜�����W(X)
 * @param[in] y �摜�����W(Y)
 * @param[in] isc_image_info �f�[�^�\����
 * @param[out] disparity ����
 * @param[out] depth ����(m)
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetPositionDepth(const int x, const int y, const IscImageInfo* isc_image_info, float* disparity, float* depth)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_image_info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (disparity == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }
    
    if (depth == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }
    
    int ret = isc_main_control_impl_->GetPositionDepth(x, y, isc_image_info, disparity, depth);
    if (ret != DPC_E_OK) {
        return ret;
    }
    
    return DPC_E_OK;
}

/**
 * �w��ʒu��3D�ʒu���擾���܂�
 *
 * @param[in] x �摜�����W(X)
 * @param[in] y �摜�����W(Y)
 * @param[in] isc_image_info �f�[�^�\����
 * @param[out] x_d ��ʒ�������̋���(m)
 * @param[out] y_d ��ʒ�������̋���(m)
 * @param[out] z_d ����(m)
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetPosition3D(const int x, const int y, const IscImageInfo* isc_image_info, float* x_d, float* y_d, float* z_d)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

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

    int ret = isc_main_control_impl_->GetPosition3D(x, y, isc_image_info, x_d, y_d, z_d);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �w��̈�̏����擾���܂�
 *
 * @param[in] x �摜�����W����(X)
 * @param[in] y �摜�����W����(Y)
 * @param[in] width ��
 * @param[in] height ����
 * @param[in] isc_image_info �f�[�^�\����
 * @param[out] isc_data_statistics �̈�̏��
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetAreaStatistics(const int x, const int y, const int width, const int height, const IscImageInfo* isc_image_info, IscAreaDataStatistics* isc_data_statistics)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_image_info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (isc_data_statistics == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->GetAreaStatistics(x, y, width, height, isc_image_info, isc_data_statistics);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

// data processing module settings

/**
 * ���p�\�ȃf�[�^�������W���[���̐����擾���܂�
 *
 * @param[out] total_count ���W���[����
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetTotalModuleCount(int* total_count)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (total_count == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->GetTotalModuleCount(total_count);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * ���p�\�ȃf�[�^�������W���[���̐����擾���܂�
 *
 * @param[in] module_index �擾���郂�W���[���̔ԍ�
 * @param[in] max_length �o�b�t�@�[�̍ő啶����
 * @param[out] module_name ���W���[����
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetModuleNameByIndex(const int module_index, wchar_t* module_name, int max_length)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (module_name == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (max_length == 0) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->GetModuleNameByIndex(module_index, module_name, max_length);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �w�肵�����W���[���̃p�����[�^���擾���܂�
 *
 * @param[in] module_index �擾���郂�W���[���̔ԍ�
 * @param[out] isc_data_proc_module_parameter �擾�����p�����[�^
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_module_parameter == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->GetDataProcModuleParameter(module_index, isc_data_proc_module_parameter);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �w�肵�����W���[���փp�����[�^��ݒ肵�܂�
 *
 * @param[in] module_index �擾���郂�W���[���̔ԍ�
 * @param[out] isc_data_proc_module_parameter �ݒ肷��p�����[�^
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::SetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_module_parameter == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->SetDataProcModuleParameter(module_index, isc_data_proc_module_parameter, is_update_file);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �w�肵�����W���[���̃p�����[�^�t�@�C���̃t�@�C�������擾���܂�
 *
 * @param[in] module_index �擾���郂�W���[���̔ԍ�
 * @param[in] max_length �o�b�t�@�[�̍ő啶����
 * @param[out] file_name �p�����[�^�t�@�C����
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetParameterFileName(const int module_index, wchar_t* file_name, const int max_length)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (file_name == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->GetParameterFileName(module_index, file_name, max_length);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * �w�肵�����W���[���փt�@�C������p�����[�^�̓ǂݍ��݂��w�����܂�
 *
 * @param[in] module_index �擾���郂�W���[���̔ԍ�
 * @param[in] file_name �p�����[�^�t�@�C����
 * @param[in] is_valid �p�����[�^�𔽉f�����邩�ǂ������w�肵�܂� true:�������f
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::ReloadParameterFromFile(const int module_index, const wchar_t* file_name, const bool is_valid)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (file_name == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->ReloadParameterFromFile(module_index, file_name, is_valid);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

// data processing module result data

/**
 * ���W���[���̏������ʃo�b�t�@�[�����������܂�
 *
 * @param[in] isc_data_proc_result_data �������ʃo�b�t�@�[
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::InitializeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }
    
    if (isc_data_proc_result_data == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->InitializeIscDataProcResultData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * ���W���[���̏������ʃo�b�t�@�[��������܂�
 *
 * @param[in] isc_data_proc_result_data �������ʃo�b�t�@�[
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::ReleaeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_result_data == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->ReleaeIscDataProcResultData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

/**
 * ���W���[���̏������ʂ��擾���܂�
 *
 * @param[in] isc_data_proc_result_data �������ʃo�b�t�@�[
 * @retval 0 ����
 * @retval other ���s
 */
int IscMainControl::GetDataProcModuleData(IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_main_control_impl_ == nullptr) {
        return ISCDPL_E_INVALID_HANDLE;
    }

    if (isc_data_proc_result_data == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int ret = isc_main_control_impl_->GetDataProcModuleData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return ret;
    }

    return DPC_E_OK;
}

