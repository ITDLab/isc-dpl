#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
from ctypes import *
from ctypes.util import find_library
from enum import Enum
from enum import IntEnum
import numpy as np
import cv2

# @enum  IscCameraModel
# @brief This is a camera model name parameter
class IscCameraModel(IntEnum) :
    kVM = 0
    kXC = 1
    k4K = 2
    k4KA = 3
    k4KJ = 4
    kUnknown = 5

# /** @enum  IscGrabMode
#  *  @brief This is a camera grab request mode 
#  */
class IscGrabMode(IntEnum) :
    kParallax = 1       #/**< 視差モード(補正後画像+視差画像) */
    kCorrect = 2        #/**< 補正後画像モード */
    kBeforeCorrect = 3  #/**< 補正前画像モード(原画像) */
    kBayerS0 = 4        #/**< 補正前Bayer画像モード(原画像) */
    kBayerS1 = 5        #/**< 補正前Bayer画像モード(原画像)(Compare Camera) */

# /** @enum  IscGrabColorMode
#  *  @brief This is a color mode on/off 
#  */
class IscGrabColorMode(IntEnum) :
    kColorOFF = 0       #/**< mode off */
    kColorON  = 1       #/**< mode on */

# /** @enum  IscGetModeRaw
#  *  @brief This is the request to get the image
#  */
class IscGetModeRaw(IntEnum) :
    kRawOff = 0     #/**< mode off */
    kRawOn = 1      #/**< mode on */

# /** @enum  IscGetModeColor
#  *  @brief This is the request to get the color image
#  */
class IscGetModeColor(IntEnum) :
    kBGR = 0            #/**< yuv(bayer) -> bgr */
    kCorrect = 1        #/**< yuv(bayer) -> bgr -> correct */
    kAwb = 2            #/**< yuv(bayer) -> bgr -> correct -> auto white balance */
    kAwbNoCorrect = 3   #/**< yuv(bayer) -> bgr -> auto white balance */

# /** @enum  IscRecordMode
#  *  @brief This is the request to save the image
#  */
class IscRecordMode(IntEnum) :
    kRecordOff = 0      #/**< mode off */
    kRecordOn = 1       #/**< mode on */


# /** @enum  IscPlayMode
#  *  @brief This is the request to play the image from file
#  */
class IscPlayMode(IntEnum) :
    kPlayOff = 0        #/**< mode off */
    kPlayOn = 1         #/**< mode on */

# /** @enum  IscCameraInfo
#  *  @brief This is a camera dependent parameter 
#  */
class IscCameraInfo(IntEnum) :
    kBF=0               #/**< BF */
    kDINF=1             #/**< D_INF */
    kDz=2               #/**< dz */
    kBaseLength=3       #/**< Base length(m) */
    kViewAngl=4         #/**< Stereo horiozntal angle(deg) */
    kProductID=5        #/**< Product ID */
    kSerialNumber=6     #/**< Serial Number */
    kFpgaVersion=7      #/**< FPGA Version majour-minor */
    kWidthMax=8         #/**< max width */
    kHeightMax=9        #/**< max height */

# /** @enum  IscCameraParameter
#  *  @brief This is a camera control parameter 
#  */
class IscCameraParameter(IntEnum) :
    kMonoS0Image=0                #/**< [for implementation inquiries] Providing base image data */
    kMonoS1Image=1             #/**< [for implementation inquiries] Providing compare image data */
    kDepthData=2                #/**< [for implementation inquiries] Providing depth data */
    kColorImage=3               #/**< [for implementation inquiries] Providing color data */
    kColorImageCorrect=4        #/**< [for implementation inquiries] Providing collected color data */
    kAlternatelyColorImage=5    #/**< [for implementation inquiries] Providing color data in alternating mode */
    kBayerColorImage=6          #/**< [for implementation inquiries] Providing color data in bayer mode */
    kShutterMode=7              #/**< [IscShutterMode] Shutter mode */
    kManualShutter=8            #/**< [for implementation inquiries] Providing manual shutter funtion */
    kSingleShutter=9            #/**< [for implementation inquiries] Providing single shutter funtion */
    kDoubleShutter=10           #/**< [for implementation inquiries] Providing double shutter funtion */
    kDoubleShutter2=11          #/**< [for implementation inquiries] Providing double shutter funtion */
    kExposure=12                #/**< [int] Exposure setting */
    kFineExposure=13            #/**< [int] Exposure for fine tune setting */
    kGain=14                    #/**< [int] Gain setting */
    kHrMode=15                  #/**< [bool] High Resolution setting */
    kHdrMode=16                 #/**< [bool] Sensor HDR mode setting */
    kAutoCalibration=17         #/**< [bool] Automatic adjustment valid */
    kManualCalibration=18       #/**< [bool] Automatic adjustment forced execution */
    kOcclusionRemoval=19        #/**< [int] Sets the occlusion removal value */
    kPeculiarRemoval=20         #/**< [bool] Settings to remove peculiarity */
    kSelfCalibration=21         #/**< [bool] Software Calibration(selft calibration) valid */
    kGenericRead=22             #/**< [uc*, uc*, int,int ] General purpose loading */
    kGenericWrite=23            #/**< [uc*, int ] General purpose writing */

# /** @enum  IscShutterMode
#  *  @brief This is a shutter control mode 
#  */
class IscShutterMode(IntEnum) :
    kManualShutter=0    #/**< Manual mode */
    kSingleShutter=1    #/**< Single shutter mode */
    kDoubleShutter=2    #/**< Double shutter mode */
    kDoubleShutter2=3   #/**< Double shutter mode(Alt) */

# /** @struct  IscDplConfiguration
#  *  @brief This is the configuration information 
#  */
class IscDplConfiguration (Structure):
    _fields_ = [
                ("configuration_file_path", c_wchar * 260), #/**< configuration file path */
                ("log_file_path", c_wchar * 260),           #/**< log file path */
                ("log_level", c_int),                       #/**< set log level 0:NONE 1:FATAL 2:ERROR 3:WARN 4:INFO 5:DEBUG 6:TRACE */
                ("enabled_camera", c_bool),                 #/**< whether to use a physical camera */
                ("isc_camera_model", c_int),                #/**< physical camera model */
                ("save_image_path", c_wchar * 260),         #/**< the path to save the image */
                ("load_image_path", c_wchar * 260),         #/**< image loading path */
                ("enabled_data_proc_module", c_bool),       #/**< whether to use a data processing library */
    ]

# @struct  IsCameraControlConfiguration
# @brief This is the configuration information
class IscCameraControlConfiguration (Structure):
    _fields_ = [
                ("configuration_file_path", c_wchar * 260) ,
                ("log_file_path", c_wchar * 260),
                ("log_level", c_int),
                ("enabled_camera", c_bool),
                ("isc_camera_model", c_int),
                ("save_image_path", c_wchar * 260),
                ("load_image_path", c_wchar * 260)
    ]

# /** @struct  IscCameraDisparityParameter
# *  @brief This is a camera-specific parameters 
# */
class IscCameraSpecificParameter (Structure):
    _fields_ = [
                ("d_inf", c_float),         # float d_inf;        /**< d inf */
                ("bf", c_float),            # float bf;		    /**< bf */					
                ("base_length", c_float),   # float base_length;	/**< base length(m) */				
                ("dz", c_float),            # float dz;	        /**< delta z */						
    ]

