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
 * @file dpc_image_writer.cpp
 * @brief This class is responsible for saving the image.
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 *
 * @details This class is responsible for saving the image.
 */

#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <locale.h>
#include <stdint.h>
#include <process.h>

#include "dpc_image_writer.h"

#include "opencv2\opencv.hpp"

int ImageReadUnicode(const TCHAR* file_name, cv::ImreadModes imread_modes, cv::Mat& read_data);
int ImageWriteUnicode(const TCHAR* file_name, cv::Mat& write_data);


DpcImageWriter::DpcImageWriter():
	max_width_(0), max_height_(0), save_image_path_(),
	max_buffer_count_(0), image_buffers_(), depth_buffers_(), image_data_count_(0),
	image_data_buffers_(nullptr), depth_data_count_(0), depth_data_buffers_(nullptr), pcd_data_count_(0), pcd_data_buffers_(nullptr), work_buffers_(),
	thread_control_(), handle_semaphore_(NULL), thread_handle_(NULL), threads_critical_(), write_state_(WriteState::idle), pcd_set_parameter_()
{

}

DpcImageWriter::~DpcImageWriter()
{

}

int DpcImageWriter::Initialize(const int max_width, const int max_height, const TCHAR* save_image_path)
{

	max_width_ = max_width;
	max_height_ = max_height;
	_stprintf_s(save_image_path_, _T("%s"), save_image_path);

	// 保存先の確認、フォルダー作成
	wchar_t save_folder_name[_MAX_PATH] = {};
	swprintf_s(save_folder_name, L"%s", save_image_path_);
	if (::PathFileExists(save_folder_name) && !::PathIsDirectory(save_folder_name)) {
		// 指定されたパスにファイルが存在、かつディレクトリでない -> NG
		return -1;
	}
	else if (::PathFileExists(save_folder_name)) {
		// 指定されたパスがディレクトリである -> OK
	}
	else {
		// 作成する
		int ret = SHCreateDirectoryEx(NULL, save_folder_name, NULL);
		if (ret == ERROR_SUCCESS) {
			// OK
		}
		else {
			// Failed
			return -1;
		}
	}

	// get buffer
	max_buffer_count_ = 4;

	const size_t frame_size = max_width_ * max_height_;
	const int max_channel_count = 4;
	image_buffers_[0] = new unsigned char[frame_size * max_channel_count * max_buffer_count_];
	depth_buffers_[0] = new float[frame_size * max_buffer_count_];
	image_buffers_[1] = new unsigned char[frame_size * max_channel_count * max_buffer_count_];
	depth_buffers_[1] = new float[frame_size * max_buffer_count_];

	image_data_buffers_ = new ImageData[max_buffer_count_];
	depth_data_buffers_ = new DepthData[max_buffer_count_];
	pcd_data_buffers_ = new PCDData[max_buffer_count_];

	for (int i = 0; i < max_buffer_count_; i++) {
		image_data_buffers_[i].id_string[0] = 0;
		image_data_buffers_[i].width = 0;
		image_data_buffers_[i].height = 0;
		image_data_buffers_[i].channel_count = 0;
		image_data_buffers_[i].is_rotate = false;
		image_data_buffers_[i].buffer = image_buffers_[0] + (i * frame_size * max_channel_count);

		depth_data_buffers_[i].id_string[0] = 0;
		depth_data_buffers_[i].width = 0;
		depth_data_buffers_[i].height = 0;
		depth_data_buffers_[i].is_rotate = false;
		depth_data_buffers_[i].camera_b = 0;
		depth_data_buffers_[i].camera_dinf = 0;
		depth_data_buffers_[i].camera_bf = 0;
		depth_data_buffers_[i].camera_set_angle = 0;
		depth_data_buffers_[i].buffer = depth_buffers_[0] + (i * frame_size);

		pcd_data_buffers_[i].id_string[0] = 0;
		pcd_data_buffers_[i].width = 0;
		pcd_data_buffers_[i].height = 0;
		pcd_data_buffers_[i].channel_count = 0;
		pcd_data_buffers_[i].is_rotate = false;
		pcd_data_buffers_[i].camera_b = 0;
		pcd_data_buffers_[i].camera_dinf = 0;
		pcd_data_buffers_[i].camera_bf = 0;
		pcd_data_buffers_[i].camera_set_angle = 0;
		pcd_data_buffers_[i].min_distance = 0;
		pcd_data_buffers_[i].max_distance = 0;

		pcd_data_buffers_[i].image = image_buffers_[1] + (i * frame_size * max_channel_count);
		pcd_data_buffers_[i].depth = depth_buffers_[1] + (i * frame_size);
	}

	for (int i = 0; i < 2; i++) {
		work_buffers_.image_work_buffer[i] = new unsigned char[frame_size * max_channel_count];
		work_buffers_.depth_work_buffer[i] = new float[frame_size];
	}

	// clear thread control
	thread_control_.terminate_request = 0;
	thread_control_.terminate_done = 0;
	thread_control_.end_code = 0;
	thread_control_.stop_request = false;

	// initalize semaphore
	char semaphoreName[64] = {};
	sprintf_s(semaphoreName, "THREAD_SEMAPHORENAME_DPCIAMGEWRITERW%d", 0);
	handle_semaphore_ = CreateSemaphoreA(NULL, 0, 8, semaphoreName);
	if (handle_semaphore_ == NULL) {
		// Fail
		return -1;
	}

	InitializeCriticalSection(&threads_critical_);

	// it start write thread
	if ((thread_handle_ = (HANDLE)_beginthreadex(0, 0, ControlThread, (void*)this, 0, 0)) == 0) {
		// Fail
		return -1;
	}

	// THREAD_PRIORITY_TIME_CRITICAL
	// THREAD_PRIORITY_HIGHEST +2
	// THREAD_PRIORITY_ABOVE_NORMAL +1
	// THREAD_PRIORITY_NORMAL  +0
	// THREAD_PRIORITY_BELOW_NORMAL -1
	SetThreadPriority(thread_handle_, THREAD_PRIORITY_BELOW_NORMAL);

	return 0;
}

