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
 * @file StereoMatching.cpp
 * @brief Implementation of Block Matching processing
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 *
 * @details
 * - 右画像を基準としその画像中のブロックと同じパターンのブロックを左画像から探索し、視差値を求める
 * - マッチング手法としてSSD(Sum of Squared Difference)を用いる
 * - サブピクセル推定方法に放物線近似を用いる
 * - バックマッチングによりオクルージョンによる誤った視差と、視野外の探索による誤った視差を除去する
 *
*/
#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "StereoMatching.h"

// サブピクセル倍率 (1000倍サブピクセル精度）
#define MATCHING_SUBPIXEL_TIMES 1000

// 最大マッチング探索幅
#define ISC_IMG_DEPTH_MAX 512

/// <summary>
/// ブロック視差値（サブピクセル精度：浮動小数）の配列（視差ブロックごと）
/// </summary>
static float *block_dsp;

/// <summary>
/// バックマッチングブロック視差値（サブピクセル精度：浮動小数）の配列（視差ブロックごと）
/// </summary>
static float *bk_block_dsp;

/// <summary>
/// ブロックコントラスト（視差ブロックごと）
/// </summary>
static int *block_crst;

/// <summary>
/// マッチング探索幅
/// </summary>
static int matchingDepth = 256;

/// <summary>
/// 画像遮蔽幅
/// </summary>
static int shadeWidth = 256;

/// <summary>
/// 入力補正画像の高さ
/// </summary>
static int correctedImageHeight = 720;

/// <summary>
/// 入力補正画像の幅
/// </summary>
static int correctedImageWidth = 1280;

/// <summary>
/// 視差ブロックサイズ　高さ
/// </summary>
static int disparityBlockHeight = 4;

/// <summary>
/// 視差ブロックサイズ　幅
/// </summary>
static int disparityBlockWidth = 4;

/// <summary>
/// マッチングブロックサイズ　高さ
/// </summary>
static int  matchingBlockHeight = 4;

/// <summary>
/// マッチングブロックサイズ　幅
/// </summary>
static int matchingBlockWidth = 4;

/// <summary>
/// コントラスト閾値
/// </summary>
static int contrastThreshold = 40;

/// <summary>
/// 階調補正モードステータス 0:オフ 1:オン
/// </summary>
static int gradationCorrectionMode = 0;

// 画像サイズ
#define IMG_WIDTH_VM 752
#define IMG_WIDTH_XC 1280

// コントラストオフセット
// 入力された画像サイズによってオフセットを選択する
// コントラスト⊿L/Lmeanを1000倍して評価値とする
#define CONTRAST_OFFSET_VM (1.8 * 1000)
#define CONTRAST_OFFSET_XC (1.2 * 1000)

// 階調補正のコントラストオフセットの倍率
#define CONTRAST_XC_DRADATION_FACTOR (2.0) 

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

/// <summary>
/// ブロックマッチングにOpenCLの使用を設定する
/// </summary>
static int dispMatchingUseOpenCL = 0;

/// <summary>
/// ブロックマッチングのシングルコア実行を設定する
/// </summary>
static int dispMatchingRunSingleCore = 0;

/// <summary>
/// 視差ブロック横オフセット
/// </summary>
static int dispBlockOffsetX = 0;

/// <summary>
/// 視差ブロック縦オフセット
/// </summary>
static int dispBlockOffsetY = 0;

/// <summary>
/// バックマッチング 0:しない 1:する
/// </summary>
static int enableBackMatching = 0;

/// <summary>
/// バックマッチング視差評価領域幅（片側）
/// </summary>
int backMatchingEvaluationWidth = 1;

/// <summary>
/// バックマッチング視差評価視差値幅
/// </summary>
int backMatchingEvaluationRange = 3;

/// <summary>
/// バックマッチング評価視差正当率（％）
/// </summary>
int backMatchingValidRatio = 30;

/// <summary>
/// バックマッチング評価視差ゼロ率（％）
/// </summary>
int backMatchingZeroRatio = 60;


/// <summary>
/// バンド分割ステレオマッチング
/// </summary>

#define NUM_OF_BANDS 8

#define MAX_NUM_OF_BANDS 40

static int numOfBands = NUM_OF_BANDS;

struct BNAD_THREAD_INFO {

	// マッチングスレッドオブジェクトのポインタ
	HANDLE bandThread;
	
	// マッチングスレッド実行開始イベントのハンドル
	HANDLE startEvent;
	// マッチングスレッド停止イベントのハンドル
	HANDLE stopEvent;
	// マッチングスレッド完了通知イベントのハンドル
	HANDLE doneEvent;

	// 入力補正画像の高さ
	int imghgt;
	// 入力補正画像の幅
	int imgwdt;
	// マッチング探索幅
	int depth;
	// コントラスト閾値
	int crstthr;
	// コントラストオフセット
	int crstofs;
	// 階調補正モードステータス
	int grdcrct;

	// マッチングステップの高さ
	int stphgt;
	// マッチングステップの幅
	int stpwdt;
	// マッチングブロックの高さ
	int blkhgt;
	// マッチングブロックの幅
	int blkwdt;
	// 視差ブロック画像の高さ
	int imghgtblk;
	// 視差ブロック画像の幅
	int imgwdtblk;

	// 入力基準画像データ
	unsigned char *pimgref;
	// 入力比較画像データ
	unsigned char *pimgcmp;
	// マッチングの視差値
	float *pblkdsp;
	// バックマッチングの視差値
	float *pblkbkdsp;
	// ブロックコントラスト
	int *pblkcrst;

	// バンド開始ライン位置（y座標）
	int bandStart;
	// バンド終了ライン位置（y座標）
	int bandEnd;

};

static BNAD_THREAD_INFO bandInfo[MAX_NUM_OF_BANDS];

/// <summary>
/// オブジェクトを生成する
/// </summary>
StereoMatching::StereoMatching()
{
}


/// <summary>
/// オブジェクトを破棄する
/// </summary>
StereoMatching::~StereoMatching()
{
}


/// <summary>
/// ステレオマッチングを初期化する
/// </summary>
/// <param name="imghgt">補正画像の高さ(IN)</param>
/// <param name="imgwdt">補正画像の幅(IN)</param>
void StereoMatching::initialize(int imghgt, int imgwdt)
{
	// バッファーを確保する
	// ブロック視差値（サブピクセル精度：浮動小数）の配列（視差ブロックごと）
	block_dsp = (float *)malloc(imghgt * imgwdt * sizeof(float));
	// バックマッチングブロック視差値（サブピクセル精度：浮動小数）の配列（視差ブロックごと）
	bk_block_dsp = (float *)malloc(imghgt * imgwdt * sizeof(float));
	// ブロックコントラスト（視差ブロックごと）
	block_crst = (int *)malloc(imghgt * imgwdt * sizeof(int));

	// 入力補正画像の高さ
	correctedImageHeight = imghgt;
	// 入力補正画像の幅
	correctedImageWidth = imgwdt;

}


/// <summary>
/// ステレオマッチングを終了する
/// </summary>
void StereoMatching::finalize()
{
	// バッファーを解放する
	free(block_dsp);
	free(bk_block_dsp);
	free(block_crst);

}


/// <summary>
/// ステレオマッチングにOpenCLの使用を設定する
/// </summary>
/// <param name="usecl">OpenCLを使用 0:しない1:する(IN)</param>
/// <param name="runsgcr">シングルスレッドで実行 0:しない1:する(IN)</param>
void StereoMatching::setUseOpenCLForMatching(int usecl, int runsgcr)
{
	dispMatchingUseOpenCL = usecl;
	dispMatchingRunSingleCore = runsgcr;

}


/// <summary>
/// ステレオマッチングパラメータを設定する
/// </summary>
/// <param name="imghgt">補正画像の高さ(IN)</param>
/// <param name="imgwdt">補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="blkhgt">視差ブロック高さ(IN)</param>
/// <param name="blkwdt">視差ブロック幅(IN)</param>
/// <param name="mtchgt">マッチングブロック高さ(IN)</param>
/// <param name="mtcwdt">マッチングブロック幅(IN)</param>
/// <param name="blkofsx">視差ブロック横オフセット(IN)</param>
/// <param name="blkofsy">視差ブロック縦オフセット(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
void StereoMatching::setMatchingParameter(int imghgt, int imgwdt, int depth,
	int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int blkofsx, int blkofsy, int crstthr, int grdcrct)
{

	correctedImageHeight = imghgt; // 入力補正画像の縦サイズ
	correctedImageWidth = imgwdt; // 入力補正画像の横サイズ
	matchingDepth = depth; // マッチング探索幅
	shadeWidth = depth; // 画像遮蔽幅

	disparityBlockHeight = blkhgt; // 視差ブロック高さ
	disparityBlockWidth = blkwdt; // 視差ブロック幅
	matchingBlockHeight = mtchgt; // マッチングブロック高さ
	matchingBlockWidth = mtcwdt; // マッチングブロック幅
	
	dispBlockOffsetX = blkofsx; // 視差ブロック横オフセット
	dispBlockOffsetY = blkofsy; // 視差ブロック縦オフセット

	contrastThreshold = crstthr; // コントラスト閾値
	gradationCorrectionMode = grdcrct; // 階調補正モードステータス

}


