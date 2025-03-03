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

class DplControl
{
public:
	DplControl();
	~DplControl();

	bool Initialize(const wchar_t* module_path, const int camera_model);
	void Terminate();

	bool InitializeBuffers(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data);
	bool ReleaseBuffers(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data);

	struct StartMode {
		bool enabled_color;
		int show_mode;
	};

	bool Start(StartMode& start_mode);
	bool StartPlayFile(StartMode& start_mode, wchar_t* play_file_name);
	bool Stop();

	bool GetCameraData(IscImageInfo* isc_image_Info);
	bool GetDataProcessingData(IscDataProcResultData* isc_data_proc_result_data);

	bool GetCameraParameter(float* b, float* bf, float* dinf, int* width, int* height);

	bool ConvertDisparityToImage(double b, const double angle, const double bf, const double dinf,
									const int width, const int height, float* depth, unsigned char* bgra_image);

	bool GetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header, IscPlayFileInformation* play_file_information);
	

private:
	StartMode start_mode_;

    wchar_t configuration_file_path_[_MAX_PATH];
    wchar_t log_file_path_[_MAX_PATH];
    wchar_t image_path_[_MAX_PATH];

	int camera_model_;

	IscImageInfo isc_image_info_;
	IscDataProcResultData isc_data_proc_result_data_;

	struct CameraParameter {
		float b;
		float bf;
		float dinf;
		float setup_angle;
	};

	CameraParameter camera_parameter_;

	IscDplConfiguration isc_dpl_configuration_;
	ns_isc_dpl::IscDpl* isc_dpl_;

	IscStartMode isc_start_mode_;

	// Disparity Color Map
	struct DispColorMap {
		double min_value;						
		double max_value;						

		int color_map_size;						
		int* color_map;							
		double color_map_step;					
	};
	DispColorMap disp_color_map_distance_;		
	DispColorMap disp_color_map_disparity_;		
	double max_disparity_;						

	int BuildColorHeatMap(DispColorMap* disp_color_map);
	int BuildColorHeatMapForDisparity(DispColorMap* disp_color_map);

	int ColorScaleBCGYR(const double min_value, const double max_value, const double in_value, int* bo, int* go, int* ro);

	bool MakeDepthColorImage(const bool is_color_by_distance, const bool is_draw_outside_bounds, const double min_length_i, const double max_length_i,
		DispColorMap* disp_color_map, double b_i, const double angle_i, const double bf_i, const double dinf_i,
		const int width, const int height, float* depth, unsigned char* bgra_image);

};

