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
 * @file dpc_draw_lib.cpp
 * @brief implementation class of camera cntrol
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a funtion for using camera.
 */
 
#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <D2d1.h>
#include <stdint.h>

#include "opencv2\opencv.hpp"

#include "dpc_image_writer.h"
#include "dpc_draw_lib.h"

constexpr double kMaxDisparityValue = 255.0;

DpcDrawLib::DpcDrawLib()
{
	show_elapsed_time_ = false;
	QueryPerformanceFrequency(&performance_freq_);

	max_width_ = 0;
	max_height_ = 0;

	disp_color_map_distance_.min_value = 0;
	disp_color_map_distance_.max_value = 0;
	disp_color_map_distance_.color_map_size = 0;
	disp_color_map_distance_.color_map = nullptr;
	disp_color_map_distance_.color_map_step = 0.01;

	disp_color_map_disparity_.min_value = 0;
	disp_color_map_disparity_.max_value = 0;
	disp_color_map_disparity_.color_map_size = 0;
	disp_color_map_disparity_.color_map = nullptr;
	disp_color_map_disparity_.color_map_step = 1.0 / 4.0;

	max_disparity_ = kMaxDisparityValue;

	bitmap_[0] = bitmap_[1] = NULL;
	color_[0] = color_[1] = nullptr;
	for (int i = 0; i < 8; i++) {
		temp_buffer_[i] = nullptr;
	}

	width_color_[0] = width_color_[1] = 0;
	height_color_[0] = height_color_[1] = 0;
	width_[0] = width_[1] = 0;
	height_[0] = height_[1] = 0;

	memset(&display_information_, 0, sizeof(display_information_));

	d2d_factory_ = nullptr;
	dc_render_target_ = nullptr;
	dwrite_factory_ = nullptr;

	brush_ = nullptr;
	brush_text_ = nullptr;
	brush_back_ = nullptr;
	text_format_ = nullptr;

	dpc_image_write_ = nullptr;

	// reset at CreateDeviceResourcesAlt()  
	draw_text_font_setting_.FontSize = 16;
	draw_text_font_setting_.y0 = (float)(draw_text_font_setting_.FontSize * 0 * 1.33);
	draw_text_font_setting_.y1 = (float)(draw_text_font_setting_.FontSize * 1 * 1.33);
	draw_text_font_setting_.y2 = (float)(draw_text_font_setting_.FontSize * 2 * 1.33);
	draw_text_font_setting_.y3 = (float)(draw_text_font_setting_.FontSize * 3 * 1.33);

	memset(&draw_parameter_, 0, sizeof(draw_parameter_));

	// create factory
	HRESULT hr = ::D2D1CreateFactory(	D2D1_FACTORY_TYPE_SINGLE_THREADED,
										IID_ID2D1Factory,
										(void**)&d2d_factory_);

	hr = ::DWriteCreateFactory(	DWRITE_FACTORY_TYPE_SHARED,
								__uuidof(dwrite_factory_),
								reinterpret_cast<IUnknown**>(&dwrite_factory_));

}

DpcDrawLib::~DpcDrawLib()
{

	if (dwrite_factory_ != nullptr) {
		dwrite_factory_->Release();
		dwrite_factory_ = nullptr;
	}

	if (d2d_factory_ != nullptr) {
		d2d_factory_->Release();
		d2d_factory_ = nullptr;
	}

}

bool DpcDrawLib::Open(const int max_width, const int max_height, const double min_distance, const double max_distance, const double max_disparity, const TCHAR* save_image_path)
{
	max_width_ = max_width;
	max_height_ = max_height;
	max_disparity_ = max_disparity;

	disp_color_map_distance_.min_value = min_distance;
	disp_color_map_distance_.max_value = max_distance;
	disp_color_map_distance_.color_map_size = 0;
	disp_color_map_distance_.color_map_step = 0.01;
	disp_color_map_distance_.color_map_size = (int)(max_distance / disp_color_map_distance_.color_map_step) + 1;
	disp_color_map_distance_.color_map = new int[disp_color_map_distance_.color_map_size + sizeof(int)];
	memset(disp_color_map_distance_.color_map, 0, disp_color_map_distance_.color_map_size + sizeof(int));
	BuildColorHeatMap(&disp_color_map_distance_);

	disp_color_map_disparity_.min_value = 0;
	disp_color_map_disparity_.max_value = max_disparity_;
	disp_color_map_disparity_.color_map_size = 0;
	disp_color_map_disparity_.color_map_step = 0.25;
	disp_color_map_disparity_.color_map_size = (int)(max_disparity_ / disp_color_map_disparity_.color_map_step) + 1;
	disp_color_map_disparity_.color_map = new int[disp_color_map_disparity_.color_map_size + sizeof(int)];
	memset(disp_color_map_disparity_.color_map, 0, disp_color_map_disparity_.color_map_size + sizeof(int));
	BuildColorHeatMapForDisparity(&disp_color_map_disparity_);

	color_[0] = new uint8_t[max_width_ * 3 * max_height_ * 4];
	color_[1] = new uint8_t[max_width_ * 3 * max_height_ * 4];

	for (int i = 0; i < 8; i++) {
		temp_buffer_[i] = new uint8_t[max_width_ * max_height_ * 4];
	}

	dpc_image_write_ = new DpcImageWriter;
	dpc_image_write_->Initialize(max_width_, max_height_, save_image_path);

	return true;
}

bool DpcDrawLib::Close()
{

	ReleaseResource();

	if (dpc_image_write_ != nullptr) {
		dpc_image_write_->Terminate();
		delete dpc_image_write_;
		dpc_image_write_ = nullptr;
	}

	delete[] disp_color_map_distance_.color_map;
	disp_color_map_distance_.color_map = nullptr;

	delete[] disp_color_map_disparity_.color_map;
	disp_color_map_disparity_.color_map = nullptr;

	for (int i = 0; i < 8; i++) {
		delete[] temp_buffer_[i];
		temp_buffer_[i] = nullptr;
	}

	delete[] color_[0];
	color_[0] = nullptr;

	delete[] color_[1];
	color_[1] = nullptr;

	return true;
}

void DpcDrawLib::ReleaseResource()
{
	if (bitmap_[0]) {
		bitmap_[0]->Release();
		bitmap_[0] = nullptr;
	}

	if (bitmap_[1]) {
		bitmap_[1]->Release();
		bitmap_[1] = nullptr;
	}

	if (text_format_ != nullptr) {
		text_format_->Release();
		text_format_ = nullptr;
	}

	if (brush_back_ != nullptr) {
		brush_back_->Release();
		brush_back_ = nullptr;
	}

	if (brush_text_ != nullptr) {
		brush_text_->Release();
		brush_text_ = nullptr;
	}

	if (brush_ != nullptr) {
		brush_->Release();
		brush_ = nullptr;
	}

	if (dc_render_target_ != nullptr) {
		dc_render_target_->Release();
		dc_render_target_ = nullptr;
	}

	return;
}

void DpcDrawLib::GetMinMaxDistance(double* min_distance, double* max_distance) const
{
	*min_distance = disp_color_map_distance_.min_value;
	*max_distance = disp_color_map_distance_.max_value;
}

void DpcDrawLib::RebuildDrawColorMap(const double min_distance, const double max_distance)
{
	delete[] disp_color_map_distance_.color_map;
	disp_color_map_distance_.color_map = nullptr;

	disp_color_map_distance_.min_value = min_distance;
	disp_color_map_distance_.max_value = max_distance;
	disp_color_map_distance_.color_map_size = 0;
	disp_color_map_distance_.color_map_step = 0.01;
	disp_color_map_distance_.color_map_size = (int)(max_distance / disp_color_map_distance_.color_map_step) + 1;
	disp_color_map_distance_.color_map = new int[disp_color_map_distance_.color_map_size + sizeof(int)];
	memset(disp_color_map_distance_.color_map, 0, disp_color_map_distance_.color_map_size + sizeof(int));
	BuildColorHeatMap(&disp_color_map_distance_);

	return;
}

void DpcDrawLib::GetDisplayMag(double* mag) const
{
	*mag = 1.0;

	return;
}

bool DpcDrawLib::InitializeImageDataSet(ImageDataSet* image_data_set)
{

	image_data_set->valid = false;
	image_data_set->mode = DpcDrawLib::ImageDrawMode::kBase;

	for (int j = 0; j < 2; j++) {
		image_data_set->image_data_list[j].image_base.width = 0;
		image_data_set->image_data_list[j].image_base.height = 0;
		image_data_set->image_data_list[j].image_base.channel_count = 0;
		image_data_set->image_data_list[j].image_base.buffer = new unsigned char[max_width_ * max_height_];

		image_data_set->image_data_list[j].image_compare.width = 0;
		image_data_set->image_data_list[j].image_compare.height = 0;
		image_data_set->image_data_list[j].image_compare.channel_count = 0;
		image_data_set->image_data_list[j].image_compare.buffer = new unsigned char[max_width_ * max_height_];

		image_data_set->image_data_list[j].depth.width = 0;
		image_data_set->image_data_list[j].depth.height = 0;
		image_data_set->image_data_list[j].depth.buffer = new float[max_width_ * max_height_];

		image_data_set->image_data_list[j].image_color.width = 0;
		image_data_set->image_data_list[j].image_color.height = 0;
		image_data_set->image_data_list[j].image_color.channel_count = 0;
		image_data_set->image_data_list[j].image_color.buffer = new unsigned char[max_width_ * max_height_ * 3];

		image_data_set->image_data_list[j].image_dpl.width = 0;
		image_data_set->image_data_list[j].image_dpl.height = 0;
		image_data_set->image_data_list[j].image_dpl.channel_count = 0;
		image_data_set->image_data_list[j].image_dpl.buffer = new unsigned char[max_width_ * max_height_ * 3];

		image_data_set->image_data_list[j].depth_dpl.width = 0;
		image_data_set->image_data_list[j].depth_dpl.height = 0;
		image_data_set->image_data_list[j].depth_dpl.buffer = new float[max_width_ * max_height_];
	}

	return true;
}

bool DpcDrawLib::ReleaseImageDataSet(ImageDataSet* image_data_set)
{
	image_data_set->valid = false;
	image_data_set->mode = DpcDrawLib::ImageDrawMode::kBase;

	for (int j = 0; j < 2; j++) {
		image_data_set->image_data_list[j].image_base.width = 0;
		image_data_set->image_data_list[j].image_base.height = 0;
		image_data_set->image_data_list[j].image_base.channel_count = 0;
		delete[] image_data_set->image_data_list[j].image_base.buffer;
		image_data_set->image_data_list[j].image_base.buffer = nullptr;

		image_data_set->image_data_list[j].image_compare.width = 0;
		image_data_set->image_data_list[j].image_compare.height = 0;
		image_data_set->image_data_list[j].image_compare.channel_count = 0;
		delete[] image_data_set->image_data_list[j].image_compare.buffer;
		image_data_set->image_data_list[j].image_compare.buffer = nullptr;

		image_data_set->image_data_list[j].depth.width = 0;
		image_data_set->image_data_list[j].depth.height = 0;
		delete[] image_data_set->image_data_list[j].depth.buffer;
		image_data_set->image_data_list[j].depth.buffer = nullptr;

		image_data_set->image_data_list[j].image_color.width = 0;
		image_data_set->image_data_list[j].image_color.height = 0;
		image_data_set->image_data_list[j].image_color.channel_count = 0;
		delete[] image_data_set->image_data_list[j].image_color.buffer;
		image_data_set->image_data_list[j].image_color.buffer = nullptr;

		image_data_set->image_data_list[j].image_dpl.width = 0;
		image_data_set->image_data_list[j].image_dpl.height = 0;
		image_data_set->image_data_list[j].image_dpl.channel_count = 0;
		delete[] image_data_set->image_data_list[j].image_dpl.buffer;
		image_data_set->image_data_list[j].image_dpl.buffer = nullptr;

		image_data_set->image_data_list[j].depth_dpl.width = 0;
		image_data_set->image_data_list[j].depth_dpl.height = 0;
		delete[] image_data_set->image_data_list[j].depth_dpl.buffer;
		image_data_set->image_data_list[j].depth_dpl.buffer = nullptr;
	}

	return true;
}

