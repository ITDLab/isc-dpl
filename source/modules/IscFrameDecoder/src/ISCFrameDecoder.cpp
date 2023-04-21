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
 * - 視差の平均化、補完処理を行う  
 * 
 */

#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "ISCFrameDecoder.h"

// サブピクセル倍率 (1000倍サブピクセル精度）
#define MATCHING_SUBPIXEL_TIMES 1000

// 平均化最大ブロック数 (17 x 17)
#define AVERAGING_BLOCKS_MAX 289

// 視差ブロック高さ
static int DISPARITY_BLOCK_HEIGHT_FPGA = 4;
// 視差ブロックブロック幅
static int DISPARITY_BLOCK_WIDTH_FPGA = 4;

// 画像サイズ
#define IMG_WIDTH_VM 752
#define IMG_WIDTH_XC 1280

// マッチング探索幅
// 入力された画像サイズでFPGAの探索幅を判断する
#define MATCHING_DEPTH_VM_FPGA 112
#define MATCHING_DEPTH_XC_FPGA 256

static int MATCHING_DEPTH_FPGA = MATCHING_DEPTH_XC_FPGA;

// コントラストオフセット
// 入力された画像サイズによってオフセットを選択する
#define CONTRAST_OFFSET_VM (1.8 * 1000)
#define CONTRAST_OFFSET_XC (1.2 * 1000)

// 輝度最小値
// ブロック内の輝度の最大値がこの値未満の場合
// コントラストをゼロにする
#define BLOCK_BRIGHTNESS_MAX 20

// FPGAサブピクセル精度 （小数部4ビット幅）1/16画素
#define FPGA_PARALLAX_VALUE 0.0625F

/// <summary>
/// 倍精度整数サブピクセル視差値（ブロックごと）
/// </summary>
static int *block_disp;

/// <summary>
/// 倍精度整数サブピクセル平均化視差値（ブロックごと）
/// </summary>
static float *average_disp;

/// <summary>
/// ブロックコントラスト（ブロックごと）
/// </summary>
static int *block_contrast;

/// <summary>
/// 視差平均化
/// </summary>

/// <summary>
/// 視差平均化処理にOpenCL使用　しない：0 する：1
/// </summary>
static int useOpenCLForAveDisp = 0;

/// <summary>
/// 特異点除去処理、視差平均化のための視差値(block_disp[]からコピー)
/// </summary>
static int *wrk;

/// <summary>
/// 平均化視差の処理 しない：0 する：1
/// </summary>
static int dispAveDisp = 0;

/// <summary>
/// 視差ブロック高さ
/// </summary>
static int disparityBlockHeight = DISPARITY_BLOCK_HEIGHT_FPGA;

/// <summary>
/// 視差ブロック幅
/// </summary>
static int disparityBlockWidth = DISPARITY_BLOCK_WIDTH_FPGA;

/// <summary>
/// 視差ブロック対角幅
/// </summary>
static double disparityBlockDiagonal = 5.657;

/// <summary>
/// マッチングブロック高さ
/// </summary>
static int matchingBlockHeight = DISPARITY_BLOCK_HEIGHT_FPGA;

/// <summary>
/// マッチングブロック幅
/// </summary>
static int matchingBlockWidth = DISPARITY_BLOCK_WIDTH_FPGA;

/// <summary>
/// 視差ブロック横オフセット
/// </summary>
static int dispBlockOffsetX = 0;

/// <summary>
/// 視差ブロック縦オフセット
/// </summary>
static int dispBlockOffsetY = 0;

/// <summary>
/// マッチング探索幅
/// </summary>
static int matchingDepth = MATCHING_DEPTH_XC_FPGA;

/// <summary>
/// 遮蔽領域幅
/// </summary>
static int shadeBandWidth = MATCHING_DEPTH_XC_FPGA;

/// <summary>
/// 視差平均化ブロック高さ（片側）
/// </summary>
static int dispAveBlockHeight = 3;

/// <summary>
/// 視差平均化ブロック幅（片側）
/// </summary>
static int dispAveBlockWidth = 3;

/// <summary>
/// 視差平均化移動積分幅（片側）
/// </summary>
static int dispAveIntegRange = 1 * MATCHING_SUBPIXEL_TIMES;

/// <summary>
/// 視差平均化分布範囲最大幅（片側）
/// </summary>
static int dispAveLimitRange = 2 * MATCHING_SUBPIXEL_TIMES;

/// <summary>
/// 視差平均化有含有率
/// </summary>
static int dispAveDispRatio = 20;

/// <summary>
/// 視差平均化有効比率
/// </summary>
static int dispAveValidRatio = 20;

/// <summary>
/// 視差平均化置換有効比率
/// </summary>
static int dispAveReplacementRatio = 50;

/// <summary>
/// 視差平均化ブロックの重み（中央）
/// </summary>
static int dispAveBlockWeightCenter = 1;

/// <summary>
/// 視差平均化ブロックの重み（近傍）
/// </summary>
static int dispAveBlockWeightNear = 1;

/// <summary>
/// 視差平均化ブロックの重み（周辺）
/// </summary>
static int dispAveBlockWeightRound = 1;

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
/// 視差補完
/// </summary>

/// <summary>
/// 視差補完処理のための視差値
/// </summary>
/// <remarks>
/// ブロック単位
/// </remarks>
static int *blkcmp;

//// <summary>
/// 視差補完処理のための視差値(小数部)
/// </summary>
/// <remarks>
/// ブロック単位
/// </remarks>
static float *avecmp;

/// <summary>
/// 視差補完処理のための重み
/// </summary>
/// <remarks>
/// ブロック単位
/// </remarks>
static int *wgtcmp;

/// <summary>
/// 視差を補完する
/// </summary>
static int dispComplementDisparity = 0;

/// <summary>
/// 視差を補完する最小視差値 
/// </summary>
static double dispComplementLowLimit = 5;

/// <summary>
/// 視差の補完幅の最大視差勾配
/// </summary>
static double dispComplementSlopeLimit = 0.1;

/// <summary>
/// 視差を補完する画素幅の視差値倍率（内側）
/// </summary>
static double dispComplementRatioInside = 1.0;

/// <summary>
/// 視差を補完する画素幅の視差値倍率（周辺）
/// </summary>
static double dispComplementRatioRound = 0.2;

/// <summary>
/// 視差を補完する画素幅の視差値倍率（下端）
/// </summary>
static double dispComplementRatioBottom = 0.1;

/// <summary>
/// 視差を補完するコントラストの上限値
/// </summary>
/// <remarks>
/// コントラスト x 1000
/// </remarks>
static int dispComplementContrastLimit = 20;

/// <summary>
/// 視差補完領域の穴埋め　0:しない 1:する
/// </summary>
static int dispComplementHoleFilling = 0;

/// <summary>
/// 視差補完領域の穴埋め幅（画素数）
/// </summary>
static double dispComplementHoleSize = 8.0;

/// <summary>
/// ブロック視差値保存フラグ
/// </summary>
static bool saveBlockInfo = false;

/// <summary>
/// バンド視差平均化
/// </summary>

#define NUM_OF_BANDS 8

#define MAX_NUM_OF_BANDS 40

static int numOfBands = NUM_OF_BANDS;

/** @struct  BNAD_THREAD_INFO
 *  @brief data for Thread execution
 */
struct BNAD_THREAD_INFO {

	// 視差平均化スレッドオブジェクトのポインタ
	HANDLE bandThread;

	// 視差平均化スレッド実行開始イベントのハンドル
	HANDLE startEvent;
	// 視差平均化スレッド停止イベントのハンドル
	HANDLE stopEvent;
	// 視差平均化スレッド完了通知イベントのハンドル
	HANDLE doneEvent;

	// 画像のブロック高さ
	int imghgtblk;
	// 画像のブロック幅
	int imgwdtblk;
	// 視差画像のブロック幅
	int dspwdtblk;

	// バンド開始ブロック位置
	int bandStart;
	// バンド終了ブロック位置
	int bandEnd;

	// ブロック視差値(倍精度整数サブピクセル)
	int * pblkval;
	// ブロック視差値(倍精度浮動小数)
	float *pavedsp;

};

static BNAD_THREAD_INFO bandInfo[MAX_NUM_OF_BANDS];


/// <summary>
/// フレームデコーダを初期化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
void ISCFrameDecoder::initialize(int imghgt, int imgwdt)
{
	// バッファーを確保する
	// 倍精度整数サブピクセル視差値（ブロックごと）
	block_disp = (int *)malloc(imghgt * imgwdt * sizeof(int));
	// 倍精度整数サブピクセル平均化視差値（ブロックごと）
	average_disp = (float *)malloc(imghgt * imgwdt * sizeof(float));
	// ブロックコントラスト（ブロックごと）
	block_contrast = (int *)malloc(imghgt * imgwdt * sizeof(int));
	// 特異点除去処理、視差平均化のための視差値(block_disp[]からコピー)
	wrk = (int *)malloc(imghgt * imgwdt * sizeof(int));
	// 視差補完処理のための視差値c
	blkcmp = (int *)malloc(imgwdt * sizeof(int));
	// 視差補完処理のための視差値(小数部)
	avecmp = (float *)malloc(imgwdt * sizeof(float));
	// 視差補完処理のための重み
	wgtcmp = (int *)malloc(imgwdt * sizeof(int));


}


