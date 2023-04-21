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
 * @file isc_image_info_ring_buffer.cpp
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

#include "isc_image_info_ring_buffer.h"

/**
 * constructor
 *
 */
IscImageInfoRingBuffer::IscImageInfoRingBuffer():
	last_mode_(false), allow_overwrite_(false), buffer_count_(0), width_(0), height_(0), channel_count_(0), buffer_data_(nullptr),
	write_inex_(0), read_index_(0), put_index_(0), geted_inedx_(0),
	buff_p1_(nullptr), buff_p2_(nullptr), buff_color_(nullptr),
	buff_depth_(nullptr), buff_raw_(nullptr), buff_bayer_base_(nullptr), buff_bayer_compare_(nullptr),
	flag_critical_()
{
}

/**
 * destructor
 *
 */
IscImageInfoRingBuffer::~IscImageInfoRingBuffer()
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
 *
 * @retval 0 成功
 * @retval -1 失敗　
 */
int IscImageInfoRingBuffer::Initialize(const bool last_mpde, const bool allow_overwrite, const int count, const int width_def, const int height_def)
{
	last_mode_ = last_mpde;
	allow_overwrite_ = allow_overwrite;
	buffer_count_ = count;
	width_ = width_def;
	height_ = height_def;
	write_inex_ = 0; read_index_ = 0; put_index_ = 0;  geted_inedx_ = 0;

	InitializeCriticalSection(&flag_critical_);

	buffer_data_ = new BufferData[buffer_count_];

	size_t one_frame_size = width_ * height_;

	buff_p1_ = new unsigned char[buffer_count_ * one_frame_size];
	buff_p2_ = new unsigned char[buffer_count_ * one_frame_size];
	buff_color_ = new unsigned char[buffer_count_ * one_frame_size * 3];
	buff_depth_ = new float[buffer_count_ * one_frame_size];
	buff_raw_ = new unsigned char[buffer_count_ * one_frame_size * 2];
	buff_bayer_base_ = new unsigned char[buffer_count_ * one_frame_size];
	buff_bayer_compare_ = new unsigned char[buffer_count_ * one_frame_size];

	memset(buff_p1_, 0, buffer_count_ * one_frame_size);
	memset(buff_p2_, 0, buffer_count_ * one_frame_size);
	memset(buff_color_, 0, buffer_count_ * one_frame_size * 3);
	memset(buff_depth_, 0, buffer_count_ * one_frame_size * sizeof(float));
	memset(buff_raw_, 0, buffer_count_ * one_frame_size * 2);
	memset(buff_bayer_base_, 0, buffer_count_ * one_frame_size);
	memset(buff_bayer_compare_, 0, buffer_count_ * one_frame_size);

	for (int i = 0; i < buffer_count_; i++) {
		buffer_data_[i].inedx = i;
		buffer_data_[i].state = 0;
		buffer_data_[i].time = 0;

		buffer_data_[i].isc_image_info.frameNo = 0;
		buffer_data_[i].isc_image_info.gain = 0;
		buffer_data_[i].isc_image_info.exposure = 0;
		buffer_data_[i].isc_image_info.grab = IscGrabMode::kParallax;
		buffer_data_[i].isc_image_info.color_grab_mode = IscGrabColorMode::kColorOFF;
		buffer_data_[i].isc_image_info.shutter_mode = IscShutterMode::kManualShutter;
		buffer_data_[i].isc_image_info.camera_specific_parameter.d_inf = 0;
		buffer_data_[i].isc_image_info.camera_specific_parameter.bf = 0;
		buffer_data_[i].isc_image_info.camera_specific_parameter.base_length = 0;
		buffer_data_[i].isc_image_info.camera_specific_parameter.dz = 0;

		buffer_data_[i].isc_image_info.camera_status.error_code = 0;
		buffer_data_[i].isc_image_info.camera_status.data_receive_tact_time = 0;

		buffer_data_[i].isc_image_info.camera_status.error_code = 0;
		buffer_data_[i].isc_image_info.camera_status.data_receive_tact_time = 0;

		buffer_data_[i].isc_image_info.p1.width = 0;
		buffer_data_[i].isc_image_info.p1.height = 0;
		buffer_data_[i].isc_image_info.p1.channel_count = 0;
		buffer_data_[i].isc_image_info.p1.image = buff_p1_ + (one_frame_size * i);

		buffer_data_[i].isc_image_info.p2.width = 0;
		buffer_data_[i].isc_image_info.p2.height = 0;
		buffer_data_[i].isc_image_info.p2.channel_count = 0;
		buffer_data_[i].isc_image_info.p2.image = buff_p2_ + (one_frame_size * i);

		buffer_data_[i].isc_image_info.color.width = 0;
		buffer_data_[i].isc_image_info.color.height = 0;
		buffer_data_[i].isc_image_info.color.channel_count = 0;
		buffer_data_[i].isc_image_info.color.image = buff_color_ + (one_frame_size * 3 * i);

		buffer_data_[i].isc_image_info.depth.width = 0;
		buffer_data_[i].isc_image_info.depth.height = 0;
		buffer_data_[i].isc_image_info.depth.image = buff_depth_ + (one_frame_size * i);

		buffer_data_[i].isc_image_info.raw.width = 0;
		buffer_data_[i].isc_image_info.raw.height = 0;
		buffer_data_[i].isc_image_info.raw.channel_count = 0;
		buffer_data_[i].isc_image_info.raw.image = buff_raw_ + (one_frame_size * 2 * i);

		buffer_data_[i].isc_image_info.bayer_base.width = 0;
		buffer_data_[i].isc_image_info.bayer_base.height = 0;
		buffer_data_[i].isc_image_info.bayer_base.channel_count = 0;
		buffer_data_[i].isc_image_info.bayer_base.image = buff_bayer_base_ + (one_frame_size * i);

		buffer_data_[i].isc_image_info.bayer_compare.width = 0;
		buffer_data_[i].isc_image_info.bayer_compare.height = 0;
		buffer_data_[i].isc_image_info.bayer_compare.channel_count = 0;
		buffer_data_[i].isc_image_info.bayer_compare.image = buff_bayer_compare_ + (one_frame_size * i);
	}

	return 0;
}