# /** @struct  IscCameraStatus
# *  @brief This represents the state of the camera
# */
class IscCameraStatus (Structure):
    _fields_ = [
                ("error_code", c_uint),                 # unsigned int error_code;                /**< error code from sdk */
                ("data_receive_tact_time", c_double)    # double data_receive_tact_time;          /**< cycle of data received from SDK */
    ]

# A Image struct */
class ImageType (Structure):
    _fields_ = [
                ("width", c_int),               # int width;              # /**< width */
                ("height", c_int),              # int height;             # /**< height */
                ("channel_count", c_int),       # int channel_count;      # /**< number of channels */
                ("image", POINTER(c_ubyte))     # unsigned char* image;   # /**< data */
    ]

# A Depth struct */
class DepthType (Structure):
    _fields_ = [
                ("width", c_int),               # int width;              # /**< width */
                ("height", c_int),              # int height;             # /**< height */
                ("image", POINTER(c_float))     # float* image;   # /**< data */
    ]

#/*! A Frame struct */
class FrameData (Structure):
    _fields_ = [
                ("camera_status", IscCameraStatus), #/**< カメラの状態 */

                ("frameNo", c_int),                 #/**< フレームの番号 */
                ("gain", c_int),                    #/**< フレームのGain値 */
                ("exposure", c_int),                #/**< フレームのExposure値 */

                ("p1", ImageType),                  #/**< 基準側画像 */
                ("p2", ImageType),                  #/**< 補正後比較画像/補正前比較画像 */
                ("color", ImageType),               #/**< カラー基準画像/カラー比較画像 */
                ("depth", DepthType),               #/**< 視差 */
                ("raw", ImageType),                 #/**< Camera RAW (展開以前のカメラデータ） */
                ("raw_color", ImageType),           #/**< Camera RAW Color(展開以前のカメラデータ） */
    ]

class IscImageInfo (Structure):
    _fields_ = [
                ("camera_specific_parameter", IscCameraSpecificParameter),  #/**< カメラ固有のパラメータ */
                ("grab", c_int),                                            #/**< 取り込みモード */
                ("color_grab_mode", c_int),                                 #/**< カラーモード */
                ("shutter_mode", c_int),                                    #/**< 露光調整モード */
                ("frame_data", FrameData * 3)                               #/**< Frameデータ単位 0:Latest 1:Previous 2:Merged for Double-Shutter */
    ]

# /** @struct  IscRawFileHeader
#  *  @brief This is the structure to file
#  */
class IscRawFileHeader (Structure):
    _fields_ = [
                ("mark", c_char * 32) ,     #/**< MARK */
                ("version", c_int) ,        #/**< Header version */
                ("header_size", c_int) ,    #/**< Header size */
                ("camera_model", c_int),    #/**< model  0:VM 1:XC 2:4K 3:4KA 4:4KJ 99:unknown */
                ("max_width", c_int) ,      #/**< maximum width */
                ("max_height", c_int) ,     #/**< maximum height */
                ("d_inf", c_float) ,        #/**< D_INF */
                ("bf", c_float) ,           #/**< BF */
                ("dz", c_float),            #/**< Dz */
                ("base_length", c_float),   #/**< Base Length(m) */
                ("grab_mode", c_int),       #/**< Grab mode */
                ("shutter_mode", c_int),    #/**< Shutter control mode */
                ("color_mode", c_int) ,     #/**< color mode on/off 0:off 1:on*/
                ("reserve", c_int * 12)     #/**< Reserve */
    ]

# /** @struct  IscGetMode
# *  @brief This is the request to get image
# */
class IscGetMode (Structure):
    _fields_ = [
                ("wait_time", c_int)    # int wait_time;    /**< the time-out interval, in milliseconds */
    ]

# /** @struct  IscPalyModeParameter
# *  @brief This is the parameter for play image
# */
class IscPalyModeParameter (Structure):         
    _fields_ = [
                ("interval", c_int),                # int interval;                       /**< intervaltime for read one frame data */
                ("play_file_name", c_wchar * 260)   # wchar_t play_file_name[_MAX_PATH];  /**< file name for play iamge */
    ]

# /** @struct  IscGrabStartMode
# *  @brief This is the request to grab
# */
class IscGrabStartMode (Structure):
    _fields_ = [
                ("isc_grab_mode", c_int),                           # IscGrabMode isc_grab_mode;              /**< grab mode request */
                ("isc_grab_color_mode", c_int),                     # IscGrabColorMode isc_grab_color_mode;   /**< color mode request */

                ("isc_get_mode", IscGetMode),                       # IscGetMode isc_get_mode;                /**< get parameter */
                ("isc_get_raw_mode", c_int),                        # IscGetModeRaw isc_get_raw_mode;         /**< raw mode on/off */
                ("isc_get_color_mode", c_int),                      # IscGetModeColor isc_get_color_mode;     /**< color mode on/off */

                ("isc_record_mode", c_int),                         # IscRecordMode isc_record_mode;          /**< save data when grabbing */
                ("isc_play_mode", c_int),                           # IscPlayMode isc_play_mode;              /**< read from file instead of camera */
                ("isc_play_mode_parameter", IscPalyModeParameter)   # IscPalyModeParameter isc_play_mode_parameter;   /**< This is the parameter for play image */
    ]

# /** @struct  IscDataProcStartMode
# *  @brief This is the parameter for start process
# */
class IscDataProcStartMode (Structure):
    _fields_ = [
                ("enabled_stereo_matching", c_bool),    # bool enabled_block_matching;                  /**< whether to use a soft stereo matching */
                ("enabled_frame_decoder", c_bool) ,     # bool enabled_frame_decoder;                   /**< whether to use a frame decoder */
                ("enabled_disparity_filter", c_bool)    # bool enabled_disparity_filter;                /**< whether to use a disparity filter */
    ]

# /** @struct  IscStartMode
# *  @brief This is the for start
# */
class IscStartMode (Structure):
    _fields_ = [
                ("isc_grab_start_mode", IscGrabStartMode),          # IscGrabStartMode isc_grab_start_mode;
                ("isc_dataproc_start_mode", IscDataProcStartMode)   # IscDataProcStartMode isc_dataproc_start_mode;
    ]

# /** @struct  IscAreaDataStatistics
#  *  @brief This is the data statistics information
#  */
class Statistics (Structure):
    _fields_ = [
                ("max_value", c_float),             #/**< maximum value */
                ("min_value", c_float),             #/**< minimum value */
                ("std_dev", c_float),               #/**< standard deviation */
                ("average", c_float),               #/**< average value */
                ("median", c_float) ,               #/**< median value */
                ("mode", c_float)                   #/**< most frequent value */
    ]

class Roi3D (Structure):
    _fields_ = [
                ("width", c_float),                 #/**<  width of region of area(m) */
                ("height", c_float),                #/**<  height of region of area(m) */
                ("distance", c_float)               #/**<  distance of region of area(m) */
    ]

class IscAreaDataStatistics (Structure):
    _fields_ = [
                ("x", c_int),                           #/**< top left of region */
                ("y", c_int),                           #/**< top left of region */
                ("width", c_int),                       #/**< width of region */
                ("height", c_int),                      #/**< height of region */

                ("min_distance", c_float) ,             #/**< Minimum display distance */
                ("max_distance", c_float) ,             #/**< Maximum display distance */

                ("statistics_depth", Statistics),       #/**< parallax stats */
                ("statistics_distance", Statistics),    #/**< distance statistics */
                ("roi_3d", Roi3D)                       #/**< distance of 3D */
    ]