/// <summary>
/// バックマッチングパラメータを設定する
/// </summary>
/// <param name="enb">バックマッチング 0:しない 1:する(IN)</param>
/// <param name="bkevlwdt">バックマッチング視差評価領域幅（片側）(IN)</param>
/// <param name="bkevlrng">バックマッチング視差評価視差値幅(IN)</param>
/// <param name="bkvldrt">バックマッチング評価視差正当率（％）(IN)</param>
/// <param name="bkzrrt">バックマッチング評価視差ゼロ率（％）(IN)</param>
void StereoMatching::setBackMatchingParameter(int enb, int bkevlwdt, int bkevlrng, int bkvldrt, int bkzrrt)
{
	
	enableBackMatching = enb; // バックマッチング 0:しない 1:する
	backMatchingEvaluationWidth = bkevlwdt; // バックマッチング評価視差幅
	backMatchingEvaluationRange = bkevlrng; // バックマッチング評価視差幅
	backMatchingValidRatio = bkvldrt; // バックマッチング評価視差数
	backMatchingZeroRatio = bkzrrt; // バックマッチング評価視差ゼロ数

}


/// <summary>
/// ステレオマッチングを実行する
/// </summary>
/// <param name="prgtimg">右画（基準）像データ(IN)</param>
/// <param name="plftimg">左画（比較）像データ</param>
void StereoMatching::matching(unsigned char* prgtimg, unsigned char* plftimg)
{

	// ステレオマッチングを実行する
	if (dispMatchingUseOpenCL == 0) {
		executeMatching(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg,
			block_dsp, block_crst);
	}
	else {
		executeMatchingOpenCL(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg,
			block_dsp, block_crst);
	}

}


/// <summary>
/// 視差ブロック情報を取得する
/// </summary>
/// <param name="pblkhgt">視差ブロック高さ(OUT)</param>
/// <param name="pblkwdt">視差ブロック幅(OUT)</param>
/// <param name="pmtchgt">マッチングブロック高さ(OUT)</param>
/// <param name="pmtcwdt">マッチングブロック幅(OUT)</param>
/// <param name="pblkofsx">視差ブロック横オフセット(OUT)</param>
/// <param name="pblkofsy">視差ブロック縦オフセット(OUT)</param>
/// <param name="pdepth">マッチング探索幅(OUT)</param>
/// <param name="pshdwdt">画像遮蔽幅(OUT)</param>
/// <param name="pblkdsp">視差ブロック視差値(OUT)</param>
/// <param name="pblkval">視差ブロック視差値(1000倍サブピクセル精度の整数)(OUT)</param>
/// <param name="pblkcrst">マッチングブロックコントラスト(OUT)</param>
void StereoMatching::getBlockDisparity(int *pblkhgt, int *pblkwdt, int *pmtchgt, int *pmtcwdt,
	int *pblkofsx, int *pblkofsy, int *pdepth, int *pshdwdt, float *pblkdsp, int *pblkval, int *pblkcrst)
{
	int i, j;

	int height = correctedImageHeight / disparityBlockHeight;
	int width = correctedImageWidth / disparityBlockWidth;

	memcpy(pblkdsp, block_dsp, height * width * sizeof(float));

	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			pblkval[j * width + i] = (int)((MATCHING_SUBPIXEL_TIMES * (block_dsp[j * width + i])) + 0.5);
		}
	}

	memcpy(pblkcrst, block_crst, height * width * sizeof(int));

	*pblkhgt = disparityBlockHeight;
	*pblkwdt = disparityBlockWidth;

	*pmtchgt = matchingBlockHeight;
	*pmtcwdt = matchingBlockWidth;

	*pblkofsx = dispBlockOffsetX;
	*pblkofsy = dispBlockOffsetY;

	*pdepth = matchingDepth;
	*pshdwdt = shadeWidth;

}


/// <summary>
/// 視差画素情報を取得する
/// </summary>
/// <param name="imghgt">視差画像を格納するバッファの高さ(IN)</param>
/// <param name="imgwdt">視差画像を格納するバッファの幅(IN)</param>
/// <param name="pdspimg">視差画像データを格納するバッファのポインタ(OUT)</param>
/// <param name="ppxldsp">視差値データを格納するバッファのポインタ(OUT)</param>
void StereoMatching::getDisparity(int imghgt, int imgwdt, unsigned char *pdspimg, float *ppxldsp)
{
	// 視差ブロックの高さ
	int stphgt = disparityBlockHeight;
	// 視差ブロックの幅
	int stpwdt = disparityBlockWidth;
	// マッチングブロックの高さ
	int blkhgt = matchingBlockHeight;
	// マッチングブロックの幅
	int blkwdt = matchingBlockWidth;

	// 視差をブロックから画素へ展開する
	spreadDisparityImage(imghgt, imgwdt, matchingDepth, shadeWidth, stphgt, stpwdt, blkhgt, blkwdt,
		dispBlockOffsetX, dispBlockOffsetY,
		block_dsp, pdspimg, ppxldsp);

}


/// <summary>
/// ステレオマッチングを実行する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">ブロックごとの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::executeMatching(int imghgt, int imgwdt, int depth, 
	unsigned char *pimgref, unsigned char *pimgcmp,
	float * pblkdsp, int *pblkcrst)
{

	// 視差ブロックの高さ
	int stphgt = disparityBlockHeight;
	// 視差ブロックの幅
	int stpwdt = disparityBlockWidth;
	// マッチングブロックの高さ
	int blkhgt = matchingBlockHeight;
	// マッチングブロックの幅
	int blkwdt = matchingBlockWidth;

	// 出力視差ブロック画像の高さ
	// * 小数切り捨て
	int imghgtblk = imghgt / stphgt;
	// 出力視差ブロック画像の幅
	// * 小数切り捨て
	int imgwdtblk = imgwdt / stpwdt;

	// コントラスト閾値
	int crstthr = contrastThreshold;
	// 階調補正モードステータス
	int grdcrct = gradationCorrectionMode;

	// コントラストオフセット
	int crstofs = 0;

	if (imgwdt == IMG_WIDTH_VM) {
		crstofs = (int)CONTRAST_OFFSET_VM;
	}
	else if (imgwdt == IMG_WIDTH_XC) {
		crstofs = (int)CONTRAST_OFFSET_XC;
		// 階調補正モードの場合
		if (grdcrct == 1) {
			crstofs = (int)(CONTRAST_OFFSET_XC * CONTRAST_XC_DRADATION_FACTOR);
		}
	}

	// バックマッチング
	float *pblkbkdsp = NULL;
	shadeWidth = depth;

	if (enableBackMatching == 1) {
		memset(bk_block_dsp, 0, imghgt * imgwdt * sizeof(float));
		pblkbkdsp = bk_block_dsp;
		shadeWidth = 0;
	}

	// 視差を取得する
	getMatchingDisparity(imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
		stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
		pimgref, pimgcmp, pblkdsp, pblkbkdsp, pblkcrst);

	if (enableBackMatching == 1) {
		blendBothMatchingDisparity(imghgt, imgwdt, imghgtblk, imgwdtblk,
			backMatchingEvaluationWidth, backMatchingEvaluationRange, backMatchingValidRatio, backMatchingZeroRatio,
			pblkdsp, pblkbkdsp);
	}

}


/// <summary>
/// 視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <param name="pblkmsk">ブロックコントラスト(OUT)</param>
void StereoMatching::getMatchingDisparity(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp, int *pblkcrst)
{

	// マルチスレッドで実行する
	if (dispMatchingRunSingleCore == 0) {
		getBandDisparity(imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pblkdsp, pblkbkdsp, pblkcrst);
	}
	// シングルスレッドで実行する
	else {
		getWholeDisparity(imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pblkdsp, pblkbkdsp, pblkcrst);
	}

}


/// <summary>
/// 画像全体の視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::getWholeDisparity(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp, int *pblkcrst)
{

	if (pblkbkdsp == NULL) {
		getDisparityInBand(imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pblkdsp, pblkcrst, 0, imghgt);
	}
	else {
		getBothDisparityInBand(imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pblkdsp, pblkbkdsp, pblkcrst, 0, imghgt);
	}

}


