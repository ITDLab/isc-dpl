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
 * @file isc_measurement.cpp
 * @brief This class is for measurement
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details TThis class is for measurement.
 */
#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <stdint.h>
#include <process.h>
#include <mutex>
#include <functional>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_log.h"
#include "utility.h"

#include "isc_measurement.h"

#include "opencv2\opencv.hpp"

struct HistogramData {
    int resolution;             // 分解能　視差ｘresolution　として計算します

    int histo_buffer_size;      // バッファーサイズ
    float* histo_buffer;        // 作業バッファー

    // for debug dispalay
    int buffer_size;
    float* buffer;

};
HistogramData disparity_histogram_, distance_histogram_;

int GetMaxDisparityValueInHistogram(cv::Mat src_data, const float min_valid_value, float* max_frequency_value, HistogramData* histogram_data);
int GetMaxDistanceValueInHistogram(cv::Mat src_data, const float min_valid_value, float* max_frequency_value, HistogramData* histogram_data);

/**
 * constructor
 *
 */
IscMeasurement::IscMeasurement():work_buffers_()
{

}

/**
 * destructor
 *
 */
IscMeasurement::~IscMeasurement()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] ipc_dpl_configuration 初期化パラメータ構造体
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMeasurement::Initialize(const int max_width, const int max_height)
{
    // get work
    work_buffers_.max_width = max_width;
    work_buffers_.max_height = max_height;
    size_t work_buffer_frame_size = max_width * max_height;

    for (int i = 0; i < 1; i++) {
        work_buffers_.image_buffer[i] = new unsigned char[work_buffer_frame_size * 3];
        memset(work_buffers_.image_buffer[i], 0, work_buffer_frame_size * 3);

        work_buffers_.depth_buffer[i] = new float[work_buffer_frame_size];
        memset(work_buffers_.depth_buffer[i], 0, work_buffer_frame_size * sizeof(float));
    }

    disparity_histogram_.resolution = 100;  // 視差の計算分解能
    disparity_histogram_.histo_buffer_size = 256 * disparity_histogram_.resolution; // disparity 0.0 ~ 255.0
    disparity_histogram_.histo_buffer = new float[disparity_histogram_.histo_buffer_size];

    disparity_histogram_.buffer_size = 256; // disparity 0.0 ~ 255.0
    disparity_histogram_.buffer = new float[disparity_histogram_.buffer_size];


    distance_histogram_.resolution = 1000;                                  // 距離の計算分解能
    distance_histogram_.histo_buffer_size = distance_histogram_.resolution; // 1000段階
    distance_histogram_.histo_buffer = new float[distance_histogram_.histo_buffer_size];

    distance_histogram_.buffer_size = 1000;
    distance_histogram_.buffer = new float[distance_histogram_.buffer_size];
 
    return DPC_E_OK;
}

