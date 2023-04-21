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
 * @file utility.cpp
 * @brief Utility functions
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides useful functions.
 * @note ISC100VM SDK 2.0.0~
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <numeric>

#include "utility.h"

/**
 * constructor
 *
 */
UtilityMeasureTime::UtilityMeasureTime():
	performance_freq_(), previous_time_(), task_time_data_(), stopwatch_start_(), elapsed_time_data_()
{
	task_time_data_.max_list_count = 64;
	elapsed_time_data_.max_list_count = 64;

	QueryPerformanceFrequency(&performance_freq_);
}

/**
 * destructor
 *
 */
UtilityMeasureTime::~UtilityMeasureTime()
{

}

/**
 * クラスを初期化します.
 *
 * @return none.
 */
void UtilityMeasureTime::Init()
{
	task_time_data_.is_data_valid = false;
	task_time_data_.filled = false;
	task_time_data_.index = 0;

	elapsed_time_data_.is_data_valid = false;
	elapsed_time_data_.filled = false;
	elapsed_time_data_.index = 0;

}

/**
 * タクトタイムを取得します.
 *
 * @return タクトタイム.
 * @note 取得する値は平均値です(64)
 * 
*/
double UtilityMeasureTime::GetTaktTime()
{
    double average_takt_time = 0;

    if (task_time_data_.is_data_valid) {
        LARGE_INTEGER current_time = {};
        QueryPerformanceCounter(&current_time);
		task_time_data_.takt_time_list[task_time_data_.index] = (double)((double)((current_time.QuadPart - previous_time_.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

		task_time_data_.index++;
        if (task_time_data_.index >= task_time_data_.max_list_count) {
            if (!task_time_data_.filled) {
				task_time_data_.filled = true;
            }
			task_time_data_.index = 0;
        }

        if (task_time_data_.filled) {
            average_takt_time = std::accumulate(&task_time_data_.takt_time_list[0], &task_time_data_.takt_time_list[task_time_data_.max_list_count - 1], 0.0F);
            average_takt_time /= task_time_data_.max_list_count;
        }

        QueryPerformanceCounter(&previous_time_);
    }
    else {
		task_time_data_.is_data_valid = true;

        QueryPerformanceCounter(&previous_time_);
    }

    return average_takt_time;
}

/**
 * ストップウォッチを開始します.
 *
 * @return none.
 *
*/
void UtilityMeasureTime::Start()
{
    QueryPerformanceCounter(&stopwatch_start_);
}

/**
 * ストップウォッチを停止し、経過時間を取得します.
 *
 * @return 経過時間.
 *
*/
double UtilityMeasureTime::Stop()
{
    LARGE_INTEGER current_time = {};
    QueryPerformanceCounter(&current_time);
    double elapsed_time = (double)((double)((current_time.QuadPart - stopwatch_start_.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

	// set data
	elapsed_time_data_.elapsed_time_list_[elapsed_time_data_.index] = elapsed_time;
	elapsed_time_data_.index++;

	if (elapsed_time_data_.index >= elapsed_time_data_.max_list_count) {
		if (!elapsed_time_data_.filled) {
			elapsed_time_data_.filled = true;
		}
		elapsed_time_data_.index = 0;
	}
	elapsed_time_data_.is_data_valid = true;


    return elapsed_time;
}

/**
 * ストップウォッチの経過時間の平均値(64)を取得します.
 *
 * @return 経過時間の平均値.
 *
*/
double UtilityMeasureTime::GetElapsedTimeAverage()
{
	double average_time = 0;

	if (!elapsed_time_data_.is_data_valid) {
		return average_time;
	}

	if (elapsed_time_data_.filled) {
		average_time = std::accumulate(&elapsed_time_data_.elapsed_time_list_[0], &elapsed_time_data_.elapsed_time_list_[elapsed_time_data_.max_list_count - 1], 0.0F);
		average_time /= elapsed_time_data_.max_list_count;
	}
	else {
		average_time = std::accumulate(&elapsed_time_data_.elapsed_time_list_[0], &elapsed_time_data_.elapsed_time_list_[elapsed_time_data_.index - 1], 0.0F);
		average_time /= elapsed_time_data_.max_list_count;
	}

	return average_time;
}
