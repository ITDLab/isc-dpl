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
 * @file isc_dataproc_resultdata_ring_buffer.cpp
 * @brief implementation class of buffer camera data
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a buffer for using camera.
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"

#include "isc_dataproc_resultdata_ring_buffer.h"

/**
 * constructor
 *
 */
IscDataprocResultdataRingBuffer::IscDataprocResultdataRingBuffer():
	last_mode_(false), allow_overwrite_(false), buffer_count_(0), width_(0), height_(0), channel_count_(0), buffer_data_(nullptr),
	write_inex_(0), read_index_(0), put_index_(0), geted_inedx_(0),
	buff_p1_(nullptr), buff_p2_(nullptr), buff_color_(nullptr),
	buff_depth_(nullptr), buff_raw_(nullptr), buff_raw_color_(nullptr), 
	flag_critical_()
{
}

/**
 * destructor
 *
 */
IscDataprocResultdataRingBuffer::~IscDataprocResultdataRingBuffer()
{
}

/**
 * バッファーを初期化します.
 *
 * @param[in] last_mpde true:最新のものを取得します false:FIFOです
 * @param[in] allow_overwrite true:オーバーフロー時に上書きします
 * @param[in] count リングバッファーの深さです
 * @param[in] width_def データ幅
 * @param[in] height_def データ高さ
 * @param[in] channel_count_def データ色深度
 *
 * @retval 0 成功
 * @retval -1 失敗　
 */
int IscDataprocResultdataRingBuffer::Initialize(const bool last_mpde, const bool allow_overwrite, const int count, const int width_def, const int height_def, const int channel_count_def)
{
	last_mode_ = last_mpde;
	allow_overwrite_ = allow_overwrite;
	buffer_count_ = count;
	width_ = width_def;
	height_ = height_def;
	channel_count_ = channel_count_def;
	write_inex_ = 0; read_index_ = 0; put_index_ = 0;  geted_inedx_ = 0;

	InitializeCriticalSection(&flag_critical_);

	buffer_data_ = new BufferData[buffer_count_];

	const size_t one_frame_size = width_ * height_;
	const int max_fd_count = kISCIMAGEINFO_FRAMEDATA_MAX_COUNT;

	buff_p1_			= new unsigned char[buffer_count_ * one_frame_size * max_fd_count];
	buff_p2_			= new unsigned char[buffer_count_ * one_frame_size * max_fd_count];
	buff_color_			= new unsigned char[buffer_count_ * one_frame_size * 3 * max_fd_count];
	buff_depth_			= new float[buffer_count_ * one_frame_size * max_fd_count];
	buff_raw_			= new unsigned char[buffer_count_ * one_frame_size * 2 * max_fd_count];
	buff_raw_color_		= new unsigned char[buffer_count_ * one_frame_size * 2 * max_fd_count];

	memset(buff_p1_,			0, buffer_count_ * one_frame_size * max_fd_count);
	memset(buff_p2_,			0, buffer_count_ * one_frame_size * max_fd_count);
	memset(buff_color_,			0, buffer_count_ * one_frame_size * 3 * max_fd_count);
	memset(buff_depth_,			0, buffer_count_ * one_frame_size * sizeof(float) * max_fd_count);
	memset(buff_raw_,			0, buffer_count_ * one_frame_size * 2 * max_fd_count);
	memset(buff_raw_color_,		0, buffer_count_ * one_frame_size * 2 * max_fd_count);

	for (int i = 0; i < buffer_count_; i++) {
		buffer_data_[i].inedx = i;
		buffer_data_[i].state = 0;
		buffer_data_[i].time = 0;

		buffer_data_[i].isc_dataproc_resultdata.number_of_modules_processed = 0;
		buffer_data_[i].isc_dataproc_resultdata.maximum_number_of_modules = 4;
		buffer_data_[i].isc_dataproc_resultdata.maximum_number_of_modulename = 32;

		buffer_data_[i].isc_dataproc_resultdata.status.error_code = 0;
		buffer_data_[i].isc_dataproc_resultdata.status.proc_tact_time = 0;

		for (int j = 0; j < buffer_data_[i].isc_dataproc_resultdata.maximum_number_of_modules; j++) {
			sprintf_s(	buffer_data_[i].isc_dataproc_resultdata.module_status[j].module_names, "\n");
			buffer_data_[i].isc_dataproc_resultdata.module_status[j].error_code = 0;
			buffer_data_[i].isc_dataproc_resultdata.module_status[j].processing_time = 0;
		}

		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.grab = IscGrabMode::kParallax;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.color_grab_mode = IscGrabColorMode::kColorOFF;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.shutter_mode = IscShutterMode::kManualShutter;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.camera_specific_parameter.d_inf = 0;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.camera_specific_parameter.bf = 0;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.camera_specific_parameter.base_length = 0;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.camera_specific_parameter.dz = 0;

		for (int j = 0; j < max_fd_count; j++) {
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].data_index = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].frameNo = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].gain = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].exposure = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].camera_status.error_code = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].camera_status.data_receive_tact_time = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p1.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p1.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p1.channel_count = 0;
			size_t unit = one_frame_size;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p1.image = buff_p1_ + (unit * ((i * max_fd_count) + j));

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p2.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p2.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p2.channel_count = 0;
			unit = one_frame_size;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p2.image = buff_p2_ + (unit * ((i * max_fd_count) + j));

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].color.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].color.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].color.channel_count = 0;
			unit = one_frame_size * 3;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].color.image = buff_color_ + (unit * ((i * max_fd_count) + j));

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].depth.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].depth.height = 0;
			unit = one_frame_size;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].depth.image = buff_depth_ + (unit * ((i * max_fd_count) + j));

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw.channel_count = 0;
			unit = one_frame_size * 2;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw.image = buff_raw_ + (unit * ((i * max_fd_count) + j));

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw_color.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw_color.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw_color.channel_count = 0;
			unit = one_frame_size * 2;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw_color.image = buff_raw_color_ + (unit * ((i * max_fd_count) + j));

		}
	}

	return 0;
}