/// <summary>
/// フレームデコーダを終了する
/// </summary>
void ISCFrameDecoder::finalize()
{
	// バッファーを解放する
	// 倍精度整数サブピクセル視差値（ブロックごと）
	free(block_disp);
	// 倍精度整数サブピクセル平均化視差値（ブロックごと）
	free(average_disp);
	// ブロックコントラスト（ブロックごと）
	free(block_contrast);
	// 特異点除去処理、視差平均化のための視差値(block_disp[]からコピー)
	free(wrk);
	// 視差補完処理のための視差値c
	free(blkcmp);
	// 視差補完処理のための視差値(小数部)
	free(avecmp);
	// 視差補完処理のための重み
	free(wgtcmp);


}


/// <summary>
/// 視差平均化処理にOpenCLの使用を設定する
/// </summary>
/// <param name="usecl"></param>
void ISCFrameDecoder::setUseOpenCLForAveragingDisparity(int usecl)
{
	useOpenCLForAveDisp = usecl;
}


/// <summary>
/// ブロックの視差値を保存する
/// </summary>
void ISCFrameDecoder::saveBlockDisparity()
{
	if (dispAveDisp == 1) {
		saveBlockInfo = true;
	}
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
/// 視差平均化パラメータを設定する
/// </summary>
/// <param name="enb">平均化処理しない：0 する：1(IN)</param>
/// <param name="blkshgt">視差平均化ブロック高さ（片側）(IN)</param>
/// <param name="blkswdt">視差平均化ブロック幅（片側）(IN)</param>
/// <param name="intg">視差平均化移動積分幅（片側）(IN)</param>
/// <param name="range">視差平均化分布範囲最大幅（片側）(IN)</param>
/// <param name="dsprt">視差平均化視差含有率(IN)</param>
/// <param name="vldrt">視差平均化有効比率(IN)</param>
/// <param name="reprt">視差平均化置換有効比率(IN)</param>
void ISCFrameDecoder::setAveragingParameter(int enb, int blkshgt, int blkswdt,
	double intg, double range, int dsprt, int vldrt, int reprt)
{

	// 平均化視差の処理 しない：0 する：1
	dispAveDisp = enb;
	// 視差平均化ブロック高さ（片側）
	dispAveBlockHeight = blkshgt;
	// 視差平均化ブロック幅（片側）
	dispAveBlockWidth = blkswdt;
	// 視差平均化移動積分幅（片側）
	dispAveIntegRange = (int)(intg * MATCHING_SUBPIXEL_TIMES);
	// 視差平均化分布範囲最大幅（片側）
	dispAveLimitRange = (int)(range * MATCHING_SUBPIXEL_TIMES);
	// 視差平均化有含有率
	dispAveDispRatio = dsprt;
	// 視差平均化有効比率
	dispAveValidRatio = vldrt;
	// 視差平均化置換有効比率
	dispAveReplacementRatio = reprt;

}


/// <summary>
/// 視差平均化ブロックの重みを設定する
/// </summary>
/// <param name="cntwgt">ブロックの重み（中央）(IN)</param>
/// <param name="nrwgt">ブロックの重み（近傍）(IN)</param>
/// <param name="rndwgt">ブロックの重み（周辺）(IN)</param>
void ISCFrameDecoder::setAveragingBlockWeight(int cntwgt, int nrwgt, int rndwgt)
{
	// 視差平均化ブロックの重み（中央）
	dispAveBlockWeightCenter = cntwgt;
	// 視差平均化ブロックの重み（近傍）
	dispAveBlockWeightNear = nrwgt;
	// 視差平均化ブロックの重み（周辺）
	dispAveBlockWeightRound = rndwgt;

}


/// <summary>
/// 視差補完パラメータを設定する
/// </summary>
/// <param name="enb">補完処理しない：0 する：1(IN)</param>
/// <param name="lowlmt">補完最小視差値(IN)</param>
/// <param name="slplmt">補完幅の最大視差勾配(IN)</param>
/// <param name="insrt">補完画素幅の視差値倍率（内側）(IN)</param>
/// <param name="rndrt">補完画素幅の視差値倍率（周辺）(IN)</param>
/// <param name="btmrt">補完画素幅の視差値倍率（下端）(IN)</param>
/// <param name="crstlmt">補完ブロックのコントラスト上限値(IN)</param>
/// <param name="hlfil">穴埋め処理しない：0 する：1 (IN)</param>
/// <param name="hlsz">穴埋め幅 (IN)</param>
void ISCFrameDecoder::setComplementParameter(int enb, double lowlmt, double slplmt, 
	double insrt, double rndrt, double btmrt, int crstlmt, int hlfil, double hlsz)
{

	// 視差を補完する
	dispComplementDisparity = enb;
	// 視差を補完する最小視差値
	dispComplementLowLimit = lowlmt;
	// 視差の補完幅の最大視差勾配
	dispComplementSlopeLimit = slplmt;

	// 視差を補完する画素幅の視差値倍率（内側）
	dispComplementRatioInside = insrt;
	// 視差を補完する画素幅の視差値倍率（周辺）
	dispComplementRatioRound = rndrt;
	// 視差を補完する画素幅の視差値倍率（下端）
	dispComplementRatioBottom = btmrt;
	// 補完ブロックのコントラスト上限値
	dispComplementContrastLimit = crstlmt;

	// 視差補完領域の穴埋め　0:しない 1:する
	dispComplementHoleFilling = hlfil;
	// 視差補完領域の穴埋め幅
	dispComplementHoleSize = hlsz;

}


/// <summary>
/// フレームデータを画像データに分割する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pfrmdat">フレームデータ(IN)</param>
/// <param name="prgtimg">右（基準）画像データ 右下原点(OUT)</param>
/// <param name="plftimg">左（比較）画像データ 右下原点(OUT)</param>
/// <param name="pdspenc">視差エンコードデータ(OUT)</param>
void ISCFrameDecoder::decodeFrameData(int imghgt, int imgwdt, unsigned char* pfrmdat, 
	unsigned char* prgtimg, unsigned char* plftimg, unsigned char* pdspenc)
{
	unsigned char* buff_mix = pfrmdat; // フレームデータ 
	unsigned char* buff_img1 = plftimg; // 左カメラ比較画像 
	unsigned char* buff_img2 = prgtimg; // 右カメラ基準画像
	unsigned char* buff_img3 = pdspenc; // 視差エンコードデータ

	// フレームデータ (buff_mix) フォーマット
	//  左右画像データ交互　1画素1バイト　
	// [0][1][0][1][0][1][0][1][0][1][0][1]...
	// [0][1][0][1][0][1][0][1][0][1][0][1]...
	//
	// [0] : 左カメラ比較画像 or 視差エンコードデータ
	// [1] : 右カメラ基準画像

	for (int j = 0; j < imghgt; j++) {
		for (int i = 0; i < imgwdt; i++) {
			// 視差エンコードデータ
			// 視差値のデコードのために保存
			*(buff_img3) = *buff_mix;

			// 左カメラ比較画像
			*(buff_img1) = *buff_mix;
			// 右カメラ基準画像
			*(buff_img2) = *(buff_mix + 1);

			// フレームデータ
			buff_mix += 2;

			// 左右画像
			buff_img1++;
			buff_img2++;

			// 視差エンコードデータ
			buff_img3++;
		}
	}

}


/// <summary>
/// 視差データをデコードして視差画像と視差情報に戻し、平均化、補完処理を行う
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="prgtimg">右（基準）画像データ 右下原点(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="pdspenc">視差エンコードデータ(IN)</param>
/// <param name="pdspimg">視差画像 右下原点(OUT)</param>
/// <param name="ppxldsp">視差データ 右下原点(OUT)</param>
/// <param name="pblkdsp">ブロック視差データ 右下基点(OUT)</param>
void ISCFrameDecoder::decodeDisparityData(int imghgt, int imgwdt, 
	unsigned char* prgtimg, int crstthr, unsigned char* pdspenc,
	unsigned char* pdspimg, float* ppxldsp, float* pblkdsp)
{

	// FPGAのマッチング探索幅を求める
	if (imgwdt == IMG_WIDTH_VM) {
		MATCHING_DEPTH_FPGA = MATCHING_DEPTH_VM_FPGA;
	}
	else if (imgwdt == IMG_WIDTH_XC) {
		MATCHING_DEPTH_FPGA = MATCHING_DEPTH_XC_FPGA;
	}

	// 視差データをデコードする
	if (dispAveDisp == 0) {
		decodeDisparityDirect(imghgt, imgwdt, pdspenc, pdspimg, ppxldsp, pblkdsp);
	}
	// 視差データをデコードして平均化する
	else {

		// コントラストオフセット
		int crstofs = 0;
		if (imgwdt == IMG_WIDTH_VM) {
			crstofs = (int)CONTRAST_OFFSET_VM;
		}
		else if (imgwdt == IMG_WIDTH_XC) {
			crstofs = (int)CONTRAST_OFFSET_XC;
		}

		// ブロック輝度最大値
		int bgtmax = BLOCK_BRIGHTNESS_MAX;

		// ブロックの視差値とコントラストを取得する
		getDisparityData(imghgt, imgwdt, prgtimg, crstthr, crstofs, bgtmax,
			pdspenc, block_disp, block_contrast);

		averageDisparityData(imghgt, imgwdt,
			DISPARITY_BLOCK_HEIGHT_FPGA, DISPARITY_BLOCK_WIDTH_FPGA,
			DISPARITY_BLOCK_HEIGHT_FPGA, DISPARITY_BLOCK_WIDTH_FPGA,
			0, 0, MATCHING_DEPTH_FPGA, MATCHING_DEPTH_FPGA,
			block_disp, block_contrast,
			pdspimg, ppxldsp, pblkdsp);

	}

}


/// <summary>
/// 視差を平均化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="blkhgt">視差ブロックの高さ(IN)</param>
/// <param name="blkwdt">視差ブロックの幅(IN)</param>
/// <param name="mtchgt">マッチングブロックの高さ(IN)</param>
/// <param name="mtcwdt">マッチングブロックの幅(IN)</param>
/// <param name="dspofsx">視差ブロック横オフセット(IN)</param>
/// <param name="dspofsy">視差ブロック縦オフセット(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="shdwdt">遮蔽領域幅(IN)</param>
/// <param name="pblkval">視差ブロック視差値(1000倍サブピクセル精度整数)(IN)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
/// <param name="pdspimg">視差画像 右下原点(OUT)</param>
/// <param name="ppxldsp">視差データ 右下原点(OUT)</param>
/// <param name="pblkdsp">ブロック視差データ 右下原点(OUT)</param>
/// <returns>処理結果を返す</returns>
bool ISCFrameDecoder::averageDisparityData(int imghgt, int imgwdt,
	int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int dspofsx, int dspofsy, int depth, int shdwdt,
	int *pblkval, int *pblkcrst,
	unsigned char* pdspimg, float* ppxldsp, float* pblkdsp)
{

	if (dispAveDisp == 0) {
		return false;
	}

	// 視差ブロック高さ
	disparityBlockHeight = blkhgt;
	// 視差ブロック幅
	disparityBlockWidth = blkwdt;
	// 視差ブロック対角幅
	disparityBlockDiagonal = sqrt(blkhgt * blkhgt + blkwdt * blkwdt);

	// マッチングブロック高さ
	matchingBlockHeight = mtchgt;
	// マッチングブロック幅
	matchingBlockWidth = mtcwdt;

	// マッチング探索幅
	matchingDepth = depth;
	// 遮蔽領域幅
	shadeBandWidth = shdwdt;

	// 視差ブロック横オフセット
	dispBlockOffsetX = dspofsx;
	// 視差ブロック縦オフセット
	dispBlockOffsetY = dspofsy;

	// 視差値を平均化する
	if (useOpenCLForAveDisp == 0) {
		getAveragingDisparity(imghgt, imgwdt, pblkval, average_disp);
	}
	else {
		getAveragingDisparityOpenCV(imghgt, imgwdt, pblkval, average_disp);
	}

	// 視差補完する
	if (dispComplementDisparity == 1) {
		getComplementDisparity(imghgt, imgwdt, pblkval, average_disp, pblkcrst);
	}

	// 視差穴埋めをする
	if (dispComplementHoleFilling == 1) {
		getHoleFillingDisparity(imghgt, imgwdt, pblkval, average_disp, pblkcrst);
	}

	// ブロックの視差を画素へ展開する
	getDisparityImage(imghgt, imgwdt, pblkval, average_disp, pdspimg, ppxldsp, pblkdsp);

	// ブロック視差値を保存する
	if (saveBlockInfo == true) {
		writeBlockDisparity(imghgt, imgwdt, average_disp);
		saveBlockInfo = false;
	}

	return true;
}


/// <summary>
/// 視差エンコードデータからブロックの視差値とマスクデータを取得し画素へ展開する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pSrcImage">視差エンコードデータ(IN)</param>
/// <param name="pDistImage">視差画像 右下基点(OUT)</param>
/// <param name="pTempParallax">視差情報 右下基点(OUT)</param>
/// <param name="pBlockDepth">ブロック視差情報 右下基点(OUT)</param>
void ISCFrameDecoder::decodeDisparityDirect(int imghgt, int imgwdt,
	unsigned char* pSrcImage, unsigned char* pDispImage, float* pTempParallax, float* pBlockDepth)
{
	unsigned char* buff_mix = pSrcImage; // 視差情報
	unsigned char storeDisparity;
	float dbValue;
	int d_tmp;
	unsigned char mask1, mask2;
	int mask;
	int shift;

	// 画像の高さブロック数
	int imghgtblk = imghgt / DISPARITY_BLOCK_HEIGHT_FPGA;
	// 画像の幅ブロック数
	int imgwdtblk = imgwdt / DISPARITY_BLOCK_WIDTH_FPGA;

	float dsprt = (float)255 / MATCHING_DEPTH_FPGA; // 視差画像表示のため視差値を256階調へ変換する

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
			for (int jj = 0; jj < DISPARITY_BLOCK_HEIGHT_FPGA; jj++) {
				for (int ii = 0; ii < DISPARITY_BLOCK_WIDTH_FPGA; ii++) {
					unsigned char *pimg = pDispImage + imgwdt * (j + jj) + i + ii;
					float *pval = pTempParallax + imgwdt * (j + jj) + i + ii;

					if (mask & shift) {
						*pimg = storeDisparity;
						*pval = dbValue;

					}
					else {
						*pimg = 0;
						*pval = 0.0;
					}
					shift = shift << 0x01;
				}
			}
		}
	}

	return;
}

