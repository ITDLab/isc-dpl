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
    @file isc_camera_def.h
    @brief Defines structures that are commonly used in projects.
*/

#ifndef ISC_CEMERA_DEF_H
#define ISC_CEMERA_DEF_H

/** @enum  IscCameraModel
 *  @brief This is a camera model name parameter
 */
enum class IscCameraModel {
    kVM,                /**< VM */
    kXC,                /**< XC */
    k4K,                /**< 4K original */
    k4KA,               /**< 4K for customer */
    k4KJ,               /**< 4K for customer */
    kUnknown            /**< error type */
};

/** @struct  IsCameraControlConfiguration
 *  @brief This is the configuration information
 */
struct IscCameraControlConfiguration {
    wchar_t configuration_file_path[_MAX_PATH]; /**< configuration file path */
    wchar_t log_file_path[_MAX_PATH];           /**< log file path */

    int log_level;                              /**< set log level 0:NONE 1:FATAL 2:ERROR 3:WARN 4:INFO 5:DEBUG 6:TRACE */

    bool enabled_camera;                        /**< decide whether to use a physical camera */
    IscCameraModel isc_camera_model;            /**< physical camera model */
    wchar_t save_image_path[_MAX_PATH];         /**< the path to save the image */
    wchar_t load_image_path[_MAX_PATH];         /**< image loading path */
    int minimum_write_interval_time;            /**< minimum free time to write (msec) */
};

/** @struct  IscSaveDataConfiguration
 *  @brief This is the configuration information
 */
constexpr int kISC_SAVE_MAX_SAVE_FOLDER_COUNT = 16;
struct IscSaveDataConfiguration {
	int max_save_folder_count;				/**< 保存先 */
	int save_folder_count;
	wchar_t save_folders[kISC_SAVE_MAX_SAVE_FOLDER_COUNT][_MAX_PATH];

	__int64 minimum_capacity_required;		/**< 最小ディスク空き容量 */

	int save_time_for_one_file;				/**< 保存ファイルの1個あたりの時間 (分) */

    int max_buffer_count;                   /**< 内部バッファーの数 */

    int minimum_write_interval_time;        /**< 書き込みの最小空き時間 (msec) */
};


/** @enum  IscCameraInfo
 *  @brief This is a camera dependent parameter 
 */
enum class IscCameraInfo {
    kBF,                /**< BF */
    kDINF,              /**< D_INF */
    kDz,                /**< dz */
    kBaseLength,        /**< Base length(m) */
    kViewAngle,         /**< Stereo horiozntal angle(deg) */
    kProductID,         /**< Product ID */
    kSerialNumber,      /**< Serial Number */
    kFpgaVersion,       /**< FPGA Version majour-minor */
    kWidthMax,          /**< max width */
    kHeightMax          /**< max height */
};

/** @enum  IscCameraParameter
 *  @brief This is a camera control parameter 
 */
enum class IscCameraParameter {
    kMonoS0Image,               /**< [for implementation inquiries] Providing monochrome sensor-0 image data */
    kMonoS1Image,               /**< [for implementation inquiries] Providing monochrome sensor-1 image data */
    kDepthData,                 /**< [for implementation inquiries] Providing depth data */
    kColorImage,                /**< [for implementation inquiries] Providing color data */
    kColorImageCorrect,         /**< [for implementation inquiries] Providing collected color data */
    kAlternatelyColorImage,     /**< [for implementation inquiries] Providing color data in alternating mode */
    kBayerColorImage,           /**< [for implementation inquiries] Providing color data in bayer mode */
    kShutterMode,               /**< [IscShutterMode] Shutter mode */
    kManualShutter,             /**< [for implementation inquiries] Providing manual shutter funtion */
    kSingleShutter,             /**< [for implementation inquiries] Providing single shutter funtion */
    kDoubleShutter,             /**< [for implementation inquiries] Providing double shutter funtion */
    kDoubleShutter2,            /**< [for implementation inquiries] Providing double shutter funtion */
    kExposure,                  /**< [int] Exposure setting */
    kFineExposure,              /**< [int] Exposure for fine tune setting */
    kGain,                      /**< [int] Gain setting */
    kHrMode,                    /**< [bool] High Resolution setting */
    kHdrMode,                   /**< [bool] Sensor HDR mode setting */
    kAutoCalibration,           /**< [bool] Automatic calibration valid */
    kManualCalibration,         /**< [bool] Automatic Calibration forced execution */
    kOcclusionRemoval,          /**< [int] Sets the occlusion removal value */
    kPeculiarRemoval,           /**< [bool] Settings to remove peculiarity */
    kSelfCalibration,           /**< [bool] Software Calibration(selft calibration) valid */
    kGenericRead,               /**< [uc*, uc*, int,int ] General purpose loading */
    kGenericWrite               /**< [uc*, int ] General purpose writing */
};

/** @struct  IscCameraDisparityParameter
 *  @brief This is a camera-specific parameters 
 */
struct IscCameraSpecificParameter {
    float d_inf;        /**< d inf */
    float bf;		    /**< bf */					
    float base_length;	/**< base length(m) */				
    float dz;	        /**< delta z */						
};

