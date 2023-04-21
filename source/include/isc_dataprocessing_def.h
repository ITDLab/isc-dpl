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
    @file isc_detaprocessing_def.h
    @brief Defines structures that are commonly used in projects.
*/

#ifndef ISC_DATAPROCESSING_DEF_H
#define ISC_DATAPROCESSING_DEF_H

/** @struct  IscDataProcModuleConfiguration
 *  @brief This is the data processing module parameter for Initialize
 */
struct IscDataProcModuleConfiguration {

    wchar_t configuration_file_path[_MAX_PATH]; /**< path for parameter file */
    wchar_t log_file_path[_MAX_PATH];           /**< path for log save */

    int log_level;                              /**< set log level 0:NONE 1:FATAL 2:ERROR 3:WARN 4:INFO 5:DEBUG 6:TRACE */

    IscCameraModel isc_camera_model;            /**< camera model */
    int max_image_width, max_image_height;      /**< maximum image size */

    bool enabled_data_proc_module;              /**< whether to use a data processing library */

};

/** @struct  IscDataProcStartMode
 *  @brief This is the parameter for start process
 */
struct IscDataProcStartMode {
    bool enabled_block_matching;                /**< whether to use a soft stereo matching */
    bool enabled_frame_decoder;                 /**< whether to use a frame decoder */
};

/** @struct  IscDataProcModuleParameter
 *  @brief This is the data processing module parameter
 */
struct IscDataProcModuleParameter {

    struct ParameterSet {
        int value_type;         /**< 0:int 1:float 2:double */
        int value_int;          /**< value for interger */
        float value_float;      /**< value for float */
        double value_double;    /**< value for double */

        wchar_t category[32];       /**< category name */
        wchar_t name[16];           /**< parameter name */
        wchar_t description[32];    /**< Parameter description */
    };

    int module_index;                   /**< index 0~ */
    wchar_t module_name[32];            /**< module name */

    int parameter_count;                /**< number of parameterse */
    ParameterSet parameter_set[32];     /**< parameters */

};

/** @struct  IscDataProcStatus
 *  @brief This is the status of data proccesing
 */
struct IscDataProcStatus {
    int error_code;             /**< error code */
    double proc_tact_time;      /**< cycle of data processing */
};

/** @struct  IscDataProcModuleStatus
 *  @brief This is the status of each data proccesing module
 */
struct IscDataProcModuleStatus {
    char module_names[32];      /**< module name */
    int error_code;             /**< error code */
    double processing_time;     /**< processing time */
};

/** @struct  IscDataProcessResultData
 *  @brief This is the data processing module result
 */
struct IscDataProcResultData {

    int number_of_modules_processed;            /**< Number of modules processed */
    int maximum_number_of_modules;              /**< Maximum number of modules = 4 */
    int maximum_number_of_modulename;           /**< Maximum number of module name = 32 */

    IscDataProcStatus status;                   /**< status of data proccesing */
    IscDataProcModuleStatus module_status[4];   /**< IscDataProcModuleStatus */

    IscImageInfo isc_image_info;                /**< result processed by the module */
};

/** @struct  IscBlockDisparityData
 *  @brief This is the result of BlockMatching
 */
struct IscBlockDisparityData {
    int image_width;    /**< 基準画像幅 */
    int image_height;   /**< 基準画像高さ */

    int pblkhgt;        /**< 視差ブロック高さ */
    int pblkwdt;        /**< 視差ブロック幅 */
    int pmtchgt;        /**< マッチングブロック高さ */
    int pmtcwdt;        /**< マッチングブロック幅 */
    int pblkofsx;       /**< 視差ブロック横オフセット */
    int pblkofsy;       /**< 視差ブロック縦オフセット */
    int pdepth;         /**< マッチング探索幅 */
    int pshdwdt;        /**< 画像遮蔽幅 */
    float* pblkdsp;     /**< 視差ブロック視差値 */
    int* pblkval;       /**< 視差ブロック視差値(1000倍サブピクセル精度の整数) */
    int* pblkcrst;      /**< マッチングブロックコントラスト */
};

#endif /* ISC_DATAPROCESSING_DEF_H */
