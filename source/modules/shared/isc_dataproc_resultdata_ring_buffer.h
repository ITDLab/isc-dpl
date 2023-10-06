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
	@file isc_dataproc_resultdata_ring_buffer.h
	@brief implementation class of buffer data processing result data.
*/

#pragma once

/**
 * @class   IscDataprocResultdataRingBuffer
 * @brief   implementation class
 * this class is an buffer for data processing result control
 */
class IscDataprocResultdataRingBuffer {
public:

	/** @struct  BufferData
	 *  @brief buffer for result data
	 */
	struct BufferData {
		int inedx;		/**< buffer number */
		int state;		/**< 0:nothing 1:under write 2: write done 3:read/using */
		ULONGLONG time;	/**< put time */

		IscDataProcResultData isc_dataproc_resultdata;	/**< result data */
	};

	IscDataprocResultdataRingBuffer();
	~IscDataprocResultdataRingBuffer();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before any function is used.
		@return 0, if successful.
	*/
	int Initialize(const bool last_mpde, const bool allow_overwrite, const int count, const int width_def, const int height_def, const int channel_count_def);

	/** @brief Initialize internal variables.
		@return 0, if successful.
	*/
	int Clear();

	/** @brief ... Shut down the system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	/** @brief it gets a buffer.
		@return index of buffer.
	*/
	int GetPutBuffer(BufferData** buffer_data, const ULONGLONG time);

	/** @brief it ends the use of the buffer.
		@return index of buffer.
	*/
	int DonePutBuffer(const int index, const int status);

	/** @brief it gets a buffer.
		@return index of buffer.
	*/
	int GetGetBuffer(BufferData** buffer_data, ULONGLONG* time_get);

	/** @brief it ends the use of the buffer.
		@return index of buffer.
	*/
	void DoneGetBuffer(const int index);

private:
	CRITICAL_SECTION flag_critical_;
	bool last_mode_;
	bool allow_overwrite_;

	int buffer_count_;
	int width_, height_;
	int channel_count_;

	BufferData* buffer_data_;

	int write_inex_, read_index_, put_index_, geted_inedx_;

	unsigned char* buff_p1_;
	unsigned char* buff_p2_;
	unsigned char* buff_color_;
	float* buff_depth_;
	unsigned char* buff_raw_;
	unsigned char* buff_raw_color_;

};