# /** @struct  IscDataProcModuleParameter
#  *  @brief This is the data processing module parameter
#  */
class ParameterSet (Structure):
    _fields_ = [
                ("value_type", c_int),          #/**< 0:int 1:float 2:double */
                ("value_int", c_int),           #/**< value for interger */
                ("value_float", c_float),       #/**< value for float */
                ("value_double", c_double),     #/**< value for double */

                ("category", c_wchar * 32),     #/**< category name */
                ("name", c_wchar * 16),         #/**< parameter name */
                ("description", c_wchar * 32)   #/**< Parameter description */
    ]

class IscDataProcModuleParameter (Structure):
    _fields_ = [
                ("module_index", c_int),                #/**< index 0~ */
                ("module_name", c_wchar * 32),          #/**< module name */

                ("parameter_count", c_int),             #/**< number of parameterse */
                ("parameter_set", ParameterSet * 48)    #/**< parameters */
    ]

# /** @struct  IscDataProcStatus
#  *  @brief This is the status of data proccesing
#  */
class IscDataProcStatus (Structure):
    _fields_ = [
                ("error_code", c_int),          #/**< error code */
                ("proc_tact_time", c_double)    #/**< cycle of data processing */
    ]

# /** @struct  IscDataProcModuleStatus
#  *  @brief This is the status of each data proccesing module
#  */
class IscDataProcModuleStatus (Structure):
    _fields_ = [
                ("module_names", c_char * 32),  #/**< module name */
                ("error_code", c_int),          #/**< error code */
                ("processing_time", c_double)   #/**< processing time */
    ]

# /** @struct  IscDataProcessResultData
#  *  @brief This is the data processing module result
#  */
class IscDataProcResultData (Structure):
    _fields_ = [
                ("number_of_modules_processed", c_int),         #/**< Number of modules processed */
                ("maximum_number_of_modules", c_int),           #/**< Maximum number of modules = 4 */
                ("maximum_number_of_modulename", c_int),        #/**< Maximum number of module name = 32 */

                ("status", IscDataProcStatus),                  #/**< status of data proccesing */
                ("module_status", IscDataProcModuleStatus * 4), #/**< IscDataProcModuleStatus */

                ("isc_image_info", IscImageInfo)                #/**< result processed by the module */
    ]