/// <summary>
/// 視差エンコードデータからブロックの視差値とマスクデータを取得する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="prgtimg">右（基準）画像データ(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="bgtmax">ブロック輝度最大値(IN)</param>
/// <param name="pdspenc">視差エンコードデータ(IN)<</param>
/// <param name="pblkval">ブロック視差値(1000倍サブピクセル精度の整数)(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void ISCFrameDecoder::getDisparityData(int imghgt, int imgwdt,
	unsigned char* prgtimg, int crstthr, int crstofs, int bgtmax,
	unsigned char* pdspenc, int* pblkval, int* pblkcrst)
{
	int i, j, ii, jj;

	unsigned char* buff_mix = pdspenc; // 視差情報

	int storeDisparity;
	float dbValue;
	int d_tmp;
	int wdt = imgwdt / DISPARITY_BLOCK_WIDTH_FPGA;

	for (j = 0, jj = 0; j < imghgt; j += DISPARITY_BLOCK_HEIGHT_FPGA, jj++) {
		for (i = 0, ii = 0; i < imgwdt; i += DISPARITY_BLOCK_WIDTH_FPGA, ii++) {

			int sumr = 0; // Σ基準画像の輝度ij
			int Lmin = 255;
			int Lmax = 0;
			int blkcnt = 0;

			for (int jpxl = j; jpxl < j + DISPARITY_BLOCK_HEIGHT_FPGA; jpxl++) {
				for (int ipxl = i; ipxl < i + DISPARITY_BLOCK_WIDTH_FPGA; ipxl++) {
					int Lpxl = prgtimg[jpxl * imgwdt + ipxl];

					sumr += Lpxl;

					if (Lpxl < Lmin) {
						Lmin = Lpxl;
					}
					else if (Lpxl > Lmax) {
						Lmax = Lpxl;
					}
					blkcnt++;

				}
			}
			// コントラストを求める
			int crst = 0;
			if (Lmax >= bgtmax) {
				crst = ((Lmax - Lmin) * 1000 - crstofs) * blkcnt / sumr;
			}

			// 視差整数部
			storeDisparity = *(buff_mix + j * imgwdt + i);

			// 視差小数部（4ビット幅）
			d_tmp = ((*(buff_mix + j * imgwdt + i + 1)) & 0xF0) >> 4;

			// 視差値（浮動小数）
			dbValue = storeDisparity + (float)(d_tmp * FPGA_PARALLAX_VALUE);

			// 倍精度整数サブピクセルブロック視差値
			int parallax = (int)(dbValue * MATCHING_SUBPIXEL_TIMES);

			int *p = pblkval + jj * wdt + ii;
			int *pcnt = pblkcrst + jj * wdt + ii;

			// コントラスト閾値を判定する
			if (crst < crstthr) {
				*p = 0;
			}
			else {
				*p = parallax;
			}

			// コントラストを保存する
			*pcnt = crst;

		}
	}
}


/// <summary>
/// 視差値を平均化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
void ISCFrameDecoder::getAveragingDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp)
{
	if (numOfBands < 2) {
		getWholeAveragingDisparity(imghgt, imgwdt, pblkval, pavedsp);
	}
	else {
		getBandAveragingDisparity(imghgt, imgwdt, pblkval, pavedsp);
	}

}


/// <summary>
/// OpenCVを使って視差値を平均化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
void ISCFrameDecoder::getAveragingDisparityOpenCV(int imghgt, int imgwdt, int* pblkval, float *pavedsp)
{

	// ブロックの高さ
	int pj = disparityBlockHeight;
	// ブロックの幅
	int pi = disparityBlockWidth;
	// 画像の高さブロック数
	int imghgtblk = imghgt / pj;
	// 画像の幅ブロック数
	int imgwdtblk = imgwdt / pi;
	// マッチング探索幅
	int depth = matchingDepth;
	// 遮蔽領域幅
	int shdwdt = shadeBandWidth;

	// 視差画像の幅ブロック数
	int dspwdtblk = (imgwdt - shdwdt) / pi;
	// 視差サブピクセル精度（倍率）
	int dspsubrt = MATCHING_SUBPIXEL_TIMES;

	// 平均化入出力視差データのMatを生成する
	cv::Mat inputDisp(imghgtblk, imgwdtblk, CV_32SC1, pblkval);
	cv::Mat outputAveDisp(imghgtblk, imgwdtblk, CV_32FC1, pavedsp);

	// 平均化入出力視差データのUMatを生成する
	cv::UMat inputUMatDisp(imghgtblk, imgwdtblk, CV_32SC1);
	cv::UMat outputUMatDisp(imghgtblk, imgwdtblk, CV_32FC1);

	// 入力視差データをUMatへコピーする
	inputDisp.copyTo(inputUMatDisp);

	// OpenCLで視差を平均化する 
	getAveragingDisparityOpenCL(imghgtblk, imgwdtblk, dspwdtblk, depth, dspsubrt, 
		inputUMatDisp, outputUMatDisp);

	// 出力視差データをMatへコピーする
	outputUMatDisp.copyTo(outputAveDisp);

	// 出力視差データを視差画像データへ変換する
	outputAveDisp.convertTo(inputDisp, CV_32SC1);

}


