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
 * @file ISCFrameDecoder.cpp
 * @brief This is the Frame Decoder implementatione
 * @author Osada
 * @date 2023.2.1
 * @version 0.1
 *
 * @details 
 * - カメラから出力されるフレームデータをSDK経由で取得し、画像データへ変換する  
 * 
 */

#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <math.h>

#include "ISCFrameDecoder.h"


// サブピクセル倍率 (1000倍サブピクセル精度）
#define MATCHING_SUBPIXEL_TIMES 1000

// 視差ブロック高さ
#define DISPARITY_BLOCK_HEIGHT_FPGA 4
// 視差ブロックブロック幅
#define DISPARITY_BLOCK_WIDTH_FPGA 4

// 画像サイズ
#define IMG_WIDTH_VM 752
#define IMG_WIDTH_XC 1280

// マッチング探索幅
// 入力された画像サイズでFPGAの探索幅を判断する
#define MATCHING_DEPTH_VM_FPGA 112
#define MATCHING_DEPTH_XC_FPGA 256

// コントラストオフセット
// 入力された画像サイズによってオフセットを選択する
#define CONTRAST_OFFSET_VM (1.8 * 1000)
#define CONTRAST_OFFSET_XC (1.2 * 1000)

// 輝度最小値
// ブロック内の輝度の最大値がこの値未満の場合
// コントラストをゼロにする
#define BLOCK_BRIGHTNESS_MAX 15

// ブロックロック平均輝度最小値
#define BLOCK_MIN_AVERAGE_BRIGHTNESS (7.5)

// 高解像度モードのコントラスト計算のための平均輝度
#define CONTRAST_CALC_AVERAGE_FOR_HIGH_RESO 255

// ブロック内の輝度差の最小値
#define BLOCK_MIN_DELTA_BRIGHTNESS 2

// FPGAサブピクセル精度 （小数部4ビット幅）1/16画素
#define FPGA_PARALLAX_VALUE 0.0625F

/// <summary>
/// 低感度視差画像（ピクセル単位）
/// </summary>
static unsigned char *disp_image_low; // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];

/// <summary>
/// 低感度視差情報（ピクセル単位）
/// </summary>
static float *pixel_disp_low; // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];

/// <summary>
/// 低感度ブロック視差値（視差ブロック単位）
/// </summary>
static float *block_disp_low; // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];

/// <summary>
/// 低感度ブロック視差値(1000倍サブピクセル精度の整数)（ブロック単位）
/// </summary>
static int *block_value_low; // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];

/// <summary>
/// 低感度ブロックコントラスト（視差ブロック単位）
/// </summary>
static int *block_contrast_low; // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];

/// <summary>
/// コントラスト閾値
/// </summary>
static int contrastThreshold = 40;

/// <summary>
/// 階調補正モードステータス 0:オフ 1:オン
/// </summary>
static int gradationCorrectionMode = 0;

/// <summary>
/// 視差値の制限　0:しない 1:する
/// </summary>
static int dispLimitation = 0;

/// <summary>
/// 視差の下限値
/// </summary>
static int dispLowerLimit = 0;

/// <summary>
/// 視差のの上限値
/// </summary>
static int dispUpperLimit = 255 * MATCHING_SUBPIXEL_TIMES;

/// <summary>
/// シャッターダブルシャッター補正出力 0:ブレンド 1:高感度 2:低感度 3:適当
/// </summary>
static int doubleShutterCrctOutput = 0;

/// <summary>
/// シャッターダブルシャッター補正出力 0:高感度 1:低感度
/// </summary>
static int doubleShutterCrctSuitSide = 0;

/// <summary>
/// シャッターダブルシャッター視差出力 0:ブレンド 1:高感度 2:低感度
/// </summary>
static int doubleShutterDispOutput = 0;