/** @enum  IscShutterMode
 *  @brief This is a shutter control mode 
 */
enum class IscShutterMode {
    kManualShutter = 0,     /**< Manual mode */
    kSingleShutter,         /**< Single shutter mode */
    kDoubleShutter,         /**< Double shutter mode */
    kDoubleShutter2,        /**< Double shutter mode(Alt) */
};

/** @enum  IscGrabMode
 *  @brief This is a camera grab request mode 
 */
enum class IscGrabMode {
    kParallax = 1,          /**< 視差モード(補正後画像+視差画像) */
    kCorrect,               /**< 補正後画像モード */
    kBeforeCorrect,         /**< 補正前画像モード(原画像) */
    kBayerS0,               /**< 補正前Bayer画像モード(原画像)(Sensor-0 Camera) */
    kBayerS1                /**< 補正前Bayer画像モード(原画像)(Sensor-1 Camera) */
};

/** @enum  IscGrabColorMode
 *  @brief This is a color mode on/off 
 */
enum class IscGrabColorMode {
    kColorOFF = 0,          /**< mode off */
    kColorON                /**< mode on */
};

/** @struct  IscGetMode
 *  @brief This is the request to get image
 */
struct IscGetMode {
    int wait_time;                  /**< the time-out interval, in milliseconds */
};

/** @enum  IscGetModeColor
 *  @brief This is the request to get the color image
 */
enum class IscGetModeColor {
    kBGR,               /**< yuv(bayer) -> bgr */
    kCorrect,           /**< yuv(bayer) -> bgr -> correct */
    kAwb,               /**< yuv(bayer) -> bgr -> correct -> auto white balance */
    kAwbNoCorrect       /**< yuv(bayer) -> bgr -> auto white balance */
};

/** @enum  IscGetModeRaw
 *  @brief This is the request to get the image
 */
enum class IscGetModeRaw {
    kRawOff = 0,        /**< mode off */
    kRawOn              /**< mode on */
};

/** @enum  IscRecordMode
 *  @brief This is the request to save the image
 */
enum class IscRecordMode {
    kRecordOff = 0,        /**< mode off */
    kRecordOn              /**< mode on */
};

/** @enum  IscPlayMode
 *  @brief This is the request to play the image from file
 */
enum class IscPlayMode {
    kPlayOff = 0,        /**< mode off */
    kPlayOn              /**< mode on */
};

/** @struct  IscPalyModeParameter
 *  @brief This is the parameter for play image
 */
struct IscPalyModeParameter {           
    int interval;                       /**< intervaltime for read one frame data */
    wchar_t play_file_name[_MAX_PATH];  /**< file name for play iamge */
};

/** @struct  IscGrabStartMode
 *  @brief This is the request to grab
 */
struct IscGrabStartMode {
    IscGrabMode isc_grab_mode;              /**< grab mode request */
    IscGrabColorMode isc_grab_color_mode;   /**< color mode request */

    IscGetMode isc_get_mode;                /**< get parameter */
    IscGetModeRaw isc_get_raw_mode;         /**< raw mode on/off */
    IscGetModeColor isc_get_color_mode;     /**< color mode on/off */

    IscRecordMode isc_record_mode;          /**< save data when grabbing */
    IscPlayMode isc_play_mode;              /**< read from file instead of camera */
    IscPalyModeParameter isc_play_mode_parameter;   /**< This is the parameter for play image */
};

/** @struct  IscCameraStatus
 *  @brief This represents the state of the camera
 */
struct IscCameraStatus {
    unsigned int error_code;                /**< error code from sdk */
    double data_receive_tact_time;          /**< cycle of data received from SDK */
};

constexpr int kISCIMAGEINFO_FRAMEDATA_MAX_COUNT = 3;    /**< max FrameData count */
constexpr int kISCIMAGEINFO_FRAMEDATA_LATEST = 0;       /**< latest FrameData */
constexpr int kISCIMAGEINFO_FRAMEDATA_PREVIOUS = 1;     /**< previous FrameData */
constexpr int kISCIMAGEINFO_FRAMEDATA_MERGED = 2;       /**< merge for double shutter FrameData */

/** @struct  IscImageInfo
 *  @brief This is the structure to get image
 */
struct IscImageInfo {
    /*! A Image struct */
    struct ImageType {
        int width;              /**< width */
        int height;             /**< height */
        int channel_count;      /**< number of channels */
        unsigned char* image;   /**< data */
    };
    
    /*! A Depth struct */
    struct DepthType {
        int width;              /**< width */
        int height;             /**< height */
        float* image;           /**< data */
    };

    /*! A Frame struct */
    struct FrameData {
        IscCameraStatus camera_status;      /**< カメラの状態 */

        __int64 frame_time;                 /**< UNIX UTC Time (msec) */

        int frameNo;                        /**< フレームの番号 */
        int gain;                           /**< フレームのGain値 */
        int exposure;                       /**< フレームのExposure値 */