/**
 * 各変変数を初期化します.
 *
 *
 * @return none.
 */
int IscDataprocResultdataRingBuffer::Clear()
{
	write_inex_ = 0; read_index_ = 0; put_index_ = 0;  geted_inedx_ = 0;

	const size_t one_frame_size = width_ * height_;
	const int max_fd_count = kISCIMAGEINFO_FRAMEDATA_MAX_COUNT;

	memset(buff_p1_,			0, buffer_count_ * one_frame_size * max_fd_count);
	memset(buff_p2_,			0, buffer_count_ * one_frame_size * max_fd_count);
	memset(buff_color_,			0, buffer_count_ * one_frame_size * 3 * max_fd_count);
	memset(buff_depth_,			0, buffer_count_ * one_frame_size * sizeof(float) * max_fd_count);
	memset(buff_raw_,			0, buffer_count_ * one_frame_size * 2 * max_fd_count);
	memset(buff_raw_color_,		0, buffer_count_ * one_frame_size * 2 * max_fd_count);

	for (int i = 0; i < buffer_count_; i++) {
		buffer_data_[i].inedx = i;
		buffer_data_[i].state = 0;
		buffer_data_[i].time = 0;

		buffer_data_[i].isc_dataproc_resultdata.number_of_modules_processed = 0;
		buffer_data_[i].isc_dataproc_resultdata.maximum_number_of_modules = 4;
		buffer_data_[i].isc_dataproc_resultdata.maximum_number_of_modulename = 32;

		buffer_data_[i].isc_dataproc_resultdata.status.error_code = 0;
		buffer_data_[i].isc_dataproc_resultdata.status.proc_tact_time = 0;

		for (int j = 0; j < buffer_data_[i].isc_dataproc_resultdata.maximum_number_of_modules; j++) {
			sprintf_s(buffer_data_[i].isc_dataproc_resultdata.module_status[j].module_names, "\n");
			buffer_data_[i].isc_dataproc_resultdata.module_status[j].error_code = 0;
			buffer_data_[i].isc_dataproc_resultdata.module_status[j].processing_time = 0;
		}

		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.grab = IscGrabMode::kParallax;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.color_grab_mode = IscGrabColorMode::kColorOFF;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.shutter_mode = IscShutterMode::kManualShutter;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.camera_specific_parameter.d_inf = 0;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.camera_specific_parameter.bf = 0;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.camera_specific_parameter.base_length = 0;
		buffer_data_[i].isc_dataproc_resultdata.isc_image_info.camera_specific_parameter.dz = 0;

		for (int j = 0; j < max_fd_count; j++) {
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].data_index = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].frameNo = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].gain = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].exposure = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].camera_status.error_code = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].camera_status.data_receive_tact_time = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p1.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p1.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p1.channel_count = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p2.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p2.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].p2.channel_count = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].color.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].color.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].color.channel_count = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].depth.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].depth.height = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw.channel_count = 0;

			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw_color.width = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw_color.height = 0;
			buffer_data_[i].isc_dataproc_resultdata.isc_image_info.frame_data[j].raw_color.channel_count = 0;

		}
	}

	return 0;
}

/**
 * 終了します.
 *
 *
 * @return none.
 */
int IscDataprocResultdataRingBuffer::Terminate()
{
	delete[] buff_p1_;
	delete[] buff_p2_;
	delete[] buff_color_;
	delete[] buff_depth_;
	delete[] buff_raw_;
	delete[] buff_raw_color_;

	buff_p1_ = nullptr;
	buff_p2_ = nullptr;
	buff_color_ = nullptr;
	buff_depth_ = nullptr;
	buff_raw_ = nullptr;
	buff_raw_color_ = nullptr;

	delete buffer_data_;
	buffer_data_ = nullptr;

	DeleteCriticalSection(&flag_critical_);


	return 0;
}

