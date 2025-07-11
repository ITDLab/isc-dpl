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
 * @file isc_file_read_control_impl.h
 * @brief read data from file
 */
 

#pragma once

/**
 * @class   IscFileReadControlImpl
 * @brief   implementation class
 * this class is an implementation for file control
 */
class IscFileReadControlImpl
{

public:
	IscFileReadControlImpl();
	~IscFileReadControlImpl();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize(const IscCameraControlConfiguration* isc_camera_control_configuration);

	/** @brief ... Shut down the system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	/** @brief start image read from file.
		@return 0, if successful.
	*/
	int Start(const IscGrabStartMode* isc_grab_start_mode);

	/** @brief stop image read sequence.
		@return 0, if successful.
	*/
	int Stop();

	/** @brief get the current capture mode.
		@return 0, if successful.
	*/
	int GetGrabMode(IscGrabStartMode* isc_grab_start_mode);

	// image & data get 

	/** @brief get captured data.
		@return 0, if successful.
	*/
	int GetData(IscImageInfo* isc_image_info);

	/** @brief get informaton from file.
		@return 0, if successful.
	*/
	int GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header, IscPlayFileInformation* play_file_information);

	/** @brief Set the read Frame to the specified number.
		@return 0, if successful.
	*/
	int SetReadFrameNumber(const __int64 frame_number);

	/** @brief Get the status.
		@return 0, if successful.
	*/
	int GetFileReadStatus(__int64* frame_number, IscFileReadStatus* file_read_status);


private:

	IscCameraControlConfiguration isc_camera_control_config_;
	IscGrabStartMode isc_grab_start_mode_;

	struct FileRaedInformation {
		IscFileReadStatus file_read_status;
		wchar_t read_file_name[_MAX_PATH];
		unsigned __int64 file_size;

		HANDLE handle_file;
		bool is_file_ready;

		IscRawFileHeader raw_file_header;
		unsigned __int64 total_read_size;

		__int64 current_frame_number;

		bool request_fo_designated_number;
		__int64 designated_number;

		IscPlayFileInformation play_file_information;
	};
	FileRaedInformation file_read_information_;

	struct RawReadData {
		IscRawDataHeader isc_raw_data_header;
		int width, height;
		unsigned char* buffer;
	};
	RawReadData raw_read_data_;

	RawDataDecoder* raw_data_decoder_;

	bool GetDatFileSize(TCHAR* file_name, unsigned __int64* file_size);

	int ReadOneRawData(IscImageInfo* isc_image_info);
	int ReadColorRawData(IscImageInfo* isc_image_info);
	int ReadDoubleShutterRawData(IscImageInfo* isc_image_info);
	int ReadDoubleShutterColorRawData(IscImageInfo* isc_image_info);

	int MoveToSpecifyFrameNumber(const __int64 specify_frame_number);


};
