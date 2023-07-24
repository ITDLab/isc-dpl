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
 * @file DisparityFilter.cpp
 * @brief Implementation of Disparity Filter
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 *
 * @details
 * - 視差の平均化、補完処理を行う
 *
*/

#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "DisparityFilter.h"

// サブピクセル倍率 (1000倍サブピクセル精度）
#define MATCHING_SUBPIXEL_TIMES 1000

// 平均化最大ブロック数 (17 x 17)
#define AVERAGING_BLOCKS_MAX 289

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

/// <summary>
/// 倍精度整数サブピクセル平均化視差値（ブロックごと）
/// </summary>
static float *average_disp;

/// <summary>
/// 視差平均化
/// </summary>

/// <summary>
/// 視差平均化処理にOpenCL使用　しない：0 する：1
/// </summary>
static int useOpenCLForAveDisp = 0;

/// <summary>
/// 特異点除去処理、視差平均化のための視差値コピー)
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

};

static BNAD_THREAD_INFO bandInfo[MAX_NUM_OF_BANDS];

/// <summary>
/// エッジ補完 0:しない 1:する
/// </summary>
static int edgeLineComplement = 1;

/// <summary>
/// エッジ検出 Canny閾値1
/// </summary>
static int edgeCannyThreshold1 = 50;

/// <summary>
/// エッジ検出 Canny閾値2
/// </summary>
static int edgeCannyThreshold2 = 100;

/// <summary>
/// ハフ変換 houghLinesP閾値　
/// </summary>
static int houghLinesPThreshold = 100;

/// <summary>
/// ハフ変換 houghLinesP 最小の線分長
/// </summary>
static int houghLinesPMinLength = 80;

/// <summary>
/// ハフ変換 houghLinesP 最大のギャップ長
/// </summary>
static int houghLinesPMaxGap = 5;

/// <summary>
/// エッジ線分上の最小視差ブロック数
/// </summary>
static int edgeLineMinBlocks = 20;

/// <summary>
/// エッジ視差の最小線形性指数（回帰線の決定係数）
/// </summary>
static double edgeLineMinLinearity = 20.0;

/// <summary>
/// エッジ線の視差ブロックの補完幅
/// </summary>
static int edgeLineComplementWidth = 1;

/// <summary>
/// 視差ブロックの補完幅の上限
/// </summary>
static int edgeComplementWidthUpper = 0;

/// <summary>
/// 視差ブロックの補完幅の下限
/// </summary>
static int edgeComplementWidthLower = 0;


// エッジ線の最大数
#define MaxLines 300

/// <summary>
/// エッジ線の始点終点座標
/// </summary>
static int LineSegments[MaxLines][4];

// エッジ線の最大視差ブロック数
#define MaxLineLength 1280

/// <summary>
/// エッジ線上の視差ブロック位置
/// </summary>
static int lineBlockPoints[MaxLineLength][2];

/// <summary>
/// エッジ線上の視差ブロックの視差値
/// </summary>
static int lineBlockValues[MaxLineLength];

/// <summary>
/// エッジ線上の視差ブロックの重み
/// </summary>
static int lineBlockWeight[MaxLineLength];

/// <summary>
/// エッジ線上の視差ブロックの補完値
/// </summary>
static int lineBlockComplement[MaxLineLength];

/// <summary>
/// エッジ線上の視差平均化移動積分幅（片側）
/// </summary>
static int lineAveIntegRange = 1 * MATCHING_SUBPIXEL_TIMES;


/// <summary>
/// 視差フィルターを初期化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
void DisparityFilter::initialize(int imghgt, int imgwdt)
{
	// 倍精度整数サブピクセル平均化視差値（ブロックごと）
	average_disp = (float *)malloc(imghgt * imgwdt * sizeof(float));
	// 特異点除去処理、視差平均化のための視差値コピー)
	wrk = (int *)malloc(imghgt * imgwdt * sizeof(int));
	// 視差補完処理のための視差値c
	blkcmp = (int *)malloc(imgwdt * sizeof(int));
	// 視差補完処理のための重み
	wgtcmp = (int *)malloc(imgwdt * sizeof(int));


}


/// <summary>
/// 視差フィルターを終了する
/// </summary>
void DisparityFilter::finalize()
{

	// 倍精度整数サブピクセル平均化視差値（ブロックごと）
	free(average_disp);
	// 特異点除去処理、視差平均化のための視差値(block_disp[]からコピー)
	free(wrk);
	// 視差補完処理のための視差値c
	free(blkcmp);
	// 視差補完処理のための重み
	free(wgtcmp);


}


/// <summary>
/// 視差平均化処理にOpenCLの使用を設定する
/// </summary>
/// <param name="usecl"></param>
void DisparityFilter::setUseOpenCLForAveragingDisparity(int usecl)
{
	useOpenCLForAveDisp = usecl;
}


