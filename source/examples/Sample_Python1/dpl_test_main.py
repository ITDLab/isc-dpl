#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import time
import argparse
import numpy as np
import cv2
from ctypes import *
from isc_dpl.isc_dpl_if import IscDplIf
from isc_dpl.isc_dpl_utility_if import IscDplUtilityIf 

def main():

    print("[INFO]Start the example of how to use DPL in Python")
    path = os.getcwd()
    print("[INFO]Current Folder:", path)
    print("")

    arg_parser = argparse.ArgumentParser(
                    prog='dpl_test_main', 
                    usage='dpl_test_main camera[0:1] mode[0:1]',
                    description='Here is a sample of how to use the DPL',
                    epilog='end of help.',
                    add_help=True,)
    
    arg_parser.add_argument("camera", type=int, help="0:VM 1:XC")
    arg_parser.add_argument("mode", type=int, help="0:Displays camera results 1:Displays the results of data processing")

    args = arg_parser.parse_args()
    print("[INFO]camera=", args.camera)
    print("[INFO]run mode=", args.mode)

    camera = args.camera
    if camera == 0:
        print("[INFO]camera=VM")
    elif camera == 1:
        print("[INFO]camera=XC")
    else:
        print("[ERROR]Invalid camera specified.")
        return -1

    run_mode = args.mode
    if run_mode == 0:
        print("[INFO]run mode=camera")
    elif run_mode == 1:
        print("[INFO]run mode=dpl")
    else:
        print("[ERROR]Invalid mode specified.")
        return -1

    # Initialize dpl
    print("[INFO]Initialize isc_dpl_if")
    isc_dpl_if = IscDplIf()
    ret = isc_dpl_if.initialize(camera)
    if ret != 0:
        print("[ERROR]isc_dpl_interface Initialize faild ret=0x{:08X}".format(ret))
        return -1
    
    print("[INFO]isc_dpl_if Initialize successful")

    # Initialize Utility
    print("[INFO]Initialize isc_dpl_utility_if")
    width = isc_dpl_if.get_with()
    height = isc_dpl_if.get_height()
    base_length = isc_dpl_if.get_base_length()
    d_inf = isc_dpl_if.get_dinf()
    bf = isc_dpl_if.get_bf()

    isc_dpl_utility_if = IscDplUtilityIf()
    ret = isc_dpl_utility_if.initialize(width, height, base_length, d_inf, bf)
    if ret != 0:
        print("[ERROR]isc_dpl_utility_if Initialize faild ret={}".format(ret))
        return -1
    
    print("[INFO]isc_dpl_utility_if Initialize successful")

     # start grab
    ret = isc_dpl_if.start(run_mode)
    if ret != 0:
        print("[ERROR]isc_dpl_if start faild ret=0x{:08X}".format(ret))
        return -1

    time.sleep(1)

    cv2.namedWindow("Base Image", cv2.WINDOW_AUTOSIZE)
    cv2.namedWindow("Color Image", cv2.WINDOW_AUTOSIZE)
    cv2.namedWindow("Depth Image", cv2.WINDOW_AUTOSIZE)

    while True:
        # returns<- ret_base_image, base_image, ret_color_image, color_image, ret_depth_data, depth_data
        if run_mode == 0:
            # camera disparity
            ret_base_image, base_image, ret_color_image, color_image, ret_depth_data, depth_data = isc_dpl_if.get_datafrom_camera()
        elif run_mode == 1:
            # data processing disparity
            ret_base_image, base_image, ret_color_image, color_image, ret_depth_data, depth_data = isc_dpl_if.get_datafrom_data_proc_module()
        
        if ret_base_image == 0:
            # base image
            cv2.imshow("Base Image", base_image)

            # color image
            if ret_color_image == 0:
                cv2.imshow("Color Image", color_image)

            # make depth image & show it
            if ret_depth_data == 0:
                depth_height, depth_width = depth_data.shape
                ret_depth_image, depth_image = isc_dpl_utility_if.disparity_to_distance_image(depth_width, depth_height, depth_data)
                if ret_depth_image == 0:
                    cv2.imshow("Depth Image", depth_image)

        key = cv2.waitKey(10) & 0xFF
        # break by ESC key
        if key == 27:
            break

    # stop grab
    ret = isc_dpl_if.stop()
    if ret != 0:
        print("[ERROR]isc_dpl_if stop faild ret=0x{:08X}".format(ret))
        return -1

    # ended
    print("[INFO]main procedure ended")

    cv2.destroyAllWindows()

    ret = isc_dpl_utility_if.terminate()
    if ret != 0:
        print("[ERROR]isc_dpl_utility_if terminate ret={}".format(ret))
        return -1
   
    ret = isc_dpl_if.terminate()
    if ret != 0:
        print("[ERROR]isc_dpl_if terminate ret=0x{:08X}".format(ret))
        return -1

    print("[INFO]terminate successful")
    print("[INFO]Exit the application")

    return 0

if __name__ == "__main__":
    sys.exit(main())

