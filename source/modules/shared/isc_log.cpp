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
 * @file isc_log.cpp
 * @brief log class
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides logging function.
 */
 #include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <stdexcept>
#include <stdarg.h>
#include <mutex>
#include <Shlwapi.h>
#include <Shlobj.h>
#include <share.h>

#include "isc_log.h"

/**
 * constructor
 *
 */
IscLog::IscLog():
	immediate_mode_(false), mutex_(), file_(NULL), file_opened_(false),
	save_path_(), save_file_name_(), log_file_name_(), log_level_(0), file_size_(0), max_file_size_(0),
	thread_terminate_request_(0), thread_terminate_done_(0), thread_end_code_(0), abort_request_(false),
	handle_semaphore_(NULL), thread_handle_(NULL), threads_critical_(), log_data_que_(nullptr)
{
	log_level_ = 99;
}

/**
 * destructor
 *
 */
IscLog::~IscLog()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] save_path ルートパス
 * @param[in] save_file_name 初期保存ファイル名
 * @param[in] log_level ログのレベル
 * @param[in] is_immediate_mode true:即時ファイルクローズ false:Close時にファイルもクローズする
 * @return none.
 * @note
 *  - Log保存レベル  
 *  -- 0:保存しない  
 *  -- 1:エラーを保存  
 *  -- 2:ワーニングを保存  
 *  -- 3:全て保存  
 *  -- 4:デバック  
 */
