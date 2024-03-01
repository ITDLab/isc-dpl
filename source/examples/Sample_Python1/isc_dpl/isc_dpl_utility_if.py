#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
from ctypes import *
from ctypes.util import find_library
from enum import Enum
from enum import IntEnum
import numpy as np

# /** @struct  DplIscUtilityParameter
#     *  @brief This is the initialize information
#     */
class DplIscUtilityParameter (Structure):
    _fields_ = [
                #// camera
                ("max_width", c_int),               #/**< Maximum width */
                ("max_height", c_int),              #/**< Maximum height */

                ("base_length", c_double),          #/**< camera base length */
                ("d_inf", c_double),                #/**< camera parameter */              
                ("bf", c_double),                   #/**< camera parameter */
                ("camera_angle", c_double),         #/**< camera parameter */

                #// display
                ("draw_outside_bounds", c_bool),    #/**< Whether to draw outside the specified range */

                ("min_distance", c_double),         #/**< Minimum display distance */
                ("max_distance", c_double),         #/**< Maximum display distance */
                ("step_distance", c_double),        #/**< Resolution of display distance */

                ("min_disparity", c_double),        #/**< Minimum parallax */
                ("max_disparity", c_double),        #/**< Maximum parallax */
                ("step_disparity", c_double),       #/**< Resolution of display */
    ]

