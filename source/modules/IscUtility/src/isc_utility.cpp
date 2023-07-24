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
 * @file isc_util.cpp
 * @brief Provides auxiliary functions
 * @author Takayuki
 * @date 2023.07.18
 * @version 0.1
 *
 * @details This provides auxiliary functions.
 * @note
 */
#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <shlwapi.h>
#include <imagehlp.h>
#include <Shlobj.h>

#include "isc_utility.h"
#include "isc_util_draw.h"

static IscUtilDraw* isc_util_draw_ = nullptr;


extern "C" {

/**
 * クラスを初期化します.
 *
 * @param[in] utility_parameter 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int DplIscUtilityInitialize(DplIscUtilityParameter* utility_parameter)
{

    // initialize draw class
    if(isc_util_draw_ != nullptr){
        isc_util_draw_->Terminate();
        delete isc_util_draw_;
        isc_util_draw_ = nullptr;
    }

    IscUtilDraw::Cameraparameter camera_parameter = {};
    camera_parameter.max_width = utility_parameter->max_width;
    camera_parameter.max_height = utility_parameter->max_height;
    camera_parameter.base_length = utility_parameter->base_length;
    camera_parameter.d_inf = utility_parameter->d_inf;
    camera_parameter.bf = utility_parameter->bf;
    camera_parameter.camera_angle = utility_parameter->camera_angle;

    IscUtilDraw::DrawParameter draw_parameter = {};
    draw_parameter.draw_outside_bounds = utility_parameter->draw_outside_bounds;
    draw_parameter.min_distance = utility_parameter->min_distance;
    draw_parameter.max_distance = utility_parameter->max_distance;
    draw_parameter.step_distance = utility_parameter->step_distance;
    draw_parameter.min_disparity = utility_parameter->min_disparity;
    draw_parameter.max_disparity = utility_parameter->max_disparity;
    draw_parameter.step_disparity = utility_parameter->step_disparity;

    isc_util_draw_ = new IscUtilDraw;
    int ret = isc_util_draw_->Initialize(&camera_parameter, &draw_parameter);
    if(ret != 0){
        return ret;
    }

    return 0;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int DplIscUtilityTerminate()
{

    if(isc_util_draw_ != nullptr){
        isc_util_draw_->Terminate();
        delete isc_util_draw_;
        isc_util_draw_ = nullptr;
    }

    return 0;
}

/**
 * Color tableを指定範囲で再作成します.
 *
 * @param[in] min_distance　最小距離
 * @param[in] max_distance　最大距離
 * @retval none
 */
void  DplIscUtilityRebuildDrawColorMap(const double min_distance, const double max_distance)
{

    if(isc_util_draw_ == nullptr){
        return;
    }

    isc_util_draw_->RebuildDrawColorMap(min_distance, max_distance);

    return;
}

/**
 * 視差を距離画像(m)へ変換します.
 *
 * @param[in] width　画像の幅
 * @param[in] height　画像の高さ
 * @param[in] disparity　視差
 * @param[out] bgr_image　カラー画像
 * @retval 0 成功
 * @retval other 失敗
 */
int Disparity2DistanceImage(const int width, const int height, float* disparity, unsigned char* bgr_image)
{

    if(isc_util_draw_ == nullptr){
        return -1;
    }

    int ret = isc_util_draw_->Disparity2DistanceImage(width, height, disparity, bgr_image);

    return ret;
}

/**
 * 視差を画像へ変換します.
 *
 * @param[in] width　画像の幅
 * @param[in] height　画像の高さ
 * @param[in] disparity　視差
 * @param[out] bgr_image　カラー画像
 * @retval 0 成功
 * @retval other 失敗
 */
int Disparity2Image(const int width, const int height, float* disparity, unsigned char* bgr_image)
{

    if(isc_util_draw_ == nullptr){
        return -1;
    }

    int ret = isc_util_draw_->Disparity2Image(width, height, disparity, bgr_image);

    return ret;
}

}


