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

	// 公開関数
	ISCSDKLIB_API int OpenISC();
	ISCSDKLIB_API int CloseISC();

	ISCSDKLIB_API int StartGrab(int nMode);
	ISCSDKLIB_API int StopGrab();

	ISCSDKLIB_API int GetImage(unsigned char* pBuffer1, unsigned char* pBuffer2,int nSkip);
	ISCSDKLIB_API int GetDepthInfo(float* pBuffer);

	ISCSDKLIB_API int SetRGBEnabled(int nMode);
	ISCSDKLIB_API int GetYUVImage(unsigned char* pBuffer, int nSkip);
	ISCSDKLIB_API int ConvertYUVToRGB(unsigned char *yuv, unsigned char *prgbimage, int dwSize);
	ISCSDKLIB_API int ApplyAutoWhiteBalance(unsigned char *prgbimage, unsigned char *prgbimageF);
	ISCSDKLIB_API int CorrectRGBImage(unsigned char *prgbimageF, unsigned char *AdjustBuffer);

	ISCSDKLIB_API int GetCameraParamInfo(CameraParamInfo* pParam);
	ISCSDKLIB_API int GetImageSize(unsigned int* pnWidth, unsigned int* pnHeight);

	ISCSDKLIB_API int SetAutoCalibration(int nMode);
	ISCSDKLIB_API int GetAutoCalibration(int* nMode);

	ISCSDKLIB_API int SetShutterControlMode(bool nMode);
	ISCSDKLIB_API int GetShutterControlMode(bool* nMode);

	ISCSDKLIB_API int SetExposureValue(unsigned int nValue);
	ISCSDKLIB_API int GetExposureValue(unsigned int* pnValue);

	ISCSDKLIB_API int SetGainValue(unsigned int nValue);
	ISCSDKLIB_API int GetGainValue(unsigned int* pnValue);

	ISCSDKLIB_API int SetMeasArea(int nTop, int nLeft, int nRight, int nBottom);
	ISCSDKLIB_API int GetMeasArea(int* nTop, int* nLeft, int* nRight, int* nBottom);

	ISCSDKLIB_API int SetNoiseFilter(unsigned int nValue);
	ISCSDKLIB_API int GetNoiseFilter(unsigned int* pnValue);

	ISCSDKLIB_API int SetShutterControlModeEx(int nMode);
	ISCSDKLIB_API int GetShutterControlModeEx(int* pnMode);

	ISCSDKLIB_API int SetMeasAreaEx(int mode, int nTop, int nLeft, int nRight, int nBottom, int nTop_Left, int nTop_Right, int nBottom_Left, int nBottom_Right);
	ISCSDKLIB_API int GetMeasAreaEx(int* mode, int* nTop, int* nLeft, int* nRight, int* nBottom, int* nTop_Left, int* nTop_Right, int* nBottom_Left, int* nBottom_Right);

	// 公開・非公開　審議中 -> 非公開
	ISCSDKLIB_API int SetMedianTarget(unsigned int nValue);
	ISCSDKLIB_API int GetMedianTarget(unsigned int* nValue);

	// 以下は、文書化されない関数です
	// 内部の保守のために存在します
	// 使わないでください

	// 非公開関数　以前のSDKとの互換のため
	ISCSDKLIB_API int SetDoubleShutterControlMode(int nMode);
	ISCSDKLIB_API int GetDoubleShutterControlMode(int* nMode);

	// 非公開関数　互換用のため非推奨
	ISCSDKLIB_API int Set_RGB_Enabled(int nMode);
	ISCSDKLIB_API int Get_YUV_Image(unsigned char* pBuffer, int nSkip);
	ISCSDKLIB_API void YUV_TO_RGB(unsigned char *yuv, unsigned char *prgbimage, int dwSize);
	ISCSDKLIB_API void RGB_TO_AWB(unsigned char *prgbimage, unsigned char *prgbimageF);
	ISCSDKLIB_API void Correct_RGB_Image(unsigned char *prgbimageF, unsigned char *AdjustBuffer);

	// 非公開関数
	ISCSDKLIB_API int SetRectTable(void);

	ISCSDKLIB_API int GetFullFrameInfo(unsigned char* pBuffer);
	ISCSDKLIB_API int GetFullFrameInfo2(unsigned char* pBuffer);

	// 1frame分の受信データを取得します
	struct RawSrcData {
		unsigned char* image;
		int startGrabMode;		// 2:pallax+image 3:correct image 4:before correctimage
		int type;				// 0:Mono 1:Color

		int index;				// Frame Index
		int status;				// Header status
		int errorCode;
		int gain;
		int exposure;
		float d_inf;
		float bf;
	};
	ISCSDKLIB_API int GetFullFrameInfo4(RawSrcData* rawSrcDataCur, RawSrcData* rawSrcDataPrev);

	ISCSDKLIB_API int SetCameraRegData(unsigned char* pwBuf, unsigned int wSize);
	ISCSDKLIB_API int GetCameraRegData(unsigned char* pwBuf, unsigned char* prBuf, unsigned int wSize, unsigned int rSize);
	ISCSDKLIB_API int GetRegInfo(BYTE* pBuff);

	ISCSDKLIB_API int SaveMemoryData(char* pSaveFolder);

	ISCSDKLIB_API int GetImageEx(unsigned char* pBuffer1, unsigned char* pBuffer2, int nSkip, int nWaitTime = 100);
	ISCSDKLIB_API int GetImageWithIndex(int* pIndex1, unsigned char* pBuffer1, int* pExposure1, int* pGain1,
										int* pIndex2, unsigned char* pBuffer2, int* pExposure2, int* pGain2, int nSkip);

	ISCSDKLIB_API int GetImageWithIndexEx(	int* pIndex1, unsigned char* pBuffer1, int* pExposure1, int* pGain1,
											int* pIndex2, unsigned char* pBuffer2, int* pExposure2, int* pGain2, int nSkip, int nWaitTime = 100);

	ISCSDKLIB_API int GetYUVImageEx(unsigned char* pBuffer, int nSkip, unsigned long nWaitTime = 100);
	ISCSDKLIB_API int GetYUVImageWidthIndex(int* pIndex1, unsigned char* pBuffer, int nSkip, unsigned long signalWaitTime = 100);
		
	ISCSDKLIB_API int GetDepthInfoWithIndex(int* index, float* pBuffer);

	// ステータス
	struct ISCLibStaus {
		int recieveErrorFrameCount;
	};

	ISCSDKLIB_API int GetISCLibStatus(ISCLibStaus* pISCLibStaus);

}
