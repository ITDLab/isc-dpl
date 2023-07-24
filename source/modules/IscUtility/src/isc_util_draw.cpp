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
 * @file isc_util_draw.cpp
 * @brief Provides auxiliary functions for display
 * @author Takayuki
 * @date 2023.07.18
 * @version 0.1
 *
 * @details This class provides auxiliary functions for display.
 * @note
 */
#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <shlwapi.h>
#include <imagehlp.h>
#include <Shlobj.h>
#include <math.h>

#include "isc_util_draw.h"

IscUtilDraw::IscUtilDraw():
    camera_parameter_(), draw_parameter_(),
    disp_color_map_distance_(), disp_color_map_disparity_()
{

}

IscUtilDraw::~IscUtilDraw()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] camera_parameter 初期化パラメータ構造体
 * @param[in] draw_parameter 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscUtilDraw::Initialize(Cameraparameter* camera_parameter, DrawParameter* draw_parameter)
{

    camera_parameter_.max_width = camera_parameter->max_width;
    camera_parameter_.max_height = camera_parameter->max_height;
    camera_parameter_.base_length = camera_parameter->base_length;
    camera_parameter_.d_inf = camera_parameter->d_inf;
    camera_parameter_.bf = camera_parameter->bf;
    camera_parameter_.camera_angle = camera_parameter->camera_angle;

    draw_parameter_.draw_outside_bounds = draw_parameter->draw_outside_bounds;
    draw_parameter_.min_distance = draw_parameter->min_distance;
    draw_parameter_.max_distance = draw_parameter->max_distance;
    draw_parameter_.step_distance = draw_parameter->step_distance;
    draw_parameter_.min_disparity = draw_parameter->min_disparity;
    draw_parameter_.max_disparity = draw_parameter->max_disparity;
    draw_parameter_.step_disparity = draw_parameter->step_disparity;

	disp_color_map_distance_.min_value = draw_parameter_.min_distance;
	disp_color_map_distance_.max_value = draw_parameter_.max_distance;
	disp_color_map_distance_.color_map_size = 0;
	disp_color_map_distance_.color_map_step = draw_parameter_.step_distance;
	disp_color_map_distance_.color_map_size = (int)(draw_parameter_.max_distance / disp_color_map_distance_.color_map_step) + 1;
	disp_color_map_distance_.color_map = new int[disp_color_map_distance_.color_map_size + sizeof(int)];
	memset(disp_color_map_distance_.color_map, 0, disp_color_map_distance_.color_map_size + sizeof(int));
	BuildColorHeatMap(&disp_color_map_distance_);

	disp_color_map_disparity_.min_value = draw_parameter_.min_disparity;
	disp_color_map_disparity_.max_value = draw_parameter_.max_disparity;
	disp_color_map_disparity_.color_map_size = 0;
	disp_color_map_disparity_.color_map_step = draw_parameter_.step_disparity;
	disp_color_map_disparity_.color_map_size = (int)(draw_parameter_.max_disparity / disp_color_map_disparity_.color_map_step) + 1;
	disp_color_map_disparity_.color_map = new int[disp_color_map_disparity_.color_map_size + sizeof(int)];
	memset(disp_color_map_disparity_.color_map, 0, disp_color_map_disparity_.color_map_size + sizeof(int));
	BuildColorHeatMapForDisparity(&disp_color_map_disparity_);

    return 0;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscUtilDraw::Terminate()
{

	delete[] disp_color_map_distance_.color_map;
	disp_color_map_distance_.color_map = nullptr;

	delete[] disp_color_map_disparity_.color_map;
	disp_color_map_disparity_.color_map = nullptr;

    return 0;
}

/**
 * Color tableを指定範囲で再作成します.
 *
 * @param[in] min_distance　最小距離
 * @param[in] max_distance　最大距離
 * @retval none
 */
void IscUtilDraw::RebuildDrawColorMap(const double min_distance, const double max_distance)
{
	delete[] disp_color_map_distance_.color_map;
	disp_color_map_distance_.color_map = nullptr;

    draw_parameter_.min_distance = min_distance;
    draw_parameter_.max_distance = max_distance;

	disp_color_map_distance_.min_value = draw_parameter_.min_distance;
	disp_color_map_distance_.max_value = draw_parameter_.max_distance;
	disp_color_map_distance_.color_map_size = 0;
	disp_color_map_distance_.color_map_step = 0.01;
	disp_color_map_distance_.color_map_size = (int)(disp_color_map_distance_.max_value / disp_color_map_distance_.color_map_step) + 1;
	disp_color_map_distance_.color_map = new int[disp_color_map_distance_.color_map_size + sizeof(int)];
	memset(disp_color_map_distance_.color_map, 0, disp_color_map_distance_.color_map_size + sizeof(int));
	BuildColorHeatMap(&disp_color_map_distance_);

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
int IscUtilDraw::Disparity2DistanceImage(const int width, const int height, float* disparity, unsigned char* bgr_image)
{

    if((disparity == nullptr) || (bgr_image == nullptr)) {
        return -1;
    }

    // Converts parallax to distance image
    const double max_length = disp_color_map_distance_.max_value;
    const double min_length = disp_color_map_distance_.min_value;
    DispColorMap* disp_color_map = &disp_color_map_distance_;

    bool ret = MakeDepthColorImage(	
        true, 
        draw_parameter_.draw_outside_bounds, 
        min_length, 
        max_length,
        disp_color_map, 
        camera_parameter_.base_length, 
        camera_parameter_.camera_angle, 
        camera_parameter_.bf, 
        camera_parameter_.d_inf,
        width, 
        height, 
        disparity, 
        bgr_image);

    return 0;
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
int IscUtilDraw::Disparity2Image(const int width, const int height, float* disparity, unsigned char* bgr_image)
{

    if((disparity == nullptr) || (bgr_image == nullptr)) {
        return -1;
    }

    // Converts parallax to image
    const double max_length = disp_color_map_disparity_.max_value;
    const double min_length = disp_color_map_disparity_.min_value;
    DispColorMap* disp_color_map = &disp_color_map_disparity_;

    bool ret = MakeDepthColorImage(	
        false, 
        draw_parameter_.draw_outside_bounds, 
        min_length, 
        max_length,
        disp_color_map, 
        camera_parameter_.base_length, 
        camera_parameter_.camera_angle, 
        camera_parameter_.bf, 
        camera_parameter_.d_inf,
        width, 
        height, 
        disparity, 
        bgr_image);

    return 0;
}

/**
 * 入力値をカラーデータへ変換します.
 *
 * @param[in] min_value　最小値
 * @param[in] max_value　最大値
 * @param[in] in_value　入力値
 * @param[out] bo　青成分
 * @param[out] go　緑成分
 * @param[out] ro　赤成分
 * @retval 0 成功
 * @retval other 失敗
 */
int IscUtilDraw::ColorScaleBCGYR(const double min_value, const double max_value, const double in_value, int* bo, int* go, int* ro)
{
	int ret = 0;
	int r = 0, g = 0, b = 0;

	// 0.0～1.0 の範囲の値をサーモグラフィみたいな色にする
	// 0.0                    1.0
	// 青    水    緑    黄    赤
	// 最小値以下 = 青
	// 最大値以上 = 赤

	if (in_value <= min_value) {
		// red
		r = 255;     
		g = 0;       
		b = 0;
	}
	else if (in_value >= max_value) {
		// blue
		r = 0;
		g = 0;
		b = 255;
	}
	else {
		double temp_in_value = in_value - min_value;
		double range = max_value - min_value;

		double  value = 1.0 - (temp_in_value / range);
		double  tmp_val = cos(4 * 3.141592653589793 * value);
		int     col_val = (int)((-tmp_val / 2 + 0.5) * 255);

		if (value >= (4.0 / 4.0)) { r = 255;     g = 0;       b = 0; }		// 赤
		else if (value >= (3.0 / 4.0)) { r = 255;     g = col_val; b = 0; }		// 黄～赤
		else if (value >= (2.0 / 4.0)) { r = col_val; g = 255;     b = 0; }		// 緑～黄
		else if (value >= (1.0 / 4.0)) { r = 0;       g = 255;     b = col_val; }	// 水～緑
		else if (value >= (0.0 / 4.0)) { r = 0;       g = col_val; b = 255; }		// 青～水
		else { r = 0;       g = 0;       b = 255; }		// 青
	}

	*bo = b;
	*go = g;
	*ro = r;

	return ret;
}

/**
 * 距離基準のヒートマップを作成します.
 *
 * @param[inout] disp_color_map　マップ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscUtilDraw::BuildColorHeatMap(DispColorMap* disp_color_map)
{
	const double min_value = disp_color_map->min_value;
	const double max_value = disp_color_map->max_value;
	const double step = disp_color_map->color_map_step;
	int* color_map = disp_color_map->color_map;

	int start = 0;
	int end = (int)(max_value / step);
	double length = 0;

	int* p_color_map = color_map;

	for (int i = start; i <= end; i++) {

		int ro = 0, go = 0, bo = 0;
		ColorScaleBCGYR(min_value, max_value, length, &bo, &go, &ro);

		*p_color_map = (0xff000000) | (ro << 16) | (go << 8) | (bo);
		p_color_map++;

		length += step;
	}

	return 0;
}

/**
 * 視差基準のヒートマップを作成します.
 *
 * @param[inout] disp_color_map　マップ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscUtilDraw::BuildColorHeatMapForDisparity(DispColorMap* disp_color_map)
{
	const double min_value = disp_color_map->min_value;
	const double max_value = disp_color_map->max_value;
	const double step = disp_color_map->color_map_step;
	int* color_map = disp_color_map->color_map;

	int start = 0;
	int end = (int)(max_value / step);
	double length = 0;

	int* p_color_map = color_map;

	double* gamma_lut = new double[end+1];
	memset(gamma_lut, 0, sizeof(double) * (end + 1));
	
	double gamma = 0.7;	// fix it, good for 4020
	for (int i = 0; i <= end; i++) {
		gamma_lut[i] = (int)(pow((double)i / 255.0, 1.0 / gamma) * 255.0);
	}

	for (int i = start; i <= end; i++) {

		int ro = 0, go = 0, bo = 0;
		double value = (double)(gamma_lut[(int)length]);

		ColorScaleBCGYR(min_value, max_value, value, &bo, &go, &ro);

		*p_color_map = (0xff000000) | (ro << 16) | (go << 8) | (bo);
		p_color_map++;

		length += step;
	}

	delete[] gamma_lut;

	return 0;
}

/**
 * 入力値をカラーデータへ変換します.
 *
 * @param[in] is_color_by_distance　true:距離基準画像　false:視差基準画像
 * @param[in] is_draw_outside_bounds　true:範囲外を描画する　false:範囲は黒とする
 * @param[in] min_length_i　最小距離
 * @param[in] max_length_i　最大距離
 * @param[in] disp_color_map　カラーテーブル
 * @param[in] b_i　基線長
 * @param[in] angle_i　カメラ角度（未使用）
 * @param[in] bf_i　カメラ固有値
 * @param[in] dinf_i　カメラ固有値
 * @param[in] width　画像幅
 * @param[in] height　画像高さ
 * @param[in] depth　視差データ
 * @param[out] bgr_image　カラー画像
 * @retval 0 成功
 * @retval other 失敗
 */
bool IscUtilDraw::MakeDepthColorImage(	const bool is_color_by_distance, const bool is_draw_outside_bounds, const double min_length_i, const double max_length_i,
										DispColorMap* disp_color_map, double b_i, const double angle_i, const double bf_i, const double dinf_i,
										const int width, const int height, float* depth, unsigned char* bgr_image)
{
	if (disp_color_map == nullptr) {
		return false;
	}

	if (depth == nullptr) {
		return false;
	}

	if (bgr_image == nullptr) {
		return false;
	}

	constexpr int channel_count = 3;

	constexpr double pi = 3.1415926535;
	const double bf = bf_i;
	const double b = b_i;
	const double dinf = dinf_i;
	const double rad = angle_i * pi / 180.0;
	const double color_map_step_mag = 1.0 / disp_color_map->color_map_step;

	if (is_color_by_distance) {
		// 距離変換

		for (int i = 0; i < height; i++) {
			float* src = depth + (i * width);
			unsigned char* dst = bgr_image + (i * width * channel_count);

			for (int j = 0; j < width; j++) {
				int r = 0, g = 0, b = 0;
				if (*src <= dinf) {
					r = 0;
					g = 0;
					b = 0;
				}
				else {
					double d = (*src - dinf);
					double za = max_length_i;
					if (d > 0) {
#if 0
						double yh = (b * (i - (height / 2))) / d;
						double z = bf / d;
						za = -1 * yh * sin(rad) + z * cos(rad);
#else
						za = bf / d;
#endif
					}

					if (is_draw_outside_bounds) {
						int map_index = (int)(za * color_map_step_mag);
						if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
							int map_value = disp_color_map->color_map[map_index];

							r = (unsigned char)(map_value >> 16);
							g = (unsigned char)(map_value >> 8);
							b = (unsigned char)(map_value);
						}
						else {
							// it's blue
							r = 0;
							g = 0;
							b = 255;
						}
					}
					else {
						if (za > max_length_i) {
							r = 0;
							g = 0;
							b = 0;
						}
						else if (za < min_length_i) {
							r = 0;
							g = 0;
							b = 0;
						}
						else {
							int map_index = (int)(za * color_map_step_mag);
							if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
								int map_value = disp_color_map->color_map[map_index];

								r = (unsigned char)(map_value >> 16);
								g = (unsigned char)(map_value >> 8);
								b = (unsigned char)(map_value);
							}
							else {
								// it's black
								r = 0;
								g = 0;
								b = 0;
							}
						}
					}
				}

				*dst++ = b;
				*dst++ = g;
				*dst++ = r;

				src++;
			}
		}
	}
	else {
		// 視差
		const double max_value = disp_color_map_disparity_.max_value;
		const double dinf = dinf_i;

		for (int i = 0; i < height; i++) {
			float* src = depth + (i * width);
			unsigned char* dst = bgr_image + (i * width * channel_count);

			for (int j = 0; j < width; j++) {
				int r = 0, g = 0, b = 0;
				if (*src <= dinf) {
					r = 0;
					g = 0;
					b = 0;
				}
				else {
					double d = 0 >= (max_value - *src - dinf) ? 0 : (max_value - *src - dinf);

					int map_index = (int)(d * color_map_step_mag);
					if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
						int map_value = disp_color_map->color_map[map_index];

						r = (unsigned char)(map_value >> 16);
						g = (unsigned char)(map_value >> 8);
						b = (unsigned char)(map_value);
					}
					else {
						// it's black
						r = 0;
						g = 0;
						b = 0;
					}
				}

				*dst++ = b;
				*dst++ = g;
				*dst++ = r;

				src++;
			}
		}
	}

	return true;
}
