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

// Here is an example using OpenCV
// Capture and display an image

#include <Windows.h>
#include <shlwapi.h>
#include <stdint.h>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"
#include "dpl_controll.h"

#include "opencv2\opencv.hpp"

#pragma comment (lib, "shlwapi")
#pragma comment (lib, "IscDpl")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world470d")
#else
#pragma comment (lib,"opencv_world470")
#endif

struct ImageState {
	int grab_mode;		// *not used* 0:parallax  1: after correct 1:before correct
	int color_mode;		// 0:off 1:on
	int width, height;

	float b, bf, dinf;
	double angle;

	DplControl* dpl_control;
	IscImageInfo isc_image_Info;
	IscDataProcResultData isc_data_proc_result_data;

	unsigned char* bgra_image;
};

int ImageHandler(const int display_scale, const int display_mode, ImageState* image_state);

void PrintUsage(void)
{
	std::cout << "SAMPLE VIWER FOR DPL\n";
	std::cout << "\n";
	std::cout << "[KEY] ESC -> exit\n";
	std::cout << "[KEY] s -> start grab\n";
	std::cout << "[KEY] t -> stop grab\n";
	std::cout << "[KEY] c -> toggle color on/off, default off, Reflected at start\n";
	std::cout << "[KEY] m -> toggle diaplay mode 0:Independent only 1: Display overlaped image too, default 0\n";
	std::cout << "[KEY] + -> enlargement\n";
	std::cout << "[KEY] - -> reduction\n";
	std::cout << "\n";

	return;
}


