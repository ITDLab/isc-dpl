#pragma once

#ifdef ISCSDKLIB_EXPORTS
#define ISCSDKLIB_API __declspec(dllexport)
#else
#define ISCSDKLIB_API __declspec(dllimport)
#endif

// ERROR CODE DEFINE

// OK
#define ISC_OK								0

// FTDI 
#define FT_INVALID_HANDLE					1
#define FT_DEVICE_NOT_FOUND					2
#define FT_DEVICE_NOT_OPENED				3
#define FT_IO_ERROR							4
#define FT_INSUFFICIENT_RESOURCES			5
#define FT_INVALID_PARAMETER				6
#define FT_INVALID_BAUD_RATE				7
#define FT_DEVICE_NOT_OPENED_FOR_ERASE		8
#define FT_DEVICE_NOT_OPENED_FOR_WRITE		9
#define FT_FAILED_TO_WRITE_DEVICE			10
#define FT_EEPROM_READ_FAILED				11
#define FT_EEPROM_WRITE_FAILED				12
#define FT_EEPROM_ERASE_FAILED				13
#define FT_EEPROM_NOT_PRESENT				14
#define FT_EEPROM_NOT_PROGRAMMED			15
#define FT_INVALID_ARGS						16
#define FT_NOT_SUPPORTED					17
#define FT_OTHER_ERROR						18
#define FT_DEVICE_LIST_NOT_READY			19

// ISC
#define ERR_READ_DATA						-1
#define ERR_WRITE_DATA						-2
#define ERR_WAIT_TIMEOUT					-3
#define ERR_OBJECT_CREATED					-4
#define ERR_USB_OPEN						-5
#define ERR_USB_SET_CONFIG					-6
#define ERR_CAMERA_SET_CONFIG				-7
#define ERR_REGISTER_SET					-8
#define ERR_THREAD_RUN						-9
#define ERR_RESET_ERROR						-10
#define ERR_FPGA_MODE_ERROR					-11
#define ERR_GRAB_MODE_ERROR					-12
#define ERR_TABLE_FILE_OPEN					-13
#define ERR_MODSET_ERROR					-14
#define ERR_CALIBRATION_TABLE				-15
#define ERR_GETIMAGE						-16
#define ERR_INVALID_VALUE					-17
#define ERR_NO_CAPTURE_MODE					-18
#define ERR_NO_VALID_IMAGES_CALIBRATING		-19
#define ERR_REQUEST_NOT_ACCEPTED			-20
#define ERR_UNSUPPORTED_FPGA_VERSION		-21
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