        ImageType p1;                       /**< 基準側画像 */
        ImageType p2;                       /**< 補正後比較画像/補正前比較画像 */
        ImageType color;                    /**< カラー基準画像/カラー比較画像 */
        DepthType depth;                    /**< 視差 */
        ImageType raw;                      /**< Camera RAW (展開以前のカメラデータ） */
        ImageType raw_color;                /**< Camera RAW Color(展開以前のカメラデータ） */
    };

    IscCameraSpecificParameter camera_specific_parameter;   /**< カメラ固有のパラメータ */

    IscGrabMode grab;                       /**< 取り込みモード */
    IscGrabColorMode color_grab_mode;       /**< カラーモード */
    IscShutterMode shutter_mode;            /**< 露光調整モード */

    FrameData frame_data[kISCIMAGEINFO_FRAMEDATA_MAX_COUNT];    /**< Frameデータ単位 0:Latest 1:Previous 2:Merged for Double-Shutter */
};

// RAW data file save format
//
// |-----------------------------------|
// |    FILE HEADER(128byte)           |
// |-----------------------------------|
// |    DATA HEADER(64byte)            |
// |-----------------------------------|
// |    DATA                           |
// |-----------------------------------|
// |    DATA HEADER(64byte)            |
// |-----------------------------------|
// |    DATA                           |
// |-----------------------------------|
//

// FILE HEADER
/*
        型	サイズ(byte)	データ		    内容
    0	CHAR	32			MARK		    "ISC VM/XC/... RAW DATA"
    1	INT		4			VERSION		    ヘッダーのバージョン
    2	INT		4			HEADER SIZE	    ヘッダーのサイズ（Byte)
    4   INT     4           CAMERA MODEL    Camera 0:VM 1:XC 2:4K 3:4KA 4:4KJ
    5   INT     4           WIDTH           最大画像幅
    6   INT     4           HEIGHT          最大画像高さ
    3	FLOAT	4			D_INF		    D_INF
    7   FLOAT	4			BF			    BF
    8   FLOAT	4			Dz			    Dz
    9   FLOAT	4			BASE LENGTH	    BASE Length
    10	INT		4			GRAB MODE	
                                            0x01 Pallax
                                            0x02 Correct Image
                                            0x03 Befor Correct Image
                                            0x04 Bayer Image
    11	INT		4			SHUTTER MODE
                                            0x00 Normal
                                            0x01 Single
                                            0x02 Double
    12  INT     4           COLOR MODE      Color有効 0:off 1:on
    13	INT		4*12		RESERVE(0)

                128
*/

constexpr int ISC_ROW_FILE_HEADER_VERSION = 200;    /**< Header Version 2.0.0 */ 

/** @struct  IscRawFileHeader
 *  @brief This is the structure to file
 */
struct IscRawFileHeader {
    char	mark[32];       /**< MARK */
    int		version;        /**< Header version */
    int		header_size;    /**< Header size */
    int     camera_model;   /**< model  0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown */
    int     max_width;      /**< maximum width */
    int     max_height;     /**< maximum height */
    float	d_inf;          /**< D_INF */
    float	bf;             /**< BF */
    float	dz;             /**< Dz */
    float	base_length;    /**< Base Length(m) */
    int		grab_mode;      /**< Grab mode */
    int		shutter_mode;   /**< Shutter control mode */
    int     color_mode;     /**< color mode on/off 0:off 1:on*/
    int		reserve[12];    /**< Reserve */
};

// DATA HEADER
/*
        型	サイズ(byte)	データ		        内容
    0	INT		4			VERSION		        ヘッダーのバージョン
    1	INT		4			HEADER SIZE	        ヘッダーのサイズ（Byte)
    2	INT		4			DATA SIZE	        データサイズ
    3   INT     4           DATA COMPRESSED     データは圧縮されている 0:none 1:compressed
    4	INT		4			FRAME_INDEX         Frame番号
    5	INT		4			TYPE		
                                                0x01 Mono
                                                0x02 Color
    6	INT		4			STATUS		        AutoCalib　Staus
    7	INT		4			ERROR CODE	        エラーコード
    8	INT		4			EXPOSURE	        Exposure値
    9	INT		4			GAIN		        Gain値
    10	INT		4*6			RESERVE(0)

                64
*/

constexpr int ISC_ROW_DATA_HEADER_VERSION = 300;     /**< Header Version 2.0.0 */  

/*
    200 -> 300 rserve[0/1] frame_time(64bit)　に変更

*/

/** @struct  IscRawDataHeader
 *  @brief This is the structure to file
 */
struct IscRawDataHeader {
    int		version;            /**< Header version */
    int		header_size;        /**< Header size */
    int		data_size;          /**< Data size */
    int     compressed;         /**< Data is compressed 0:none 1:compressed */
    int		frame_index;        /**< Frame index */
    int		type;               /**< Type 1:mono 2:color*/
    int		status;             /**< AutoCalib　Staus */
    int		error_code;         /**< Error code */
    int		exposure;           /**< Exposure value */
    int		gain;               /**< Gain Value */
    int     frame_time_low;     /**< frame time (UTC msec) lower part */
    int     frame_time_high;    /**< frame time (UTC msec) high part */
    int		reserve[4];         /**< Reserve */
};

#endif /* ISC_CEMERA_DEF_H */