/// <summary>
/// バンド内の視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param> 
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
void StereoMatching::getDisparityInBand(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, float * pblkdsp, int *pblkcrst,
	int jstart, int jend)
{

	// jpx : マッチングブロックのy座標
	// ipx : マッチングブロックのx座標
	for (int jpx = jstart; jpx < jend && jpx <= (imghgt - blkhgt); jpx++) {
		for (int ipx = 0; ipx <= (imgwdt - depth - blkwdt); ipx++) {
			// SSDにより視差値を求める
			getDisparityBySSD(ipx, jpx, imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				pimgref, pimgcmp, pblkdsp, pblkcrst);
		}
	}

}


/// <summary>
/// SSDにより視差を取得する
/// </summary>
/// <param name="x">ブロック左上画素x座標(IN)</param>
/// <param name="y">ブロック左上画素y座標(IN)</param>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::getDisparityBySSD(int x, int y, int imghgt, int imgwdt, int depth,
	int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, float * pblkdsp, int *pblkcrst)
{

	// 視差画像の幅
	int imgwdtdsp = imgwdt - depth;
	int jpx = y;
	int ipx = x;
	if (ipx >= imgwdtdsp || jpx % stphgt != 0 || ipx % stpwdt != 0) {
		return;
	}

	// jblk : マッチングブロックのyインデックス
	// iblk : マッチングブロックのxインデックス
	int jblk = jpx / stphgt;
	int iblk = ipx / stpwdt;

	// ブロック輝度最大値の最小値
	int bgtmax = BLOCK_BRIGHTNESS_MAX;
	// 高解像度モードのコントラスト計算のための平均輝度
	int crstave = CONTRAST_CALC_AVERAGE_FOR_HIGH_RESO;
	// ブロック平均輝度最小値
	double minlave = BLOCK_MIN_AVERAGE_BRIGHTNESS;
	// ブロック内の輝度差の最小値
	int mindltl = BLOCK_MIN_DELTA_BRIGHTNESS;

	// SSDを保存する配列
	// 配列サイズはマッチング探索幅
	float ssd[ISC_IMG_DEPTH_MAX];
	// SSD最大値を設定
	float misum = FLT_MAX;

	// 視差値の初期値を設定する
	int disp = 0;

	// マッチング評価関数
	// 左右画像の同じ位置iにある輝度の差の二乗を求め，それを領域全体に渡って足し合わせる
	// SSD = Σ(((右画像の輝度ij - 右画像ブロック輝度平均) - (左画像の輝度ij - 左画像ブロック輝度平均))^2) 
	// 右画像を1画素ずつ探索範囲DEPTHまで右へ動かしてSSDを計算する
	// 一番小さいSSDのとき，最も類似している
	// その探索位置を視差値（整数部）とする
	// 前後のSSDからサブピクセル値（少数部）を算出する
	int jpxe = jpx + blkhgt;
	int ipxe = ipx + blkwdt;

	int sumr = 0; // Σ基準画像の輝度ij
	int sumrr = 0; // Σ(基準画像の輝度ij^2)
	double Lsumr = 0.0; // Σ基準画像の輝度ij
	double Lmin = 255.0; // ブロック内輝度最小値（初期値256階調最大輝度値）
	double Lmax = 0.0; // ブロック内輝度最大値（初期値256階調最小輝度値）
	int blkcnt = blkhgt * blkwdt;

	// 右画像ブロック輝度を評価する
	for (int j = jpx; j < jpxe; j++) {
		for (int i = ipx; i < ipxe; i++) {
			double rfx = (double)pimgref[j * imgwdt + i];
			double xrfx = rfx * rfx;

			sumr += (int)rfx;
			sumrr += (int)xrfx;

			// 階調補正モードオンの場合
			// 補正前の値へ変換する
			if (grdcrct == 1) {
				rfx = xrfx / 255;
			}
			Lsumr += rfx;
			if (Lmin > rfx) {
				Lmin = rfx;
			}
			if (Lmax < rfx) {
				Lmax = rfx;
			}

		}
	}

	// コントラストを求める
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
			crst = (int)((deltaL * 1000 - crstofs) / Lave);
		}

	// コントラストが閾値未満の場合は視差値ゼロにする
	if (crst < crstthr) {
		// 視差値
		pblkdsp[jblk * imgwdtblk + iblk] = 0.0;
		// コントラスト
		pblkcrst[jblk * imgwdtblk + iblk] = 0;
		return;
	}

	for (int k = 0; k < depth; k++) {
		float sumsq = 0.0;

		int sumc = 0; // Σ比較画像の輝度ij
		int sumcc = 0; // Σ(比較画像の輝度ij^2)
		int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)

		for (int j = jpx; j < jpxe; j++) {
			for (int i = ipx; i < ipxe; i++) {
				int rfx = pimgref[j * imgwdt + i];
				int cpx = pimgcmp[j * imgwdt + i + k];

				sumc += cpx;
				sumcc += cpx * cpx;
				sumrc += rfx * cpx;

			}
		}
		sumsq = sumrr + sumcc - 2 * sumrc - (float)(sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;

		// 右画像を1画素ずつ右へ動かしてSSDを計算する
		// 一番小さいSSDのとき，最も類似している
		ssd[k] = sumsq;
		if (sumsq < misum) {
			misum = sumsq;
			disp = k;
		}
	}

	// 視差値が探索開始スキップ幅：1未満の場合はサブピクセルを算出しない
	// 視差値が探索幅の上限に達していた場合はサブピクセルを算出しない
	// 視差値が1未満の場合は視差値ゼロにする
	// 視差値が探索幅の上限に達していた場合は視差値ゼロにする
	if (disp < 1 || disp >= (depth - 1)) {
		pblkdsp[jblk * imgwdtblk + iblk] = 0.0;
	}
	else {
		// サブピクセル推定
		// 折れ線近似
		// Xsub = (S(-1) - S(1)) / (2 x S(-1) - 2 x S(0)) S(-1) > S(1)
		// Xsub = (S(1) - S(-1)) / (2 x S(1) - 2 x S(0)) S(-1) ≦ S(1)
		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		// 折れ線+放物線近似
		// Xsub = (S(-1) - S(1)) / (S(-1) - S(0) - S(-1) + S(2))
		// Xsub = (S(1) - S(-1)) / (S(-2) - S(-1) - S(0) + S(1))
		float sub;
		float ssdprv = ssd[disp - 1];
		float ssdcnt = ssd[disp];
		float ssdnxt = ssd[disp + 1];

		// 放物線近似
		sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);

		pblkdsp[jblk * imgwdtblk + iblk] = disp + sub;

	}

	// コントラストを保存する
	pblkcrst[jblk * imgwdtblk + iblk] = crst;

}


/// <summary>
/// バンド内の両方向の視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param> 
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
void StereoMatching::getBothDisparityInBand(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp, int *pblkcrst,
	int jstart, int jend)
{

	// jpx : マッチングブロックのy座標
	// ipx : マッチングブロックのx座標
	for (int jpx = jstart; jpx < jend && jpx <= (imghgt - blkhgt); jpx++) {
		for (int ipx = 0; ipx <= (imgwdt - blkwdt); ipx++) {
			// SSDにより視差値を求める
			getBothDisparityBySSD(ipx, jpx, imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				pimgref, pimgcmp, pblkdsp, pblkbkdsp, pblkcrst);
		}
	}

}