void IscLog::Open(wchar_t* save_path, wchar_t* save_file_name, int log_level, bool is_immediate_mode)
{

	// init
	thread_terminate_done_ = 0;
	thread_terminate_request_ = 0;
	thread_end_code_ = 0;
	abort_request_ = false;

	log_level_ = log_level;
	max_file_size_ = 10 * 1024 * 1024;
	immediate_mode_ = is_immediate_mode;

	if (log_level_ == 0) {
		// 保存しない
		return;
	}
	
	// folder check
	swprintf_s(save_path_, L"%s", save_path);
	swprintf_s(save_file_name_, L"%s", save_file_name);

	if (::PathFileExists(save_path_) && !::PathIsDirectory(save_path_)) {
		// 指定されたパスにファイルが存在、かつディレクトリでない
	}
	else if (::PathFileExists(save_path_)) {
		// 指定されたパスがディレクトリである
	}
	else {
		// 作成する
		SHCreateDirectoryEx(NULL, save_path_, NULL);
	}

	// open file
	if (file_opened_) {
		fclose(file_);
		file_ = NULL;
		file_opened_ = false;
	}

	SYSTEMTIME st;
	GetLocalTime(&st);
	swprintf_s(log_file_name_, L"%s-%02d%02d%02d%02d%02d.txt", save_file_name_, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	// Threadで書き込みます
	if (immediate_mode_) {
		// 書き込み時にファイルを開き直します
		errno_t err = _wfopen_s(&file_, log_file_name_, L"wt+, ccs=UTF-8");
		if (file_ == NULL) {
			file_opened_ = false;
		}
		else {
			fclose(file_);
			file_ = NULL;
			file_opened_ = false;
		}
	}
	else {
		errno_t err = _wfopen_s(&file_, log_file_name_, L"wt+, ccs=UTF-8");
		if (file_ == NULL) {
			file_opened_ = false;
		}
		else {
			file_opened_ = true;
		}
	}

	log_data_que_ = new LogDataQue;
	log_data_que_->open(128, false);		// mode=all data
			
	// thread start
	wchar_t semaphore_name[64] = {};
	swprintf_s(semaphore_name, sizeof(semaphore_name) / sizeof(wchar_t), L"MAIN_THREAD_SEMAPHORENAME_%d", 0);
	handle_semaphore_ = CreateSemaphore(NULL, 0, 1, semaphore_name);
	if (handle_semaphore_ == NULL) {
		// Fail
	}
	InitializeCriticalSection(&threads_critical_);

	if ((thread_handle_ = (HANDLE)_beginthreadex(0, 0, LogThread, (void*)this, 0, 0)) == 0) {
		// Fail
	}

	// THREAD_PRIORITY_TIME_CRITICAL
	// THREAD_PRIORITY_HIGHEST +2
	// THREAD_PRIORITY_ABOVE_NORMAL +1
	// THREAD_PRIORITY_NORMAL  +0
	// THREAD_PRIORITY_BELOW_NORMAL -1
	if (thread_handle_ != NULL) {
		SetThreadPriority(thread_handle_, THREAD_PRIORITY_BELOW_NORMAL);
	}

	return;
}


/**
 * 終了処理をします.
 *
 * @return none
 */
void IscLog::Close()
{

	// stop thread and release
	// 
	// 1回はLoopさせておく
	Sleep(60);

	thread_terminate_done_ = 0;
	thread_terminate_request_ = 1;
	thread_end_code_ = 0;

	ReleaseSemaphore(handle_semaphore_, 1, NULL);
	Sleep(60);

	int count = 0;
	while (thread_terminate_done_ == 0) {
		if (count > 100) {
			break;
		}
		count++;
		Sleep(10L);
	}

	if (thread_terminate_done_ == 0) {
		abort_request_ = true;

		count = 0;
		while (thread_terminate_done_ == 0) {
			if (count > 100) {
				break;
			}
			count++;
			Sleep(10L);
		}
	}

	if (thread_handle_ != NULL) {
		CloseHandle(thread_handle_);
		thread_handle_ = NULL;
	}
	if (handle_semaphore_ != NULL) {
		CloseHandle(handle_semaphore_);
		handle_semaphore_ = NULL;
	}
	DeleteCriticalSection(&threads_critical_);

	Flush();

	if (file_ && file_opened_) {
		fclose(file_);
		file_ = NULL;
		file_opened_ = false;
	}

	return;
}

/**
 * ログファイルをFlushします.
 *
 * @return none
 */
void IscLog::Flush()
{
	if (file_ && file_opened_)
	{
		fflush(file_);
	}

	return;
}

/**
 * Threadから呼び出されるログ処理本体です.
 *
 * @param[in] st 現在時刻
 * @param[in] head タイトル
 * @param[in] level ログのレベル
 * @param[in] str 本文
 *
 * @return none.
 */
void IscLog::LogForThread(const SYSTEMTIME* st, const wchar_t *head, Level level, const wchar_t *str)
{
	if (log_level_ == 0) {
		// off
		return;
	}

	if (immediate_mode_) {
		errno_t err = _wfopen_s(&file_, log_file_name_, L"at+, ccs=UTF-8");
		if (file_ == NULL) {
			file_opened_ = false;
		}
		else {
			file_opened_ = true;
		}
	}

	if (file_ && file_opened_) {

		bool en_log = false;
		if (log_level_ == 1) {
			// エラーを保存(Debug含む）
			if ((int)level >= (int)Level::Error) {
				en_log = true;
			}
		}
		else if (log_level_ == 2) {
			// ワーニングを保存
			if ((int)level >= (int)Level::Warning) {
				en_log = true;
			}
		}
		else if (log_level_ >= 3) {
			// 全て保存
			if ((int)level >= (int)Level::Information) {
				en_log = true;
			}
		}

		if (en_log) {

			const wchar_t *type;
			switch (level)
			{
			case Level::Debug:
				type = L"DEBUG";
				break;
			case Level::Error:
				type = L"ERROR";
				break;
			case Level::Warning:
				type = L"WARN";
				break;
			case Level::Information:
				type = L"INFO";
				break;
			default:
				type = L"UNKOWN";
				break;
			}

			wchar_t dt[1024] = {};
			swprintf_s(dt, L"%02d%02d:%02d%02d%02d.%03d[%s][%s] ", st->wMonth, st->wDay, st->wHour, st->wMinute, st->wSecond, st->wMilliseconds, head, type);

			fputws(dt, file_);

			file_size_ += (int)wcslen(dt);
			fputws(str, file_);

			file_size_ += (int)wcslen(str);

			if (level == Level::Error) {
				fflush(file_);
			}

			if (file_size_ >= max_file_size_) {
				fclose(file_);
				swprintf_s(log_file_name_, L"%s-%02d%02d%02d%02d%02d.txt", save_file_name_, st->wMonth, st->wDay, st->wHour, st->wMinute, st->wSecond);

				if (immediate_mode_) {
					// 書き込み時にファイルを開き直します
					errno_t err = _wfopen_s(&file_, log_file_name_, L"wt+, ccs=UTF-8");
					if (file_ == NULL) {
						file_opened_ = false;
					}
					else {
						file_opened_ = true;
					}
				}
				else {
					errno_t err = _wfopen_s(&file_, log_file_name_, L"wt+, ccs=UTF-8");
					if (file_ == NULL) {
						file_opened_ = false;
					}
					else {
						file_opened_ = true;
					}
				}

				file_size_ = 0;
			}
		}
	}

	if (immediate_mode_ && (file_ != NULL)) {
		Flush();
		fclose(file_);
		file_ = NULL;
		file_opened_ = false;
	}

	return;
}

/**
 * 外部から呼び出されるログ関数です.(Debug)
 *
 * @param[in] head タイトル
 * @param[in] str 本文
 *
 * @return none.
 */
void IscLog::LogDebug(const wchar_t *head, const wchar_t *str)
{
	if (log_level_ == 0) {
		// off
		return;
	}

	mutex_.lock();
	log_data_que_->put(Level::Debug, head, str);
	mutex_.unlock();

	return;
}

/**
 * 外部から呼び出されるログ関数です.(Error)
 *
 * @param[in] head タイトル
 * @param[in] str 本文
 *
 * @return none.
 */
void IscLog::LogError(const wchar_t *head, const wchar_t *str)
{
	if (log_level_ == 0) {
		// off
		return;
	}

	mutex_.lock();
	log_data_que_->put(Level::Error, head, str);
	mutex_.unlock();

	return;
}

/**
 * 外部から呼び出されるログ関数です.(Warning)
 *
 * @param[in] head タイトル
 * @param[in] str 本文
 *
 * @return none.
 */
void IscLog::LogWarning(const wchar_t *head, const wchar_t *str)
{
	if (log_level_ == 0) {
		// off
		return;
	}

	mutex_.lock();
	log_data_que_->put(Level::Warning, head, str);
	mutex_.unlock();

	return;
}

/**
 * 外部から呼び出されるログ関数です.(Information)
 *
 * @param[in] head タイトル
 * @param[in] str 本文
 *
 * @return none.
 */
void IscLog::LogInfo(const wchar_t *head, const wchar_t *str)
{
	if (log_level_ == 0) {
		// off
		return;
	}

	mutex_.lock();
	log_data_que_->put(Level::Information, head, str);
	mutex_.unlock();

	return;
}

/**
 * Thread関数です.
 *
 * @param[in] context Thread引数
 *
 * @return 0 成功.
 */
unsigned __stdcall IscLog::LogThread(void* context)
{
	IscLog* isc_lLog = (IscLog*)context;

	if (isc_lLog == nullptr) {
		return -1;
	}

	int ret = isc_lLog->LogProc();

	return ret;
}

/**
 * Thread処理本体です.
 *
 * @return 0 成功.
 */
int IscLog::LogProc()
{

	DWORD sleep_time = 30;

	wchar_t head[2048] = {};
	wchar_t buffer[2048] = {};
	int count = 0;
	Level level = Level::Information;
	SYSTEMTIME st;

	while (thread_terminate_request_ < 1) {

		int get_index = log_data_que_->get(&st, &level, head, sizeof(head) / sizeof(wchar_t), buffer, sizeof(buffer) / sizeof(wchar_t));
		if (get_index >= 0) {

			// Log
			switch (level)
			{
			case Level::Debug:
				this->LogForThread(&st, head, level, buffer);
				break;
			case Level::Error:
				this->LogForThread(&st, head, level, buffer);
				break;
			case Level::Warning:
				this->LogForThread(&st, head, level, buffer);
				break;
			case Level::Information:
				this->LogForThread(&st, head, level, buffer);
				break;
			default:
				break;
			}

			log_data_que_->done(get_index);

			// 一旦全部書き込む
			while (true) {

				if (abort_request_) {
					break;
				}

				int get_index = log_data_que_->get(&st, &level, head, sizeof(head) / sizeof(wchar_t), buffer, sizeof(buffer) / sizeof(wchar_t));
				if (get_index >= 0) {

					// Log
					switch (level)
					{
					case Level::Debug:
						this->LogForThread(&st, head, level, buffer);
						break;
					case Level::Error:
						this->LogForThread(&st, head, level, buffer);
						break;
					case Level::Warning:
						this->LogForThread(&st, head, level, buffer);
						break;
					case Level::Information:
						this->LogForThread(&st, head, level, buffer);
						break;
					default:
						break;
					}

					log_data_que_->done(get_index);
				}
				else {
					break;
				}
			}

			sleep_time = 1;
		}
		else {
			sleep_time = 30;
		}

		Sleep(sleep_time);
	}

	thread_terminate_request_ = 0;
	thread_terminate_done_ = 1;

	return 0;
}