/// <summary>
/// 視差平均化カーネルプログラム
/// </summary>
/// <remarks>平均化最大ブロック数 AVERAGING_BLOCKS_MAX 289 (17x17)</remarks>
char* kernelAverageDisparity = (char*)"__kernel void kernelAverageDisparity(\n\
	int dspwdtblk, int depth, int dspsubrt,\n\
	int aveBlockHeight, int	aveBlockWidth,\n\
	int aveIntegRange, int aveLimitRange,\n\
	int aveReplacementRatio, int aveDispRatio, int aveValidRatio,\n\
	int cntwgt, int nrwgt, int rndwgt,\n\
	__global int* input, int input_step, int input_offset,\n\
	__global float* result, int result_step, int result_offset,\n\
	int height, int width)\n\
{\n\
	int x = get_global_id(0);\n\
	int y = get_global_id(1);\n\
	if (x >= width || y >= height) {\n\
		return; \n\
	}\n\
	int dspcnt = 0;\n\
	int dspblks[289];\n\
	int wgtblks[289];\n\
	int poswgt[9];\n\
	int dspwdt = depth * dspsubrt;\n\
	int dspitgrt = dspwdt / 1024 + 1;\n\
	int integ[1024] = { 0 };\n\
	int dspitgwdt = dspwdt / dspitgrt;\n\
	int idx = width * y + x;\n\
	if (x > dspwdtblk ||\n\
		y < aveBlockHeight || y >= height - aveBlockHeight ||\n\
		x < aveBlockWidth || x >= dspwdtblk - aveBlockWidth) {\n\
		result[idx] = 0.0;\n\
		return;\n\
	}\n\
	poswgt[0] = cntwgt;\n\
	poswgt[1] = nrwgt;\n\
	poswgt[2] = nrwgt;\n\
	poswgt[3] = rndwgt;\n\
	poswgt[4] = rndwgt;\n\
	poswgt[5] = rndwgt;\n\
	poswgt[6] = rndwgt;\n\
	poswgt[7] = rndwgt;\n\
	poswgt[8] = rndwgt;\n\
	int wgtttlcnt = 0;\n\
	int wgtdspcnt = 0;\n\
	int tgval = input[idx];\n\
	for (int j = y - aveBlockHeight, jj = (-1) * aveBlockHeight; j <= y + aveBlockHeight; j++, jj++) {\n\
		for (int i = x - aveBlockWidth, ii = (-1) * aveBlockWidth; i <= x + aveBlockWidth; i++, ii++) {\n\
			int disp = input[width * j + i];\n\
			int pos = jj * jj + ii * ii;\n\
			int wgt = 1;\n\
			if (pos < 9) {\n\
				wgt = poswgt[pos];\n\
			}\n\
			wgtttlcnt += wgt;\n\
			if (disp > dspsubrt) {\n\
				dspblks[dspcnt] = disp;\n\
				wgtblks[dspcnt] = wgt;\n\
				dspcnt++;\n\
				wgtdspcnt += wgt;\n\
				int stwi = (disp - aveIntegRange) / dspitgrt;\n\
				int endwi = (disp + aveIntegRange) / dspitgrt;\n\
				stwi = stwi < 0 ? 0 : stwi;\n\
				endwi = endwi >= dspitgwdt ? (dspitgwdt - 1) : endwi;\n\
				for (int k = stwi; k <= endwi; k++) {\n\
					integ[k] += wgt;\n\
				}\n\
			}\n\
		}\n\
	}\n\
	float density = (float)wgtdspcnt / wgtttlcnt * 100;\n\
	if (density < aveDispRatio) {\n\
		result[idx] = 0.0;\n\
		return;\n\
	}\n\
	int maxcnt = 0;\n\
	int maxdsp = 0;\n\
	int maxwnd = 0;\n\
	bool maxin = false;\n\
	for (int i = 0; i < dspitgwdt; i++) {\n\
		if (integ[i] > maxcnt) {\n\
			maxcnt = integ[i];\n\
			maxdsp = i;\n\
			maxwnd = 0;\n\
			maxin = true;\n\
		}\n\
		if (maxin == true) {\n\
			if (integ[i] == maxcnt) {\n\
				maxwnd++;\n\
			}\n\
			else {\n\
				maxin = false;\n\
			}\n\
		}\n\
	}\n\
	maxdsp += (maxwnd - 1) / 2;\n\
	int mode = maxdsp * dspitgrt;\n\
	int high = mode + aveLimitRange;\n\
	int low = mode - aveLimitRange;\n\
	high = high >= dspwdt ? (dspwdt - 1) : high;\n\
	low = low < 0 ? 0 : low;\n\
	int sum = 0;\n\
	int cnt = 0;\n\
	float ave = 0;\n\
	for (int i = 0; i < dspcnt; i++) {\n\
		if (dspblks[i] >= low && dspblks[i] <= high) {\n\
			sum = sum + dspblks[i] * wgtblks[i];\n\
			cnt += wgtblks[i];\n\
		}\n\
	}\n\
	if (cnt != 0) {\n\
		ave = (float)sum / cnt;\n\
	}\n\
	float reprt = (float)cnt / wgtttlcnt * 100;\n\
	if ((tgval < low || tgval > high) && reprt < aveReplacementRatio) {\n\
		result[idx] = 0.0;\n\
		return;\n\
	}\n\
	float ratio = (float)cnt / wgtdspcnt * 100;\n\
	if (ratio >= aveValidRatio) {\n\
		result[idx] = ave;\n\
	}\n\
	else {\n\
		result[idx] = 0.0;\n\
	}\n\
}";


/// <summary>
/// OpenCLコンテキストの初期化フラグ
/// </summary>
bool openCLContextInit = false;

/// <summary>
/// OpenCLコンテキスト
/// </summary>
cv::ocl::Context contextAveraging;

/// <summary>
/// カーネルプログラム
/// </summary>
cv::ocl::Program kernelProgramAveraging;

/// <summary>
/// カーネルオブジェクト
/// </summary>
cv::ocl::Kernel kernelObjectAveraging;

/// <summary>
/// glaobalWorkSize
/// </summary>
size_t globalSizeAveraging[2];


/// <summary>
/// 視差値を平均化する（OpenCLを呼び出す）
/// </summary>
/// <param name="imghgtblk">画像の高さブロック数(IN)</param>
/// <param name="imgwdtblk">画像の幅ブロック数(IN)</param>
/// <param name="dspwdtblk">画像の幅ブロック数(IN)</param>
/// <param name="depth">探索幅(IN)</param>
/// <param name="dspsubrt">視差サブピクセル精度（倍率）(IN)</param>
/// <param name="src">視差値データUMat(IN)</param>
/// <param name="dst">平均化視差値データUMat(OUT)</param>
void ISCFrameDecoder::getAveragingDisparityOpenCL(int imghgtblk, int imgwdtblk, int dspwdtblk,
	int depth, int dspsubrt, cv::UMat src, cv::UMat dst)
{

	bool success;

	// OpenCLのコンテキストを初期化する
	if (openCLContextInit == false) {

		// コンテキストを生成する
		// * コンテキストContextを保持する？ <<<<<<　Context　スタティック
		// clCreateContext
		// context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
		success = contextAveraging.create(cv::ocl::Device::TYPE_GPU);

		if (success == false) {
			OutputDebugString(_T("FALSE : context.create()\n"));
		}

		// デバイスを選択する
		// clGetDeviceIDs
		// clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
		cv::ocl::Device(contextAveraging.device(0));

		// カーネルソースを生成する
		// clCreateProgramWithSource
		// command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
		// program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
		cv::ocl::ProgramSource programSource(kernelAverageDisparity);

		cv::String errMsg;

		cv::String bldOpt = "";

		// カーネルプログラムをビルドする
		// * Programプログラムを保持する <<<<<<
		// clBuildProgram
		// ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
		kernelProgramAveraging = contextAveraging.getProg(programSource, bldOpt, errMsg);

		if (!errMsg.empty()) {
			cv::String msg;

			msg = "Compile Error has occurred.\n";
			msg += errMsg.c_str();
			msg += "\n";

			OutputDebugStringA(msg.c_str());

		}

		// カーネルオブジェクトを生成する
		// * Kernelカーネルオブジェクトを保持する <<<<<< new する
		// clCreateKernel
		// kernel = clCreateKernel(program, "matrix_dot_matrix", &ret);
		kernelObjectAveraging.create("kernelAverageDisparity", kernelProgramAveraging);

		// OpenCL初期化フラグをセットする
		openCLContextInit = true;

	}

	// カーネルの引数を設定する
	// Kernel& cv::ocl::Kernel::args(const _Tp0 & a0, const _Tp1 & a1)
	//
	// clSetKernelArg
	//  /* Create Memory Buffer */
	//  matrixAMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, matrixAMemSize, matrixA, &ret);
	//  matrixBMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, matrixBMemSize, matrixB, &ret);
	//  matrixRMemObj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, matrixRMemSize, NULL, &ret);
	//  /* Set OpenCL Kernel Parameters */
	//  ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&matrixAMemObj);
	//  ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&matrixBMemObj);
	//  ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&matrixRMemObj);
	//  ret |= clSetKernelArg(kernel, 3, sizeof(int), (void *)&wA);
	//  ret |= clSetKernelArg(kernel, 4, sizeof(int), (void *)&wB);
	//
	kernelObjectAveraging.args(
		dspwdtblk, // 1:画像の幅ブロック数(IN)
		depth, // 2:探索幅(IN)
		dspsubrt, // 3:視差サブピクセル精度（倍率）
		dispAveBlockHeight, // 4:視差平均化ブロック高さ
		dispAveBlockWidth, // 5:視差平均化ブロック幅
		dispAveIntegRange, // 6:視差平均化移動平均幅
		dispAveLimitRange, // 7:視差平均化分布範囲最大幅
		dispAveReplacementRatio, // 8:視差平均化置換有効視差率
		dispAveDispRatio, // 9:視差平均化有効視差含有率
		dispAveValidRatio, // 10:視差平均化有効視差比率
		dispAveBlockWeightCenter, // 11:視差平均化ブロックの重み（中央）
		dispAveBlockWeightNear, // 12:視差平均化ブロックの重み（近傍）
		dispAveBlockWeightRound, // 13:視差平均化ブロックの重み（周辺）
		cv::ocl::KernelArg::ReadOnlyNoSize(src), cv::ocl::KernelArg::ReadWrite(dst));

	// globalWorkSize : 行いたい処理の総数
	// localWorkSize : 並列に行いたい処理の数
	globalSizeAveraging[0] = (size_t)src.cols;
	globalSizeAveraging[1] = (size_t)src.rows;

	// カーネルを実行する
	// bool cv::ocl::Kernel::run(int dims, size_t globalsize[], size_t localsize[], bool sync, const Queue & q = Queue())
	//
	// clEnqueueTask
	// ret = clEnqueueNDRangeKernel(command_queue, kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	// ret = clEnqueueReadBuffer(command_queue, matrixRMemObj, CL_TRUE, 0, matrixRMemSize, matrixR, 0, NULL, NULL)
	// 
	success = kernelObjectAveraging.run(2, globalSizeAveraging, NULL, true);

	if (success == false) {
		OutputDebugString(_T("FALSE : kernel.run()\n"));
	}

}


