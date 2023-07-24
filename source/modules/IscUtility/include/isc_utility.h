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
 * @file isc_utils.h
 * @brief Provides auxiliary functions
 */
 
#pragma once

#ifdef ISCUTILITY_EXPORTS
#define ISCUTILITY_EXPORTS_API __declspec(dllexport)
#else
#define ISCUTILITY_EXPORTS_API __declspec(dllimport)
#endif

extern "C" {

    /** @struct  DplIscUtilityParameter
     *  @brief This is the initialize information
     */
    struct DplIscUtilityParameter {
        // camera
        int max_width;                  /**< Maximum width */
        int max_height;                 /**< Maximum height */

        double base_length;             /**< camera base length */
        double d_inf;                   /**< camera parameter */              
        double bf;                      /**< camera parameter */
        double camera_angle;            /**< camera parameter */

        // display
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
    ISCUTILITY_EXPORTS_API int DplIscUtilityInitialize(DplIscUtilityParameter* utility_parameter);

    /** @brief Release resources and process termination.
        @return 0, if successful.
     */
    ISCUTILITY_EXPORTS_API int DplIscUtilityTerminate();

    /** @brief Create in the specified range.
       @return none.
    */
    ISCUTILITY_EXPORTS_API void  DplIscUtilityRebuildDrawColorMap(const double min_distance, const double max_distance);

    /** @brief Converts disparity data to distance image.
       @return none.
    */
    ISCUTILITY_EXPORTS_API int Disparity2DistanceImage(const int width, const int height, float* disparity, unsigned char* bgr_image);

    /** @brief Converts disparity data to images..
       @return none.
    */
    ISCUTILITY_EXPORTS_API int Disparity2Image(const int width, const int height, float* disparity, unsigned char* bgr_image);


} /* extern "C" { */