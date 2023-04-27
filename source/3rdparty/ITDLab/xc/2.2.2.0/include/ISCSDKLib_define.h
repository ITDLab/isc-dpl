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
#pragma once

#ifdef ISCSDKLIB_EXPORTS
#define ISCSDKLIB_API __declspec(dllexport)
#else
#define ISCSDKLIB_API __declspec(dllimport)
#endif

// ERROR CODE DEFINE

// OK
#define ISC_OK					0

#define ERR_READ_DATA			-1
#define ERR_WRITE_DATA			-2
#define ERR_WAIT_TIMEOUT		-3
#define ERR_OBJECT_CREATED		-4
#define ERR_USB_OPEN			-5
#define ERR_USB_SET_CONFIG		-6
#define ERR_CAMERA_SET_CONFIG	-7
#define ERR_REGISTER_SET		-8
#define ERR_THREAD_RUN			-9
#define ERR_RESET_ERROR			-10
#define ERR_FPGA_MODE_ERROR		-11
#define ERR_GRAB_MODE_ERROR		-12
#define ERR_TABLE_FILE_OPEN		-13
#define ERR_MODSET_ERROR		-14
#define ERR_CALIBRATION_TABLE	-15
#define ERR_GETIMAGE			-16
#define ERR_INVALID_VALUE		-17
#define ERR_NO_CAPTURE_MODE		-18
#define ERR_NO_VALID_IMAGES_CALIBRATING		-19
#define ERR_REQUEST_NOT_ACCEPTED			-20
#define ERR_USB_ERR							-100
#define ERR_USB_ALREADY_OPEN				-101
#define ERR_USB_NO_IMAGE					-102

#define ERR_FPGA_ERROR						-200
#define ERR_AUTOCALIB_GIVEUP_WARN			-201
#define ERR_AUTOCALIB_GIVEUP_ERROR			-202
#define ERR_AUTOCALIB_OUTRANGE				-203
#define ERR_IMAGEINPUT_IMAGEERROR			-204

#define ERR_AUTOCALIB_BAD_IMAGE				-205
#define ERR_AUTOCALIB_FAIL_AUTOCALIB		-206
#define ERR_AUTOCALIB_POOR_IMAGEINFO		-207
#define ERR_AUTOCALIB_POOR_IMAGEINFO_BAD_IMAGE		-208
#define ERR_AUTOCALIB_POOR_IMAGEINFO_OUTRANGE		-209
#define ERR_AUTOCALIB_POOR_IMAGEINFO_FAIL_AUTOCALIB	-210

// Shutter Control Mode Define
#define SHUTTER_CONTROLMODE_MANUAL						0
#define SHUTTER_CONTROLMODE_AUTO						1
#define SHUTTER_CONTROLMODE_DOUBLE						2
#define SHUTTER_CONTROLMODE_DOUBLE_INDEPENDENT_OUT		3
#define SHUTTER_CONTROLMODE_SYSTEM_DEFAULT				4

// Auto Calibration
#define AUTOCALIBRATION_COMMAND_STOP					0
#define AUTOCALIBRATION_COMMAND_AUTO_ON					1
#define AUTOCALIBRATION_COMMAND_MANUAL_START			2
#define AUTOCALIBRATION_STATUS_BIT_AUTO_ON				0x00000002
#define AUTOCALIBRATION_STATUS_BIT_MANUAL_RUNNING		0x00000001

extern "C" {

	struct CameraParamInfo
	{
		float	fD_INF;
		unsigned int nD_INF;
		float fBF;
		float fBaseLength;
		float fViewAngle;
		unsigned int nImageWidth;
		unsigned int nImageHeight;
		unsigned int nProductNumber;
		unsigned int nSerialNumber;
		unsigned int nFPGA_version;
		unsigned int nDistanceHistValue;
		unsigned int  nParallaxThreshold;
	};

}