/// <summary>
/// 視差値を平均化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
void ISCFrameDecoder::getWholeAveragingDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp)
{

	int pj = disparityBlockHeight;
	int pi = disparityBlockWidth;

	// 遮蔽領域幅
	int shdwdt = shadeBandWidth;

	// サブピクセル精度の視差値をwrkにコピーする
	memcpy(wrk, pblkval, imghgt * imgwdt * sizeof(int));

	// 画像の高さブロック数
	int imghgtblk = imghgt / pj;
	// 画像の幅ブロック数
	int imgwdtblk = imgwdt / pi;
	// 画像の幅ブロック数
	int dspwdtblk = (imgwdt - shdwdt) / pi;

	// 平均化する
	getAveragingDisparityInBand(imghgtblk, imgwdtblk, dspwdtblk,
		pblkval, pavedsp, 0, imghgtblk);

}


/// <summary>
/// 視差値を平均化する
/// </summary>
/// <param name="imghgtblk">画像のブロック高さ(IN)</param>
/// <param name="imgwdtblk">画像のブロック幅(IN)</param>
/// <param name="dspwdtblk">有効画像のブロック幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(OUT)</param>
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
void ISCFrameDecoder::getAveragingDisparityInBand(int imghgtblk, int imgwdtblk, int dspwdtblk,
	int* pblkval, float *pavedsp, int jstart, int jend)
{
	// jd : マッチングブロックのyインデックス
	// id : マッチングブロックのxインデックス
	int id, jd, i, j, ii, jj;

	// マッチング探索幅
	int depth = matchingDepth;

	// 視差検出ブロック配列
	// 最大16x16ブロック
	int dspblks[AVERAGING_BLOCKS_MAX];

	// ブロック位置重み
	// 最大16x16ブロック配列
	int wgtblks[AVERAGING_BLOCKS_MAX];
	// ブロック位置から重みへ変換するテーブル
	int poswgt[9];
	// ブロック位置から重み変換の初期化
	// 視差平均化ブロックの重み（中央）
	poswgt[0] = dispAveBlockWeightCenter;
	// 視差平均化ブロックの重み（近傍）
	poswgt[1] = dispAveBlockWeightNear;
	poswgt[2] = dispAveBlockWeightNear;
	// 視差平均化ブロックの重み（周辺）
	poswgt[3] = dispAveBlockWeightRound;
	poswgt[4] = dispAveBlockWeightRound;
	poswgt[5] = dispAveBlockWeightRound;
	poswgt[6] = dispAveBlockWeightRound;
	poswgt[7] = dispAveBlockWeightRound;
	poswgt[8] = dispAveBlockWeightRound;

	// 探索幅の倍精度整数
	int dspwdt = depth * MATCHING_SUBPIXEL_TIMES;
	// 視差サブピクセル精度倍率
	int dspitgrt = dspwdt / 1024 + 1;
	// 移動積分視差サブピクセル幅
	int dspitgwdt = dspwdt / dspitgrt;

	for (jd = jstart; jd < jend; jd++) {
		for (id = 0; id < dspwdtblk; id++) {

			// 視差検出ブロック数
			int dspcnt = 0;
			// 移動積分ヒストグラム
			int integ[1024] = { 0 };

			// 着目ブロックのインデックス
			int idx = imgwdtblk * jd + id;

			// 平均化対象外の周辺部分は視差なしにする
			if (jd < dispAveBlockHeight || jd >= imghgtblk - dispAveBlockHeight ||
				id < dispAveBlockWidth || id >= dspwdtblk - dispAveBlockWidth) {
				pblkval[idx] = 0;
				pavedsp[idx] = 0.0;
				continue;
			}

			// ブロック位置重みの総数
			int wgtttlcnt = 0;
			// ブロック位置重み数
			int wgtdspcnt = 0;

			// 着目ブロックの視差値を求める
			int tgval = wrk[imgwdtblk * jd + id];

			// 平均対象領域のヒストグラムを作成
			for (j = jd - dispAveBlockHeight, jj = (-1) * dispAveBlockHeight; j <= jd + dispAveBlockHeight; j++, jj++) {
				for (i = id - dispAveBlockWidth, ii = (-1) * dispAveBlockWidth; i <= id + dispAveBlockWidth; i++, ii++) {
					int disp = wrk[imgwdtblk * j + i];

					// ブロック位置重み
					int pos = jj * jj + ii * ii;
					int wgt = 1;
					if (pos < 9) {
						wgt = poswgt[pos];
					}
					wgtttlcnt += wgt;

					if (disp > MATCHING_SUBPIXEL_TIMES) {
						// ブロックの視差値を保存する
						dspblks[dspcnt] = disp;
						// ブロック位置重みを保存する
						wgtblks[dspcnt] = wgt;
						// 視差検出ブロック数をカウントする
						dspcnt++;
						// ブロック位置重みをカウントする
						wgtdspcnt += wgt;

						// 倍精度整数サブピクセルの精度をヒストグラム幅に合わせて落とす
						// 移動積分を求める
						int stwi = (disp - dispAveIntegRange) / dspitgrt;
						int endwi = (disp + dispAveIntegRange) / dspitgrt;
						stwi = stwi < 0 ? 0 : stwi;
						endwi = endwi >= dspitgwdt ? (dspitgwdt - 1) : endwi;
						for (int k = stwi; k <= endwi; k++) {
							integ[k] += wgt;
						}
					}
				}
			}

			// 視差平均化有効比率をチェックする
			// 視差含有率
			float density = (float)wgtdspcnt / wgtttlcnt * 100;

			if (density < dispAveDispRatio) {
				pblkval[idx] = 0;
				pavedsp[idx] = 0.0;
				continue;
			}

			// 最頻値を求める
			int maxcnt = 0;
			int maxdsp = 0;
			// 最頻値が連続する場合はその中央値を最頻値とする
			int maxwnd = 0;
			bool maxin = false;
			for (i = 0; i < dspitgwdt; i++) {
				if (integ[i] > maxcnt) {
					maxcnt = integ[i];
					maxdsp = i;
					maxwnd = 0;
					maxin = true;
				}
				// 最頻値の連続幅を求める
				if (maxin == true) {
					if (integ[i] == maxcnt) {
						maxwnd++;
					}
					else {
						maxin = false;
					}
				}
			}
			maxdsp += (maxwnd - 1) / 2;

			// 平均計算の対象を範囲を求める
			// 最頻値のサブピクセル精度を倍精度整数へ戻す
			int mode = maxdsp * dspitgrt;

			int high = mode + dispAveLimitRange;
			int low = mode - dispAveLimitRange;

			high = high >= dspwdt ? (dspwdt - 1) : high;
			low = low < 0 ? 0 : low;

			// 範囲内の平均を求める
			int sum = 0;
			int cnt = 0;
			float ave = 0;

			for (i = 0; i < dspcnt; i++) {
				if (dspblks[i] >= low && dspblks[i] <= high) {
					sum = sum + dspblks[i] * wgtblks[i];
					cnt += wgtblks[i];
				}
			}
			if (cnt != 0) {
				ave = (float)sum / cnt;
			}

			// 着目ブロックの視差値が分布境界幅に入っているかチェックする

			// 分布境界幅に入っていない、かつ有効視差率が置換レベルにいない場合
			float reprt = (float)cnt / wgtttlcnt * 100;

			if ((tgval < low || tgval > high) && reprt < dispAveReplacementRatio) {
				pblkval[idx] = 0;
				pavedsp[idx] = 0.0;
				continue;
			}

			// 有効視差率
			float ratio = (float)cnt / wgtdspcnt * 100;

			if (ratio >= dispAveValidRatio) {
				pblkval[idx] = (int)ave;
				pavedsp[idx] = ave;
			}
			else {
				pblkval[idx] = 0;
				pavedsp[idx] = 0.0;
			}
		}
	}

}