/// <summary>
/// フレームデコーダを初期化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
void ISCFrameDecoder::initialize(int imghgt, int imgwdt)
{
	// 低感度視差画像（ピクセル単位）
	disp_image_low = (unsigned char *)malloc(imghgt * imgwdt);  // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];
	// 低感度視差情報（ピクセル単位）
	pixel_disp_low = (float *)malloc(imghgt * imgwdt * sizeof(float)); // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];
	// 低感度ブロック視差値（視差ブロック単位）
	block_disp_low = (float *)malloc(imghgt * imgwdt * sizeof(float)); // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];
	// 低感度ブロック視差値(1000倍サブピクセル精度の整数)（ブロック単位）
	block_value_low = (int *)malloc(imghgt * imgwdt * sizeof(int)); // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];
	// 低感度ブロックコントラスト（視差ブロック単位）
	block_contrast_low = (int *)malloc(imghgt * imgwdt * sizeof(int)); // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];


}


/// <summary>
/// フレームデコーダを終了する
/// </summary>
void ISCFrameDecoder::finalize()
{
	// 低感度視差画像（ピクセル単位）
	free(disp_image_low); // = (unsigned char *)malloc(imghgt * imgwdt);  // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];
	// 低感度視差情報（ピクセル単位）
	free(pixel_disp_low); // = (float *)malloc(imghgt * imgwdt * sizeof(float)); // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];
	// 低感度ブロック視差値（視差ブロック単位）
	free(block_disp_low); // = (float *)malloc(imghgt * imgwdt * sizeof(float)); // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];
	// 低感度ブロック視差値(1000倍サブピクセル精度の整数)（ブロック単位）
	free(block_value_low);  //= (int *)malloc(imghgt * imgwdt * sizeof(int)); // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];
	// 低感度ブロックコントラスト（視差ブロック単位）
	free(block_contrast_low); // = (int *)malloc(imghgt * imgwdt * sizeof(int)); // [ISC_IMG_HEIGHT_MAX * ISC_IMG_WIDTH_MAX];


}


/// <summary>
/// フレームデコーダのパラメータを設定する
/// </summary>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
void ISCFrameDecoder::setFrameDecoderParameter(int crstthr, int grdcrct)
{
	contrastThreshold = crstthr;
	gradationCorrectionMode = grdcrct;
}


/// <summary>
/// 視差の下限値、上限値を設定する
/// </summary>
/// <param name="limit">視差値の制限　0:しない 1:する(IN)</param>
/// <param name="lower">視差値の下限(IN)</param>
/// <param name="upper">視差値の上限(IN)</param>
void ISCFrameDecoder::setDisparityLimitation(int limit, double lower, double upper)
{
	dispLimitation = limit;

	dispLowerLimit = (int)(lower * MATCHING_SUBPIXEL_TIMES);
	dispUpperLimit = (int)(upper * MATCHING_SUBPIXEL_TIMES);

}


/// <summary>
/// ダブルシャッター出力を設定する
/// </summary>
/// <param name="dblout">ダブルシャッター視差出力 0:ブレンド 1:高感度 2:低感度(IN)</param>
/// <param name="dbdout">ダブルシャッター補正画像出力 0:ブレンド 1:高感度 2:低感度 3:適当(IN)</param>
void ISCFrameDecoder::setDoubleShutterOutput(int dbdout, int dbcout)
{
	doubleShutterDispOutput = dbdout;
	doubleShutterCrctOutput = dbcout;
}


/// <summary>
/// フレームデータを画像データまたは視差エンコードデータに分割する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pfrmdat">フレームデータ(IN)</param>
/// <param name="prgtimg">右（基準）画像データ(OUT)</param>
/// <param name="plftimg">左（比較）画像データまたは視差エンコードデータ(OUT)</param>
void ISCFrameDecoder::decodeFrameData(int imghgt, int imgwdt, unsigned char* pfrmdat, 
	unsigned char* prgtimg, unsigned char* plftimg)
{
	unsigned char* buff_mix = pfrmdat; // フレームデータ 
	unsigned char* buff_img1 = plftimg; // 左カメラ比較画像 
	unsigned char* buff_img2 = prgtimg; // 右カメラ基準画像

	// フレームデータ (buff_mix) フォーマット
	//  左右画像データ交互　1画素1バイト　
	// [0][1][0][1][0][1][0][1][0][1][0][1]...
	// [0][1][0][1][0][1][0][1][0][1][0][1]...
	//
	// [0] : 左カメラ比較画像 or 視差エンコードデータ
	// [1] : 右カメラ基準画像

	for (int j = 0; j < imghgt; j++) {
		for (int i = 0; i < imgwdt; i++) {
			// 左カメラ比較画像
			*(buff_img1) = *buff_mix;
			// 右カメラ基準画像
			*(buff_img2) = *(buff_mix + 1);

			// フレームデータ
			buff_mix += 2;

			// 左右画像
			buff_img1++;
			buff_img2++;

		}
	}

}


