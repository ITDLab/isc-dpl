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

// Here is an example of a Yolo call
// Calculate and display the distance to the rectangles obtained

#include <Windows.h>
#include <shlwapi.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <queue>
#include <fstream>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"
#include "dpl_controll.h"

#include "opencv2\opencv.hpp"

#define OPENCV
#include "yolo_v2_class.hpp"    

#pragma comment (lib, "shlwapi")
#pragma comment (lib, "IscDpl")

#ifdef _DEBUG
#pragma comment (lib,"opencv_world480d")
#else
#pragma comment (lib,"opencv_world480")
#endif

#pragma comment (lib, "yolo_cpp_dll")

struct ImageState {
	int grab_mode;		// *not used* 0:parallax  1: after correct 1:before correct
	int color_mode;		// 0:off 1:on
	int width, height;
	int enabled_yolo;	// 0:off 1:on

	float b, bf, dinf;
	double angle;

	DplControl* dpl_control;
	IscImageInfo isc_image_Info;
	IscDataProcResultData isc_data_proc_result_data;

	unsigned char* bgra_image;
};

int ImageHandler(const int display_scale, const int display_mode, ImageState* image_state, Detector* detector_, std::vector<std::string>& obj_names);

void PrintUsage(void)
{
	std::cout << "SAMPLE VIWER FOR DPL\n";
	std::cout << "\n";
	std::cout << "[KEY] ESC -> exit\n";
	std::cout << "[KEY] s -> start grab\n";
	std::cout << "[KEY] t -> stop grab\n";
	std::cout << "[KEY] c -> toggle color on/off, default off, Reflected at start\n";
	std::cout << "[KEY] a -> toggle rnable AI mode, default off\n";
	std::cout << "[KEY] + -> enlargement\n";
	std::cout << "[KEY] - -> reduction\n";
	std::cout << "\n";

	return;
}

std::vector<std::string> ReadNamesFile(const char* file_name)
{
	std::ifstream ifs(file_name);
	std::vector<std::string> names_list;

	if (!ifs.is_open()) {
		return names_list;
	}

	std::string line;
	while (getline(ifs, line)) {
		names_list.push_back(line);
	}

	return names_list;
}