// Function
extern "C" {

	struct CameraParamInfo
	{
		float	fD_INF;
		unsigned int nD_INF;
		float fBF;
		float fBaseLength;
		float fdZ;
		float fViewAngle;
		unsigned int nImageWidth;
		unsigned int nImageHeight;
		unsigned int nProductNumber;
		unsigned int nProductNumber2;
		char nSerialNumber[16];
		unsigned int nFPGA_version_major;
		unsigned int nFPGA_version_minor;
		unsigned int nDistanceHistValue;
		unsigned int nParallaxThreshold;
	};

	struct ShutterControlParamInfo {
		float fBlownOutHighLightsUpperLimit;
		float fBlownOutHighLightsLowerLimit;
	};

	// 外部公開関数
	ISCSDKLIB_API int OpenISC();
	ISCSDKLIB_API int CloseISC();

	ISCSDKLIB_API int SetISCRunMode(int nMode);
	ISCSDKLIB_API int GetISCRunMode(int* pMode);

	ISCSDKLIB_API int StartGrab(int nMode);
	ISCSDKLIB_API int StopGrab();

	ISCSDKLIB_API int GetImage(unsigned char* pBuffer1, unsigned char* pBuffer2,int nSkip);
	ISCSDKLIB_API int GetImageEx(unsigned char* pBuffer1, unsigned char* pBuffer2, int nSkip, int nWaitTime = 100);
	ISCSDKLIB_API int GetDepthInfo(float* pBuffer);

	ISCSDKLIB_API int GetCameraParamInfo(CameraParamInfo* pParam);
	ISCSDKLIB_API int GetImageSize(unsigned int* pnWidth, unsigned int* pnHeight);

	ISCSDKLIB_API int SetAutoCalibration(int nMode);
	ISCSDKLIB_API int GetAutoCalibration(int* nMode);

	ISCSDKLIB_API int SetAutomaticDisparityAdjustment(int nMode);
	ISCSDKLIB_API int GetAutomaticDisparityAdjustment(int* pnMode);

	ISCSDKLIB_API int SetForceDisparityAdjustment(int nMode);
	ISCSDKLIB_API int GetForceDisparityAdjustment(int* pnMode);

	ISCSDKLIB_API int SetShutterControlMode(int nMode);
	ISCSDKLIB_API int GetShutterControlMode(int* pnMode);

	ISCSDKLIB_API int SetShutterControlParamInfo(ShutterControlParamInfo* param);
	ISCSDKLIB_API int GetShutterControlParamInfo(ShutterControlParamInfo* param);

	ISCSDKLIB_API int SetExposureValue(unsigned int nValue);
	ISCSDKLIB_API int GetExposureValue(unsigned int* pnValue );

	ISCSDKLIB_API int SetFineExposureValue(unsigned int nValue);
	ISCSDKLIB_API int GetFineExposureValue(unsigned int* pnValue);

	ISCSDKLIB_API int SetGainValue(unsigned int nValue);
	ISCSDKLIB_API int GetGainValue(unsigned int* pnValue);

	ISCSDKLIB_API int SetHiResolutionMode(int nValue);
	ISCSDKLIB_API int GetHiResolutionMode(int* pnMode);

	ISCSDKLIB_API int SetNoiseFilter(unsigned int nDCDX);
	ISCSDKLIB_API int GetNoiseFilter(unsigned int* nDCDX);

	ISCSDKLIB_API int SetMeasArea(int mode, int nTop, int nLeft, int nRight, int nBottom, int nTop_Left, int nTop_Right, int nBottom_Left, int nBottom_Right);
	ISCSDKLIB_API int GetMeasArea(int* mode, int* nTop, int* nLeft, int* nRight, int* nBottom, int* nTop_Left, int* nTop_Right, int* nBottom_Left, int* nBottom_Right);

	ISCSDKLIB_API int SetCameraFPSMode(int nMode);
	ISCSDKLIB_API int GetCameraFPSMode(int* pnMode, int* pnNominalFPS);


	// Undocumented functions
	// Only available to developers inside the company
	ISCSDKLIB_API int SetCameraRegData(unsigned char* pwBuf, unsigned int wSize);
	ISCSDKLIB_API int GetCameraRegData(unsigned char* pwBuf, unsigned char* prBuf, unsigned int wSize, unsigned int rSize);

	// 非公開関数
	struct HDRParamInfo {
		unsigned int nCoarseShutterWidth1;
		unsigned int nCoarseShutterWidth2;
		unsigned int nT2Ratio;
		unsigned int nT3Ratio;
		int nExposureKneePointAutoAdjustEnable;
		int nSingleKneeEnable;
		unsigned int nV1VoltageLevel;
		unsigned int nV2VoltageLevel;
		unsigned int nV3VoltageLevel;
		unsigned int nV4VoltageLevel;
		unsigned int nFineShutterWidth1;
		unsigned int nFineShutterWidth2;
	};

	ISCSDKLIB_API int SetHDRMode(int nValue);
	ISCSDKLIB_API int GetHDRMode(int* pnMode);
	ISCSDKLIB_API int SetHDRParameter(HDRParamInfo* param);
	ISCSDKLIB_API int GetHDRParameter(HDRParamInfo* param);

	ISCSDKLIB_API int GetFullFrameInfo(unsigned char* pBuffer);
	ISCSDKLIB_API int GetFullFrameInfo2(unsigned char* pBuffer);
	ISCSDKLIB_API void ExtractByte(unsigned short* pFullFrameInfo, unsigned char* pRawImage, unsigned char* pParaIntgerPart, unsigned char *pParaDecimalPart, int sensor_width, int sensor_height);
	ISCSDKLIB_API void ExtractFloat(unsigned short* pFullFrameInfo, unsigned char* pRawImage, unsigned char* pParaIntgerPart, float *pParallax, int sensor_width, int sensor_height);
	ISCSDKLIB_API void ExtractStereo(unsigned short* pFullFrameInfo, unsigned char* pLeftImage, unsigned char* pRightImage, int sensor_width, int sensor_height);
	ISCSDKLIB_API int SetFPSMode(int fps);

	struct DoubleShutterSrcData {
		unsigned char* image;
		int gain;
		int exposure;
	};

	struct DoubleShutterDstData {
		unsigned char* image1;
		unsigned char* image2;
		float* depth;
	};

	ISCSDKLIB_API int GetFullFrameInfo3(DoubleShutterSrcData* doubleShutterSrcDataCur, DoubleShutterSrcData* doubleShutterSrcDataPrev);
	ISCSDKLIB_API int DoubleShutterImageComposition(DoubleShutterSrcData* doubleShutterSrcDataCur, DoubleShutterSrcData* doubleShutterSrcDataPrev, DoubleShutterDstData* doubleShutterDstData);

	ISCSDKLIB_API int SaveMemoryData(char* DirPath);

	ISCSDKLIB_API int GetImageWithIndex(int* pIndex1, unsigned char* pBuffer1, int* pExposure1, int* pGain1,
										int* pIndex2, unsigned char* pBuffer2, int* pExposure2, int* pGain2,int nSkip);

	ISCSDKLIB_API int GetImageWithIndexEx(	int* pIndex1, unsigned char* pBuffer1, int* pExposure1, int* pGain1,
											int* pIndex2, unsigned char* pBuffer2, int* pExposure2, int* pGain2, int nSkip, int nWaitTime = 100);

	ISCSDKLIB_API int GetDepthInfoWithIndex(int* index, float* pBuffer);

	// ステータス
	struct ISCLibStaus {
		int recieveErrorFrameCount;
	};

	ISCSDKLIB_API int GetISCLibStatus(ISCLibStaus* pISCLibStaus);

	// 拡張レジスター　インターフェース関数(非公開)
	struct Camera_System_Register_Product_Management_Product_Id {
		/*
			// 製品名
			// 製造型識別名
			// 製造シリアル番号
		*/
		char product_name[32];
		char manufacturing_type_name[32];
		char serial_number[16];
	};
	struct Camera_System_Register_Product_Management_License {
		/*
			// ライセンスキー
		*/
		unsigned int license_key;
	};
	struct Camera_System_Register_Product_Management_Vendor_Id {
		/*
			// 製造者番号
			// 製造ベンダー番号
			// プロダクト番号（個体識別番号）
		*/
		unsigned int manufacturer_number;
		unsigned int vendor_number;
		unsigned long long product_number_id_number;
	};
	struct Camera_System_Register_FPGA_Information {
		/*
			// 開発製品識別番号
			// FPGAパージョン番号 FPGA メイン パージョン
			// FPGAパージョン番号 FPGA サブ パージョン
		*/
		unsigned int development_product_id_number;
		unsigned int FPGA_version_number_major;
		unsigned int FPGA_version_number_minor;
	};
	struct Camera_System_Register_Camera_Profile {
		/*
			// D0（単位：1/10000画素）
			// BF（単位：1/10000画素・m）
			// dZ（単位：μm）
			// 基線長（単位：mm）
			// 視野角補正画像 H（単位：1/100度）
			// 視野角補正画像 V（単位：1/100度）
			// 視野角視差画像 H（単位：1/100度）
			// 視野角視差画像 V（単位：1/100度）
			// 画像サイズ幅
			// 画像サイズ高さ
			// 視差上限（視差画像左カット幅）
			// 画像上無効幅（視差画像上カット幅）
			// 画像上無効幅（視差画像下カット幅）
			// 補正画像生成開始ライン
		*/
		unsigned int d0;
		unsigned int bf;
		signed   int dz;
		unsigned int base_length;
		unsigned int angle_correction_H;
		unsigned int angle_correction_V;
		unsigned int angle_parallax_H;
		unsigned int angle_parallax_V;
		unsigned int image_size_width;
		unsigned int image_size_height;
		unsigned int parallax_left_cut_width;
		unsigned int parallax_upper_cut_width;
		unsigned int parallax_lower_cut_width;
		unsigned int rectification_start_line;
	};
	struct Camera_System_Register_Reserved_Data {
		// Reserved data
		// [BANK][DATA]
		unsigned short reserved_data[8][32];
	};
	struct Camera_System_Register_Recognition_Uses {
		/*
			// focus
			// d_slope
			// d_cut
			// X_ORG
			// 認識用 D0
			// 認識用 BF
			// 認識用 Base Length
			// 認識用 DZ
			// FPS
		*/
		float focus;
		signed   int d_slope;
		unsigned int d_cut;
		unsigned int x_org;
		float d0_for_recognition;
		float bf_for_recognition;
		float base_length_for_recognition;
		float dz_for_recognition;
		int fps;
	};
	struct Camera_System_Register_Camera_Profile_Spare {
		// ライセンスIPコード
		// 製品調整回数
		// 製品管理情報作成日	UNIX時刻　UTC
		// セキュリティレベル
		// ハッシュ値
		unsigned int license_IP_code;
		unsigned int number_of_product_adjustments;
		struct timespec management_info_creation_date_UTC;
		unsigned int security_level;
		unsigned int hash_value;
	};
	struct Camera_System_Register_Time_Stamp_Table {
		/*
			// テーブル作成日付		UNIX時刻　UTC
		*/
		// The dates below are utc time
		struct timespec table_creation_date_UTC;
	};
	struct Camera_System_Register_Time_Stamp_BFd0dz {
		/*
			// Bf、D0、dZ算出日付	UNIX時刻　UTC
		*/
		// The dates below are utc time
		struct timespec Bf_D0_dZ_calculation_date_UTC;
	};
	struct Camera_System_Register_Extensions {
		/*
			// セキュリティステータス (RO)
			// Hash operation trigger  , If the value is 1, the trigger is output.
		*/
		unsigned int security_status;
		unsigned int hash_operation_trigger;
	};

	struct Camera_System_Register {
		// 製品管理（製造者）
		Camera_System_Register_Product_Management_Product_Id	sysreg_product_manage_product_id;
		// 製品管理（ITDLab）
		Camera_System_Register_Product_Management_License		sysreg_product_manage_license;
		// 製品管理（ITDLab）
		Camera_System_Register_Product_Management_Vendor_Id		sysreg_product_manage_vendor_id;
		// FPGAプログラム
		Camera_System_Register_FPGA_Information					sysreg_fpga_information;
		// カメラプロファイル
		Camera_System_Register_Camera_Profile					sysreg_camera_profile;
		// 認識用
		Camera_System_Register_Recognition_Uses					sysreg_recognition_uses;
		// カメラプロファイル予備
		Camera_System_Register_Camera_Profile_Spare				sysreg_camera_profile_spare;
		// タイムスタンプ
		Camera_System_Register_Time_Stamp_Table					sysreg_time_stamp_table;
		// タイムスタンプ
		Camera_System_Register_Time_Stamp_BFd0dz				sysreg_time_stamp_bfd0dz;
		// Extensions
		Camera_System_Register_Extensions						sysreg_extensions;
		// Rederved data
		Camera_System_Register_Reserved_Data					sysreg_reserved_data;

	};

	ISCSDKLIB_API int GetSysreg_product_manage_product_id(Camera_System_Register_Product_Management_Product_Id* pParam);
	ISCSDKLIB_API int SetSysreg_product_manage_product_id(Camera_System_Register_Product_Management_Product_Id* pParam, bool updateEEPROMRequest);

	ISCSDKLIB_API int GetSysreg_product_manage_license(Camera_System_Register_Product_Management_License* pParam);
	ISCSDKLIB_API int SetSysreg_product_manage_license(Camera_System_Register_Product_Management_License* pParam, bool updateEEPROMRequest);

	ISCSDKLIB_API int GetSysreg_product_manage_vendor_id(Camera_System_Register_Product_Management_Vendor_Id* pParam);
	ISCSDKLIB_API int SetSysreg_product_manage_vendor_id(Camera_System_Register_Product_Management_Vendor_Id* pParam, bool updateEEPROMRequest);

	ISCSDKLIB_API int GetSysreg_fpga_information(Camera_System_Register_FPGA_Information* pParam);
	ISCSDKLIB_API int SetSysreg_fpga_information(Camera_System_Register_FPGA_Information* pParam, bool updateEEPROMRequest);

	ISCSDKLIB_API int GetSysreg_camera_profile(Camera_System_Register_Camera_Profile* pParam);
	ISCSDKLIB_API int SetSysreg_camera_profile(Camera_System_Register_Camera_Profile* pParam, bool updateEEPROMRequest);

	ISCSDKLIB_API int GetSysreg_recognition_uses(Camera_System_Register_Recognition_Uses* pParam);
	ISCSDKLIB_API int SetSysreg_recognition_uses(Camera_System_Register_Recognition_Uses* pParam, bool updateEEPROMRequest);
	
	ISCSDKLIB_API int GetSysreg_time_stamp_table(Camera_System_Register_Time_Stamp_Table* pParam);
	ISCSDKLIB_API int SetSysreg_time_stamp_table(Camera_System_Register_Time_Stamp_Table* pParam, bool updateEEPROMRequest);

	ISCSDKLIB_API int GetSysreg_time_stamp_bfd0dz(Camera_System_Register_Time_Stamp_BFd0dz* pParam);
	ISCSDKLIB_API int SetSysreg_time_stamp_bfd0dz(Camera_System_Register_Time_Stamp_BFd0dz* pParam, bool updateEEPROMRequest);

	ISCSDKLIB_API int GetSysreg_rederved_data(Camera_System_Register_Reserved_Data* pParam);
	ISCSDKLIB_API int SetSysreg_rederved_data(Camera_System_Register_Reserved_Data* pParam, bool updateEEPROMRequest);
	ISCSDKLIB_API int getSysreg_reserved_area_bank_count(void);
	ISCSDKLIB_API int getSysreg_reserved_area_data_count(const int bank);

	ISCSDKLIB_API int GetSysreg_camera_profile_spare(Camera_System_Register_Camera_Profile_Spare* pParam);
	ISCSDKLIB_API int SetSysreg_camera_profile_spare(Camera_System_Register_Camera_Profile_Spare* pParam, bool updateEEPROMRequest);

	ISCSDKLIB_API int GetSysreg_extensions(Camera_System_Register_Extensions* pParam);
	ISCSDKLIB_API int SetSysreg_extensions(Camera_System_Register_Extensions* pParam, bool updateEEPROMRequest = false);

	ISCSDKLIB_API int GetSystemRegister(Camera_System_Register* pParam);
	ISCSDKLIB_API int SetSystemRegister(Camera_System_Register* pParam, bool updateEEPROMRequest);

	struct Auto_Calib_Register {
		/*
		イネーブルレジスタ
		基準位置レジスタVertical
		基準位置レジスタRotate
		Vertical設定範囲1 (強制調整用)
		Vertical設定範囲1 (自動調整用)
		Rotate設定範囲 (強制調整用)
		Rotate設定範囲 (自動調整用)
		Vertical設定範囲2 (強制調整用)
		Vertical設定範囲2 (自動調整用)
		DCDX値強度閾値
		自動調整失敗判断
		自動調整動作条件
		DCDX値レシオgood閾値
		DCDX値レシオbad閾値
		有効視差閾値
		視差監視期間
		コマンドレジスタ
		ステータスレジスタ1
		ステータスレジスタ2
		テータスレジスタ3
		ステータスレジスタ4
		*/

		bool enable_register_forced_adjustment;
		bool enable_register_auto_adjustment;
		bool enable_register_rotate_enabled;
		bool enable_register_manual_step;
		bool enable_register_enabled_warning;		// > 0x75
		bool enable_register_autocalib_pause;		// > 0x75
		bool enable_register_auto_adjustment_flag;
		bool enable_register_forced_adjustment_flag;

		int reference_position_vertical_translation_position;
		bool reference_position_vertical_update_flag;

		int reference_position_rotate_rotate_position;
		bool reference_position_rotate_update_flag;

		int setting_range_vertical1_auto_translation_adj_range;
		float setting_range_vertical1_auto_translation_step;
		int setting_range_vertical1_forced_translation_adj_range;
		float setting_range_vertical1_forced_translation_step;

		int setting_range_rotate_auto_translation_adj_range;
		float setting_range_rotate_auto_translation_step;
		int setting_range_rotate_forced_translation_adj_range;
		float setting_range_rotate_forced_translation_step;

		int setting_range_vertical2_auto_translation_adj_range;
		float setting_range_vertical2_auto_translation_step;
		int setting_range_vertical2_forced_translation_adj_range;
		float setting_range_vertical2_forced_translation_step;

		int DCDX_intensity_threshold;

		int op_conditions_adj_giveup;		// FPGA	~0x74
		int op_conditions_adj_warn;			// FPGA	~0x74
		int op_conditions_adj_judgment;		// FPGA	~0x74
		int fail_autocalib_condition;		// > 0x75

		int DCDX_ratio_good_threshold;
		int DCDX_ratio_bad_threshold;

		int effective_parallax_threshold_low_limit;
		int effective_parallax_threshold_high_limit;

		int parallax_monitoring_period;		// > 0x75

		bool command_register_manual_step;
		bool command_register_adj_position_update;
		int command_register_adr_monitor;
		int command_register_disparity_monitoring_period_setting;	// > 0x75

		bool status_register1_bad_image;			// > 0x75	
		bool status_register1_auto_adjust_error;
		bool status_register1_outrange_error;
		bool status_register1_adjusting;

		unsigned short status_register1;
		unsigned short status_register2;
		unsigned short status_register3;
		unsigned short status_register4;

		int fail_autocalib_clear_condition;	// > 0x75
	};

	ISCSDKLIB_API int GetAutoCalibRegister(Auto_Calib_Register* pParam);
	ISCSDKLIB_API int SetAutoCalibRegister(Auto_Calib_Register* pParam, bool updateEEPROMRequest);

	// AutoCalib Error Flag bit
	// [0] Automatic adjustment failed
	// [1] Outrange error
	// [2] Bad Image
	// [3:31] not used
	ISCSDKLIB_API int GetAutoCalibErrorStatus(unsigned int* pParam);
	ISCSDKLIB_API int SetAutoCalibErrorClear(unsigned int Param, bool updateEEPROMRequest);

	// EEPROM re-load
	ISCSDKLIB_API int SetReloadEEPROM();

}