/// <summary>
/// 右画像データと視差エンコードデータとからブロックの視差データを取得する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="prgtimg">右（基準）画像データ(IN)</param>
/// <param name="pSrcImage">視差エンコードデータ(IN)</param>
/// <param name="pDistImage">視差画像(OUT)</param>
/// <param name="pTempParallax">視差情報(OUT)</param>
/// <param name="pBlockDepth">ブロック視差情報(OUT)</param>
/// <param name="pblkval">ブロック視差値(1000倍サブピクセル精度の整数)(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void ISCFrameDecoder::decodeDisparityData(int imghgt, int imgwdt, unsigned char* prgtimg, 
	unsigned char* pSrcImage, unsigned char* pDispImage, float* pTempParallax, float* pBlockDepth,
	int* pblkval, int* pblkcrst)
{
	unsigned char* buff_mix = pSrcImage; // 視差情報
	unsigned char storeDisparity;
	float dbValue;
	int d_tmp;

	unsigned char mask1, mask2;
	int mask;
	int shift;

	// コントラスト閾値
	int crstthr = contrastThreshold;

	// 階調補正モードステータス 0:オフ 1:オン
	int grdcrct = gradationCorrectionMode;

	// FPGAのマッチング探索幅を求める
	// コントラストオフセットを設定する
	int crstofs = 0;
	int depth;
	if (imgwdt == IMG_WIDTH_VM) {
		crstofs = (int)CONTRAST_OFFSET_VM;
		depth = MATCHING_DEPTH_VM_FPGA;
	}
	else if (imgwdt == IMG_WIDTH_XC) {
		crstofs = (int)CONTRAST_OFFSET_XC;
		depth = MATCHING_DEPTH_XC_FPGA;
	}

	// ブロック輝度最大値の最小値
	int bgtmax = BLOCK_BRIGHTNESS_MAX;

	// 高解像度モードのコントラスト計算のための平均輝度
	int crstave = CONTRAST_CALC_AVERAGE_FOR_HIGH_RESO;

	// ブロック平均輝度最小値
	double minlave = BLOCK_MIN_AVERAGE_BRIGHTNESS;

	// ブロック内の輝度差の最小値
	int mindltl = BLOCK_MIN_DELTA_BRIGHTNESS;

	// 画像の幅ブロック数
	int imgwdtblk = imgwdt / DISPARITY_BLOCK_WIDTH_FPGA;

	float dsprt = (float)255 / depth; // 視差画像表示のため視差値を256階調へ変換する

	for (int j = 0, jj = 0; j < imghgt; j += DISPARITY_BLOCK_HEIGHT_FPGA, jj++) {
		for (int i = 0, ii = 0; i < imgwdt; i += DISPARITY_BLOCK_WIDTH_FPGA, ii++) {

			// ブロックの視差値とマスクデータを取得する

			// 視差エンコードデータ（buff_mix）フォーマット
			//  4x4画素ブロック単位　1ブロック4バイト　
			// [0][1][2][3][0][1][2][3][0][1][2][3]...
			// ...
			// ...
			// ...
			// [0][1][2][3][0][1][2][3][0][1][2][3]...
			// ...
			//
			// [0] : 視差整数部
			// [1] : 視差小数部
			//    [7:4] - 視差小数部
			// [2] : マスクビット1（mask1）
			//    [7:4] - ブロック4ライン目 (4画素分)
			//    [3:0] - ブロック3ライン目 (4画素分)
			// [3] : マスクビット2（mask2）
			//    [7:4] - ブロック2ライン目 (4画素分)
			//    [3:0] - ブロック1ライン目 (4画素分)
			//
			//  マスクビット画素位置
			//             +-+-+-+-+
			//   1ライン目 |0|1|2|3|
			//             +-+-+-+-+
			//   2ライン目 |4|7|6|7|
			//             +-+-+-+-+
			//   3ライン目 |0|1|2|3|
			//             +-+-+-+-+
			//   4ライン目 |4|5|6|7|
			//             +-+-+-+-+
			//

			// 視差整数部
			storeDisparity = *(buff_mix + j * imgwdt + i);

			// 視差小数部（4ビット幅）
			d_tmp = ((*(buff_mix + j * imgwdt + i + 1)) & 0xF0) >> 4;

			// 視差値（浮動小数）
			dbValue = storeDisparity;
			dbValue += (float)(d_tmp * FPGA_PARALLAX_VALUE);

			// 倍精度整数サブピクセルブロック視差値
			int parallax = (int)(dbValue * MATCHING_SUBPIXEL_TIMES);

			// 視差値の範囲を制限する
			if (dispLimitation == 1) {
				// 倍精度整数サブピクセルにして判定する
				int disp = (int)(dbValue * MATCHING_SUBPIXEL_TIMES);
				if (disp < dispLowerLimit || disp > dispUpperLimit) {
					storeDisparity = 0;
					dbValue = 0.0;
				}
			}

			// 表示用に256階調の視差値へ変換する
			storeDisparity = (unsigned char)(storeDisparity * dsprt);

			// ブロック視差情報を保存する
			pBlockDepth[jj * imgwdtblk + ii] = dbValue;

			// 視差マスクデータ
			mask1 = *(buff_mix + j * imgwdt + i + 2);
			mask2 = *(buff_mix + j * imgwdt + i + 3);
			mask = (mask1 << 8) + mask2;

			shift = 0x01;

			// 画素へ展開する
			// 視差ブロック内の最大最小輝度を取得する
			double Lsumr = 0.0; // Σ基準画像の輝度ij
			double Lmin = 255.0;
			double Lmax = 0.0;
			int blkcnt = 0;

			for (int jpxl = j; jpxl < j + DISPARITY_BLOCK_HEIGHT_FPGA; jpxl++) {
				for (int ipxl = i; ipxl < i + DISPARITY_BLOCK_WIDTH_FPGA; ipxl++) {
					// 視差値を画素へ展開する
					if (mask & shift) {
						pDispImage[jpxl * imgwdt + ipxl] = storeDisparity;
						pTempParallax[jpxl * imgwdt + ipxl] = dbValue;
					}
					else {
						pDispImage[jpxl * imgwdt + ipxl] = 0;
						pTempParallax[jpxl * imgwdt + ipxl] = 0.0;
					}
					shift = shift << 0x01;

					// 右（基準）画像データから視差ブロック内の最大最小輝度を取得する
					double Lpxl = (double)prgtimg[jpxl * imgwdt + ipxl];
					// 階調補正モードオンの場合
					// 補正前の値へ変換する
					if (grdcrct == 1) {
						Lpxl = (Lpxl * Lpxl) / 255;
					}
					Lsumr += Lpxl;
					if (Lpxl < Lmin) {
						Lmin = Lpxl;
					}
					if (Lpxl > Lmax) {
						Lmax = Lpxl;
					}
					blkcnt++;

				}
			}

			// コントラスト値を求める
			int crst = 0;

			// ブロックの輝度値の平均
			double Lave = Lsumr / blkcnt;

			// ブロック内の輝度差
			double deltaL = Lmax - Lmin;
			
			// 以下の場合はコントラスト値はゼロ
			// ブロックの最小輝度値が閾値未満
			// ブロック内の輝度差が閾値未満
			// ブロック平均輝度が閾値未満
			if (Lmax >= (double)bgtmax && deltaL >= (double)mindltl && Lave >= minlave) {
				// 階調補正モードオフの場合
					crst = (int)((deltaL * 1000 - crstofs) / Lave);
				}

			// コントラスト閾値を判定する
			if (crst < crstthr) {
				pblkval[jj * imgwdtblk + ii] = 0;
				// 視差なしにする 
				pBlockDepth[jj * imgwdtblk + ii] = 0.0;
				for (int jpxl = j; jpxl < j + DISPARITY_BLOCK_HEIGHT_FPGA; jpxl++) {
					for (int ipxl = i; ipxl < i + DISPARITY_BLOCK_WIDTH_FPGA; ipxl++) {
						pDispImage[jpxl * imgwdt + ipxl] = 0;
						pTempParallax[jpxl * imgwdt + ipxl] = 0.0;
					}
				}
			}
			else {
				pblkval[jj * imgwdtblk + ii] = parallax;
			}

			// コントラストを保存する
			pblkcrst[jj * imgwdtblk + ii] = crst;

		}
	}

	return;
}