int main(int argc, char* argv[]) try
{
	// for Visual Studio debug console
	SetConsoleOutputCP(932);

	if (argc < 5) {
		std::cout << "Usage : ViewOCV.exe [camera_model] [voc.names] [yolo-voc.cfg] [yolo-voc.weights]\n";
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
		std::cout << "Usage : ViewOCV.exe [camera_model] [voc.names] [yolo-voc.cfg] [yolo-voc.weights]\n";
		std::cout << "         camera_model:0:VM 1:XC\n";

		return 0;
	}

	PrintUsage();

	BOOL file_exists = PathFileExistsA(argv[2]);
	if (file_exists == FALSE) {
		std::cout << "[ERROR]File does not exist " << argv[2] << std::endl;
		return 0;
	}
	file_exists = PathFileExistsA(argv[3]);
	if (file_exists == FALSE) {
		std::cout << "[ERROR]File does not exist " << argv[3] << std::endl;
		return 0;
	}
	file_exists = PathFileExistsA(argv[4]);
	if (file_exists == FALSE) {
		std::cout << "[ERROR]File does not exist " << argv[4] << std::endl;
		return 0;
	}


	// open yolo
	char* voc_file = argv[2];
	char* cfg_file = argv[3];
	char* weights_file = argv[4];

	Detector* detector_ = new Detector(std::string(cfg_file), std::string(weights_file), 0, 1);
	std::vector<std::string> object_names = ReadNamesFile(voc_file);

	if (object_names.empty()) {
		return 0;
	}

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
	cv::namedWindow("Yolo Image", cv::WINDOW_AUTOSIZE | cv::WINDOW_FREERATIO);

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
				ret = ImageHandler(display_scale, display_mode, &image_state, detector_, object_names);

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
		else if (key == 97) {
			// a
			if (image_state.enabled_yolo == 0) {
				image_state.enabled_yolo = 1;
			}
			else {
				image_state.enabled_yolo = 0;
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
	delete detector_;
	detector_ = nullptr;

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

void Get3DPostion(const float b, const float bf, const float dinf, const float angle, cv::Mat& mat_depth, std::vector<bbox_t>& result_vec)
{
	int index = 0;

	for (auto& current_result : result_vec) {

		current_result.x_3d = 0;
		current_result.y_3d = 0;
		current_result.z_3d = -1.0;

		// get rect data
		cv::Rect rect;
		rect.x = std::min(std::max(0, (int)current_result.x), mat_depth.size().width - 1);
		rect.y = std::min(std::max(0, (int)current_result.y), mat_depth.size().height- 1);
		rect.width = std::min(std::max(1, (int)current_result.w), mat_depth.size().width - (int)current_result.x - 1);
		rect.height = std::min(std::max(1, (int)current_result.h), mat_depth.size().height - (int)current_result.y - 1);

		cv::Mat mat_rect(mat_depth, rect);
		cv::Mat mat_rect_dist(mat_rect.size().height, mat_rect.size().width, CV_32F);

		// 距離に変換
		double sum_of_depth = 0;
		double sum_of_distance = 0;
		double count = 0;
		for (int y = 0; y < mat_rect.size().height; y++) {
			float* src = mat_rect.ptr<float>(y);
			float* dst = mat_rect_dist.ptr<float>(y);

			for (int x = 0; x < mat_rect.size().width; x++) {
				if (*src - dinf > 0) {
					sum_of_depth += *src;
					float value = (bf) / (*src - dinf);
					*dst = (float)((int)(value * 100.0F)) / 100.0F;	// unit is cm
					sum_of_distance += *dst;
					count++;
				}
				else {
					*dst = 0;
				}
				src++;
				dst++;
			}
		}

		if ((count == 0) || (sum_of_distance == 0) || (sum_of_depth == 0)) {
			continue;
		}

		/*
			距離の平均値
		*/
		double mean_dist = sum_of_distance / count;

		/*
			平均の10%上下の最頻値を1cm単位で集計する
		*/
		int start = (int)((mean_dist - (mean_dist * 0.1)) * 100.0);
		if (start < 0) start = 0;
		int end = (int)((mean_dist + (mean_dist * 0.1)) * 100.0);
		int range = end - start + 1;

		double* mode_list = new double[range];
		memset(mode_list, 0, sizeof(double) * range);
		for (int y = 0; y < mat_rect_dist.size().height; y++) {
			float* src = mat_rect_dist.ptr<float>(y);

			for (int x = 0; x < mat_rect_dist.size().width; x++) {
				int value_cm = (int)(*src * 100.0);
				if (value_cm >= start && value_cm < end) {
					int pos = value_cm - start;
					if (pos > 0) {
						mode_list[pos]++;
					}
				}
				src++;
			}
		}

		/*
			距離の最頻値
		*/
		double max_value = mode_list[0];
		int max_index = 0;
		for (int i = 0; i < range; i++) {
			if (max_value < mode_list[i]) {
				max_value = mode_list[i];
				max_index = i;
			}
		}
		int mode_value = max_index + start;
		double mode_distance = (double)mode_value / 100.0;

		/*
			視差の平均値
		*/
		double mean = sum_of_depth / count;

		/*
			結果を設定

			上記にて以下を計算している
			①距離の平均
			②距離の最頻値
			③視差の平均

			このサンプルコードでは、②距離の最頻値を採用する
			位置は、矩形の中心とし、位置の計算には、②距離の最頻値より計算する視差を使用する

		*/

		double rect_disparity = 0;
		if (mode_distance != 0) {
			rect_disparity = bf / mode_distance;
		}

		cv::Point2f rect_center = {};
		rect_center.x = float(rect.x + rect.size().width / 2.0);
		rect_center.y = float(rect.y + rect.size().height / 2.0);

		cv::Point2f image_center = {};
		image_center.x = float(mat_depth.size().width / 2.0);
		image_center.y = float(mat_depth.size().height / 2.0);

		if (rect_disparity > 0) {
			current_result.x_3d = (float)(((rect_center.x - image_center.x) * b) / rect_disparity);
			current_result.y_3d = (float)(((image_center.y - rect_center.y) * b) / rect_disparity);
			current_result.z_3d = (float)mode_distance;
		}

		index++;

		delete[] mode_list;

	}

	return;
}

void DrawResultBox(cv::Mat src_image_mat, std::vector<bbox_t> result_list, std::vector<std::string> object_names)
{
	for (auto& current_result : result_list) {
	
		// draw rectangle
		cv::Scalar color = obj_id_to_color(current_result.obj_id);	// in "yolo_v2_class.hpp" 
		cv::rectangle(src_image_mat, cv::Rect(current_result.x, current_result.y, current_result.w, current_result.h), color, 2);

		// draw text
		std::string object_name = (current_result.obj_id >= object_names.size()) ? "unknown object" : object_names[current_result.obj_id];

		char msg_prob[64] = {};
		sprintf_s(msg_prob, ":%.2f", current_result.prob);
		std::string prob_data(msg_prob);
		object_name += prob_data;

		const int font_face = cv::FONT_HERSHEY_PLAIN;	// cv::FONT_HERSHEY_COMPLEX_SMALL;
		const double font_scale = 1.0;
		const double font_scale_3d = 0.8;
		const int thickness = 1;

		cv::Size text_size = cv::getTextSize(object_name, font_face, font_scale, thickness, 0);
		int max_text_width = text_size.width;
		if (text_size.width > (current_result.w - thickness)) {
			max_text_width = current_result.w - thickness;
		}

		std::string string_3d;
		if (current_result.z_3d > 0) {
			char msg_3d[128] = {};
			sprintf_s(msg_3d, "x:%.2fm y:%.2fm z:%.2fm", current_result.x_3d, current_result.y_3d, current_result.z_3d);

			std::string temp_str(msg_3d);
			string_3d = temp_str;

			cv::Size size = cv::getTextSize(string_3d, font_face, font_scale_3d, thickness, 0);
			int max_text_width_3d = size.width;
			if (max_text_width_3d <  (current_result.w + thickness)) {
				max_text_width_3d = current_result.w + thickness;
			}

			if (max_text_width_3d > max_text_width) {
				max_text_width = max_text_width_3d;
			}
		}

		cv::rectangle(	src_image_mat,
						cv::Point2f(std::max((int)current_result.x - 1, 0),
							std::max((int)current_result.y - (text_size.height * 2), 0)),
						cv::Point2f(std::min((int)current_result.x + max_text_width, src_image_mat.size().width - 1),
							std::min((int)current_result.y, src_image_mat.size().height - 1)),
						color,
						CV_FILLED);

		putText(src_image_mat, 
				object_name, 
				cv::Point2f(current_result.x, current_result.y - text_size.height), 
				font_face,
				font_scale,
				cv::Scalar(0, 0, 0), 
				2);

		if (!string_3d.empty()) {
			putText(src_image_mat,
				string_3d,
				cv::Point2f(current_result.x, current_result.y),
				font_face,
				font_scale_3d,
				cv::Scalar(0, 0, 0),
				2);
		}
	}

	return;
}

void ShowResultToConsole(std::vector<bbox_t> result_list, std::vector<std::string> object_names)
{
	for (auto& current_result : result_list) {
		if (current_result.obj_id >= object_names.size()) {
			std::cout << "  unknown object" << " ";
		}
		else {
			std::cout << "  " << object_names[current_result.obj_id] << " ";
		}
		std::cout	<< "obj_id=" << current_result.obj_id 
					<< ",  x = " << current_result.x
					<< ", y = " << current_result.y
					<< ", w = " << current_result.w
					<< ", h = " << current_result.h
					<< std::setprecision(3) << ", prob = " << current_result.prob
					<< std::endl;
	}
}

int ImageHandler(const int display_scale, const int display_mode, ImageState* image_state, Detector* detector_, std::vector<std::string>& obj_names)
{

	const int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;
	
	if (image_state->enabled_yolo == 0) {
		// images from camera
		bool camera_status = image_state->dpl_control->GetCameraData(&image_state->isc_image_Info);

		if ((image_state->isc_image_Info.frame_data[fd_index].p1.width == 0) ||
			(image_state->isc_image_Info.frame_data[fd_index].p1.height == 0)) {

			camera_status = false;
		}

		cv::Mat mat_s0_image_scale_flip;
		if (camera_status) {
			bool is_color_exists = false;
			if (image_state->color_mode == 1) {
				if ((image_state->isc_image_Info.frame_data[fd_index].color.width != 0) &&
					(image_state->isc_image_Info.frame_data[fd_index].color.height != 0)) {

					is_color_exists = true;
				}
			}

			if (is_color_exists) {
				// color image
				cv::Mat mat_s0_image(image_state->isc_image_Info.frame_data[fd_index].color.height, image_state->isc_image_Info.frame_data[fd_index].color.width, CV_8UC3, image_state->isc_image_Info.frame_data[fd_index].color.image);

				double ratio = 1.0 / (double)display_scale;
				cv::Mat mat_s0_image_scale;
				cv::resize(mat_s0_image, mat_s0_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

				cv::flip(mat_s0_image_scale, mat_s0_image_scale_flip, -1);

				cv::imshow("Base Image", mat_s0_image_scale_flip);
			}
			else {
				// base image
				cv::Mat mat_s0_image(image_state->isc_image_Info.frame_data[fd_index].p1.height, image_state->isc_image_Info.frame_data[fd_index].p1.width, CV_8U, image_state->isc_image_Info.frame_data[fd_index].p1.image);

				double ratio = 1.0 / (double)display_scale;
				cv::Mat mat_s0_image_scale;
				cv::resize(mat_s0_image, mat_s0_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

				cv::Mat mat_s0_image_scale_flip_temp;
				cv::flip(mat_s0_image_scale, mat_s0_image_scale_flip_temp, -1);

				cv::Mat mat_s0_image_color;
				cv::cvtColor(mat_s0_image_scale_flip_temp, mat_s0_image_scale_flip, cv::COLOR_GRAY2RGB);

				cv::imshow("Base Image", mat_s0_image_scale_flip);
			}
		}

		// data processing result
		bool data_proc_status = image_state->dpl_control->GetDataProcessingData(&image_state->isc_data_proc_result_data);

		if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width == 0) ||
			(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height == 0)) {

			data_proc_status = false;
		}

		cv::Mat mat_depth_image_scale_flip;
		if (data_proc_status) {
			// depth
			const int width = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width;
			const int height = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height;
			float* depth = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.image;

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
	}
	else {
		bool data_proc_status = image_state->dpl_control->GetDataProcessingData(&image_state->isc_data_proc_result_data);

		if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width == 0) ||
			(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height == 0)) {

			data_proc_status = false;
		}

		cv::Mat mat_data_proc_image_scale_flip;
		bool is_color_exists = false;
		if (image_state->color_mode == 1) {
			if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.width != 0) &&
				(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.height != 0)) {

				is_color_exists = true;
			}
		}

		if (data_proc_status) {
			if (is_color_exists) {
				// color image
				cv::Mat mat_s0_image(	image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.height,
										image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.width,
										CV_8UC3,
										image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].color.image);

				double ratio = 1.0 / (double)display_scale;
				cv::Mat mat_s0_image_scale;
				cv::resize(mat_s0_image, mat_s0_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

				cv::flip(mat_s0_image_scale, mat_data_proc_image_scale_flip, -1);

			}
			else {
				// base image
				cv::Mat mat_s0_image(	image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.height,
										image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.width,
										CV_8U,
										image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].p1.image);

				double ratio = 1.0 / (double)display_scale;
				cv::Mat mat_s0_image_scale;
				cv::resize(mat_s0_image, mat_s0_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

				cv::Mat mat_s0_image_scale_flip_temp;
				cv::flip(mat_s0_image_scale, mat_s0_image_scale_flip_temp, -1);

				cv::Mat mat_s0_image_color;
				cv::cvtColor(mat_s0_image_scale_flip_temp, mat_data_proc_image_scale_flip, cv::COLOR_GRAY2RGB);
			}

			// Yolo
			if (!mat_data_proc_image_scale_flip.empty()) {

				auto start = std::chrono::steady_clock::now();

				auto det_image = detector_->mat_to_image_resize(mat_data_proc_image_scale_flip);

				float thresh = 0.6;
				std::vector<bbox_t> result_vec = detector_->detect_resized(*det_image, mat_data_proc_image_scale_flip.size().width, mat_data_proc_image_scale_flip.size().height, thresh);
				
				auto end = std::chrono::steady_clock::now();
				std::chrono::duration<double> spent = end - start;
				std::cout << "[INFO]Detector time: " << spent.count() << " sec \n";
				ShowResultToConsole(result_vec, obj_names);

				// get 3D position form depth
				const int depth_width = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.width;
				const int depth_height = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.height;
				float* depth = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_index].depth.image;

				cv::Mat mat_depth(depth_height, depth_width, CV_32F, depth);
				cv::Mat mat_depth_flip;
				cv::flip(mat_depth, mat_depth_flip, -1);

				Get3DPostion(image_state->b, image_state->bf, image_state->dinf, image_state->angle, mat_depth_flip, result_vec);
				
				DrawResultBox(mat_data_proc_image_scale_flip, result_vec, obj_names);

				cv::imshow("Yolo Image", mat_data_proc_image_scale_flip);
			}
		}
	}

	return 0;
}