/// <summary>
/// 両方向のステレオマッチングにより視差値を求める
/// </summary>
/// <param name="x">ブロック左上画素x座標(IN)</param>
/// <param name="y">ブロック左上画素y座標(IN)</param>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="bgtmax">ブロック輝度最大値(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::getBothDisparityBySSD(int x, int y, int imghgt, int imgwdt, int depth,
	int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp,
	float * pblkdsp, float * pblkbkdsp, int *pblkcrst)
{

	// 視差画像の幅
	int imgwdtdsp = imgwdt - blkwdt;
	int jpx = y;
	int ipx = x;
	if (ipx >= imgwdtdsp || jpx % stphgt != 0 || ipx % stpwdt != 0) {
		return;
	}

	// jblk : マッチングブロックのyインデックス
	// iblk : マッチングブロックのxインデックス
	int jblk = jpx / stphgt;
	int iblk = ipx / stpwdt;

	// ブロック輝度最大値の最小値
	int bgtmax = BLOCK_BRIGHTNESS_MAX;
	// 高解像度モードのコントラスト計算のための平均輝度
	int crstave = CONTRAST_CALC_AVERAGE_FOR_HIGH_RESO;
	// ブロック平均輝度最小値
	double minlave = BLOCK_MIN_AVERAGE_BRIGHTNESS;
	// ブロック内の輝度差の最小値
	int mindltl = BLOCK_MIN_DELTA_BRIGHTNESS;

	// SSDを保存する配列
	// 配列サイズはマッチング探索幅
	float ssd[ISC_IMG_DEPTH_MAX];
	float bk_ssd[ISC_IMG_DEPTH_MAX];
	// SSD最大値を設定
	float misum = FLT_MAX;
	float bk_misum = FLT_MAX;

	// 視差値の初期値を設定する
	int disp = 0;
	int bk_disp = 0;

	// マッチング評価関数
	// 左右画像の同じ位置iにある輝度の差の二乗を求め，それを領域全体に渡って足し合わせる
	// SSD = Σ(((右画像の輝度ij - 右画像ブロック輝度平均) - (左画像の輝度ij - 左画像ブロック輝度平均))^2) 
	// 右画像を1画素ずつ探索範囲DEPTHまで右へ動かしてSSDを計算する
	// 一番小さいSSDのとき，最も類似している
	// その探索位置を視差値（整数部）とする
	// 前後のSSDからサブピクセル値（少数部）を算出する
	int jpxe = jpx + blkhgt;
	int ipxe = ipx + blkwdt;

	// フォアマッチング
	int sumr = 0; // Σ基準画像の輝度ij
	int sumrr = 0; // Σ(基準画像の輝度ij^2)
	double Lsumr = 0.0; // Σ基準画像の輝度ij
	double Lmin = 255.0; // ブロック内輝度最小値（初期値256階調最大輝度値）
	double Lmax = 0.0; // ブロック内輝度最大値（初期値256階調最小輝度値）

	// バックマッチング
	int bk_sumr = 0; // Σ基準画像の輝度ij
	int bk_sumrr = 0; // Σ(基準画像の輝度ij^2)
	double bk_Lsumr = 0.0; // Σ基準画像の輝度ij
	double bk_Lmin = 255.0; // ブロック内輝度最小値（初期値256階調最大輝度値）
	double bk_Lmax = 0.0; // ブロック内輝度最大値（初期値256階調最小輝度値）

	int blkcnt = blkhgt * blkwdt;

	// 基準画像ブロック輝度を評価する
	for (int j = jpx; j < jpxe; j++) {
		for (int i = ipx; i < ipxe; i++) {

			double rfx = (double)pimgref[j * imgwdt + i]; // 基準（右）カメラ
			double xrfx = rfx * rfx;
			double cpx = (double)pimgcmp[j * imgwdt + i]; // 比較（左）カメラ
			double xcpx = cpx * cpx;

			// フォアマッチング
			sumr += (int)rfx;
			sumrr += (int)xrfx;
			// バックマッチング
			bk_sumr += (int)cpx;
			bk_sumrr += (int)xcpx;

			// 階調補正モードオンの場合
			// 補正前の値へ変換する
			if (grdcrct == 1) {
				rfx = xrfx / 255;
				cpx = xcpx / 255;
			}
			// フォアマッチング
			Lsumr += rfx;
			if (Lmin > rfx) {
				Lmin = rfx;
			}
			if (Lmax < rfx) {
				Lmax = rfx;
			}
			// バックマッチング
			bk_Lsumr += cpx;
			if (cpx > bk_Lmax) {
				bk_Lmax = cpx;
			}
			else if (cpx < bk_Lmin) {
				bk_Lmin = cpx;
			}

		}
	}

	// コントラストを求める
	// フォアマッチング
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
			crst = (int)((deltaL * 1000 - crstofs) / Lave);
		}

	// バックマッチング
	int bk_crst = 0;

	// ブロックの輝度値の平均
	double bk_Lave = bk_Lsumr / blkcnt;
	// ブロック内の輝度差
	double bk_deltaL = bk_Lmax - bk_Lmin;

	if (bk_Lmax >= (double)bgtmax && bk_deltaL >= (double)mindltl && bk_Lave >= minlave) {
			bk_crst = (int)((bk_deltaL * 1000 - crstofs) / bk_Lave);
		}

	// 探索マージンを求める
	// フォアマッチング
	int fr_mrgn = imgwdt - (ipx + depth + blkwdt);
	// バックマッチング
	int bk_mrgn = ipx - depth;
	// 探索幅を求める
	// フォワード
	int fr_depth = depth;
	if (fr_mrgn < 0) {
		fr_depth = depth + fr_mrgn + 1;
	}
	// バックワード探索幅
	int bk_depth = depth;
	if (bk_mrgn < 0) {
		bk_depth = depth + bk_mrgn + 1;
	}

	for (int k = 0; k < depth; k++) {

		// フォアマッチング
		int sumc = 0; // Σ比較画像の輝度ij
		int sumcc = 0; // Σ(比較画像の輝度ij^2)
		int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
		// バックマッチング
		int bk_sumc = 0; // Σ比較画像の輝度ij
		int bk_sumcc = 0; // Σ(比較画像の輝度ij^2)
		int bk_sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)

		for (int j = jpx; j < jpxe; j++) {
			for (int i = ipx; i < ipxe; i++) {
				// フォアマッチング
				if (k < fr_depth) {
					int rfx = pimgref[j * imgwdt + i];
					int cpx = pimgcmp[j * imgwdt + i + k];
					sumc += cpx;
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
				// バックマッチング
				if (k < bk_depth) {
					int rfx = pimgcmp[j * imgwdt + i];
					int cpx = pimgref[j * imgwdt + i - k];
					bk_sumc += cpx;
					bk_sumcc += cpx * cpx;
					bk_sumrc += rfx * cpx;
				}
			}
		}
		// フォアマッチング
		float sumsq = sumrr + sumcc - 2 * sumrc - (float)(sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
		// バックマッチング
		float bk_sumsq = bk_sumrr + bk_sumcc - 2 * bk_sumrc
			- (float)(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;

		// 右画像を1画素ずつ右へ動かしてSSDを計算する
		// 一番小さいSSDのとき，最も類似している
		// フォアマッチング
		ssd[k] = sumsq;
		if (sumsq < misum) {
			misum = sumsq;
			disp = k;
		}
		// バックマッチング
		bk_ssd[k] = bk_sumsq;
		if (bk_sumsq < bk_misum) {
			bk_misum = bk_sumsq;
			bk_disp = k;
		}
	}

	// 視差値が探索開始スキップ幅：1未満の場合はサブピクセルを算出しない
	// 視差値が探索幅の上限に達していた場合はサブピクセルを算出しない
	// 視差値が1未満の場合は視差値ゼロにする
	// 視差値が探索幅の上限に達していた場合は視差値ゼロにする
	// コントラストが閾値未満の場合は視差値ゼロにする
	float sub;
	float ssdprv;
	float ssdcnt;
	float ssdnxt;
	// フォアマッチング
	if (fr_depth < 3 || disp < 1 || disp >= (fr_depth - 1) || crst < crstthr) {
		pblkdsp[jblk * imgwdtblk + iblk] = 0.0;
		crst = 0;
	}
	else {
		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		ssdprv = ssd[disp - 1];
		ssdcnt = ssd[disp];
		ssdnxt = ssd[disp + 1];
		// 放物線近似
		sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);
		// 視差値を保存する
		pblkdsp[jblk * imgwdtblk + iblk] = disp + sub;
	}
	// バックマッチング
	if (bk_depth >= 3 && bk_disp >= 1 && bk_disp < (bk_depth - 1) && bk_crst >= crstthr) {
		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		ssdprv = bk_ssd[bk_disp - 1];
		ssdcnt = bk_ssd[bk_disp];
		ssdnxt = bk_ssd[bk_disp + 1];
		// 放物線近似
		sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);
		float bk_disp_sub = bk_disp + sub;
		// バックマッチングの結果を基準画像の座標へ展開
		// 視差ブロック番号
		int bk_iblk = (int)((ipx - bk_disp_sub) / stpwdt);
		// 視差値を保存する
		pblkbkdsp[jblk * imgwdtblk + bk_iblk] = bk_disp_sub;
	}

	// コントラストを保存する
	pblkcrst[jblk * imgwdtblk + iblk] = crst;

}