/// <summary>
/// ブロックの視差値を保存する
/// </summary>
void DisparityFilter::saveBlockDisparity()
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
void DisparityFilter::setDisparityLimitation(int limit, double lower, double upper)
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
void DisparityFilter::setAveragingParameter(int enb, int blkshgt, int blkswdt,
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
void DisparityFilter::setAveragingBlockWeight(int cntwgt, int nrwgt, int rndwgt)
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
void DisparityFilter::setComplementParameter(int enb, double lowlmt, double slplmt, 
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
/// エッジ補完パラメータを設定する
/// </summary>
/// <param name="edgcmp">エッジ補完 0:しない 1:する(IN)</param>
/// <param name="minblks">エッジ線分上の最小視差ブロック数(IN)</param>
/// <param name="mincoef">エッジ視差の最小線形性指数（回帰線の決定係数）(IN)</param>
/// <param name="cmpwdt">エッジ線の補完視差ブロック幅(IN)</param>
void DisparityFilter::setEdgeComplementParameter(int edgcmp, int minblks, double mincoef, int cmpwdt)
{
	// エッジ補完 0:しない 1:する
	edgeLineComplement = edgcmp;

	// エッジ線分上の最小視差ブロック数
	edgeLineMinBlocks = minblks;
	// エッジ視差の最小線形性指数（回帰線の決定係数）
	edgeLineMinLinearity = mincoef;
	// 視差ブロックの補完幅
	edgeLineComplementWidth = cmpwdt;

	// 視差ブロックの補完幅の上限、下限
	int lnwdt = edgeLineComplementWidth - 1;
	if (lnwdt > 0) {
		edgeComplementWidthUpper = lnwdt / 2;
		edgeComplementWidthLower = (-1) * (lnwdt % 2 + edgeComplementWidthUpper);
	}
	else {
		edgeComplementWidthUpper = 0;
		edgeComplementWidthLower = 0;
	}

}


/// <summary>
/// ハフ変換パラメータを設定する
/// </summary>
/// <param name="edgthr1">Cannyエッジ検出閾値1(IN)</param>
/// <param name="edgthr2">Cannyエッジ検出閾値2(IN)</param>
/// <param name="linthr">HoughLinesP投票閾値(IN)</param>
/// <param name="minlen">HoughLinesP最小線分長(IN)</param>
/// <param name="maxgap">HoughLinesP最大ギャップ長(IN)</param>
void DisparityFilter::setHoughTransformParameter(int edgthr1, int edgthr2, int linthr, int minlen, int maxgap)
{
	// エッジ検出 Canny閾値1
	edgeCannyThreshold1 = edgthr1;
	// エッジ検出 Canny閾値2
	edgeCannyThreshold2 = edgthr2;

	// ハフ変換 houghLinesP閾値　
	houghLinesPThreshold = linthr;
	// ハフ変換 houghLinesP 最小の線分長
	houghLinesPMinLength = minlen;
	// ハフ変換 houghLinesP 最大のギャップ長
	houghLinesPMaxGap = maxgap;

}


/// <summary>
/// 視差を平均化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="prgtimg">右（基準）画像データ 右下原点(IN)</param>
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
bool DisparityFilter::averageDisparityData(int imghgt, int imgwdt, unsigned char* prgtimg,
	int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int dspofsx, int dspofsy, int depth, int shdwdt,
	int *pblkval, int *pblkcrst,
	unsigned char* pdspimg, float* ppxldsp, float* pblkdsp)
{

	if (dispAveDisp == 0 && edgeLineComplement == 0) {
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

	// エッジ線を補完する
	if (edgeLineComplement == 1) {
		sharpenLinearEdge(imghgt, imgwdt, prgtimg,
			blkhgt, blkwdt, mtchgt, mtcwdt, dspofsx, dspofsy, depth, shdwdt,
			pblkval);
	}

	// 視差値を平均化する
	if (dispAveDisp == 1) {

		if (useOpenCLForAveDisp == 0) {
			getAveragingDisparity(imghgt, imgwdt, pblkval);
		}
		else {
			getAveragingDisparityOpenCV(imghgt, imgwdt, pblkval, average_disp);
		}

		// 視差補完する
		if (dispComplementDisparity == 1) {
			getComplementDisparity(imghgt, imgwdt, pblkval, pblkcrst);
		}

		// 視差穴埋めをする
		if (dispComplementHoleFilling == 1) {
			getHoleFillingDisparity(imghgt, imgwdt, pblkval, pblkcrst);
		}
	}

	// ブロックの視差を画素へ展開する
	getDisparityImage(imghgt, imgwdt, pblkval, pdspimg, ppxldsp, pblkdsp);

	// ブロック視差値を保存する
	if (saveBlockInfo == true) {
		writeBlockDisparity(imghgt, imgwdt, average_disp);
		saveBlockInfo = false;
	}

	return true;
}


/// <summary>
/// 直線エッジの視差を鮮明化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="prgtimg">右（基準）画像データ 右下原点(IN)</param>
/// <param name="blkhgt">視差ブロックの高さ(IN)</param>
/// <param name="blkwdt">視差ブロックの幅(IN)</param>
/// <param name="mtchgt">マッチングブロックの高さ(IN)</param>
/// <param name="mtcwdt">マッチングブロックの幅(IN)</param>
/// <param name="dspofsx">視差ブロック横オフセット(IN)</param>
/// <param name="dspofsy">視差ブロック縦オフセット(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="shdwdt">遮蔽領域幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <returns>処理結果を返す</returns>
void DisparityFilter::sharpenLinearEdge(int imghgt, int imgwdt, unsigned char* prgtimg,
	int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int dspofsx, int dspofsy, int depth, int shdwdt,
	int *pblkval)
{

	// 画像からエッジ線分を取得する
	int linall;
	int linno = getEdgeLineSegment(imghgt, imgwdt, prgtimg, MaxLines, LineSegments, &linall);

	// 線分上の視差ブロックを取得する
	getLineSegmentBlocks(imghgt, imgwdt, prgtimg,
		blkhgt, blkwdt, mtchgt, mtcwdt, dspofsx, dspofsy, depth, shdwdt,
		pblkval, linno, LineSegments, lineBlockPoints, lineBlockValues, lineBlockWeight, lineBlockComplement);

}


/// <summary>
/// 視差値を平均化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
void DisparityFilter::getAveragingDisparity(int imghgt, int imgwdt, int* pblkval)
{
	if (numOfBands < 2) {
		getWholeAveragingDisparity(imghgt, imgwdt, pblkval);
	}
	else {
		getBandAveragingDisparity(imghgt, imgwdt, pblkval);
	}

}


/// <summary>
/// OpenCVを使って視差値を平均化する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN/OUT)</param>
void DisparityFilter::getAveragingDisparityOpenCV(int imghgt, int imgwdt, int* pblkval, float *pavedsp)
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
static char *kernelAverageDisparity = (char*)"__kernel void kernelAverageDisparity(\n\
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
static bool openCLContextInit = false;

/// <summary>
/// OpenCLコンテキスト
/// </summary>
static cv::ocl::Context contextAveraging;

/// <summary>
/// カーネルプログラム
/// </summary>
static cv::ocl::Program kernelProgramAveraging;

/// <summary>
/// カーネルオブジェクト
/// </summary>
cv::ocl::Kernel kernelObjectAveraging;

/// <summary>
/// glaobalWorkSize
/// </summary>
static size_t globalSizeAveraging[2];


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
void DisparityFilter::getAveragingDisparityOpenCL(int imghgtblk, int imgwdtblk, int dspwdtblk,
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

			msg = ("Compile Error has occurred.\n");
			msg += errMsg.c_str();
			msg += ("\n");

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
void DisparityFilter::getWholeAveragingDisparity(int imghgt, int imgwdt, int* pblkval)
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
		pblkval, 0, imghgtblk);

}


/// <summary>
/// 視差値を平均化する
/// </summary>
/// <param name="imghgtblk">画像のブロック高さ(IN)</param>
/// <param name="imgwdtblk">画像のブロック幅(IN)</param>
/// <param name="dspwdtblk">有効画像のブロック幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(OUT)</param>
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
void DisparityFilter::getAveragingDisparityInBand(int imghgtblk, int imgwdtblk, int dspwdtblk,
	int* pblkval, int jstart, int jend)
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
				continue;
			}

			// 有効視差率
			float ratio = (float)cnt / wgtdspcnt * 100;

			if (ratio >= dispAveValidRatio) {
				pblkval[idx] = (int)ave;
			}
			else {
				pblkval[idx] = 0;
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
/// <param name="pDestImage">視差画像 右下基点(OUT)</param>
/// <param name="pTempParallax">視差情報 右下基点(OUT)</param>
/// <param name="pBlockDepth">ブロック視差情報 右下基点(OUT)</param>
void DisparityFilter::getDisparityImage(int imghgt, int imgwdt,
	int* pblkval, unsigned char* pDestImage, float* pTempParallax, float* pBlockDepth)
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
			float dsp = (float)pblkval[imgwdtblk * jblk + iblk];

			// 視差値の範囲を制限する
			// 範囲を超えた場合は視差なしにする
			// 補完で埋め戻す
			if (dispLimitation == 1 &&
				(dsp < dispLowerLimit || dsp > dispUpperLimit)) {
				dsp = 0.0;
			}

			dsp = dsp / MATCHING_SUBPIXEL_TIMES;

			// ブロック視差情報を保存する
			pBlockDepth[imgwdtblk * jblk + iblk] = dsp;

			// jpxl : 視差ブロックのy座標
			// ipxl : 視差ブロックのx座標
			int jpxl = jblk * disparityBlockHeight + dispBlockOffsetY;
			int ipxl = iblk * disparityBlockWidth + dispBlockOffsetX;

			// 視差をブロックから画素へ展開する
			for (int j = jpxl; j < jpxl + disparityBlockHeight; j++) {
				for (int i = ipxl; i < ipxl + disparityBlockWidth; i++) {
					pDestImage[imgwdt * j + i] = (unsigned char)(dsp * dsprt);
					pTempParallax[imgwdt * j + i] = dsp;
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
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void DisparityFilter::getComplementDisparity(int imghgt, int imgwdt, int* pblkval, int* pblkcrst)
{
	// 視差を補完する
	getVerticalComplementDisparity(imghgt, imgwdt, false, pblkval, pblkcrst);
	getHorizontalComplementDisparity(imghgt, imgwdt, false, pblkval, pblkcrst);
	getDiagonalUpComplementDisparity(imghgt, imgwdt, false, pblkval, pblkcrst);
	getDiagonalDownComplementDisparity(imghgt, imgwdt, false, pblkval, pblkcrst);

}


/// <summary>
/// 視差を穴埋めする
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void DisparityFilter::getHoleFillingDisparity(int imghgt, int imgwdt, int* pblkval, int* pblkcrst)
{
	// 視差補完領域の穴埋めをする
	getHorizontalComplementDisparity(imghgt, imgwdt, true, pblkval, pblkcrst);
	getVerticalComplementDisparity(imghgt, imgwdt, true, pblkval, pblkcrst);
	getDiagonalUpComplementDisparity(imghgt, imgwdt, true, pblkval, pblkcrst);
	getDiagonalDownComplementDisparity(imghgt, imgwdt, true, pblkval, pblkcrst);
	getHorizontalComplementDisparity(imghgt, imgwdt, true, pblkval, pblkcrst);
	getVerticalComplementDisparity(imghgt, imgwdt, true, pblkval, pblkcrst);

}


/// <summary>
/// 視差なしを補完する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="holefill">補完領域の穴埋め(IN)</param>
/// <param name="pblkval">ブロック視差値(倍精度整数サブピクセル)(IN/OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void DisparityFilter::getHorizontalComplementDisparity(int imghgt, int imgwdt, bool holefill,
	int* pblkval, int* pblkcrst)
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
			complementForward(imgblkwdt, id, hztOfs, jd, id, pblkval);
		}
		// ブロック後方走査
		// 右下基点画像 左から右　
		int stid = id - 1;
		for (id = stid; id >= hztOfs; id--) {
			complementBackward(imgblkwdt, id, stid, jd, id, pblkval, pblkcrst, holefill,
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
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void DisparityFilter::getVerticalComplementDisparity(int imghgt, int imgwdt, bool holefill, 
	int* pblkval, int* pblkcrst)
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
			complementForward(imgblkwdt, jd, vrtOfs, jd, id, pblkval);
		}
		// ブロック上方走査
		// 右下基点画像 上から下
		int stjd = jd - 1;
		for (jd = stjd; jd >= vrtOfs; jd--) {
			complementBackward(imgblkwdt, jd, stjd, jd, id, pblkval, pblkcrst, holefill,
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
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void DisparityFilter::getDiagonalDownComplementDisparity(int imghgt, int imgwdt, bool holefill, 
	int* pblkval, int* pblkcrst)
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
			complementForward(imgblkwdt, id, idd, jd, id, pblkval);
		}
		// ブロック斜め左上走査
		// 右下原点画像 左上から右下
		int stid = id - 1;
		for (jd = jd - 1, id = stid; jd >= vrtOfs && id >= hztOfs; jd--, id--) {
			complementBackward(imgblkwdt, id, stid, jd, id, pblkval, pblkcrst, holefill,
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
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
void DisparityFilter::getDiagonalUpComplementDisparity(int imghgt, int imgwdt, bool holefill,
	int* pblkval, int* pblkcrst)
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
			complementForward(imgblkwdt, jd, jdd, jd, id, pblkval);
		}
		// ブロック斜め左下走査
		// 右下基点画像 左下から右上
		int stjd = jd - 1;
		for (jd = stjd, id = id + 1; (jd >= vrtOfs) && (id < (ie - hztOfs)); jd--, id++) {
			complementBackward(imgblkwdt, jd, stjd, jd, id, pblkval, pblkcrst, holefill,
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
void DisparityFilter::complementForward(int imgblkwdt, int ii, int sti, int jd, int id,
	int* pblkval)
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

	if (ii != sti) {
		// 視差値ゼロ（視差なし）を補完する
		if (blkcmp[ii] == 0) {
			if (blkcmp[ii - 1] > 0) {
				// 後方の重みをインクリメントしてセットする
				// 補完視差値を継承する
				wgtcmp[ii] = wgtcmp[ii - 1] + 1;
				blkcmp[ii] = blkcmp[ii - 1];
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
/// <param name="pblkcrst">ブロックコントラスト(IN)</param>
/// <param name="holefill">補完領域の穴埋め(IN)</param>
/// <param name="blkwdt">補完ブロックの画素幅(IN)</param>
/// <param name="midprt">中央領域補完画素幅の視差値倍率(IN)</param>
/// <param name="toprt">先頭輪郭補完画素幅の視差値倍率(IN)</param>
/// <param name="btmrt">後端輪郭補完画素幅の視差値倍率(IN)</param>
void DisparityFilter::complementBackward(int imgblkwdt, int ii, int sti, int jd, int id,
	int* pblkval, int *pblkcrst, bool holefill,
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
		}
	}

	// 前方の重みと補完視差値を取得する
	int wgttmp = wgtcmp[ii + 1];
	int blktmp = blkcmp[ii + 1];

	// 先頭が補完視差値がゼロの場合を補完する
	// 先頭は重みだけセットされている
	// 後方の補完視差値を前方の補完視差値と同じにする
	if ((blkcmp[ii] == 0 && wgtcmp[ii] > 0)) {
		blkcmp[ii] = blktmp;

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
						float dspbwd = (float)blktmp;
						float dspfwd = (float)blkcmp[ii];
						float dspsubpix = (dspfwd * wgttmp + dspbwd * wgtcmp[ii]) / (wgttmp + wgtcmp[ii]);

						pblkval[imgblkwdt * jd + id] = (int)dspsubpix;
					}
				}
			}

		}
		wgtcmp[ii] = wgttmp;
		blkcmp[ii] = blktmp;
	}

}


/// <summary>
/// ブロックの視差値を書き出す
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="pavedsp">ブロック視差値(倍精度浮動小数)(IN)</param>
void DisparityFilter::writeBlockDisparity(int imghgt, int imgwdt, float *pavedsp)
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
void DisparityFilter::createAveragingThread()
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
void DisparityFilter::deleteAveragingThread()
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
UINT DisparityFilter::averagingBandThread(LPVOID parg)
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
			pBand->pblkval, pBand->bandStart, pBand->bandEnd);

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
void DisparityFilter::getBandAveragingDisparity(int imghgt, int imgwdt, int* pblkval)
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


/// <summary>
/// 画像からエッジ線分を取得する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="prgtimg">右（基準）画像データ 右下原点(IN)/param>
/// <param name="linnum">線分座標を格納する配列のサイズ</param>
/// <param name="linseg">線分座標を格納する配列(OUT)</param>
/// <param name="plinnum">検出した線分の総数(OUT)</param>
/// <returns>検出した線分の数を返す</returns>
int DisparityFilter::getEdgeLineSegment(int imghgt, int imgwdt, unsigned char *prgtimg,
	int linnum, int linseg[][4], int *plinall)
{

	// 右補正画像
	cv::Mat imgInput(imghgt, imgwdt, CV_8UC1, prgtimg);
	// エッジ検出画像
	cv::Mat imgCanny(imghgt, imgwdt, CV_8UC1);

	// エッジを検出する
	// OpenCV::Cannyを使用してエッジ画像を生成する (入力画像,出力画像,閾値1,閾値2)
	cv::Canny(imgInput, imgCanny, edgeCannyThreshold1, edgeCannyThreshold2);

	// ハフ変換を行う
	// OpenCV::Cannyをエッジ画像から線分を抽出する
	//  (入力画像,検出された線分,距離分解能,角度分解能,投票の閾値,最小の線分長,最大距離)
	std::vector<cv::Vec4i> lines;

	int houghLinesPRho = 1;
	int houghLinesPTheta = 1;

	cv::HoughLinesP(imgCanny, lines, houghLinesPRho, houghLinesPTheta * CV_PI / 180,
		houghLinesPThreshold, houghLinesPMinLength, houghLinesPMaxGap);

	int linno = 0;

	for (size_t i = 0; i < lines.size(); i++) {

		// 配列の上限に達したら抜ける 
		if (linno >= linnum) {
			break;
		}
		int orgx = lines[i][0]; // 始点x座標
		int orgy = lines[i][1]; // 始点y座標
		int endx = lines[i][2]; // 終点x座標
		int endy = lines[i][3]; // 終点y座標
		// 水平な線分を除く
		int difx = abs(orgy - endy);
		if (difx < 4) {
			continue;
		}
		// 線分の始点、終点の座標を保存する
		linseg[linno][0] = orgx;
		linseg[linno][1] = orgy;
		linseg[linno][2] = endx;
		linseg[linno][3] = endy;
		linno++;
	}

	// 検出した線分の総数
	*plinall = (int)lines.size();

	return linno;
}


/// <summary>
/// 線分上の視差ブロックを取得する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// /// <param name="prgtimg">右（基準）画像データ 右下原点(IN)/param>
/// <param name="blkhgt">視差ブロックの高さ(IN)</param>
/// <param name="blkwdt">視差ブロックの幅(IN)</param>
/// <param name="mtchgt">マッチングブロックの高さ(IN)</param>
/// <param name="mtcwdt">マッチングブロックの幅(IN)</param>
/// <param name="dspofsx">視差ブロック横オフセット(IN)</param>
/// <param name="dspofsy">視差ブロック縦オフセット(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="shdwdt">遮蔽領域幅(IN)</param>
/// <param name="pblkval">視差ブロック視差値(1000倍サブピクセル精度整数)(IN/OUT)</param>
/// <param name="linnum">線分の数(IN)</param>
/// <param name="linseg">線分座標の配列(IN)</param>
/// <param name="linblk">線分上の視差ブロックの位置を格納する配列(OUT)</param>
/// <param name="dspval">線分上の視差ブロックの視差を格納する配列(OUT)</param>
/// <param name="dspwgt">線分上の視差ブロックの補完重みを格納する配列(OUT)</param>
/// <param name="dspcmp">線分上の視差ブロックの補完値を格納する配列(OUT)</param>
void DisparityFilter::getLineSegmentBlocks(int imghgt, int imgwdt, unsigned char *prgtimg,
	int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int dspofsx, int dspofsy, int depth, int shdwdt,
	int *pblkval, int linnum, int linseg[][4], int linblk[][2], int *dspval, int *dspwgt, int *dspcmp)
{

	// 画像の高さブロック数
	int imghgtblk = imghgt / blkhgt;
	// 画像の幅ブロック数
	int imgwdtblk = imgwdt / blkwdt;

	// 視差ブロックの補完幅の上限
	int maxlnw = edgeComplementWidthUpper;
	// 視差ブロックの補完幅の下限
	int minlnw = edgeComplementWidthLower;

	// 線分上を走査する
	for (int i = 0; i < linnum; i++) {

		// 線分の始点座標
		int orgx = linseg[i][0];
		int orgy = linseg[i][1];
		// 線分の終点座標
		int endx = linseg[i][2];
		int endy = linseg[i][3];
		// 線分の変位　 
		int diffx = endx - orgx;
		int diffy = endy - orgy;

		// 線分の傾きを求める
		int absdfx = abs(diffx);
		int absdfy = abs(diffy);

		double slope;
		int blkno;

		// 傾きaが1未満の場合
		if (absdfx > absdfy) {
			// y = ax + b
			slope = (double)diffy / diffx;

			// 始点終点を入れ替える
			// xを正方向に走査できる
			if (diffx < 0) {
				orgx = linseg[i][2];
				orgy = linseg[i][3];
				endx = linseg[i][0];
				endy = linseg[i][1];
			}
			// y = slope * (x - orgx) + orgy
			blkno = 0;

			// 線分上を探索する
			// ステップ幅を視差ブロック幅の半分
			double step_d = (double)blkwdt / 2;
			for (double x = (double)orgx; x < (double)endx + step_d; x += step_d) {
				double y = slope * (x - orgx) + orgy;

				// ブロック位置を求める
				int blkx = (int)((x - dspofsx) / blkwdt);
				int blky = (int)((y - dspofsy) / blkhgt);

				// ブロック位置を保存する
				linblk[blkno][0] = blkx;
				linblk[blkno][1] = blky;

				// ブロック視差を保存する
				dspval[blkno] = pblkval[blky * imgwdtblk + blkx];

				blkno++;

			}

			// 外れ視差を除去する
			int dspcnt = removeOutsideDisparity(blkno, dspval, depth);

			// 線分上の視差の回帰直線を求める
			double regslp;
			double regint;
			double coefdet;
			int dspnum = calculateRegressionLine(blkno, dspval, &regslp, &regint, &coefdet);

			// 最小視差ブロック数以上かつ最小線形性指数以上の場合
			if (dspnum >= edgeLineMinBlocks && coefdet >= edgeLineMinLinearity) {
				// 視差なしを両端の視差を使って補完する
				// 端の視差なしは回帰直線で求めた傾きを使って延ばす
				setComplementDisparity(blkno, dspval, dspwgt, dspcmp, regslp);

				// 埋め戻す 
				for (int k = 0; k < blkno; k++) {
					int blkx = linblk[k][0];
					int blky = linblk[k][1];

					for (int ln = minlnw; ln <= maxlnw; ln++) {
						int lnblky = blky + ln;
						if (lnblky >= 0 && lnblky < imghgtblk) {
							pblkval[lnblky * imgwdtblk + blkx] = dspval[k];
						}
					}

				}
			}
		}
		// 傾きaが1以上の場合
		else {
			// x = ay + b
			slope = (double)diffx / diffy;

			// 始点終点を入れ替える
			// yを正方向に走査できる
			if (diffy < 0) {
				orgx = linseg[i][2];
				orgy = linseg[i][3];
				endx = linseg[i][0];
				endy = linseg[i][1];
			}
			// x = slope * (y - orgy) + orgx
			blkno = 0;

			// 線分上を探索する
			// ステップ幅を視差ブロック幅の半分
			double step_d = (double)blkhgt / 2;
			for (double y = (double)orgy; y < (double)endy + step_d; y += step_d) {
				double x = slope * (y - orgy) + orgx;

				// ブロック位置を求める
				int blkx = (int)((x - dspofsx) / blkwdt);
				int blky = (int)((y - dspofsy) / blkhgt);

				// ブロック位置を保存する
				linblk[blkno][0] = blkx;
				linblk[blkno][1] = blky;

				// ブロック視差を保存する
				dspval[blkno] = pblkval[blky * imgwdtblk + blkx];

				blkno++;

			}

			// 外れ視差を除去する
			int dspcnt = removeOutsideDisparity(blkno, dspval, depth);

			// 線分上の視差の回帰直線を求める
			double regslp;
			double regint;
			double coefdet;
			int dspnum = calculateRegressionLine(blkno, dspval, &regslp, &regint, &coefdet);

			// 最小視差ブロック数以上かつ最小線形性指数以上の場合
			if (dspnum >= edgeLineMinBlocks && coefdet >= edgeLineMinLinearity) {
				// 視差なしを両端の視差を使って補完する
				// 端の視差なしは回帰直線で求めた傾きを使って延ばす
				setComplementDisparity(blkno, dspval, dspwgt, dspcmp, regslp);
				// 埋め戻す 
				for (int k = 0; k < blkno; k++) {
					int blkx = linblk[k][0];
					int blky = linblk[k][1];

					for (int ln = minlnw; ln <= maxlnw; ln++) {
						int lnblkx = blkx + ln;
						if (lnblkx >= 0 && lnblkx < imgwdtblk) {
							pblkval[blky * imgwdtblk + lnblkx] = dspval[k];
						}
					}

				}
			}
		}

	}

}


/// <summary>
/// 視差を補完する
/// </summary>
/// <param name="blknum">視差ブロック数(IN)</param>
/// <param name="dspval">視差値を格納した配列(IN/OUT)</param>
/// <param name="dspwgt">線分上の視差ブロックの補完重みを格納する配列</param>
/// <param name="dspcmp">線分上の視差ブロックの補完値を格納する配列(OUT)</param>
/// <param name="slope">回帰直線の傾き(IN)</param>
void DisparityFilter::setComplementDisparity(int blknum, int *dspval, int *dspwgt, int *dspcmp, double slope)
{

	// 始点->終点
	// 始点から終点に向かって視差なしブロックを重み付けする
	int prvwgt = 0;
	int prvcmp = 0;
	for (int i = 0; i < blknum; i++) {
		// 視差なしブロックの場合
		// 重み付けする
		// 補完視差は引き継がれる
		// 始点が視差なしの場合は補完視差になる
		if (dspval[i] == 0) {
			dspwgt[i] = prvwgt + 1;
			dspcmp[i] = prvcmp;
		}
		// 視差なしブロックの場合
		// 重み付け0にする
		// 補完視差は同じにする
		else {
			dspwgt[i] = 0;
			dspcmp[i] = dspval[i];
		}
		prvwgt = dspwgt[i];
		prvcmp = dspcmp[i];
	}

	// すべて視差なしの場合
	// 補完しない
	if (prvcmp == 0) {
		return;
	}

	// 終点->始点
	// 始点から終点に向かって視差なしを補完する
	prvwgt = 0;
	prvcmp = 0;
	int	setval;
	for (int i = blknum - 1; i >= 0; i--) {
		// 重みなし(0)はそのまま
		// 重み付きは補完値を求める
		if (dspwgt[i] == 0) {
			setval = dspcmp[i];
			prvwgt = 0;
			prvcmp = setval;
		}
		else {
			// 終点が視差なしの場合
			// 前補完視差が0
			if (prvcmp == 0) {
				// y = ax + b
				// 補完視差値 = slope * dspwgt[i] + dspcmp[i]  
				setval = (int)(slope * dspwgt[i] + dspcmp[i]);
				// dspwgt[i]が0になるまでprvcmpは0のまま繰り返す
			}
			// 始点が視差なしの場合
			// 現補完視差が0
			else if (dspcmp[i] == 0) {
				// y = ax + b
				// 始点へ向かって前視差値から傾き分減らす
				// 補完視差値 = prvcmp - a  
				setval = (int)(prvcmp - slope);
				prvcmp = setval;
			}
			// 前後の視差から補完視差を求める
			// 重み平均 : (前視差値 * 後重み + 後視差値 * 前重み) / (後重み + 前重み)
			else {
				prvwgt = prvwgt + 1;
				setval = (dspcmp[i] * prvwgt + prvcmp * dspwgt[i]) / (prvwgt + dspwgt[i]);
				// dspwgt[i]が0になるまでprvcmpはそのまま繰り返す
			}
		}
		// 補完視差を保存する
		dspval[i] = setval;
	}

}


/// <summary>
/// 線分上の視差の回帰直線を求める
/// </summary>
/// <param name="blknum">視差ブロック数(IN)</param>
/// <param name="dspval">視差値を格納した配列(IN)</param>
/// <param name="pslope">回帰直線の傾き(OUT)</param>
/// <param name="pintcpt">回帰直線の切片(OUT)</param>
/// <param name="pcoefdet">決定係数(OUT)</param>
/// <returns>計算対象のブロック数を返す</returns>
int DisparityFilter::calculateRegressionLine(int blknum, int *dspval, double *pslope, double *pintcpt, double *pcoefdet)
{

	// num : 計算対象（視差がある）のブロック数
	// avex : xの平均 xはブロック位置（配列のインデックス）
	// avey : yの平均 yは視差値
	// varx : xの分散
	// vary : yの分散
	// covar : xとyの共分散
	// vardy : 残差平方和
	// coefdet : 決定係数
	// slope : 回帰直線の傾き
	// intcpt : 回帰直線の切片
	int num = 0;
	int sumx = 0;
	int sumy = 0;

	double avex = 0.0;
	double avey = 0.0;
	double varx = 0.0;
	double vary = 0.0;
	double covar = 0.0;
	double vardy = 0.0;
	double coefdet = 0.0;

	double slope = 0.0;
	double intcpt = 0.0;

	// 総和を求める
	for (int i = 0; i < blknum; i++) {
		if (dspval[i] > 0) {
			sumx += i;
			sumy += dspval[i];
			num++;
		}
	}

	// 視差ありブロックが2つ以上ある場合
	if (num > 2) {
		// 平均を求める
		avex = (double)sumx / num;
		avey = (double)sumy / num;

		// 分散を求める
		for (int i = 0; i < blknum; i++) {
			if (dspval[i] > 0) {
				double devx = (double)i - avex;
				double devy = dspval[i] - avey;
				varx += (devx * devx);
				vary += (devy * devy);
				covar += (devx * devy);
			}
		}

		// 傾き、切片を求める
		slope = covar / varx;
		intcpt = avey - slope * avex;

		// 残差平方和を求める
		for (int i = 0; i < blknum; i++) {
			if (dspval[i] > 0) {
				double calcy = slope * i + intcpt;
				double diff = dspval[i] - calcy;
				vardy += (diff * diff);
			}
		}

		// 決定係数を求める
		coefdet = (1.0 - vardy / vary) * 100;

	}

	*pslope = slope;
	*pintcpt = intcpt;
	*pcoefdet = coefdet;

	return num;
}


/// <summary>
/// 外れ視差を除去する
/// </summary>
/// <param name="blknum">視差ブロック数(IN)</param>
/// <param name="dspval">視差ブロックの配列(IN/OUT)</param>
/// <param name="depth">探索幅</param>
int DisparityFilter::removeOutsideDisparity(int blknum, int *dspval, int depth)
{
	// ヒストグラム幅固定
	// 最大幅は探索幅で決まる
	// この幅で収まるようにヒストグラムを作成する

	// 探索幅の倍精度整数
	int dspwdt = depth * MATCHING_SUBPIXEL_TIMES;
	// 視差サブピクセル精度倍率
	int dspitgrt = dspwdt / 1024 + 1;
	// 移動積分視差サブピクセル幅
	int dspitgwdt = dspwdt / dspitgrt;

	// 視差検出ブロック数
	int dspcnt = 0;

	// 移動積分ヒストグラム
	int integ[1024] = { 0 };

	for (int i = 0; i < blknum; i++) {
		// 視差値を取得する
		int disp = dspval[i];

		// ヒストグラムを作成する
		if (disp > MATCHING_SUBPIXEL_TIMES) {
			// 視差検出ブロック数をカウントする
			dspcnt++;

			// 倍精度整数サブピクセルの精度をヒストグラム幅に合わせて落とす
			// 移動積分を求める
			int stwi = (disp - lineAveIntegRange) / dspitgrt;
			int endwi = (disp + lineAveIntegRange) / dspitgrt;
			stwi = stwi < 0 ? 0 : stwi;
			endwi = endwi >= dspitgwdt ? (dspitgwdt - 1) : endwi;
			for (int k = stwi; k <= endwi; k++) {
				integ[k]++;
			}
		}
	}

	// 最頻値を求める
	int maxcnt = 0;
	int maxdsp = 0;
	// 最頻値が連続する場合はその中央値を最頻値とする
	int maxwnd = 0;
	bool maxin = false;
	for (int i = 0; i < dspitgwdt; i++) {
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

	// 有効範囲を求める
	// 最頻値のサブピクセル精度を倍精度整数へ戻す
	int mode = maxdsp * dspitgrt;

	// 最頻値の±1/4幅を有効範囲とする
	int high = mode + mode / 4;
	int low = mode - mode / 4;

	high = high >= dspwdt ? (dspwdt - 1) : high;
	low = low < MATCHING_SUBPIXEL_TIMES ? MATCHING_SUBPIXEL_TIMES : low;

	// 有効範囲外の視差を除去する
	for (int i = 0; i < blknum; i++) {
		if (dspval[i] <= low || dspval[i] > high) {
			dspval[i] = 0;
		}
	}

	return dspcnt;
}

