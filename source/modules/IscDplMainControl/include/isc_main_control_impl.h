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
 * @file isc_main_control_impl.h
 * @brief main control class for ISC_DPL
 */
 
#pragma once

/**
 * @class   IscMainControlImpl
 * @brief   implementation class
 * this class is an inplementation for isc DPL
 */
class IscMainControlImpl
{

public:

	IscMainControlImpl();
	~IscMainControlImpl();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize(const IscDplConfiguration* ipc_dpl_configuration);

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
	int Start(const IscStartMode* isc_start_mode);

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
	int GetCameraData(IscImageInfo* isc_image_Info);

	/** @brief get the information of the file header.
		@return 0, if successful.
	*/
	int GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header, IscPlayFileInformation* play_file_information);

	/** @brief Set the read Frame to the specified number.
		@return 0, if successful.
	*/
	int SetReadFrameNumber(const __int64 frame_number);

	// get information for depth, distance, ...

	/** @brief gets the distance of the given coordinates.
		@return 0, if successful.
	*/
	int GetPositionDepth(const int x, const int y, const IscImageInfo* isc_image_info, float* disparity, float* depth);

	/** @brief gets the 3D position of the given coordinates.
		@return 0, if successful.
	*/
	int GetPosition3D(const int x, const int y, const IscImageInfo* isc_image_info, float* x_d, float* y_d, float* z_d);

	/** @brief get information for the specified region.
		@return 0, if successful.
	*/
	int GetAreaStatistics(const int x, const int y, const int width, const int height, const IscImageInfo* isc_image_info, IscAreaDataStatistics* isc_data_statistics);

	// data processing module settings

	/** @brief get the number of installed modules.
		@return 0, if successful.
	*/
	int GetTotalModuleCount(int* total_count);

	/** @brief get the name of the specified module.
		@return 0, if successful.
	*/
	int GetModuleNameByIndex(const int module_index, wchar_t* module_name, int max_length);

	/** @brief get the parameters of the specified module.
		@return 0, if successful.
	*/
	int GetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter);

	/** @brief sets a parameter to the specified module.
		@return 0, if successful.
	*/
	int SetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file);

	/** @brief gets the name of the configuration file for the specified module.
		@return 0, if successful.
	*/
	int GetParameterFileName(const int module_index, wchar_t* file_name, const int max_length);

	/** @brief requests the specified module to reload its parameters.
		@return 0, if successful.
	*/
	int ReloadParameterFromFile(const int module_index, const wchar_t* file_name, const bool is_valid);

	// data processing module result data

	/** @brief initialize IscDataProcResultData. Allocate the required space.
		@return 0, if successful.
	*/
	int InitializeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data);

	/** @brief release the allocated space.
		@return 0, if successful.
	*/
	int ReleaeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data);

	/** @brief get module processing result.
		@return 0, if successful.
	*/
	int GetDataProcModuleData(IscDataProcResultData* isc_data_proc_result_data);


private:
	IscLog* isc_log_;
	wchar_t log_file_name_[_MAX_PATH];

	LARGE_INTEGER freq_for_performance_counter;

	IscDplConfiguration ipc_dpl_configuration_;

	IscCameraControl* isc_camera_control_;
	IscDataProcessingControl* isc_data_processing_control_;

	IscImageInfoRingBuffer* isc_image_info_ring_buffer_;
	IscMeasurement* isc_measurement_;

	IscGrabStartMode temp_isc_grab_start_mode_;
	IscDataProcStartMode temp_isc_dataproc_start_mode_;

	struct WorkBuffers {
		int max_width;
		int max_height;

		unsigned char* image_buffer[4];
		float* depth_buffer[4];
	};
	WorkBuffers work_buffers_;

	// Thread Control
	struct ThreadControl {
		int terminate_request;
		int terminate_done;
		int end_code;
		bool stop_request;
	};
	ThreadControl thread_control_camera_;

	HANDLE handle_semaphore_camera_;
	HANDLE thread_handle_camera_;
	CRITICAL_SECTION	threads_critical_camera_;

	static unsigned __stdcall ControlThreadCamera(void* context);
	int RecieveDataProcCamera();

	int CopyIscImageInfo(IscImageInfo* dst_isc_image_Info, IscImageInfo* src_isc_image_Info);

};