/// <summary>
/// ブロックの視差を画素へ展開する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN)</param>
/// <param name="pDestImage">視差画像 右下基点(OUT)</param>
/// <param name="pTempParallax">視差情報 右下基点(OUT)</param>
/// <param name="pBlockDepth">ブロック視差情報 右下基点(OUT)</param>
void ISCFrameDecoder::getDisparityImage(int imghgt, int imgwdt,
	int* pblkval, float *pavedsp, unsigned char* pDestImage, float* pTempParallax, float* pBlockDepth)
{

	memset(pDestImage, 0x00, imghgt * imgwdt * sizeof(unsigned char));
	memset(pTempParallax, 0x00, imghgt * imgwdt * sizeof(float));

	// 出力視差ブロック画像の高さ
	int imghgtblk = imghgt / disparityBlockHeight;
	int dsphgtblk = (imghgt - matchingBlockHeight - dispBlockOffsetY) / disparityBlockHeight + 1;
	// 出力視差ブロック画像の幅
	int imgwdtblk = imgwdt / disparityBlockWidth;
	int dspwdtblk = (imgwdt - shadeBandWidth - matchingBlockWidth - dispBlockOffsetX) / disparityBlockWidth + 1;
	// 視差画像表示のため視差値を256階調へ変換する
	float dsprt = (float)255 / matchingDepth;

	// jblk : 視差ブロックのyインデックス
	// iblk : 視差ブロックのxインデックス
	for (int jblk = 0; jblk < dsphgtblk; jblk++) {
		for (int iblk = 0; iblk < dspwdtblk; iblk++) {

			int dsp = pblkval[imgwdtblk * jblk + iblk];
			float ave = pavedsp[imgwdtblk * jblk + iblk];

			// 視差値の範囲を制限する
			// 範囲を超えた場合は視差なしにする
			// 補完で埋め戻す
			if (dispLimitation == 1 &&
				(dsp < dispLowerLimit || dsp > dispUpperLimit)) {
				dsp = 0;
				ave = 0.0;
			}

			dsp = dsp / MATCHING_SUBPIXEL_TIMES;
			ave = ave / MATCHING_SUBPIXEL_TIMES;

			// ブロック視差情報を保存する
			pBlockDepth[imgwdtblk * jblk + iblk] = ave;

			// jpxl : 視差ブロックのy座標
			// ipxl : 視差ブロックのx座標
			int jpxl = jblk * disparityBlockHeight + dispBlockOffsetY;
			int ipxl = iblk * disparityBlockWidth + dispBlockOffsetX;

			// 視差をブロックから画素へ展開する
			for (int j = jpxl; j < jpxl + disparityBlockHeight; j++) {
				for (int i = ipxl; i < ipxl + disparityBlockWidth; i++) {
					pDestImage[imgwdt * j + i] = (unsigned char)(ave * dsprt);
					pTempParallax[imgwdt * j + i] = ave;
				}
			}
		}
	}

}


/// <summary>
/// 視差を補完する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void ISCFrameDecoder::getComplementDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp, int* pblkcrst)
{
	// 視差を補完する
	getVerticalComplementDisparity(imghgt, imgwdt, false, pblkval, pavedsp, pblkcrst);
	getHorizontalComplementDisparity(imghgt, imgwdt, false, pblkval, pavedsp, pblkcrst);
	getDiagonalUpComplementDisparity(imghgt, imgwdt, false, pblkval, pavedsp, pblkcrst);
	getDiagonalDownComplementDisparity(imghgt, imgwdt, false, pblkval, pavedsp, pblkcrst);

}


/// <summary>
/// 視差を穴埋めする
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void ISCFrameDecoder::getHoleFillingDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp, int* pblkcrst)
{
	// 視差補完領域の穴埋めをする
	getHorizontalComplementDisparity(imghgt, imgwdt, true, pblkval, pavedsp, pblkcrst);
	getVerticalComplementDisparity(imghgt, imgwdt, true, pblkval, pavedsp, pblkcrst);
	getDiagonalUpComplementDisparity(imghgt, imgwdt, true, pblkval, pavedsp, pblkcrst);
	getDiagonalDownComplementDisparity(imghgt, imgwdt, true, pblkval, pavedsp, pblkcrst);
	getHorizontalComplementDisparity(imghgt, imgwdt, true, pblkval, pavedsp, pblkcrst);
	getVerticalComplementDisparity(imghgt, imgwdt, true, pblkval, pavedsp, pblkcrst);

}


/// <summary>
/// 視差なしを補完する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="holefill">補完領域の穴埋め(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void ISCFrameDecoder::getHorizontalComplementDisparity(int imghgt, int imgwdt, bool holefill,
	int* pblkval, float *pavedsp, int* pblkcrst)
{
	int id, jd, ie, je, pj, pi;

	pj = disparityBlockHeight;
	pi = disparityBlockWidth;

	je = (imghgt - matchingBlockWidth) / pj + 1;
	ie = (imgwdt - shadeBandWidth - matchingBlockWidth) / pi + 1;

	int imgblkwdt = imgwdt / pi;

	int vrtOfs = dispAveBlockHeight;
	int hztOfs = dispAveBlockWidth;

	// 右下基点画像　
	// 補完走査の先頭は右端
	for (jd = vrtOfs; jd < je - vrtOfs; jd++) {
		// ブロック前方走査
		// 右下基点画像 右から左
		for (id = hztOfs; id < ie - hztOfs; id++) {
			complementForward(imgblkwdt, id, hztOfs, jd, id, pblkval, pavedsp);
		}
		// ブロック後方走査
		// 右下基点画像 左から右　
		int stid = id - 1;
		for (id = stid; id >= hztOfs; id--) {
			complementBackward(imgblkwdt, id, stid, jd, id, pblkval, pavedsp, pblkcrst, holefill,
				disparityBlockWidth,
				dispComplementRatioInside, dispComplementRatioRound, dispComplementRatioRound);
		}
	}

}


/// <summary>
/// 垂直走査で視差なしを補完する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="holefill">補完領域の穴埋め(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void ISCFrameDecoder::getVerticalComplementDisparity(int imghgt, int imgwdt, bool holefill, 
	int* pblkval, float *pavedsp, int* pblkcrst)
{
	int id, jd, ie, je, pj, pi;

	pj = disparityBlockHeight;
	pi = disparityBlockWidth;

	je = (imghgt - matchingBlockWidth) / pj + 1;
	ie = (imgwdt - shadeBandWidth - matchingBlockWidth) / pi + 1;

	int imgblkwdt = imgwdt / pi;

	int vrtOfs = dispAveBlockHeight;
	int hztOfs = dispAveBlockWidth;

	// 右下基点画像　
	// 補完走査の先頭は下端
	for (id = hztOfs; id < ie - hztOfs; id++) {
		// ブロック下方走査
		// 右下基点画像 下から上
		for (jd = vrtOfs; jd < je - vrtOfs; jd++) {
			complementForward(imgblkwdt, jd, vrtOfs, jd, id, pblkval, pavedsp);
		}
		// ブロック上方走査
		// 右下基点画像 上から下
		int stjd = jd - 1;
		for (jd = stjd; jd >= vrtOfs; jd--) {
			complementBackward(imgblkwdt, jd, stjd, jd, id, pblkval, pavedsp, pblkcrst, holefill,
				disparityBlockHeight,
				dispComplementRatioInside, dispComplementRatioBottom, dispComplementRatioRound);
		}
	}
}


/// <summary>
/// 対角下向き走査で視差なしを補完する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="holefill">補完領域の穴埋め(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void ISCFrameDecoder::getDiagonalDownComplementDisparity(int imghgt, int imgwdt, bool holefill, 
	int* pblkval, float *pavedsp, int* pblkcrst)
{
	int id, jd, ie, je, pj, pi;
	int ij, idd, jdd;
	int drct;

	pj = disparityBlockHeight;
	pi = disparityBlockWidth;

	je = (imghgt - matchingBlockWidth) / pj + 1;
	ie = (imgwdt - shadeBandWidth - matchingBlockWidth) / pi + 1;

	int imgblkwdt = imgwdt / pi;

	int vrtOfs = dispAveBlockHeight;
	int hztOfs = dispAveBlockWidth;

	drct = 0;
	idd = hztOfs - 1;

	// 走査の先頭の補完倍率
	double headrt = dispComplementRatioBottom;

	// 右下原点画像
	// 右下から左上へ斜め方向の補完
	// 補完走査の先頭は右下端
	// 先ず走査の先頭を右下端から左へ　走査の先頭は常に下端
	// 次に走査の先頭を右下端から上へ　走査の先頭は常に右端
	for (ij = 0; ij < (je + ie); ij++) {
		// 対角走査開始位置を求める
		// 1行目右方向
		if (drct == 0) {
			jdd = vrtOfs;
			idd++;
			if (idd > (ie - (hztOfs + 1))) {
				drct = 1;
			}
		}
		// 1列目下方向
		else {
			jdd++;
			if (jdd > (je - (vrtOfs + 1))) {
				break;
			}
			idd = hztOfs;
			headrt = dispComplementRatioRound;
		}
		// ブロック斜め右下走査
		// 右下原点画像 右下から左上
		for (jd = jdd, id = idd; (jd < (je - vrtOfs)) && (id < (ie - hztOfs)); jd++, id++) {
			complementForward(imgblkwdt, id, idd, jd, id, pblkval, pavedsp);
		}
		// ブロック斜め左上走査
		// 右下原点画像 左上から右下
		int stid = id - 1;
		for (jd = jd - 1, id = stid; jd >= vrtOfs && id >= hztOfs; jd--, id--) {
			complementBackward(imgblkwdt, id, stid, jd, id, pblkval, pavedsp, pblkcrst, holefill,
				disparityBlockDiagonal,
				dispComplementRatioInside, headrt, dispComplementRatioRound);
		}
	}

}