int DpcImageWriter::Terminate()
{

	if (thread_handle_ != NULL) {
		// close thread procedure
		thread_control_.stop_request = true;
		thread_control_.terminate_done = 0;
		thread_control_.end_code = 0;
		thread_control_.terminate_request = 1;

		// release any resources
		ReleaseSemaphore(handle_semaphore_, 1, NULL);

		int count = 0;
		while (thread_control_.terminate_done == 0) {
			if (count > 100) {
				break;
			}
			count++;
			Sleep(10L);
		}

		if (thread_handle_ != NULL) {
			CloseHandle(thread_handle_);
			thread_handle_ = NULL;
		}
	}

	if (handle_semaphore_ != NULL) {
		CloseHandle(handle_semaphore_);
		handle_semaphore_ = NULL;
	}
	DeleteCriticalSection(&threads_critical_);

	for (int i = 0; i < 2; i++) {
		delete[] work_buffers_.depth_work_buffer[i];
		work_buffers_.depth_work_buffer[i] = nullptr;

		delete[] work_buffers_.image_work_buffer[i];
		work_buffers_.image_work_buffer[i] = nullptr;
	}

	delete[] depth_data_buffers_;
	depth_data_buffers_ = nullptr;
	delete[] image_data_buffers_;
	image_data_buffers_ = nullptr;

	delete[] depth_buffers_[0];
	depth_buffers_[0] = nullptr;
	delete[] image_buffers_[0];
	image_buffers_[0] = nullptr;
	delete[] depth_buffers_[1];
	depth_buffers_[1] = nullptr;
	delete[] image_buffers_[1];
	image_buffers_[1] = nullptr;

	return 0;
}