/// <summary>
/// 視差データをデコードして視差画像と視差情報に戻し、平均化、補完処理を行う
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="prgtimg">右（基準）画像データ(IN)</param>
/// <param name="pdspenc">視差エンコードデータ(IN)</param>
/// <param name="pblkhgt">視差ブロック高さ(OUT)</param>
/// <param name="pblkwdt">視差ブロック幅(OUT)</param>
/// <param name="pmtchgt">マッチングブロック高さ(OUT)</param>
/// <param name="pmtcwdt">マッチングブロック幅(OUT)</param>
/// <param name="pblkofsx">視差ブロック横オフセット(OUT)</param>
/// <param name="pblkofsy">視差ブロック縦オフセット(OUT)</param>
/// <param name="pdepth">マッチング探索幅(OUT)</param>
/// <param name="pshdwdt">画像遮蔽幅(OUT)</param>
/// <param name="pdspimg">視差画像 右下原点(OUT)</param>
/// <param name="ppxldsp">視差データ 右下原点(OUT)</param>
/// <param name="pblkdsp">ブロック視差データ 右下基点(OUT)</param>
/// <param name="pblkval">視差ブロック視差値(1000倍サブピクセル精度整数)(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void ISCFrameDecoder::getDisparityData(int imghgt, int imgwdt, unsigned char* prgtimg, unsigned char* pdspenc,
	int* pblkhgt, int* pblkwdt, int* pmtchgt, int* pmtcwdt,
	int* pblkofsx, int* pblkofsy, int* pdepth, int* pshdwdt,
	unsigned char* pdspimg, float* ppxldsp, float* pblkdsp, int *pblkval, int *pblkcrst)
{

	// 視差データを取得する
	// ブロックの視差値とコントラストを取得する
	decodeDisparityData(imghgt, imgwdt, prgtimg, pdspenc,
		pdspimg, ppxldsp, pblkdsp, pblkval, pblkcrst);

	// FPGAのマッチング探索幅を求める
	int depth = 0;
	if (imgwdt == IMG_WIDTH_VM) {
		depth = MATCHING_DEPTH_VM_FPGA;
	}
	else if (imgwdt == IMG_WIDTH_XC) {
		depth = MATCHING_DEPTH_XC_FPGA;
	}

	*pblkhgt = DISPARITY_BLOCK_HEIGHT_FPGA;
	*pblkwdt = DISPARITY_BLOCK_WIDTH_FPGA;
	*pmtchgt = DISPARITY_BLOCK_HEIGHT_FPGA;
	*pmtcwdt = DISPARITY_BLOCK_WIDTH_FPGA;
	*pblkofsx = 0;
	*pblkofsy = 0;
	*pdepth = depth;
	*pshdwdt = depth;

#if 0
	// 上位での呼び出しへ変更

	// 視差データを平均化する
	DisparityFilter::averageDisparityData(imghgt, imgwdt, prgtimg,
		DISPARITY_BLOCK_HEIGHT_FPGA, DISPARITY_BLOCK_WIDTH_FPGA,
		DISPARITY_BLOCK_HEIGHT_FPGA, DISPARITY_BLOCK_WIDTH_FPGA,
		0, 0, depth, depth,
		pblkval, pblkcrst, pdspimg, ppxldsp, pblkdsp);
#endif

}