/**
 * 終了処理をします.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMeasurement::Terminate()
{
    delete[] distance_histogram_.buffer;
    distance_histogram_.buffer = nullptr;

    delete[] distance_histogram_.histo_buffer;
    distance_histogram_.histo_buffer = nullptr;

    delete[] disparity_histogram_.buffer;
    disparity_histogram_.buffer = nullptr;

    delete[] disparity_histogram_.histo_buffer;
    disparity_histogram_.histo_buffer = nullptr;

    for (int i = 0; i < 1; i++) {
        delete[] work_buffers_.image_buffer[i];
        delete[] work_buffers_.depth_buffer[i];
    }
    work_buffers_.max_width = 0;
    work_buffers_.max_height = 0;

    return DPC_E_OK;
}

/**
 * 指定位置の視差と距離を取得します
 *
 * @param[in] x 画像内座標(X)
 * @param[in] y 画像内座標(Y)
 * @param[in] isc_image_info データ構造体
 * @param[out] disparity 視差
 * @param[out] depth 距離(m)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMeasurement::GetPositionDepth(const int x, const int y, const IscImageInfo* isc_image_info, float* disparity, float* depth)
{

    if (isc_image_info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (disparity == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (depth == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    IscShutterMode shutter_mode = isc_image_info->shutter_mode;

    if (shutter_mode == IscShutterMode::kDoubleShutter) {
        // Double Shutterモードで、結合結果のデータがあれば、それを使用する
        int temp_width  = isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_MERGED].depth.width;
        int temp_height = isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_MERGED].depth.height;

        if (temp_width > 0 && temp_height > 0) {
            fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
        }
    }

    int width = isc_image_info->frame_data[fd_index].depth.width;
    int height = isc_image_info->frame_data[fd_index].depth.height;

    /*
        視差の取り込みが無い取り込みモードであれば、width/heightが0であるため、下記でエラーとなる
    
    */

    if ((x <= 0) || (x >= width)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if ((y <= 0) || (y >= height)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    /*
        視差は、4x4のBlockで算出されており、それを展開し画像サイズとしている
        展開時に、MaskによるOn/Offにより、視差のない部分が発生しいる
        
        ユーザーが画面上で選択した座標は厳密ではいため、Block内に視差がれば、そこが選択れたものとして値を計算する
    */

    int x_s = (int)((int)((double)x / 4.0) * 4);
    int x_e = x_s + 4;
    if (x_e >= width) {
        x_e = width - 1;
    }

    int y_s = (int)((int)((double)y / 4.0) * 4);
    int y_e = y_s + 4;
    if (y_e >= height) {
        y_e = height - 1;
    }

    float depth_array[16] = {};
    int index = 0;
    for (int i = y_s; i < y_e; i++) {
        float* src = isc_image_info->frame_data[fd_index].depth.image + (i * width);

        for (int j = x_s; j < x_e; j++) {
            depth_array[index] = *(src + j);
            index++;
        }
    }
    
    if (index == 0) {
        *disparity = 0;
        *depth = 0;
    }
    else {
        float block_disparity = 0.0f;
        for (auto value : depth_array) {
            if (value > 0) {
                // 有効な視差
                block_disparity = value;
                break;
            }
        }

        if (block_disparity > isc_image_info->camera_specific_parameter.d_inf) {
            *disparity = block_disparity;
            *depth = isc_image_info->camera_specific_parameter.bf / (block_disparity - isc_image_info->camera_specific_parameter.d_inf);
        }
        else {
            *disparity = 0;
            *depth = 0;
        }
    }

    return DPC_E_OK;
}

/**
 * 指定位置の3D位置を取得します
 *
 * @param[in] x 画像内座標(X)
 * @param[in] y 画像内座標(Y)
 * @param[in] isc_image_info データ構造体
 * @param[out] x_d 画面中央からの距離(m)
 * @param[out] y_d 画面中央からの距離(m)
 * @param[out] z_d 距離(m)
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMeasurement::GetPosition3D(const int x, const int y, const IscImageInfo* isc_image_info, float* x_d, float* y_d, float* z_d)
{
    if (isc_image_info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (x_d == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (y_d == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (z_d == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    float disparity = 0.0f;
    float depth = 0.0f;
    int ret = GetPositionDepth(x, y, isc_image_info, &disparity, &depth);

    if (ret != DPC_E_OK) {
        return ret;
    }

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    IscShutterMode shutter_mode = isc_image_info->shutter_mode;

    if (shutter_mode == IscShutterMode::kDoubleShutter) {
        // Double Shutterモードで、結合結果のデータがあれば、それを使用する
        int temp_width = isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_MERGED].depth.width;
        int temp_height = isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_MERGED].depth.height;

        if (temp_width > 0 && temp_height > 0) {
            fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
        }
    }

    int width = isc_image_info->frame_data[fd_index].depth.width;
    int height = isc_image_info->frame_data[fd_index].depth.height;

    if (disparity > isc_image_info->camera_specific_parameter.d_inf) {
        float bd = isc_image_info->camera_specific_parameter.base_length / disparity;

        *x_d = (float)(x - width / 2) * bd;
        *y_d = (float)(height / 2 - y) * bd;
        *z_d = depth;
    }
    else {
        *x_d = 0;
        *y_d = 0;
        *z_d = 0;
    }

    return DPC_E_OK;
}

/**
 * 中央値を取得します
 *
 * @param[in] input_org 入力配列
 * @retval 中央値
 * @note 失敗時には、0を戻します
 */
float MedianMat(cv::Mat& input_org) {

    std::vector<float> vec_from_mat;

    for (int i = 0; i < input_org.rows; i++) {
        float* src = input_org.ptr<float>(i);

        for (int j = 0; j < input_org.cols; j++) {
            if (*src > 1) {
                vec_from_mat.push_back(*src);
            }
            src++;
        }
    }

    if (vec_from_mat.size() == 0) {
        return 0.0F;
    }

    //Input = Input.reshape(0, 1); // spread Input Mat to single row
    //Input.copyTo(vec_from_mat); // Copy Input Mat to vector vec_from_mat    

    // sort vec_from_mat
    std::sort(vec_from_mat.begin(), vec_from_mat.end());

    // in case of even-numbered matrix
    if (vec_from_mat.size() % 2 == 0) { 
        return (vec_from_mat[vec_from_mat.size() / 2 - 1] + vec_from_mat[vec_from_mat.size() / 2]) / 2; 
    } 
    
    // odd-number of elements in matrix
    return vec_from_mat[(vec_from_mat.size() - 1) / 2]; 
}

/**
 * 指定領域の情報を取得します
 *
 * @param[in] x 画像内座標左上(X)
 * @param[in] y 画像内座標左上(Y)
 * @param[in] width 幅
 * @param[in] height 高さ
 * @param[in] isc_image_info データ構造体
 * @param[out] isc_data_statistics 領域の情報
 * @retval 0 成功
 * @retval other 失敗
 */
int IscMeasurement::GetAreaStatistics(const int x, const int y, const int width, const int height, const IscImageInfo* isc_image_info, IscAreaDataStatistics* isc_data_statistics)
{
    
    if (isc_image_info == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (isc_data_statistics == nullptr) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    double min_distance = isc_data_statistics->min_distance;
    double max_distance = isc_data_statistics->max_distance;

    memset(isc_data_statistics, 0, sizeof(IscAreaDataStatistics));

    isc_data_statistics->min_distance = min_distance;
    isc_data_statistics->max_distance = max_distance;

    int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

    IscShutterMode shutter_mode = isc_image_info->shutter_mode;

    if (shutter_mode == IscShutterMode::kDoubleShutter) {
        // Double Shutterモードで、結合結果のデータがあれば、それを使用する
        int temp_width = isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_MERGED].depth.width;
        int temp_height = isc_image_info->frame_data[kISCIMAGEINFO_FRAMEDATA_MERGED].depth.height;

        if (temp_width > 0 && temp_height > 0) {
            fd_index = kISCIMAGEINFO_FRAMEDATA_MERGED;
        }
    }

    int image_width = isc_image_info->frame_data[fd_index].depth.width;
    int image_height = isc_image_info->frame_data[fd_index].depth.height;

    if ((x <= 0) || (x >= image_width)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if ((y <= 0) || (y >= image_height)) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    // 対象の切り出し
    cv::Rect roi = {};
    roi.x = x;
    roi.y = y;
    roi.width = x + width < image_width ? width : image_width - (x + width) - 1;
    roi.height = y + height < image_height ? height : image_height - (y + height) - 1;

    if (roi.width < 0) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    if (roi.height < 0) {
        return ISCDPL_E_INVALID_PARAMETER;
    }

    cv::Mat src_disparity(image_height, image_width, CV_32F, isc_image_info->frame_data[fd_index].depth.image);
    cv::Mat roi_depth = src_disparity(roi);

    const float valid_minimum = isc_image_info->camera_specific_parameter.d_inf;
    
    // Histogramより最頻値を取得
    float max_disparity_frequency_value = 0.0F;
    int histo_ret = GetMaxDisparityValueInHistogram(roi_depth, valid_minimum, &max_disparity_frequency_value, &disparity_histogram_);

    // 視差の平均を計算
    float sum_of_depth = 0;
    float max_of_depth = 0, min_of_depth = 999;

    unsigned int sum_of_depth_count = 0;
    for (int i = 0; i < roi_depth.rows; i++) {
        float* src = roi_depth.ptr<float>(i);

        for (int j = 0; j < roi_depth.cols; j++) {
            if (*src > valid_minimum) {
                sum_of_depth += *src;
                sum_of_depth_count++;

                if (*src > max_of_depth) {
                    max_of_depth = *src;
                }
                if (*src < min_of_depth) {
                    min_of_depth = *src;
                }
            }
            src++;
        }
    }

    float average_of_depth = 0;
    if (sum_of_depth_count != 0) {
        average_of_depth = sum_of_depth / (float)sum_of_depth_count;
    }

    // 中央値を計算
    float median_of_depth = 0;
    if (sum_of_depth_count != 0) {
        median_of_depth = MedianMat(roi_depth);
    }

    // 視差の標準偏差を計算
    float std_dev_of_depth = 0;
    if (sum_of_depth_count != 0) {

        float sum_of_mean_diff = 0;
        unsigned int sum_of_mean_diff_count = 0;
        for (int i = 0; i < roi_depth.rows; i++) {
            float* src = roi_depth.ptr<float>(i);

            for (int j = 0; j < roi_depth.cols; j++) {
                if (*src > valid_minimum) {
                    sum_of_mean_diff += ((*src - average_of_depth) * (*src - average_of_depth));
                    sum_of_mean_diff_count++;
                }
                src++;
            }
        }
        std_dev_of_depth = sqrt(sum_of_mean_diff / sum_of_mean_diff_count);
    }

    // 結果を設定
    isc_data_statistics->x = x;
    isc_data_statistics->y = x;
    isc_data_statistics->width = roi_depth.cols;
    isc_data_statistics->height = roi_depth.rows;

    isc_data_statistics->statistics_depth.max_value = max_of_depth;
    isc_data_statistics->statistics_depth.min_value = min_of_depth;
    isc_data_statistics->statistics_depth.std_dev = std_dev_of_depth;
    isc_data_statistics->statistics_depth.average = average_of_depth;
    isc_data_statistics->statistics_depth.median = median_of_depth;

    float d_inf = isc_image_info->camera_specific_parameter.d_inf;
    float bf = isc_image_info->camera_specific_parameter.bf;
    float base_length = isc_image_info->camera_specific_parameter.base_length;
    if (max_disparity_frequency_value > d_inf) {
        float bd = base_length / (max_disparity_frequency_value - d_inf);
        isc_data_statistics->roi_3d.width = bd * isc_data_statistics->width;
        isc_data_statistics->roi_3d.height = bd * isc_data_statistics->height;
        isc_data_statistics->roi_3d.distance = bf / (max_disparity_frequency_value - d_inf);
    }
    else {
        isc_data_statistics->roi_3d.width = 0;
        isc_data_statistics->roi_3d.height = 0;
        isc_data_statistics->roi_3d.distance = 0;
    }

    // convert to distance
    float sum_of_distance = 0;
    float max_of_distance = 0, min_of_distance = 99999;
    unsigned int sum_of_distance_count = 0;

    cv::Mat src_distance(roi_depth.rows, roi_depth.cols, CV_32F, work_buffers_.depth_buffer[0]);

    for (int i = 0; i < roi_depth.rows; i++) {
        float* src = roi_depth.ptr<float>(i);
        float* dst = src_distance.ptr<float>(i);

        for (int j = 0; j < roi_depth.cols; j++) {
            if (*src > valid_minimum) {

                float distance = bf / (*src - d_inf);
                
                if ((distance > isc_data_statistics->min_distance) && (distance < isc_data_statistics->max_distance)) {
                    *dst = distance;

                    sum_of_distance += distance;
                    sum_of_distance_count++;

                    if (distance > max_of_distance) {
                        max_of_distance = distance;
                    }
                    if (distance < min_of_distance) {
                        min_of_distance = distance;
                    }
                }
                else {
                    *dst = 0;
                }
            }
            else {
                *dst = 0;
            }
            src++;
            dst++;
        }
    }

    float average_of_distance = 0;
    if (sum_of_distance_count != 0) {
        average_of_distance = sum_of_distance / (float)sum_of_distance_count;
    }

    float median_of_distance = 0;
    if (sum_of_distance_count != 0) {
        median_of_distance = MedianMat(src_distance);
    }

    float std_dev_of_distance = 0;
    if (sum_of_distance_count != 0) {

        float sum_of_mean_diff = 0;
        unsigned int sum_of_mean_diff_count = 0;
        for (int i = 0; i < src_distance.rows; i++) {
            float* src = src_distance.ptr<float>(i);

            for (int j = 0; j < src_distance.cols; j++) {
                if (*src > 0) {
                    sum_of_mean_diff += ((*src - average_of_distance) * (*src - average_of_distance));
                    sum_of_mean_diff_count++;
                }
                src++;
            }
        }
        std_dev_of_distance = sqrt(sum_of_mean_diff / sum_of_mean_diff_count);
    }

    // Histogramより最頻値を取得
    float max_distance_frequency_value = 0.0F;
    float valid_distance_minimum = std::max((double)isc_data_statistics->min_distance, std::max(0.5, isc_image_info->camera_specific_parameter.bf / 255.0));

    histo_ret = GetMaxDistanceValueInHistogram(src_distance, valid_distance_minimum, &max_distance_frequency_value, &distance_histogram_);

    // 結果を設定
    isc_data_statistics->statistics_distance.max_value = max_of_distance;
    isc_data_statistics->statistics_distance.min_value = min_of_distance;
    isc_data_statistics->statistics_distance.std_dev = std_dev_of_distance;
    isc_data_statistics->statistics_distance.average = average_of_distance;
    isc_data_statistics->statistics_distance.median = median_of_distance;

    return DPC_E_OK;
}

int GetMaxDisparityValueInHistogram(cv::Mat src_data, const float min_valid_value, float* max_frequency_value, HistogramData* histogram_data)
{
    // Histogram
    /*
        Histogramの最頻値を取得します
    */

    float* hist_data = histogram_data->histo_buffer;
    memset(hist_data, 0, sizeof(float) * histogram_data->histo_buffer_size);

    for (int i = 0; i < src_data.rows; i++) {
        float* src = src_data.ptr<float>(i);

        for (int j = 0; j < src_data.cols; j++) {
            int index = (int)(*src * (float)histogram_data->resolution);
            if (index < histogram_data->histo_buffer_size) {
                hist_data[index]++;
            }
            src++;
        }
    }

    // 無効分のデータをクリアする
    int start = 0;
    int end = (int)(min_valid_value + 0.5);
    for (int i = start; i < end; i++) {
        hist_data[i] = 0;
    }
    double max_histo_val = 0;
    cv::Point max_histo_point = {};
    cv::Mat src_hist_data(histogram_data->histo_buffer_size, 1, CV_32F, hist_data);

    cv::minMaxLoc(src_hist_data, 0, &max_histo_val, 0, &max_histo_point);

    // 最頻値
    *max_frequency_value = max_histo_point.y / (float)histogram_data->resolution;

    // (debug)Histogramの結果を表示する
    bool show_histo_image = false;
    if (show_histo_image) {
        // 分解能1でHistogramを作成して表示します
        hist_data = histogram_data->buffer;
        memset(hist_data, 0, sizeof(float) * histogram_data->buffer_size);

        for (int i = 0; i < src_data.rows; i++) {
            float* src = src_data.ptr<float>(i);

            for (int j = 0; j < src_data.cols; j++) {
                int index = (int)(*src);
                if (index < histogram_data->buffer_size) {
                    hist_data[index]++;
                }
                src++;
            }
        }
        // 無効分のデータをクリアする
        int start = 0;
        int end = (int)(min_valid_value + 0.5);
        for (int i = start; i < end; i++) {
            hist_data[i] = 0;
        }

        cv::Mat display_hist_data(histogram_data->buffer_size, 1, CV_32F, hist_data);

        int hist_w = 512, hist_h = 400;
        int bin_w = cvRound((double)hist_w / histogram_data->buffer_size);
        cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(0, 0, 0));
        cv::normalize(display_hist_data, display_hist_data, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());

        for (int i = 1; i < histogram_data->buffer_size; i++) {
            line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(display_hist_data.at<float>(i - 1))),
                cv::Point(bin_w * (i), hist_h - cvRound(display_hist_data.at<float>(i))),
                cv::Scalar(255, 0, 0), 2, 8, 0);
        }
        imshow("Disparity Histogram of ROI", histImage);
        cv::waitKey();
    }


    // OpenCVを使ったサンプル