int DpcImageWriter::PushImageDepthData(const ImageDepthDataSet* image_depth_data_set)
{
	
	if ((image_depth_data_set->image_datacount == 0) && (image_depth_data_set->depth_data_count == 0)) {
		return 0;
	}

	// 書き込み中？
	EnterCriticalSection(&threads_critical_);
	if (write_state_ == WriteState::writing) {
		LeaveCriticalSection(&threads_critical_);
		return -1;
	}

	write_state_ = WriteState::writing;
	LeaveCriticalSection(&threads_critical_);

	// データ　コピー
	image_data_count_ = image_depth_data_set->image_datacount;

	for (int i = 0; i < image_data_count_; i++) {
		_stprintf_s(image_data_buffers_[i].id_string, _T("%s"), image_depth_data_set->image_data[i].id_string);
		image_data_buffers_[i].width = image_depth_data_set->image_data[i].width;
		image_data_buffers_[i].height = image_depth_data_set->image_data[i].height;
		image_data_buffers_[i].channel_count = image_depth_data_set->image_data[i].channel_count;
		image_data_buffers_[i].is_rotate = image_depth_data_set->image_data[i].is_rotate;

		size_t frame_size = image_data_buffers_[i].width * image_data_buffers_[i].height * image_data_buffers_[i].channel_count;
		memcpy(image_data_buffers_[i].buffer, image_depth_data_set->image_data[i].buffer, frame_size);
	}

	depth_data_count_ = image_depth_data_set->depth_data_count;

	for (int i = 0; i < depth_data_count_; i++) {
		_stprintf_s(depth_data_buffers_[i].id_string, _T("%s"), image_depth_data_set->depth_data[i].id_string);
		depth_data_buffers_[i].width = image_depth_data_set->depth_data[i].width;
		depth_data_buffers_[i].height = image_depth_data_set->depth_data[i].height;
		depth_data_buffers_[i].is_rotate = image_depth_data_set->depth_data[i].is_rotate;

		depth_data_buffers_[i].camera_b = image_depth_data_set->depth_data[i].camera_b;
		depth_data_buffers_[i].camera_dinf = image_depth_data_set->depth_data[i].camera_dinf;
		depth_data_buffers_[i].camera_bf = image_depth_data_set->depth_data[i].camera_bf;
		depth_data_buffers_[i].camera_set_angle = image_depth_data_set->depth_data[i].camera_set_angle;

		size_t frame_size = depth_data_buffers_[i].width * depth_data_buffers_[i].height * sizeof(float);
		memcpy(depth_data_buffers_[i].buffer, image_depth_data_set->depth_data[i].buffer, frame_size);
	}

	pcd_data_count_ = image_depth_data_set->pcd_data_count;
	for (int i = 0; i < pcd_data_count_; i++) {
		_stprintf_s(pcd_data_buffers_[i].id_string, _T("%s"), image_depth_data_set->pcd_data[i].id_string);
		pcd_data_buffers_[i].width = image_depth_data_set->pcd_data[i].width;
		pcd_data_buffers_[i].height = image_depth_data_set->pcd_data[i].height;
		pcd_data_buffers_[i].channel_count = image_depth_data_set->pcd_data[i].channel_count;
		pcd_data_buffers_[i].is_rotate = image_depth_data_set->pcd_data[i].is_rotate;

		pcd_data_buffers_[i].camera_b = image_depth_data_set->pcd_data[i].camera_b;
		pcd_data_buffers_[i].camera_dinf = image_depth_data_set->pcd_data[i].camera_dinf;
		pcd_data_buffers_[i].camera_bf = image_depth_data_set->pcd_data[i].camera_bf;
		pcd_data_buffers_[i].camera_set_angle = image_depth_data_set->pcd_data[i].camera_set_angle;
		pcd_data_buffers_[i].min_distance = image_depth_data_set->pcd_data[i].min_distance;
		pcd_data_buffers_[i].max_distance = image_depth_data_set->pcd_data[i].max_distance;

		size_t frame_size = pcd_data_buffers_[i].width * pcd_data_buffers_[i].height * pcd_data_buffers_[i].channel_count;
		memcpy(pcd_data_buffers_[i].image, image_depth_data_set->pcd_data[i].image, frame_size);

		frame_size = pcd_data_buffers_[i].width * pcd_data_buffers_[i].height * sizeof(float);
		memcpy(pcd_data_buffers_[i].depth, image_depth_data_set->pcd_data[i].depth, frame_size);
	}

	BOOL result_release = ReleaseSemaphore(handle_semaphore_, 1, NULL);

	return 0;
}

/**
* @brief データ受信　Thread
* @param context
* @retuen {@code 0} 成功 , {@code >0} 失敗
* @throws never
*/
unsigned __stdcall DpcImageWriter::ControlThread(void* context)
{
	DpcImageWriter* dpc_iamge_writer = (DpcImageWriter*)context;

	if (dpc_iamge_writer == nullptr) {
		return -1;
	}

	int ret = dpc_iamge_writer->WriteDataProc(dpc_iamge_writer);

	return ret;
}

