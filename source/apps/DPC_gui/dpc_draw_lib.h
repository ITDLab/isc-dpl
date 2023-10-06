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

#pragma once

class DpcDrawLib
{
public:
	DpcDrawLib();
	~DpcDrawLib();

	bool Open(const int max_width, const int max_height, const double min_distance, const double max_distance, const double max_disparity, const TCHAR* save_image_path);

	bool Close();

	void ReleaseResource();

	void GetMinMaxDistance(double* min_distance, double* max_distance) const;

	void RebuildDrawColorMap(const double min_distance, const double max_distance);

	void GetDisplayMag(double* mag) const;

	enum class ImageDrawMode {
								/**< image_data_list	[0]					[1]				*/
		kBase,					/**< 0:					image_base							*/
		kCompare,				/**< 1:					image_compare						*/
		kDepth,					/**< 2:					depth								*/
		kColor,					/**< 3:					image_color							*/
		kBaseCompare,			/**< 4:					image_base,			image_compare	*/
		kDepthBase,				/**< 5:					depth_data,			image_base		*/
		kDepthColor,			/**< 6:					depth_data,			image_color		*/
		kOverlapedDepthBase,	/**< 7:					depth_data,			image_base		*/

		kDplImage,				/**< 8:					image_dpl							*/
		kDplImageBase,			/**< 9:					image_dpl,			image_base		*/
		kDplImageColor,			/**< 10:				image_dpl,			image_color		*/
		kDplDepth,				/**< 11:				depth_dpl							*/
		kDplDepthBase,			/**< 12:				depth_dpl,			image_base		*/
		kDplDepthColor,			/**< 13:				depth_dpl,			image_color		*/
		kDplDepthDepth,			/**< 14:				depth_dpl,			depth			*/
		kOverlapedDplDepthBase,	/**< 15:				depth_dpl,			image_base		*/
		kUnknown=99				/**< 99:				(error case)						*/
	};

	struct ImageData {
		int width;
		int height;
		int channel_count;

		unsigned char* buffer;

		ImageData()
		{
			width = height = 0;
			channel_count = 1;
			buffer = nullptr;
		}

	};

	struct DepthData {
		int width;
		int height;

		float* buffer;

		DepthData()
		{
			width = height = 0;
			buffer = nullptr;
		}
	};

	struct ImageDataList {
		ImageData	image_base;		/**< image on the base camera */
		ImageData	image_compare;	/**< image on the comapre camera */
		DepthData	depth;			/**< disparity data */
		ImageData	image_color;	/**< color image */
		ImageData	image_dpl;		/**< image of data process result */
		DepthData	depth_dpl;		/**< depth data of data process result */
	};

	struct ImageDataSet {
		bool valid;							/**< true: valid data */
		ImageDrawMode mode;					/**< mode for display */

		ImageDataList image_data_list[2];	/**< image and data */
	};

	bool InitializeImageDataSet(ImageDataSet* image_data_set);
	bool ReleaseImageDataSet(ImageDataSet* image_data_set);

	struct TextData {
		int x, y;           /**< 表示位置 */
		TCHAR string[64];   /**< 表示文字 */
	};

	struct TextDataSet {
		int count;				/**< 表示個数 */
		TextData text_data[4];	/**< 文字情報 */
	};

	struct RectDataSet {
		int count;				/**< 表示個数 */
		RECT rect_data[4];		/**< 矩形情報 */
	};

	struct DrawParameter{
		bool depth_draw_distance;	/**< 視差の表示を距離で行う */
		bool draw_outside_bounds;	/**< 範囲外を描画する */
		double camera_b;			/**< カメラパラメータ */
		double camera_dinf;			/**< カメラパラメータ */
		double camera_bf;			/**< カメラパラメータ */
		double camera_set_angle;	/**< カメラパラメータ */

		double magnification;		/**< 拡大率 > 0 */
		POINT magnification_center; /**< 拡大中心座標 */

		bool save_image_request;	/**< ワンタイム　画像保存要求 */
	};

	bool Render(HDC hdc1, LPRECT rect1, HDC hdc2, LPRECT rect2,	
				ImageDataSet* image_data_set_0, ImageDataSet* image_data_set_1, 
				TextDataSet* text_data_set, RectDataSet* rect_data_set,DrawParameter* draw_parameter);

	bool ScreenPostionToDrawImagePosition(const POINT& screen_position, POINT* image_position);

	bool ScreenPostionToImagePosition(const POINT& screen_position, POINT* image_position, int* selected_inex);
	