/**
 * 各変変数を初期化します.
 *
 *
 * @return none.
 */
int IscImageInfoRingBuffer::Clear()
{
	write_inex_ = 0; read_index_ = 0; put_index_ = 0;  geted_inedx_ = 0;

	size_t one_frame_size = width_ * height_;
	memset(buff_p1_, 0, buffer_count_ * one_frame_size);
	memset(buff_p2_, 0, buffer_count_ * one_frame_size);
	memset(buff_color_, 0, buffer_count_ * one_frame_size * 3);
	memset(buff_depth_, 0, buffer_count_ * one_frame_size * sizeof(float));
	memset(buff_raw_, 0, buffer_count_ * one_frame_size * 2);
	memset(buff_bayer_base_, 0, buffer_count_ * one_frame_size);
	memset(buff_bayer_compare_, 0, buffer_count_ * one_frame_size);

	return 0;
}

/**
 * 動作モードを設定します.
 *
 * @param[in] last_mpde true:最新のものを取得します false:FIFOです
 * @param[in] allow_overwrite true:オーバーフロー時に上書きします
 *
 * @retval 0 成功
 * @retval -1 失敗　
 */
int IscImageInfoRingBuffer::SetMode(const bool last_mpde, const bool allow_overwrite)
{
	last_mode_ = last_mpde;
	allow_overwrite_ = allow_overwrite;

	return 0;
}

/**
 * 終了します.
 *
 *
 * @return none.
 */
int IscImageInfoRingBuffer::Terminate()
{
	delete[] buff_p1_;
	delete[] buff_p2_;
	delete[] buff_color_;
	delete[] buff_depth_;
	delete[] buff_raw_;
	delete[] buff_bayer_base_;
	delete[] buff_bayer_compare_;

	buff_p1_ = nullptr;
	buff_p2_ = nullptr;
	buff_color_ = nullptr;
	buff_depth_ = nullptr;
	buff_raw_ = nullptr;
	buff_bayer_base_ = nullptr;
	buff_bayer_compare_ = nullptr;

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
int IscImageInfoRingBuffer::GetPutBuffer(BufferData** buffer_data, const ULONGLONG time)
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
int IscImageInfoRingBuffer::DonePutBuffer(const int index, const int status)
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
int IscImageInfoRingBuffer::GetGetBuffer(BufferData** buffer_data, ULONGLONG* time_get)
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
void IscImageInfoRingBuffer::DoneGetBuffer(const int index)
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

