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
 * @file isc_util_draw.h
 * @brief Provides auxiliary functions for display
 */
 
#pragma once

/**
 * @class   IscUtilDraw
 * @brief   draw function class
 * this class provides auxiliary functions for display
 */
class IscUtilDraw {
public:
    IscUtilDraw();
    ~IscUtilDraw();

    struct Cameraparameter {
        int max_width;                  /**< Maximum width */
        int max_height;                 /**< Maximum height */

        double base_length;             /**< camera base length */
        double d_inf;                   /**< camera parameter */
        double bf;                      /**< camera parameter */
        double camera_angle;            /**< camera parameter */
    };

    struct DrawParameter {
        bool draw_outside_bounds;       /**< Whether to draw outside the specified range */

        double min_distance;            /**< Minimum display distance */
        double max_distance;            /**< Maximum display distance */
        double step_distance;           /**< Resolution of display distance */
        
        double min_disparity;           /**< Minimum parallax */
        double max_disparity;           /**< Maximum parallax */
        double step_disparity;          /**< Resolution of display */
    };

    /** @brief Initializes the class . Must be called at least once before streaming is started.
        @return 0, if successful.
     */
    int Initialize(Cameraparameter* camera_parameter, DrawParameter* draw_parameter);

    /** @brief Release resources and process termination.
       @return 0, if successful.
    */
    int Terminate();

    /** @brief Create in the specified range.
      @return none.
    */
    void RebuildDrawColorMap(const double min_distance, const double max_distance);

    /** @brief Converts disparity data to distance image.
      @return none.
    */
    int Disparity2DistanceImage(const int width, const int height, float* disparity, unsigned char* bgr_image);

    /** @brief Converts disparity data to images..
       @return none.
    */
    int Disparity2Image(const int width, const int height, float* disparity, unsigned char* bgr_image);

private:
    // parameter
    Cameraparameter camera_parameter_;
    DrawParameter draw_parameter_;

	// Disparity Color Map
	struct DispColorMap {
		double min_value;						/**<  視差表示 最小 */
		double max_value;						/**<  視差表示 最大 */

		int color_map_size;						/**<  視差表示用 LUT Size*/
		int* color_map;							/**<  視差表示用 LUT */
		double color_map_step;					/**<  start ~end までの分解能をいくつにするか */
	};
	DispColorMap disp_color_map_distance_;		/**<  視差表示用 LUT 距離基準 */
	DispColorMap disp_color_map_disparity_;		/**<  視差表示用 LUT 視差基準 */

    // functions
	int BuildColorHeatMap(DispColorMap* disp_color_map);

	int BuildColorHeatMapForDisparity(DispColorMap* disp_color_map);

	int ColorScaleBCGYR(const double min_value, const double max_value, const double in_value, int* bo, int* go, int* ro);

	bool MakeDepthColorImage(const bool is_color_by_distance, const bool is_draw_outside_bounds, const double min_length_i, const double max_length_i,
		DispColorMap* disp_color_map, double b_i, const double angle_i, const double bf_i, const double dinf_i,
		const int width, const int height, float* depth, unsigned char* bgr_image);

};