int main(int argc, char* argv[]) try
{
	// for Visual Studio debug console
	SetConsoleOutputCP(932);

	if (argc < 2) {
		std::cout << "Usage : ViewOCV.exe camera_model\n";
		std::cout << "         camera_model:0:VM 1:XC\n";

		return 0;
	}

	char mdule_file_name[_MAX_PATH] = {};
	DWORD len = GetModuleFileNameA(NULL, mdule_file_name, sizeof(mdule_file_name));

	char* locRet = setlocale(LC_ALL, "ja-JP");
	wchar_t module_path[_MAX_PATH] = {};

	size_t return_value = 0;
	mbstate_t mb_state = { 0 };
	const char* src = mdule_file_name;
	size_t size_in_words = _MAX_PATH;
	errno_t err = mbsrtowcs_s(&return_value, &module_path[0], size_in_words, &src, size_in_words - 1, &mb_state);
	if (err != 0) {
		return 0;
	}

	PathRemoveFileSpec(module_path);

	const int camera_model = atoi(argv[1]);
	if (camera_model == 0 || camera_model == 1) {
		if (camera_model == 0) {
			std::cout << "[INFO]Your specified camera is a VM\n";
		}
		else if (camera_model == 1) {
			std::cout << "[INFO]Your specified camera is a XC\n";
		}
		std::cout << "\n";
	}
	else {
		std::cout << "Usage : ViewOCV.exe camera_model\n";
		std::cout << "         camera_model:0:VM 1:XC\n";

		return 0;
	}

	PrintUsage();

	// open modules
	ImageState image_state = {};

	image_state.dpl_control = new DplControl;
	bool ret = image_state.dpl_control->Initialize(module_path, camera_model);
	if (!ret) {
		std::cout << "[ERR]initialization failed\n";

		image_state.dpl_control->Terminate();
		delete image_state.dpl_control;

		return 0;
	}

	ret = image_state.dpl_control->InitializeBuffers(&image_state.isc_image_Info, &image_state.isc_data_proc_result_data);
	if (!ret) {
		std::cout << "[ERR]initialization failed\n";

		image_state.dpl_control->Terminate();
		delete image_state.dpl_control;

		return 0;
	}

	ret = image_state.dpl_control->GetCameraParameter(&image_state.b, &image_state.bf, &image_state.dinf, &image_state.width, &image_state.height);
	if (!ret) {
		std::cout << "[ERR]initialization failed\n";

		image_state.dpl_control->Terminate();
		delete image_state.dpl_control;

		return 0;
	}

	if (image_state.width != 0 && image_state.height != 0) {
		image_state.bgra_image = new unsigned char[image_state.width * image_state.height * 4];
	}

	// 表示windowの準備
	cv::namedWindow("Base Image", cv::WINDOW_AUTOSIZE | cv::WINDOW_FREERATIO);
	cv::namedWindow("Depth Image", cv::WINDOW_AUTOSIZE | cv::WINDOW_FREERATIO);
	cv::namedWindow("Blend Image", cv::WINDOW_AUTOSIZE | cv::WINDOW_FREERATIO);

	int temp_key = cv::waitKey(15) & 0x000000FF;

	// start process

	// Diaply Parameter
	int display_scale = 1;
	int display_mode = 0;

	// start mode
	DplControl::StartMode start_mode = {};

	// start camera process
	enum class MainState {
		idle = 0,
		start,
		run,
		stop,
		exit
	};

	MainState main_state = MainState::idle;

	struct RequestFlags {
		bool start, stop, exit;
	};

	RequestFlags request_flags;
	request_flags.start = false;
	request_flags.stop = false;
	request_flags.exit = false;

	bool proessEndRequest = false;

	while (true) {

		switch (main_state) {
		case MainState::idle:
			if (request_flags.exit) {
				request_flags.exit = false;
				main_state = MainState::exit;
			}
			else if (request_flags.start) {
				if (image_state.color_mode == 1) {
					start_mode.enabled_color = true;
				}
				else {
					start_mode.enabled_color = false;
				}
				request_flags.start = false;
				main_state = MainState::start;
			}
			break;

		case MainState::start:
			ret = image_state.dpl_control->Start(start_mode);

			if (ret != 0) {
				main_state = MainState::exit;
			}

			main_state = MainState::run;
			break;

		case MainState::run:
			// get image and show it
			if (request_flags.exit) {
				image_state.dpl_control->Stop();
				request_flags.exit = false;
				main_state = MainState::exit;
			}
			else if (request_flags.stop) {
				request_flags.stop = false;
				main_state = MainState::stop;
			}
			else {
				ret = ImageHandler(display_scale, display_mode, &image_state);

				if (ret != 0) {
					main_state = MainState::exit;
				}
			}
			break;

		case MainState::stop:
			image_state.dpl_control->Stop();
			main_state = MainState::idle;
			break;

		case MainState::exit:
			proessEndRequest = true;
			break;

		default:
			break;
		}

		// キーが押されるまで待つ
		int key = cv::waitKey(15) & 0x000000FF;
		if (key == 27) {
			// ESC
			request_flags.exit = true;;
		}
		else if (key == 43) {
			// +
			display_scale--;
			if (display_scale < 1) {
				display_scale = 1;
			}
		}
		else if (key == 45) {
			// -
			display_scale++;
			if (display_scale > 4) {
				display_scale = 4;
			}
		}
		else if (key == 115) {
			// s
			request_flags.start = true;
		}
		else if (key == 116) {
			// t
			request_flags.stop = true;
		}
		else if (key == 109) {
			// m
			display_mode++;
			if (display_mode > 1) {
				display_mode = 0;
			}
		}
		else if (key == 99) {
			// c
			if (image_state.color_mode == 0) {
				image_state.color_mode = 1;
			}
			else {
				image_state.color_mode = 0;
			}
		}
		else if (key == 50) {
			// 2
			image_state.grab_mode = 2;
		}
		else if (key == 51) {
			// 3
			image_state.grab_mode = 3;
		}
		else if (key == 52) {
			// 4
			image_state.grab_mode = 4;
		}

		if (proessEndRequest) {
			break;
		}
	}

	cv::destroyAllWindows();

	// ended
	delete[] image_state.bgra_image;
	ret = image_state.dpl_control->ReleaseBuffers(&image_state.isc_image_Info, &image_state.isc_data_proc_result_data);

	image_state.dpl_control->Terminate();
	delete image_state.dpl_control;
	image_state.dpl_control = nullptr;

	return 0;
}
catch (const std::exception& e)
{
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}

