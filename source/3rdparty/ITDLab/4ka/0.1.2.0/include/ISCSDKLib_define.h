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

// ERROR CODE DEFINE

// OK
#define ISC_OK					0

// Error
#define ERR_READ_DATA			-1
#define ERR_WRITE_DATA			-2
#define ERR_WAIT_TIMEOUT		-3
#define ERR_OBJECT_CREATED		-4
#define ERR_USB_OPEN			-5
#define ERR_USB_SET_CONFIG		-6
#define ERR_CAMERA_SET_CONFIG	-7
#define ERR_REGISTER_SET		-8
#define ERR_FPGA_MODE_ERROR		-11
#define ERR_TABLE_FILE_OPEN		-13
#define ERR_GETIMAGE			-16
#define ERR_INVALID_VALUE		-17
#define ERR_NO_CAPTURE_MODE		-18
#define ERR_REQUEST_NOT_ACCEPTED			-20

#define ERR_USB_ERR							-21
#define ERR_USB_ALREADY_OPEN				-22

// Warning or error
#define ERR_USB_NO_IMAGE					-102
#define ERR_NO_VALID_IMAGES_CALIBRATING		-119

#define ERR_FPGA_ERROR						-200
#define ERR_AUTOCALIB_OUTRANGE				-203

#define ERR_AUTOCALIB_BAD_IMAGE				-205
#define ERR_AUTOCALIB_FAIL_AUTOCALIB		-206
#define ERR_AUTOCALIB_POOR_IMAGEINFO		-207
#define ERR_AUTOCALIB_POOR_IMAGEINFO_BAD_IMAGE		-208
#define ERR_AUTOCALIB_POOR_IMAGEINFO_OUTRANGE		-209
#define ERR_AUTOCALIB_POOR_IMAGEINFO_FAIL_AUTOCALIB	-210


#define AUTOCALIBRATION_STATUS_BIT_AUTO_ON				0x00000002
#define AUTOCALIBRATION_STATUS_BIT_MANUAL_RUNNING		0x00000001

namespace ISCSDKLib {
	
/**
 * 画像取得モードの指定
 */
enum ISC_GrabMode {
	/// 視差モード(補正後画像+視差画像)
	ParallaxImage = 1,
	/// 補正後画像モード
	CorrectedImage,
	/// 補正前画像モード(原画像)
	OriginalImage,
	/// 補正前Bayer画像モード(原画像)
	OriginalBayerImage,
	/// 補正前Bayer画像モード(原画像)(Left)　※ユーザーへは非公開とする
	OriginalBayerImageLeft
};

/**
 * 自動調整モードの指定
 */
enum ISC_CalibrationMode {
	/// 停止
	Off = 0,
	/// 自動調整
	AutoCalibration,
	/// 強制調整
	ForceCalibration
};

/**
 * 自動露光調整モードの指定
 */
enum ISC_ShutterMode {
	/// マニュアルモード
	Manual = 0,
	/// シングルシャッターモード
	SingleShutter,
	/// ダブルシャッターモード
	DoubleShutter,
	/// システムデフォルト
	SystemDefault,
};

/**
 * 画像取得用の構造体
 */
struct ISC_ImageInfo {
	int frameNo;		// フレームの番号
	int gain;			// フレームのGain値
	int exposure;		// フレームのExposure値
	enum ISC_GrabMode grab;	// 取り込みモード
	enum ISC_ShutterMode shutter;	// 露光調整モード
	int p1_width;		// フレームのp1画像幅
	int p1_height;		// フレームのp1画像高さ
	unsigned char* p1;	// 基準側画像/カラー基準画像/カラー比較画像
	int p2_width;		// フレームのp2画像幅
	int p2_height;		// フレームのp2画像高さ
	unsigned char* p2;	// 視差画像/補正後比較画像/補正前比較画像
};

/**
 * RAW画像取得用の構造体
 */
struct ISC_RawImageInfo {
	int frameNo;		// フレームの番号
	int gain;			// フレームのGain値
	int exposure;		// フレームのExposure値
	enum ISC_GrabMode grab;	// 取り込みモード
	enum ISC_ShutterMode shutter;	// 露光調整モード
	int p1_width;		// フレームのp1画像幅
	int p1_height;		// フレームのp1画像高さ
	unsigned short* p1;	// 基準側画像
	int p2_width;		// フレームのp2画像幅
	int p2_height;		// フレームのp2画像高さ
	unsigned short* p2;	// 視差画像/補正後比較画像/補正前比較画像
};

/**
 * カメラパラメータ定義
 */
struct CameraParamInfo
{
	float	fD_INF;						// 無限遠値
	unsigned int nD_INF;				// 未使用
	float fBF;							// BF値
	float fBaseLength;					// 基線長
	float fViewAngle;					// 視野角
	unsigned int nImageWidth;			// センサー画像幅
	unsigned int nImageHeight;			// センサー画像高さ
	unsigned long long nProductNumber;	// 製品番号
	char szSerialNumber[16];			// 製品シリアル番号(8文字)
	unsigned int nFPGA_version_major;	// 製品FPGAメジャーバージョン
	unsigned int nFPGA_version_minor;	// 製品FPGAマイナーバージョン
	unsigned int nDistanceHistValue;	// 未使用
	unsigned int  nParallaxThreshold;	// 未使用
};

}
