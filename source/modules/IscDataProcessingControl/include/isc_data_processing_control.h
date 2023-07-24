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
 * @file isc_data_processing_control.h
 * @brief main control class for ISC_DPL
 */
 
#pragma once

#ifdef ISCDATAPROCESSINGCONTROL_EXPORTS
#define ISCDATAPROCESSINGCONTROL_EXPORTS_API __declspec(dllexport)
#else
#define ISCDATAPROCESSINGCONTROL_EXPORTS_API __declspec(dllimport)
#endif

/**
 * @class   IscDataProcessingControl
 * @brief   interface class
 * this class is an interface for data processing module
 */
class ISCDATAPROCESSINGCONTROL_EXPORTS_API IscDataProcessingControl
{
public:

    IscDataProcessingControl();
    ~IscDataProcessingControl();

  	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	 */
	int Initialize(IscDataProcModuleConfiguration* isc_data_proc_module_configuration);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();
  
	/** @brief it will start processing.
		@return 0, if successful.
	 */
	int Start(const IscDataProcStartMode* isc_dataproc_start_mode);

	/** @brief stop processing.
		@return 0, if successful.
	 */
	int Stop();

    // data processing module settings

	/** @brief get the number of modules implemented.
		@return 0, if successful.
	 */
	int GetTotalModuleCount(int* total_count);

	/** @brief get the name of the specified module.
		@return 0, if successful.
	 */
	int GetModuleNameByIndex(const int module_index, wchar_t* module_name, int max_length);

	/** @brief get the configuration of the specified module.
		@return 0, if successful.
	 */
	int GetParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter);

	/** @brief sets a parameter to the specified module.
		@return 0, if successful.
	 */
	int SetParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file);

	/** @brief gets the name of the parameter file for the specified module.
		@return 0, if successful.
	 */
	int GetParameterFileName(const int module_index, wchar_t* file_name, const int max_length);

	/** @brief get parameters for the specified module from a file.
		@return 0, if successful.
	 */
	int ReloadParameterFromFile(const int module_index, const wchar_t* file_name, const bool is_valid);

	// get data

	/** @brief initialize IscStereoDisparityData. Allocate the required space.
		@return 0, if successful.
	*/
	int InitializeIscBlockDisparityData(IscBlockDisparityData* isc_stereo_disparity_data);

	/** @brief release the allocated space.
		@return 0, if successful.
	*/
	int ReleaeIscIscBlockDisparityData(IscBlockDisparityData* isc_stereo_disparity_data);

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

	// data processing module 

	/** @brief do the processing.
		@return 0, if successful.
	*/
	int Run(IscImageInfo* isc_image_info);

private:

	UtilityMeasureTime* measure_time_;

	IscDataProcModuleConfiguration isc_data_proc_module_configuration_;

	IscDataProcStartMode isc_dataproc_start_mode_;

	IscImageInfoRingBuffer* isc_image_info_ring_buffer_;
	IscDataprocResultdataRingBuffer* isc_dataproc_resultdata_ring_buffer_;

	IscBlockDisparityData isc_block_disparity_data_;

	// modules
	IscFramedecoderInterface* isc_frame_decoder_;
	IscStereoMatchingInterface* isc_stereo_matching_;
	IscDisparityFilterInterface* isc_disparity_filter_;

	// Thread Control
	struct ThreadControl {
		int terminate_request;
		int terminate_done;
		int end_code;
		bool stop_request;
	};
	ThreadControl thread_control_dataproc_;

	HANDLE handle_semaphore_dataproc_;
	HANDLE thread_handle_dataproc_;
	CRITICAL_SECTION	threads_critical_dataproc_;

	static unsigned __stdcall ControlThreadDataProc(void* context);
	int DataProc();

	// data processing module 
	int SyncRun(IscImageInfo* isc_image_info);
	int AsyncRun(IscImageInfo* isc_image_info);
	int ClearIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data);

	int RunDataProcModules(IscImageInfo* isc_image_info, IscDataProcResultData* isc_data_proc_result_data);
	int RunDataProcStereoMatching(IscImageInfo* isc_image_info, IscDataProcResultData* isc_data_proc_result_data);
	int RunDataProcFrameDecoder(IscImageInfo* isc_image_info, IscDataProcResultData* isc_data_proc_result_data);
	int RunDataProcFrameDecoderInDoubleShutter(IscImageInfo* isc_image_info, IscDataProcResultData* isc_data_proc_result_data);

};