/// <summary>
/// 対角上向き走査で視差なしを補完する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="holefill">補完領域の穴埋め(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void ISCFrameDecoder::getDiagonalUpComplementDisparity(int imghgt, int imgwdt, bool holefill,
	int* pblkval, float *pavedsp, int* pblkcrst)
{
	int id, jd, ie, je, pj, pi;

	int ij, idd, jdd;
	int drct;

	pj = disparityBlockHeight;
	pi = disparityBlockWidth;

	je = (imghgt - matchingBlockWidth) / pj + 1;
	ie = (imgwdt - shadeBandWidth - matchingBlockWidth) / pi + 1;

	int imgblkwdt = imgwdt / pi;

	int vrtOfs = dispAveBlockHeight;
	int hztOfs = dispAveBlockWidth;

	drct = 0;

	// i開始位置
	idd = hztOfs - 1;

	// 走査の先頭の補完倍率
	double headrt = dispComplementRatioBottom;


	// 右下原点画像　
	// 左下から右上へ斜め方向の補完
	// 補完走査の開始位置は右下端
	// 先ず走査の先頭を右下端から左へ　走査の先頭は常に下端
	// 補完走査の開始位置は左下端へ移して
	// 次に走査の先頭を左下端から上へ　走査の先頭は常に左端
	for (ij = 0; ij < (je + ie); ij++) {
		// 対角走査開始位置を求める
		// 1行目左方向（右下原点画像）
		if (drct == 0) {
			jdd = vrtOfs; // 行固定
			idd++;
			if (idd > (ie - (hztOfs + 1))) {
				drct = 1;
			}
		}
		// 最終列目上方向（右下原点画像）
		else {
			jdd++;
			if (jdd > (je - (vrtOfs + 1))) {
				break;
			}

			idd = ie - (hztOfs + 1); //最終列固定;
			// 補完倍率を切り替える
			headrt = dispComplementRatioRound;
		}
		// ブロック斜め右上走査
		// 右下基点画像 右上から左下
		for (jd = jdd, id = idd; (jd < (je - vrtOfs)) && (id >= hztOfs); jd++, id--) {
			complementForward(imgblkwdt, jd, jdd, jd, id, pblkval, pavedsp);
		}
		// ブロック斜め左下走査
		// 右下基点画像 左下から右上
		int stjd = jd - 1;
		for (jd = stjd, id = id + 1; (jd >= vrtOfs) && (id < (ie - hztOfs)); jd--, id++) {
			complementBackward(imgblkwdt, jd, stjd, jd, id, pblkval, pavedsp, pblkcrst, holefill,
				disparityBlockDiagonal,
				dispComplementRatioInside, headrt, dispComplementRatioBottom);
		}
	}

}


/// <summary>
/// 視差ブロック配列の昇順走査で注目ブロックを補完する
/// </summary>
/// <param name="imgblkwdt">画像のブロック幅(IN)</param>
/// <param name="ii">補完ブロック配列の走査インデックス(IN)</param>
/// <param name="sti">補完ブロック配列走査の開始インデックス(IN)</param>
/// <param name="jd">注目ブロックのyインデックス(IN)</param>
/// <param name="id">注目ブロックのxインデックス(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN)</param>
void ISCFrameDecoder::complementForward(int imgblkwdt, int ii, int sti, int jd, int id,
	int* pblkval, float *pavedsp)
{

	// 
	// 視差補完
	// 視差なしを前後の有効な視差を使って補完する
	// ブロックの視差値の配列を前方へ走査し補完値を重みを付けて保存する
	// 次に、後方へ走査して重みから補完値を決定しる
	// 両端のブロックは参照しない
	// 
	//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// | |0|0|x|0|0|y|y|0|z|0|0|0|0|a|0|0|0|0|0| |
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
	//    +-------> 昇順走査
	// wgtcmp 
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// | |1|2|0|1|2|0|0|1|0|1|2|3|4|0|1|2|3|4|5| |
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// blkcmp 
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// | |0|0|X|X|X|Y|Y|Y|Z|Z|Z|Z|Z|A|A|A|A|A|A| |
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	//

	// 重み（平均計算用）
	wgtcmp[ii] = 0;
	// 視差値（補完値計算用）
	blkcmp[ii] = *(pblkval + imgblkwdt * jd + id);
	avecmp[ii] = *(pavedsp + imgblkwdt * jd + id);

	if (ii != sti) {
		// 視差値ゼロ（視差なし）を補完する
		if (blkcmp[ii] == 0) {
			if (blkcmp[ii - 1] > 0) {
				// 後方の重みをインクリメントしてセットする
				// 補完視差値を継承する
				wgtcmp[ii] = wgtcmp[ii - 1] + 1;
				blkcmp[ii] = blkcmp[ii - 1];
				avecmp[ii] = avecmp[ii - 1];
			}
			// 先頭から補完する場合
			// 補完視差値はゼロのまま
			// 後方の重みをインクリメントしてセットする
			else {
				if (wgtcmp[ii - 1] > 0) {
					wgtcmp[ii] = wgtcmp[ii - 1] + 1;
				}
			}
		}
	}
	else {
		// 先頭の視差値がゼロの場合
		if (blkcmp[ii] == 0) {
			// 重みを1にセットする
			wgtcmp[ii] = 1;
		}
	}

}


/// <summary>
/// 視差ブロック配列の降順走査で注目ブロックを補完する
/// </summary>
/// <param name="imgblkwdt">画像のブロック幅(IN)</param>
/// <param name="ii">補完ブロック配列の走査インデックス(IN)</param>
/// <param name="sti">補完ブロック配列走査の開始インデックス(IN)</param>
/// <param name="jd">注目ブロックのyインデックス(IN)</param>
/// <param name="id">注目ブロックのxインデックス(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
/// <param name="holefill">補完領域の穴埋め(IN)</param>
/// <param name="blkwdt">補完ブロックの画素幅(IN)</param>
/// <param name="midprt">中央領域補完画素幅の視差値倍率(IN)</param>
/// <param name="toprt">先頭輪郭補完画素幅の視差値倍率(IN)</param>
/// <param name="btmrt">後端輪郭補完画素幅の視差値倍率(IN)</param>
void ISCFrameDecoder::complementBackward(int imgblkwdt, int ii, int sti, int jd, int id,
	int* pblkval, float *pavedsp, int *pblkcrst, bool holefill,
	double blkwdt, double midrt, double toprt, double btmrt)
{

	// 走査の開始インデックスの場合
	if (ii == sti) {
		// 終端の重みと補完視差値をセットする
		// 終端の重みは補完視差値に倍率を掛けた値にする
		// 終端（画像の端）が視差なしの場合の補完幅を調整する
		// 
		//  ... 0 1 2 3 4 5 6 7 8 9 0
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+
		// | |a|b|0|0|0|0|0|0|0|0|0| |
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+ 
		//       降順走査 <-------+ 
		// wgtcmp 
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+
		// | |0|0|1|2|3|4|5|6|7|8|9|x|
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+
		//  x = 補完視差値B * 調整倍率
		// blkcmp 
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+
		// | |A|B|B|B|B|B|B|B|B|B|B|B|
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+
		//
		if (wgtcmp[ii] > 0) {
			wgtcmp[ii + 1] = (int)(blkcmp[ii] / MATCHING_SUBPIXEL_TIMES / blkwdt * (2.0 * midrt - btmrt));
			blkcmp[ii + 1] = blkcmp[ii];
			avecmp[ii + 1] = avecmp[ii];
		}
	}

	// 前方の重みと補完視差値を取得する
	int wgttmp = wgtcmp[ii + 1];
	int blktmp = blkcmp[ii + 1];
	float avetmp = avecmp[ii + 1];


	// 先頭が補完視差値がゼロの場合を補完する
	// 先頭は重みだけセットされている
	// 後方の補完視差値を前方の補完視差値と同じにする
	if ((blkcmp[ii] == 0 && wgtcmp[ii] > 0)) {
		blkcmp[ii] = blktmp;
		avecmp[ii] = avetmp;

		// 先頭の重みは補完視差値に倍率を掛けた値にする
		// 先頭（画像の端）が視差なしの場合の補完幅を調整する
		// 
		//  0 1 2 3 4 5 6 7 8 9 0 ...
		// +-+-+-+-+-+-+-+-+-+-+-+-+-
		// | |0|0|0|0|0|0|0|0|0|a|b|
		// +-+-+-+-+-+-+-+-+-+-+-+-+-
		//   降順走査 <-------+ 
		// wgtcmp 
		// +-+-+-+-+-+-+-+-+-+-+-+-+-
		// | |1|2|3|4|5|6|7|8|9|0|0|
		// +-+-+-+-+-+-+-+-+-+-+-+-+-
		//                    x 
		//  x = 補完視差値A * 調整倍率
		// blkcmp 
		// +-+-+-+-+-+-+-+-+-+-+-+-+-
		// | |0|0|0|0|0|0|0|0|0|A|B| 
		// +-+-+-+-+-+-+-+-+-+-+-+-+-
		//
		if (wgttmp == 0) {
			wgttmp = (int)(blktmp / MATCHING_SUBPIXEL_TIMES / blkwdt * (2.0 * midrt - toprt));
		}
	}

	// 現在の補完視差値と重みがセットされている場合
	// 現在の補完視差値の重み計算をする
	if ((blkcmp[ii] > 0 && wgtcmp[ii] > 0)) {
		// 前方の重みをインクリメントする
		wgttmp = wgttmp + 1;

		// 弱パターンの場合に補完する
		if (holefill == true || pblkcrst[imgblkwdt * jd + id] <= dispComplementContrastLimit) {

			// 最小視差値以上を補完する
			if (blktmp >= (dispComplementLowLimit * MATCHING_SUBPIXEL_TIMES) &&
				blkcmp[ii] >= (dispComplementLowLimit * MATCHING_SUBPIXEL_TIMES)) {

				double rng = (wgttmp + wgtcmp[ii]) * blkwdt;
				double rngtmp = (double)blktmp * midrt / MATCHING_SUBPIXEL_TIMES;
				double rngcmp = (double)blkcmp[ii] * midrt / MATCHING_SUBPIXEL_TIMES;

				// 補完画素幅が視差値倍率以下を補完する
				// 補完幅が広過ぎる場合は除外する
				//
				// 視差なし連画素幅 < 後方視差値*倍率 + 前方視差値*倍率
				//
				if ((holefill == true && rng < (dispComplementHoleSize + blkwdt))
					|| (holefill == false && rng <= (rngtmp + rngcmp))) {

					// 補完画素幅の視差勾配が最大値未満を補完する
					// 補完幅が狭すぎる場合は除外する
					//
					// 視差勾配 : (後方視差値 - 前方視差値) / 視差なし連画素幅
					//
					int diff = abs(blktmp - blkcmp[ii]) / MATCHING_SUBPIXEL_TIMES;
					double slp = (double)(diff) / rng;

					if (slp < dispComplementSlopeLimit) {
						// 重み平均して補完視差値を求める
						// 前方重み : 前方視差ブロックから注目ブロックまでの距離
						// 後方重み : 後方視差ブロックから注目ブロックまでの距離
						// 
						// 重み平均 : (前方視差値 * 後方重み + 後方視差値 * 前方重み) / (後方重み + 前方重み)
						// 
						float dspbwd = avetmp;
						float dspfwd = avecmp[ii];

						float dspsubpix = (dspfwd * wgttmp + dspbwd * wgtcmp[ii]) / (wgttmp + wgtcmp[ii]);

						pblkval[imgblkwdt * jd + id] = (int)dspsubpix;
						pavedsp[imgblkwdt * jd + id] = dspsubpix;

					}
				}
			}

		}
		wgtcmp[ii] = wgttmp;
		blkcmp[ii] = blktmp;
		avecmp[ii] = avetmp;
	}

}