/// <summary>
/// 両方向のマッチングの視差を合成する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="bkevlwdt">バックマッチング視差評価領域幅（片側）(IN)</param>
/// <param name="bkevlrng">バックマッチング視差評価視差値幅(IN)</param>
/// <param name="bkvldrt">バックマッチング評価視差正当率（％）(IN)</param>
/// <param name="bkzrrt">バックマッチング評価視差ゼロ率（％）(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(IN/OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(IN)</param>
void StereoMatching::blendBothMatchingDisparity(int imghgt, int imgwdt, int imghgtblk, int imgwdtblk, 
	int bkevlwdt, int bkevlrng, int bkvldrt, int bkzrrt, float *pblkdsp, float *pblkbkdsp)
{

	// バックマッチング視差評価ブロック数
	int bkevlblk = (bkevlwdt * 2 + 1) * (bkevlwdt * 2 + 1);
	// バックマッチング評価視差正当ブロック数
	int bkvldnum = (int)((bkevlblk * bkvldrt) / 100);
	// バックマッチング評価視差ゼロブロック数
	int	bkzrnum = (int)((bkevlblk * bkzrrt) / 100);

	for (int jd = 0; jd < imghgtblk; jd++) {
		for (int id = 0; id < imgwdtblk; id++) {

			if (jd < bkevlwdt || jd >= imghgtblk - bkevlwdt || id < bkevlwdt || id >= imgwdtblk - bkevlwdt) {
				pblkdsp[jd * imgwdtblk + id] = 0.0;
				continue;
			}

			float disp = pblkdsp[jd * imgwdtblk + id];
			if (disp != 0.0) {
				int bk_zrcnt = 0;
				int bk_evlcnt = 0;
				for (int j = jd - bkevlwdt; j <= jd + bkevlwdt; j++) {
					for (int i = id - bkevlwdt; i <= id + bkevlwdt; i++) {
						float bk_disp = pblkbkdsp[j * imgwdtblk + i];
						float dispdiff = abs(bk_disp - disp);
						// 視差値ゼロをカウントする
						if (bk_disp == 0.0) {
							bk_zrcnt++;
						}
						// 指定差以下の視差をカウントする
						if (dispdiff <= (float)bkevlrng) {
							bk_evlcnt++;
						}
					}
				}
				// 視差値ゼロの数が指定以上の場合は視差値ゼロにする
				if (bk_zrcnt >= bkzrnum) {
					pblkdsp[jd * imgwdtblk + id] = 0.0;
				}
				// 範囲内視差差の数が指定以上の場合は視差を残す
				// 指定未満の場合は視差値ゼロにする
				else if (bk_evlcnt < bkvldnum) {
					pblkdsp[jd * imgwdtblk + id] = 0.0;
				}
			}
		}
	}

}


/// <summary>
///  視差をブロックから画素へ展開する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="shdwdt">遮蔽幅(IN)</param>
/// <param name="stphgt">視差ブロックの高さ(IN)</param>
/// <param name="stpwdt">視差ブロックの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="dspofsx">視差ブロック横オフセット(IN)</param>
/// <param name="dspofsy">視差ブロック縦オフセット(IN)</param>
/// <param name="pblkdsp">ブロックごとの視差値(IN)</param>
/// <param name="ppxldsp">画素ごとの視差値(OUT)</param>
/// <param name="ppxlsub">画素ごとのサブピクセル視差値(OUT)</param>
void StereoMatching::spreadDisparityImage(int imghgt, int imgwdt, int depth, int shdwdt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int dspofsx, int dspofsy,
	float * pblkdsp, unsigned char * ppxldsp, float *ppxlsub)
{

	// 出力視差ブロック画像の高さ
	int imghgtblk = imghgt / stphgt;
	int dsphgtblk = (imghgt - blkhgt - dspofsy) / stphgt + 1;
	// 出力視差ブロック画像の幅
	int imgwdtblk = imgwdt / stpwdt;
	int dspwdtblk = (imgwdt - shdwdt - blkwdt - dspofsx) / stpwdt + 1;

	float dsprt = (float)255 / depth; // 視差画像表示のため視差値を256階調へ変換する

	// jblk : 視差ブロックのyインデックス
	// iblk : 視差ブロックのxインデックス
	for (int jblk = 0; jblk < dsphgtblk; jblk++) {
		for (int iblk = 0; iblk < dspwdtblk; iblk++) {

			// jpxl : 視差ブロックのy座標
			// ipxl : 視差ブロックのx座標
			int jpxl = jblk * stphgt + dspofsy;
			int ipxl = iblk * stpwdt + dspofsx;

			float disp = pblkdsp[jblk * imgwdtblk + iblk];

			// 視差をブロックから画素へ展開する
			for (int j = jpxl; j < jpxl + stphgt; j++) {
				for (int i = ipxl; i < ipxl + stpwdt; i++) {
					ppxldsp[j * imgwdt + i] = (unsigned char)(disp * dsprt);
					ppxlsub[j * imgwdt + i] = disp;
				}
			}
		}
	}

}


/// <summary>
/// ステレオマッチングを実行する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">ブロックごとの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::executeMatchingOpenCL(int imghgt, int imgwdt, int depth,
	unsigned char *pimgref, unsigned char *pimgcmp,
	float * pblkdsp, int *pblkcrst)
{

	// 視差ブロックの高さ
	int stphgt = disparityBlockHeight;
	// 視差ブロックの幅
	int stpwdt = disparityBlockWidth;
	// マッチングブロックの高さ
	int blkhgt = matchingBlockHeight;
	// マッチングブロックの幅
	int blkwdt = matchingBlockWidth;
	// 出力視差ブロック画像の高さ
	int imghgtblk = imghgt / stphgt;
	// 出力視差ブロック画像の幅
	int imgwdtblk = imgwdt / stpwdt;

	// コントラスト閾値
	int crstthr = contrastThreshold;
	// 階調補正モードステータス
	int grdcrct = gradationCorrectionMode;

	// コントラストオフセット
	int crstofs = 0;
	if (imgwdt == IMG_WIDTH_VM) {
		crstofs = (int)CONTRAST_OFFSET_VM;
	}
	else if (imgwdt == IMG_WIDTH_XC) {
		crstofs = (int)CONTRAST_OFFSET_XC;
		// 階調補正モードの場合
		if (grdcrct == 1) {
			crstofs = (int)(CONTRAST_OFFSET_XC * CONTRAST_XC_DRADATION_FACTOR);
		}
	}

	// 入力補正画像データのMatを生成する
	cv::Mat inputImageRef(imghgt, imgwdt, CV_8UC1, pimgref);
	cv::Mat inputImageCmp(imghgt, imgwdt, CV_8UC1, pimgcmp);
	// 出力視差画像データのMatを生成する
	// 視差値出力はブロックごと
	// サイズを最大画像サイズにしおく
	cv::Mat outputDisp(imghgt, imgwdt, CV_32FC1, pblkdsp);
	cv::Mat outputCrst(imghgt, imgwdt, CV_32SC1, pblkcrst);

	// 入力補正画像データのUMatを生成する
	cv::UMat inputUMatImageRef(imghgt, imgwdt, CV_8UC1);
	cv::UMat inputUMatImageCmp(imghgt, imgwdt, CV_8UC1);
	// 出力視差画像データのMatを生成する
	cv::UMat outputUMatDisp(imghgt, imgwdt, CV_32FC1);
	cv::UMat outputUMatCrst(imghgt, imgwdt, CV_32SC1);

	// 入力補正画像データをUMatへコピーする
	inputImageRef.copyTo(inputUMatImageRef);
	inputImageCmp.copyTo(inputUMatImageCmp);

	if (enableBackMatching == 0) {
		shadeWidth = depth;

		// SSDにより視差値を求める
		getDisparityBySSDOpenCL(imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			inputUMatImageRef, inputUMatImageCmp, outputUMatDisp, outputUMatCrst);
		// 出力視差データをMatへコピーする
		outputUMatDisp.copyTo(outputDisp);
		// 出力視差マスクをMatへコピーする
		outputUMatCrst.copyTo(outputCrst);

	}
	else {
		shadeWidth = 0;

		float *pblkbkdsp = bk_block_dsp;
		// バックマッチング出力視差画像データのMatを生成する
		cv::Mat outputBkDisp(imghgt, imgwdt, CV_32FC1, pblkbkdsp);
		// 出力視差画像データのMatを生成する
		cv::UMat outputUMatBkDisp(imghgt, imgwdt, CV_32FC1, cv::Scalar(0));

		// 両方向マッチングによる視差を取得する
		getBothDisparityBySSDOpenCL(imghgt, imgwdt, depth, crstthr, crstofs, grdcrct,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			inputUMatImageRef, inputUMatImageCmp, outputUMatDisp, outputUMatBkDisp, outputUMatCrst);

		// 出力視差データをMatへコピーする
		outputUMatDisp.copyTo(outputDisp);
		// バックマッチング出力視差データをMatへコピーする
		outputUMatBkDisp.copyTo(outputBkDisp);
		// 出力視差マスクをMatへコピーする
		outputUMatCrst.copyTo(outputCrst);

		blendBothMatchingDisparity(imghgt, imgwdt, imghgtblk, imgwdtblk,
			backMatchingEvaluationWidth, backMatchingEvaluationRange, backMatchingValidRatio, backMatchingZeroRatio,
			pblkdsp, pblkbkdsp);

	}

}


