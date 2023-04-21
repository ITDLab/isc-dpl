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
    @file isc_dpl_def.h
    @brief Defines structures that are commonly used in projects.
*/

#ifndef ISC_DPL_DEF_H
#define ISC_DPL_DEF_H

#include "isc_camera_def.h"
#include "isc_dataprocessing_def.h"


/** @struct  IscDplConfiguration
 *  @brief This is the configuration information 
 */
struct IscDplConfiguration {
    wchar_t configuration_file_path[_MAX_PATH]; /**< configuration file path */
    wchar_t log_file_path[_MAX_PATH];           /**< log file path */

    int log_level;                              /**< set log level 0:NONE 1:FATAL 2:ERROR 3:WARN 4:INFO 5:DEBUG 6:TRACE */

    bool enabled_camera;                        /**< whether to use a physical camera */
    IscCameraModel isc_camera_model;            /**< physical camera model */
    wchar_t save_image_path[_MAX_PATH];         /**< the path to save the image */
    wchar_t load_image_path[_MAX_PATH];         /**< image loading path */

    bool enabled_data_proc_module;              /**< whether to use a data processing library */
};

/** @struct  IscStartMode
 *  @brief This is the for start
 */
struct IscStartMode {
    IscGrabStartMode isc_grab_start_mode;
    IscDataProcStartMode isc_dataproc_start_mode;
};

/** @struct  IscAreaDataStatistics
 *  @brief This is the data statistics information
 */
struct IscAreaDataStatistics {
    struct Statistics {
        float max_value;                /**< maximum value */
        float min_value;                /**< minimum value */
        float std_dev;                  /**< standard deviation */
        float average;                  /**< average value */
        float median;                   /**< median value */
    };

    struct Roi3D {
        float width;                    /**<  width of region of area(m) */
        float height;                   /**<  height of region of area(m) */
        float distance;                 /**<  distance of region of area(m) */
    };

    int x;                              /**< top left of region */
    int y;                              /**< top left of region */
    int width;                          /**< width of region */
    int height;                         /**< height of region */

    Statistics statistics_depth;        /**< parallax stats */
    Statistics statistics_distance;     /**< distance statistics */
    Roi3D roi_3d;                       /**< distance of 3D */
};

#endif /* ISC_DPL_DEF_H */