	bool ScreenPostionToDepthImagePosition(const POINT& screen_position, POINT* image_position);
		
	bool Image3DPositionToScreenPostion(const float x, const float y, const float z, float* xr, float* yr, float* zr);

	bool GetCurrentDrawParameter(DrawParameter* draw_parameter);

	bool GetOriginalMagnificationPosition(const POINT& screen_position, POINT* original_screen_position);

	// test function
	void RenderAlt(HDC hDC, LPRECT pRect);

private:
	LARGE_INTEGER performance_freq_;
	bool show_elapsed_time_;

	int max_width_, max_height_;

	// Disparity Color Map
	struct DispColorMap {
		double min_value;						/**<  視差表示 最小 */
		double max_value;						/**<  視差表示 最大 */

		int color_map_size;						/**<  視差表示用 LUT Size*/
		int* color_map;							/**<  視差表示用 LUT */
		double color_map_step;					/**<  start ~end までの分解能をいくつにするか */
	};
	DispColorMap disp_color_map_distance_;		/**<  視差表示用 LUT 距離基準 */
	DispColorMap disp_color_map_disparity_;		/**<  視差表示用 LUT 視差基準 */
	double max_disparity_;						/**<  視差最大値 */

	// Direct2D
	ID2D1Factory* d2d_factory_;					/**< Direct2Dファクトリ オブジェクト */
	IDWriteFactory* dwrite_factory_;			/**< Direct2D write ファクトリーオブジェクト */

	ID2D1DCRenderTarget* dc_render_target_;		/**< Direct2D DC互換のレンダー ターゲット  */

	ID2D1SolidColorBrush* brush_;				/**< Direct2D ブラシオブジェクト */
	ID2D1SolidColorBrush* brush_text_;			/**< Direct2D テキスト用ブラシオブジェクト */
	ID2D1SolidColorBrush* brush_back_;			/**< Direct2D ブラシオブジェクト */

	IDWriteTextFormat* text_format_;			/**< Direct2D テキストフォーマット */ 
	
	// Text表示用設定
	struct DrawTextFontSetting {
		int FontSize;
		float y0, y1, y2, y3;
	};
	DrawTextFontSetting draw_text_font_setting_;

	// 描画用のカラー画像
	int width_color_[2], height_color_[2];
	uint8_t	*color_[2];
	uint8_t	*temp_buffer_[8];

	// Direct2D用の画像
	int width_[2], height_[2];
	ID2D1Bitmap *bitmap_[2];

	// 位置変換用データ
	struct DisplayInformation {
		bool valid;						/**< true: valid data */
		ImageDrawMode mode;				/**< mode for display */

		SIZE original_image_size[2];	/**< original image size[left,right] */
		SIZE draw_image_size[2];		/**< draw image size[left,right] */
		SIZE image_size;				/**< image for draw size */
		double magnification;			/**< display magnification */
		POINT magnification_center;		/**< magnification center positionrequest */
		RECT rectangle_to_display[2];	/**< rectangle to display */

		D2D1_SIZE_F draw_terget_size;				/**< draw target size */	
		D2D1_POINT_2F draw_rotate_center;			/**< the center of rotation used in the display */
		D2D1_POINT_2F draw_magnification_cenater;	/**< magnification center */
		D2D1_POINT_2F draw_translation;				/**< draw shift */
		D2D1_RECT_F draw_lb_box;					/**< Rectangle of Letter Box used for display */
	};
	DisplayInformation display_information_;

	DrawParameter draw_parameter_;

	// ファイル保存用
	DpcImageWriter* dpc_image_write_;

	// functions
	HRESULT CreateDeviceResourcesAlt();

	void BuildBitmap(const int index, const ImageDrawMode mode, TextDataSet* text_data_set, DrawParameter* draw_parameter, SIZE* target_size,
		DisplayInformation* diaplay_information, ImageDataList* image_data_list_0, ImageDataList* image_data_list_1);

	int BuildColorHeatMap(DispColorMap* disp_color_map);
	int BuildColorHeatMapForDisparity(DispColorMap* disp_color_map);

	int ColorScaleBCGYR(const double min_value, const double max_value, const double in_value, int* bo, int* go, int* ro);

	bool MakeDepthColorImage(const bool is_color_by_distance, const bool is_draw_outside_bounds, const double min_length_i, const double max_length_i,
		DispColorMap* disp_color_map, double b_i, const double angle_i, const double bf_i, const double dinf_i,
		const int width, const int height, float* depth, unsigned char* bgra_image);

};