/// <summary>
/// SSDで視差値を算出するカーネルプログラム
/// </summary>
/// <remarks>最大探索幅 : ISC_IMG_DEPTH_MAX 512</remarks>
/// <remarks>SSD最大値 : 268435456 256階調64x64画素ブロック</remarks>
/// <remarks>ブロック輝度最大値の最小値 : BLOCK_BRIGHTNESS_MAX 15</remarks>
/// <remarks>ブロック内の輝度差の最小値 : BLOCK_MIN_DELTA_BRIGHTNESS 2</remarks>
/// <remarks>ブロック平均輝度最小値 : BLOCK_MIN_AVERAGE_BRIGHTNESS 7.5</remarks>
/// <remarks>高解像度モードのコントラスト計算のための平均輝度：CONTRAST_CALC_AVERAGE_FOR_HIGH_RESO 255</remarks>
static char *kernelGetDisparityBySSD = (char*)"__kernel void kernelGetDisparityBySSD(\n\
	int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,\n\
	int stphgt, int	stpwdt, int blkhgt, int blkwdt,\n\
	int imghgtblk, int imgwdtblk,\n\
	__global uchar* imgref, int imgref_step, int imgref_offset,\n\
	__global uchar* imgcmp, int imgcmp_step, int imgcmp_offset,\n\
	__global float* blkdsp, int blkdsp_step, int blkdsp_offset,\n\
	int height, int width,\n\
	__global int* blkcrst, int blkcrst_step, int blkcrst_offset,\n\
	int mskhgt, int mskwdt)\n\
{\n\
	int x = get_global_id(0);\n\
	int y = get_global_id(1);\n\
	if (x >= width || y >= height) {\n\
		return; \n\
	}\n\
	int imgwdtdsp = imgwdt - depth - blkwdt;\n\
	int jpx = y;\n\
	int ipx = x;\n\
	if (ipx > imgwdtdsp || jpx > (imghgt - blkhgt) || jpx % stphgt != 0 || ipx % stpwdt != 0) {\n\
		return;\n\
	}\n\
	int jblk = jpx / stphgt;\n\
	int iblk = ipx / stpwdt;\n\
	float ssd[512];\n\
	float misum = (float)(268435456);\n\
	int disp = 0;\n\
	int jpxe = jpx + blkhgt;\n\
	int ipxe = ipx + blkwdt;\n\
	int sumr = 0;\n\
	int sumrr = 0;\n\
	double Lsumr = 0.0;\n\
	double Lmin = 255.0;\n\
	double Lmax = 0.0;\n\
	int blkcnt = blkhgt * blkwdt;\n\
	for (int j = jpx; j < jpxe; j++) {\n\
		for (int i = ipx; i < ipxe; i++) {\n\
			double rfx = (double)imgref[j * imgwdt + i];\n\
			double xrfx = rfx * rfx;\n\
			sumr += (int)rfx;\n\
			sumrr += (int)xrfx;\n\
			if (grdcrct == 1) {\n\
				rfx = xrfx / 255;\n\
			}\n\
			Lsumr += rfx;\n\
			if (Lmin > rfx) {\n\
				Lmin = rfx; \n\
			}\n\
			if (Lmax < rfx) {\n\
				Lmax = rfx; \n\
			}\n\
		}\n\
	}\n\
	int crst = 0;\n\
	double Lave = Lsumr / blkcnt;\n\
	double deltaL = Lmax - Lmin;\n\
	if (Lmax >= 15.0 && deltaL >= 2.0 && Lave >= 7.5) {\n\
			crst = (deltaL * 1000 - crstofs) / Lave;\n\
		}\n\
	if (crst < crstthr) {\n\
		blkdsp[jblk * imgwdtblk + iblk] = 0.0;\n\
		blkcrst[jblk * imgwdtblk + iblk] = 0;\n\
		return;\n\
	}\n\
	for (int k = 0; k < depth; k++) {\n\
		int sumc = 0;\n\
		int sumcc = 0;\n\
		int sumrc = 0;\n\
		for (int j = jpx; j < jpxe; j++) {\n\
			for (int i = ipx; i < ipxe; i++) {\n\
				int rfx = imgref[j * imgwdt + i];\n\
				int cpx = imgcmp[j * imgwdt + i + k];\n\
				sumc += cpx;\n\
				sumcc += cpx * cpx;\n\
				sumrc += rfx * cpx;\n\
			}\n\
		}\n\
		float sumsq = sumrr + sumcc - 2 * sumrc - (float)(sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
		ssd[k] = sumsq;\n\
		if (sumsq < misum) {\n\
			misum = sumsq;\n\
			disp = k;\n\
		}\n\
	}\n\
	if (disp < 1 || disp >= (depth - 1)) {\n\
		blkdsp[jblk * imgwdtblk + iblk] = 0.0;\n\
	}\n\
	else {\n\
		float sub;\n\
		float ssdprv = ssd[disp - 1];\n\
		float ssdcnt = ssd[disp];\n\
		float ssdnxt = ssd[disp + 1];\n\
		sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);\n\
		blkdsp[jblk * imgwdtblk + iblk] = disp + sub;\n\
	}\n\
	blkcrst[jblk * imgwdtblk + iblk] = crst;\n\
}"; 


/// <summary>
/// OpenCLコンテキストの初期化フラグ
/// </summary>
static bool openCLMatchingContextInit = false;

/// <summary>
/// OpenCLコンテキスト
/// </summary>
static cv::ocl::Context contextMatching;

/// <summary>
/// カーネルプログラム
/// </summary>
static cv::ocl::Program kernelProgramMatching;

/// <summary>
/// カーネルオブジェクト
/// </summary>
static cv::ocl::Kernel kernelObjectMatching;

/// <summary>
/// glaobalWorkSize
/// </summary>
static size_t globalSizeMatching[2];


/// <summary>
/// SSDにより視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="imgref">入力基準画像データUMat(IN)</param>
/// <param name="imgcmp">入力比較画像データUMat(IN)</param>
/// <param name="blkdsp">マッチングの視差値UMat(OUT)</param>
/// <param name="blkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::getDisparityBySSDOpenCL(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	cv::UMat imgref, cv::UMat imgcmp, cv::UMat blkdsp, cv::UMat blkcrst)
{

	bool success;

	// OpenCLのコンテキストを初期化する
	if (openCLMatchingContextInit == false) {

		// コンテキストを生成する
		// * コンテキストContextを保持する？ <<<<<<　Context　スタティック
		// clCreateContext
		// context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
		success = contextMatching.create(cv::ocl::Device::TYPE_GPU);

		if (success == false) {
			OutputDebugString(_T("FALSE : context.create()\n"));
		}

		// デバイスを選択する
		// clGetDeviceIDs
		// clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
		cv::ocl::Device(contextMatching.device(0));

		// カーネルソースを生成する
		// clCreateProgramWithSource
		// command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
		// program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
		cv::ocl::ProgramSource programSource(kernelGetDisparityBySSD);

		cv::String errMsg;

		cv::String bldOpt = "";

		// カーネルプログラムをビルドする
		// * Programプログラムを保持する <<<<<<
		// clBuildProgram
		// ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
		kernelProgramMatching = contextMatching.getProg(programSource, bldOpt, errMsg);

		if (!errMsg.empty()) {
			cv::String msg;

			msg = "Compile Error has occurred.\n";
			msg += errMsg;
			msg += "\n";

			OutputDebugStringA(msg.c_str());

		}

		// カーネルオブジェクトを生成する
		// * Kernelカーネルオブジェクトを保持する <<<<<< new する
		// clCreateKernel
		// kernel = clCreateKernel(program, "matrix_dot_matrix", &ret);
		kernelObjectMatching.create("kernelGetDisparityBySSD", kernelProgramMatching);

		// OpenCL初期化フラグをセットする
		openCLMatchingContextInit = true;

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
	kernelObjectMatching.args(
		imghgt, // 1:入力補正画像の高さ
		imgwdt, // 2:入力補正画像の幅
		depth, // 3:マッチング探索幅
		crstthr, // 4:コントラスト閾値
		crstofs, // 5:コントラストオフセット
		grdcrct, // 6:階調補正モードステータス
		stphgt, // 7:マッチングステップの高さ
		stpwdt, // 8:マッチングステップの幅
		blkhgt, // 9:マッチングブロックの高さ
		blkwdt, // 10:マッチングブロックの幅
		imghgtblk, // 11:視差ブロック画像の高さ
		imgwdtblk, // 12:視差ブロック画像の幅
		cv::ocl::KernelArg::ReadOnlyNoSize(imgref),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmp),
		cv::ocl::KernelArg::ReadWrite(blkdsp),
		cv::ocl::KernelArg::ReadWrite(blkcrst)
	);

	// globalWorkSize : 行いたい処理の総数
	// localWorkSize : 並列に行いたい処理の数
	globalSizeMatching[0] = (size_t)imgref.cols;
	globalSizeMatching[1] = (size_t)imgcmp.rows;

	// カーネルを実行する
	// bool cv::ocl::Kernel::run(int dims, size_t globalsize[], size_t localsize[], bool sync, const Queue & q = Queue())
	//
	// clEnqueueTask
	// ret = clEnqueueNDRangeKernel(command_queue, kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	// ret = clEnqueueReadBuffer(command_queue, matrixRMemObj, CL_TRUE, 0, matrixRMemSize, matrixR, 0, NULL, NULL)
	// 
	success = kernelObjectMatching.run(2, globalSizeMatching, NULL, true);

	if (success == false) {
		OutputDebugString(_T("FALSE : kernel.run()\n"));
	}

}