/**
* @brief データ受信　
* @retuen {@code 0} 成功 , {@code >0} 失敗
* @throws never
*/
int DpcImageWriter::WriteDataProc(DpcImageWriter* dpc_iamge_writer)
{

	/*
		File Name

		Image:
		[save folder]\[date_time]_[id_string].png

		Depth:
		[save folder]\[date_time]_[id_string].bin

	*/

	while (dpc_iamge_writer->thread_control_.terminate_request < 1) {

		// Wait for start
		DWORD wait_result = WaitForSingleObject(dpc_iamge_writer->handle_semaphore_, INFINITE);

		if (dpc_iamge_writer->thread_control_.stop_request) {
			dpc_iamge_writer->thread_control_.stop_request = false;
			break;
		}

		if (wait_result == WAIT_OBJECT_0) {

			SYSTEMTIME st = {};
			wchar_t date_time_name[_MAX_PATH] = {};
			GetLocalTime(&st);
			// YYYYMMDD_HHMMSS
			_stprintf_s(date_time_name, _T("%04d%02d%02d_%02d%02d%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

			// 保存
			TCHAR write_file_name[_MAX_PATH] = {};

			for (int i = 0; i < dpc_iamge_writer->image_data_count_; i++) {

				_stprintf_s(write_file_name, _T("%s\\%s_%s.png"), dpc_iamge_writer->save_image_path_, date_time_name, dpc_iamge_writer->image_data_buffers_[i].id_string);

				int width = dpc_iamge_writer->image_data_buffers_[i].width;
				int height = dpc_iamge_writer->image_data_buffers_[i].height;
				int channel_count = dpc_iamge_writer->image_data_buffers_[i].channel_count;
				bool is_rotate = dpc_iamge_writer->image_data_buffers_[i].is_rotate;
				unsigned char* image = dpc_iamge_writer->image_data_buffers_[i].buffer;

				int ret = WriteImageToFileAsPng(write_file_name, width, height, channel_count, image, is_rotate);
			}

			for (int i = 0; i < dpc_iamge_writer->depth_data_count_; i++) {

				_stprintf_s(write_file_name, _T("%s\\%s_%s.bin"), dpc_iamge_writer->save_image_path_, date_time_name, dpc_iamge_writer->depth_data_buffers_[i].id_string);

				int width = dpc_iamge_writer->depth_data_buffers_[i].width;
				int height = dpc_iamge_writer->depth_data_buffers_[i].height;
				bool is_rotate = dpc_iamge_writer->depth_data_buffers_[i].is_rotate;
				unsigned char* data = nullptr;

				if (is_rotate) {
					cv::Mat mat_depth(height, width, CV_32F, dpc_iamge_writer->depth_data_buffers_[i].buffer);
					cv::Mat mat_depth_rotate(height,width, CV_32F, work_buffers_.depth_work_buffer[0]);

					cv::flip(mat_depth, mat_depth_rotate, -1);

					data = (unsigned char*)work_buffers_.depth_work_buffer[0];
				}
				else {
					data = (unsigned char*)dpc_iamge_writer->depth_data_buffers_[i].buffer;
				}

				int data_size = width * height * sizeof(float);
				int ret = WriteDepthToFileAsBinary(write_file_name, data_size, data);

				// debug
				if (1) {
					_stprintf_s(write_file_name, _T("%s\\%s_%s.png"), dpc_iamge_writer->save_image_path_, date_time_name, dpc_iamge_writer->depth_data_buffers_[i].id_string);
					WriteDepthToFileAsImage(write_file_name, width, height, (float*)data);
				}
			}

			for (int i = 0; i < dpc_iamge_writer->pcd_data_count_; i++) {
				_stprintf_s(write_file_name, _T("%s\\%s_%s.pcd"), dpc_iamge_writer->save_image_path_, date_time_name, dpc_iamge_writer->pcd_data_buffers_[i].id_string);

				int width				= dpc_iamge_writer->pcd_data_buffers_[i].width;
				int height				= dpc_iamge_writer->pcd_data_buffers_[i].height;
				double base_length		= dpc_iamge_writer->pcd_data_buffers_[i].camera_b;
				double bf				= dpc_iamge_writer->pcd_data_buffers_[i].camera_bf;
				double d_inf			= dpc_iamge_writer->pcd_data_buffers_[i].camera_dinf;
				int channel_count		= dpc_iamge_writer->pcd_data_buffers_[i].channel_count;
				double min_distance		= dpc_iamge_writer->pcd_data_buffers_[i].min_distance;;
				double max_distance		= dpc_iamge_writer->pcd_data_buffers_[i].max_distance;;
				bool is_rotate			= dpc_iamge_writer->pcd_data_buffers_[i].is_rotate;
				unsigned char* image	= dpc_iamge_writer->pcd_data_buffers_[i].image;
				float* depth			= dpc_iamge_writer->pcd_data_buffers_[i].depth;

				int ret = WriteDepthToFileAsPCD(write_file_name, width, height, base_length, bf, d_inf,
					min_distance, max_distance, channel_count, image, depth, is_rotate, &dpc_iamge_writer->work_buffers_);
			}
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

		EnterCriticalSection(&threads_critical_);
		write_state_ = WriteState::ended;
		LeaveCriticalSection(&threads_critical_);
	}

	dpc_iamge_writer->thread_control_.end_code = 0;
	dpc_iamge_writer->thread_control_.terminate_done = 1;

	return 0;
}

int DpcImageWriter::WriteImageToFileAsPng(const TCHAR* file_name, const int width, const int height, const int channel_count, unsigned char* image, const bool is_rotate)
{
	int ret = 0;

	if ((width == 0) || (height == 0) || (channel_count == 0)){
		return -1;
	}

	if (!is_rotate) {
		if (channel_count == 1) {
			cv::Mat mat_work_image(height, width, CV_8UC1, image);
			ret = ImageWriteUnicode(file_name, mat_work_image);
		}
		else if (channel_count == 3) {
			cv::Mat mat_work_image(height, width, CV_8UC3, image);
			ret = ImageWriteUnicode(file_name, mat_work_image);
		}
		else if (channel_count == 4) {
			cv::Mat mat_work_image(height, width, CV_8UC4, image);

			cv::Mat mat_work_image_cvt;
			cv::cvtColor(mat_work_image, mat_work_image_cvt, cv::COLOR_BGRA2BGR);

			ret = ImageWriteUnicode(file_name, mat_work_image_cvt);
		}
		else {
			return -1;
		}
	}
	else {
		if (channel_count == 1) {
			cv::Mat mat_work_image(height, width, CV_8UC1, image);
			cv::Mat mat_work_image_rotate;
			cv::flip(mat_work_image, mat_work_image_rotate, -1);

			ret = ImageWriteUnicode(file_name, mat_work_image_rotate);
		}
		else if (channel_count == 3) {
			cv::Mat mat_work_image(height, width, CV_8UC3, image);
			cv::Mat mat_work_image_rotate;
			cv::flip(mat_work_image, mat_work_image_rotate, -1);

			ret = ImageWriteUnicode(file_name, mat_work_image_rotate);
		}
		else if (channel_count == 4) {
			cv::Mat mat_work_image(height, width, CV_8UC4, image);
			cv::Mat mat_work_image_rotate;
			cv::flip(mat_work_image, mat_work_image_rotate, -1);

			cv::Mat mat_work_image_cvt;
			cv::cvtColor(mat_work_image_rotate, mat_work_image_cvt, cv::COLOR_BGRA2BGR);

			ret = ImageWriteUnicode(file_name, mat_work_image_cvt);
		}
		else {
			return -1;
		}
	}

	return ret;
}

int DpcImageWriter::WriteDepthToFileAsBinary(const TCHAR* file_name, const int data_size, const unsigned char* data)
{
	TCHAR write_file_name[_MAX_PATH] = {};
	_stprintf_s(write_file_name, _T("%s"), file_name);

	HANDLE hadle_file = NULL;
	if ((hadle_file = CreateFile(write_file_name,
								GENERIC_WRITE,
								NULL,
								NULL,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL)) == INVALID_HANDLE_VALUE) {

								//TCHAR msg[256];
								//wsprintf(msg, TEXT("[ERROR]Create/Open File error. Filename = %s"), writeFileName);
								//MessageBox(NULL, msg, TEXT("File Write "), MB_OK);
								return -1;
	}

	DWORD Nnmber_of_write = 0;
	DWORD write_size = (DWORD)data_size;

	if (WriteFile(hadle_file, data, write_size, &Nnmber_of_write, NULL) != TRUE) {
		CloseHandle(hadle_file);
		return -1;
	}

	CloseHandle(hadle_file);

	return 0;
}

int DpcImageWriter::WriteDepthToFileAsImage(const TCHAR* file_name, const int width, const int height, const float* data)
{

	unsigned char* work = new unsigned char[width * height];
	memset(work, 0, width * height);

	for (int y = 0; y < height; y++) {

		const float* src_line = data + (y * width);
		unsigned char* dst_line = work + (y * width);

		for (int x = 0; x < width; x++) {
			*dst_line++ = (unsigned char)(*src_line++);
		}
	}

	int ret = WriteImageToFileAsPng(file_name, width, height, 1, work, false);

	delete[] work;

	return 0;
}

#define MAKE_RGB(r, g, b) ((int(r) << 16) + (int(g) << 8) + int(b) )

int DpcImageWriter::WriteDepthToFileAsPCD(const TCHAR* file_name, const int width, const int height, const double base_length, const double bf, const double d_inf, 
	const double min_distance, const double max_distance, const int channel_count, unsigned char* image, float* depth, const bool is_rotate, WorkBuffers* work_buffers)
{

	// rotate depth
	cv::Mat mat_depth(height, width, CV_32F, work_buffers->depth_work_buffer[0]);
	if (is_rotate) {
		cv::Mat mat_src_depth(height, width, CV_32F, depth);
		cv::flip(mat_src_depth, mat_depth, -1);
	}
	else {
		memcpy(work_buffers->depth_work_buffer[0], depth, width * height * sizeof(float));
	}

	// rotate image
	cv::Mat mat_image(height, width, CV_8UC3, work_buffers->image_work_buffer[0]);
	if (!is_rotate) {
		if (channel_count == 1) {
			cv::Mat mat_work_image(height, width, CV_8UC1, image);
			cv::cvtColor(mat_work_image, mat_image, cv::COLOR_GRAY2BGR);
		}
		else if (channel_count == 3) {
			memcpy(work_buffers->image_work_buffer[0], image, width * height);
		}
		else if (channel_count == 4) {
			cv::Mat mat_work_image(height, width, CV_8UC4, image);
			cv::cvtColor(mat_work_image, mat_image, cv::COLOR_BGRA2BGR);
		}
		else {
			return -1;
		}
	}
	else {
		if (channel_count == 1) {
			cv::Mat mat_work_image(height, width, CV_8UC1, image);
			cv::Mat mat_work_image_rotate(height, width, CV_8UC1, work_buffers->image_work_buffer[1]);
			cv::flip(mat_work_image, mat_work_image_rotate, -1);
			cv::cvtColor(mat_work_image_rotate, mat_image, cv::COLOR_GRAY2BGR);
		}
		else if (channel_count == 3) {
			cv::Mat mat_work_image(height, width, CV_8UC3, image);
			cv::flip(mat_work_image, mat_image, -1);
		}
		else if (channel_count == 4) {
			cv::Mat mat_work_image(height, width, CV_8UC4, image);
			cv::Mat mat_work_image_rotate(height, width, CV_8UC4, work_buffers->image_work_buffer[1]);
			cv::flip(mat_work_image, mat_work_image_rotate, -1);
			cv::cvtColor(mat_work_image_rotate, mat_image, cv::COLOR_BGRA2BGR);
		}
		else {
			return -1;
		}
	}

	std::vector<pt_t> pts;
	size_t ssize = height * width;
	pts.reserve(ssize);

	int yc = height / 2;
	int xc = width / 2;
	float b = (float)base_length;
	pt_t pt(1.0, 1.0, 1.0, 1);

	// Unity -> ROS
	// Position: Unity(x,y,z) -> ROS(z,-x,y)
	pcd_set_parameter_.is_axis_unity = true;
	pcd_set_parameter_.axis_reverse_x = false;
	pcd_set_parameter_.axis_reverse_y = false;
	pcd_set_parameter_.axis_reverse_z = false;

	for (int i = 0; i < height; i++) {
		
		float* src_depth = mat_depth.ptr<float>(i);
		cv::Vec3b* src_image = mat_image.ptr<cv::Vec3b>(i);
		
		for (int j = 0; j < width; j++) {
			float value = src_depth[j] - (float)d_inf;
			float x = std::numeric_limits<float>::quiet_NaN();
			float y = std::numeric_limits<float>::quiet_NaN();
			float z = std::numeric_limits<float>::quiet_NaN();
			unsigned int col = 0;

			pt = pt_t(x, y, z, col);

			if (value > 0) {
				if (pcd_set_parameter_.is_axis_unity) {
					// Unity
					if (pcd_set_parameter_.axis_reverse_x) {
						x = -1 * ((b * (xc - j)) / value);
					}
					else {
						x = ((b * (xc - j)) / value);
					}
					if (pcd_set_parameter_.axis_reverse_y) {
						y = -1 * ((b * (yc - i)) / value);
					}
					else {
						y = (b * (yc - i)) / value;
					}
					if (pcd_set_parameter_.axis_reverse_z) {
						z = -1 * ((float)bf / value);
					}
					else {
						z = (float)bf / value;
					}

					if (abs(z) >= min_distance && abs(z) < max_distance) {
						unsigned char b = src_image[j][0];
						unsigned char g = src_image[j][1];
						unsigned char r = src_image[j][2];
						col = MAKE_RGB(r, g, b);

						pt = pt_t(x, y, z, col);
					}
				}
				else {
					// ROS
					if (pcd_set_parameter_.axis_reverse_x) {
						x = -1 * ((float)bf / value);
					}
					else {
						x = (float)bf / value;
					}
					if (pcd_set_parameter_.axis_reverse_y) {
						y = -1 * ((b * (xc - j)) / value);
					}
					else {
						y = ((b * (xc - j)) / value);
					}
					if (pcd_set_parameter_.axis_reverse_z) {
						z = -1 * ((b * (yc - i)) / value);
					}
					else {
						z = (b * (yc - i)) / value;
					}

					if (abs(x) >= min_distance && abs(x) < max_distance) {
						unsigned char b = src_image[j][0];
						unsigned char g = src_image[j][1];
						unsigned char r = src_image[j][2];
						col = MAKE_RGB(r, g, b);

						pt = pt_t(x, y, z, col);
					}
				}
			}

			pts.push_back(pt);
		}
	}

	// PCD
	// write to file
	char write_file_name[_MAX_PATH] = {};
	size_t wlen = 0;
	errno_t err = wcstombs_s(&wlen, write_file_name, file_name, sizeof(write_file_name));

	FILE* fp = nullptr;

#if 1
	err = fopen_s(&fp, write_file_name, "wb+");
	if (err != 0 || fp == nullptr) {
		return -1;
	}
	fprintf(fp, "# .PCD v0.7 - Point Cloud Data file format\n");
	fprintf(fp, "VERSION 0.7\nFIELDS x y z rgb\nSIZE 4 4 4 4\nTYPE F F F U\nCOUNT 1 1 1 1\n");
	fprintf(fp, "WIDTH %d\nHEIGHT %d\nVIEWPOINT 0 0 0 1 0 0 0\nPOINTS %zd\nDATA binaryd\n", width, height, pts.size());

	// 中身をバイナリで書き込み
	for (int i = 0; i < pts.size(); i++) {
		float xyz[3] = { pts[i].x, pts[i].y, pts[i].z };

		fwrite(xyz, sizeof(float), 3, fp);
		fwrite(&pts[i].col, sizeof(unsigned int), 1, fp);
	}

#else
	if (err != 0 || fp == nullptr) {
		return -1;
	}
	err = fopen_s(&fp, write_file_name, "wt+");
	fprintf(fp, "# .PCD v0.7 - Point Cloud Data file format\n");
	fprintf(fp, "VERSION 0.7\nFIELDS x y z rgb\nSIZE 4 4 4 4\nTYPE F F F U\nCOUNT 1 1 1 1\n");
	fprintf(fp, "WIDTH %d\nHEIGHT %d\nVIEWPOINT 0 0 0 1 0 0 0\nPOINTS %d\nDATA ascii\n", width, height, pts.size());

	// 中身をASCIIで書き込み
	for (int i = 0; i < pts.size(); i++) {
		fprintf_s(fp, ("%.5f %.5f %.5f %d\n"), pts[i].x, pts[i].y, pts[i].z, pts[i].col);
	}
#endif

	fclose(fp);

	return 0;
}

int ImageReadUnicode(const TCHAR* file_name, cv::ImreadModes imread_modes, cv::Mat& read_data)
{
	HANDLE hadle_file = NULL;

	if ((hadle_file = CreateFile(file_name,
								GENERIC_READ,
								FILE_SHARE_READ,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL)) == INVALID_HANDLE_VALUE) {

								//int ii = GetLastError();
								//TCHAR msg[256] = {};
								//_stprintf_s(msg, _T("[ERROR]reate/Open File error. Filename = %s Error Code = %lx"), fileName, ii);
								//MessageBox(NULL, msg, _T("File Read "), MB_OK);

								return -1;
	}

	DWORD file_size = GetFileSize(hadle_file, NULL);
	if (file_size != 0xFFFFFFFF) {
		printf("FileSize = %u bytes\n", file_size);
	}
	else {
		CloseHandle(hadle_file);
		return -1;
	}

	unsigned char* temp_buffer = new unsigned char[file_size];

	DWORD number_of_read = 0;
	if (FALSE == ReadFile(hadle_file, temp_buffer, file_size, &number_of_read, NULL)) {
		CloseHandle(hadle_file);
		return -1;
	}

	CloseHandle(hadle_file);

	// to mat
	std::vector<unsigned char> temp_vec(temp_buffer, temp_buffer + file_size);
	cv::Mat img = cv::imdecode(temp_vec, imread_modes);

	// copy
	read_data = img.clone();

	// show
	//cv::imshow("sample", img);
	//cv::waitKey();
	//cv::destroyAllWindows();

	delete[] temp_buffer;

	return 0;
}

int ImageWriteUnicode(const TCHAR* file_name, cv::Mat& write_data)
{

	// Check File Type
	TCHAR file_extention[_MAX_PATH] = {};
	_stprintf_s(file_extention, _T("%s"), PathFindExtension(file_name));
	CharUpper(file_extention);

	enum class WriteFileType {
		unknown,
		bmp,
		jpg,
		png
	};

	int ret = -1;
	WriteFileType write_file_type = WriteFileType::unknown;

	if ((ret = _tcscmp(file_extention, _T(".JPG"))) == 0) {
		write_file_type = WriteFileType::jpg;
		//_tprintf_s(_T("is jpeg"));
	}
	else if ((ret = _tcscmp(file_extention, _T(".BMP"))) == 0) {
		write_file_type = WriteFileType::bmp;
		//_tprintf_s(_T("is bmp"));
	}
	else if ((ret = _tcscmp(file_extention, _T(".PNG"))) == 0) {
		write_file_type = WriteFileType::png;
		//_tprintf_s(_T("is bmp"));
	}
	else {
		write_file_type = WriteFileType::unknown;
		//_tprintf_s(_T("is ?"));

		return -1;
	}

	// Write
	std::vector<uchar> buff;						// buffer for coding
	std::vector<int> param = std::vector<int>(2);	// param for jpeg

	switch (write_file_type) {
	case(WriteFileType::jpg):
		param[0] = cv::IMWRITE_JPEG_QUALITY;
		param[1] = 100;// 95;//default(95) 0-100
		cv::imencode(".jpg", write_data, buff, param);
		break;

	case(WriteFileType::bmp):
		cv::imencode(".bmp", write_data, buff);
		break;

	case(WriteFileType::png):
		param[0] = cv::IMWRITE_PNG_COMPRESSION;
		param[1] = 3;//default(3)  0-9.
		cv::imencode(".png", write_data, buff);
		break;

	default:
		break;

	}

	TCHAR write_file_name[_MAX_PATH] = {};
	_stprintf_s(write_file_name, _T("%s"), file_name);

	HANDLE hadle_file = NULL;
	if ((hadle_file = CreateFile(write_file_name,
								GENERIC_WRITE,
								NULL,
								NULL,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL)) == INVALID_HANDLE_VALUE) {

								//TCHAR msg[256];
								//wsprintf(msg, TEXT("[ERROR]Create/Open File error. Filename = %s"), writeFileName);
								//MessageBox(NULL, msg, TEXT("File Write "), MB_OK);
								return -1;
	}

	DWORD Nnmber_of_write = 0;
	DWORD write_size = (DWORD)buff.size();

	if (WriteFile(hadle_file, &buff[0], write_size, &Nnmber_of_write, NULL) != TRUE) {
		CloseHandle(hadle_file);
		return -1;
	}

	CloseHandle(hadle_file);

	return 0;
}