/// <summary>
/// ダブルシャッターの視差エンコードデータをデコードする
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pimghigh">高感度フレーム画像データ(IN)</param>
/// <param name="penchigh">高感度フレーム視差エンコードデータ(IN)</param>
/// <param name="pimglow">低感度フレーム画像データ(IN)</param>
/// <param name="penclow">低感度フレーム視差エンコードデータ(IN)</param>
/// <param name="pbldimg">合成画像(OUT)</param>
/// <param name="pdspimg">視差画像(OUT)</param>
/// <param name="ppxldsp">視差情報(OUT)</param>
/// <param name="pblkdsp">ブロック視差情報(OUT)</param>
/// <param name="pblkval">ブロック視差値(1000倍サブピクセル精度の整数)(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void ISCFrameDecoder::decodeDoubleDisparityData(int imghgt, int imgwdt,
	unsigned char* pimghigh, unsigned char* penchigh,
	unsigned char* pimglow, unsigned char* penclow,
	unsigned char* pbldimg, unsigned char* pdspimg, float* ppxldsp, float* pblkdsp,
	int* pblkval, int* pblkcrst)
{
	// ダブルシャッター補正出力設定
	// 0:合成 1:高感度 2:低感度 3:適当
	int blendcrctsel = doubleShutterCrctOutput;

	// 補正出力適当指定時 0:高感度 1:低感度
	int suitcrct = doubleShutterCrctSuitSide;
	if (blendcrctsel == 3) {
		if (suitcrct == 0) {
			blendcrctsel = 1;
		}
		else {
			blendcrctsel = 2;
		}
	}

	// ダブルシャッター視差出力設定
	// 0:合成 1:高感度 2:低感度
	int blenddspsel = doubleShutterDispOutput;

	// 出力視差画像
	unsigned char *poutimg;

	// デコードする
	// 視差の低感度を出力する場合
	if (blenddspsel == 2) {
		// 低感度をエンコードする
		decodeDisparityData(imghgt, imgwdt, pimglow, penclow,
			pdspimg, ppxldsp, pblkdsp, pblkval, pblkcrst);
		poutimg = pimglow;
	}
	// 視差の合成または高感度を出力する場合
	else {
		// 高感度をエンコードする
		decodeDisparityData(imghgt, imgwdt, pimghigh, penchigh,
			pdspimg, ppxldsp, pblkdsp, pblkval, pblkcrst);
		poutimg = pimghigh;
	}

	memcpy(pbldimg, poutimg, imghgt * imgwdt);

	// 視差を合成する場合
	if (blenddspsel == 0) {
		// 低感度をエンコードする
		decodeDisparityData(imghgt, imgwdt, pimglow, penclow,
			disp_image_low, pixel_disp_low, block_disp_low, block_value_low, block_contrast_low);

		// 合成する
		blendDisparityData(imghgt, imgwdt,
			pbldimg, pdspimg, ppxldsp, pblkdsp, pblkval, pblkcrst,
			pimglow, disp_image_low, pixel_disp_low, block_disp_low, block_value_low, block_contrast_low);

		// 補正画像高感度
		if (blendcrctsel == 1) {
			memcpy(pbldimg, pimghigh, imghgt * imgwdt);
		}
		// 補正画像低感度
		else if (blendcrctsel == 2) {
			memcpy(pbldimg, pimglow, imghgt * imgwdt);
		}
	}

}


