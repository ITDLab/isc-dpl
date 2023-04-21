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
	@file isc_log.h
	@brief this class provides logging function.
*/

#pragma once

/**
 * @class   IscLog
 * @brief   logging class
 * this class provides logging function.
 */
class IscLog
{
public:

	/** @enum  Level
	 *  @brief Specifies the log retention level
	 */
	enum class Level
	{
		Information = 1,	/**< notification */
		Warning = 2,		/**< warning */
		Error = 3,			/**< error */
		Debug = 4,			/**< debug */
	};


public:

	IscLog();
	~IscLog();

	/** @brief initialize the class.
		@return none.
	*/
	void Open(wchar_t* save_path, wchar_t* save_file_name, int log_level, bool is_immediate_mode = false);

	/** @brief close the file and exit.
		@return none.
	*/
	void Close();

	/** @brief flush te buffer.
		@return none.
	*/
	void Flush();

	/** @brief log for debug level.
		@return none.
	*/
	void LogDebug(const wchar_t *head, const wchar_t *str);

	/** @brief log for error level.
		@return none.
	*/
	void LogError(const wchar_t *head, const wchar_t *str);

	/** @brief log for warning level.
		@return none.
	*/
	void LogWarning(const wchar_t *head, const wchar_t *str);

	/** @brief log for informaton level.
		@return none.
	*/
	void LogInfo(const wchar_t *head, const wchar_t *str);

private:

	bool immediate_mode_;	/**< mode 0:keep the file open 1:close the file after each write */

	std::mutex mutex_;		/**< mutex for thread control */

	FILE* file_;			/**< file handle */
	bool file_opened_;		/**< true:faile opened */

	/// 保存フォルダー名
	wchar_t save_path_[_MAX_PATH];			/**< root path of log file */
	wchar_t save_file_name_[_MAX_PATH];		/**< full path name of log file input */
	wchar_t log_file_name_[_MAX_PATH];		/**< current write file name */

	/// Level
	int log_level_;			/**< log level for logging */

	/// file size
	size_t file_size_;		/**< current file size */
	size_t max_file_size_;	/**< File size to update file */

	/** @brief Log function called from Thread.
		@return none.
	*/
	void LogForThread(const SYSTEMTIME* st, const wchar_t *head, Level level, const wchar_t *str);

	/// Thread
	int thread_terminate_request_;	/**< Thread termination request */
	int thread_terminate_done_;		/**< Thread stop completion flag */
	int thread_end_code_;			/**< Thread exit code */
	bool abort_request_;			/**< Suspend request for Thread */

	HANDLE handle_semaphore_;		/**< semaphore handle for log start*/
	HANDLE thread_handle_;			/**< handle of thread */
	CRITICAL_SECTION	threads_critical_;	/**< handle for critical section */

	/** @brief Functions registered in Thread.
		@return 0, if successful
	*/
	static unsigned __stdcall LogThread(void* context);

	/** @brief Functions that work with Thread.
		@return 0, if successful
	*/
	int LogProc();

	/**
	 * @class   LogDataQue
	 * @brief   que class
	 * this class provides que for string.
	 */
	class LogDataQue {
		CRITICAL_SECTION	buffer_critical_;
		bool mode_latest_;

		int write_index_, read_index_, put_index_, geted_index_, last_done_index_;

		int max_head_count_;
		int max_data_count_;
		int max_list_count_;

		struct Data {
			int index;
			int state;	// 0:nothing 1:write 3:read

			SYSTEMTIME st;
			Level level;
			wchar_t head[256];
			wchar_t data[1024];
		};

		Data* data_list_;

	public:
		LogDataQue()
		{
			max_head_count_ = 256;
			max_data_count_ = 1024;
			max_list_count_ = 256;
			write_index_ = read_index_ = put_index_ = geted_index_ = last_done_index_ = 0;
			data_list_ = nullptr;

			mode_latest_ = true;
			InitializeCriticalSection(&buffer_critical_);
		}

		~LogDataQue()
		{
			DeleteCriticalSection(&buffer_critical_);
		}

		bool open(int max_list_count, bool latest)
		{
			if (max_list_count < 1) {
				max_list_count = 1;
			}
			max_list_count_ = max_list_count;
			write_index_ = read_index_ = put_index_ = geted_index_ = last_done_index_ = 0;

			data_list_ = new Data[max_list_count_];
			for (int i = 0; i < max_list_count_; i++) {
				data_list_[i].index = i;
				data_list_[i].state = 0;
				data_list_[i].level = Level::Information;

				memset(data_list_[i].head, 0, max_head_count_);
				memset(data_list_[i].data, 0, max_data_count_);
			}

			mode_latest_ = latest;

			return true;
		}

		bool close(void)
		{
			delete[] data_list_;
			data_list_ = nullptr;

			return true;
		}

		int put(Level level, const wchar_t* head, const wchar_t* data)
		{
			if (data == nullptr) {
				return -1;
			}

			if (data_list_[write_index_].state != 0) {
				return -1;
			}

			swprintf_s(data_list_[write_index_].head, L"%s", head);
			swprintf_s(data_list_[write_index_].data, L"%s", data);

			GetLocalTime(&data_list_[write_index_].st);
			data_list_[write_index_].level = level;
			data_list_[write_index_].state = 1;

			EnterCriticalSection(&buffer_critical_);
			if (mode_latest_) {
				read_index_ = write_index_;
			}
			LeaveCriticalSection(&buffer_critical_);

			put_index_ = write_index_;

			write_index_++;
			if (write_index_ >= max_list_count_) {
				write_index_ = 0;
			}

			return put_index_;
		}

		int get(SYSTEMTIME* st, Level* level, wchar_t* head, int max_head_count, wchar_t* data, int max_data_count)
		{
			if (data == nullptr) {
				return -1;
			}
			if (data_list_[read_index_].state != 1) {
				return -1;
			}

			EnterCriticalSection(&buffer_critical_);
			int cur_index = read_index_;
			LeaveCriticalSection(&buffer_critical_);

			swprintf_s(head, max_head_count, L"%s", data_list_[cur_index].head);
			swprintf_s(data, max_data_count, L"%s", data_list_[cur_index].data);

			*level = data_list_[cur_index].level;
			*st = data_list_[cur_index].st;

			data_list_[cur_index].state = 3;
			geted_index_ = cur_index;

			EnterCriticalSection(&buffer_critical_);
			if (!mode_latest_) {
				read_index_++;
				if (read_index_ >= max_list_count_) {
					read_index_ = 0;
				}
			}
			LeaveCriticalSection(&buffer_critical_);

			return geted_index_;
		}

		void done(int index)
		{
			if (data_list_ == nullptr) {
				return;
			}
			if (index < 0 || index >= max_list_count_) {
				return;
			}
			data_list_[index].state = 0;

			if (mode_latest_) {
				int st = last_done_index_;
				int ed = index;

				if (ed < st) {
					for (int i = st; i < max_list_count_; i++) {
						data_list_[i].state = 0;
					}
					for (int i = 0; i < ed; i++) {
						data_list_[i].state = 0;
					}
				}
				else {
					for (int i = st; i < ed; i++) {
						data_list_[i].state = 0;
					}
				}

				last_done_index_ = index;
			}

			return;
		}

	};
	LogDataQue* log_data_que_;	/**< buffer for string */

};