/// <summary>
/// 両方向マッチングで視差値を算出するカーネルプログラム
/// </summary>
/// <remarks>最大探索幅 ISC_IMG_DEPTH_MAX 512</remarks>
/// <remarks>SSD最大値 268435456 256階調64x64画素ブロック</remarks>
/// <remarks>ブロック輝度最大値の最小値 : BLOCK_BRIGHTNESS_MAX 15</remarks>
/// <remarks>ブロック内の輝度差の最小値 : BLOCK_MIN_DELTA_BRIGHTNESS 2</remarks>
/// <remarks>ブロック平均輝度最小値 : BLOCK_MIN_AVERAGE_BRIGHTNESS 7.5</remarks>
/// <remarks>高解像度モードのコントラスト計算のための平均輝度：CONTRAST_CALC_AVERAGE_FOR_HIGH_RESO 255</remarks>
static char *kernelGetBothDisparityBySSD = (char*)"__kernel void kernelGetBothDisparityBySSD(\n\
	int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,\n\
	int stphgt, int	stpwdt, int blkhgt, int blkwdt,\n\
	int imghgtblk, int imgwdtblk,\n\
	__global uchar* imgref, int imgref_step, int imgref_offset,\n\
	__global uchar* imgcmp, int imgcmp_step, int imgcmp_offset,\n\
	__global float* blkdsp, int blkdsp_step, int blkdsp_offset,\n\
	int height, int width,\n\
	__global float* blkbkdsp, int blkbkdsp_step, int blkbkdsp_offset,\n\
	int bkheight, int bkwidth,\n\
	__global int* blkcrst, int blkcrst_step, int blkcrst_offset,\n\
	int mskhgt, int mskwdt)\n\
{\n\
	int x = get_global_id(0);\n\
	int y = get_global_id(1);\n\
	if (x >= width || y >= height) {\n\
		return; \n\
	}\n\
	int jpx = y;\n\
	int ipx = x;\n\
	if (ipx > (imgwdt - blkwdt) || jpx > (imghgt - blkhgt) || jpx % stphgt != 0 || ipx % stpwdt != 0) {\n\
		return;\n\
	}\n\
	int jblk = jpx / stphgt;\n\
	int iblk = ipx / stpwdt;\n\
	float ssd[512];\n\
	float bk_ssd[512];\n\
	float misum = (float)(268435456);\n\
	float bk_misum = (float)(268435456);\n\
	int disp = 0;\n\
	int bk_disp = 0;\n\
	int jpxe = jpx + blkhgt;\n\
	int ipxe = ipx + blkwdt;\n\
	int sumr = 0;\n\
	int sumrr = 0;\n\
	double Lsumr = 0.0;\n\
	double Lmin = 255.0;\n\
	double Lmax = 0.0;\n\
	int bk_sumr = 0;\n\
	int bk_sumrr = 0;\n\
	double bk_Lsumr = 0.0;\n\
	double bk_Lmin = 255.0;\n\
	double bk_Lmax = 0.0;\n\
	int blkcnt = blkhgt * blkwdt;\n\
	for (int j = jpx; j < jpxe; j++) {\n\
		for (int i = ipx; i < ipxe; i++) {\n\
			double rfx = (double)imgref[j * imgwdt + i];\n\
			double cpx = (double)imgcmp[j * imgwdt + i];\n\
			double xrfx = rfx * rfx;\n\
			double xcpx = cpx * cpx;\n\
			sumr += (int)rfx; \n\
			sumrr += (int)xrfx; \n\
			bk_sumr += (int)cpx;\n\
			bk_sumrr += (int)xcpx;\n\
			if (grdcrct == 1) {\n\
				rfx = xrfx / 255;\n\
				cpx = xcpx / 255;\n\
			}\n\
			Lsumr += rfx;\n\
			bk_Lsumr += cpx;\n\
			if (Lmin > rfx) {\n\
				Lmin = rfx; \n\
			}\n\
			if (Lmax < rfx) {\n\
				Lmax = rfx; \n\
			}\n\
			if (cpx > bk_Lmax) {\n\
				bk_Lmax = cpx;\n\
			}\n\
			if (cpx < bk_Lmin) {\n\
				bk_Lmin = cpx;\n\
			}\n\
		}\n\
	}\n\
	int crst = 0;\n\
	double Lave = Lsumr / blkcnt;\n\
	double deltaL = Lmax - Lmin;\n\
	if (Lmax >= 15.0 && deltaL >= 2.0 && Lave >= 7.5) {\n\
			crst = (deltaL * 1000 - crstofs) / Lave;\n\
		}\n\
	int bk_crst = 0;\n\
	double bk_Lave = bk_sumr / blkcnt;\n\
	double bk_deltaL = bk_Lmax - bk_Lmin;\n\
	if (bk_Lmax >= 15.0 && bk_deltaL >= 2.0 && bk_Lave >= 7.5) {\n\
			bk_crst = (bk_deltaL * 1000 - crstofs) / Lave;\n\
		}\n\
	int fr_mrgn = imgwdt - (ipx + depth + blkwdt);\n\
	int bk_mrgn = ipx - depth;\n\
	int fr_depth = depth;\n\
	if (fr_mrgn < 0) {\n\
		fr_depth = depth + fr_mrgn + 1;\n\
	}\n\
	int bk_depth = depth;\n\
	if (bk_mrgn < 0) {\n\
		bk_depth = depth + bk_mrgn + 1;\n\
	}\n\
	for (int k = 0; k < depth; k++) {\n\
		int sumc = 0;\n\
		int sumcc = 0;\n\
		int sumrc = 0;\n\
		int bk_sumc = 0;\n\
		int bk_sumcc = 0;\n\
		int bk_sumrc = 0;\n\
		for (int j = jpx; j < jpxe; j++) {\n\
			for (int i = ipx; i < ipxe; i++) {\n\
				if (k < fr_depth) {\n\
					int rfx = imgref[j * imgwdt + i];\n\
					int cpx = imgcmp[j * imgwdt + i + k];\n\
					sumc += cpx;\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
				if (k < bk_depth) {\n\
					int rfx = imgcmp[j * imgwdt + i];\n\
					int cpx = imgref[j * imgwdt + i - k];\n\
					bk_sumc += cpx;\n\
					bk_sumcc += cpx * cpx;\n\
					bk_sumrc += rfx * cpx;\n\
				}\n\
			}\n\
		}\n\
		float sumsq = sumrr + sumcc - 2 * sumrc - (float)(sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
		float bk_sumsq = bk_sumrr + bk_sumcc - 2 * bk_sumrc\n\
			- (float)(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;\n\
		ssd[k] = sumsq;\n\
		if (sumsq < misum) {\n\
			misum = sumsq;\n\
			disp = k;\n\
		}\n\
		bk_ssd[k] = bk_sumsq;\n\
		if (bk_sumsq < bk_misum) {\n\
			bk_misum = bk_sumsq;\n\
			bk_disp = k;\n\
		}\n\
	}\n\
	float sub;\n\
	float ssdprv;\n\
	float ssdcnt;\n\
	float ssdnxt;\n\
	if (fr_depth < 3 || disp < 1 || disp >= (depth - 1) || crst < crstthr) {\n\
		blkdsp[jblk * imgwdtblk + iblk] = 0.0;\n\
		crst = 0;\n\
	}\n\
	else {\n\
		sub;\n\
		ssdprv = ssd[disp - 1];\n\
		ssdcnt = ssd[disp];\n\
		ssdnxt = ssd[disp + 1];\n\
		sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);\n\
		blkdsp[jblk * imgwdtblk + iblk] = disp + sub;\n\
	}\n\
	if (bk_depth >= 3 && bk_disp >= 1 && bk_disp < (bk_depth - 1) && bk_crst >= crstthr) {\n\
		ssdprv = bk_ssd[bk_disp - 1];\n\
		ssdcnt = bk_ssd[bk_disp];\n\
		ssdnxt = bk_ssd[bk_disp + 1];\n\
		sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);\n\
		float bk_disp_sub = bk_disp + sub;\n\
		int bk_iblk = (int)((ipx - bk_disp_sub) / stpwdt);\n\
		blkbkdsp[jblk * imgwdtblk + bk_iblk] = bk_disp_sub;\n\
	}\n\
	blkcrst[jblk * imgwdtblk + iblk] = crst;\n\
}";


/// <summary>
/// OpenCLコンテキストの初期化フラグ
/// </summary>
static bool openCLBothMatchingContextInit = false;