int ImageHandler(const int display_scale, const int display_mode, ImageState* image_state)
{

	// images from camera
	bool camera_status = image_state->dpl_control->GetCameraData(&image_state->isc_image_Info);
	const int fd_inex = kISCIMAGEINFO_FRAMEDATA_LATEST;

	if ((image_state->isc_image_Info.frame_data[fd_inex].p1.width == 0) ||
		(image_state->isc_image_Info.frame_data[fd_inex].p1.height == 0)) {

		camera_status = false;
	}

	cv::Mat mat_base_image_scale_flip;
	if (camera_status) {
		bool is_color_exists = false;
		if (image_state->color_mode == 1) {
			if ((image_state->isc_image_Info.frame_data[fd_inex].color.width != 0) &&
				(image_state->isc_image_Info.frame_data[fd_inex].color.height != 0)) {

				is_color_exists = true;
			}
		}

		if (is_color_exists) {
			// color image
			cv::Mat mat_base_image(image_state->isc_image_Info.frame_data[fd_inex].color.height, image_state->isc_image_Info.frame_data[fd_inex].color.width, CV_8UC3, image_state->isc_image_Info.frame_data[fd_inex].color.image);

			double ratio = 1.0 / (double)display_scale;
			cv::Mat mat_base_image_scale;
			cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

			cv::flip(mat_base_image_scale, mat_base_image_scale_flip, -1);

			cv::imshow("Base Image", mat_base_image_scale_flip);
		}
		else {
			// base image
			cv::Mat mat_base_image(image_state->isc_image_Info.frame_data[fd_inex].p1.height, image_state->isc_image_Info.frame_data[fd_inex].p1.width, CV_8U, image_state->isc_image_Info.frame_data[fd_inex].p1.image);

			double ratio = 1.0 / (double)display_scale;
			cv::Mat mat_base_image_scale;
			cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

			cv::Mat mat_base_image_scale_flip_temp;
			cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

			cv::Mat mat_base_image_color;
			cv::cvtColor(mat_base_image_scale_flip_temp, mat_base_image_scale_flip, cv::COLOR_GRAY2RGB);

			cv::imshow("Base Image", mat_base_image_scale_flip);
		}
	}

	// data processing result
	bool data_proc_status = image_state->dpl_control->GetDataProcessingData(&image_state->isc_data_proc_result_data);

	if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.width == 0) ||
		(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.height == 0)) {

		data_proc_status = false;
	}

	cv::Mat mat_depth_image_scale_flip;
	if (data_proc_status) {
		// depth
		const int width = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.width;
		const int height = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.height;
		float* depth = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.image;

		image_state->dpl_control->ConvertDisparityToImage(image_state->b, image_state->angle, image_state->bf, image_state->dinf,
			width, height, depth, image_state->bgra_image);

		cv::Mat mat_depth_image_temp(height, width, CV_8UC4, image_state->bgra_image);

		cv::Mat mat_depth_image;
		cv::cvtColor(mat_depth_image_temp, mat_depth_image, cv::COLOR_BGRA2BGR);

		double ratio = 1.0 / (double)display_scale;
		cv::Mat mat_depth_image_scale;
		cv::resize(mat_depth_image, mat_depth_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

		cv::flip(mat_depth_image_scale, mat_depth_image_scale_flip, -1);

		cv::imshow("Depth Image", mat_depth_image_scale_flip);
	}

	// overlap image
	if (display_mode == 1) {
		if (camera_status && data_proc_status) {

			if (mat_base_image_scale_flip.empty()) {
				return 0;
			}

			if (mat_depth_image_scale_flip.empty()) {
				return 0;
			}

			cv::Mat mat_data_proc_image_scale_flip;
			bool is_color_exists = false;
			if (image_state->color_mode == 1) {
				if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.width != 0) &&
					(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.height != 0)) {

					is_color_exists = true;
				}
			}

			if (is_color_exists) {
				// color image
				cv::Mat mat_base_image(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.height,
					image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.width,
					CV_8UC3,
					image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.image);

				double ratio = 1.0 / (double)display_scale;
				cv::Mat mat_base_image_scale;
				cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

				cv::flip(mat_base_image_scale, mat_data_proc_image_scale_flip, -1);

			}
			else {
				// base image
				cv::Mat mat_base_image(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.height,
					image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.width,
					CV_8U,
					image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.image);

				double ratio = 1.0 / (double)display_scale;
				cv::Mat mat_base_image_scale;
				cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

				cv::Mat mat_base_image_scale_flip_temp;
				cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

				cv::Mat mat_base_image_color;
				cv::cvtColor(mat_base_image_scale_flip_temp, mat_data_proc_image_scale_flip, cv::COLOR_GRAY2RGB);
			}

			double alpha = 0.7;
			double beta = (1.0 - alpha);

			// blend it
			cv::Mat mat_blend_image;
			cv::addWeighted(mat_data_proc_image_scale_flip, alpha, mat_depth_image_scale_flip, beta, 0.0, mat_blend_image);

			cv::imshow("Blend Image", mat_blend_image);
		}
	}

	return 0;
}
