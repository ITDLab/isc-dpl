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
 * @file isc_camera_control.h
 * @brief Common camera control
 */

#pragma once

#ifdef ISCCAMERACONTROL_EXPORTS
#define ISCCAMERACONTROL_API __declspec(dllexport)
#else
#define ISCCAMERACONTROL_API __declspec(dllimport)
#endif

/**
 * @class   IscCameraControl
 * @brief   interface class
 * this class is an interface for camera
 */
class ISCCAMERACONTROL_API IscCameraControl
{

public:

	IscCameraControl();
	~IscCameraControl();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize(const IscCameraControlConfiguration* isc_camera_control_configuration, IscLog* isc_log);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	// camera dependent paraneter

	/** @brief whether or not the parameter is implemented.
		@return true, if implemented.
	*/
	bool DeviceOptionIsImplemented(const IscCameraInfo option_name);

	/** @brief whether the parameter is readable.
		@return true, if readable.
	*/
	bool DeviceOptionIsReadable(const IscCameraInfo option_name);

	/** @brief whether the parameter is writable.
		@return true, if writable.
	*/
	bool DeviceOptionIsWritable(const IscCameraInfo option_name);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraInfo option_name, int* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraInfo option_name, int* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionInc(const IscCameraInfo option_name, int* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, int* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const int value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraInfo option_name, float* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraInfo option_name, float* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, float* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const float value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, bool* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const bool value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, char* value, const int max_length);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const char* value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraInfo option_name, uint64_t* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraInfo option_name, uint64_t* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionInc(const IscCameraInfo option_name, uint64_t* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraInfo option_name, uint64_t* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraInfo option_name, const uint64_t value);

	// camera control parameter

	/** @brief whether or not the parameter is implemented.
		@return true, if implemented.
	*/
	bool DeviceOptionIsImplemented(const IscCameraParameter option_name);

	/** @brief whether the parameter is readable.
		@return true, if readable.
	*/
	bool DeviceOptionIsReadable(const IscCameraParameter option_name);

	/** @brief whether the parameter is writable.
		@return true, if writable.
	*/
	bool DeviceOptionIsWritable(const IscCameraParameter option_name);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraParameter option_name, int* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraParameter option_name, int* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionInc(const IscCameraParameter option_name, int* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, int* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const int value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraParameter option_name, float* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraParameter option_name, float* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, float* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const float value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, bool* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const bool value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, char* value, const int max_length);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const char* value);

	/** @brief get the minimum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMin(const IscCameraParameter option_name, uint64_t* value);

	/** @brief get the maximum value of a parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionMax(const IscCameraParameter option_name, uint64_t* value);

	/** @brief Gets the unit of increment or decrement for the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOptionInc(const IscCameraParameter option_name, uint64_t* value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, uint64_t* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const uint64_t value);

	/** @brief get the value of the parameter.
		@return 0, if successful.
	*/
	int DeviceGetOption(const IscCameraParameter option_name, IscShutterMode* value);

	/** @brief set the parameters.
		@return 0, if successful.
	*/
	int DeviceSetOption(const IscCameraParameter option_name, const IscShutterMode value);

	// grab control

	/** @brief start image acquisition.
		@return 0, if successful.
	*/
	int Start(const IscGrabStartMode* isc_grab_start_mode);

	/** @brief stop image capture.
		@return 0, if successful.
	*/
	int Stop();

	/** @brief get the current capture mode.
		@return 0, if successful.
	*/
	int GetGrabMode(IscGrabStartMode* isc_grab_start_mode);

	// image & data get 

	/** @brief initialize IscImageInfo. Allocate the required space.
		@return 0, if successful.
	*/
	int InitializeIscIamgeinfo(IscImageInfo* isc_image_Info);

	/** @brief release the allocated space.
		@return 0, if successful.
	*/
	int ReleaeIscIamgeinfo(IscImageInfo* isc_image_Info);

	/** @brief get captured data.
		@return 0, if successful.
	*/
	int GetData(IscImageInfo* isc_image_Info);

	/** @brief get informaton from file.
	@return 0, if successful.
	*/
	int GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header);

private:
	IscLog* isc_log_;

	UtilityMeasureTime* measure_tackt_time_;

	IscCameraControlConfiguration isc_camera_control_config_;

	IscCameraSpecificParameter camera_specific_parameter_;

	IscSdkControl* isc_sdk_control_;

	IscFileWriteControlImpl* isc_file_write_control_impl_;
	IscFileReadControlImpl* isc_file_read_control_impl_;

	enum class IscRunState {
		stop,
		run
	};
	struct IscRunStatus {
		IscRunState isc_run_state;
		IscGrabStartMode isc_grab_start_mode;
	};
	IscRunStatus isc_run_status_;

	IscImageInfoRingBuffer* isc_image_info_ring_buffer_;

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
	int RecieveDataProc();

	int ImageHandler(IscImageInfoRingBuffer::BufferData* buffer_data);

	int GetDataLiveCamera(IscImageInfo* isc_image_info);
	int GetDataReadFile(IscImageInfo* isc_image_info);


};