class IscDplUtilityIf:
    def __init__(self):
        pass

    def __del__(self):
        pass

    def load_function(self):
        # load dpl dlls
        try:
            lib_isc_utility = WinDLL("./IscUtility.dll")
        except FileNotFoundError as err:
            print("[ERROR][IscDplUtilityIf] DLL load error:",err)
            return 1

        try:
            # load functions
            # /** @brief Initializes the class . Must be called at least once before streaming is started.
            #     @return 0, if successful.
            # */
            # ISCUTILITY_EXPORTS_API int DplIscUtilityInitialize(DplIscUtilityParameter* utility_parameter);
            self.DplIscUtilityInitialize = lib_isc_utility.DplIscUtilityInitialize
            self.DplIscUtilityInitialize.argtypes = [POINTER(DplIscUtilityParameter)]
            self.DplIscUtilityInitialize.restype = c_int

            # /** @brief Release resources and process termination.
            #     @return 0, if successful.
            # */
            # ISCUTILITY_EXPORTS_API int DplIscUtilityTerminate();
            self.DplIscUtilityTerminate = lib_isc_utility.DplIscUtilityTerminate
            self.DplIscUtilityTerminate.restype = c_int

            # /** @brief Create in the specified range.
            # @return none.
            # */
            # ISCUTILITY_EXPORTS_API void  DplIscUtilityRebuildDrawColorMap(const double min_distance, const double max_distance);
            self.DplIscUtilityRebuildDrawColorMap = lib_isc_utility.DplIscUtilityRebuildDrawColorMap
            self.DplIscUtilityRebuildDrawColorMap.argtypes = [c_double, c_double]
            self.DplIscUtilityRebuildDrawColorMap.restype = c_int

            # /** @brief Converts disparity data to distance image.
            # @return none.
            # */
            # ISCUTILITY_EXPORTS_API int Disparity2DistanceImage(const int width, const int height, float* disparity, unsigned char* bgr_image);
            self.Disparity2DistanceImage = lib_isc_utility.Disparity2DistanceImage
            self.Disparity2DistanceImage.argtypes = [c_int, c_int, POINTER(c_float), POINTER(c_ubyte)]
            self.Disparity2DistanceImage.restype = c_int

            # /** @brief Converts disparity data to images..
            # @return none.
            # */
            # ISCUTILITY_EXPORTS_API int Disparity2Image(const int width, const int height, float* disparity, unsigned char* bgr_image);
            self.Disparity2Image = lib_isc_utility.Disparity2Image
            self.Disparity2Image.argtypes = [c_int, c_int, POINTER(c_float), POINTER(c_ubyte)]
            self.Disparity2Image.restype = c_int

        except AttributeError as err:
            print("[ERROR][IscDplUtilityIf]AttributeError:", err)
            return 1

        return 0

    def initialize(self, width, height, base_length, d_inf, bf):
         # load function from dll
        ret = self.load_function()
        if ret != 0:
            print("[ERROR][IscDplUtilityIf]load_function failed{}".format(ret))
            return 1
        
        print("[INFO][IscDplUtilityIf]load_function was successful.")
       
        # Initialize
        self.dpliscutility_parameter = DplIscUtilityParameter()
        self.dpliscutility_parameter.max_width = width
        self.dpliscutility_parameter.max_height = height

        base_length_d = base_length
        d_inf_d = d_inf
        bf_d = bf
        self.dpliscutility_parameter.base_length = c_double(base_length_d)
        self.dpliscutility_parameter.d_inf = c_double(d_inf_d)
        self.dpliscutility_parameter.bf = c_double(bf_d)
        self.dpliscutility_parameter.camera_angle = c_double(0.0)

        self.dpliscutility_parameter.draw_outside_bounds = c_bool(True)

        self.dpliscutility_parameter.min_distance = c_double(0.5)
        self.dpliscutility_parameter.max_distance = c_double(20.0)
        self.dpliscutility_parameter.step_distance = c_double(0.01)

        self.dpliscutility_parameter.min_disparity = c_double(0.0)
        self.dpliscutility_parameter.max_disparity = c_double(255.0)
        self.dpliscutility_parameter.step_disparity =c_double(0.25)

        ret = self.DplIscUtilityInitialize(self.dpliscutility_parameter)

        if ret != 0:
            print("[ERROR][IscDplUtilityIf]initialize failed({})".format(ret))
            return 1

        print("[INFO][IscDplUtilityIf]initialize was successful.")

        # get work
        alloc_size = self.dpliscutility_parameter.max_width * self.dpliscutility_parameter.max_height * 3
        self.bgr_image = np.zeros(alloc_size, dtype=c_ubyte)

        return 0

    def terminate(self):

        ret = self.DplIscUtilityTerminate()
        if ret != 0:
            print("[ERROR][IscDplUtilityIf]terminate failed({})".format(ret))
            return 1
    
        print("[INFO][IscDplUtilityIf]terminate was successful.")

        return 0


    def rebuild_draw_color_map(self, min_distance, max_distance):

        ret = self.DplIscUtilityRebuildDrawColorMap(c_int(min_distance), c_int(max_distance))
        if ret != 0:
            print("[ERROR][IscDplUtilityIf]rebuild_draw_color_map failed({})".format(ret))
            return -1

        return ret
    
    def disparity_to_distance_image(self, width, height, disparity):

        ret = self.Disparity2DistanceImage(c_int(width), c_int(height), disparity.ctypes.data_as(POINTER(c_float)), self.bgr_image.ctypes.data_as(POINTER(c_ubyte)))
        if ret != 0:
            print("[ERROR][IscDplUtilityIf]Disparity2DistanceImage failed({})".format(ret))
            self.bgr_image[:] = 0
            bgr_image_2d = np.reshape(self.bgr_image, (height, width, 3))
            return 1, bgr_image_2d
        
        bgr_image_2d = np.reshape(self.bgr_image, (height, width, 3))
        return ret, bgr_image_2d
    
    def disparity_to_image(self, width, height, disparity):

        ret = self.Disparity2Image(c_int(width), c_int(height), disparity.ctypes.data_as(POINTER(c_float)), self.bgr_image.ctypes.data_as(POINTER(c_ubyte)))
        if ret != 0:
            print("[ERROR][IscDplUtilityIf]Disparity2Image failed({})".format(ret))
            self.bgr_image[:] = 0
            bgr_image_2d = np.reshape(self.bgr_image, (height, width, 3))
            return 1, bgr_image_2d
        
        bgr_image_2d = np.reshape(self.bgr_image, (height, width, 3))
        return ret, bgr_image_2d