/// <summary>
/// ダブルシャッターの視差エンコードデータをデコードして、視差の平均化、補完処理を行う
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pimgcur">現フレーム画像データ(IN)</param>
/// <param name="penccur">現フレーム視差エンコードデータ(IN)</param>
/// <param name="pexpcur">現フレームシャッター露光値(IN)</param>
/// <param name="pgaincur">現フレームシャッターゲイン値(IN)</param>
/// <param name="pimgprev">前フレーム画像データ(IN)</param>
/// <param name="pencprev">前フレーム視差エンコードデータ(IN)</param>
/// <param name="pexpprev">前フレームシャッター露光値(IN)</param>
/// <param name="pgainprev">前フレームシャッターゲイン値(IN)</param>
/// <param name="pblkhgt">視差ブロック高さ(OUT)</param>
/// <param name="pblkwdt">視差ブロック幅(OUT)</param>
/// <param name="pmtchgt">マッチングブロック高さ(OUT)</param>
/// <param name="pmtcwdt">マッチングブロック幅(OUT)</param>
/// <param name="pblkofsx">視差ブロック横オフセット(OUT)</param>
/// <param name="pblkofsy">視差ブロック縦オフセット(OUT)</param>
/// <param name="pdepth">マッチング探索幅(OUT)</param>
/// <param name="pshdwdt">画像遮蔽幅(OUT)</param>
/// <param name="pbldimg">合成画像 右下基点(OUT)</param>
/// <param name="pdspimg">視差画像 右下基点(OUT)</param>
/// <param name="ppxldsp">視差情報 右下基点(OUT)</param>
/// <param name="pblkdsp">ブロック視差情報 右下基点(OUT)</param>
/// <param name="pblkval">ブロック視差値(1000倍サブピクセル精度の整数)(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void ISCFrameDecoder::getDoubleDisparityData(int imghgt, int imgwdt,
	unsigned char* pimgcur, unsigned char* penccur, int expcur, int gaincur,
	unsigned char* pimgprev, unsigned char* pencprev, int expprev, int gainprev,
	int* pblkhgt, int* pblkwdt, int* pmtchgt, int* pmtcwdt,
	int* pblkofsx, int* pblkofsy, int* pdepth, int* pshdwdt,
	unsigned char* pbldimg, unsigned char* pdspimg, float* ppxldsp, float* pblkdsp,
	int* pblkval, int* pblkcrst)
{

	// 高感度、低感度フレームを判定する
	unsigned char *pimghigh = pimgcur;
	unsigned char *penchigh = penccur;
	unsigned char *pimglow = pimgprev;
	unsigned char *penclow = pencprev;

	int gainhigh = gaincur;
	int gainlow = gainprev;

	// 最適な補正画像
	// 0:高感度側 1:低感度側
	// 半自動ダブルシャッターの場合、露光調整中は低感度側
	doubleShutterCrctSuitSide = 0;

	// FPGAのマッチング探索幅を求める
	int depth = 0;
	if (imgwdt == IMG_WIDTH_VM) {
		depth = MATCHING_DEPTH_VM_FPGA;
		// 露光値が	大きい方が高感度
		// 露光値が同じならゲイン値の大きい方が高感度
		if (expcur < expprev || gaincur < gainprev) {
			pimglow = pimgcur;
			penclow = penccur;
			pimghigh = pimgprev;
			penchigh = pencprev;
		}
	}
	else if (imgwdt == IMG_WIDTH_XC) {
		depth = MATCHING_DEPTH_XC_FPGA;
		// 露光値が小さい方が高感度
		// 露光値が同じならゲイン値の大きい方が高感度
		if (expcur > expprev || gaincur < gainprev) {
			pimglow = pimgcur;
			penclow = penccur;
			pimghigh = pimgprev;
			penchigh = pencprev;
			gainhigh = gainprev;
			gainlow = gaincur;

		}

		// 通常照度では高感度側と低感度側で露光値が異なる
		// 低照度では露光を最高にしてゲインを使用する
		if (expcur != expprev || gainlow < 250) {
			doubleShutterCrctSuitSide = 1;
		}
	}

	decodeDoubleDisparityData(imghgt, imgwdt,
		pimghigh, penchigh,
		pimglow, penclow,
		pbldimg, pdspimg, ppxldsp, pblkdsp,
		pblkval, pblkcrst);

	*pblkhgt = DISPARITY_BLOCK_HEIGHT_FPGA;
	*pblkwdt = DISPARITY_BLOCK_WIDTH_FPGA;
	*pmtchgt = DISPARITY_BLOCK_HEIGHT_FPGA;
	*pmtcwdt = DISPARITY_BLOCK_WIDTH_FPGA;
	*pblkofsx = 0;
	*pblkofsy = 0;
	*pdepth = depth;
	*pshdwdt = depth;

#if 0
	// 上位での呼び出しへ変更
	
	// 視差データを平均化する
	DisparityFilter::averageDisparityData(imghgt, imgwdt, pbldimg,
		DISPARITY_BLOCK_HEIGHT_FPGA, DISPARITY_BLOCK_WIDTH_FPGA,
		DISPARITY_BLOCK_HEIGHT_FPGA, DISPARITY_BLOCK_WIDTH_FPGA,
		0, 0, depth, depth,
		pblkval, pblkcrst, pdspimg, ppxldsp, pblkdsp);
#endif

}


