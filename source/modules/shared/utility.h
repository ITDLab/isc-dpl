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
	@file utility.h
	@brief this class provides utility function.　Time measurement, etc...
*/

#pragma once

/**
 * @class   UtilityMeasureTime
 * @brief   Measure takt time / stopwatch
 * this class measure takt time
 */
class UtilityMeasureTime {

public:
    UtilityMeasureTime();
    ~UtilityMeasureTime();

	/** @brief initialize the class.
		@return none.
	*/
	void Init();
    
	/** @brief get takt time.
		@return takt time.
	*/
	double GetTaktTime();

	/** @brief start the stopwatch.
		@return none.
	*/
	void Start();
    
	/** @brief get the elapsed time of the stopwatch.
		@return elapsed time.
	*/
	double Stop();

	/** @brief get the average elapsed time.
		@return none.
	*/
	double GetElapsedTimeAverage();

private:
    LARGE_INTEGER performance_freq_;	/**< performance counter frequency */
    LARGE_INTEGER previous_time_;		/**< last time for takt measurement */

	/** @struct  TaskTimeData
	 *  @brief This is the tact time data
	 */
	struct TaskTimeData {
		bool is_data_valid;				/**< true, if measuring */
		int index;						/**< index for write */
		int max_list_count;				/**< maximum number of lists */
		bool filled;					/**< true if the list is filled */

		double takt_time_list[128];		/**< list data */
	};
	TaskTimeData task_time_data_;		/**< data for takt measurement */

    LARGE_INTEGER stopwatch_start_;		/**< stopwatch start time */

	/** @struct  ElapsedTimeData
	 *  @brief This is the stop watch data
	 */
	struct ElapsedTimeData {
		bool is_data_valid;				/**< true, if measuring */
		int index;						/**< index for write */
		int max_list_count;				/**< maximum number of lists */
		bool filled;					/**< true if the list is filled */

		double elapsed_time_list_[128];	/**< list data */
	};
	ElapsedTimeData elapsed_time_data_;	/**< data for stop watch */

};