#if 0

    // 視差を 量子化します
    //int hist_size= 256;
    int hist_size = (int)(256.0 / 0.25);

    // 視差の範囲は 0 から 255 です．
    float ranges[] = { 0.0F, 255.0F };
    const float* hist_ranges[] = { ranges };

    cv::MatND hist;
    // 0 番目のチャンネルからヒストグラムを求めます．
    int channels[] = { 0 };

    bool uniform = true, accumulate = false;

    cv::calcHist(
        &src_data, 1, channels, cv::Mat(),  // マスクは利用しません
        hist, 1, &hist_size, hist_ranges,
        uniform,                            // ヒストグラムは一様です
        accumulate);

    double max_val = 0;
    cv::Point max_point = {};

    hist.at<float>(0) = 0;
    cv:minMaxLoc(hist, 0, &max_val, 0, &max_point);

    // 最頻値
    *max_frequency_value = max_point.y * 0.25;
    
    // (debug)Histogramの結果を表示する
    bool show_cv_histo_image = false;
    if (show_cv_histo_image) {
        int hist_w = 512, hist_h = 400;
        int bin_w = cvRound((double)hist_w / hist_size);
        cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(0, 0, 0));
        cv::normalize(hist, hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());

        for (int i = 1; i < hist_size; i++)
        {
            line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
                cv::Point(bin_w * (i), hist_h - cvRound(hist.at<float>(i))),
                cv::Scalar(255, 0, 0), 2, 8, 0);
        }
        imshow("calcHist Demo", histImage);
        cv::waitKey();
    }