/// <summary>
/// 視差データを合成する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pbldimg_h">高感度 合成画像(IN/OUT)</param>
/// <param name="pdspimg_h">高感度 視差画像(IN/OUT)</param>
/// <param name="ppxldsp_h">高感度 視差情報(IN/OUT)</param>
/// <param name="pblkdsp_h">高感度 ブロック視差情報(IN/OUT)</param>
/// <param name="pblkval_h">高感度 ブロック視差値(1000倍サブピクセル精度の整数)(IN/OUT)</param>
/// <param name="pblkcrst_h">高感度 ブロックコントラスト(IN/OUT)</param>
/// <param name="pbldimg_l">低感度 合成画像(IN)</param>
/// <param name="pdspimg_l">低感度 視差画像(IN)</param>
/// <param name="ppxldsp_l">低感度 視差情報(IN)</param>
/// <param name="pblkdsp_l">低感度 ブロック視差情報(IN)</param>
/// <param name="pblkval_l">低感度 ブロック視差値(1000倍サブピクセル精度の整数)(IN)</param>
/// <param name="pblkcrst_l">低感度 ブロックコントラスト(IN)</param>
void ISCFrameDecoder::blendDisparityData(int imghgt, int imgwdt, 
	unsigned char* pbldimg_h, unsigned char* pdspimg_h, float* ppxldsp_h, float* pblkdsp_h,	int* pblkval_h, int* pblkcrst_h,
	unsigned char* pbldimg_l, unsigned char* pdspimg_l, float* ppxldsp_l, float* pblkdsp_l, int* pblkval_l, int* pblkcrst_l)
{

	// FPGAのマッチング探索幅を求める
	int depth;
	if (imgwdt == IMG_WIDTH_VM) {
		depth = MATCHING_DEPTH_VM_FPGA;
	}
	else if (imgwdt == IMG_WIDTH_XC) {
		depth = MATCHING_DEPTH_XC_FPGA;
	}

	// 画像の幅ブロック数
	int imgwdtblk = imgwdt / DISPARITY_BLOCK_WIDTH_FPGA;

	float dsprt = (float)255 / depth; // 視差画像表示のため視差値を256階調へ変換する

	for (int j = 0, jj = 0; j < imghgt; j += DISPARITY_BLOCK_HEIGHT_FPGA, jj++) {
		for (int i = 0, ii = 0; i < imgwdt; i += DISPARITY_BLOCK_WIDTH_FPGA, ii++) {
			// ブロック単位の合成
			// 高感度側が視差なしの場合、低感度の視差で埋める
			if (pblkval_h[jj * imgwdtblk + ii] == 0) {
				pblkval_h[jj * imgwdtblk + ii] = pblkval_l[jj * imgwdtblk + ii];
				pblkcrst_h[jj * imgwdtblk + ii] = pblkcrst_l[jj * imgwdtblk + ii];
			}

			// 画素単位の合成
			// SDKの視差合成
			float dsp = (float)pblkval_h[jj * imgwdtblk + ii] / MATCHING_SUBPIXEL_TIMES;
			for (int jpxl = j; jpxl < j + DISPARITY_BLOCK_HEIGHT_FPGA; jpxl++) {
				for (int ipxl = i; ipxl < i + DISPARITY_BLOCK_WIDTH_FPGA; ipxl++) {
					// 高感度側が視差なしの場合、低感度の視差で埋める
					if (ppxldsp_h[jpxl * imgwdt + ipxl] < 2.0) {
						ppxldsp_h[jpxl * imgwdt + ipxl] = ppxldsp_l[jpxl * imgwdt + ipxl];
						pdspimg_h[jpxl * imgwdt + ipxl] = pdspimg_l[jpxl * imgwdt + ipxl];
						// 補正画像の合成
						// 高感度側が飽和している場合に低感度の輝度で埋める
						if (pbldimg_h[jpxl * imgwdt + ipxl] >= 255) {
							pbldimg_h[jpxl * imgwdt + ipxl] = pbldimg_l[jpxl * imgwdt + ipxl];
						}
					}
				}
			}
		}
	}

}


