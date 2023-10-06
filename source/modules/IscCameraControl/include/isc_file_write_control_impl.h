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
 * @file isc_file_write_control_impl.h
 * @brief wrte data to file
 */

#pragma once

/**
 * @class   IscFileWriteControlImpl
 * @brief   implementation class
 * this class is an implementation for file control
 */
class IscFileWriteControlImpl
{

public:

	IscFileWriteControlImpl();
	~IscFileWriteControlImpl();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize(const IscCameraControlConfiguration* isc_camera_control_configuration, const IscSaveDataConfiguration* isc_save_data_configuration, const int width, const int height, IscLog* isc_log);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	/** @brief start image write to file.
		@return 0, if successful.
	*/
	int Start(const IscCameraSpecificParameter* camera_specific_parameter, const IscGrabStartMode* isc_grab_start_mode, const IscShutterMode shutter_mode);

	/** @brief add image for write.
		@return 0, if successful.
	*/
	int Add(IscImageInfo* isc_image_info);

	/** @brief stop image write sequence.
		@return 0, if successful.
	*/
	int Stop();

	/** @brief get thread's running state.
		@return 0, if successful.
	*/
	int QueryThreadStatus(bool* is_running);

private:

	IscCameraControlConfiguration isc_camera_control_config_;
	int camera_width_, camera_height_;

	IscSaveDataConfiguration isc_save_data_configuration_;

	IscImageInfoRingBuffer* isc_image_info_ring_buffer_;

	IscLog* isc_log_;

	UtilityMeasureTime* utility_measure_time_;

	struct FileWriteSpeedInformation {
		ULONGLONG start_time;

		int check_interval_count;
		int write_count;

		int Init(int interval_count)
		{
			check_interval_count = interval_count;
			start_time = 0;
			write_count = 0;
			return 0;
		}

		int Start()
		{
			start_time = 0;
			write_count = 0;
			return 0;
		}

		int WriteOnce()
		{
			if (write_count == 0) {
				start_time = GetTickCount64();
				write_count++;
				return -1;
			}
			write_count++;
			if (write_count >= check_interval_count) {
				ULONGLONG time = GetTickCount64();
				double elapsed_time = (double)(time - start_time) / 1000.0;

				int fps = (int)((double)write_count / elapsed_time);

				write_count = 0;

				return fps;
			}

			return -1;
		}
	};
	FileWriteSpeedInformation file_write_speed_info_;

	struct FileWriteInformation {
		int target_folder_count;
		int current_folder_index;
		wchar_t root_folder[kISC_SAVE_MAX_SAVE_FOLDER_COUNT][_MAX_PATH];
		wchar_t write_folder[kISC_SAVE_MAX_SAVE_FOLDER_COUNT][_MAX_PATH];
		wchar_t write_file_name[_MAX_PATH];

		__int64 initial_size;

		__int64 minimum_capacity_required;				/**< 最小ディスク空き容量 */

		__int64 start_time_of_current_file_msec;		/**< 現在のファイルの保存開始時間 msec(=GetTickCount) */
		int save_time_for_one_file_sec;					/**< 保存ファイルの1個あたりの時間 (秒) */ 

		__int64 previous_time_free_space_monitoring;	/**< 空き容量の監視　前回の時間 msec(=GetTickCount) */
		int free_space_monitoring_cycle_sec;			/**< 空き容量の監視周期 (秒) */

		HANDLE handle_file;
		bool is_file_ready;

		unsigned int frame_index;
		IscRawFileHeader raw_file_hedaer;
	};
	FileWriteInformation file_write_information_;

	HRESULT EnablePrivilege(const wchar_t* privilege_name, const BOOL enabled);
	int PrepareFileforWriting(FileWriteInformation* file_write_information);
	int CreateWriteFile(FileWriteInformation* file_write_information);

	int PrepareNewFileforWriting(FileWriteInformation* file_write_information);
	int CreateNewWriteFile(FileWriteInformation* file_write_information);

	int WriteDataToFile(HANDLE handle_file, LPCVOID buffer, __int64 number_of_write_to_file, __int64* number_of_byteswritten, LPOVERLAPPED overlapped);

	int CheckFreeSpace(FileWriteInformation* file_write_information);
		
	// Thread Control
	struct ThreadControl {
		int terminate_request;
		int terminate_done;
		int end_code;
		bool stop_request;
	};
	ThreadControl thread_control_;

	HANDLE handle_semaphore_;
	HANDLE thread_handle_;
	CRITICAL_SECTION	threads_critical_;

	static unsigned __stdcall ControlThread(void* context);
	int WriteDataProc(IscFileWriteControlImpl* isc_file_write_Control);

	// 空き容量がOKか？
	bool GetFreeDiskSpace(const wchar_t* folder, unsigned __int64* free_disk_space);
	bool CheckDiskFreeSpace(const wchar_t* target_folder, const unsigned __int64 requested_size);

};