/// <summary>
/// ブロックの視差値を書き出す
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN)</param>
void ISCFrameDecoder::writeBlockDisparity(int imghgt, int imgwdt, float *pavedsp)
{
	int id, jd, ie, je, pj, pi;

	pj = disparityBlockHeight;
	pi = disparityBlockWidth;

	je = imghgt / pj;
	ie = imgwdt / pi;


	SYSTEMTIME st = {};
	wchar_t date_time_str[_MAX_PATH] = {};
	GetLocalTime(&st);

	// YYYYMMDD_HHMMSS
	swprintf_s(date_time_str, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	wchar_t file_name[_MAX_PATH] = {};
	swprintf_s(file_name, L"block_depth_%s.csv", date_time_str);

#define STR_BUFFER_SIZE 5000

	wchar_t* sbuf = (wchar_t*)malloc(STR_BUFFER_SIZE);
	int slen = 0;
	size_t len;
	wchar_t str[32];

	FILE* fp = nullptr;
	errno_t err = _wfopen_s(&fp, file_name, _T("wt+, ccs=UTF-8 "));

	if (err != 0) {
		return;
	}

	if (fp) {
	}
	else {
		return;
	}

	// x軸インデックスを書き出す
	for (id = 0; id < ie; id++) {
		swprintf_s(str, L",%d", id);
		len = wcslen(str);
		wcscpy_s(sbuf + slen, len + 1, str);
		slen += (int)len;
	}
	_ftprintf_s(fp, sbuf);

	// y軸インデックスと視差値を書き出す
	for (jd = 0; jd < je; jd++) {
		slen = 0;
		for (id = 0; id < ie; id++) {

			// 3Dグラフ表示のために左右反転する
			// 右下基点から左下基点へ
			float dsp = *(pavedsp + ie * (jd + 1) - id - 1) / MATCHING_SUBPIXEL_TIMES;

			if (id == 0) {
				swprintf_s(str, L"\n%d,%d", jd, (int)dsp);
				len = wcslen(str);
				wcscpy_s(sbuf + slen, len + 1, str);
				slen += (int)len;
			}
			else {
				swprintf_s(str, L",%d", (int)dsp);
				len = wcslen(str);
				wcscpy_s(sbuf + slen, len + 1, str);
				slen += (int)len;
			}
		}
		_ftprintf_s(fp, sbuf);
	}

	fclose(fp);

	free(sbuf);

	return;
}


/// <summary>
/// 視差平均化スレッドを生成する
/// </summary>
void ISCFrameDecoder::createAveragingThread()
{
	// バンド数が2以上の場合、スレッドを生成する
	if (numOfBands > 1) {
		// バンド数分のスレッドを生成する
		for (int i = 0; i < numOfBands; i++) {
			// イベントを生成する
			// 自動リセット非シグナル状態
			bandInfo[i].startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			bandInfo[i].stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			bandInfo[i].doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

			// スレッドを生成する
			bandInfo[i].bandThread = (HANDLE)_beginthreadex(0, 0, averagingBandThread, (LPVOID)&bandInfo[i], 0, 0);
		}
	}
}


/// <summary>
/// 視差平均化スレッドを破棄する
/// </summary>
void ISCFrameDecoder::deleteAveragingThread()
{
	// バンド数が2以上の場合
	if (numOfBands > 1) {
		for (int i = 0; i < numOfBands; i++) {
			// 停止イベントを送信する
			SetEvent(bandInfo[i].stopEvent);
			// 開始イベントを送信する
			SetEvent(bandInfo[i].startEvent);
			// 受信スレッドの終了を待つ
			WaitForSingleObject(bandInfo[i].bandThread, INFINITE);

			// スレッドオブジェクトを破棄する
			CloseHandle(bandInfo[i].bandThread);

			// イベントオブジェクトを破棄する
			CloseHandle(bandInfo[i].startEvent);
			CloseHandle(bandInfo[i].stopEvent);
			CloseHandle(bandInfo[i].doneEvent);
		}
	}
}


/// <summary>
/// 視差平均化スレッド
/// </summary>
/// <param name="parg">バンド情報(IN)</param>
/// <returns>処理結果を返す</returns>
UINT ISCFrameDecoder::averagingBandThread(LPVOID parg)
{
	BNAD_THREAD_INFO *pBand = (BNAD_THREAD_INFO *)parg;

	DWORD st;

	while (1) {
		// 実行開始イベントを待つ
		st = WaitForSingleObject(pBand->startEvent, INFINITE);
		// 停止イベントの通知をチェックする
		st = WaitForSingleObject(pBand->stopEvent, 0);

		// 停止イベントが通知された場合、スレッドを終了する
		if (st == WAIT_OBJECT_0) {
			break;
		}

		// 平均化する
		getAveragingDisparityInBand(pBand->imghgtblk, pBand->imgwdtblk, pBand->dspwdtblk,
			pBand->pblkval, pBand->pavedsp,
			pBand->bandStart, pBand->bandEnd);

		// 視差平均化完了イベントを通知する
		SetEvent(pBand->doneEvent);
	}
	return 0;
}


/// <summary>
/// 視差値を平均化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
void ISCFrameDecoder::getBandAveragingDisparity(int imghgt, int imgwdt, int* pblkval, float *pavedsp)
{

	int pj, pi, i, n;

	pj = disparityBlockHeight;
	pi = disparityBlockWidth;

	// 遮蔽領域幅
	int shdwdt = shadeBandWidth;

	// 画像の高さブロック数
	int imghgtblk = imghgt / pj;
	// 画像の幅ブロック数
	int imgwdtblk = imgwdt / pi;
	// 画像の幅ブロック数
	int dspwdtblk = (imgwdt - shdwdt) / pi;

	// 完了イベントの配列
	HANDLE doneevt[MAX_NUM_OF_BANDS];

	memcpy(wrk, pblkval, imghgt * imgwdt * sizeof(int));

	// バンドの高さブロック数
	int bndhgtblk = imghgtblk / numOfBands;

	for (i = 0, n = 0; i < numOfBands; i++, n += bndhgtblk) {
		// 画像の高さブロック数
		bandInfo[i].imghgtblk = imghgtblk;
		// 画像の幅ブロック数
		bandInfo[i].imgwdtblk = imgwdtblk;
		// 画像の幅ブロック数
		bandInfo[i].dspwdtblk = dspwdtblk;

		// バンド開始ブロック位置
		bandInfo[i].bandStart = n;
		// バンド終了ブロック位置
		bandInfo[i].bandEnd = n + bndhgtblk;

		// ブロック視差値(倍精度整数サブピクセル)
		bandInfo[i].pblkval = pblkval;
		// 平均化視差値データ
		bandInfo[i].pavedsp = pavedsp;

	}
	bandInfo[i - 1].bandEnd = imghgtblk;

	DWORD st;

	for (i = 0; i < numOfBands; i++) {
		// 開始イベントを送信する
		SetEvent(bandInfo[i].startEvent);
		// 完了イベントのハンドルを配列に格納する
		doneevt[i] = bandInfo[i].doneEvent;
	}

	// 全ての完了イベントを待つ
	st =  WaitForMultipleObjects(numOfBands, doneevt, TRUE, INFINITE);

	if (st == WAIT_OBJECT_0) {

	}

}

