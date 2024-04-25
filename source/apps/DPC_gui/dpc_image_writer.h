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

#pragma once

class DpcImageWriter
{

public:

	DpcImageWriter();
	~DpcImageWriter();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize(const int max_width, const int max_height, const TCHAR* save_image_path);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	struct ImageData {
		TCHAR id_string[16];
		int width;
		int height;
		int channel_count;
		bool is_rotate;

		unsigned char* buffer;
	};

	struct DepthData {
		TCHAR id_string[16];
		int width;
		int height;
		bool is_rotate;

		double camera_b;			/**< カメラパラメータ */
		double camera_dinf;			/**< カメラパラメータ */
		double camera_bf;			/**< カメラパラメータ */
		double camera_set_angle;	/**< カメラパラメータ */

		float* buffer;
	};

	struct PCDData {

		TCHAR id_string[16];
		int width;
		int height;
		int channel_count;
		bool is_rotate;

		double camera_b;			/**< カメラパラメータ */
		double camera_dinf;			/**< カメラパラメータ */
		double camera_bf;			/**< カメラパラメータ */
		double camera_set_angle;	/**< カメラパラメータ */

		double min_distance;		/**< 表示最小距離 */
		double max_distance;		/**< 表示最大距離 */

		unsigned char* image;
		float* depth;
	};

	struct ImageDepthDataSet {
		int image_datacount;
		ImageData image_data[4];

		int depth_data_count;
		DepthData depth_data[4];

		int pcd_data_count;
		PCDData pcd_data[4];
	};

	int PushImageDepthData(const ImageDepthDataSet* image_depth_data_set);

private:

	int max_width_;
	int max_height_;

	TCHAR save_image_path_[_MAX_PATH];

	// buffers
	int max_buffer_count_;
	unsigned char* image_buffers_[2];
	float* depth_buffers_[2];

	int image_data_count_;
	ImageData* image_data_buffers_;
	int depth_data_count_;
	DepthData* depth_data_buffers_;
	int pcd_data_count_;
	PCDData* pcd_data_buffers_;

	struct WorkBuffers {
		unsigned char* image_work_buffer[2];
		float* depth_work_buffer[2];
	};
	WorkBuffers work_buffers_;

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

	enum class WriteState {
		idle,
		writing,
		ended,
	};
	WriteState write_state_;

	struct pt_t {
		float x, y, z;
		unsigned int col;

		pt_t() { ; }
		pt_t(float _x, float _y, float _z, unsigned int _col) {
			x = _x;
			y = _y;
			z = _z;
			col = _col;
		}
	};

	struct PCDSetParameter {
		bool is_axis_unity;

		bool axis_reverse_x;
		bool axis_reverse_y;
		bool axis_reverse_z;
	};

	PCDSetParameter pcd_set_parameter_;

	static unsigned __stdcall ControlThread(void* context);
	int WriteDataProc(DpcImageWriter* dpc_iamge_writer);

	int WriteImageToFileAsPng(const TCHAR* file_name, const int width, const int height, const int channel_count, unsigned char* image, const bool is_rotate);
	int WriteDepthToFileAsBinary(const TCHAR* file_name, const int data_size, const unsigned char* data);
	int WriteDepthToFileAsImage(const TCHAR* file_name, const int width, const int height, const float* data);
	int WriteDepthToFileAsPCD(const TCHAR* file_name, const int width, const int height, const double base_length, const double bf, const double d_inf, 
		const double min_distance, const double max_distance, const int channel_count, unsigned char* image, float* depth, const bool is_rotate, WorkBuffers* work_buffers);


};