HRESULT DpcDrawLib::CreateDeviceResourcesAlt()  
{
	HRESULT hr = S_OK;

	// ファクトリが無ければ何もしない       
	if (d2d_factory_ == NULL) return E_FAIL;

	// DC互換のレンダーターゲットの作成(このブロックを変更)       
	if (dc_render_target_ == NULL)
	{
		D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(                       
																			D2D1_RENDER_TARGET_TYPE_DEFAULT,
																			D2D1::PixelFormat(
																				DXGI_FORMAT_B8G8R8A8_UNORM,
																				D2D1_ALPHA_MODE_IGNORE
																			), 0.0F, 0.0F,
																			D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE          
																		);

		hr = d2d_factory_->CreateDCRenderTarget(&props, &dc_render_target_);

		if (!SUCCEEDED(hr)) return hr;
	}

	// 赤枠ブラシの作成 
	if (brush_ == nullptr)
	{
		ASSERT(dc_render_target_ != nullptr);
		hr = dc_render_target_->CreateSolidColorBrush(D2D1::ColorF(1.0, 0.0, 0.0), &brush_);
	}
	// テキスト表示用ブラシの作成 
	if (brush_text_ == nullptr)
	{
		ASSERT(dc_render_target_ != nullptr);
		hr = dc_render_target_->CreateSolidColorBrush(D2D1::ColorF(0.0F, 0.63F, 0.9F), &brush_text_);
	}
	// テキスト背景用ブラシの作成 
	if (brush_back_ == nullptr)
	{
		ASSERT(dc_render_target_ != nullptr);
		hr = dc_render_target_->CreateSolidColorBrush(D2D1::ColorF(0.0F, 0.0F, 0.0F, 0.4F), &brush_back_);
	}

	if (text_format_ == nullptr)
	{
		static const WCHAR* msc_font_name = L"MS ゴシック"; // L"メイリオ";
		static const FLOAT msc_font_size = 16.0F;
		hr = dwrite_factory_->CreateTextFormat(	msc_font_name,
												NULL,
												DWRITE_FONT_WEIGHT_NORMAL,
												DWRITE_FONT_STYLE_NORMAL,
												DWRITE_FONT_STRETCH_NORMAL,
												msc_font_size,
												L"ja-jp", //locale
												&text_format_);

		text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	
		draw_text_font_setting_.FontSize = (int)msc_font_size;
		draw_text_font_setting_.y0 = (float)(draw_text_font_setting_.FontSize * 0 * 1.33);
		draw_text_font_setting_.y1 = (float)(draw_text_font_setting_.FontSize * 1 * 1.33);
		draw_text_font_setting_.y2 = (float)(draw_text_font_setting_.FontSize * 2 * 1.33);
		draw_text_font_setting_.y3 = (float)(draw_text_font_setting_.FontSize * 3 * 1.33);
	}

	return hr;
}

// 描画(別例)   
void DpcDrawLib::RenderAlt(HDC hDC, LPRECT pRect) 
{
	HRESULT hr;
	hr = CreateDeviceResourcesAlt();     
	if (!SUCCEEDED(hr)) {
		return;
	}

	hr = dc_render_target_->BindDC(hDC, pRect);
	dc_render_target_->BeginDraw();
	
	//背景クリア       
	dc_render_target_->Clear(D2D1::ColorF(1.0F, 1.0F, 1.0F));
	
	////円の描画       
	//D2D1_ELLIPSE ellipse1 = D2D1::Ellipse(D2D1::Point2F(120.0F, 120.0F), 100.0F, 100.0F);
	//dc_render_target_->DrawEllipse(ellipse1, green_brush_, 10.0F);
	//
	////1つ目の四角形の描画       
	//D2D1_RECT_F rect1 = D2D1::RectF(100.0F, 50.0F, 300.0F, 100.F);
	//dc_render_target_->FillRectangle(&rect1, blue_brush_);


	int width = 960;
	int height = 480;
	unsigned char* buffer = new unsigned char[width * height * 4];
	memset(buffer, 0, width * height * 4);
	for (int y = 0; y < height; y++) {

		unsigned char* pb = buffer + (y * width * 4);

		int v = 0;
		for (int x = 0; x < width; x++) {
			memset(pb, v, 4);
			v++;
			pb += 4;
		}
	}

	D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
	dc_render_target_->CreateBitmap
	(
		D2D1::SizeU(width, height),
		NULL,
		width * sizeof(UINT32),
		D2D1::BitmapProperties(pixelFormat),
		&bitmap_[0]
	);

	D2D1_RECT_U rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;
	bitmap_[0]->CopyFromMemory(&rect, buffer, width * sizeof(UINT32));

	D2D1_RECT_F RectF0 = {};
	RectF0.top		= (float)(pRect->top);
	RectF0.bottom	= (float)(pRect->bottom);
	RectF0.left		= (float)(pRect->left);
	RectF0.right	= (float)(pRect->right);

	dc_render_target_->DrawBitmap(bitmap_[0], &RectF0);

	dc_render_target_->EndDraw();

	delete[] buffer;

	return;
}

