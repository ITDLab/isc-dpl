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
 * @file isc_raw_data_decoder.h
 * @brief decode raw file
 */

#pragma once
class RawDataDecoder
{
public:
	RawDataDecoder();
	~RawDataDecoder();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize();

	/** @brief ... Shut down the  system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	/** @brief Extract RAW data.
		@return 0, if successful.
	*/
	int Decode(const IscCameraModel isc_camera_model, const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode, const IscGetModeColor isc_get_color_mode,
		const int width, const int height, IscImageInfo* isc_image_info);


private:

	ns_vmsdk_wrapper::VmSdkWrapper* vm_sdk_wrapper_;
	ns_xcsdk_wrapper::XcSdkWrapper* xc_sdk_wrapper_;

	/** @brief Extract RAW data.
		@return 0, if successful.
	*/
	int DecodeVM(const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode, const IscGetModeColor isc_get_color_mode,
				const int width, const int height, IscImageInfo* isc_image_info);

	/** @brief Extract RAW data.
		@return 0, if successful.
	*/
	int DecodeXC(const IscGrabMode isc_grab_mode, const IscGrabColorMode isc_grab_color_mode, const IscGetModeColor isc_get_color_mode,
		const int width, const int height, IscImageInfo* isc_image_info);

};