/**
 * 書き込み対象のバッファーのポインタを取得します.
 *
 * @param[in/out] buffer_data バッファーのポインタを書き込みます
 * @param[in] time 現在時間です
 *
 * @retval >0 バッファーのIndex
 * @retval -1 失敗　空きバッファー無し
 */
int IscDataprocResultdataRingBuffer::GetPutBuffer(BufferData** buffer_data, const ULONGLONG time)
{
	if (buffer_data_ == nullptr) {
		return -1;
	}

	EnterCriticalSection(&flag_critical_);

	int local_write_inex = write_inex_;

	if (buffer_data_[local_write_inex].state == 3) {
		// in use for Get...
		LeaveCriticalSection(&flag_critical_);
		return -1;
	}

	if (!allow_overwrite_) {
		if (buffer_data_[local_write_inex].state != 0) {
			LeaveCriticalSection(&flag_critical_);
			return -1;
		}
	}

	*buffer_data = &buffer_data_[local_write_inex];

	buffer_data_[local_write_inex].time = time;	// GetTickCount();
	buffer_data_[local_write_inex].state = 1;
	put_index_ = local_write_inex;

	LeaveCriticalSection(&flag_critical_);

	return put_index_;
}

/**
 * 取得したバッファーの使用を終了します.
 *
 * @param[in] index バッファーのIndexです。取得時のものです
 * @param[in] status 1:有効です
 *
 * @retval 0 成功
 * @retval -1 失敗　
 */
int IscDataprocResultdataRingBuffer::DonePutBuffer(const int index, const int status)
{
	// status 0:This data is invalid 1:This data is valid

	if (buffer_data_ == nullptr) {
		return -1;
	}

	if (index < 0 || index >= buffer_count_) {
		return -1;
	}

	if (index != put_index_) {
		// Get and Done must correspond one-to-one on the same Thread
		return -1;
	}

	EnterCriticalSection(&flag_critical_);

	if (buffer_data_[index].state != 1) {
		// error, this case should not exist
		__debugbreak();
		LeaveCriticalSection(&flag_critical_);
		return -1;
	}

	if (status == 0) {
		// it change 1 -> 0 (not use)
		buffer_data_[index].state = 0;
	}
	else {
		buffer_data_[index].state = 2;

		if (last_mode_) {
			read_index_ = index;
		}

		write_inex_++;
		if (write_inex_ >= buffer_count_) {
			write_inex_ = 0;
		}
	}

	LeaveCriticalSection(&flag_critical_);

	return 0;
}


/**
 * 読み込み対象のバッファーのポインタを取得します.
 *
 * @param[in/out] buffer_data バッファーのポインタを書き込みます
 * @param[in] time 書き込み時間です
 *
 * @retval >0 バッファーのIndex
 * @retval -1 失敗　データ無し
 */
int IscDataprocResultdataRingBuffer::GetGetBuffer(BufferData** buffer_data, ULONGLONG* time_get)
{
	if (buffer_data_ == nullptr) {
		return -1;
	}

	EnterCriticalSection(&flag_critical_);
	int local_read_index = read_index_;

	if (buffer_data_[local_read_index].state != 2) {
		LeaveCriticalSection(&flag_critical_);
		return -1;
	}

	*buffer_data = &buffer_data_[local_read_index];

	*time_get = buffer_data_[local_read_index].time;
	buffer_data_[local_read_index].state = 3;
	geted_inedx_ = local_read_index;

	if (last_mode_) {
		if (!allow_overwrite_) {
			// clear previous 
			int s = local_read_index - 1;
			int e = local_read_index + 1;
			if (s == -1) {
				s = buffer_count_ - 1;
			}
			if (e >= buffer_count_) {
				e = 0;
			}
			for (int i = s; i != e;) {
				if (buffer_data_[i].state != 2) {
					break;
				}
				buffer_data_[i].state = 0;

				i--;
				if (i == 0) {
					i = buffer_count_ - 1;
				}
			}
		}
	}
	else {
		read_index_++;
		if (read_index_ >= buffer_count_) {
			read_index_ = 0;
		}
	}

	LeaveCriticalSection(&flag_critical_);

	return geted_inedx_;
}

/**
 * 取得したバッファーの使用を終了します.
 *
 * @param[in] index バッファーのIndexです。取得時のものです
 *
 * @retval 0 成功
 * @retval -1 失敗　
 */
void IscDataprocResultdataRingBuffer::DoneGetBuffer(const int index)
{
	if (buffer_data_ == nullptr) {
		return;
	}

	if (index < 0 || index >= buffer_count_) {
		return;
	}

	if (index != geted_inedx_) {
		// Get and Done must correspond one-to-one on the same Thread
		return;
	}

	EnterCriticalSection(&flag_critical_);
	buffer_data_[index].state = 0;
	LeaveCriticalSection(&flag_critical_);

	return;
}

