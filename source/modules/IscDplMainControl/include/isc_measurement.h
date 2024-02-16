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
 * @file isc_measurement.h
 * @brief This class is for measurement.
 */
 
#pragma once

/**
 * @class   IscMeasurement
 * @brief   This class is for measurement.
 * 
 */
class IscMeasurement
{
public:
    IscMeasurement();
    ~IscMeasurement();

	/** @brief Initializes the buffers. Must be called at least once before streaming is started.
		@return 0, if successful.
	*/
	int Initialize(const int max_width, const int max_height);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	int Terminate();

	// get information for depth, distance, ...

	/** @brief gets the distance of the given coordinates.
		@return 0, if successful.
	*/
	int GetPositionDepth(const int x, const int y, const IscImageInfo* isc_image_info, float* disparity, float* depth);

	/** @brief gets the 3D position of the given coordinates.
		@return 0, if successful.
	*/
	int GetPosition3D(const int x, const int y, const IscImageInfo* isc_image_info, float* x_d, float* y_d, float* z_d);

	/** @brief get information for the specified region.
		@return 0, if successful.
	*/
	int GetAreaStatistics(const int x, const int y, const int width, const int height, const IscImageInfo* isc_image_info, IscAreaDataStatistics* isc_data_statistics);


private:
	struct WorkBuffers {
		int max_width;
		int max_height;

		unsigned char* image_buffer[4];
		float* depth_buffer[4];
	};
	WorkBuffers work_buffers_;


};
