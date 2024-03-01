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

    arg_parser = argparse.ArgumentParser(
                    prog='dpl_test_main', 
                    usage='dpl_test_main file show[0:1]',
                    description='Here is a sample of how to use the DPL',
                    epilog='end of help.',
                    add_help=True,)
    
    arg_parser.add_argument("file", help="file name for play")
    arg_parser.add_argument("show", type=int, help="0:Show camera results 1:Show the results of data processing")

    args = arg_parser.parse_args()

    print("[INFO]Start the example of how to use DPL in Python")
    path = os.getcwd()
    print("[INFO]Current Folder:", path)
    print("")

    print("[INFO]file=", args.file)
    print("[INFO]show=", args.show)

    play_file_name = args.file
    show_mode = args.show

    # Initialize dpl
    print("[INFO]Initialize isc_dpl_if")
    isc_dpl_if = IscDplIf()
    ret = isc_dpl_if.initialize_file_mode(play_file_name)
    if ret != 0:
        print("[ERROR]isc_dpl_interface Initialize faild ret=0x{:08X}".format(ret))
        return -1
    
    print("[INFO]isc_dpl_if Initialize successful")

    # start grab
    ret = isc_dpl_if.start_file_mode(show_mode, play_file_name)
    if ret != 0:
        print("[ERROR]isc_dpl_if start faild ret=0x{:08X}".format(ret))
        return -1

    time.sleep(1)

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

    cv2.namedWindow("Base Image", cv2.WINDOW_AUTOSIZE)
    cv2.namedWindow("Color Image", cv2.WINDOW_AUTOSIZE)
    cv2.namedWindow("Depth Image", cv2.WINDOW_AUTOSIZE)

    # CVS保存　On/Off 保存時は非常におそくなるので注意すること
    is_save_csv_file = False

    # 画像保存の番号
    write_image_index = 0

    while True:
        # returns<- ret_base_image, base_image, ret_color_image, color_image, ret_depth_data, depth_data

        # camera disparity
        ret_base_image_camera, base_image_camera, ret_color_image_camera, color_image_camera, ret_depth_data_camera, depth_data_camera = isc_dpl_if.get_datafrom_camera()
        # data processing disparity
        ret_base_image_dp, base_image_dp, ret_color_image_dp, color_image_dp, ret_depth_data_dp, depth_data_dp = isc_dpl_if.get_datafrom_data_proc_module()

        # mode: 0 カメラ入力の視差を表示
        if show_mode == 0:
            if ret_base_image_camera == 0:
                # base image
                cv2.imshow("Base Image", base_image_camera)

                # color image
                if ret_color_image_camera == 0:
                    cv2.imshow("Color Image", color_image_camera)

                # make depth image & show it
                if ret_depth_data_camera == 0:
                    depth_height, depth_width = depth_data_camera.shape
                    ret_depth_image, depth_image = isc_dpl_utility_if.disparity_to_distance_image(depth_width, depth_height, depth_data_camera)
                    if ret_depth_image == 0:
                        cv2.imshow("Depth Image", depth_image)
                
        # mode: 1 カメラ入力の視差をフィルター（ノイズ除去、補間）したデータを取得、表示
        elif show_mode == 1:
            if ret_base_image_dp == 0:
                # base image
                cv2.imshow("Base Image", base_image_dp)
                # Here is an example when you want to save an image.
                #cvt_img = cv2.cvtColor(base_image_dp, cv2.COLOR_GRAY2BGR)
                #cv2.imwrite(f"./Base_Image_{write_image_index}.png", cvt_img) 

                # color image
                if ret_color_image_dp == 0:
                    cv2.imshow("Color Image", color_image_dp)
                    # Here is an example when you want to save an image.
                    #cv2.imwrite(f"./Color_Image_{write_image_index}.png", color_image_dp) 

                # make depth image & show it
                if ret_depth_data_dp == 0:
                    depth_height, depth_width = depth_data_dp.shape # depth_height: 720, depth_width: 1280
                    ret_depth_image, depth_image = isc_dpl_utility_if.disparity_to_distance_image(depth_width, depth_height, depth_data_dp)
                    if ret_depth_image == 0:
                        cv2.imshow("Depth Image", depth_image)
                        # Here is an example when you want to save an image.
                        #cv2.imwrite(f"./Depth_Image_{write_image_index}.png", depth_image) 
                    
                    # Save data as CSV
                    if is_save_csv_file:
                        np.savetxt(f"./Disparity_{write_image_index}.csv", depth_data_dp, delimiter=',')

                        # Converts disparity to distance data and saves it to a CSV file.
                        image_size = depth_width * depth_height
                        depth_m_data = np.zeros(image_size)

                        for y in range(depth_height):
                            for x in range(depth_width):
                                if (depth_data_dp[y][x] > d_inf):
                                    z = bf / (depth_data_dp[y][x] - d_inf)
                                    depth_m_data[y * depth_width + x] = z

                        depth_m_data_wh = np.reshape(depth_m_data, (depth_height, depth_width))
                        np.savetxt(f"./Distance_{write_image_index}.csv", depth_m_data_wh, delimiter=',')

                write_image_index += 1

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

