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
    @file isc_dpl_error_def.h
    @brief Error codes for dpl lib.
*/

#ifndef ISC_DPL_ERROR_DEF_H
#define ISC_DPL_ERROR_DEF_H

/*
 * TODO: Define a more elaborate numbering scheme
 */

/**
* @defgroup errorcodes Error&nbsp;Codes
* @brief Error codes returned by any of the \ API functions.
*
* These error codes can be returned from any of the \ API functions.\n
* To get a textual representation for the error code you can call DpcApiGetLastErrorMessage().\n
* To get a detailed error description you can call DpcApiGetLastErrorDetail(). The string returned will contain the specific reason why the call has failed.
* @note If a function returns an error, you must call DpcApiGetLastErrorMessage() and/or DpcApiGetLastErrorDetail() directly after you received the error.
* Calling other functions will either reset or overwrite the error text.
*
* @{
*/

#define DPL_RESULT int

#define DPC_E_OK                                ((DPL_RESULT) 0)           /**< Operation completed successfully. */

#define ISCDPL_E_FAIL                           ((DPL_RESULT) 0x81000001)  /**< Unspecified error occurred. */
#define ISCDPL_E_OPVERLAPED_OPERATION           ((DPL_RESULT) 0x81000002)  /**< The processing overlaps. */
#define ISCDPL_E_INVALID_HANDLE                 ((DPL_RESULT) 0x81000003)  /**< The handle is invalid. */
#define ISCDPL_E_INVALID_PARAMETER              ((DPL_RESULT) 0x81000004)  /**< The parameter is invalid. */
#define ISCDPL_E_INCORRECT_MODE                 ((DPL_RESULT) 0x81000005)  /**< The current mode of operation is an incorrect mode. */

#define CAMCONTROL_E_FAIL                       ((DPL_RESULT) 0xC2000001)  /**< Unspecified error occurred. */
#define CAMCONTROL_E_OPVERLAPED_OPERATION       ((DPL_RESULT) 0xC2000002)  /**< The processing overlaps. */
#define CAMCONTROL_E_INVALID_DEVICEHANDLE       ((DPL_RESULT) 0xC2000003)  /**< The handle is invalid. */
#define CAMCONTROL_E_OPEN_DEVICE_FAILED         ((DPL_RESULT) 0xC2000004)  /**< OpenISC faild. */
#define CAMCONTROL_E_CLOSE_DEVICE_FAILED        ((DPL_RESULT) 0xC2000005)  /**< CloseISC faild. */
#define CAMCONTROL_E_INVALID_REQUEST            ((DPL_RESULT) 0xC2000006)  /**< The request is invalid. */
#define CAMCONTROL_E_INVALID_PARAMETER          ((DPL_RESULT) 0xC2000007)  /**< The parameter is invalid. */
#define CAMCONTROL_E_NO_IMAGE                   ((DPL_RESULT) 0xC2000008)  /**< No image. */
#define CAMCONTROL_E_FTDI_ERROR                 ((DPL_RESULT) 0xC2000009)  /**< USB error. */
#define CAMCONTROL_E_CAMERA_UNDER_CARIBRATION   ((DPL_RESULT) 0xC200000A)  /**< Camera is in calibrating operation. */
#define CAMCONTROL_E_GRAB_START_FAILED          ((DPL_RESULT) 0xC200000B)  /**< Failed. */
#define CAMCONTROL_E_GRAB_STOP_FAILED           ((DPL_RESULT) 0xC200000C)  /**< Failed. */
#define CAMCONTROL_E_GET_IMAGE_FAILED           ((DPL_RESULT) 0xC200000D)  /**< Failed. */
#define CAMCONTROL_E_GET_DEPTH_FAILED           ((DPL_RESULT) 0xC200000E)  /**< Failed. */
#define CAMCONTROL_E_GET_FULL_FRAME_FAILED      ((DPL_RESULT) 0xC200000F)  /**< Failed. */
#define CAMCONTROL_E_SET_FETURE_FAILED          ((DPL_RESULT) 0xC2000010)  /**< Failed. */
#define CAMCONTROL_E_GET_FETURE_FAILED          ((DPL_RESULT) 0xC2000011)  /**< Failed. */
#define CAMCONTROL_E_LOAD_DLL_FAILED            ((DPL_RESULT) 0xC2000012)  /**< Load DLL function failed. */
#define CAMCONTROL_E_SART_SAVE_FAILED           ((DPL_RESULT) 0xC2000020)  /**< Unable to start. */
#define CAMCONTROL_E_INVALID_SAVE_FOLDER        ((DPL_RESULT) 0xC2000021)  /**< The specified save folder is invalid. */
#define CAMCONTROL_E_CREATE_SAVE_FILE           ((DPL_RESULT) 0xC2000022)  /**< Failed to create write file. */
#define CAMCONTROL_E_WRITE_FAILED               ((DPL_RESULT) 0xC2000023)  /**< I failed to write. */
#define CAMCONTROL_E_OPEN_READ_FILE_FAILED      ((DPL_RESULT) 0xC2000030)  /**< can't open read file. */
#define CAMCONTROL_E_READ_FILE_FAILED           ((DPL_RESULT) 0xC2000031)  /**< can't read from file. */
#define CAMCONTROL_E_READ_FILE_FAILED_RETRY     ((DPL_RESULT) 0xC2000032)  /**< can't read from file and request retry. */
#define CAMCONTROL_E_READ_CAMERA_MODEL          ((DPL_RESULT) 0xC2000033)  /**< camera model not match. */
#define CAMCONTROL_E_NOT_ENOUGH_FREE_SPACE      ((DPL_RESULT) 0xC2000034)  /**< Not enough free space. */

#define DPCCONTROL_E_FAIL                       ((DPL_RESULT) 0xB2000001)  /**< Unspecified error occurred. */
#define DPCCONTROL_E_OPVERLAPED_OPERATION       ((DPL_RESULT) 0xB2000002)  /**< The processing overlaps. */
#define DPCCONTROL_E_INVALID_DEVICEHANDLE       ((DPL_RESULT) 0xB2000003)  /**< The handle is invalid. */
#define DPCCONTROL_E_INVALID_PARAMETER          ((DPL_RESULT) 0xB2000004)  /**< The handle is invalid. */

#define DPCPROCESS_E_FAIL                       ((DPL_RESULT) 0xA2000001)  /**< Unspecified error occurred. */
#define DPCPROCESS_E_OPVERLAPED_OPERATION       ((DPL_RESULT) 0xA2000002)  /**< The processing overlaps. */
#define DPCPROCESS_E_INVALID_DEVICEHANDLE       ((DPL_RESULT) 0xA2000003)  /**< The handle is invalid. */
#define DPCPROCESS_E_INVALID_PARAMETER          ((DPL_RESULT) 0xA2000004)  /**< The parameter is invalid. */
#define DPCPROCESS_E_FILE_NOT_FOUND             ((DPL_RESULT) 0xA2000005)  /**< The parameter is invalid. */
#define DPCPROCESS_E_INVALID_MODE               ((DPL_RESULT) 0xA2000006)  /**< The parameter is invalid. */
#define DPCPROCESS_E_FILTER_THROUGH             ((DPL_RESULT) 0xA2000007)  /**< The parameter is invalid. */

/**
* @}
*/

#endif /* ISC_DPL_ERROR_DEF_H */