#endif

    return 0;
}

int GetMaxDistanceValueInHistogram(cv::Mat src_data, const float min_valid_value, float* max_frequency_value, HistogramData* histogram_data)
{
    // Histogram
    /*
        Histogramの最頻値を取得します

        最大値により分解能を変更します

        最大値　分解能
        <10m    0.01m (10mm)
        <100m   0.1m (100mm)
        <1000m  1m

    */

    // 最大最小の取得
    double minVal = 0;
    double maxVal = 0;
    cv::Point minLoc = {};
    cv::Point maxLoc = {};
        
    cv::minMaxLoc(
                    src_data,
                    &minVal,
                    &maxVal,
                    &minLoc,
                    &maxLoc);

    if (maxVal > 1000.0) {
        // 範囲外
        *max_frequency_value = 0;
        return -1;
    }

    double resolution = 1.0;
    if (maxVal < 10.0) {
        resolution = 100.0;
    }
    else if (maxVal < 100.0) {
        resolution = 10.0;
    }
    else if (maxVal < 1000.0) {
        resolution = 1.0;
    }

    float* hist_data = histogram_data->histo_buffer;
    memset(hist_data, 0, sizeof(float) * histogram_data->histo_buffer_size);

    for (int i = 0; i < src_data.rows; i++) {
        float* src = src_data.ptr<float>(i);

        for (int j = 0; j < src_data.cols; j++) {
            int index = (int)(*src * (float)resolution);
            if (index < histogram_data->histo_buffer_size) {
                hist_data[index]++;
            }
            src++;
        }
    }

    // 無効分のデータをクリアする
    int start = 0;
    int end = (int)(min_valid_value + 0.5);
    for (int i = start; i < end; i++) {
        hist_data[i] = 0;
    }
    double max_histo_val = 0;
    cv::Point max_histo_point = {};
    cv::Mat src_hist_data(histogram_data->histo_buffer_size, 1, CV_32F, hist_data);

    cv::minMaxLoc(src_hist_data, 0, &max_histo_val, 0, &max_histo_point);

    // 最頻値
    *max_frequency_value = max_histo_point.y / (float)resolution;

    // (debug)Histogramの結果を表示する
    bool show_histo_image = false;
    if (show_histo_image) {
        // 分解能1でHistogramを作成して表示します
        hist_data = histogram_data->buffer;
        memset(hist_data, 0, sizeof(float) * histogram_data->buffer_size);

        for (int i = 0; i < src_data.rows; i++) {
            float* src = src_data.ptr<float>(i);

            for (int j = 0; j < src_data.cols; j++) {
                int index = (int)(*src * (float)resolution);
                if (index < histogram_data->buffer_size) {
                    hist_data[index]++;
                }
                src++;
            }
        }
        // 無効分のデータをクリアする
        int start = 0;
        int end = (int)(min_valid_value + 0.5);
        for (int i = start; i < end; i++) {
            hist_data[i] = 0;
        }

        cv::Mat display_hist_data(histogram_data->buffer_size, 1, CV_32F, hist_data);

        int hist_w = 512, hist_h = 400;
        int bin_w = cvRound((double)hist_w / histogram_data->buffer_size);
        cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(0, 0, 0));
        cv::normalize(display_hist_data, display_hist_data, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());

        for (int i = 1; i < histogram_data->buffer_size; i++) {
            line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(display_hist_data.at<float>(i - 1))),
                cv::Point(bin_w * (i), hist_h - cvRound(display_hist_data.at<float>(i))),
                cv::Scalar(255, 0, 0), 2, 8, 0);
        }
        imshow("Depth Histogram of ROI", histImage);
        cv::waitKey();
    }

    return 0;
}