void  DpcDrawLib::BuildBitmap(const int index, const ImageDrawMode mode, TextDataSet* text_data_set, DrawParameter* draw_parameter, SIZE* target_size,
				DisplayInformation* diaplay_information, ImageDataList* image_data_list_0, ImageDataList* image_data_list_1)
{

	//enum class ImageDrawMode {
	//							/**< image_data_list	[0]					[1]				*/
	//	kBase,					/**< 0:					image_base							*/
	//	kCompare,				/**< 1:					image_compare						*/
	//	kDepth,					/**< 2:					depth								*/
	//	kColor,					/**< 3:					image_color							*/
	//	kBaseCompare,			/**< 4:					image_base,			image_compare	*/
	//	kDepthBase,				/**< 5:					depth_data,			image_base		*/
	//	kDepthColor,			/**< 6:					depth_data,			image_color		*/
	//	kOverlapedDepthBase,	/**< 7:					depth_data,			image_base		*/

	//	kDplImage,				/**< 8:					image_dpl							*/
	//	kDplImageBase,			/**< 9:					image_dpl,			image_base		*/
	//	kDplImageColor,			/**< 10:				image_dpl,			image_color		*/
	//	kDplDepth,				/**< 11:				depth_dpl							*/
	//	kDplDepthBase,			/**< 12:				depth_dpl,			image_base		*/
	//	kDplDepthColor,			/**< 13:				depth_dpl,			image_color		*/
	//	kDplDepthDepth,			/**< 14:				depth_dpl,			depth			*/
	//	kOverlapedDplDepthBase,	/**< 15:				depth_dpl,			image_base		*/
	//	kUnknown = 99			/**< 99:				(error case)						*/
	//};

	double magnification = draw_parameter->magnification;
	POINT magnification_center = draw_parameter->magnification_center;
	RECT zero_rect = {};

	DpcImageWriter::ImageDepthDataSet image_depth_data_set = {};

	diaplay_information->mode = mode;

	switch(mode){
	case ImageDrawMode::kBase:
		{
			ImageData* image_data_left = &image_data_list_0->image_base;
			if (image_data_left->width == 0 || image_data_left->height == 0) {
				return;
			}

			double ratio = 1.0;

			cv::Size image_size(image_data_left->width, image_data_left->height);
			cv::Mat mat_color_image0(image_size, CV_8UC4, color_[index]);

			if (image_data_left->channel_count == 1) {
				cv::Mat mat_src_image0(image_size, CV_8UC1, image_data_left->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC1, temp_buffer_[0]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
			}
			else if (image_data_left->channel_count == 3) {
				cv::Mat mat_src_image0(image_size, CV_8UC3, image_data_left->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC3, temp_buffer_[0]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			width_color_[index] = mat_color_image0.cols;
			height_color_[index] = mat_color_image0.rows;

			display_information_.original_image_size[0].cx = image_data_left->width;
			display_information_.original_image_size[0].cy = image_data_left->height;
			display_information_.original_image_size[1].cx = 0;
			display_information_.original_image_size[1].cy = 0;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = image_data_left->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = image_data_left->width;
			display_information_.rectangle_to_display[1] = zero_rect;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 1;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("BASE_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_left->width;
				image_depth_data_set.image_data[0].height = image_data_left->height;
				image_depth_data_set.image_data[0].channel_count = image_data_left->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_left->buffer;

				image_depth_data_set.depth_data_count = 0;
				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;
		
	case ImageDrawMode::kCompare:
		{
			ImageData* image_data_right = &image_data_list_0->image_compare;
			if (image_data_right->width == 0 || image_data_right->height == 0) {
				return;
			}

			double ratio = 1.0;

			cv::Size image_size(image_data_right->width, image_data_right->height);
			cv::Mat mat_color_image0(image_size, CV_8UC4, color_[index]);

			if (image_data_right->channel_count == 1) {
				cv::Mat mat_src_image0(image_size, CV_8UC1, image_data_right->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC1, temp_buffer_[0]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
			}
			else if (image_data_right->channel_count == 3) {
				cv::Mat mat_src_image0(image_size, CV_8UC3, image_data_right->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC3, temp_buffer_[0]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			width_color_[index] = mat_color_image0.cols;
			height_color_[index] = mat_color_image0.rows;		

			display_information_.original_image_size[0].cx = image_data_right->width;
			display_information_.original_image_size[0].cy = image_data_right->height;
			display_information_.original_image_size[1].cx = 0;
			display_information_.original_image_size[1].cy = 0;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = image_data_right->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = image_data_right->width;
			display_information_.rectangle_to_display[1] = zero_rect;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 1;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("COMPARE_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_right->width;
				image_depth_data_set.image_data[0].height = image_data_right->height;
				image_depth_data_set.image_data[0].channel_count = image_data_right->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_right->buffer;

				image_depth_data_set.depth_data_count = 0;
				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

	case ImageDrawMode::kDepth:
		{
			DepthData* depth_data = &image_data_list_0->depth;
			if (depth_data->width == 0 || depth_data->height == 0) {
				return;
			}

			double depth_ratio = 1.0;
			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(	draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
											disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
											depth_data->width, depth_data->height, depth_data->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_depth_image_scaleimage(cv::Size(depth_data->width, depth_data->height), CV_8UC4, color_[index]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image, mat_depth_image_scaleimage, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				//mat_depth_image_scaleimage = mat_depth_image.clone();
				memcpy(color_[index], temp_buffer_[2], depth_data->width * depth_data->height * 4);
			}
			width_color_[index] = mat_depth_image_scaleimage.cols;
			height_color_[index] = mat_depth_image_scaleimage.rows;

			display_information_.original_image_size[0].cx = depth_data->width;
			display_information_.original_image_size[0].cy = depth_data->height;
			display_information_.original_image_size[1].cx = 0;
			display_information_.original_image_size[1].cy = 0;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_data->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_data->width;
			display_information_.rectangle_to_display[1] = zero_rect;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 1;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("DEPTH_IMAGE"));
				image_depth_data_set.image_data[0].width = depth_data->width;
				image_depth_data_set.image_data[0].height = depth_data->height;
				image_depth_data_set.image_data[0].channel_count = 4;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = temp_buffer_[2];

				image_depth_data_set.depth_data_count = 1;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_data->width;
				image_depth_data_set.depth_data[0].height = depth_data->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_data->buffer;

				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

	case ImageDrawMode::kColor:
		{
			ImageData* image_data_color = &image_data_list_0->image_color;
			if (image_data_color->width == 0 || image_data_color->height == 0) {
				return;
			}

			double ratio = 1.0;

			cv::Size image_size(image_data_color->width, image_data_color->height);
			cv::Mat mat_color_image0(image_size, CV_8UC4, color_[index]);

			if (image_data_color->channel_count == 1) {
				// type error
			}
			else if (image_data_color->channel_count == 3) {
				cv::Mat mat_src_image0(image_size, CV_8UC3, image_data_color->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC3, temp_buffer_[0]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			width_color_[index] = mat_color_image0.cols;
			height_color_[index] = mat_color_image0.rows;

			display_information_.original_image_size[0].cx = image_data_color->width;
			display_information_.original_image_size[0].cy = image_data_color->height;
			display_information_.original_image_size[1].cx = 0;
			display_information_.original_image_size[1].cy = 0;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = image_data_color->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = image_data_color->width;
			display_information_.rectangle_to_display[1] = zero_rect;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 1;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("COLOR_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_color->width;
				image_depth_data_set.image_data[0].height = image_data_color->height;
				image_depth_data_set.image_data[0].channel_count = image_data_color->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_color->buffer;

				image_depth_data_set.depth_data_count = 0;
				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kBaseCompare:
		{
			ImageData* image_data_left = &image_data_list_0->image_base;
			ImageData* image_data_right = &image_data_list_1->image_compare;
			if (image_data_left->width == 0 || image_data_left->height == 0) {
				return;
			}
			if (image_data_right->width == 0 || image_data_right->height == 0) {
				return;
			}

			double ratio = 1.0;

			// 画像は同じサイズとする
			if ((image_data_left->width != image_data_right->width) || (image_data_left->height != image_data_right->height)) {
				return;
			}

			cv::Size image_size_left(image_data_left->width, image_data_left->height);
			cv::Mat mat_color_image0(image_size_left, CV_8UC4, temp_buffer_[0]);

			if (image_data_left->channel_count == 1) {
				cv::Mat mat_src_image0(image_size_left, CV_8UC1, image_data_left->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_left, CV_8UC1, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
			}
			else if (image_data_left->channel_count == 3) {
				cv::Mat mat_src_image0(image_size_left, CV_8UC3, image_data_left->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_left, CV_8UC3, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			cv::Size image_size_right(image_data_right->width, image_data_right->height);
			cv::Mat mat_color_image1(image_size_right, CV_8UC4, temp_buffer_[2]);

			if (image_data_right->channel_count == 1) {
				cv::Mat mat_src_image0(image_size_right, CV_8UC1, image_data_right->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_right, CV_8UC1, temp_buffer_[3]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
			}
			else if (image_data_right->channel_count == 3) {
				cv::Mat mat_src_image0(image_size_right, CV_8UC3, image_data_right->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_right, CV_8UC3, temp_buffer_[3]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			// 表示用の合わせた画像を作成
			int combine_height = mat_color_image0.rows;
			int combine_width = mat_color_image0.cols + mat_color_image1.cols;

			cv::Mat combined_img(combine_height, combine_width, CV_8UC4, color_[index]);

			cv::Mat roi1(combined_img, cv::Rect(0, 0, mat_color_image0.cols, mat_color_image0.rows));
			mat_color_image0.copyTo(roi1);

			cv::Mat roi2(combined_img, cv::Rect(mat_color_image0.cols, 0, mat_color_image1.cols, mat_color_image1.rows));
			mat_color_image1.copyTo(roi2);

			width_color_[index] = combined_img.cols;
			height_color_[index] = combined_img.rows;

			display_information_.original_image_size[0].cx = image_data_left->width;
			display_information_.original_image_size[0].cy = image_data_left->height;
			display_information_.original_image_size[1].cx = image_data_right->width;
			display_information_.original_image_size[1].cy = image_data_right->height;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = image_data_left->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = image_data_left->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_right->height;
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_right->width;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("BASE_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_left->width;
				image_depth_data_set.image_data[0].height = image_data_left->height;
				image_depth_data_set.image_data[0].channel_count = image_data_left->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_left->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("COMPARE_IMAGE"));
				image_depth_data_set.image_data[1].width = image_data_right->width;
				image_depth_data_set.image_data[1].height = image_data_right->height;
				image_depth_data_set.image_data[1].channel_count = image_data_right->channel_count;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = image_data_right->buffer;

				image_depth_data_set.depth_data_count = 0;
				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDepthBase:
		{
			DepthData* depth_data = &image_data_list_0->depth;
			ImageData* image_data_right = &image_data_list_1->image_base;
			if (depth_data->width == 0 || depth_data->height == 0) {
				return;
			}
			if (image_data_right->width == 0 || image_data_right->height == 0) {
				return;
			}

			// 高さを合わせます
			double right_ratio = 1.0;
			double depth_ratio = 1.0;
			if (image_data_right->height != depth_data->height) {
				if (image_data_right->height > depth_data->height) {
					right_ratio = (double)depth_data->height / (double)image_data_right->height;
					right_ratio = std::round(right_ratio * 10) / 10;
				}
				else {
					depth_ratio = (double)image_data_right->height / (double)depth_data->height;
					depth_ratio = std::round(depth_ratio * 10) / 10;
				}
			}

			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_data->width, depth_data->height, depth_data->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_color_image0(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[3]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image, mat_color_image0, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				mat_color_image0 = mat_depth_image.clone();
			}

			cv::Size image_size(image_data_right->width, image_data_right->height);
			cv::Mat mat_color_image1(image_size, CV_8UC4, temp_buffer_[0]);

			if (image_data_right->channel_count == 1) {
				cv::Mat mat_src_image0(image_size, CV_8UC1, image_data_right->buffer);

				if (right_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC1, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), right_ratio, right_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
			}
			else {
				// error
			}

			// 表示用の合わせた画像を作成
			int combine_height = mat_color_image0.rows;
			int combine_width = mat_color_image0.cols + mat_color_image1.cols;

			cv::Mat combined_img(combine_height, combine_width, CV_8UC4, color_[index]);

			cv::Mat roi1(combined_img, cv::Rect(0, 0, mat_color_image0.cols, mat_color_image0.rows));
			mat_color_image0.copyTo(roi1);

			cv::Mat roi2(combined_img, cv::Rect(mat_color_image0.cols, 0, mat_color_image1.cols, mat_color_image1.rows));
			mat_color_image1.copyTo(roi2);

			width_color_[index] = combined_img.cols;
			height_color_[index] = combined_img.rows;

			display_information_.original_image_size[0].cx = depth_data->width;
			display_information_.original_image_size[0].cy = depth_data->height;
			display_information_.original_image_size[1].cx = image_data_right->width; 
			display_information_.original_image_size[1].cy = image_data_right->height; 
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_data->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_data->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_right->height; 
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_right->width; 

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;
				
				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("BASE_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_right->width;
				image_depth_data_set.image_data[0].height = image_data_right->height;
				image_depth_data_set.image_data[0].channel_count = image_data_right->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_right->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DEPTH_IMAGE"));
				image_depth_data_set.image_data[1].width = depth_data->width;
				image_depth_data_set.image_data[1].height = depth_data->height;
				image_depth_data_set.image_data[1].channel_count = 4;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = temp_buffer_[2];

				image_depth_data_set.depth_data_count = 1;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_data->width;
				image_depth_data_set.depth_data[0].height = depth_data->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_data->buffer;
				
				image_depth_data_set.pcd_data_count = 1;
				_stprintf_s(image_depth_data_set.pcd_data[0].id_string, _T("PCD_DATA"));
				image_depth_data_set.pcd_data[0].width = depth_data->width;
				image_depth_data_set.pcd_data[0].height = depth_data->height;
				image_depth_data_set.pcd_data[0].channel_count = image_data_right->channel_count;
				image_depth_data_set.pcd_data[0].is_rotate = true;

				image_depth_data_set.pcd_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.pcd_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.pcd_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.pcd_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.pcd_data[0].min_distance = disp_color_map_distance_.min_value;
				image_depth_data_set.pcd_data[0].max_distance = disp_color_map_distance_.max_value;

				image_depth_data_set.pcd_data[0].image = image_data_right->buffer;
				image_depth_data_set.pcd_data[0].depth = depth_data->buffer;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDepthColor:
		{
			DepthData* depth_data = &image_data_list_0->depth;
			ImageData* image_data_color = &image_data_list_1->image_color;
			if (depth_data->width == 0 || depth_data->height == 0) {
				return;
			}
			if (image_data_color->width == 0 || image_data_color->height == 0) {
				return;
			}

			// 高さを合わせます
			double color_ratio = 1.0;
			double depth_ratio = 1.0;
			if (image_data_color->height != depth_data->height) {
				if (image_data_color->height > depth_data->height) {
					color_ratio = (double)depth_data->height / (double)image_data_color->height;
					color_ratio = std::round(color_ratio * 10) / 10;
				}
				else {
					depth_ratio = (double)image_data_color->height / (double)depth_data->height;
					depth_ratio = std::round(depth_ratio * 10) / 10;
				}
			}

			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_data->width, depth_data->height, depth_data->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_color_image0(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[3]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image, mat_color_image0, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				mat_color_image0 = mat_depth_image.clone();
			}

			cv::Size image_size(image_data_color->width, image_data_color->height);
			cv::Mat mat_color_image1(image_size, CV_8UC4, temp_buffer_[0]);

			if (image_data_color->channel_count == 1) {
				// type error
			}
			else if (image_data_color->channel_count == 3) {
				cv::Mat mat_src_image0(image_size, CV_8UC3, image_data_color->buffer);

				if (color_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC3, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), color_ratio, color_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			// 表示用の合わせた画像を作成
			int combine_height = mat_color_image0.rows;
			int combine_width = mat_color_image0.cols + mat_color_image1.cols;

			cv::Mat combined_img(combine_height, combine_width, CV_8UC4, color_[index]);

			cv::Mat roi1(combined_img, cv::Rect(0, 0, mat_color_image0.cols, mat_color_image0.rows));
			mat_color_image0.copyTo(roi1);

			cv::Mat roi2(combined_img, cv::Rect(mat_color_image0.cols, 0, mat_color_image1.cols, mat_color_image1.rows));
			mat_color_image1.copyTo(roi2);

			width_color_[index] = combined_img.cols;
			height_color_[index] = combined_img.rows;

			display_information_.original_image_size[0].cx = depth_data->width;
			display_information_.original_image_size[0].cy = depth_data->height;
			display_information_.original_image_size[1].cx = image_data_color->width; 
			display_information_.original_image_size[1].cy = image_data_color->height; 
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_data->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_data->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_color->height; 
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_color->width; 

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("COLOR_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_color->width;
				image_depth_data_set.image_data[0].height = image_data_color->height;
				image_depth_data_set.image_data[0].channel_count = image_data_color->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_color->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DEPTH_IMAGE"));
				image_depth_data_set.image_data[1].width = depth_data->width;
				image_depth_data_set.image_data[1].height = depth_data->height;
				image_depth_data_set.image_data[1].channel_count = 4;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = temp_buffer_[2];

				image_depth_data_set.depth_data_count = 1;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_data->width;
				image_depth_data_set.depth_data[0].height = depth_data->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_data->buffer;

				image_depth_data_set.pcd_data_count = 1;
				_stprintf_s(image_depth_data_set.pcd_data[0].id_string, _T("PCD_DATA"));
				image_depth_data_set.pcd_data[0].width = depth_data->width;
				image_depth_data_set.pcd_data[0].height = depth_data->height;
				image_depth_data_set.pcd_data[0].channel_count = image_data_color->channel_count;
				image_depth_data_set.pcd_data[0].is_rotate = true;

				image_depth_data_set.pcd_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.pcd_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.pcd_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.pcd_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.pcd_data[0].min_distance = disp_color_map_distance_.min_value;
				image_depth_data_set.pcd_data[0].max_distance = disp_color_map_distance_.max_value;

				image_depth_data_set.pcd_data[0].image = image_data_color->buffer;
				image_depth_data_set.pcd_data[0].depth = depth_data->buffer;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kOverlapedDepthBase:
		{
			DepthData* depth_data = &image_data_list_0->depth;
			ImageData* image_data_right = &image_data_list_1->image_base;
			if (depth_data->width == 0 || depth_data->height == 0) {
				return;
			}
			if (image_data_right->width == 0 || image_data_right->height == 0) {
				return;
			}

			// 高さを合わせます
			double right_ratio = 1.0;
			double depth_ratio = 1.0;
			if (image_data_right->height != depth_data->height) {
				if (image_data_right->height > depth_data->height) {
					right_ratio = (double)depth_data->height / (double)image_data_right->height;
					right_ratio = std::round(right_ratio * 10) / 10;
				}
				else {
					depth_ratio = (double)image_data_right->height / (double)depth_data->height;
					depth_ratio = std::round(depth_ratio * 10) / 10;
				}
			}

			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_data->width, depth_data->height, depth_data->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_color_image0(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[3]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image, mat_color_image0, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				mat_color_image0 = mat_depth_image.clone();
			}

			cv::Size image_size(image_data_right->width, image_data_right->height);
			cv::Mat mat_color_image1(image_size, CV_8UC4, temp_buffer_[0]);

			if (image_data_right->channel_count == 1) {
				cv::Mat mat_src_image0(image_size, CV_8UC1, image_data_right->buffer);

				if (right_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC1, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), right_ratio, right_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
			}
			else {
				// error
			}

			//ブレンドを行う
			double alpha = 0.7;
			double beta = (1.0 - alpha);

			int new_height = mat_color_image1.rows;
			int new_width = mat_color_image1.cols;
			cv::Mat matBlendImageColor(new_height, new_width, CV_8UC4, color_[index]);

			cv::addWeighted(mat_color_image1, alpha, mat_color_image0, beta, 0.0, matBlendImageColor);

			width_color_[index] = matBlendImageColor.cols;
			height_color_[index] = matBlendImageColor.rows;

			display_information_.original_image_size[0].cx = depth_data->width;
			display_information_.original_image_size[0].cy = depth_data->height;
			display_information_.original_image_size[1].cx = image_data_right->width;
			display_information_.original_image_size[1].cy = image_data_right->height; 
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_data->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_data->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_right->height; 
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_right->width;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("BASE_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_right->width;
				image_depth_data_set.image_data[0].height = image_data_right->height;
				image_depth_data_set.image_data[0].channel_count = image_data_right->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_right->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DEPTH_IMAGE"));
				image_depth_data_set.image_data[1].width = depth_data->width;
				image_depth_data_set.image_data[1].height = depth_data->height;
				image_depth_data_set.image_data[1].channel_count = 4;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = temp_buffer_[2];

				image_depth_data_set.depth_data_count = 1;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_data->width;
				image_depth_data_set.depth_data[0].height = depth_data->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_data->buffer;

				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDplImage:
		{
			ImageData* image_dp_result = &image_data_list_0->image_dpl;
			if (image_dp_result->width == 0 || image_dp_result->height == 0) {
				return;
			}

			double ratio = 1.0;

			cv::Size image_size(image_dp_result->width, image_dp_result->height);
			cv::Mat mat_color_image0(image_size, CV_8UC4, color_[index]);

			if (image_dp_result->channel_count == 1) {
				cv::Mat mat_src_image0(image_size, CV_8UC1, image_dp_result->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC1, temp_buffer_[0]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
			}
			else if (image_dp_result->channel_count == 3) {
				cv::Mat mat_src_image0(image_size, CV_8UC3, image_dp_result->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC3, temp_buffer_[0]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			width_color_[index] = mat_color_image0.cols;
			height_color_[index] = mat_color_image0.rows;

			display_information_.original_image_size[0].cx = image_dp_result->width;
			display_information_.original_image_size[0].cy = image_dp_result->height;
			display_information_.original_image_size[1].cx = 0;
			display_information_.original_image_size[1].cy = 0;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = image_dp_result->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = image_dp_result->width;
			display_information_.rectangle_to_display[1] = zero_rect;		

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 1;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("DPL_IMAGE"));
				image_depth_data_set.image_data[0].width = image_dp_result->width;
				image_depth_data_set.image_data[0].height = image_dp_result->height;
				image_depth_data_set.image_data[0].channel_count = image_dp_result->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_dp_result->buffer;

				image_depth_data_set.depth_data_count = 0;
				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDplImageBase:
		{
			ImageData* image_dp_result = &image_data_list_0->image_dpl;
			ImageData* image_data_right = &image_data_list_1->image_base;
			if (image_dp_result->width == 0 || image_dp_result->height == 0) {
				return;
			}
			if (image_data_right->width == 0 || image_data_right->height == 0) {
				return;
			}

			double ratio = 1.0;

			// 画像は同じサイズとする
			if ((image_dp_result->width != image_data_right->width) || (image_dp_result->height != image_data_right->height)) {
				return;
			}

			cv::Size image_size_left(image_dp_result->width, image_dp_result->height);
			cv::Mat mat_color_image0(image_size_left, CV_8UC4, temp_buffer_[0]);

			if (image_dp_result->channel_count == 1) {
				cv::Mat mat_src_image0(image_size_left, CV_8UC1, image_dp_result->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_left, CV_8UC1, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
			}
			else if (image_dp_result->channel_count == 3) {
				cv::Mat mat_src_image0(image_size_left, CV_8UC3, image_dp_result->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_left, CV_8UC3, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			cv::Size image_size_right(image_data_right->width, image_data_right->height);
			cv::Mat mat_color_image1(image_size_right, CV_8UC4, temp_buffer_[2]);

			if (image_data_right->channel_count == 1) {
				cv::Mat mat_src_image0(image_size_right, CV_8UC1, image_data_right->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_right, CV_8UC1, temp_buffer_[3]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
			}
			else if (image_data_right->channel_count == 3) {
				cv::Mat mat_src_image0(image_size_right, CV_8UC3, image_data_right->buffer);

				if (ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_right, CV_8UC3, temp_buffer_[3]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			// 表示用の合わせた画像を作成
			int combine_height = mat_color_image0.rows;
			int combine_width = mat_color_image0.cols + mat_color_image1.cols;

			cv::Mat combined_img(combine_height, combine_width, CV_8UC4, color_[index]);

			cv::Mat roi1(combined_img, cv::Rect(0, 0, mat_color_image0.cols, mat_color_image0.rows));
			mat_color_image0.copyTo(roi1);

			cv::Mat roi2(combined_img, cv::Rect(mat_color_image0.cols, 0, mat_color_image1.cols, mat_color_image1.rows));
			mat_color_image1.copyTo(roi2);

			width_color_[index] = combined_img.cols;
			height_color_[index] = combined_img.rows;

			display_information_.original_image_size[0].cx = image_dp_result->width;
			display_information_.original_image_size[0].cy = image_dp_result->height;
			display_information_.original_image_size[1].cx = image_data_right->width;
			display_information_.original_image_size[1].cy = image_data_right->height;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = image_dp_result->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = image_dp_result->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_right->height;
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_right->width;		
		
			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("BASE_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_right->width;
				image_depth_data_set.image_data[0].height = image_data_right->height;
				image_depth_data_set.image_data[0].channel_count = image_data_right->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_right->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DPL_IMAGE"));
				image_depth_data_set.image_data[1].width = image_dp_result->width;
				image_depth_data_set.image_data[1].height = image_dp_result->height;
				image_depth_data_set.image_data[1].channel_count = image_dp_result->channel_count;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = image_dp_result->buffer;

				image_depth_data_set.depth_data_count = 0;
				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDplImageColor:
		{
			ImageData* image_dp_result = &image_data_list_0->image_dpl;
			ImageData* image_data_color = &image_data_list_1->image_color;
			if (image_dp_result->width == 0 || image_dp_result->height == 0) {
				return;
			}
			if (image_data_color->width == 0 || image_data_color->height == 0) {
				return;
			}

			// 高さを合わせます
			double color_ratio = 1.0;
			double dp_result_ratio = 1.0;
			if (image_data_color->height != image_dp_result->height) {
				if (image_data_color->height > image_dp_result->height) {
					color_ratio = (double)image_dp_result->height / (double)image_data_color->height;
					color_ratio = std::round(color_ratio * 10) / 10;
				}
				else {
					dp_result_ratio = (double)image_data_color->height / (double)image_dp_result->height;
					dp_result_ratio = std::round(dp_result_ratio * 10) / 10;
				}
			}

			cv::Size image_size_left(image_dp_result->width, image_dp_result->height);
			cv::Mat mat_color_image0(image_size_left, CV_8UC4, temp_buffer_[0]);

			if (image_dp_result->channel_count == 1) {
				cv::Mat mat_src_image0(image_size_left, CV_8UC1, image_dp_result->buffer);

				if (dp_result_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_left, CV_8UC1, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), dp_result_ratio, dp_result_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_GRAY2BGRA);
				}
			}
			else if (image_dp_result->channel_count == 3) {
				cv::Mat mat_src_image0(image_size_left, CV_8UC3, image_dp_result->buffer);

				if (dp_result_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size_left, CV_8UC3, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), dp_result_ratio, dp_result_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image0, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			cv::Size image_size(image_data_color->width, image_data_color->height);
			cv::Mat mat_color_image1(image_size, CV_8UC4, temp_buffer_[0]);

			if (image_data_color->channel_count == 1) {
				// type error
			}
			else if (image_data_color->channel_count == 3) {
				cv::Mat mat_src_image0(image_size, CV_8UC3, image_data_color->buffer);

				if (color_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC3, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), color_ratio, color_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			// 表示用の合わせた画像を作成
			int combine_height = mat_color_image0.rows;
			int combine_width = mat_color_image0.cols + mat_color_image1.cols;

			cv::Mat combined_img(combine_height, combine_width, CV_8UC4, color_[index]);

			cv::Mat roi1(combined_img, cv::Rect(0, 0, mat_color_image0.cols, mat_color_image0.rows));
			mat_color_image0.copyTo(roi1);

			cv::Mat roi2(combined_img, cv::Rect(mat_color_image0.cols, 0, mat_color_image1.cols, mat_color_image1.rows));
			mat_color_image1.copyTo(roi2);

			width_color_[index] = combined_img.cols;
			height_color_[index] = combined_img.rows;

			display_information_.original_image_size[0].cx = image_dp_result->width;
			display_information_.original_image_size[0].cy = image_dp_result->height;
			display_information_.original_image_size[1].cx = image_data_color->width;
			display_information_.original_image_size[1].cy = image_data_color->height;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = image_dp_result->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = image_dp_result->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_color->height;
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_color->width;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("COLOR_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_color->width;
				image_depth_data_set.image_data[0].height = image_data_color->height;
				image_depth_data_set.image_data[0].channel_count = image_data_color->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_color->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DPL_IMAGE"));
				image_depth_data_set.image_data[1].width = image_dp_result->width;
				image_depth_data_set.image_data[1].height = image_dp_result->height;
				image_depth_data_set.image_data[1].channel_count = image_dp_result->channel_count;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = image_dp_result->buffer;

				image_depth_data_set.depth_data_count = 0;
				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDplDepth:
		{
			DepthData* depth_dpresult = &image_data_list_0->depth_dpl;
			if (depth_dpresult->width == 0 || depth_dpresult->height == 0) {
				return;
			}

			double depth_ratio = 1.0;
			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_dpresult->width, depth_dpresult->height, depth_dpresult->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_depth_image_scaleimage(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, color_[index]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image, mat_depth_image_scaleimage, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				//mat_depth_image_scaleimage = mat_depth_image.clone();
				memcpy(color_[index], temp_buffer_[2], depth_dpresult->width * depth_dpresult->height * 4);
			}
			width_color_[index] = mat_depth_image_scaleimage.cols;
			height_color_[index] = mat_depth_image_scaleimage.rows;

			display_information_.original_image_size[0].cx = depth_dpresult->width;
			display_information_.original_image_size[0].cy = depth_dpresult->height;
			display_information_.original_image_size[1].cx = 0;
			display_information_.original_image_size[1].cy = 0;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_dpresult->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_dpresult->width;
			display_information_.rectangle_to_display[1] = zero_rect;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 1;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("DPL_DEPTH_IMAGE"));
				image_depth_data_set.image_data[0].width = depth_dpresult->width;
				image_depth_data_set.image_data[0].height = depth_dpresult->height;
				image_depth_data_set.image_data[0].channel_count = 4;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = temp_buffer_[2];

				image_depth_data_set.depth_data_count = 1;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DPL_DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_dpresult->width;
				image_depth_data_set.depth_data[0].height = depth_dpresult->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_dpresult->buffer;

				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDplDepthBase:
		{
			DepthData* depth_dpresult = &image_data_list_0->depth_dpl;
			ImageData* image_data_right = &image_data_list_1->image_base;
			if (depth_dpresult->width == 0 || depth_dpresult->height == 0) {
				return;
			}
			if (image_data_right->width == 0 || image_data_right->height == 0) {
				return;
			}

			// 高さを合わせます
			double right_ratio = 1.0;
			double depth_ratio = 1.0;
			if (image_data_right->height != depth_dpresult->height) {
				if (image_data_right->height > depth_dpresult->height) {
					right_ratio = (double)depth_dpresult->height / (double)image_data_right->height;
					right_ratio = std::round(right_ratio * 10) / 10;
				}
				else {
					depth_ratio = (double)image_data_right->height / (double)depth_dpresult->height;
					depth_ratio = std::round(depth_ratio * 10) / 10;
				}
			}

			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_dpresult->width, depth_dpresult->height, depth_dpresult->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_color_image0(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[3]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image, mat_color_image0, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				mat_color_image0 = mat_depth_image.clone();
			}

			cv::Size image_size(image_data_right->width, image_data_right->height);
			cv::Mat mat_color_image1(image_size, CV_8UC4, temp_buffer_[0]);

			if (image_data_right->channel_count == 1) {
				cv::Mat mat_src_image0(image_size, CV_8UC1, image_data_right->buffer);

				if (right_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC1, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), right_ratio, right_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
			}
			else {
				// error
			}

			// 表示用の合わせた画像を作成
			int combine_height = mat_color_image0.rows;
			int combine_width = mat_color_image0.cols + mat_color_image1.cols;

			cv::Mat combined_img(combine_height, combine_width, CV_8UC4, color_[index]);

			cv::Mat roi1(combined_img, cv::Rect(0, 0, mat_color_image0.cols, mat_color_image0.rows));
			mat_color_image0.copyTo(roi1);

			cv::Mat roi2(combined_img, cv::Rect(mat_color_image0.cols, 0, mat_color_image1.cols, mat_color_image1.rows));
			mat_color_image1.copyTo(roi2);

			width_color_[index] = combined_img.cols;
			height_color_[index] = combined_img.rows;

			display_information_.original_image_size[0].cx = depth_dpresult->width;
			display_information_.original_image_size[0].cy = depth_dpresult->height;
			display_information_.original_image_size[1].cx = image_data_right->width; 
			display_information_.original_image_size[1].cy = image_data_right->height; 
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_dpresult->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_dpresult->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_right->height; 
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_right->width; 

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("BASE_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_right->width;
				image_depth_data_set.image_data[0].height = image_data_right->height;
				image_depth_data_set.image_data[0].channel_count = image_data_right->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_right->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DPL_DEPTH_IMAGE"));
				image_depth_data_set.image_data[1].width = depth_dpresult->width;
				image_depth_data_set.image_data[1].height = depth_dpresult->height;
				image_depth_data_set.image_data[1].channel_count = 4;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = temp_buffer_[2];

				image_depth_data_set.depth_data_count = 1;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DPL_DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_dpresult->width;
				image_depth_data_set.depth_data[0].height = depth_dpresult->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_dpresult->buffer;

				image_depth_data_set.pcd_data_count = 1;
				_stprintf_s(image_depth_data_set.pcd_data[0].id_string, _T("PCD_DATA"));
				image_depth_data_set.pcd_data[0].width = depth_dpresult->width;
				image_depth_data_set.pcd_data[0].height = depth_dpresult->height;
				image_depth_data_set.pcd_data[0].channel_count = image_data_right->channel_count;
				image_depth_data_set.pcd_data[0].is_rotate = true;

				image_depth_data_set.pcd_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.pcd_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.pcd_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.pcd_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.pcd_data[0].min_distance = disp_color_map_distance_.min_value;
				image_depth_data_set.pcd_data[0].max_distance = disp_color_map_distance_.max_value;

				image_depth_data_set.pcd_data[0].image = image_data_right->buffer;
				image_depth_data_set.pcd_data[0].depth = depth_dpresult->buffer;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDplDepthColor:
		{
			DepthData* depth_dpresult = &image_data_list_0->depth_dpl;
			ImageData* image_data_color = &image_data_list_1->image_color;
			if (depth_dpresult->width == 0 || depth_dpresult->height == 0) {
				return;
			}
			if (image_data_color->width == 0 || image_data_color->height == 0) {
				return;
			}

			// 高さを合わせます
			double color_ratio = 1.0;
			double depth_ratio = 1.0;
			if (image_data_color->height != depth_dpresult->height) {
				if (image_data_color->height > depth_dpresult->height) {
					color_ratio = (double)depth_dpresult->height / (double)image_data_color->height;
					color_ratio = std::round(color_ratio * 10) / 10;
				}
				else {
					depth_ratio = (double)image_data_color->height / (double)depth_dpresult->height;
					depth_ratio = std::round(depth_ratio * 10) / 10;
				}
			}

			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_dpresult->width, depth_dpresult->height, depth_dpresult->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_color_image0(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[3]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image, mat_color_image0, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				mat_color_image0 = mat_depth_image.clone();
			}

			cv::Size image_size(image_data_color->width, image_data_color->height);
			cv::Mat mat_color_image1(image_size, CV_8UC4, temp_buffer_[0]);

			if (image_data_color->channel_count == 1) {
				// type error
			}
			else if (image_data_color->channel_count == 3) {
				cv::Mat mat_src_image0(image_size, CV_8UC3, image_data_color->buffer);

				if (color_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC3, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), color_ratio, color_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_BGR2BGRA);
				}
			}
			else {
				// error 
				return;
			}

			// 表示用の合わせた画像を作成
			int combine_height = mat_color_image0.rows;
			int combine_width = mat_color_image0.cols + mat_color_image1.cols;

			cv::Mat combined_img(combine_height, combine_width, CV_8UC4, color_[index]);

			cv::Mat roi1(combined_img, cv::Rect(0, 0, mat_color_image0.cols, mat_color_image0.rows));
			mat_color_image0.copyTo(roi1);

			cv::Mat roi2(combined_img, cv::Rect(mat_color_image0.cols, 0, mat_color_image1.cols, mat_color_image1.rows));
			mat_color_image1.copyTo(roi2);

			width_color_[index] = combined_img.cols;
			height_color_[index] = combined_img.rows;

			display_information_.original_image_size[0].cx = depth_dpresult->width;
			display_information_.original_image_size[0].cy = depth_dpresult->height;
			display_information_.original_image_size[1].cx = image_data_color->width;
			display_information_.original_image_size[1].cy = image_data_color->height;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_dpresult->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_dpresult->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_color->height;
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_color->width;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("COLOR_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_color->width;
				image_depth_data_set.image_data[0].height = image_data_color->height;
				image_depth_data_set.image_data[0].channel_count = image_data_color->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_color->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DPL_DEPTH_IMAGE"));
				image_depth_data_set.image_data[1].width = depth_dpresult->width;
				image_depth_data_set.image_data[1].height = depth_dpresult->height;
				image_depth_data_set.image_data[1].channel_count = 4;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = temp_buffer_[2];

				image_depth_data_set.depth_data_count = 1;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DPL_DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_dpresult->width;
				image_depth_data_set.depth_data[0].height = depth_dpresult->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_dpresult->buffer;

				image_depth_data_set.pcd_data_count = 1;
				_stprintf_s(image_depth_data_set.pcd_data[0].id_string, _T("PCD_DATA"));
				image_depth_data_set.pcd_data[0].width = depth_dpresult->width;
				image_depth_data_set.pcd_data[0].height = depth_dpresult->height;
				image_depth_data_set.pcd_data[0].channel_count = image_data_color->channel_count;
				image_depth_data_set.pcd_data[0].is_rotate = true;

				image_depth_data_set.pcd_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.pcd_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.pcd_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.pcd_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.pcd_data[0].min_distance = disp_color_map_distance_.min_value;
				image_depth_data_set.pcd_data[0].max_distance = disp_color_map_distance_.max_value;

				image_depth_data_set.pcd_data[0].image = image_data_color->buffer;
				image_depth_data_set.pcd_data[0].depth = depth_dpresult->buffer;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kDplDepthDepth:
		{
			DepthData* depth_dpresult = &image_data_list_0->depth_dpl;
			DepthData* depth_data = &image_data_list_1->depth;
			if (depth_dpresult->width == 0 || depth_dpresult->height == 0) {
				return;
			}
			if (depth_data->width == 0 || depth_data->height == 0) {
				return;
			}

			// 高さを合わせます
			double right_ratio = 1.0;
			double depth_ratio = 1.0;
			if (depth_data->height != depth_dpresult->height) {
				if (depth_data->height > depth_dpresult->height) {
					right_ratio = (double)depth_dpresult->height / (double)depth_data->height;
					right_ratio = std::round(right_ratio * 10) / 10;
				}
				else {
					depth_ratio = (double)depth_data->height / (double)depth_dpresult->height;
					depth_ratio = std::round(depth_ratio * 10) / 10;
				}
			}

			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_dpresult->width, depth_dpresult->height, depth_dpresult->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image0(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_color_image0(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[3]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image0, mat_color_image0, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				mat_color_image0= mat_depth_image0.clone();
			}

			ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_data->width, depth_data->height, depth_data->buffer, temp_buffer_[4]);

			cv::Mat mat_depth_image1(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[4]);

			cv::Mat mat_color_image1(cv::Size(depth_data->width, depth_data->height), CV_8UC4, temp_buffer_[5]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image1, mat_color_image1, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				mat_color_image1 = mat_depth_image1.clone();
			}

			// 表示用の合わせた画像を作成
			int combine_height = mat_color_image0.rows;
			int combine_width = mat_color_image0.cols + mat_color_image1.cols;

			cv::Mat combined_img(combine_height, combine_width, CV_8UC4, color_[index]);

			cv::Mat roi1(combined_img, cv::Rect(0, 0, mat_color_image0.cols, mat_color_image0.rows));
			mat_color_image0.copyTo(roi1);

			cv::Mat roi2(combined_img, cv::Rect(mat_color_image0.cols, 0, mat_color_image1.cols, mat_color_image1.rows));
			mat_color_image1.copyTo(roi2);

			width_color_[index] = combined_img.cols;
			height_color_[index] = combined_img.rows;

			display_information_.original_image_size[0].cx = depth_dpresult->width;
			display_information_.original_image_size[0].cy = depth_dpresult->height;
			display_information_.original_image_size[1].cx = depth_data->width;
			display_information_.original_image_size[1].cy = depth_data->height;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_dpresult->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_dpresult->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = depth_data->width;
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = depth_data->height;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("DEPTH_IMAGE"));
				image_depth_data_set.image_data[0].width = depth_data->width;
				image_depth_data_set.image_data[0].height = depth_data->height;
				image_depth_data_set.image_data[0].channel_count = 4;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = temp_buffer_[2];

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DPL_DEPTH_IMAGE"));
				image_depth_data_set.image_data[1].width = depth_dpresult->width;
				image_depth_data_set.image_data[1].height = depth_dpresult->height;
				image_depth_data_set.image_data[1].channel_count = 4;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = temp_buffer_[4];

				image_depth_data_set.depth_data_count = 2;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_data->width;
				image_depth_data_set.depth_data[0].height = depth_data->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_data->buffer;

				_stprintf_s(image_depth_data_set.depth_data[1].id_string, _T("DPL_DEPTH_DATA"));
				image_depth_data_set.depth_data[1].width = depth_dpresult->width;
				image_depth_data_set.depth_data[1].height = depth_dpresult->height;
				image_depth_data_set.depth_data[1].is_rotate = true;

				image_depth_data_set.depth_data[1].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[1].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[1].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[1].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[1].buffer = depth_dpresult->buffer;

				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;

		case ImageDrawMode::kOverlapedDplDepthBase:
		{
			DepthData* depth_dpresult = &image_data_list_0->depth_dpl;
			ImageData* image_data_right = &image_data_list_1->image_base;
			if (depth_dpresult->width == 0 || depth_dpresult->height == 0) {
				return;
			}
			if (image_data_right->width == 0 || image_data_right->height == 0) {
				return;
			}

			// 高さを合わせます
			double right_ratio = 1.0;
			double depth_ratio = 1.0;
			if (image_data_right->height != depth_dpresult->height) {
				if (image_data_right->height > depth_dpresult->height) {
					right_ratio = (double)depth_dpresult->height / (double)image_data_right->height;
					right_ratio = std::round(right_ratio * 10) / 10;
				}
				else {
					depth_ratio = (double)image_data_right->height / (double)depth_dpresult->height;
					depth_ratio = std::round(depth_ratio * 10) / 10;
				}
			}

			const double max_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.max_value : disp_color_map_disparity_.max_value;
			const double min_length = draw_parameter->depth_draw_distance ? disp_color_map_distance_.min_value : disp_color_map_disparity_.min_value;
			DispColorMap* disp_color_map = draw_parameter->depth_draw_distance ? &disp_color_map_distance_ : &disp_color_map_disparity_;

			bool ret = MakeDepthColorImage(draw_parameter->depth_draw_distance, draw_parameter->draw_outside_bounds, min_length, max_length,
				disp_color_map, draw_parameter->camera_b, draw_parameter->camera_set_angle, draw_parameter->camera_bf, draw_parameter->camera_dinf,
				depth_dpresult->width, depth_dpresult->height, depth_dpresult->buffer, temp_buffer_[2]);

			cv::Mat mat_depth_image(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[2]);

			cv::Mat mat_color_image0(cv::Size(depth_dpresult->width, depth_dpresult->height), CV_8UC4, temp_buffer_[3]);
			if (depth_ratio != 1.0) {
				cv::resize(mat_depth_image, mat_color_image0, cv::Size(), depth_ratio, depth_ratio, cv::INTER_NEAREST);
			}
			else {
				mat_color_image0 = mat_depth_image.clone();
			}

			cv::Size image_size(image_data_right->width, image_data_right->height);
			cv::Mat mat_color_image1(image_size, CV_8UC4, temp_buffer_[0]);

			if (image_data_right->channel_count == 1) {
				cv::Mat mat_src_image0(image_size, CV_8UC1, image_data_right->buffer);

				if (right_ratio != 1.0) {
					cv::Mat mat_scale_image0(image_size, CV_8UC1, temp_buffer_[1]);
					cv::resize(mat_src_image0, mat_scale_image0, cv::Size(), right_ratio, right_ratio, cv::INTER_NEAREST);

					cv::cvtColor(mat_scale_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
				else {
					cv::cvtColor(mat_src_image0, mat_color_image1, cv::COLOR_GRAY2BGRA);
				}
			}
			else {
				// error
			}

			//ブレンドを行う
			double alpha = 0.7;
			double beta = (1.0 - alpha);

			int new_height = mat_color_image1.rows;
			int new_width = mat_color_image1.cols;
			cv::Mat matBlendImageColor(new_height, new_width, CV_8UC4, color_[index]);

			cv::addWeighted(mat_color_image1, alpha, mat_color_image0, beta, 0.0, matBlendImageColor);

			width_color_[index] = matBlendImageColor.cols;
			height_color_[index] = matBlendImageColor.rows;

			display_information_.original_image_size[0].cx = depth_dpresult->width;
			display_information_.original_image_size[0].cy = depth_dpresult->height;
			display_information_.original_image_size[1].cx = image_data_right->width;
			display_information_.original_image_size[1].cy = image_data_right->height;
			display_information_.image_size.cx = width_[0];
			display_information_.image_size.cy = height_[0];
			display_information_.magnification = magnification;
			display_information_.magnification_center = magnification_center;
			display_information_.rectangle_to_display[0].top = 0;
			display_information_.rectangle_to_display[0].bottom = depth_dpresult->height;
			display_information_.rectangle_to_display[0].left = 0;
			display_information_.rectangle_to_display[0].right = depth_dpresult->width;
			display_information_.rectangle_to_display[1].top = 0;
			display_information_.rectangle_to_display[1].bottom = image_data_right->height;
			display_information_.rectangle_to_display[1].left = 0;
			display_information_.rectangle_to_display[1].right = image_data_right->width;

			if (draw_parameter->save_image_request) {
				image_depth_data_set.image_datacount = 2;

				_stprintf_s(image_depth_data_set.image_data[0].id_string, _T("BASE_IMAGE"));
				image_depth_data_set.image_data[0].width = image_data_right->width;
				image_depth_data_set.image_data[0].height = image_data_right->height;
				image_depth_data_set.image_data[0].channel_count = image_data_right->channel_count;
				image_depth_data_set.image_data[0].is_rotate = true;
				image_depth_data_set.image_data[0].buffer = image_data_right->buffer;

				_stprintf_s(image_depth_data_set.image_data[1].id_string, _T("DPL_DEPTH_IMAGE"));
				image_depth_data_set.image_data[1].width = depth_dpresult->width;
				image_depth_data_set.image_data[1].height = depth_dpresult->height;
				image_depth_data_set.image_data[1].channel_count = 4;
				image_depth_data_set.image_data[1].is_rotate = true;
				image_depth_data_set.image_data[1].buffer = temp_buffer_[2];

				image_depth_data_set.depth_data_count = 1;

				_stprintf_s(image_depth_data_set.depth_data[0].id_string, _T("DPL_DEPTH_DATA"));
				image_depth_data_set.depth_data[0].width = depth_dpresult->width;
				image_depth_data_set.depth_data[0].height = depth_dpresult->height;
				image_depth_data_set.depth_data[0].is_rotate = true;

				image_depth_data_set.depth_data[0].camera_b = draw_parameter->camera_b;
				image_depth_data_set.depth_data[0].camera_dinf = draw_parameter->camera_dinf;
				image_depth_data_set.depth_data[0].camera_bf = draw_parameter->camera_bf;
				image_depth_data_set.depth_data[0].camera_set_angle = draw_parameter->camera_set_angle;

				image_depth_data_set.depth_data[0].buffer = depth_dpresult->buffer;

				image_depth_data_set.pcd_data_count = 0;

				dpc_image_write_->PushImageDepthData(&image_depth_data_set);
			}
		}
		break;
	}

	return;
}

RECT FitBoxToRect(const RECT& rect_src, const RECT& rect_dst)
{
	int src_width = rect_src.right - rect_src.left;
	int src_height = rect_src.bottom - rect_src.top;

	int dst_width = rect_dst.right - rect_dst.left;
	int dst_height = rect_dst.bottom - rect_dst.top;

	int dst_lb_width = 0;
	int dst_lb_height = 0;

	if (MulDiv(src_width, dst_height, src_height) <= dst_width)
	{
		// Column letterboxing ("pillar box")
		dst_lb_width = MulDiv(dst_height, src_width, src_height);
		dst_lb_height = dst_height;
	}
	else
	{
		// Row letterboxing.
		dst_lb_width = dst_width;
		dst_lb_height = MulDiv(dst_width, src_height, src_width);
	}

	LONG left = rect_dst.left + ((dst_width - dst_lb_width) / 2);
	LONG top = rect_dst.top + ((dst_height - dst_lb_height) / 2);

	RECT rc = {};
	SetRect(&rc, left, top, left + dst_lb_width, top + dst_lb_height);

	return rc;
}

bool DpcDrawLib::Render(HDC hdc1, LPRECT rect1, HDC hdc2, LPRECT rect2,
						ImageDataSet* image_data_set_0, ImageDataSet* image_data_set_1,
						TextDataSet* text_data_set, RectDataSet* rect_data_set, DrawParameter* draw_parameter)
{

	LARGE_INTEGER start_time = {}, end_time = {};

	// create resource
	if (show_elapsed_time_) {
		QueryPerformanceCounter(&start_time);
	}

	HRESULT hr;
	hr = CreateDeviceResourcesAlt();
	if (!SUCCEEDED(hr)) {
		return false;
	}
	if (show_elapsed_time_) {
		QueryPerformanceCounter(&end_time);
		double elapsed_time = (double)((double)((end_time.QuadPart - start_time.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

		TCHAR msg[128] = {};
		_stprintf_s(msg, _T("[INFO]DpcDrawLib::CreateDeviceResourcesAlt time=%.04f\n"), elapsed_time);
		OutputDebugString(msg);
	}

	// Make data
	if (show_elapsed_time_) {
		QueryPerformanceCounter(&start_time);
	}
	
	if (image_data_set_0->valid) {
		SIZE target_size = {};
		target_size.cx = rect1->right - rect1->left + 1;
		target_size.cy = rect1->bottom - rect1->top + 1;

		BuildBitmap(0, image_data_set_0->mode, text_data_set, draw_parameter, &target_size,
					&display_information_, &image_data_set_0->image_data_list[0], &image_data_set_0->image_data_list[1]);
	}

	if (image_data_set_1->valid) {
		SIZE target_size = {};
		target_size.cx = rect2->right - rect2->left + 1;
		target_size.cy = rect2->bottom - rect2->top + 1;

		BuildBitmap(1, image_data_set_1->mode, text_data_set, draw_parameter, &target_size,
					&display_information_, &image_data_set_1->image_data_list[0], &image_data_set_1->image_data_list[1]);
	}

	if (show_elapsed_time_) {
		QueryPerformanceCounter(&end_time);
		double elapsed_time = (double)((double)((end_time.QuadPart - start_time.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

		TCHAR msg[128] = {};
		_stprintf_s(msg, _T("[INFO]DpcDrawLib::buildBitmap time=%.04f\n"), elapsed_time);
		OutputDebugString(msg);
	}

	// start draw 1
	if (image_data_set_0->valid) {
		draw_parameter_.depth_draw_distance = draw_parameter->depth_draw_distance;
		draw_parameter_.draw_outside_bounds = draw_parameter->draw_outside_bounds;
		draw_parameter_.camera_b = draw_parameter->camera_b;
		draw_parameter_.camera_dinf = draw_parameter->camera_dinf;
		draw_parameter_.camera_bf = draw_parameter->camera_bf;
		draw_parameter_.camera_set_angle = draw_parameter->camera_set_angle;
		draw_parameter_.magnification = draw_parameter->magnification;
		draw_parameter_.magnification_center = draw_parameter->magnification_center;

		if (show_elapsed_time_) {
			QueryPerformanceCounter(&start_time);
		}

		if (dc_render_target_)
		{
			hr = dc_render_target_->BindDC(hdc1, rect1);

			// draw
			dc_render_target_->BeginDraw();
			dc_render_target_->SetTransform(D2D1::Matrix3x2F::Identity());
			dc_render_target_->Clear(D2D1::ColorF(D2D1::ColorF::Gray));
			//dc_render_target_->Clear(D2D1::ColorF(1.0F, 1.0F, 1.0F));

			if ((width_[0] != width_color_[0]) || (height_[0] != height_color_[0]))
			{
				width_[0] = width_color_[0];
				height_[0] = height_color_[0];
				if (bitmap_[0])
				{
					bitmap_[0]->Release();
					bitmap_[0] = NULL;
				}
			}

			if (bitmap_[0] == NULL)
			{
				D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
				dc_render_target_->CreateBitmap
				(
					D2D1::SizeU(width_[0], height_[0]),
					NULL,
					width_[0] * sizeof(UINT32),
					D2D1::BitmapProperties(pixelFormat),
					&bitmap_[0]
				);
			}

			try
			{
				D2D1_RECT_U rect;
				rect.left = 0;
				rect.top = 0;
				rect.right = width_[0];
				rect.bottom = height_[0];
				bitmap_[0]->CopyFromMemory(&rect, color_[0], width_[0] * sizeof(UINT32));
			}
			catch (...)
			{
				bitmap_[0]->Release();
				bitmap_[0] = NULL;

				return false;
			}

			if (bitmap_[0] != NULL) {
				// Draw

				D2D1_SIZE_F target_size = dc_render_target_->GetSize();
				D2D1_POINT_2F rotate_center = D2D1::Point2F(
					target_size.width / 2,
					target_size.height / 2
				);


#if 0
				// Rotate the next bitmap by 180 degrees.
				dc_render_target_->SetTransform(
					D2D1::Matrix3x2F::Rotation(180.0F, rotate_center)
				);
#elif 1

				// Rotate the next bitmap by 180 degrees.
				D2D1::Matrix3x2F matrix_rotate = D2D1::Matrix3x2F::Rotation(180.0F, rotate_center);

				// Apply the scale transform to the render target.
				double magnification = draw_parameter->magnification;
				D2D1_POINT_2F point_magnification_cenater = D2D1::Point2F(
					0, //draw_parameter->magnification_center.x,
					0. //draw_parameter->magnification_center.y
				);
				D2D1::Matrix3x2F matrix_scale = D2D1::Matrix3x2F::Scale(D2D1::Size(magnification, magnification), point_magnification_cenater);

				// Transrate
				D2D1_POINT_2F point_translation = D2D1::Point2F(
					draw_parameter->magnification_center.x - (draw_parameter->magnification_center.x * (FLOAT)magnification),
					draw_parameter->magnification_center.y - (draw_parameter->magnification_center.y * (FLOAT)magnification)
				);
				D2D1::Matrix3x2F matrix_translation = D2D1::Matrix3x2F::Translation(
					point_translation.x,
					point_translation.y
				);

				D2D1::Matrix3x2F matrix_3x2f = matrix_rotate * matrix_scale * matrix_translation;

				dc_render_target_->SetTransform(matrix_3x2f);

#else
				double magnification = draw_parameter->magnification;
				D2D1_POINT_2F magnification_cenater = D2D1::Point2F(
					target_size.width - draw_parameter->magnification_center.x,
					target_size.height - draw_parameter->magnification_center.y
				);
				
				// Apply the scale transform to the render target.
				D2D1::Matrix3x2F matrix_scale = D2D1::Matrix3x2F::Scale(D2D1::Size(magnification, magnification), magnification_cenater);

				// Rotate the next bitmap by 180 degrees.
				D2D1::Matrix3x2F matrix_rotate = D2D1::Matrix3x2F::Rotation(180.0F, rotate_center);

				D2D1::Matrix3x2F matrix_3x2f = matrix_scale * matrix_rotate;

				dc_render_target_->SetTransform(matrix_3x2f);
#endif

				D2D1_RECT_F d2d1_rect_dst = {};
				d2d1_rect_dst.top = (float)(rect1->top);
				d2d1_rect_dst.bottom = (float)(rect1->bottom);
				d2d1_rect_dst.left = (float)(rect1->left);
				d2d1_rect_dst.right = (float)(rect1->right);

				// LB Box
				D2D1_RECT_F lb_box_rect = {};
				{
					D2D1_SIZE_F src_size = bitmap_[0]->GetSize();

					RECT src_rect = {};
					src_rect.top = 0;
					src_rect.bottom = (int)(src_size.height - 1);
					src_rect.left = 0;
					src_rect.right = (int)(src_size.width - 1);

					RECT dst_rect = {};
					dst_rect.top = (int)d2d1_rect_dst.top;
					dst_rect.bottom = (int)d2d1_rect_dst.bottom;
					dst_rect.left = (int)d2d1_rect_dst.left;
					dst_rect.right = (int)d2d1_rect_dst.right;

					RECT lb_rect = {};
					lb_rect = FitBoxToRect(src_rect, dst_rect);

					lb_box_rect.top = (float)(lb_rect.top);
					lb_box_rect.bottom = (float)(lb_rect.bottom);
					lb_box_rect.left = (float)(lb_rect.left);
					lb_box_rect.right = (float)(lb_rect.right);
				}

				dc_render_target_->DrawBitmap(bitmap_[0], &lb_box_rect);

				dc_render_target_->SetTransform(
					D2D1::Matrix3x2F::Identity()
				);

				// draw text
				if (text_data_set->count > 0) {
					for (int i = 0; i < text_data_set->count; i++) {
						size_t len = _tcsclen(text_data_set->text_data[i].string);
						if (len == 0) {
						}
						else {
							D2D1_RECT_F rect =
								D2D1::RectF(
									(FLOAT)text_data_set->text_data[i].x,
									(FLOAT)text_data_set->text_data[i].y,
									(FLOAT)text_data_set->text_data[i].x + draw_text_font_setting_.FontSize * len,
									(FLOAT)text_data_set->text_data[i].y + draw_text_font_setting_.y1 * 1);

							rect.top = (rect.top * (FLOAT)magnification) + matrix_translation.dy;
							rect.left = (rect.left * (FLOAT)magnification) + matrix_translation.dx;
							rect.bottom = (rect.bottom * (FLOAT)magnification) + matrix_translation.dy;
							rect.right = (rect.right * (FLOAT)magnification) + matrix_translation.dx;

							dc_render_target_->FillRectangle(&rect, brush_back_);

							D2D1_RECT_F rectText = rect;
							rectText.bottom = rectText.top + draw_text_font_setting_.y1;
							CString infoStr(text_data_set->text_data[i].string);
							{
								dc_render_target_->DrawText(infoStr,
									(UINT32)_tcsclen(infoStr),
									text_format_,
									rectText,
									brush_text_);
							}
						}
					}
				}

				// draw rectangle
				if (rect_data_set->count != 0) {
					for (int i = 0; i < rect_data_set->count; i++) {
						D2D1_RECT_F temp_rect = {};
						temp_rect.top = ((FLOAT)rect_data_set->rect_data[i].top * (FLOAT)magnification) + matrix_translation.dy;
						temp_rect.left = ((FLOAT)rect_data_set->rect_data[i].left * (FLOAT)magnification) + matrix_translation.dx;
						temp_rect.bottom = ((FLOAT)rect_data_set->rect_data[i].bottom * (FLOAT)magnification) + matrix_translation.dy;
						temp_rect.right = ((FLOAT)rect_data_set->rect_data[i].right * (FLOAT)magnification) + matrix_translation.dx;

						D2D1_RECT_F rectF =
							D2D1::RectF(
								(FLOAT)temp_rect.left,
								(FLOAT)temp_rect.top,
								(FLOAT)temp_rect.right,
								(FLOAT)temp_rect.bottom);

						dc_render_target_->DrawRectangle(&rectF, brush_, 1.0F);
					}
				}

				// set information
				display_information_.valid = true;
				display_information_.draw_terget_size = target_size;
				display_information_.draw_rotate_center = rotate_center;
				display_information_.draw_magnification_cenater = point_magnification_cenater;
				display_information_.draw_translation = point_translation;
				display_information_.draw_lb_box = lb_box_rect;
			}

			if (dc_render_target_->EndDraw() == D2DERR_RECREATE_TARGET) {
				ReleaseResource();

				{
					OutputDebugString(_T("[ERROR]EndDraw() D2DERR_RECREATE_TARGET\n"));
				}
			}
		}

		if (show_elapsed_time_) {
			QueryPerformanceCounter(&end_time);
			double elapsed_time = (double)((double)((end_time.QuadPart - start_time.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

			TCHAR msg[128] = {};
			_stprintf_s(msg, _T("[INFO]DpcDrawLib::Draw1 time=%.04f\n"), elapsed_time);
			OutputDebugString(msg);
		}

	}

	// start draw 2
	if (image_data_set_1->valid) {
		if (show_elapsed_time_) {
			QueryPerformanceCounter(&start_time);
		}

		if (dc_render_target_)
		{
			hr = dc_render_target_->BindDC(hdc2, rect2);

			// draw
			dc_render_target_->BeginDraw();

			//背景クリア       
			dc_render_target_->Clear(D2D1::ColorF(1.0F, 1.0F, 1.0F));

			if ((width_[1] != width_color_[1]) || (height_[1] != height_color_[1]))
			{
				width_[1] = width_color_[1];
				height_[1] = height_color_[1];
				if (bitmap_[1])
				{
					bitmap_[1]->Release();
					bitmap_[1] = NULL;
				}
			}

			if (bitmap_[1] == NULL)
			{
				D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
				dc_render_target_->CreateBitmap
				(
					D2D1::SizeU(width_[1], height_[1]),
					NULL,
					width_[1] * sizeof(UINT32),
					D2D1::BitmapProperties(pixelFormat),
					&bitmap_[1]
				);
			}

			try
			{
				D2D1_RECT_U rect;
				rect.left = 0;
				rect.top = 0;
				rect.right = width_[1];
				rect.bottom = height_[1];
				bitmap_[1]->CopyFromMemory(&rect, color_[1], width_[1] * sizeof(UINT32));
			}
			catch (...)
			{
				bitmap_[1]->Release();
				bitmap_[1] = NULL;
			}

			if (bitmap_[1] != NULL) {
				// Draw
				D2D1_RECT_F RectF1 = {};
				RectF1.top = (float)(rect2->top);
				RectF1.bottom = (float)(rect2->bottom);
				RectF1.left = (float)(rect2->left);
				RectF1.right = (float)(rect2->right);

				// LB Box
				{
					D2D1_SIZE_F src_size = bitmap_[1]->GetSize();

					RECT src_rect = {};
					src_rect.top = 0;
					src_rect.bottom = (int)(src_size.height - 1);
					src_rect.left = 0;
					src_rect.right = (int)(src_size.width - 1);

					RECT dst_rect = {};
					dst_rect.top = (int)RectF1.top;
					dst_rect.bottom = (int)RectF1.bottom;
					dst_rect.left = (int)RectF1.left;
					dst_rect.right = (int)RectF1.right;

					RECT lb_rect = {};
					lb_rect = FitBoxToRect(src_rect, dst_rect);

					RectF1.top = (float)(lb_rect.top);
					RectF1.bottom = (float)(lb_rect.bottom);
					RectF1.left = (float)(lb_rect.left);
					RectF1.right = (float)(lb_rect.right);
				}

				dc_render_target_->DrawBitmap(bitmap_[1], &RectF1, 1.0, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
			}

			if (dc_render_target_->EndDraw() == D2DERR_RECREATE_TARGET) {
				ReleaseResource();

				{
					TCHAR str[1024];
					_stprintf_s(str, _T("[ERROR]EndDraw() D2DERR_RECREATE_TARGET\n"));
					OutputDebugString(str);
				}
			}
		}

		if (show_elapsed_time_) {
			QueryPerformanceCounter(&end_time);
			double elapsed_time = (double)((double)((end_time.QuadPart - start_time.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

			TCHAR msg[128] = {};
			_stprintf_s(msg, _T("[INFO]DpcDrawLib::Draw2 time=%.04f\n"), elapsed_time);
			OutputDebugString(msg);
		}
	}

	return true;
}

int DpcDrawLib::ColorScaleBCGYR(const double min_value, const double max_value, const double in_value, int* bo, int* go, int* ro)
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

int DpcDrawLib::BuildColorHeatMap(DispColorMap* disp_color_map)
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

int DpcDrawLib::BuildColorHeatMapForDisparity(DispColorMap* disp_color_map)
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

bool DpcDrawLib::MakeDepthColorImage(	const bool is_color_by_distance, const bool is_draw_outside_bounds, const double min_length_i, const double max_length_i,
										DispColorMap* disp_color_map, double b_i, const double angle_i, const double bf_i, const double dinf_i,
										const int width, const int height, float* depth, unsigned char* bgra_image)
{
	if (disp_color_map == nullptr) {
		return false;
	}

	if (depth == nullptr) {
		return false;
	}

	if (bgra_image == nullptr) {
		return false;
	}

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
			unsigned char* dst = bgra_image + (i * width * 4);

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
				*dst++ = 255;

				src++;
			}
		}
	}
	else {
		// 視差
		const double max_value = max_disparity_;
		const double dinf = dinf_i;

		for (int i = 0; i < height; i++) {
			float* src = depth + (i * width);
			unsigned char* dst = bgra_image + (i * width * 4);

			for (int j = 0; j < width; j++) {
				int r = 0, g = 0, b = 0;
				if (*src <= dinf) {
					r = 0;
					g = 0;
					b = 0;
				}
				else {
					double d = MAX(0, (max_value - *src - dinf));

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
				*dst++ = 255;

				src++;
			}
		}
	}

	return true;
}

bool DpcDrawLib::ScreenPostionToDrawImagePosition(const POINT& screen_position, POINT* image_position)
{
	/*
	  この関数は、（回転した）表示画像上での位置を戻します
	  2つの画像を並べた場合は、それぞれの画像での位置となります
	*/

	image_position->x = -1;
	image_position->y = -1;

	if (!display_information_.valid) {
		return true;
	}

	// LB変換を戻す
	FLOAT lb_x_offset = display_information_.draw_lb_box.left;
	FLOAT lb_y_offset = display_information_.draw_lb_box.top;

	POINT position_1 = {};
	position_1.x = screen_position.x - (LONG)lb_x_offset;
	position_1.y = screen_position.y - (LONG)lb_y_offset;

	double lb_x_magnification = (double)display_information_.image_size.cx / (double)(display_information_.draw_lb_box.right - display_information_.draw_lb_box.left);

	position_1.x = (int)((double)position_1.x * lb_x_magnification);
	position_1.y = (int)((double)position_1.y * lb_x_magnification);

	POINT position_on_image = {};

	// The following uses fall through
	switch (display_information_.mode) {
	// single case
	case ImageDrawMode::kBase:
	case ImageDrawMode::kCompare:
	case ImageDrawMode::kDepth:
	case ImageDrawMode::kColor:
	case ImageDrawMode::kOverlapedDepthBase:
	case ImageDrawMode::kDplImage:
	case ImageDrawMode::kDplDepth:
	{
		position_on_image.x = position_1.x;
		position_on_image.y = position_1.y;
	}
	break;

	// dual case
	case ImageDrawMode::kBaseCompare:
	case ImageDrawMode::kDepthBase:
	case ImageDrawMode::kDepthColor:
	case ImageDrawMode::kDplImageBase:
	case ImageDrawMode::kDplImageColor:
	case ImageDrawMode::kDplDepthBase:
	case ImageDrawMode::kDplDepthColor:
	case ImageDrawMode::kDplDepthDepth:
	{
		// 左右の区別
		if (position_1.x <= display_information_.original_image_size[0].cx) {
			// 左
			position_on_image.x = position_1.x;
			position_on_image.y = position_1.y;
		}
		else {
			// 右
			position_on_image.x = position_1.x - display_information_.original_image_size[0].cx;
			position_on_image.y = position_1.y;
		}
	}
	break;

	}

	image_position->x = position_on_image.x;
	image_position->y = position_on_image.y;


	return true;
}

bool DpcDrawLib::ScreenPostionToImagePosition(const POINT& screen_position, POINT* image_position, int* selected_inex)
{
	/*
	  この関数は、元の画像上の座標に変換し、座標上のデータを取得を可能とします
	  ScreenPostionToDrawImagePosition　と違い、回転も戻します
	  2つの画像を並べた場合は、それぞれの画像での位置となります

	*/

	image_position->x = -1;
	image_position->y = -1;
	*selected_inex = -1;

	if (!display_information_.valid) {
		return true;
	}

	// LB変換を戻す
	FLOAT lb_x_offset = display_information_.draw_lb_box.left;
	FLOAT lb_y_offset = display_information_.draw_lb_box.top;

	POINT position_1 = {};
	position_1.x = screen_position.x - (LONG)lb_x_offset;
	position_1.y = screen_position.y - (LONG)lb_y_offset;

	double lb_x_magnification = (double)display_information_.image_size.cx / (double)(display_information_.draw_lb_box.right - display_information_.draw_lb_box.left);

	position_1.x = (int)((double)position_1.x * lb_x_magnification);
	position_1.y = (int)((double)position_1.y * lb_x_magnification);

	POINT position_on_image = {};
	int currently_selected_index = -1;

	// The following uses fall through
	switch (display_information_.mode) {
	// single
	case ImageDrawMode::kBase:
	case ImageDrawMode::kCompare:
	case ImageDrawMode::kDepth:
	case ImageDrawMode::kColor:
	case ImageDrawMode::kOverlapedDepthBase:
	case ImageDrawMode::kDplImage:
	case ImageDrawMode::kDplDepth:
	{
		position_on_image.x = display_information_.original_image_size[0].cx - position_1.x;
		position_on_image.y = display_information_.original_image_size[0].cy - position_1.y;
		currently_selected_index = 0;
	}
	break;

	// dual
	case ImageDrawMode::kBaseCompare:
	case ImageDrawMode::kDepthBase:
	case ImageDrawMode::kDepthColor:
	case ImageDrawMode::kDplImageBase:
	case ImageDrawMode::kDplImageColor:
	case ImageDrawMode::kDplDepthBase:
	case ImageDrawMode::kDplDepthColor:
	case ImageDrawMode::kDplDepthDepth:
	{
		// 左右の区別
		if (position_1.x <= display_information_.original_image_size[0].cx) {
			// 左
			position_on_image.x = display_information_.original_image_size[0].cx - position_1.x;
			position_on_image.y = display_information_.original_image_size[0].cy - position_1.y;
			currently_selected_index = 1;
		}
		else {
			// 右
			position_on_image.x = display_information_.original_image_size[1].cx - (position_1.x - display_information_.original_image_size[0].cx);
			position_on_image.y = display_information_.original_image_size[0].cy - position_1.y;
			currently_selected_index = 0;
		}
	}
	break;
	}

	image_position->x = position_on_image.x;
	image_position->y = position_on_image.y;
	*selected_inex = currently_selected_index;

	return true;
}

bool DpcDrawLib::Image3DPositionToScreenPostion(const float x, const float y, const float z, float* xr, float* yr, float* zr)
{
	/*
	  この関数は、元の画像上での３D座標を、表示画像上での位置へに変換します
	  回転補正を行いますが、3DでのX,Yは中心からの基準なので、符号反転を行います
	  Zは、回転で変わらないので、そのままです

	*/

	if (!display_information_.valid) {
		return true;
	}

	*xr = -1 * x;
	*yr = -1 * y;
	*zr = z;

	return true;
}

bool DpcDrawLib::GetCurrentDrawParameter(DrawParameter* draw_parameter)
{
	draw_parameter->depth_draw_distance = draw_parameter_.depth_draw_distance;
	draw_parameter->draw_outside_bounds = draw_parameter_.draw_outside_bounds;
	draw_parameter->camera_b = draw_parameter_.camera_b;
	draw_parameter->camera_dinf = draw_parameter_.camera_dinf;
	draw_parameter->camera_bf = draw_parameter_.camera_bf;
	draw_parameter->camera_set_angle = draw_parameter_.camera_set_angle;
	draw_parameter->magnification = draw_parameter_.magnification;
	draw_parameter->magnification_center = draw_parameter_.magnification_center;

	return true;
}

bool DpcDrawLib::GetOriginalMagnificationPosition(const POINT& screen_position, POINT* original_screen_position)
{

	original_screen_position->x = screen_position.x;
	original_screen_position->y = screen_position.y;

	if (!display_information_.valid) {
		return true;
	}

	original_screen_position->x = (LONG)(((FLOAT)screen_position.x - display_information_.draw_translation.x) / display_information_.magnification);
	original_screen_position->y = (LONG)(((FLOAT)screen_position.y - display_information_.draw_translation.y) / display_information_.magnification);

	return true;
}