class IscDplIf:
    def __init__(self):
        pass

    def __del__(self):
        pass

    def load_functions(self):
        # load dpl dlls
        try:
            libdpl = WinDLL("./IscDplC.dll")
        except FileNotFoundError as err:
            print("[ERROR][IscDplIf] DLL load error:",err)
            return 1

        try:
            # load functions
            #ISCDPLC_EXPORTS_API int DplInitialize(const IscDplConfiguration* ipc_dpl_configuratio);
            self.DplInitialize = libdpl.DplInitialize
            self.DplInitialize.argtypes = [POINTER(IscDplConfiguration)]
            self.DplInitialize.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplTerminate();
            self.DplTerminate = libdpl.DplTerminate
            self.DplTerminate.restype = c_uint

            #// camera dependent paraneter

            #ISCDPLC_EXPORTS_API bool DplDeviceOptionIsImplementedInfo(const IscCameraInfo option_name);
            self.DplDeviceOptionIsImplementedInfo = libdpl.DplDeviceOptionIsImplementedInfo
            self.DplDeviceOptionIsImplementedInfo.argtypes = [c_int]
            self.DplDeviceOptionIsImplementedInfo.restype = c_bool

            #ISCDPLC_EXPORTS_API bool DplDeviceOptionIsReadableInfo(const IscCameraInfo option_name);
            self.DplDeviceOptionIsReadableInfo = libdpl.DplDeviceOptionIsReadableInfo
            self.DplDeviceOptionIsReadableInfo.argtypes = [c_int]
            self.DplDeviceOptionIsReadableInfo.restype = c_bool

            #ISCDPLC_EXPORTS_API bool DplDeviceOptionIsWritableInfo(const IscCameraInfo option_name);
            self.DplDeviceOptionIsWritableInfo = libdpl.DplDeviceOptionIsWritableInfo
            self.DplDeviceOptionIsWritableInfo.argtypes = [c_int]
            self.DplDeviceOptionIsWritableInfo.restype = c_bool

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinInfoInt(const IscCameraInfo option_name, int* value);
            self.DplDeviceGetOptionMinInfoInt = libdpl.DplDeviceGetOptionMinInfoInt
            self.DplDeviceGetOptionMinInfoInt.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionMinInfoInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxInfoInt(const IscCameraInfo option_name, int* value);
            self.DplDeviceGetOptionMaxInfoInt = libdpl.DplDeviceGetOptionMaxInfoInt
            self.DplDeviceGetOptionMaxInfoInt.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionMaxInfoInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionIncInfoInt(const IscCameraInfo option_name, int* value);
            self.DplDeviceGetOptionIncInfoInt = libdpl.DplDeviceGetOptionIncInfoInt
            self.DplDeviceGetOptionIncInfoInt.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionIncInfoInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoInt(const IscCameraInfo option_name, int* value);
            self.DplDeviceGetOptionInfoInt = libdpl.DplDeviceGetOptionInfoInt
            self.DplDeviceGetOptionInfoInt.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionInfoInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoInt(const IscCameraInfo option_name, const int value);
            self.DplDeviceSetOptionInfoInt = libdpl.DplDeviceSetOptionInfoInt
            self.DplDeviceSetOptionInfoInt.argtypes = [c_int, c_int]
            self.DplDeviceSetOptionInfoInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinInfoFloat(const IscCameraInfo option_name, float* value);
            self.DplDeviceGetOptionMinInfoFloat = libdpl.DplDeviceGetOptionMinInfoFloat
            self.DplDeviceGetOptionMinInfoFloat.argtypes = [c_int, POINTER(c_float)]
            self.DplDeviceGetOptionMinInfoFloat.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxInfoFloat(const IscCameraInfo option_name, float* value);
            self.DplDeviceGetOptionMaxInfoFloat = libdpl.DplDeviceGetOptionMaxInfoFloat
            self.DplDeviceGetOptionMaxInfoFloat.argtypes = [c_int, POINTER(c_float)]
            self.DplDeviceGetOptionMaxInfoFloat.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoFloat(const IscCameraInfo option_name, float* value);
            self.DplDeviceGetOptionInfoFloat = libdpl.DplDeviceGetOptionInfoFloat
            self.DplDeviceGetOptionInfoFloat.argtypes = [c_int, POINTER(c_float)]
            self.DplDeviceGetOptionInfoFloat.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoFloat(const IscCameraInfo option_name, const float value);
            self.DplDeviceSetOptionInfoFloat = libdpl.DplDeviceSetOptionInfoFloat
            self.DplDeviceSetOptionInfoFloat.argtypes = [c_int, c_float]
            self.DplDeviceSetOptionInfoFloat.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoBool(const IscCameraInfo option_name, bool* value);
            self.DplDeviceGetOptionInfoBool = libdpl.DplDeviceGetOptionInfoBool
            self.DplDeviceGetOptionInfoBool.argtypes = [c_int, POINTER(c_bool)]
            self.DplDeviceGetOptionInfoBool.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoBool(const IscCameraInfo option_name, const bool value);
            self.DplDeviceSetOptionInfoBool = libdpl.DplDeviceSetOptionInfoBool
            self.DplDeviceSetOptionInfoBool.argtypes = [c_int, c_bool]
            self.DplDeviceSetOptionInfoBool.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoChar(const IscCameraInfo option_name, char* value, const int max_length);
            self.DplDeviceGetOptionInfoChar = libdpl.DplDeviceGetOptionInfoChar
            self.DplDeviceGetOptionInfoChar.argtypes = [c_int, POINTER(c_char), c_int]
            self.DplDeviceGetOptionInfoChar.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoChar(const IscCameraInfo option_name, const char* value);
            self.DplDeviceSetOptionInfoChar = libdpl.DplDeviceSetOptionInfoChar
            self.DplDeviceSetOptionInfoChar.argtypes = [c_int, POINTER(c_char)]
            self.DplDeviceSetOptionInfoChar.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinInfoInt64(const IscCameraInfo option_name, uint64_t* value);
            self.DplDeviceGetOptionMinInfoInt64 = libdpl.DplDeviceGetOptionMinInfoInt64
            self.DplDeviceGetOptionMinInfoInt64.argtypes = [c_int, POINTER(c_uint64)]
            self.DplDeviceGetOptionMinInfoInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxInfoInt64(const IscCameraInfo option_name, uint64_t* value);
            self.DplDeviceGetOptionMaxInfoInt64 = libdpl.DplDeviceGetOptionMaxInfoInt64
            self.DplDeviceGetOptionMaxInfoInt64.argtypes = [c_int, POINTER(c_uint64)]
            self.DplDeviceGetOptionMaxInfoInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionIncInfoInt64(const IscCameraInfo option_name, uint64_t* value);
            self.DplDeviceGetOptionIncInfoInt64 = libdpl.DplDeviceGetOptionIncInfoInt64
            self.DplDeviceGetOptionIncInfoInt64.argtypes = [c_int, POINTER(c_uint64)]
            self.DplDeviceGetOptionIncInfoInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionInfoInt64(const IscCameraInfo option_name, uint64_t* value);
            self.DplDeviceGetOptionInfoInt64 = libdpl.DplDeviceGetOptionInfoInt64
            self.DplDeviceGetOptionInfoInt64.argtypes = [c_int, POINTER(c_uint64)]
            self.DplDeviceGetOptionInfoInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionInfoInt64(const IscCameraInfo option_name, const uint64_t value);
            self.DplDeviceSetOptionInfoInt64 = libdpl.DplDeviceSetOptionInfoInt64
            self.DplDeviceSetOptionInfoInt64.argtypes = [c_int, c_uint64]
            self.DplDeviceSetOptionInfoInt64.restype = c_uint

            #// camera control parameter

            #ISCDPLC_EXPORTS_API bool DplDeviceOptionIsImplementedPara(const IscCameraParameter option_name);
            self.DplDeviceOptionIsImplementedPara = libdpl.DplDeviceOptionIsImplementedPara
            self.DplDeviceOptionIsImplementedPara.argtypes = [c_int]
            self.DplDeviceOptionIsImplementedPara.restype = c_bool

            #ISCDPLC_EXPORTS_API bool DplDeviceOptionIsReadablePara(const IscCameraParameter option_name);
            self.DplDeviceOptionIsReadablePara = libdpl.DplDeviceOptionIsReadablePara
            self.DplDeviceOptionIsReadablePara.argtypes = [c_int]
            self.DplDeviceOptionIsReadablePara.restype = c_bool

            #ISCDPLC_EXPORTS_API bool DplDeviceOptionIsWritablePara(const IscCameraParameter option_name);
            self.DplDeviceOptionIsWritablePara = libdpl.DplDeviceOptionIsWritablePara
            self.DplDeviceOptionIsWritablePara.argtypes = [c_int]
            self.DplDeviceOptionIsWritablePara.restype = c_bool

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinParaInt(const IscCameraParameter option_name, int* value);
            self.DplDeviceGetOptionMinParaInt = libdpl.DplDeviceGetOptionMinParaInt
            self.DplDeviceGetOptionMinParaInt.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionMinParaInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxParaInt(const IscCameraParameter option_name, int* value);
            self.DplDeviceGetOptionMaxParaInt = libdpl.DplDeviceGetOptionMaxParaInt
            self.DplDeviceGetOptionMaxParaInt.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionMaxParaInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionIncParaInt(const IscCameraParameter option_name, int* value);
            self.DplDeviceGetOptionIncParaInt = libdpl.DplDeviceGetOptionIncParaInt
            self.DplDeviceGetOptionIncParaInt.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionIncParaInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaInt(const IscCameraParameter option_name, int* value);
            self.DplDeviceGetOptionParaInt = libdpl.DplDeviceGetOptionParaInt
            self.DplDeviceGetOptionParaInt.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionParaInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaInt(const IscCameraParameter option_name, const int value);
            self.DplDeviceSetOptionParaInt = libdpl.DplDeviceSetOptionParaInt
            self.DplDeviceSetOptionParaInt.argtypes = [c_int, c_int]
            self.DplDeviceSetOptionParaInt.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinParaFloat(const IscCameraParameter option_name, float* value);
            self.DplDeviceGetOptionMinParaFloat = libdpl.DplDeviceGetOptionMinParaFloat
            self.DplDeviceGetOptionMinParaFloat.argtypes = [c_int, POINTER(c_float)]
            self.DplDeviceGetOptionMinParaFloat.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxParaFloat(const IscCameraParameter option_name, float* value);
            self.DplDeviceGetOptionMaxParaFloat = libdpl.DplDeviceGetOptionMaxParaFloat
            self.DplDeviceGetOptionMaxParaFloat.argtypes = [c_int, POINTER(c_float)]
            self.DplDeviceGetOptionMaxParaFloat.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaFloat(const IscCameraParameter option_name, float* value);
            self.DplDeviceGetOptionParaFloat = libdpl.DplDeviceGetOptionParaFloat
            self.DplDeviceGetOptionParaFloat.argtypes = [c_int, POINTER(c_float)]
            self.DplDeviceGetOptionParaFloat.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaFloat(const IscCameraParameter option_name, const float value);
            self.DplDeviceSetOptionParaFloat = libdpl.DplDeviceSetOptionParaFloat
            self.DplDeviceSetOptionParaFloat.argtypes = [c_int, c_float]
            self.DplDeviceSetOptionParaFloat.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaBool(const IscCameraParameter option_name, bool* value);
            self.DplDeviceGetOptionParaBool = libdpl.DplDeviceGetOptionParaBool
            self.DplDeviceGetOptionParaBool.argtypes = [c_int, POINTER(c_bool)]
            self.DplDeviceGetOptionParaBool.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaBool(const IscCameraParameter option_name, const bool value);
            self.DplDeviceSetOptionParaBool = libdpl.DplDeviceSetOptionParaBool
            self.DplDeviceSetOptionParaBool.argtypes = [c_int, c_bool]
            self.DplDeviceSetOptionParaBool.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaChar(const IscCameraParameter option_name, char* value, const int max_length);
            self.DplDeviceGetOptionParaChar = libdpl.DplDeviceGetOptionParaChar
            self.DplDeviceGetOptionParaChar.argtypes = [c_int, POINTER(c_char), c_int]
            self.DplDeviceGetOptionParaChar.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaChar(const IscCameraParameter option_name, const char* value);
            self.DplDeviceSetOptionParaChar = libdpl.DplDeviceSetOptionParaChar
            self.DplDeviceSetOptionParaChar.argtypes = [c_int, POINTER(c_char)]
            self.DplDeviceSetOptionParaChar.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMinParaInt64(const IscCameraParameter option_name, uint64_t* value);
            self.DplDeviceGetOptionMinParaInt64 = libdpl.DplDeviceGetOptionMinParaInt64
            self.DplDeviceGetOptionMinParaInt64.argtypes = [c_int, POINTER(c_uint64)]
            self.DplDeviceGetOptionMinParaInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionMaxParaInt64(const IscCameraParameter option_name, uint64_t* value);
            self.DplDeviceGetOptionMaxParaInt64 = libdpl.DplDeviceGetOptionMaxParaInt64
            self.DplDeviceGetOptionMaxParaInt64.argtypes = [c_int, POINTER(c_uint64)]
            self.DplDeviceGetOptionMaxParaInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionIncParaInt64(const IscCameraParameter option_name, uint64_t* value);
            self.DplDeviceGetOptionIncParaInt64 = libdpl.DplDeviceGetOptionIncParaInt64
            self.DplDeviceGetOptionIncParaInt64.argtypes = [c_int, POINTER(c_uint64)]
            self.DplDeviceGetOptionIncParaInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaInt64(const IscCameraParameter option_name, uint64_t* value);
            self.DplDeviceGetOptionParaInt64 = libdpl.DplDeviceGetOptionParaInt64
            self.DplDeviceGetOptionParaInt64.argtypes = [c_int, POINTER(c_uint64)]
            self.DplDeviceGetOptionParaInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaInt64(const IscCameraParameter option_name, const uint64_t value);
            self.DplDeviceSetOptionParaInt64 = libdpl.DplDeviceSetOptionParaInt64
            self.DplDeviceSetOptionParaInt64.argtypes = [c_int, c_uint64]
            self.DplDeviceSetOptionParaInt64.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceGetOptionParaShMode(const IscCameraParameter option_name, IscShutterMode* value);
            self.DplDeviceGetOptionParaShMode = libdpl.DplDeviceGetOptionParaShMode
            self.DplDeviceGetOptionParaShMode.argtypes = [c_int, POINTER(c_int)]
            self.DplDeviceGetOptionParaShMode.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplDeviceSetOptionParaShMode(const IscCameraParameter option_name, const IscShutterMode value);
            self.DplDeviceSetOptionParaShMode = libdpl.DplDeviceSetOptionParaShMode
            self.DplDeviceSetOptionParaShMode.argtypes = [c_int, c_int]
            self.DplDeviceSetOptionParaShMode.restype = c_uint

            #// grab control

            #ISCDPLC_EXPORTS_API int DplStart(const IscStartMode* isc_start_mode);
            self.DplStart = libdpl.DplStart
            self.DplStart.argtypes = [POINTER(IscStartMode)]
            self.DplStart.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplStop();
            self.DplStop = libdpl.DplStop
            self.DplStop.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetGrabMode(IscGrabStartMode* isc_grab_start_mode);
            self.DplGetGrabMode = libdpl.DplGetGrabMode
            self.DplGetGrabMode.argtypes = [POINTER(IscGrabStartMode)]
            self.DplGetGrabMode.restype = c_uint

            #// image & data get 

            #ISCDPLC_EXPORTS_API int DplInitializeIscIamgeinfo(IscImageInfo* isc_image_Info);
            self.DplInitializeIscIamgeinfo = libdpl.DplInitializeIscIamgeinfo
            self.DplInitializeIscIamgeinfo.argtypes = [POINTER(IscImageInfo)]
            self.DplInitializeIscIamgeinfo.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplReleaeIscIamgeinfo(IscImageInfo* isc_image_Info);
            self.DplReleaeIscIamgeinfo = libdpl.DplReleaeIscIamgeinfo
            self.DplReleaeIscIamgeinfo.argtypes = [POINTER(IscImageInfo)]
            self.DplReleaeIscIamgeinfo.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetCameraData(IscImageInfo* isc_image_Info);
            self.DplGetCameraData = libdpl.DplGetCameraData
            self.DplGetCameraData.argtypes = [POINTER(IscImageInfo)]
            self.DplGetCameraData.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetFileInformation(wchar_t* play_file_name, IscRawFileHeader* raw_file_header);
            self.DplGetFileInformation = libdpl.DplGetFileInformation
            self.DplGetFileInformation.argtypes = [POINTER(c_wchar), POINTER(IscRawFileHeader)]
            self.DplGetFileInformation.restype = c_uint

            #// get information for depth, distance, ...

            #ISCDPLC_EXPORTS_API int DplGetPositionDepth(const int x, const int y, const IscImageInfo* isc_image_info, float* disparity, float* depth);
            self.DplGetPositionDepth = libdpl.DplGetPositionDepth
            self.DplGetPositionDepth.argtypes = [c_int, c_int, POINTER(IscImageInfo), POINTER(c_float), POINTER(c_float)]
            self.DplGetPositionDepth.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetPosition3D(const int x, const int y, const IscImageInfo* isc_image_info, float* x_d, float* y_d, float* z_d);
            self.DplGetPosition3D = libdpl.DplGetPosition3D
            self.DplGetPosition3D.argtypes = [c_int, c_int, POINTER(IscImageInfo), POINTER(c_float), POINTER(c_float), POINTER(c_float)]
            self.DplGetPosition3D.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetAreaStatistics(const int x, const int y, const int width, const int height, const IscImageInfo* isc_image_info, IscAreaDataStatistics* isc_data_statistics);
            self.DplGetAreaStatistics = libdpl.DplGetAreaStatistics
            self.DplGetAreaStatistics.argtypes = [c_int, c_int, c_int, c_int, POINTER(IscImageInfo), POINTER(IscAreaDataStatistics)]
            self.DplGetAreaStatistics.restype = c_uint

            #// data processing module settings

            #ISCDPLC_EXPORTS_API int DplGetTotalModuleCount(int* total_count);
            self.DplGetTotalModuleCount = libdpl.DplGetTotalModuleCount
            self.DplGetTotalModuleCount.argtypes = [POINTER(c_int)]
            self.DplGetTotalModuleCount.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetModuleNameByIndex(const int module_index, wchar_t* module_name, int max_length);
            self.DplGetModuleNameByIndex = libdpl.DplGetModuleNameByIndex
            self.DplGetModuleNameByIndex.argtypes = [c_int, POINTER(c_wchar), c_int]
            self.DplGetModuleNameByIndex.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter);
            self.DplGetDataProcModuleParameter = libdpl.DplGetDataProcModuleParameter
            self.DplGetDataProcModuleParameter.argtypes = [c_int, POINTER(IscDataProcModuleParameter)]
            self.DplGetDataProcModuleParameter.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplSetDataProcModuleParameter(const int module_index, IscDataProcModuleParameter* isc_data_proc_module_parameter, const bool is_update_file);
            self.DplSetDataProcModuleParameter = libdpl.DplSetDataProcModuleParameter
            self.DplSetDataProcModuleParameter.argtypes = [c_int, POINTER(IscDataProcModuleParameter), c_bool]
            self.DplSetDataProcModuleParameter.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetParameterFileName(const int module_index, wchar_t* file_name, const int max_length);
            self.DplGetParameterFileName = libdpl.DplGetParameterFileName
            self.DplGetParameterFileName.argtypes = [c_int, POINTER(c_wchar), c_int]
            self.DplGetParameterFileName.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplReloadParameterFromFile(const int module_index, const wchar_t* file_name, const bool is_valid);
            self.DplReloadParameterFromFile = libdpl.DplReloadParameterFromFile
            self.DplReloadParameterFromFile.argtypes = [c_int, POINTER(c_wchar), c_int]
            self.DplReloadParameterFromFile.restype = c_uint

            #// data processing module result data

            #ISCDPLC_EXPORTS_API int DplInitializeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data);
            self.DplInitializeIscDataProcResultData = libdpl.DplInitializeIscDataProcResultData
            self.DplInitializeIscDataProcResultData.argtypes = [POINTER(IscDataProcResultData)]
            self.DplInitializeIscDataProcResultData.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplReleaeIscDataProcResultData(IscDataProcResultData* isc_data_proc_result_data);
            self.DplReleaeIscDataProcResultData = libdpl.DplReleaeIscDataProcResultData
            self.DplReleaeIscDataProcResultData.argtypes = [POINTER(IscDataProcResultData)]
            self.DplReleaeIscDataProcResultData.restype = c_uint

            #ISCDPLC_EXPORTS_API int DplGetDataProcModuleData(IscDataProcResultData* isc_data_proc_result_data);        
            self.DplGetDataProcModuleData = libdpl.DplGetDataProcModuleData
            self.DplGetDataProcModuleData.argtypes = [POINTER(IscDataProcResultData)]
            self.DplGetDataProcModuleData.restype = c_uint

        except AttributeError as err:
            print("[ERROR][IscDplIf]AttributeError:", err)
            return 1

        return 0

    def initialize(self, camera):

        # load function from dll
        ret = self.load_functions()
        if ret != 0:
            print("[ERROR][IscDplIf]load_functions failed(0x{:08X})".format(ret))
            return 1
        
        print("[INFO][IscDplIf]load_functions was successful.")

        # Initialize
        self.isc_dpl_config = IscDplConfiguration()
        self.isc_dpl_config.configuration_file_path = u"./"
        self.isc_dpl_config.log_file_path = u"c:/temp"
        self.isc_dpl_config.log_level = c_int(0)
        self.isc_dpl_config.enabled_camera = c_bool(True)

        if camera == 0:
            self.isc_dpl_config.isc_camera_model = IscCameraModel.kVM
        elif camera == 1:
            self.isc_dpl_config.isc_camera_model = IscCameraModel.kXC

        self.isc_dpl_config.save_image_path = u"c:/temp"
        self.isc_dpl_config.load_image_path = u"c:/temp"
        
        self.isc_dpl_config.enabled_data_proc_module = c_bool(True)

        ret = self.DplInitialize(self.isc_dpl_config)
        if ret != 0:
            print("[ERROR][IscDplIf]DplInitialize failed(0x{:08X})".format(ret))
            return 1

        # get image buffer
        print("[INFO][IscDplIf]Initialize IscIamgeinfo")
        self.isc_image_info = IscImageInfo()
        ret = self.DplInitializeIscIamgeinfo(self.isc_image_info)
        if ret != 0:
            print("[ERROR][IscDplIf]DplInitializeIscIamgeinfo failed(0x{:08X})".format(ret))
            return 1
        
        print("[INFO][IscDplIf]DplInitializeIscIamgeinfo was successful.")

        # get data proc result buffer
        print("[INFO][IscDplIf]Initialize IscDataProcResultData")
        self.iscdataproc_result_data = IscDataProcResultData()
        ret = self.DplInitializeIscDataProcResultData(self.iscdataproc_result_data)
        if ret != 0:
            print("[ERROR][IscDplIf]DplInitializeIscDataProcResultData failed(0x{:08X})".format(ret))
            return 1
        
        print("[INFO][IscDplIf]DplInitializeIscDataProcResultData was successful.")

        # start mode initialize
        self.isc_start_mode = IscStartMode()

        self.isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode.kParallax
        self.isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode.kColorON
        self.isc_start_mode.isc_grab_start_mode.isc_get_mode.wait_time = c_int(100)
        self.isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw.kRawOn
        self.isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor.kAwb
        self.isc_start_mode.isc_grab_start_mode.isc_record_mode = IscRecordMode.kRecordOff
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode = IscPlayMode.kPlayOff
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.interval = c_int(16)
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name = u"c:/temp/dummy.dat"
        self.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = c_bool(False)
        self.isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = c_bool(True)
        self.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = c_bool(True)

        # get camera parameter
        ctypes_base_length = c_float(0.0)
        ctypes_bf = c_float(0.0)
        ctypes_dinf = c_float(0.0)
        ctypes_width = c_int(0)
        ctypes_height = c_int(0)

        ret = self.DplDeviceGetOptionInfoFloat(IscCameraInfo.kBaseLength, pointer(ctypes_base_length));
        if ret != 0:
            print("[ERROR][IscDplIf]DplDeviceGetOptionInfoFloat(kBaseLength) failed(0x{:08X})".format(ret))
            return 1

        ret = self.DplDeviceGetOptionInfoFloat(IscCameraInfo.kBF, pointer(ctypes_bf));
        if ret != 0:
            print("[ERROR][IscDplIf]DplDeviceGetOptionInfoFloat(kBF) failed(0x{:08X})".format(ret))
            return 1

        ret = self.DplDeviceGetOptionInfoFloat(IscCameraInfo.kDINF, pointer(ctypes_dinf));
        if ret != 0:
            print("[ERROR][IscDplIf]DplDeviceGetOptionInfoFloat(kDINF) failed(0x{:08X})".format(ret))
            return 1

        ret = self.DplDeviceGetOptionInfoInt(IscCameraInfo.kWidthMax, pointer(ctypes_width));
        if ret != 0:
            print("[ERROR][IscDplIf]DplDeviceGetOptionInfoInt(kWidthMax) failed(0x{:08X})".format(ret))
            return 1

        ret = self.DplDeviceGetOptionInfoInt(IscCameraInfo.kHeightMax, pointer(ctypes_height));
        if ret != 0:
            print("[ERROR][IscDplIf]DplDeviceGetOptionInfoInt(kHeightMax) failed(0x{:08X})".format(ret))
            return 1

        self.base_length = ctypes_base_length.value
        self.bf = ctypes_bf.value
        self.dinf = ctypes_dinf.value
        self.width = ctypes_width.value
        self.height = ctypes_height.value

        print("[INFO][IscDplIf]Base length={0:.4f},BF={1:.4f},DINF={2:.4f}".format(self.base_length, self.bf, self.dinf))
        print("[INFO][IscDplIf]Width={0} Height={1}".format(self.width, self.height))

        # set shutter mode to single shutter
        ret = self.DplDeviceSetOptionParaShMode(IscCameraParameter.kShutterMode, IscShutterMode.kSingleShutter);
        if ret != 0:
            print("[ERROR][IscDplIf]DplDeviceSetOptionParaShMode failed(0x{:08X})".format(ret))
            return 1

        ctypes_shutter_mode_current = c_int(0)
        ret = self.DplDeviceGetOptionParaShMode(IscCameraParameter.kShutterMode, pointer(ctypes_shutter_mode_current));
        if ret != 0:
            print("[ERROR][IscDplIf]DplDeviceGetOptionParaShMode failed(0x{:08X})".format(ret))
            return 1

        print("[INFO][IscDplIf]Shutter mode={0}".format(ctypes_shutter_mode_current.value))

        # prepare dummy error image
        image_size = self.width * self.height

        self.error_image_base = np.zeros(image_size, dtype=c_ubyte)
        np.reshape(self.error_image_base, (self.height, self.width))
        self.error_image_base[:] = 0

        self.error_image_color = np.zeros(image_size * 3, dtype=c_ubyte)
        np.reshape(self.error_image_color, (self.height, self.width, 3))
        self.error_image_color[:] = 0

        self.error_data_depth = np.zeros(image_size, dtype=c_float)
        np.reshape(self.error_data_depth, (self.height, self.width))
        self.error_data_depth[:] = 0

        print("[INFO][IscDplIf]initialize was successful.")

        return 0

    def initialize_file_mode(self, file_name):

        # load function from dll
        ret = self.load_functions()
        if ret != 0:
            print("[ERROR][IscDplIf]load_functions failed(0x{:08X})".format(ret))
            return 1
        
        print("[INFO][IscDplIf]load_functions was successful.")

        # Initialize
        self.isc_dpl_config = IscDplConfiguration()
        self.isc_dpl_config.configuration_file_path = u"./"
        self.isc_dpl_config.log_file_path = u"c:/temp"
        self.isc_dpl_config.log_level = c_int(0)
        self.isc_dpl_config.enabled_camera = c_bool(False)  # not use camera

        # Temporary Camera Designation
        self.isc_dpl_config.isc_camera_model = IscCameraModel.kXC

        self.isc_dpl_config.save_image_path = u"c:/temp"
        self.isc_dpl_config.load_image_path = u"c:/temp"
        
        self.isc_dpl_config.enabled_data_proc_module = c_bool(True)

        ret = self.DplInitialize(self.isc_dpl_config)
        if ret != 0:
            print("[ERROR][IscDplIf]DplInitialize failed(0x{:08X})".format(ret))
            return 1

        # get image buffer
        print("[INFO][IscDplIf]Initialize IscIamgeinfo")
        self.isc_image_info = IscImageInfo()
        ret = self.DplInitializeIscIamgeinfo(self.isc_image_info)
        if ret != 0:
            print("[ERROR][IscDplIf]DplInitializeIscIamgeinfo failed(0x{:08X})".format(ret))
            return 1
        
        print("[INFO][IscDplIf]DplInitializeIscIamgeinfo was successful.")

        # get data proc result buffer
        print("[INFO][IscDplIf]Initialize IscDataProcResultData")
        self.iscdataproc_result_data = IscDataProcResultData()
        ret = self.DplInitializeIscDataProcResultData(self.iscdataproc_result_data)
        if ret != 0:
            print("[ERROR][IscDplIf]DplInitializeIscDataProcResultData failed(0x{:08X})".format(ret))
            return 1
        
        print("[INFO][IscDplIf]DplInitializeIscDataProcResultData was successful.")

        # start mode initialize
        self.isc_start_mode = IscStartMode()

        self.isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode.kParallax
        self.isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode.kColorON
        self.isc_start_mode.isc_grab_start_mode.isc_get_mode.wait_time = c_int(100)
        self.isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw.kRawOn
        self.isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor.kAwb
        self.isc_start_mode.isc_grab_start_mode.isc_record_mode = IscRecordMode.kRecordOff
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode = IscPlayMode.kPlayOn
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.interval = c_int(16)
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name = file_name
        self.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = c_bool(False)
        self.isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = c_bool(True)
        self.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = c_bool(True)

        # get parameter from file
        self.isc_raw_file_header = IscRawFileHeader()
        ret = self.DplGetFileInformation(self.isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, self.isc_raw_file_header)
        if ret != 0:
            print("[ERROR][IscDplIf]DplGetFileInformation failed(0x{:08X})".format(ret))
            return 1

        # file information
        print("[INFO][IscDplIf]File information")
        print("                camera model:{}".format(self.isc_raw_file_header.camera_model))
        print("                max_width:{}".format(self.isc_raw_file_header.max_width))
        print("                max_height:{}".format(self.isc_raw_file_header.max_height))
        print("                Base length={0:.4f},BF={1:.4f},DINF={2:.4f}".format(self.isc_raw_file_header.base_length, self.isc_raw_file_header.bf, self.isc_raw_file_header.d_inf))
        print("                grab_mode:{}".format(self.isc_raw_file_header.grab_mode))
        print("                shutter_mode:{}".format(self.isc_raw_file_header.shutter_mode))
        print("                color_mode:{}".format(self.isc_raw_file_header.color_mode))
        print("                *If grab_mode=2, then the show parameter should be 1. If 0, no disparity is displayed")
        
        self.base_length = self.isc_raw_file_header.base_length
        self.bf = self.isc_raw_file_header.bf
        self.dinf = self.isc_raw_file_header.d_inf
        self.width = self.isc_raw_file_header.max_width
        self.height = self.isc_raw_file_header.max_height

        print("[INFO][IscDplIf]Width={0} Height={1}".format(self.width, self.height))

        # prepare dummy error image
        image_size = self.width * self.height

        self.error_image_base = np.zeros(image_size, dtype=c_ubyte)
        np.reshape(self.error_image_base, (self.height, self.width))
        self.error_image_base[:] = 0

        self.error_image_color = np.zeros(image_size * 3, dtype=c_ubyte)
        np.reshape(self.error_image_color, (self.height, self.width, 3))
        self.error_image_color[:] = 0

        self.error_data_depth = np.zeros(image_size, dtype=c_float)
        np.reshape(self.error_data_depth, (self.height, self.width))
        self.error_data_depth[:] = 0

        print("[INFO][IscDplIf]initialize was successful.")

        return 0
        
    def terminate(self):

        ret = self.DplReleaeIscDataProcResultData(self.iscdataproc_result_data)
        if ret != 0:
            print("[ERROR][IscDplIf]DplReleaeIscDataProcResultData failed(0x{:08X})".format(ret))

        ret = self.DplReleaeIscIamgeinfo(self.isc_image_info)
        if ret != 0:
            print("[ERROR][IscDplIf]DplReleaeIscIamgeinfo failed(0x{:08X})".format(ret))

        ret = self.DplTerminate()
        if ret != 0:
            print("[ERROR][IscDplIf]DplTerminate failed(0x{:08X})".format(ret))
    
        print("[INFO][IscDplIf]terminate was successful.")

        return 0

    def get_with(self):
        return self.width
    
    def get_height(self):
        return self.height

    def get_base_length(self):
        return self.base_length
    
    def get_bf(self):
        return self.bf
    
    def get_dinf(self):
        return self.dinf

    def start(self, disparity_mode, show_mode):
        # start grab
        print("[INFO][IscDplIf]start")

        # reset mode
        if disparity_mode == 0:
            # camera disparity
            self.isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode.kParallax

            if show_mode == 0:
                # Use disparity from the camera
                self.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = c_bool(False)
                self.isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = c_bool(False)
                self.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = c_bool(False)
            else:
                # Use software matching disparity
                self.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = c_bool(False)
                self.isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = c_bool(True)
                self.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = c_bool(True)

        elif disparity_mode == 1:
            # data processing disparity
            self.isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode.kCorrect
            # Use software matching disparity
            self.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = c_bool(True)
            self.isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = c_bool(True)
            self.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = c_bool(True)

        self.isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode.kColorON
        self.isc_start_mode.isc_grab_start_mode.isc_get_mode.wait_time = c_int(100)
        self.isc_start_mode.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw.kRawOn
        self.isc_start_mode.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor.kAwb
        self.isc_start_mode.isc_grab_start_mode.isc_record_mode = IscRecordMode.kRecordOff
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode = IscPlayMode.kPlayOff
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.interval = c_int(16)
        self.isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name = u"c:/temp/dummy.dat"

        ret = self.DplStart(self.isc_start_mode)
        # Error check
        if ret != 0:
            print("[ERROR][IscDplIf]start failed(0x{:08X})".format(ret))
            return -1

        return 0

    def start_file_mode(self, show_mode, play_file_name):
        # start grab
        print("[INFO][IscDplIf]start file mode")
        print("[INFO][IscDplIf]file name:{}".format(self.isc_start_mode.isc_grab_start_mode.isc_play_mode_parameter.play_file_name))

        # set up start mode from file header information
        if self.isc_raw_file_header.grab_mode == 1:
            self.isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode.kParallax

            if show_mode == 0:
                # Use disparity from the camera
                self.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = c_bool(False)
                self.isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = c_bool(False)
                self.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = c_bool(False)
            else:
                # Use software matching disparity
                self.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = c_bool(False)
                self.isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = c_bool(True)
                self.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = c_bool(True)

        elif self.isc_raw_file_header.grab_mode == 2:
            self.isc_start_mode.isc_grab_start_mode.isc_grab_mode = IscGrabMode.kCorrect
            
            # Use software matching disparity
            self.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching = c_bool(True)
            self.isc_start_mode.isc_dataproc_start_mode.enabled_frame_decoder = c_bool(True)
            self.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter = c_bool(True)

        if self.isc_raw_file_header.color_mode == 1:
            self.isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode.kColorON
        else:        
            self.isc_start_mode.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode.kColorOFF

        ret = self.DplStart(self.isc_start_mode)
        # Error check
        if ret != 0:
            print("[ERROR][IscDplIf]start failed(0x{:08X})".format(ret))
            return -1

        return 0

    def stop(self):
        # stop grab
        print("[INFO][IscDplIf]stop")
        ret = self.DplStop()
        # Error check
        if ret != 0:
            print("[ERROR][IscDplIf]stop failed(0x{:08X})".format(ret))
            return -1

        return 0
    
    def get_datafrom_camera(self):
        # returns<- ret_base_image, base_image, ret_color_image, color_image, ret_depth_data, depth_data
        ret = self.DplGetCameraData(self.isc_image_info)

        ret_base_image = -1
        ret_color_image =  -1
        ret_depth_data = -1

        if ret == 0:
            frame_index = 0

            # base image
            p1_width = self.isc_image_info.frame_data[frame_index].p1.width
            p1_height = self.isc_image_info.frame_data[frame_index].p1.height

            if p1_width == 0 or p1_height == 0:
                return -1, self.error_image_base, -1, self.error_image_color, -1, self.error_data_depth
            else:
                ret_base_image = 0

            nd_array_base_image = np.ctypeslib.as_array(self.isc_image_info.frame_data[frame_index].p1.image, (p1_height, p1_width))
            np.reshape(nd_array_base_image, (p1_height, p1_width))
            nd_array_base_image_flip = cv2.flip(nd_array_base_image, -1)        

            # color
            color_width = self.isc_image_info.frame_data[frame_index].color.width
            color_height = self.isc_image_info.frame_data[frame_index].color.height

            if color_width == 0 or color_height == 0:
                ret_color_image = -1
                nd_array_color_image_flip = self.error_image_color
            else:
                ret_color_image = 0
                nd_array_color_image = np.ctypeslib.as_array(self.isc_image_info.frame_data[frame_index].color.image, (p1_height, p1_width, 3))
                np.reshape(nd_array_color_image, (p1_height, p1_width, 3))
                nd_array_color_image_flip = cv2.flip(nd_array_color_image, -1)        

            # depth
            depth_width = self.isc_image_info.frame_data[frame_index].depth.width
            depth_height = self.isc_image_info.frame_data[frame_index].depth.height

            if depth_width == 0 or depth_height == 0:
                ret_depth_data = -1
                nd_array_depth_data_flip = self.error_data_depth
            else:
                ret_depth_data = 0
                nd_array_depth_data = np.ctypeslib.as_array(self.isc_image_info.frame_data[frame_index].depth.image, (depth_height, depth_width))
                np.reshape(nd_array_depth_data, (depth_height, depth_width))
                nd_array_depth_data_flip = cv2.flip(nd_array_depth_data, -1)        

        else:
            return -1, self.error_image_base, -1, self.error_image_color, -1, self.error_data_depth
        
        return ret_base_image, nd_array_base_image_flip, ret_color_image, nd_array_color_image_flip, ret_depth_data, nd_array_depth_data_flip

    def get_datafrom_data_proc_module(self):

        # returns<- ret_base_image, base_image, ret_color_image, color_image, ret_depth_data, depth_data
        ret = self.DplGetDataProcModuleData(self.iscdataproc_result_data)

        ret_base_image = -1
        ret_color_image =  -1
        ret_depth_data = -1

        if ret == 0:
            frame_index = 0

            # base image
            p1_width = self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].p1.width
            p1_height = self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].p1.height

            if p1_width == 0 or p1_height == 0:
                return -1, self.error_image_base, -1, self.error_image_color, -1, self.error_data_depth
            else:
                ret_base_image = 0

            nd_array_base_image = np.ctypeslib.as_array(self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].p1.image, (p1_height, p1_width))
            np.reshape(nd_array_base_image, (p1_height, p1_width))
            nd_array_base_image_flip = cv2.flip(nd_array_base_image, -1)        

            # color
            color_width = self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].color.width
            color_height = self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].color.height

            if color_width == 0 or color_height == 0:
                ret_color_image = -1
                nd_array_color_image_flip = self.error_image_color
            else:
                ret_color_image = 0
                nd_array_color_image = np.ctypeslib.as_array(self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].color.image, (p1_height, p1_width, 3))
                np.reshape(nd_array_color_image, (p1_height, p1_width, 3))
                nd_array_color_image_flip = cv2.flip(nd_array_color_image, -1)        

            # depth
            depth_width = self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].depth.width
            depth_height = self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].depth.height

            if depth_width == 0 or depth_height == 0:
                ret_depth_data = -1
                nd_array_depth_data_flip = self.error_data_depth
            else:
                ret_depth_data = 0
                nd_array_depth_data = np.ctypeslib.as_array(self.iscdataproc_result_data.isc_image_info.frame_data[frame_index].depth.image, (depth_height, depth_width))
                np.reshape(nd_array_depth_data, (depth_height, depth_width))
                nd_array_depth_data_flip = cv2.flip(nd_array_depth_data, -1)        

        else:
            return -1, self.error_image_base, -1, self.error_image_color, -1, self.error_data_depth
        
        return ret_base_image, nd_array_base_image_flip, ret_color_image, nd_array_color_image_flip, ret_depth_data, nd_array_depth_data_flip