/// <summary>
/// OpenCLコンテキスト
/// </summary>
static cv::ocl::Context contextBothMatching;

/// <summary>
/// カーネルプログラム
/// </summary>
static cv::ocl::Program kernelProgramBothMatching;

/// <summary>
/// カーネルオブジェクト
/// </summary>
static cv::ocl::Kernel kernelObjectBothMatching;

/// <summary>
/// glaobalWorkSize
/// </summary>
static size_t globalSizeBothMatching[2];


/// <summary>
/// 両方向マッチングにより視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="imgref">入力基準画像データUMat(IN)</param>
/// <param name="imgcmp">入力比較画像データUMat(IN)</param>
/// <param name="blkdsp">マッチングの視差値UMat(OUT)</param>
/// <param name="blkbkdsp">バックマッチングの視差値UMat(OUT)</param>
/// <param name="blkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::getBothDisparityBySSDOpenCL(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	cv::UMat imgref, cv::UMat imgcmp, cv::UMat blkdsp, cv::UMat blkbkdsp, cv::UMat blkcrst)
{

	bool success;

	// OpenCLのコンテキストを初期化する
	if (openCLBothMatchingContextInit == false) {

		// コンテキストを生成する
		// * コンテキストContextを保持する？ <<<<<<　Context　スタティック
		// clCreateContext
		// context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
		success = contextBothMatching.create(cv::ocl::Device::TYPE_GPU);

		if (success == false) {
			OutputDebugString(_T("FALSE : context.create()\n"));
		}

		// デバイスを選択する
		// clGetDeviceIDs
		// clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
		cv::ocl::Device(contextBothMatching.device(0));

		// カーネルソースを生成する
		// clCreateProgramWithSource
		// command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
		// program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
		cv::ocl::ProgramSource programSource(kernelGetBothDisparityBySSD);

		cv::String errMsg;

		cv::String bldOpt = "";

		// カーネルプログラムをビルドする
		// * Programプログラムを保持する <<<<<<
		// clBuildProgram
		// ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
		kernelProgramBothMatching = contextBothMatching.getProg(programSource, bldOpt, errMsg);

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
		kernelObjectBothMatching.create("kernelGetBothDisparityBySSD", kernelProgramBothMatching);

		// OpenCL初期化フラグをセットする
		openCLBothMatchingContextInit = true;

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
	kernelObjectBothMatching.args(
		imghgt, // 1:入力補正画像の高さ
		imgwdt, // 2:入力補正画像の幅
		depth, // 3:マッチング探索幅
		crstthr, // 4:コントラスト閾値
		crstofs, // 5:コントラストオフセット
		grdcrct, // 6:階調補正モードステータス
		stphgt, // 7:マッチングステップの高さ
		stpwdt, // 8:マッチングステップの幅
		blkhgt, // 9:マッチングブロックの高さ
		blkwdt, // 10:マッチングブロックの幅
		imghgtblk, // 11:視差ブロック画像の高さ
		imgwdtblk, // 12:視差ブロック画像の幅
		cv::ocl::KernelArg::ReadOnlyNoSize(imgref),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmp),
		cv::ocl::KernelArg::ReadWrite(blkdsp),
		cv::ocl::KernelArg::ReadWrite(blkbkdsp),
		cv::ocl::KernelArg::ReadWrite(blkcrst)
	);

	// globalWorkSize : 行いたい処理の総数
	// localWorkSize : 並列に行いたい処理の数
	globalSizeBothMatching[0] = (size_t)imgref.cols;
	globalSizeBothMatching[1] = (size_t)imgcmp.rows;

	// カーネルを実行する
	// bool cv::ocl::Kernel::run(int dims, size_t globalsize[], size_t localsize[], bool sync, const Queue & q = Queue())
	//
	// clEnqueueTask
	// ret = clEnqueueNDRangeKernel(command_queue, kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	// ret = clEnqueueReadBuffer(command_queue, matrixRMemObj, CL_TRUE, 0, matrixRMemSize, matrixR, 0, NULL, NULL)
	// 
	success = kernelObjectBothMatching.run(2, globalSizeBothMatching, NULL, true);

	if (success == false) {
		OutputDebugString(_T("FALSE : kernel.run()\n"));
	}

}


/// <summary>
/// ステレオマッチングスレッド
/// </summary>
/// <param name="parg">バンド情報(IN)</param>
/// <returns>処理結果を返す</returns>
UINT  StereoMatching::matchingBandThread(LPVOID parg)
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

		// バンド内の視差を取得する
		if (pBand->pblkbkdsp == NULL) {
			getDisparityInBand(pBand->imghgt, pBand->imgwdt, pBand->depth, pBand->crstthr, pBand->crstofs, pBand->grdcrct,
				pBand->stphgt, pBand->stpwdt, pBand->blkhgt, pBand->blkwdt, pBand->imghgtblk, pBand->imgwdtblk,
				pBand->pimgref, pBand->pimgcmp, pBand->pblkdsp, pBand->pblkcrst, pBand->bandStart, pBand->bandEnd);
		}
		else {
			getBothDisparityInBand(pBand->imghgt, pBand->imgwdt, pBand->depth, pBand->crstthr, pBand->crstofs, pBand->grdcrct,
				pBand->stphgt, pBand->stpwdt, pBand->blkhgt, pBand->blkwdt, pBand->imghgtblk, pBand->imgwdtblk,
				pBand->pimgref, pBand->pimgcmp, pBand->pblkdsp, pBand->pblkbkdsp, pBand->pblkcrst,
				pBand->bandStart, pBand->bandEnd);
		}

		// 視差平均化完了イベントを通知する
		SetEvent(pBand->doneEvent);
	}
	return 0;
}


/// <summary>
/// ステレオマッチングスレッドを生成する
/// </summary>
void StereoMatching::createMatchingThread()
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
			// スレッドを生成する
			bandInfo[i].bandThread = (HANDLE)_beginthreadex(0, 0, matchingBandThread, (LPVOID)&bandInfo[i], 0, 0);
		}
	}
}


/// <summary>
/// マッチングスレッドを破棄する
/// </summary>
void StereoMatching::deleteMatchingThread()
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
/// バンド分割して視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::getBandDisparity(int imghgt, int imgwdt, int depth, int crstthr, int crstofs, int grdcrct,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp, int *pblkcrst)
{

	int i, n;

	// 完了イベントの配列
	HANDLE doneevt[MAX_NUM_OF_BANDS];

	// バンドの高さライン数
	int bndhgt = imghgt / numOfBands;

	for (i = 0, n = 0; i < numOfBands; i++, n += bndhgt) {
		// 入力補正画像の高さ
		bandInfo[i].imghgt = imghgt;
		// 入力補正画像の幅
		bandInfo[i].imgwdt = imgwdt;
		// マッチング探索幅
		bandInfo[i].depth = depth;
		// コントラスト閾値
		bandInfo[i].crstthr = crstthr;
		// コントラストオフセット
		bandInfo[i].crstofs = crstofs;
		// 階調補正モードステータス
		bandInfo[i].grdcrct = grdcrct;

		// マッチングステップの高さ
		bandInfo[i].stphgt = stphgt;
		// マッチングステップの幅
		bandInfo[i].stpwdt = stpwdt;
		// マッチングブロックの高さ
		bandInfo[i].blkhgt = blkhgt;
		// マッチングブロックの幅
		bandInfo[i].blkwdt = blkwdt;
		// 視差ブロック画像の高さ
		bandInfo[i].imghgtblk = imghgtblk;
		// 視差ブロック画像の幅
		bandInfo[i].imgwdtblk = imgwdtblk;
		// 入力基準画像データ
		bandInfo[i].pimgref = pimgref;
		// 入力比較画像データ
		bandInfo[i].pimgcmp = pimgcmp;
		// マッチングの視差値
		bandInfo[i].pblkdsp = pblkdsp;
		// バックマッチングの視差値
		bandInfo[i].pblkbkdsp = pblkbkdsp;
		// ブロックコントラスト
		bandInfo[i].pblkcrst = pblkcrst;

		// バンド開始ライン位置（y座標）
		bandInfo[i].bandStart = n;
		// バンド終了ライン位置（y座標）
		bandInfo[i].bandEnd = n + bndhgt;

	}
	bandInfo[i - 1].bandEnd = imghgt;

	DWORD st;

	for (i = 0; i < numOfBands; i++) {
		// 開始イベントを送信する
		SetEvent(bandInfo[i].startEvent);
		// 完了イベントのハンドルを配列に格納する
		doneevt[i] = bandInfo[i].doneEvent;
	}

	// 全ての完了イベントを待つ
	st = WaitForMultipleObjects(numOfBands, doneevt, TRUE, INFINITE);

	if (st == WAIT_OBJECT_0) {

	}

}

