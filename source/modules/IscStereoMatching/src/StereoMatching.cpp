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
/// ダブルシャッターブロック視差値（サブピクセル精度：浮動小数）の配列（視差ブロックごと）
/// </summary>
static float* dbl_block_dsp;

/// <summary>
/// バックマッチングブロック視差値（サブピクセル精度：浮動小数）の配列（視差ブロックごと）
/// </summary>
static float *bk_block_dsp;

/// <summary>
/// ブロックコントラスト（画素ごと）
/// </summary>
static int *block_crst;

/// <summary>
/// ダブルシャッターブロックコントラスト（画素ごと）
/// </summary>
static int* dbl_block_crst;

/// <summary>
/// 比較ブロックコントラスト（画素ごと）
/// </summary>
static int* cmp_block_crst;

/// <summary>
/// 比較画像の視差位置（画素ごと）
/// </summary>
/// <remarks>重複マッチング検出のため</remarks>
static int *dsp_posi;

/// <summary>
/// 基準画像のブロック輝度値（画素ごと）
/// </summary>
/// <remarks>マッチングスキップのため</remarks>
static int* ref_block_brt;

/// <summary>
/// 比較画像のブロック輝度値（画素ごと）
/// </summary>
/// <remarks>マッチングスキップのため</remarks>
static int *cmp_block_brt;

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
/// マッチングブロック最低輝度比率(%)
/// </summary>
static int matchingMinBrightRatio = 85;

/// <summary>
/// コントラスト閾値
/// </summary>
static int contrastThreshold = 40;

/// <summary>
/// 重複マッチング除去：0:しない 1:する
/// </summary>
static int removeDuplicateMatching = 0;

/// <summary>
/// 階調補正モードステータス 0:オフ 1:オン
/// </summary>
static int gradationCorrectionMode = 0;

/// <summary>
/// 拡張マッチング 0:しない 1:する
/// </summary>
static int matchingExtension = 0;

/// <summary>
/// 拡張マッチング探索制限幅
/// </summary>
static int matchingExtLimitWidth = 10;

/// <summary>
/// 拡張マッチング信頼限界
/// </summary>
static int matchingExtConfidenceLimit = 20;


// 画像サイズ
#define IMG_WIDTH_VM 752
#define IMG_WIDTH_XC 1280
#define IMG_WIDTH_2K 1920
#define IMG_WIDTH_4K 3840

// コントラストオフセット
// 入力された画像サイズによってオフセットを選択する
// コントラスト⊿L/Lmeanを1000倍して評価値とする
#define CONTRAST_OFFSET_VM (1.8 * 1000)
#define CONTRAST_OFFSET_XC (1.2 * 1000)
#define CONTRAST_OFFSET_2K (1.2 * 1000)
#define CONTRAST_OFFSET_4K (1.2 * 1000)

// ゲインに対するコントラストオフセット比
#define CONTRAST_OFFSET_GAIN_RT 0.03
// ゲインに対するコントラスト差比
#define CONTRAST_DIFF_GAIN_RT 0.00020

// ブロック内の輝度差の最小値
#define BLOCK_MIN_DELTA_BRIGHTNESS 3

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
/// 近傍マッチングブロック視差値 1
/// </summary>
static float *block_dsp_n1;

/// <summary>
/// 近傍マッチングブロック視差値 2
/// </summary>
static float *block_dsp_n2;

/// <summary>
/// 近傍マッチング基準画像 1
/// </summary>
static unsigned char *ref_img_n1;

/// <summary>
/// 近傍マッチング基準画像 2
/// </summary>
static unsigned char *ref_img_n2;

/// <summary>
/// 近傍マッチング比較画像 1
/// </summary>
static unsigned char *cmp_img_n1;

/// <summary>
/// 近傍マッチング比較画像 2
/// </summary>
static unsigned char *cmp_img_n2;

/// <summary>
/// 近傍マッチング基準画像 1 (2バイト画素)
/// </summary>
static unsigned short *ref_img_n1_16U;

/// <summary>
/// 近傍マッチング基準画像 2 (2バイト画素)
/// </summary>
static unsigned short *ref_img_n2_16U;

/// <summary>
/// 近傍マッチング比較画像 1 (2バイト画素)
/// </summary>
static unsigned short *cmp_img_n1_16U;

/// <summary>
/// 近傍マッチング比較画像 2 (2バイト画素)
/// </summary>
static unsigned short *cmp_img_n2_16U;

/// <summary>
/// バックマッチング視差評価領域幅（片側）
/// </summary>
static int backMatchingEvaluationWidth = 1;

/// <summary>
/// バックマッチング視差評価視差値幅
/// </summary>
static int backMatchingEvaluationRange = 3;

/// <summary>
/// バックマッチング評価視差正当率（％）
/// </summary>
static int backMatchingValidRatio = 20;

/// <summary>
/// バックマッチング評価視差ゼロ率（％）
/// </summary>
static int backMatchingZeroRatio = 80;

/// <summary>
/// 近傍マッチング 0:しない 1:する
/// </summary>
static int neighborMatching = 0;

/// <summary>
/// 近傍マッチング回転角（ラジアン）
/// </summary>
static double neighborMatchingRotateRad = 0.001;

/// <summary>
/// 近傍マッチング垂直シフト
/// </summary>
static double neighborMatchingVertShift = 0.10;

/// <summary>
/// 近傍マッチング水平シフト
/// </summary>
static double neighborMatchingHorzShift = 0.5;

/// <summary>
/// 近傍マッチング視差変化範囲
/// </summary>
static float neighborMatchingDispRange = 10.0;

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
	// マッチング探索打ち切り幅
	int brkwdt;
	// 拡張マッチング信頼限界
	int extcnf;

	// コントラスト閾値
	int crstthr;
	// コントラストオフセット
	int crstofs;
	// 階調補正モードステータス
	int grdcrct;
	// マッチングブロック最低輝度比率(%)
	int minbrtrt;

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
	// 入力基準画像データ（12ビット階調対応）
	unsigned short *pimgref_16U;
	// 入力比較画像データ（12ビット階調対応）
	unsigned short *pimgcmp_16U;

	// マッチングの視差値
	float *pblkdsp;
	// バックマッチングの視差値
	float *pblkbkdsp;

	// 基準ブロックコントラスト
	int* pblkrefcrst;
	// 比較ブロックコントラスト
	int* pblkcmpcrst;

	// 基準画像のブロック輝度値
	int* pimgrefbrt;
	// 比較画像のブロック輝度値
	int* pimgcmpbrt;

	// バンド開始ライン位置（y座標）
	int bandStart;
	// バンド終了ライン位置（y座標）
	int bandEnd;

};

static BNAD_THREAD_INFO bandInfo[MAX_NUM_OF_BANDS] = {};

/// <summary>
/// バンド分割ブロック輝度
/// </summary>
struct BNAD_THREAD_BLOCK_INFO {
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
	unsigned char* pimgref;
	// 入力比較画像データ
	unsigned char* pimgcmp;
	// 入力基準画像データ（12ビット階調対応）
	unsigned short* pimgref_16U;
	// 入力比較画像データ（12ビット階調対応）
	unsigned short* pimgcmp_16U;

	// 基準ブロックコントラスト
	int* pblkrefcrst;
	// 比較ブロックコントラスト
	int* pblkcmpcrst;

	// 基準画像のブロック輝度
	int* pimgrefbrt;
	// 比較画像のブロック輝度
	int* pimgcmpbrt;

	// バンド開始ライン位置（y座標）
	int bandStart;
	// バンド終了ライン位置（y座標）
	int bandEnd;

};

static BNAD_THREAD_BLOCK_INFO bandBlockInfo[MAX_NUM_OF_BANDS] = {};


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
	memset(block_dsp, 0, imghgt * imgwdt * sizeof(float));
	// バックマッチングブロック視差値（サブピクセル精度：浮動小数）の配列（視差ブロックごと）
	bk_block_dsp = (float *)malloc(imghgt * imgwdt * sizeof(float));
	memset(bk_block_dsp, 0, imghgt * imgwdt * sizeof(float));

	// ダブルシャッターブロック視差値（サブピクセル精度：浮動小数）の配列（視差ブロックごと）
	dbl_block_dsp = (float*)malloc(imghgt * imgwdt * sizeof(float));
	memset(dbl_block_dsp, 0, imghgt * imgwdt * sizeof(float));

	// ブロックコントラスト（視差ブロックごと）
	block_crst = (int *)malloc(imghgt * imgwdt * sizeof(int));

	// ダブルシャッターブロックコントラスト（視差ブロックごと）
	dbl_block_crst = (int*)malloc(imghgt * imgwdt * sizeof(int));

	// 比較ブロックコントラスト（視差ブロックごと）
	cmp_block_crst = (int*)malloc(imghgt * imgwdt * sizeof(int));

	// 比較画像の視差位置（画素ごと）
	dsp_posi = (int *)malloc(imghgt * imgwdt * sizeof(int));

	// 入力補正画像の高さ
	correctedImageHeight = imghgt;
	// 入力補正画像の幅
	correctedImageWidth = imgwdt;

	// 近傍マッチングブロック視差値
	block_dsp_n1 = (float *)malloc(imghgt * imgwdt * sizeof(float));
	block_dsp_n2 = (float *)malloc(imghgt * imgwdt * sizeof(float));
	// 近傍マッチング基準画像
	ref_img_n1 = (unsigned char *)malloc(imghgt * imgwdt);
	ref_img_n2 = (unsigned char *)malloc(imghgt * imgwdt);
	ref_img_n1_16U = (unsigned short *)malloc(imghgt * imgwdt * sizeof(unsigned short));
	ref_img_n2_16U = (unsigned short *)malloc(imghgt * imgwdt * sizeof(unsigned short));
	// 近傍マッチング比較画像
	cmp_img_n1 = (unsigned char *)malloc(imghgt * imgwdt);
	cmp_img_n2 = (unsigned char *)malloc(imghgt * imgwdt);
	cmp_img_n1_16U = (unsigned short *)malloc(imghgt * imgwdt * sizeof(unsigned short));
	cmp_img_n2_16U = (unsigned short *)malloc(imghgt * imgwdt * sizeof(unsigned short));

	// ブロック輝度値
	ref_block_brt = (int*)malloc(imghgt * imgwdt * sizeof(int));
	cmp_block_brt = (int*)malloc(imghgt * imgwdt * sizeof(int));

}


/// <summary>
/// ステレオマッチングを終了する
/// </summary>
void StereoMatching::finalize()
{
	// バッファーを解放する
	free(block_dsp);
	free(bk_block_dsp);
	free(dbl_block_dsp);

	free(block_crst);
	free(dbl_block_crst);
	free(cmp_block_crst);
	free(dsp_posi);

	free(block_dsp_n1);
	free(block_dsp_n2);
	free(ref_img_n1);
	free(ref_img_n2);
	free(ref_img_n1_16U);
	free(ref_img_n2_16U);
	free(cmp_img_n1);
	free(cmp_img_n2);
	free(cmp_img_n1_16U);
	free(cmp_img_n2_16U);

	free(ref_block_brt);
	free(cmp_block_brt);

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
/// <param name="rmvdup">重複マッチング除去：0:しない 1:する(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
void StereoMatching::setMatchingParameter(int imghgt, int imgwdt, int depth,
	int blkhgt, int blkwdt, int mtchgt, int mtcwdt, int blkofsx, int blkofsy, int crstthr, int grdcrct, 
	int rmvdup, int minbrtrt)
{

	correctedImageHeight = imghgt; // 入力補正画像の縦サイズ
	correctedImageWidth = imgwdt; // 入力補正画像の横サイズ
	matchingDepth = depth; // マッチング探索幅

	disparityBlockHeight = blkhgt; // 視差ブロック高さ
	disparityBlockWidth = blkwdt; // 視差ブロック幅
	matchingBlockHeight = mtchgt; // マッチングブロック高さ
	matchingBlockWidth = mtcwdt; // マッチングブロック幅
	
	dispBlockOffsetX = blkofsx; // 視差ブロック横オフセット
	dispBlockOffsetY = blkofsy; // 視差ブロック縦オフセット

	contrastThreshold = crstthr; // コントラスト閾値
	gradationCorrectionMode = grdcrct; // 階調補正モードステータス
	removeDuplicateMatching = rmvdup; // 重複マッチング除去：0:しない 1:する

	// マッチングブロック最低輝度比率(%)
	matchingMinBrightRatio = minbrtrt;
	
}


/// <summary>
/// 拡張マッチングパラメータを設定する
/// </summary>
/// <param name="extmtc">拡張マッチング 0:しない 1:する(IN)</param>
/// <param name="extlim">拡張マッチング探索制限幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
void StereoMatching::setExtensionMatchingParameter(int extmtc, int extlim, int extcnf)
{
	matchingExtension = extmtc;
	matchingExtLimitWidth = extlim;
	matchingExtConfidenceLimit = extcnf;

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
/// 近傍マッチングパラメータを設定する
/// </summary>
/// <param name="enb">近傍マッチング 0:しない 1:する(IN)</param>
/// <param name="neibrot">近傍マッチング回転角(度)(IN)</param>
/// <param name="neibvsft">近傍マッチング垂直シフト(IN)</param>
/// <param name="neibhsft">近傍マッチング水平シフト(IN)</param>
/// <param name="neibrng">近傍マッチング視差変化範囲(IN)</param>
void StereoMatching::setNeighborMatchingParameter(int enb, double neibrot, double neibvsft, double neibhsft, double neibrng)
{
	neighborMatching = enb; // 近傍マッチング 0:しない 1:する
	double th = neibrot / 180 * 3.14159265359;
	neighborMatchingRotateRad = th; // 近傍マッチング回転角(ラジアン)
	neighborMatchingVertShift = neibvsft; // 近傍マッチング垂直シフト
	neighborMatchingHorzShift = neibhsft; // 近傍マッチング水平シフト
	neighborMatchingDispRange = (float)neibrng; // 近傍マッチング視差変化範囲

}


/// <summary>
/// ステレオマッチングを実行する
/// </summary>
/// <param name="prgtimg">右画（基準）像データ(IN)</param>
/// <param name="plftimg">左画（比較）像データ(IN)</param>
/// <param name="frmgain">画像フレームのセンサーゲイン値(IN)</param>
void StereoMatching::matching(unsigned char* prgtimg, unsigned char* plftimg, int frmgain)
{
	doMatching(prgtimg, plftimg, frmgain, block_dsp, block_crst);

}


/// <summary>
/// ステレオマッチングを実行する
/// </summary>
/// <param name="prgtimg">右画（基準）像データ(IN)</param>
/// <param name="plftimg">左画（比較）像データ(IN)</param>
/// <param name="frmgain">画像フレームのセンサーゲイン値(IN)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::matching(unsigned short* prgtimg, unsigned short* plftimg, int frmgain)
{
	doMatching16U(prgtimg, plftimg, frmgain, block_dsp, block_crst);

}


/// <summary>
/// ダブルシャッター画像のステレオマッチングを実行する
/// </summary>
/// <param name="prgtimghigh">高感度右画（基準）像データ(IN)</param>
/// <param name="plftimghigh">高感度左画（比較）像データ(IN)</param>
/// <param name="frmgainhigh">高感度画像フレームのセンサーゲイン値(IN)</param>
/// <param name="prgtimglow">低感度右画（基準）像データ(IN)</param>
/// <param name="plftimglow">低感度左画（比較）像データ(IN)</param>
/// <param name="frmgainlow">低感度画像フレームのセンサーゲイン値(IN)</param>
void StereoMatching::matchingDouble(unsigned char* prgtimghigh, unsigned char* plftimghigh, int frmgainhigh,
	unsigned char* prgtimglow, unsigned char* plftimglow, int frmgainlow)
{
	doMatching(prgtimghigh, plftimghigh, frmgainhigh, block_dsp, block_crst);
	doMatching(prgtimglow, plftimglow, frmgainlow, dbl_block_dsp, dbl_block_crst);

	int imghgt = correctedImageHeight;
	int imgwdt = correctedImageWidth;
	int blkhgt = disparityBlockHeight;
	int blkwdt = disparityBlockWidth;

	blendDobleDisparity(imghgt, imgwdt, blkhgt, blkwdt,
		block_dsp, block_crst, dbl_block_dsp, dbl_block_crst);

}


/// <summary>
/// ダブルシャッター画像のステレオマッチングを実行する
/// </summary>
/// <param name="prgtimghigh">高感度右画（基準）像データ(IN)</param>
/// <param name="plftimghigh">高感度左画（比較）像データ(IN)</param>
/// <param name="frmgainhigh">高感度画像フレームのセンサーゲイン値(IN)</param>
/// <param name="prgtimglow">低感度右画（基準）像データ(IN)</param>
/// <param name="plftimglow">低感度左画（比較）像データ(IN)</param>
/// <param name="frmgainlow">低感度画像フレームのセンサーゲイン値(IN)</param>
void StereoMatching::matchingDouble(unsigned short* prgtimghigh, unsigned short* plftimghigh, int frmgainhigh,
	unsigned short* prgtimglow, unsigned short* plftimglow, int frmgainlow)
{
	doMatching16U(prgtimghigh, plftimghigh, frmgainhigh, block_dsp, block_crst);
	doMatching16U(prgtimglow, plftimglow, frmgainlow, dbl_block_dsp, dbl_block_crst);

	int imghgt = correctedImageHeight;
	int imgwdt = correctedImageWidth;
	int blkhgt = disparityBlockHeight;
	int blkwdt = disparityBlockWidth;

	blendDobleDisparity(imghgt, imgwdt, blkhgt, blkwdt,
		block_dsp, block_crst, dbl_block_dsp, dbl_block_crst);

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

	// 視差値とコントラストをコピーする
	// 視差値 block_dspはブロック単位、コントラスト ref_block_crstは画素単位
	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			pblkval[j * width + i] = (int)((MATCHING_SUBPIXEL_TIMES * (block_dsp[j * width + i])) + 0.5);
			pblkcrst[j * width + i] = block_crst[correctedImageWidth * j * disparityBlockHeight + i * disparityBlockWidth];
		}
	}

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
/// 近傍マッチングのデータを記録 0:しない 1:する
/// </summary>
static int recordNeighborMatching = 0;


/// <summary>
/// 近傍マッチングのデータを記録する
/// </summary>
void StereoMatching::setRecordNeighborMatching()
{

	if (recordNeighborMatching == 0) {
		recordNeighborMatching = 1;
	}

}

/// <summary>
/// ステレオマッチングを実行する
/// </summary>
/// <param name="prgtimg">右画（基準）像データ(IN)</param>
/// <param name="plftimg">左画（比較）像データ(IN)</param>
/// <param name="frmgain">画像フレームのセンサーゲイン値(IN)</param>
/// <param name="pblkdsp">マッチングブロック視差(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::doMatching(unsigned char* prgtimg, unsigned char* plftimg, int frmgain, float* pblkdsp, int *pblkcrst)
{
	// 近傍マッチング
	if (neighborMatching == 0) {
		// ステレオマッチングを実行する
		if (dispMatchingUseOpenCL == 0) {
			executeMatching(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg, frmgain,
				pblkdsp, pblkcrst);
		}
		else {
			executeMatchingOpenCL(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg, frmgain,
				pblkdsp, pblkcrst);
		}
	}
	else {
		// 近傍マッチング基準画像を生成する
		makeNeighborImage(correctedImageHeight, correctedImageWidth, 
			neighborMatchingRotateRad, 0.0, 0.0,
			prgtimg, ref_img_n1);
		makeNeighborImage(correctedImageHeight, correctedImageWidth,
			(-1.0) * neighborMatchingRotateRad, 0.0, 0.0,
			prgtimg, ref_img_n2);

		// 近傍マッチング比較画像を生成する
		makeNeighborImage(correctedImageHeight, correctedImageWidth,
			neighborMatchingRotateRad, neighborMatchingVertShift, neighborMatchingHorzShift,
			plftimg, cmp_img_n1);
		makeNeighborImage(correctedImageHeight, correctedImageWidth,
			(-1.0) * neighborMatchingRotateRad, (-1.0) * neighborMatchingVertShift, (-0.1) * neighborMatchingHorzShift,
			plftimg, cmp_img_n2);

		// ステレオマッチングを実行する
		if (dispMatchingUseOpenCL == 0) {
			executeMatching(correctedImageHeight, correctedImageWidth, matchingDepth, ref_img_n1, cmp_img_n1, frmgain,
				block_dsp_n1, pblkcrst);
			executeMatching(correctedImageHeight, correctedImageWidth, matchingDepth, ref_img_n2, cmp_img_n2, frmgain,
				block_dsp_n2, pblkcrst);
			executeMatching(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg, frmgain,
				pblkdsp, pblkcrst);
		}
		else {
			executeMatchingOpenCL(correctedImageHeight, correctedImageWidth, matchingDepth, ref_img_n1, cmp_img_n1, frmgain,
				block_dsp_n1, pblkcrst);
			executeMatchingOpenCL(correctedImageHeight, correctedImageWidth, matchingDepth, ref_img_n2, cmp_img_n2, frmgain,
				block_dsp_n2, pblkcrst);
			executeMatchingOpenCL(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg, frmgain,
				pblkdsp, pblkcrst);
		}

		// 近傍マッチングの視差を合成する
		int imghgt = correctedImageHeight;
		int imgwdt = correctedImageWidth;
		int blkhgt = disparityBlockHeight;
		int blkwdt = disparityBlockWidth;
		float neibrng = neighborMatchingDispRange;

		blendNeighborMatchingDisparity(imghgt, imgwdt, blkhgt, blkwdt, neibrng, block_dsp_n1, block_dsp_n2, pblkdsp);

	}

}


/// <summary>
/// ステレオマッチングを実行する
/// </summary>
/// <param name="prgtimg">右画（基準）像データ(IN)</param>
/// <param name="plftimg">左画（比較）像データ(IN)</param>
/// <param name="frmgain">画像フレームのセンサーゲイン値(IN)</param>
/// <param name="pblkdsp">マッチングブロック視差(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::doMatching16U(unsigned short* prgtimg, unsigned short* plftimg, int frmgain, float *pblkdsp, int *pblkcrst)
{

	// 近傍マッチング
	if (neighborMatching == 0) {
		// ステレオマッチングを実行する
		if (dispMatchingUseOpenCL == 0) {
			executeMatching16U(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg, frmgain,
				pblkdsp, pblkcrst);
		}
		else {
			executeMatchingOpenCL16U(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg, frmgain,
				pblkdsp, pblkcrst);
		}
	}
	else {
		// 近傍マッチング基準画像を生成する
		makeNeighborImage16U(correctedImageHeight, correctedImageWidth,
			neighborMatchingRotateRad, 0.0,
			prgtimg, ref_img_n1_16U);
		makeNeighborImage16U(correctedImageHeight, correctedImageWidth,
			(-1.0) * neighborMatchingRotateRad, 0.0,
			prgtimg, ref_img_n2_16U);

		// 近傍マッチング比較画像を生成する
		makeNeighborImage16U(correctedImageHeight, correctedImageWidth,
			neighborMatchingRotateRad, neighborMatchingVertShift, neighborMatchingHorzShift,
			plftimg, cmp_img_n1_16U);
		makeNeighborImage16U(correctedImageHeight, correctedImageWidth,
			(-1.0) * neighborMatchingRotateRad, (-1.0) * neighborMatchingVertShift, (-1.0) * neighborMatchingHorzShift,
			plftimg, cmp_img_n2_16U);

		// ステレオマッチングを実行する
		if (dispMatchingUseOpenCL == 0) {
			executeMatching16U(correctedImageHeight, correctedImageWidth, matchingDepth, ref_img_n1_16U, cmp_img_n1_16U, frmgain,
				block_dsp_n1, pblkcrst);
			executeMatching16U(correctedImageHeight, correctedImageWidth, matchingDepth, ref_img_n2_16U, cmp_img_n2_16U, frmgain,
				block_dsp_n2, pblkcrst);
			executeMatching16U(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg, frmgain,
				pblkdsp, pblkcrst);
		}
		else {
			executeMatchingOpenCL16U(correctedImageHeight, correctedImageWidth, matchingDepth, ref_img_n1_16U, cmp_img_n1_16U, frmgain,
				block_dsp_n1, pblkcrst);
			executeMatchingOpenCL16U(correctedImageHeight, correctedImageWidth, matchingDepth, ref_img_n2_16U, cmp_img_n2_16U, frmgain,
				block_dsp_n2, pblkcrst);
			executeMatchingOpenCL16U(correctedImageHeight, correctedImageWidth, matchingDepth, prgtimg, plftimg, frmgain,
				pblkdsp, pblkcrst);
		}

		// 近傍マッチングの視差を合成する
		int imghgt = correctedImageHeight;
		int imgwdt = correctedImageWidth;
		int blkhgt = disparityBlockHeight;
		int blkwdt = disparityBlockWidth;
		float neibrng = neighborMatchingDispRange;
			
		blendNeighborMatchingDisparity(imghgt, imgwdt, blkhgt, blkwdt, neibrng, block_dsp_n1, block_dsp_n2, pblkdsp);
			
	}

}


/// <summary>
/// ダブルシャッターの視差データを合成する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="blkhgt">視差ブロック高さ(IN)</param>
/// <param name="blkwdt">視差ブロック幅(IN)</param>
/// <param name="pblkdsp_h">高感度 ブロック視差情報(IN/OUT)</param>
/// <param name="pblkcrst_h">高感度 ブロックコントラスト(IN/OUT)</param>
/// <param name="pblkdsp_l">低感度 ブロック視差情報(IN)</param>
/// <param name="pblkcrst_l">低感度 ブロックコントラスト(IN)</param>
void StereoMatching::blendDobleDisparity(int imghgt, int imgwdt, int blkhgt, int blkwdt,
	float* pblkdsp_h, int* pblkcrst_h, float* pblkdsp_l, int* pblkcrst_l)
{

	// 画像の幅ブロック数
	int imgwdtblk = imgwdt / blkwdt;

	for (int j = 0, jj = 0; j < imghgt; j += blkhgt, jj++) {
		int bidxjj = jj * imgwdtblk;
		int idxj = j * imgwdt;

		for (int i = 0, ii = 0; i < imgwdt; i += blkwdt, ii++) {
			int bidxii = bidxjj + ii;
			int idxi = idxj + i;
			// ブロック単位の合成
			// 高感度側が視差なしの場合、低感度の視差で埋める
			if (pblkdsp_h[bidxii] == 0.0) {
				pblkdsp_h[bidxii] = pblkdsp_l[bidxii];
				pblkcrst_h[idxi] = pblkcrst_l[idxi];
			}
		}
	}

}


/// <summary>
/// 近傍マッチングの視差を合成する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="blkhgt">視差ブロック高さ(IN)</param>
/// <param name="blkwdt">視差ブロック幅(IN)</param>
/// <param name="neibrng">近傍マッチング視差変化範囲(IN)</param>
/// <param name="pblkdsp_n1">近傍マッチングブロック視差１(IN)</param>
/// <param name="pblkdsp_n2">近傍マッチングブロック視差２(IN)</param>
/// <param name="pblkdsp">マッチングブロック視差(IN/OUT)</param>
void StereoMatching::blendNeighborMatchingDisparity(int imghgt, int imgwdt, int blkhgt, int blkwdt,
	float neibrng, 	float *pblkdsp_n1, float* pblkdsp_n2, float* pblkdsp)
{

	// 出力視差ブロック画像の高さ
	int imghgtblk = imghgt / blkhgt;
	// 出力視差ブロック画像の幅
	int imgwdtblk = imgwdt / blkwdt;

	for (int j = 0; j < imghgtblk; j++) {
		int bidxj = j * imgwdtblk;
		for (int i = 0; i < imgwdtblk; i++) {
			int bidxi = bidxj + i;
			float dsp_t0 = pblkdsp[bidxi];
			if (dsp_t0 != 0.0) {
				float dsp_t1 = pblkdsp_n1[bidxi];
				float dsp_t2 = pblkdsp_n2[bidxi];
				float dif_t1 = dsp_t0 - dsp_t1;
				float dif_t2 = dsp_t0 - dsp_t2;
				if (dif_t1 > neibrng || dif_t1 < (-1.0) * neibrng ||
					dif_t2 > neibrng || dif_t2 < (-1.0) * neibrng) {
					dsp_t0 = 0.0;
				}
			}
			pblkdsp[bidxi] = dsp_t0;
		}
	}

}


/// <summary>
/// 近傍マッチング画像を生成する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="rotrad">回転角(ラジアン)(IN)</param>
/// <param name="vrtsft">垂直シフト量(画素)(IN)</param>
/// <param name="hrzsft">水平シフト量(画素)(IN)</param>
/// <param name="psrcimg">変換前画像(IN)</param>
/// <param name="pdstimg">変換後画像(OUT)</param>
void StereoMatching::makeNeighborImage(int imghgt, int imgwdt, double rotrad, double vrtsft, double hrzsft,
	unsigned char* psrcimg, unsigned char* pdstimg)
{
	double cntx = ((double)(imgwdt - 1) / 2);
	double cnty = ((double)(imghgt - 1) / 2);

	for (int j = 0; j < imghgt; j++) {
		for (int i = 0; i < imgwdt; i++) {

			double cofsx = (double)i - cntx;
			double cofsy = (double)j - cnty;

			double wdx = cofsx * cos(rotrad) - cofsy * sin(rotrad) + cntx + hrzsft;
			double wdy = cofsx * sin(rotrad) + cofsy * cos(rotrad) + cnty + vrtsft;

			int inti = (int)wdx;
			int intj = (int)wdy;
			double deci = wdx - inti;
			double decj = wdy - intj;

			// 補正画像の輝度を算出する
			// 画像サイズを超える場合は画像端の輝度データを使う
			if (intj < (0)) {
				intj = 0;
				decj = 0.0;
			}
			if (inti < (0)) {
				inti = 0;
				deci = 0.0;
			}
			if (intj >= (imghgt - 1)) {
				intj = imghgt - 2;
				decj = 0.0;
			}
			if (inti >= (imgwdt - 1)) {
				inti = imgwdt - 2;
				deci = 0.0;
			}

			int idxi0 = intj * imgwdt + inti;
			int idxi1 = (intj + 1) * imgwdt + inti;

			pdstimg[j * imgwdt + i] = (unsigned char)(
				(1.0 - deci) * (1.0 - decj) * (double)psrcimg[idxi0] //[intj][inti]
				+ deci * (1.0 - decj) * (double)psrcimg[idxi0 + 1] //[intj][inti + 1]
				+ (1.0 - deci) * decj * (double)psrcimg[idxi1] //[intj + 1][inti]
				+ deci * decj * (double)psrcimg[idxi1 + 1] //[intj + 1][inti + 1];
				);
		}
	}

}


/// <summary>
/// 近傍マッチング画像を生成する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="rotrad">回転角(ラジアン)(IN)</param>
/// <param name="vrtsft">垂直シフト量(画素)(IN)</param>
/// <param name="psrcimg">変換前画像(IN)</param>
/// <param name="pdstimg">変換後画像(OUT)</param>
void StereoMatching::makeNeighborImage(int imghgt, int imgwdt, double rotrad, double vrtsft,
	unsigned char* psrcimg, unsigned char* pdstimg)
{

	double cntx = ((double)(imgwdt - 1) / 2);
	double cnty = ((double)(imghgt - 1) / 2);

	for (int j = 0; j < imghgt; j++) {
		for (int i = 0; i < imgwdt; i++) {

			double cofsx = (double)i - cntx;
			double cofsy = (double)j - cnty;

			double wdx = cofsx * cos(rotrad) - cofsy * sin(rotrad) + cntx;
			double wdy = cofsx * sin(rotrad) + cofsy * cos(rotrad) + cnty + vrtsft;

			int inti = (int)wdx;
			int intj = (int)wdy;
			double deci = wdx - inti;
			double decj = wdy - intj;

			// 補正画像の輝度を算出する
			// 画像サイズを超える場合は画像端の輝度データを使う
			if (intj < (0)) {
				intj = 0;
				decj = 0.0;
			}
			if (inti < (0)) {
				inti = 0;
				deci = 0.0;
			}
			if (intj >= (imghgt - 1)) {
				intj = imghgt - 2;
				decj = 0.0;
			}
			if (inti >= (imgwdt - 1)) {
				inti = imgwdt - 2;
				deci = 0.0;
			}

			int idxi0 = intj * imgwdt + inti;
			int idxi1 = (intj + 1) * imgwdt + inti;

			pdstimg[j * imgwdt + i] = (unsigned char)(
				(1.0 - deci)*(1.0 - decj)*(double)psrcimg[idxi0] //[intj][inti]
				+ deci * (1.0 - decj)*(double)psrcimg[idxi0 + 1] //[intj][inti + 1]
				+ (1.0 - deci)*decj*(double)psrcimg[idxi1] //[intj + 1][inti]
				+ deci * decj*(double)psrcimg[idxi1 + 1] //[intj + 1][inti + 1];
				);
		}
	}

}


/// <summary>
/// 近傍マッチング画像を生成する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="rotrad">回転角(ラジアン)(IN)</param>
/// <param name="vrtsft">垂直シフト量(画素)(IN)</param>
/// <param name="hrzsft">水平シフト量(画素)(IN)</param>
/// <param name="psrcimg">変換前画像(IN)</param>
/// <param name="pdstimg">変換後画像(OUT)</param>
void StereoMatching::makeNeighborImage16U(int imghgt, int imgwdt, double rotrad, double vrtsft, double hrzsft,
	unsigned short* psrcimg, unsigned short* pdstimg)
{

	double cntx = ((double)(imgwdt - 1) / 2);
	double cnty = ((double)(imghgt - 1) / 2);

	for (int j = 0; j < imghgt; j++) {
		for (int i = 0; i < imgwdt; i++) {

			double cofsx = (double)i - cntx;
			double cofsy = (double)j - cnty;

			double wdx = cofsx * cos(rotrad) - cofsy * sin(rotrad) + cntx + hrzsft;
			double wdy = cofsx * sin(rotrad) + cofsy * cos(rotrad) + cnty + vrtsft;

			int inti = (int)wdx;
			int intj = (int)wdy;
			double deci = wdx - inti;
			double decj = wdy - intj;

			// 補正画像の輝度を算出する
			// 画像サイズを超える場合は画像端の輝度データを使う
			if (intj < (0)) {
				intj = 0;
				decj = 0.0;
			}
			if (inti < (0)) {
				inti = 0;
				deci = 0.0;
			}
			if (intj >= (imghgt - 1)) {
				intj = imghgt - 2;
				decj = 0.0;
			}
			if (inti >= (imgwdt - 1)) {
				inti = imgwdt - 2;
				deci = 0.0;
			}

			int idxi0 = intj * imgwdt + inti;
			int idxi1 = (intj + 1) * imgwdt + inti;

			pdstimg[j * imgwdt + i] = (unsigned short)(
				(1.0 - deci) * (1.0 - decj) * (double)psrcimg[idxi0] //[intj][inti]
				+ deci * (1.0 - decj) * (double)psrcimg[idxi0 + 1] //[intj][inti + 1]
				+ (1.0 - deci) * decj * (double)psrcimg[idxi1] //[intj + 1][inti]
				+ deci * decj * (double)psrcimg[idxi1 + 1] //[intj + 1][inti + 1];
				);
		}
	}

}


/// <summary>
/// 近傍マッチング画像を生成する
/// </summary>
/// <param name="imghgt">画像の高さ(IN)</param>
/// <param name="imgwdt">画像の幅(IN)</param>
/// <param name="rotrad">回転角(ラジアン)(IN)</param>
/// <param name="vrtsft">垂直シフト量(画素)(IN)</param>
/// <param name="psrcimg">変換前画像(IN)</param>
/// <param name="pdstimg">変換後画像(OUT)</param>
void StereoMatching::makeNeighborImage16U(int imghgt, int imgwdt, double rotrad, double vrtsft,
	unsigned short* psrcimg, unsigned short* pdstimg)
{

	double cntx = ((double)(imgwdt - 1) / 2);
	double cnty = ((double)(imghgt - 1) / 2);

	for (int j = 0; j < imghgt; j++) {
		for (int i = 0; i < imgwdt; i++) {

			double cofsx = (double)i - cntx;
			double cofsy = (double)j - cnty;

			double wdx = cofsx * cos(rotrad) - cofsy * sin(rotrad) + cntx;
			double wdy = cofsx * sin(rotrad) + cofsy * cos(rotrad) + cnty + vrtsft;

			int inti = (int)wdx;
			int intj = (int)wdy;
			double deci = wdx - inti;
			double decj = wdy - intj;

			// 補正画像の輝度を算出する
			// 画像サイズを超える場合は画像端の輝度データを使う
			if (intj < (0)) {
				intj = 0;
				decj = 0.0;
			}
			if (inti < (0)) {
				inti = 0;
				deci = 0.0;
			}
			if (intj >= (imghgt - 1)) {
				intj = imghgt - 2;
				decj = 0.0;
			}
			if (inti >= (imgwdt - 1)) {
				inti = imgwdt - 2;
				deci = 0.0;
			}

			int idxi0 = intj * imgwdt + inti;
			int idxi1 = (intj + 1) * imgwdt + inti;

			pdstimg[j * imgwdt + i] = (unsigned short)(
				(1.0 - deci)*(1.0 - decj)*(double)psrcimg[idxi0] //[intj][inti]
				+ deci * (1.0 - decj)*(double)psrcimg[idxi0 + 1] //[intj][inti + 1]
				+ (1.0 - deci)*decj*(double)psrcimg[idxi1] //[intj + 1][inti]
				+ deci * decj*(double)psrcimg[idxi1 + 1] //[intj + 1][inti + 1];
				);
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
/// <param name="frmgain">画像フレームのセンサーゲイン値(IN)</param>
/// <param name="pblkdsp">ブロックごとの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::executeMatching(int imghgt, int imgwdt, int depth, 
	unsigned char *pimgref, unsigned char *pimgcmp, int frmgain,
	float * pblkdsp, int *pblkcrst)
{

	// 視差ブロックの高さ
	int stphgt = disparityBlockHeight;
	// 視差ブロックの幅
	int stpwdt = disparityBlockWidth;

	// マッチングブロック最低輝度比率(%)
	int minbrtrt = matchingMinBrightRatio;

	// マッチング探索打ち切り幅
	int brkwdt = depth;
	// 拡張マッチングの場合
	if (matchingExtension == 1) {
		brkwdt = matchingExtLimitWidth;
	}
	// 拡張マッチング信頼限界
	int extcnf = matchingExtConfidenceLimit;
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
		}
	else if (imgwdt == IMG_WIDTH_2K) {
		crstofs = (int)CONTRAST_OFFSET_2K;
	}
	else if (imgwdt == IMG_WIDTH_4K) {
		crstofs = (int)CONTRAST_OFFSET_4K;
	}

	if (crstthr != 0) {
		// ゲインによるコントラスト差分を加える
		crstthr = crstthr + (int)(frmgain * CONTRAST_DIFF_GAIN_RT * 1000);
		// ゲインによるコントラストオフセットを加える
		crstofs = crstofs + (int)(frmgain * CONTRAST_OFFSET_GAIN_RT * 1000);
	}

	// 重複マッチング除去
	int rmvdup = removeDuplicateMatching;

	// バックマッチング
	float *pblkbkdsp = NULL;

	// 遮蔽幅を設定する
	shadeWidth = brkwdt;

	// 比較画像の視差位置をクリアする
	memset(dsp_posi, 0, imghgt * imgwdt * sizeof(int));

	if (enableBackMatching == 1) {
		memset(bk_block_dsp, 0, imghgt * imgwdt * sizeof(float));
		pblkbkdsp = bk_block_dsp;
		shadeWidth = 0;
	}

	// 視差を取得する
	getMatchingDisparity(imghgt, imgwdt, depth, brkwdt, extcnf,
		crstthr, crstofs, grdcrct, minbrtrt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
		pimgref, pimgcmp, pblkdsp, pblkbkdsp, ref_block_brt, cmp_block_brt, pblkcrst, cmp_block_crst);

	// バックマッチングの視差を合成する
	if (enableBackMatching == 1) {
		blendBothMatchingDisparity(imghgt, imgwdt, imghgtblk, imgwdtblk,
			backMatchingEvaluationWidth, backMatchingEvaluationRange, backMatchingValidRatio, backMatchingZeroRatio,
			pblkdsp, pblkbkdsp);
	}
	else {
		// 重複マッチング除去の場合
		if (rmvdup == 1) {
			removeDuplicateBlock(imghgt, imgwdt,
		stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				ref_block_brt, cmp_block_brt, pblkdsp, dsp_posi);
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
/// <param name="frmgain">画像フレームのセンサーゲイン値(IN)</param>
/// <param name="pblkdsp">ブロックごとの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::executeMatching16U(int imghgt, int imgwdt, int depth,
	unsigned short *pimgref, unsigned short *pimgcmp, int frmgain,
	float * pblkdsp, int *pblkcrst)
{

	// 視差ブロックの高さ
	int stphgt = disparityBlockHeight;
	// 視差ブロックの幅
	int stpwdt = disparityBlockWidth;

	// マッチングブロック最低輝度比率(%)
	int minbrtrt = matchingMinBrightRatio;

	// マッチング探索打ち切り幅
	int brkwdt = depth;
	// 拡張マッチングの場合
	if (matchingExtension == 1) {
		brkwdt = matchingExtLimitWidth;
	}
	// 拡張マッチング信頼限界
	int extcnf = matchingExtConfidenceLimit;
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
	}
	else if (imgwdt == IMG_WIDTH_2K) {
		crstofs = (int)CONTRAST_OFFSET_2K;
	}
	else if (imgwdt == IMG_WIDTH_4K) {
		crstofs = (int)CONTRAST_OFFSET_4K;
	}

	if (crstthr != 0) {
		// ゲインによるコントラスト差分を加える
		crstthr = crstthr + (int)(frmgain * CONTRAST_DIFF_GAIN_RT * 1000);
		// ゲインによるコントラストオフセットを加える
		crstofs = crstofs + (int)(frmgain * CONTRAST_OFFSET_GAIN_RT * 1000);
	}

	// 重複マッチング除去
	int rmvdup = removeDuplicateMatching;

	// バックマッチング
	float *pblkbkdsp = NULL;

	// 遮蔽幅を設定する
	shadeWidth = brkwdt;

	// 比較画像の視差位置をクリアする
	memset(dsp_posi, 0, imghgt * imgwdt * sizeof(int));

	if (enableBackMatching == 1) {
		memset(bk_block_dsp, 0, imghgt * imgwdt * sizeof(float));
		pblkbkdsp = bk_block_dsp;
		shadeWidth = 0;
	}

	// 視差を取得する
	getMatchingDisparity16U(imghgt, imgwdt, depth, brkwdt, extcnf,
		crstthr, crstofs, grdcrct, minbrtrt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
		pimgref, pimgcmp, pblkdsp, pblkbkdsp, ref_block_brt, cmp_block_brt, pblkcrst, cmp_block_crst);

	if (enableBackMatching == 1) {
		blendBothMatchingDisparity(imghgt, imgwdt, imghgtblk, imgwdtblk,
			backMatchingEvaluationWidth, backMatchingEvaluationRange, backMatchingValidRatio, backMatchingZeroRatio,
			pblkdsp, pblkbkdsp);
	}
	else {
		// 重複マッチング除去の場合
		if (rmvdup == 1) {
			removeDuplicateBlock(imghgt, imgwdt,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				ref_block_brt, cmp_block_brt, pblkdsp, dsp_posi);
		}
	}

}


/// <summary>
/// 重複ブロックを除去する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(IN/OUT)</param>
/// <param name="pdspposi">比較画像の視差位置(OUT)</param>
void StereoMatching::removeDuplicateBlock(int imghgt, int imgwdt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	int* pimgrefbrt, int* pimgcmpbrt, float* pblkdsp, int* pdspposi)
{

	for (int jb = 0; jb < (imghgtblk - 0); jb++) {
		for (int ib = 2; ib < (imgwdtblk - 2); ib++) {

			int jpx = stphgt * jb;
			int ipx = stpwdt * ib;

			int disp = (int)(pblkdsp[jb * imgwdtblk + ib] + 0.5f);
			if (disp > 0) {
				// 今回マッチしたブロックの比較カメラ画像の座標
				// ipx：マッチングブロックのx座標（基準画像上）
				// disp：視差値整数部
				// cmpipx：マッチングしたブロックのx座標（比較画像上）
				// ipx0：前回マッチングしたブロックのx座標（基準画像上）
				int cmpipx = ipx + disp;
				// すでにマッチしていたブロックかどうかチェックする
				// pdspposiには前回マッチングしたときの視差値が記録されている
				int prvdsp = pdspposi[jpx * imgwdt + cmpipx];
				int ipx0 = cmpipx - prvdsp;

				// 重複していた場合
				if (prvdsp > 0) {
					// 3x3画素のブロック輝度を使って類似度を比較する
					int blkcnt = 5;
					unsigned int sumr0 = 0; // Σ基準画像の輝度ij
					unsigned int sumr1 = 0; // Σ基準画像の輝度ij
					unsigned int sumrr0 = 0; // Σ(基準画像の輝度ij^2)
					unsigned int sumrr1 = 0; // Σ(基準画像の輝度ij^2)
					unsigned int sumc = 0; // Σ比較画像の輝度ij
					unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
					unsigned int sumrc0 = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
					unsigned int sumrc1 = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)

					for (int col = -2; col <= 2; col++) {
						unsigned int rfx0 = pimgrefbrt[(jpx) * imgwdt + ipx0 + (col * stpwdt)];
						unsigned int rfx1 = pimgrefbrt[(jpx) * imgwdt + ipx + (col * stpwdt)];
						unsigned int cpx = pimgcmpbrt[(jpx) * imgwdt + cmpipx + (col * stpwdt)];

						sumr0 += rfx0;
						sumr1 += rfx1;
						sumc += cpx;
						sumrr0 = rfx0 * rfx0;
						sumrr1 = rfx1 * rfx1;
						sumcc += cpx * cpx;
						sumrc0 += rfx0 * cpx;
						sumrc1 += rfx1 * cpx;
					}

					// SSDを算出する
					// 整数にするためにブロックサイズを掛ける
					unsigned int sumsq0 = (sumrr0 + sumcc - 2 * sumrc0) * blkcnt - (sumr0 * sumr0 + sumc * sumc - 2 * sumr0 * sumc);
					unsigned int sumsq1 = (sumrr1 + sumcc - 2 * sumrc1) * blkcnt - (sumr1 * sumr1 + sumc * sumc - 2 * sumr1 * sumc);

					// 重複ブロックを除去する
					// 前回の方が類似度が高い場合
					if (sumsq0 < sumsq1) {
						// 今回のブロックを視差なしにする
						pblkdsp[jb * imgwdtblk + ib] = 0.0f;
					}
					// 今回の方が類似度が高い場合
					else {
						// 前回のブロックを視差なしにする
						int prviblk = ipx0 / stpwdt;
						pblkdsp[jb * imgwdtblk + prviblk] = 0.0f;
						// 今回の視差値を記録する
						pdspposi[jpx * imgwdt + cmpipx] = disp;
					}
				}
				else {
					// 視差値を記録する
					pdspposi[jpx * imgwdt + cmpipx] = disp;
				}
			}
		}
	}


}


/// <summary>
/// 視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
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
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
void StereoMatching::getMatchingDisparity(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, float *pblkdsp, float *pblkbkdsp,
	int *pimgrefbrt, int *pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst)
{

	// マルチスレッドで実行する
	if (dispMatchingRunSingleCore == 0) {
		// ブロック輝度を取得する
		getBandBlockBrightnessContrast(imghgt, imgwdt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			crstthr, crstofs, grdcrct, pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst);
		// 視差を取得する
		getBandDisparity(imghgt, imgwdt, depth, brkwdt, extcnf,
			crstthr, crstofs, grdcrct, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, pblkbkdsp);
	}
	// シングルスレッドで実行する
	else {
		// ブロック輝度を取得する
		getWholeBlockBrightnessContrast(imghgt, imgwdt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			crstthr, crstofs, grdcrct, pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst);
		// 視差を取得する
		getWholeDisparity(imghgt, imgwdt, depth, brkwdt, extcnf,
			crstthr, crstofs, grdcrct, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, pblkbkdsp);
	}

}


/// <summary>
/// 視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
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
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getMatchingDisparity16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned short *pimgref, unsigned short *pimgcmp, float *pblkdsp, float *pblkbkdsp,
	int *pimgrefbrt, int *pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst)
{

	// マルチスレッドで実行する
	if (dispMatchingRunSingleCore == 0) {
		// ブロック輝度を取得する
		getBandBlockBrightnessContrast16U(imghgt, imgwdt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			crstthr, crstofs, grdcrct,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst);
		// 視差を取得する
		getBandDisparity16U(imghgt, imgwdt, depth, brkwdt, extcnf,
			crstthr, crstofs, grdcrct, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, pblkbkdsp);
	}
	// シングルスレッドで実行する
	else {
		// ブロック輝度を取得する
		getWholeBlockBrightnessContrast16U(imghgt, imgwdt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			crstthr, crstofs, grdcrct,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst);
		// 視差を取得する
		getWholeDisparity16U(imghgt, imgwdt, depth, brkwdt, extcnf,
			crstthr, crstofs, grdcrct, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, pblkbkdsp);
	}

	}


/// <summary>
/// 画像全体のブロック輝度とコントラストを取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
void StereoMatching::getWholeBlockBrightnessContrast(int imghgt, int imgwdt, int stphgt, int stpwdt,
	int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
	unsigned char* pimgref, unsigned char* pimgcmp,
	int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst)
{
	// ブロック輝度とコントラストを取得する
	getBlockBrightnessContrastInBand(imghgt, imgwdt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
		crstthr, crstofs, grdcrct,
		pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, 0, imghgt);

}


/// <summary>
/// 画像全体のブロック輝度とコントラストを取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
void StereoMatching::getWholeBlockBrightnessContrast16U(int imghgt, int imgwdt, int stphgt, int stpwdt,
	int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
	unsigned short* pimgref, unsigned short* pimgcmp,
	int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst)
{
	// ブロック輝度とコントラストを取得する
	getBlockBrightnessContrastInBand16U(imghgt, imgwdt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
		crstthr, crstofs, grdcrct,
		pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, 0, imghgt);

}


/// <summary>
/// 画像全体の視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
void StereoMatching::getWholeDisparity(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float *pblkdsp, float *pblkbkdsp)
{
	if (pblkbkdsp == NULL) {
		getDisparityInBand(imghgt, imgwdt, depth, brkwdt, extcnf,
			crstthr, crstofs, grdcrct, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, 0, imghgt);
	}
	else {
		getBothDisparityInBand(imghgt, imgwdt, depth, 
			crstthr, crstofs, grdcrct, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, pblkbkdsp, 0, imghgt);
	}

}


/// <summary>
/// 画像全体の視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getWholeDisparity16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned short *pimgref, unsigned short *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float *pblkdsp, float *pblkbkdsp)
{

	if (pblkbkdsp == NULL) {
		getDisparityInBand16U(imghgt, imgwdt, depth, brkwdt, extcnf,
			crstthr, crstofs, grdcrct, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, 0, imghgt);
	}
	else {
		getBothDisparityInBand16U(imghgt, imgwdt, depth,
			crstthr, crstofs, grdcrct, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, pblkbkdsp, 0, imghgt);
	}

}


/// <summary>
/// バンド内の比較画像のブロック輝度とコントラストを取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
void StereoMatching::getBlockBrightnessContrastInBand(int imghgt, int imgwdt, int stphgt, int stpwdt,
	int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
	unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, 
	int* pblkrefcrst, int* pblkcmpcrst,
	int jstart, int jend)
{
	// jpx : マッチングブロックのy座標
	// ipx : マッチングブロックのx座標
	for (int jpx = jstart; jpx < jend && jpx <= (imghgt - blkhgt); jpx++) {
		for (int ipx = 0; ipx <= (imgwdt - blkwdt); ipx++) {
			// ブロック輝度を求める
			getBlockBrightnessContrast(ipx, jpx, imghgt, imgwdt, stphgt, stpwdt, blkhgt, blkwdt,
				imghgtblk, imgwdtblk, crstthr, crstofs, grdcrct,
				pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst);
		}
	}

}


/// <summary>
/// バンド内の比較画像のブロック輝度とコントラストを取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getBlockBrightnessContrastInBand16U(int imghgt, int imgwdt, int stphgt, int stpwdt,
	int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
	unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int *pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst,
	int jstart, int jend)
{
	// jpx : マッチングブロックのy座標
	// ipx : マッチングブロックのx座標
	for (int jpx = jstart; jpx < jend && jpx <= (imghgt - blkhgt); jpx++) {
		for (int ipx = 0; ipx <= (imgwdt - blkwdt); ipx++) {
			// ブロック輝度を求める
			getBlockBrightnessContrast16U(ipx, jpx, imghgt, imgwdt, stphgt, stpwdt, blkhgt, blkwdt,
				imghgtblk, imgwdtblk, crstthr, crstofs, grdcrct,
				pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst);
		}
	}

}


/// <summary>
/// バンド内の視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param> 
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
void StereoMatching::getDisparityInBand(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float * pblkdsp,
	int jstart, int jend)
{
	// jpx : マッチングブロックのy座標
	// ipx : マッチングブロックのx座標
	for (int jpx = jstart; jpx < jend && jpx <= (imghgt - blkhgt); jpx++) {
		for (int ipx = 0; ipx <= (imgwdt - brkwdt - blkwdt); ipx++) {
			// SSDにより視差値を求める
			getDisparityBySSD(ipx, jpx, imghgt, imgwdt, depth, extcnf, crstthr, minbrtrt,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp);

		}
	}

}


/// <summary>
/// バンド内の視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getDisparityInBand16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned short *pimgref, unsigned short *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float * pblkdsp,
	int jstart, int jend)
{

	// jpx : マッチングブロックのy座標
	// ipx : マッチングブロックのx座標
	for (int jpx = jstart; jpx < jend && jpx <= (imghgt - blkhgt); jpx++) {
		for (int ipx = 0; ipx <= (imgwdt - brkwdt - blkwdt); ipx++) {
			// SSDにより視差値を求める
			getDisparityBySSD16U(ipx, jpx, imghgt, imgwdt, depth, extcnf, crstthr, minbrtrt,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp);
		}
	}

}


/// <summary>
/// 画像のブロック輝度とコントラストを取得する
/// </summary>
/// <param name="x">ブロック左上画素x座標(IN)</param>
/// <param name="y">ブロック左上画素y座標(IN)</param>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">視差ブロックの高さ(IN)</param>
/// <param name="stpwdt">視差ブロックの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
void StereoMatching::getBlockBrightnessContrast(int x, int y, int imghgt, int imgwdt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	int crstthr, int crstofs, int grdcrct,
	unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst)
{

	// 視差画像の幅
	int jpx = y;
	int ipx = x;

	// ブロック幅を引いた残り幅
	// ブロックを1画素ずつずらしす
	int remwdt = imgwdt - ipx - blkwdt;

	if (remwdt < 0 || jpx % stphgt != 0) {
		return;
	}

	// ブロック内の輝度差の最小値
	int mindltl = BLOCK_MIN_DELTA_BRIGHTNESS;

	// マッチングブロックの画素数
	int blkcnt = blkhgt * blkwdt;

	// 画像ブロックサイズ内の輝度値の総和を求める
	int sumr = 0; // Σ基準画像の輝度ij
	int Lsumr = 0; // Σ基準画像（階調補正）の輝度ij
	int Lminr = 255; // ブロック内輝度最小値（初期値8ビット階調最大輝度値）
	int Lmaxr = 0; // ブロック内輝度最大値（初期値8ビット階調最小輝度値）

	int sumc = 0; // Σ比較画像の輝度ij
	int Lsumc = 0; // Σ比較画像（階調補正）の輝度ij
	int Lminc = 255; // ブロック内輝度最小値（初期値8ビット階調最大輝度値）
	int Lmaxc = 0; // ブロック内輝度最大値（初期値8ビット階調最小輝度値）

	int jpxe = jpx + blkhgt;
	int ipxe = ipx + blkwdt;

	// 画素位置を設定する
	int idx = jpx * imgwdt + ipx;

	for (int j = jpx; j < jpxe; j++) {
		int idxj = j * imgwdt;
		for (int i = ipx; i < ipxe; i++) {
			int rfx = pimgref[idxj + i];
			int cpx = pimgcmp[idxj + i];
			sumr += rfx;
			sumc += cpx;
			unsigned int xrfx = rfx * rfx;
			unsigned int xcpx = cpx * cpx;
			if (grdcrct == 1) {
				rfx = xrfx / 255;
				cpx = xcpx / 255;
			}
			Lsumr += rfx;
			if (Lminr > rfx) {
				Lminr = rfx;
			}
			if (Lmaxr < rfx) {
				Lmaxr = rfx;
			}
			Lsumc += cpx;
			if (Lminc > cpx) {
				Lminc = cpx;
			}
			if (Lmaxc < cpx) {
				Lmaxc = cpx;
			}
		}
	}

	// ブロック輝度（画素単位）を保存する
	pimgrefbrt[idx] = sumr;
	pimgcmpbrt[idx] = sumc;

	// コントラストを求める
	int crstr = 0;
	int crstc = 0;
	// ブロック内の輝度差
	int deltaLr = Lmaxr - Lminr;
	int deltaLc = Lmaxc - Lminc;
	// コントラスト値を算出する
	// 以下の場合はコントラスト値はゼロ
	// コントラスト閾値がゼロ
	// ブロック内の輝度差が閾値未満
	// ブロック平均輝度がゼロ
	if (crstthr > 0 && deltaLr >= mindltl && Lsumr > 0) {
		crstr = (deltaLr * 1000 - crstofs) * blkcnt / Lsumr;
	}
	if (crstthr > 0 && deltaLc >= mindltl && Lsumc > 0) {
		crstc = (deltaLc * 1000 - crstofs) * blkcnt / Lsumc;
	}
	// コントラストを保存する
	pblkrefcrst[idx] = crstr;
	pblkcmpcrst[idx] = crstc;


}


/// <summary>
/// 画像のブロック輝度とコントラストを取得する
/// </summary>
/// <param name="x">ブロック左上画素x座標(IN)</param>
/// <param name="y">ブロック左上画素y座標(IN)</param>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">視差ブロックの高さ(IN)</param>
/// <param name="stpwdt">視差ブロックの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getBlockBrightnessContrast16U(int x, int y, int imghgt, int imgwdt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	int crstthr, int crstofs, int grdcrct,
	unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst)
{

	// 視差画像の幅
	int jpx = y;
	int ipx = x;

	// ブロック幅を引いた残り幅
	// ブロックを1画素ずつずらしす
	int remwdt = imgwdt - ipx - blkwdt;

	if (remwdt < 0 || jpx % stphgt != 0) {
		return;
	}

	// ブロック内の輝度差の最小値
	int mindltl = BLOCK_MIN_DELTA_BRIGHTNESS * 16;

	// マッチングブロックの画素数
	int blkcnt = blkhgt * blkwdt;

	// 画像ブロックサイズ内の輝度値の総和を求める
	int sumr = 0; // Σ基準画像の輝度ij
	int Lsumr = 0; // Σ基準画像（階調補正）の輝度ij
	int Lminr = 4095; // ブロック内輝度最小値（初期値12ビット階調最大輝度値）
	int Lmaxr = 0; // ブロック内輝度最大値（初期値12ビット階調最小輝度値）

	int sumc = 0; // Σ比較画像の輝度ij
	int Lsumc = 0; // Σ比較画像（階調補正）の輝度ij
	int Lminc = 4095; // ブロック内輝度最小値（初期値12ビット階調最大輝度値）
	int Lmaxc = 0; // ブロック内輝度最大値（初期値12ビット階調最小輝度値）

	int jpxe = jpx + blkhgt;
	int ipxe = ipx + blkwdt;

	// 画素位置を設定する
	int idx = jpx * imgwdt + ipx;

	for (int j = jpx; j < jpxe; j++) {
		int idxj = j * imgwdt;
		for (int i = ipx; i < ipxe; i++) {
			int rfx = pimgref[idxj + i];
			int cpx = pimgcmp[idxj + i];
			sumr += rfx;
			sumc += cpx;
			unsigned int xrfx = rfx * rfx;
			unsigned int xcpx = cpx * cpx;
			if (grdcrct == 1) {
				rfx = xrfx / 4095;
				cpx = xcpx / 4095;
			}
			Lsumr += rfx;
			if (Lminr > rfx) {
				Lminr = rfx;
			}
			if (Lmaxr < rfx) {
				Lmaxr = rfx;
			}
			Lsumc += cpx;
			if (Lminc > cpx) {
				Lminc = cpx;
			}
			if (Lmaxc < cpx) {
				Lmaxc = cpx;
			}
		}
	}

	// ブロック輝度（画素単位）を保存する
	pimgrefbrt[idx] = sumr;
	pimgcmpbrt[idx] = sumc;

	// コントラストを求める
	int crstr = 0;
	int crstc = 0;
	// ブロック内の輝度差
	int deltaLr = Lmaxr - Lminr;
	int deltaLc = Lmaxc - Lminc;
	// コントラスト値を算出する
	// 以下の場合はコントラスト値はゼロ
	// コントラスト閾値がゼロ
	// ブロック内の輝度差が閾値未満
	// ブロック平均輝度がゼロ
	if (crstthr > 0 && deltaLr >= mindltl && Lsumr > 0) {
		crstr = (deltaLr * 1000 - crstofs * 16) * blkcnt / Lsumr;
	}
	if (crstthr > 0 && deltaLc >= mindltl && Lsumc > 0) {
		crstc = (deltaLc * 1000 - crstofs * 16) * blkcnt / Lsumc;
	}
	// コントラストを保存する
	pblkrefcrst[idx] = crstr;
	pblkcmpcrst[idx] = crstc;


}


/// <summary>
/// SSDにより視差を取得する
/// </summary>
/// <param name="x">ブロック左上画素x座標(IN)</param>
/// <param name="y">ブロック左上画素y座標(IN)</param>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)<param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)<param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
void StereoMatching::getDisparityBySSD(int x, int y, int imghgt, int imgwdt, 
	int depth, int extcnf, int crstthr, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char* pimgref, unsigned char* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst,
	float* pblkdsp)
{
	// 視差画像の幅
	int jpx = y;
	int ipx = x;

	// ブロック幅を引いた残り幅
	int remwdt = imgwdt - ipx - blkwdt;
	if (remwdt <= 0 || jpx % stphgt != 0 || ipx % stpwdt != 0) {
		return;
	}

	// 一致度閾値
	// 一画素当たりの差分
	float pxdthr = 0.0f;
	// 一画素当たりの差分
	// 拡張マッチング領域の先頭で最大、終端で最小にする
	// 差分最小値
	float pxdmin = 6.0f;
	// 一致度閾値
	unsigned int sumthr;

	// 拡張マッチング幅
	// 拡張領域でない場合はゼロ
	int extmtcwdt = 0;

	// 拡張マッチング領域に入った場合
	// 探索幅以上あれば探索幅のまま
	// そうでない場合はそれが探索幅になる
	if (remwdt < depth) {
		// 拡張マッチング信頼限界がゼロの場合は信頼度を評価しない
		if (extcnf > 0) {
			// 拡張マッチング幅
			extmtcwdt = remwdt;
			// 一画素当たりの差分を求める
			// マッチング幅が狭くなるほど差分を小さくする
			pxdthr = pxdmin + (float)extcnf * extmtcwdt / depth;
		}
		// 探索残り幅を探索幅に設定する
		depth = remwdt;
	}

	// jblk : マッチングブロックのyインデックス
	// iblk : マッチングブロックのxインデックス
	int jblk = jpx / stphgt;
	int iblk = ipx / stpwdt;

	// SSDを保存する配列
	// 配列サイズはマッチング探索幅
	unsigned int ssd[ISC_IMG_DEPTH_MAX];
	// マッチングブロックの画素数
	int blkcnt = blkhgt * blkwdt;
	// SSD最大値を設定
	// 8ビット階調
	unsigned int maxsum = 255 * 255 * blkcnt;
	unsigned int misum = maxsum;

	// 視差値の初期値を設定する
	int disp = 0;

	// 画素位置を設定する
	int idx = jpx * imgwdt + ipx;
	// ブロック位置を設定する
	int bidx = jblk * imgwdtblk + iblk;

	// マッチング評価関数
	// 左右画像の同じ位置iにある輝度の差の二乗を求め，それを領域全体に渡って足し合わせる
	// SSD = Σ(((右画像の輝度ij - 右画像ブロック輝度平均) - (左画像の輝度ij - 左画像ブロック輝度平均))^2) 
	// 右画像を1画素ずつ探索範囲DEPTHまで右へ動かしてSSDを計算する
	// 一番小さいSSDのとき，最も類似している
	// その探索位置を視差値（整数部）とする
	// 前後のSSDからサブピクセル値（小数部）を算出する
	int jpxe = jpx + blkhgt;
	int ipxe = ipx + blkwdt;
	// 基準画像のブロック輝度を取得する
	unsigned int sumr = pimgrefbrt[idx]; // Σ基準画像の輝度ij
	unsigned int sumrr = 0; // Σ(基準画像の輝度ij^2)
	// コントラストを取得する
	int crst = pblkrefcrst[idx];

	// コントラストが閾値未満の場合は視差値ゼロにする
	if (crst < crstthr) {
		// 視差値ゼロにする
		pblkdsp[bidx] = 0.0f;
		return;
	}

	// 拡張マッチング
	// 一致度閾値を求める
	// 一致度は明るさに依存する
	// 一画素当たりの差分閾値からブロックの閾値を求める
	// 差分は明るさに比例する
	// SSDはブロック内の差分の二乗和
	// 8ビット階調対応
	pxdthr = pxdthr * sumr / blkcnt / 255;
	sumthr = (unsigned int)(pxdthr * pxdthr * blkcnt);

	for (int k = 0; k < depth; k++) {
		// 比較画像のブロックコントラストを取得する
		int crstc = pblkcmpcrst[idx + k];
		// コントラストが閾値未満の場合はスキップにする
		if (crstc < crstthr) {
			// SSDに最大値を設定する
			ssd[k] = maxsum;
			continue;
		}

		// 比較画像のブロック輝度を取得する
		unsigned int sumc = pimgcmpbrt[idx + k]; // Σ比較画像の輝度ij
		// ブロックの輝度差が閾値を超えた場合はスキップする
		// 暗い側のブロック輝度閾値=明るい側のブロック輝度xロック最低輝度比率
		unsigned int highbrt;
		unsigned int minbrt;
		unsigned int lowbrt;
		if (sumc > sumr) {
			highbrt = sumc;
			lowbrt = sumr;
		}
		else {
			highbrt = sumr;
			lowbrt = sumc;
		}
		minbrt = (highbrt * minbrtrt) / 100;

		if (lowbrt < minbrt) {
			// SSDに最大値を設定する
			ssd[k] = maxsum;
			continue;
		}
		sumrr = 0; // Σ(基準画像の輝度ij^2)
		unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
		unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
	for (int j = jpx; j < jpxe; j++) {
			int idxj = j * imgwdt;
		for (int i = ipx; i < ipxe; i++) {
				int idxi = idxj + i;
				unsigned int rfx = pimgref[idxi];
				unsigned int cpx = pimgcmp[idxi + k];
				sumrr += rfx * rfx;
				sumcc += cpx * cpx;
				sumrc += rfx * cpx;
			}
		}

		// SSDを算出する
		unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;

		// 右画像を1画素ずつ右へ動かしてSSDを計算する
		// 一番小さいSSDのとき，最も類似している
		ssd[k] = sumsq;
		if (sumsq < misum) {
			misum = sumsq;
			disp = k;
			}
			}

	// 視差値が1未満の場合は視差値ゼロにする
	// 視差値が探索幅の上限に達していた場合は視差値ゼロにする
	// 拡張マッチングの場合、一致度が閾値を超えた場合に視差値ゼロにする
	if (disp < 1 || disp >= (depth - 1) || (extmtcwdt > 0 && misum > sumthr)) {
		// 視差値ゼロにする
		pblkdsp[bidx] = 0.0f;
	}
	else {
		// 前ブロックのSSDが未算出の場合
		if (ssd[disp - 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + disp - 1]; // Σ比較画像の輝度ij
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj + disp - 1;
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgref[idxj + i];
					unsigned int cpx = pimgcmp[idxjdsp + i];
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
			ssd[disp - 1] = sumsq;
		}
		// 後ブロックのSSDが未算出の場合
		if (ssd[disp + 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + disp + 1]; // Σ比較画像の輝度ij
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj + disp + 1;
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgref[idxj + i];
					unsigned int cpx = pimgcmp[idxjdsp + i];
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
			ssd[disp + 1] = sumsq;
			}

		// サブピクセル推定
		// 折れ線近似
		// Xsub = (S(-1) - S(1)) / (2 x S(-1) - 2 x S(0)) S(-1) > S(1)
		// Xsub = (S(1) - S(-1)) / (2 x S(1) - 2 x S(0)) S(-1) ≦ S(1)
		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		// 折れ線+放物線近似
		// Xsub = (S(-1) - S(1)) / (S(-1) - S(0) - S(-1) + S(2))
		// Xsub = (S(1) - S(-1)) / (S(-2) - S(-1) - S(0) + S(1))
		int ssdprv = ssd[disp - 1];
		int ssdcnt = ssd[disp];
		int ssdnxt = ssd[disp + 1];
		// 放物線で近似する
		// 中ブロックのSSDが最小になっている場合
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {
			// サブピクセルを算出する
			float sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);
			pblkdsp[bidx] = disp + sub;
		}
		else {
			// 視差値ゼロにする
			pblkdsp[bidx] = 0.0f;
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
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)<param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getDisparityBySSD16U(int x, int y, int imghgt, int imgwdt,
	int depth, int extcnf, int crstthr, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned short* pimgref, unsigned short* pimgcmp, int* pimgrefbrt, int* pimgcmpbrt, int* pblkrefcrst, int* pblkcmpcrst,
	float* pblkdsp)
{

	// 視差画像の幅
	int jpx = y;
	int ipx = x;

	// ブロック幅を引いた残り幅
	int remwdt = imgwdt - ipx - blkwdt;
	if (remwdt <= 0 || jpx % stphgt != 0 || ipx % stpwdt != 0) {
		return;
	}

	// 一致度閾値
	// 一画素当たりの差分
	float pxdthr = 0.0f;
	// 一画素当たりの差分
	// 拡張マッチング領域の先頭で最大、終端で最小にする
	// 差分最小値
	float pxdmin = 6.0f;
	// 一致度閾値
	unsigned int sumthr;
	// 拡張マッチング幅
	// 拡張領域でない場合はゼロ
	int extmtcwdt = 0;

	// 拡張マッチング領域に入った場合
	// 探索幅以上あれば探索幅のまま
	// そうでない場合はそれが探索幅になる
	if (remwdt < depth) {
		// 拡張マッチング信頼限界がゼロの場合は信頼度を評価しない
		if (extcnf > 0) {
			// 拡張マッチング幅
			extmtcwdt = remwdt;
			// 一画素当たりの差分を求める
			// マッチング幅が狭くなるほど差分を小さくする
			// 12ビット階調対応
			pxdthr = (pxdmin + (float)extcnf * extmtcwdt / depth) * 16;
		}
		// 探索残り幅を探索幅に設定する
		depth = remwdt;
		}

	// jblk : マッチングブロックのyインデックス
	// iblk : マッチングブロックのxインデックス
	int jblk = jpx / stphgt;
	int iblk = ipx / stpwdt;

	// SSDを保存する配列
	// 配列サイズはマッチング探索幅
	unsigned int ssd[ISC_IMG_DEPTH_MAX];
	// マッチングブロックの画素数
	int blkcnt = blkhgt * blkwdt;
	// SSD最大値を設定
	// 12ビット階調対応
	unsigned int maxsum = 4095 * 4095 * blkcnt;
	unsigned int misum = maxsum;

	// 視差値の初期値を設定する
	int disp = 0;

	// 画素位置を設定する
	int idx = jpx * imgwdt + ipx;
	// ブロック位置を設定する
	int bidx = jblk * imgwdtblk + iblk;

	// マッチング評価関数
	// 左右画像の同じ位置iにある輝度の差の二乗を求め，それを領域全体に渡って足し合わせる
	// SSD = Σ(((右画像の輝度ij - 右画像ブロック輝度平均) - (左画像の輝度ij - 左画像ブロック輝度平均))^2) 
	// 右画像を1画素ずつ探索範囲DEPTHまで右へ動かしてSSDを計算する
	// 一番小さいSSDのとき，最も類似している
	// その探索位置を視差値（整数部）とする
	// 前後のSSDからサブピクセル値（小数部）を算出する
	int jpxe = jpx + blkhgt;
	int ipxe = ipx + blkwdt;

	//	unsigned int sumr = 0; // Σ基準画像の輝度ij
	// 基準画像のブロック輝度を取得する
	unsigned int sumr = pimgrefbrt[idx]; // Σ基準画像の輝度ij
	unsigned int sumrr = 0; // Σ(基準画像の輝度ij^2)

	// コントラストを取得する
	int crst = pblkrefcrst[idx];
	// コントラストが閾値未満の場合は視差値ゼロにする
	if (crst < crstthr) {
		// 視差値ゼロにする
		pblkdsp[bidx] = 0.0f;
		return;
	}

	// 拡張マッチング
	// 一致度閾値を求める
	// 一致度は明るさに依存する
	// 一画素当たりの差分閾値からブロックの閾値を求める
	// 差分は明るさに比例する
	// 12ビット階調対応
	pxdthr = pxdthr * sumr / blkcnt / 4095;
	sumthr = (unsigned int)(pxdthr * pxdthr * blkcnt);

	for (int k = 0; k < depth; k++) {
		// 比較画像のブロックコントラストを取得する
		int crstc = pblkcmpcrst[idx + k];
		// コントラストが閾値未満の場合はスキップにする
		if (crstc < crstthr) {
			// SSDに最大値を設定する
			ssd[k] = maxsum;
			continue;
		}

		// 比較画像のブロック輝度を取得する
		unsigned int sumc = pimgcmpbrt[idx + k]; // Σ比較画像の輝度ij
		// ブロックの輝度差が閾値を超えた場合はスキップする
		// 暗い側のブロック輝度閾値=明るい側のブロック輝度xロック最低輝度比率
		unsigned int highbrt;
		unsigned int minbrt;
		unsigned int lowbrt;
		if (sumc > sumr) {
			highbrt = sumc;
			lowbrt = sumr;
		}
		else {
			highbrt = sumr;
			lowbrt = sumc;
		}
		minbrt = (highbrt * minbrtrt) / 100;
		if (lowbrt < minbrt) {
			// SSDに最大値を設定する
			ssd[k] = maxsum;
			continue;
		}
		sumrr = 0; // Σ(基準画像の輝度ij^2)
		unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
		unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
		for (int j = jpx; j < jpxe; j++) {
			int idxj = j * imgwdt;
			for (int i = ipx; i < ipxe; i++) {
				int idxi = idxj + i;
				unsigned int rfx = pimgref[idxi];
				unsigned int cpx = pimgcmp[idxi + k];
				sumrr += rfx * rfx;
				sumcc += cpx * cpx;
				sumrc += rfx * cpx;
			}
		}
		unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;

		// 右画像を1画素ずつ右へ動かしてSSDを計算する
		// 一番小さいSSDのとき，最も類似している
		ssd[k] = sumsq;
		if (sumsq < misum) {
			misum = sumsq;
			disp = k;
		}
	}

	// 視差値が1未満の場合は視差値ゼロにする
	// 視差値が探索幅の上限に達していた場合は視差値ゼロにする
	// 拡張マッチングの場合、一致度が閾値を超えた場合に視差値ゼロにする
	if (disp < 1 || disp >= (depth - 1) || (extmtcwdt > 0 && misum > sumthr)) {
		// 視差値ゼロにする
		pblkdsp[bidx] = 0.0f;
	}
	else {
		// 前ブロックのSSDが未算出の場合
		if (ssd[disp - 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + disp - 1]; // Σ比較画像の輝度ij
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj + disp - 1;
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgref[idxj + i];
					unsigned int cpx = pimgcmp[idxjdsp + i];
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
			ssd[disp - 1] = sumsq;
		}

		// 後ブロックのSSDが未算出の場合
		if (ssd[disp + 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + disp + 1]; // Σ比較画像の輝度ij
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj + disp + 1;
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgref[idxj + i];
					unsigned int cpx = pimgcmp[idxjdsp + i];
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
			ssd[disp + 1] = sumsq;
		}

		// サブピクセル推定
		// 折れ線近似
		// Xsub = (S(-1) - S(1)) / (2 x S(-1) - 2 x S(0)) S(-1) > S(1)
		// Xsub = (S(1) - S(-1)) / (2 x S(1) - 2 x S(0)) S(-1) ≦ S(1)
		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		// 折れ線+放物線近似
		// Xsub = (S(-1) - S(1)) / (S(-1) - S(0) - S(-1) + S(2))
		// Xsub = (S(1) - S(-1)) / (S(-2) - S(-1) - S(0) + S(1))
		float ssdprv = (float)ssd[disp - 1]; // S(-1)
		float ssdcnt = (float)ssd[disp]; // S(0)
		float ssdnxt = (float)ssd[disp + 1]; // S(1)

		// 放物線で近似する
		// 中ブロックのSSDが最小になっている場合
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {
			// サブピクセルを算出する
			float sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);
			pblkdsp[bidx] = disp + sub;
		}
		else {
			// 視差値ゼロにする
			pblkdsp[bidx] = 0.0f;
		}
	}

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
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
void StereoMatching::getBothDisparityInBand(int imghgt, int imgwdt, int depth, 
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float *pblkdsp, float *pblkbkdsp,
	int jstart, int jend)
{

	// jpx : マッチングブロックのy座標
	// ipx : マッチングブロックのx座標
	for (int jpx = jstart; jpx < jend && jpx <= (imghgt - blkhgt); jpx++) {
		for (int ipx = 0; ipx <= (imgwdt - blkwdt); ipx++) {
			// SSDにより視差値を求める
			getBothDisparityBySSD(ipx, jpx, imghgt, imgwdt, depth, crstthr, crstofs, grdcrct, minbrtrt,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, pblkbkdsp);
		}
	}

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
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <param name="jstart">バンド開始ブロック位置(IN)</param>
/// <param name="jend">バンド終了ブロック位置(IN)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getBothDisparityInBand16U(int imghgt, int imgwdt, int depth,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned short *pimgref, unsigned short *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float *pblkdsp, float *pblkbkdsp,
	int jstart, int jend)
{

	// jpx : マッチングブロックのy座標
	// ipx : マッチングブロックのx座標
	for (int jpx = jstart; jpx < jend && jpx <= (imghgt - blkhgt); jpx++) {
		for (int ipx = 0; ipx <= (imgwdt - blkwdt); ipx++) {
			// SSDにより視差値を求める
			getBothDisparityBySSD16U(ipx, jpx, imghgt, imgwdt, depth, crstthr, crstofs, grdcrct, minbrtrt,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				pimgref, pimgcmp, pimgrefbrt, pimgcmpbrt, pblkrefcrst, pblkcmpcrst, pblkdsp, pblkbkdsp);
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
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
void StereoMatching::getBothDisparityBySSD(int x, int y, int imghgt, int imgwdt, int depth,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float * pblkdsp, float * pblkbkdsp)
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

	// SSDを保存する配列
	// 配列サイズはマッチング探索幅
	unsigned int ssd[ISC_IMG_DEPTH_MAX];
	unsigned int bk_ssd[ISC_IMG_DEPTH_MAX];

	// マッチングブロックの画素数
	int blkcnt = blkhgt * blkwdt;

	// SSD最大値を設定
	unsigned int maxsum = 255 * 255 * blkcnt;
	unsigned int misum = maxsum;
	unsigned int bk_misum = maxsum;

	// 視差値の初期値を設定する
	int disp = 0;
	int bk_disp = 0;

	// 画素位置を設定する
	int idx = jpx * imgwdt + ipx;
	// ブロック位置を設定する
	int bidx = jblk * imgwdtblk + iblk;

	// マッチング評価関数
	// 左右画像の同じ位置iにある輝度の差の二乗を求め，それを領域全体に渡って足し合わせる
	// SSD = Σ(((右画像の輝度ij - 右画像ブロック輝度平均) - (左画像の輝度ij - 左画像ブロック輝度平均))^2) 
	// 右画像を1画素ずつ探索範囲DEPTHまで右へ動かしてSSDを計算する
	// 一番小さいSSDのとき，最も類似している
	// その探索位置を視差値（整数部）とする
	// 前後のSSDからサブピクセル値（小数部）を算出する
	int jpxe = jpx + blkhgt;
	int ipxe = ipx + blkwdt;

	// フォアマッチング
	unsigned int sumr = pimgrefbrt[idx]; // Σ基準画像の輝度ij
	unsigned int sumrr = 0; // Σ(基準画像の輝度ij^2)

	// バックマッチング
	unsigned int bk_sumr = pimgcmpbrt[idx]; // Σ基準画像の輝度ij
	unsigned int bk_sumrr = 0; // Σ(基準画像の輝度ij^2)

	// コントラストを取得する
	// フォアマッチング　基準画像
	int crst = pblkrefcrst[idx];

	// バックマッチング
	int bk_crst = pblkcmpcrst[idx];

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

			// フォアマッチング
	if (crst >= crstthr) {
		// フォアマッチング
		for (int k = 0; k < fr_depth; k++) {
			// 比較画像のブロックコントラストを取得する
			int crstc = pblkcmpcrst[idx + k];
			// コントラストが閾値未満の場合はスキップにする
			if (crstc < crstthr) {
				// SSDに最大値を設定する
				ssd[k] = maxsum;
				continue;
			}

			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + k]; // Σ比較画像の輝度ij
			// ブロックの輝度差が閾値を超えた場合はスキップする
			// 暗い側のブロック輝度閾値=明るい側のブロック輝度xロック最低輝度比率
			unsigned int highbrt;
			unsigned int minbrt;
			unsigned int lowbrt;
			if (sumc > sumr) {
				highbrt = sumc;
				lowbrt = sumr;
			}
			else {
				highbrt = sumr;
				lowbrt = sumc;
			}
			minbrt = (highbrt * minbrtrt) / 100;

			if (lowbrt < minbrt) {
				// SSDに最大値を設定する
				ssd[k] = maxsum;
				continue;
			}
			sumrr = 0;
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				for (int i = ipx; i < ipxe; i++) {
					int idxi = idxj + i;
					unsigned int rfx = pimgref[idxi];
					unsigned int cpx = pimgcmp[idxi + k];
					sumrr += rfx * rfx;
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
			}
			}
			// 右画像を1画素ずつ右へ動かしてSSDを計算する
			// 一番小さいSSDのとき，最も類似している
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc)
				- (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;

			ssd[k] = sumsq;
			if (sumsq < misum) {
				misum = sumsq;
				disp = k;
			}
		}
	}

	// バックマッチング
	if (bk_crst >= crstthr) {
			// バックマッチング
		for (int k = 0; k < bk_depth; k++) {
			// 比較画像のブロックコントラストを取得する
			int bk_crstc = pblkrefcrst[idx - k];
			// コントラストが閾値未満の場合はスキップにする
			if (bk_crstc < crstthr) {
				// SSDに最大値を設定する
				bk_ssd[k] = maxsum;
				continue;
			}

			// 比較画像のブロック輝度を取得する
			unsigned int bk_sumc = pimgrefbrt[idx - k]; // Σ比較画像の輝度ij
			// ブロックの輝度差が閾値を超えた場合はスキップする
			// 暗い側のブロック輝度閾値=明るい側のブロック輝度xロック最低輝度比率
			unsigned int highbrt;
			unsigned int minbrt;
			unsigned int lowbrt;
			if (bk_sumc > bk_sumr) {
				highbrt = bk_sumc;
				lowbrt = bk_sumr;
			}
			else {
				highbrt = bk_sumr;
				lowbrt = bk_sumc;
			}
			minbrt = (highbrt * minbrtrt) / 100;

			if (lowbrt < minbrt) {
				// SSDに最大値を設定する
				bk_ssd[k] = maxsum;
				continue;
			}
			bk_sumrr = 0;
			unsigned int bk_sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int bk_sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				for (int i = ipx; i < ipxe; i++) {
					int idxi = idxj + i;
					unsigned int rfx = pimgcmp[idxi];
					unsigned int cpx = pimgref[idxi - k];
					bk_sumrr += rfx * rfx;
					bk_sumcc += cpx * cpx;
					bk_sumrc += rfx * cpx;
			}
			}
			// 右画像を1画素ずつ右へ動かしてSSDを計算する
			// 一番小さいSSDのとき，最も類似している
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) -
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;

			bk_ssd[k] = bk_sumsq;
			if (bk_sumsq < bk_misum) {
				bk_misum = bk_sumsq;
				bk_disp = k;
			}
		}
	}

	// 視差値が探索開始スキップ幅：1未満の場合はサブピクセルを算出しない
	// 視差値が探索幅の上限に達していた場合はサブピクセルを算出しない
	// 視差値が1未満の場合は視差値ゼロにする
	// 視差値が探索幅の上限に達していた場合は視差値ゼロにする
	// コントラストが閾値未満の場合は視差値ゼロにする
	float sub;
	int ssdprv;
	int ssdcnt;
	int ssdnxt;

	// フォアマッチング
	if (fr_depth < 3 || disp < 1 || disp >= (fr_depth - 1)) {
		pblkdsp[bidx] = 0.0f;
	}
	else {
		// 前ブロックのSSDが未算出の場合
		if (ssd[disp - 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + disp - 1]; // Σ比較画像の輝度ij
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj + disp - 1;
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgref[idxj + i];
					unsigned int cpx = pimgcmp[idxjdsp + i];
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
			ssd[disp - 1] = sumsq;
		}

		// 後ブロックのSSDが未算出の場合
		if (ssd[disp + 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + disp + 1]; // Σ比較画像の輝度ij
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj + disp + 1;
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgref[idxj + i];
					unsigned int cpx = pimgcmp[idxjdsp + i];
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
			ssd[disp + 1] = sumsq;
		}

		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		ssdprv = ssd[disp - 1];
		ssdcnt = ssd[disp];
		ssdnxt = ssd[disp + 1];
		// 中ブロックのSSDが最小になっている場合
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {
			// サブピクセルを算出する
			sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);
			// 視差値を保存する
			pblkdsp[bidx] = disp + sub;
		}
		else {
			// 視差値ゼロにする
			pblkdsp[bidx] = 0.0f;
		}
		}

	// バックマッチング
	if (bk_depth >= 3 && bk_disp >= 1 && bk_disp < (bk_depth - 1)) {
		// 前ブロックのSSDが未算出の場合
		if (bk_ssd[bk_disp - 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int bk_sumc = pimgrefbrt[idx - (bk_disp - 1)]; // Σ比較画像の輝度ij
			unsigned int bk_sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int bk_sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj - (bk_disp - 1);
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgcmp[idxj + i];
					unsigned int cpx = pimgref[idxjdsp + i];
					bk_sumcc += cpx * cpx;
					bk_sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) -
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;
			bk_ssd[bk_disp - 1] = bk_sumsq;
		}

		// 後ブロックのSSDが未算出の場合
		// 8ビット階調対応
		if (bk_ssd[bk_disp + 1] == maxsum /* 255 * 255 * blkcnt */ ) {
			// 比較画像のブロック輝度を取得する
			unsigned int bk_sumc = pimgrefbrt[idx - (bk_disp + 1)]; // Σ比較画像の輝度ij
			unsigned int bk_sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int bk_sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj - (bk_disp + 1);
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgcmp[idxj + i];
					unsigned int cpx = pimgref[idxjdsp + i];
					bk_sumcc += cpx * cpx;
					bk_sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) - 
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;
			bk_ssd[bk_disp + 1] = bk_sumsq;
		}
		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		ssdprv = bk_ssd[bk_disp - 1];
		ssdcnt = bk_ssd[bk_disp];
		ssdnxt = bk_ssd[bk_disp + 1];
		// 中ブロックのSSDが最小になっている場合
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {
			// 放物線近似
			sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);
			float bk_disp_sub = bk_disp + sub;
			// バックマッチングの結果を基準画像の座標へ展開
			// 視差ブロック番号
			int bk_iblk = (int)((ipx - bk_disp_sub) / stpwdt);
			// 視差値を保存する
			pblkbkdsp[jblk * imgwdtblk + bk_iblk] = bk_disp_sub;
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
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getBothDisparityBySSD16U(int x, int y, int imghgt, int imgwdt, int depth,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned short *pimgref, unsigned short *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float * pblkdsp, float * pblkbkdsp)
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

	// SSDを保存する配列
	// 配列サイズはマッチング探索幅
	unsigned int ssd[ISC_IMG_DEPTH_MAX];
	unsigned int bk_ssd[ISC_IMG_DEPTH_MAX];

	// マッチングブロックの画素数
	int blkcnt = blkhgt * blkwdt;

	// SSD最大値を設定
	unsigned int maxsum = 4095 * 4095 * blkcnt;
	unsigned int misum = maxsum;
	unsigned int bk_misum = maxsum;

	// 視差値の初期値を設定する
	int disp = 0;
	int bk_disp = 0;

	// 画素位置を設定する
	int idx = jpx * imgwdt + ipx;
	// ブロック位置を設定する
	int bidx = jblk * imgwdtblk + iblk;

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
	unsigned int sumr = pimgrefbrt[idx]; // Σ基準画像の輝度ij
	unsigned int sumrr = 0; // Σ(基準画像の輝度ij^2)

	// バックマッチング
	unsigned int bk_sumr = pimgcmpbrt[idx]; // Σ基準画像の輝度ij
	unsigned int bk_sumrr = 0; // Σ(基準画像の輝度ij^2)

	// コントラストを求める
	// フォアマッチング
	int crst = pblkrefcrst[idx];
	// バックマッチング
	int bk_crst = pblkcmpcrst[idx];

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

		// フォアマッチング
	if (crst >= crstthr) {
		// フォアマッチング
		for (int k = 0; k < fr_depth; k++) {
			// 比較画像のブロックコントラストを取得する
			int crstc = pblkcmpcrst[idx + k];
			// コントラストが閾値未満の場合はスキップにする
			if (crstc < crstthr) {
				// SSDに最大値を設定する
				ssd[k] = maxsum;
				continue;
			}

			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + k]; // Σ比較画像の輝度ij
			// ブロックの輝度差が閾値を超えた場合はスキップする
			// 暗い側のブロック輝度閾値=明るい側のブロック輝度xロック最低輝度比率
			unsigned int highbrt;
			unsigned int minbrt;
			unsigned int lowbrt;
			if (sumc > sumr) {
				highbrt = sumc;
				lowbrt = sumr;
			}
			else {
				highbrt = sumr;
				lowbrt = sumc;
			}
			minbrt = (highbrt * minbrtrt) / 100;

			if (lowbrt < minbrt) {
				ssd[k] = maxsum;
				continue;
			}
			sumrr = 0; // Σ(基準画像の輝度ij^2)
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
		for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
			for (int i = ipx; i < ipxe; i++) {
					int idxi = idxj + i;
					unsigned int rfx = pimgref[idxi];
					unsigned int cpx = pimgcmp[idxi + k];
					sumrr += rfx * rfx;
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
		}
		// 右画像を1画素ずつ右へ動かしてSSDを計算する
		// 一番小さいSSDのとき，最も類似している
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc)
				- (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;

		ssd[k] = sumsq;
		if (sumsq < misum) {
			misum = sumsq;
			disp = k;
		}
		}
	}

	// バックマッチング
	if (bk_crst >= crstthr) {
		// バックマッチング
		for (int k = 0; k < bk_depth; k++) {
			// 比較画像のブロックコントラストを取得する
			int bk_crstc = pblkrefcrst[idx - k];
			// コントラストが閾値未満の場合はスキップにする
			if (bk_crstc < crstthr) {
				// SSDに最大値を設定する
				bk_ssd[k] = maxsum;
				continue;
			}

			// 比較画像のブロック輝度を取得する
			unsigned int bk_sumc = pimgrefbrt[idx - k]; // Σ比較画像の輝度ij
			// ブロックの輝度差が閾値を超えた場合はスキップする
			// 暗い側のブロック輝度閾値=明るい側のブロック輝度xロック最低輝度比率
			unsigned int highbrt;
			unsigned int minbrt;
			unsigned int lowbrt;
			if (bk_sumc > bk_sumr) {
				highbrt = bk_sumc;
				lowbrt = bk_sumr;
			}
			else {
				highbrt = bk_sumr;
				lowbrt = bk_sumc;
			}
			minbrt = (highbrt * minbrtrt) / 100;

			if (lowbrt < minbrt) {
				bk_ssd[k] = maxsum;
				continue;
			}
			bk_sumrr = 0;
			unsigned int bk_sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int bk_sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				for (int i = ipx; i < ipxe; i++) {
					int idxi = idxj + i;
					unsigned int rfx = pimgcmp[idxi];
					unsigned int cpx = pimgref[idxi - k];
					bk_sumrr += rfx * rfx;
					bk_sumcc += cpx * cpx;
					bk_sumrc += rfx * cpx;
				}
			}
			// 右画像を1画素ずつ右へ動かしてSSDを計算する
			// 一番小さいSSDのとき，最も類似している
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) 
				- (bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;

		bk_ssd[k] = bk_sumsq;
		if (bk_sumsq < bk_misum) {
			bk_misum = bk_sumsq;
			bk_disp = k;
		}
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
	if (fr_depth < 3 || disp < 1 || disp >= (fr_depth - 1)) {
		// 視差値ゼロにする
		pblkdsp[bidx] = 0.0;
	}
	else {

		// 前ブロックのSSDが未算出の場合
		if (ssd[disp - 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + disp - 1]; // Σ比較画像の輝度ij
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj + disp - 1;
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgref[idxj + i];
					unsigned int cpx = pimgcmp[idxjdsp + i];
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
			ssd[disp - 1] = sumsq;
		}

		// 後ブロックのSSDが未算出の場合
		if (ssd[disp + 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int sumc = pimgcmpbrt[idx + disp + 1]; // Σ比較画像の輝度ij
			unsigned int sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj + disp + 1;
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgref[idxj + i];
					unsigned int cpx = pimgcmp[idxjdsp + i];
					sumcc += cpx * cpx;
					sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;
			ssd[disp + 1] = sumsq;
		}

		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		ssdprv = (float)ssd[disp - 1];
		ssdcnt = (float)ssd[disp];
		ssdnxt = (float)ssd[disp + 1];

		// 中ブロックのSSDが最小になっている場合
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {
		// 放物線近似
		sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);
		// 視差値を保存する
			pblkdsp[bidx] = disp + sub;
	}
		else {
			// 視差値ゼロにする
			pblkdsp[bidx] = 0.0f;
		}
	}

	// バックマッチング
	if (bk_depth >= 3 && bk_disp >= 1 && bk_disp < (bk_depth - 1)) {
		// 前ブロックのSSDが未算出の場合
		if (bk_ssd[bk_disp - 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int bk_sumc = pimgrefbrt[idx - (bk_disp - 1)]; // Σ比較画像の輝度ij
			unsigned int bk_sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int bk_sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj - (bk_disp - 1);
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgcmp[idxj + i];
					unsigned int cpx = pimgref[idxjdsp + i];
					bk_sumcc += cpx * cpx;
					bk_sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) -
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;
			bk_ssd[bk_disp - 1] = bk_sumsq;
		}

		// 後ブロックのSSDが未算出の場合
		if (bk_ssd[bk_disp + 1] == maxsum) {
			// 比較画像のブロック輝度を取得する
			unsigned int bk_sumc = pimgrefbrt[idx - (bk_disp + 1)]; // Σ比較画像の輝度ij
			unsigned int bk_sumcc = 0; // Σ(比較画像の輝度ij^2)
			unsigned int bk_sumrc = 0; // Σ(基準画像の輝度ij*比較画像の輝度ij)
			for (int j = jpx; j < jpxe; j++) {
				int idxj = j * imgwdt;
				int idxjdsp = idxj - (bk_disp + 1);
				for (int i = ipx; i < ipxe; i++) {
					unsigned int rfx = pimgcmp[idxj + i];
					unsigned int cpx = pimgref[idxjdsp + i];
					bk_sumcc += cpx * cpx;
					bk_sumrc += rfx * cpx;
				}
			}
			// SSDを算出する
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) -
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;
			bk_ssd[bk_disp + 1] = bk_sumsq;
		}

		// 放物線近似
		// Xsub = (S(1) - S(-1)) / (2 x S(-1) - 4 x S(0) + 2 x S(1))
		ssdprv = (float)bk_ssd[bk_disp - 1];
		ssdcnt = (float)bk_ssd[bk_disp];
		ssdnxt = (float)bk_ssd[bk_disp + 1];

		// 中ブロックのSSDが最小になっている場合
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {
		// 放物線近似
		sub = (ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);
		float bk_disp_sub = bk_disp + sub;
		// バックマッチングの結果を基準画像の座標へ展開
		// 視差ブロック番号
		int bk_iblk = (int)((ipx - bk_disp_sub) / stpwdt);
		// 視差値を保存する
		pblkbkdsp[jblk * imgwdtblk + bk_iblk] = bk_disp_sub;
	}
	}

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
/// <param name="frmgain">画像フレームのセンサーゲイン値(IN)</param>
/// <param name="pblkdsp">ブロックごとの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
void StereoMatching::executeMatchingOpenCL(int imghgt, int imgwdt, int depth,
	unsigned char *pimgref, unsigned char *pimgcmp, int frmgain,
	float * pblkdsp, int *pblkcrst)
{

	// 視差ブロックの高さ
	int stphgt = disparityBlockHeight;
	// 視差ブロックの幅
	int stpwdt = disparityBlockWidth;

	// マッチングブロック最低輝度比率(%)
	int minbrtrt = matchingMinBrightRatio;

	// マッチング探索打ち切り幅
	int brkwdt = depth;
	// 拡張マッチングの場合
	if (matchingExtension == 1) {
		brkwdt = matchingExtLimitWidth;
	}
	// 拡張マッチング信頼限界
	int extcnf = matchingExtConfidenceLimit;
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
		}
	else if (imgwdt == IMG_WIDTH_2K) {
		crstofs = (int)CONTRAST_OFFSET_2K;
	}
	else if (imgwdt == IMG_WIDTH_4K) {
		crstofs = (int)CONTRAST_OFFSET_4K;
	}

	if (crstthr != 0) {
		// ゲインによるコントラスト差分を加える
		crstthr = crstthr + (int)(frmgain * CONTRAST_DIFF_GAIN_RT * 1000);
		// ゲインによるコントラストオフセットを加える
		crstofs = crstofs + (int)(frmgain * CONTRAST_OFFSET_GAIN_RT * 1000);
	}

	// 重複マッチング除去
	int rmvdup = removeDuplicateMatching;

	// 比較画像の視差位置をクリアする
	memset(dsp_posi, 0, imghgt * imgwdt * sizeof(int));

	// 入力補正画像データのMatを生成する
	cv::Mat inputImageRef(imghgt, imgwdt, CV_8UC1, pimgref);
	cv::Mat inputImageCmp(imghgt, imgwdt, CV_8UC1, pimgcmp);
	// 出力視差画像データのMatを生成する
	// 視差値出力はブロックごと
	// サイズを最大画像サイズにしおく
	cv::Mat outputDisp(imghgt, imgwdt, CV_32FC1, pblkdsp);
	// 画像コントラストデータのMatを生成する
	cv::Mat outputRefCrst(imghgt, imgwdt, CV_32SC1, pblkcrst);
	cv::Mat outputCmpCrst(imghgt, imgwdt, CV_32SC1, cmp_block_crst);

	// 入力補正画像データのUMatを生成する
	cv::UMat inputUMatImageRef(imghgt, imgwdt, CV_8UC1);
	cv::UMat inputUMatImageCmp(imghgt, imgwdt, CV_8UC1);
	// 出力視差画像データのUMatを生成する
	cv::UMat outputUMatDisp(imghgt, imgwdt, CV_32FC1);
	// 画像コントラストデータのUMatを生成する
	cv::UMat outputUMatRefCrst(imghgt, imgwdt, CV_32SC1);
	cv::UMat outputUMatCmpCrst(imghgt, imgwdt, CV_32SC1);

	// 入力補正画像データをUMatへコピーする
	inputImageRef.copyTo(inputUMatImageRef);
	inputImageCmp.copyTo(inputUMatImageCmp);

	// ブロック輝度データのUMatを生成する
	cv::UMat outputUMatImgRefBrt(imghgt, imgwdt, CV_32SC1, cv::Scalar(0));
	cv::UMat outputUMatImgCmpBrt(imghgt, imgwdt, CV_32SC1, cv::Scalar(0));

	// 画像のブロック輝度とコントラストを求める
	getBlockBrightnessContrastOpenCL(imghgt, imgwdt,
		stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk, crstthr, crstofs, grdcrct,
		inputUMatImageRef, inputUMatImageCmp,
		outputUMatImgRefBrt, outputUMatImgCmpBrt, outputUMatRefCrst, outputUMatCmpCrst);

	if (enableBackMatching == 0) {
		// 遮蔽幅を設定する
		shadeWidth = brkwdt;

		// SSDにより視差値を求める
		getDisparityBySSDOpenCL(imghgt, imgwdt, depth, brkwdt, extcnf,
			crstthr, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			inputUMatImageRef, inputUMatImageCmp, outputUMatImgRefBrt, outputUMatImgCmpBrt, outputUMatRefCrst, outputUMatCmpCrst,
			outputUMatDisp);

		// 出力視差データをMatへコピーする
		outputUMatDisp.copyTo(outputDisp);
		// 画像コントラストをMatへコピーする
		outputUMatRefCrst.copyTo(outputRefCrst);

		// 重複マッチング除去の場合
		if (rmvdup == 1) {
			// ブロック輝度データを出力するMatを生成する
			cv::Mat outputImgRefBrt(imghgt, imgwdt, CV_32SC1, ref_block_brt);
			cv::Mat outputImgCmpBrt(imghgt, imgwdt, CV_32SC1, cmp_block_brt);
			// ブロック輝度データをMatへコピーする
			outputUMatImgRefBrt.copyTo(outputImgRefBrt);
			outputUMatImgCmpBrt.copyTo(outputImgCmpBrt);

			removeDuplicateBlock(imghgt, imgwdt,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				ref_block_brt, cmp_block_brt, pblkdsp, dsp_posi);
		}
	}
	else {
		shadeWidth = 0;

		float *pblkbkdsp = bk_block_dsp;
		// バックマッチング出力視差画像データのMatを生成する
		cv::Mat outputBkDisp(imghgt, imgwdt, CV_32FC1, pblkbkdsp);
		// 出力視差画像データのUMatを生成する
		cv::UMat outputUMatBkDisp(imghgt, imgwdt, CV_32FC1, cv::Scalar(0));

		// 両方向マッチングによる視差を取得する
		getBothDisparityBySSDOpenCL(imghgt, imgwdt, depth, 
			crstthr, minbrtrt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			inputUMatImageRef, inputUMatImageCmp, outputUMatImgRefBrt, outputUMatImgCmpBrt, outputUMatRefCrst, outputUMatCmpCrst,
			outputUMatDisp, outputUMatBkDisp);

		// 出力視差データをMatへコピーする
		outputUMatDisp.copyTo(outputDisp);
		// バックマッチング出力視差データをMatへコピーする
		outputUMatBkDisp.copyTo(outputBkDisp);
		// 出力視差マスクをMatへコピーする
		outputUMatRefCrst.copyTo(outputRefCrst);

		blendBothMatchingDisparity(imghgt, imgwdt, imghgtblk, imgwdtblk,
			backMatchingEvaluationWidth, backMatchingEvaluationRange, backMatchingValidRatio, backMatchingZeroRatio,
			pblkdsp, pblkbkdsp);
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
/// <param name="frmgain">画像フレームのセンサーゲイン値(IN)</param>
/// <param name="pblkdsp">ブロックごとの視差値(OUT)</param>
/// <param name="pblkcrst">ブロックコントラスト(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::executeMatchingOpenCL16U(int imghgt, int imgwdt, int depth,
	unsigned short *pimgref, unsigned short *pimgcmp, int frmgain,
	float * pblkdsp, int *pblkcrst)
{

	// 視差ブロックの高さ
	int stphgt = disparityBlockHeight;
	// 視差ブロックの幅
	int stpwdt = disparityBlockWidth;

	// マッチングブロック最低輝度比率(%)
	int minbrtrt = matchingMinBrightRatio;

	// マッチング探索打ち切り幅
	int brkwdt = depth;
	// 拡張マッチングの場合
	if (matchingExtension == 1) {
		brkwdt = matchingExtLimitWidth;
	}
	// 拡張マッチング信頼限界
	int extcnf = matchingExtConfidenceLimit;
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
	}
	else if (imgwdt == IMG_WIDTH_2K) {
		crstofs = (int)CONTRAST_OFFSET_2K;
	}
	else if (imgwdt == IMG_WIDTH_4K) {
		crstofs = (int)CONTRAST_OFFSET_4K;
	}

	if (crstthr != 0) {
		// ゲインによるコントラスト差分を加える
		crstthr = crstthr + (int)(frmgain * CONTRAST_DIFF_GAIN_RT * 1000);
		// ゲインによるコントラストオフセットを加える
		crstofs = crstofs + (int)(frmgain * CONTRAST_OFFSET_GAIN_RT * 1000);
	}

	// 重複マッチング除去
	int rmvdup = removeDuplicateMatching;

	// 比較画像の視差位置をクリアする
	memset(dsp_posi, 0, imghgt * imgwdt * sizeof(int));

	// 入力補正画像データのMatを生成する
	cv::Mat inputImageRef(imghgt, imgwdt, CV_16UC1, pimgref);
	cv::Mat inputImageCmp(imghgt, imgwdt, CV_16UC1, pimgcmp);
	// 出力視差画像データのMatを生成する
	// 視差値出力はブロックごと
	// サイズを最大画像サイズにしおく
	cv::Mat outputDisp(imghgt, imgwdt, CV_32FC1, pblkdsp);
	// 画像コントラストデータのMatを生成する
	cv::Mat outputRefCrst(imghgt, imgwdt, CV_32SC1, pblkcrst);
	cv::Mat outputCmpCrst(imghgt, imgwdt, CV_32SC1, cmp_block_crst);

	// 比較画像の視差位置のMatを生成する
	cv::Mat outputPosi(imghgt, imgwdt, CV_32SC1, dsp_posi);
	// 入力補正画像データのUMatを生成する
	cv::UMat inputUMatImageRef(imghgt, imgwdt, CV_16UC1);
	cv::UMat inputUMatImageCmp(imghgt, imgwdt, CV_16UC1);
	// 出力視差画像データのUMatを生成する
	cv::UMat outputUMatDisp(imghgt, imgwdt, CV_32FC1);
	// 画像コントラストデータのUMatを生成する
	cv::UMat outputUMatRefCrst(imghgt, imgwdt, CV_32SC1);
	cv::UMat outputUMatCmpCrst(imghgt, imgwdt, CV_32SC1);

	// 入力補正画像データをUMatへコピーする
	inputImageRef.copyTo(inputUMatImageRef);
	inputImageCmp.copyTo(inputUMatImageCmp);

	// ブロック輝度データのUMatを生成する
	cv::UMat outputUMatImgRefBrt(imghgt, imgwdt, CV_32SC1, cv::Scalar(0));
	cv::UMat outputUMatImgCmpBrt(imghgt, imgwdt, CV_32SC1, cv::Scalar(0));

	// 画像のブロック輝度とコントラストを求める
	getBlockBrightnessContrastOpenCL16U(imghgt, imgwdt,
		stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk, crstthr, crstofs, grdcrct,
		inputUMatImageRef, inputUMatImageCmp,
		outputUMatImgRefBrt, outputUMatImgCmpBrt, outputUMatRefCrst, outputUMatCmpCrst);

	if (enableBackMatching == 0) {

		// 遮蔽幅を設定する
		shadeWidth = brkwdt;

		// SSDにより視差値を求める
		getDisparityBySSDOpenCL16U(imghgt, imgwdt, depth, brkwdt, extcnf,
			crstthr, minbrtrt,
			stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			inputUMatImageRef, inputUMatImageCmp, outputUMatImgRefBrt, outputUMatImgCmpBrt, outputUMatRefCrst, outputUMatCmpCrst,
			outputUMatDisp);

		// 出力視差データをMatへコピーする
		outputUMatDisp.copyTo(outputDisp);
		// 画像コントラストをMatへコピーする
		outputUMatRefCrst.copyTo(outputRefCrst);

		// 重複マッチング除去の場合
		if (rmvdup == 1) {
			// ブロック輝度データを出力するMatを生成する
			cv::Mat outputImgRefBrt(imghgt, imgwdt, CV_32SC1, ref_block_brt);
			cv::Mat outputImgCmpBrt(imghgt, imgwdt, CV_32SC1, cmp_block_brt);
			// ブロック輝度データをMatへコピーする
			outputUMatImgRefBrt.copyTo(outputImgRefBrt);
			outputUMatImgCmpBrt.copyTo(outputImgCmpBrt);

			removeDuplicateBlock(imghgt, imgwdt,
				stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
				ref_block_brt, cmp_block_brt, pblkdsp, dsp_posi);
		}
	}
	else {
		shadeWidth = 0;

		float *pblkbkdsp = bk_block_dsp;
		// バックマッチング出力視差画像データのMatを生成する
		cv::Mat outputBkDisp(imghgt, imgwdt, CV_32FC1, pblkbkdsp);
		// 出力視差画像データのUMatを生成する
		cv::UMat outputUMatBkDisp(imghgt, imgwdt, CV_32FC1, cv::Scalar(0));

		// 両方向マッチングによる視差を取得する
		getBothDisparityBySSDOpenCL16U(imghgt, imgwdt, depth,
			crstthr, minbrtrt, stphgt, stpwdt, blkhgt, blkwdt, imghgtblk, imgwdtblk,
			inputUMatImageRef, inputUMatImageCmp, outputUMatImgRefBrt, outputUMatImgCmpBrt, outputUMatRefCrst, outputUMatCmpCrst,
			outputUMatDisp, outputUMatBkDisp);

		// 出力視差データをMatへコピーする
		outputUMatDisp.copyTo(outputDisp);
		// バックマッチング出力視差データをMatへコピーする
		outputUMatBkDisp.copyTo(outputBkDisp);
		// 出力視差マスクをMatへコピーする
		outputUMatRefCrst.copyTo(outputRefCrst);

		blendBothMatchingDisparity(imghgt, imgwdt, imghgtblk, imgwdtblk,
			backMatchingEvaluationWidth, backMatchingEvaluationRange, backMatchingValidRatio, backMatchingZeroRatio,
			pblkdsp, pblkbkdsp);

	}

}


/// <summary>
/// ブロック輝度とコントラストを算出するカーネルプログラム
/// </summary>
static char* kernelGetBlockBrightnessContrast = (char*)"__kernel void kernelGetBlockBrightnessContrast(\n\
	int imghgt, int imgwdt,\n\
	int stphgt, int	stpwdt, int blkhgt, int blkwdt,\n\
	int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,\n\
	__global uchar* imgref, int imgref_step, int imgref_offset,\n\
	__global uchar* imgcmp, int imgcmp_step, int imgcmp_offset,\n\
	__global int* imgrefbrt, int imgrefbrt_step, int imgrefbrt_offset,\n\
	int height, int width,\n\
	__global int* imgcmpbrt, int imgcmpbrt_step, int imgcmpbrt_offset,\n\
	int imgcmpbrt_hgt, int imgcmpbrt_wdt,\n\
	__global int* blkrefcrst, int blkrefcrst_step, int blkrefcrst_offset,\n\
	int blkrefcrst_hgt, int blkrefcrst_wdt,\n\
	__global int* blkcmpcrst, int blkcmpcrst_step, int blkcmpcrst_offset,\n\
	int blkcmpcrst_hgt, int blkcmpcrst_wdt)\n\
{\n\
	int x = get_global_id(0);\n\
	int y = get_global_id(1);\n\
	if (x >= width || y >= height) {\n\
		return; \n\
	}\n\
	int jpx = y;\n\
	int ipx = x;\n\
	int remwdt = imgwdt - ipx - blkwdt;\n\
	if (remwdt < 0 || jpx % stphgt != 0) {\n\
		return;\n\
	}\n\
	int sumr = 0;\n\
	int sumc = 0;\n\
	int Lsumr = 0;\n\
	int Lminr = 255;\n\
	int Lmaxr = 0;\n\
	int Lsumc = 0;\n\
	int Lminc = 255;\n\
	int Lmaxc = 0;\n\
	int jpxe = jpx + blkhgt;\n\
	int ipxe = ipx + blkwdt;\n\
	int idx = jpx * imgwdt + ipx;\n\
	for (int j = jpx; j < jpxe; j++) {\n\
		int idxj = j * imgwdt;\n\
		for (int i = ipx; i < ipxe; i++) {\n\
			int rpx = imgref[idxj + i];\n\
			int cpx = imgcmp[idxj + i];\n\
			sumr += rpx;\n\
			sumc += cpx;\n\
			int xrpx = rpx * rpx;\n\
			int xcpx = cpx * cpx;\n\
			if (grdcrct == 1) {\n\
				rpx = xrpx / 255;\n\
				cpx = xcpx / 255;\n\
			}\n\
			Lsumr += rpx;\n\
			if (Lminr > rpx) {\n\
				Lminr = rpx;\n\
			}\n\
			if (Lmaxr < rpx) {\n\
				Lmaxr = rpx;\n\
			}\n\
			Lsumc += cpx;\n\
			if (Lminc > cpx) {\n\
				Lminc = cpx;\n\
			}\n\
			if (Lmaxc < cpx) {\n\
				Lmaxc = cpx;\n\
			}\n\
		}\n\
	}\n\
	imgrefbrt[idx] = sumr;\n\
	imgcmpbrt[idx] = sumc;\n\
	int blkcnt = blkhgt * blkwdt;\n\
	int crstr = 0;\n\
	int crstc = 0;\n\
	int deltaLr = Lmaxr - Lminr;\n\
	int deltaLc = Lmaxc - Lminc;\n\
	if (crstthr > 0 && deltaLr >= 3 && Lsumr > 0) {\n\
		crstr = (deltaLr * 1000 - crstofs) * blkcnt / Lsumr;\n\
	}\n\
	if (crstthr > 0 && deltaLc >= 3 && Lsumc > 0) {\n\
		crstc = (deltaLc * 1000 - crstofs) * blkcnt / Lsumc;\n\
	}\n\
	blkrefcrst[idx] = crstr;\n\
	blkcmpcrst[idx] = crstc;\n\
}";

/// <summary>
/// OpenCLコンテキストの初期化フラグ
/// </summary>
static bool openCLBrightnessContrastContextInit = false;

/// <summary>
/// OpenCLコンテキスト
/// </summary>
static cv::ocl::Context contextBrightnessContrast;

/// <summary>
/// カーネルプログラム
/// </summary>
static cv::ocl::Program kernelProgramBrightnessContrast;

/// <summary>
/// カーネルオブジェクト
/// </summary>
static cv::ocl::Kernel kernelObjectBrightnessContrast;

/// <summary>
/// glaobalWorkSize
/// </summary>
static size_t globalSizeBrightnessContrast[2];


/// <summary>
/// 画像のブロック輝度とコントラストを取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">視差ブロックの高さ(IN)</param>
/// <param name="stpwdt">視差ブロックの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="imgref">入力基準画像データUMat(IN)</param>
/// <param name="imgcmp">入力比較画像データUMat(IN)</param>
/// <param name="imgrefbrt">比較画像のブロック輝度UMat(OUT)</param>
/// <param name="imgcmpbrt">比較画像のブロック輝度UMat(OUT)</param>
/// <param name="blkrefcrst">基準ブロックコントラストUMat(OUT)</param>
/// <param name="blkcmpcrst">比較ブロックコントラストUMat(OUT)</param>
void StereoMatching::getBlockBrightnessContrastOpenCL(int imghgt, int imgwdt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
	cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst)
{
	bool success;

	// OpenCLのコンテキストを初期化する
	if (openCLBrightnessContrastContextInit == false) {

		// コンテキストを生成する
		// * コンテキストContextを保持する？ <<<<<<　Context　スタティック
		// clCreateContext
		// context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
		success = contextBrightnessContrast.create(cv::ocl::Device::TYPE_GPU);

		if (success == false) {
			OutputDebugString(_T("FALSE : context.create()\n"));
		}

		// デバイスを選択する
		// clGetDeviceIDs
		// clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
		cv::ocl::Device(contextBrightnessContrast.device(0));

		// カーネルソースを生成する
		// clCreateProgramWithSource
		// command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
		// program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
		cv::ocl::ProgramSource programSource(kernelGetBlockBrightnessContrast);

		cv::String errMsg;

		cv::String bldOpt = "";

		// カーネルプログラムをビルドする
		// * Programプログラムを保持する <<<<<<
		// clBuildProgram
		// ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
		kernelProgramBrightnessContrast = contextBrightnessContrast.getProg(programSource, bldOpt, errMsg);

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
		kernelObjectBrightnessContrast.create("kernelGetBlockBrightnessContrast", kernelProgramBrightnessContrast);

		// OpenCL初期化フラグをセットする
		openCLBrightnessContrastContextInit = true;

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
	kernelObjectBrightnessContrast.args(
		imghgt, // 1:入力補正画像の高さ
		imgwdt, // 2:入力補正画像の幅
		stphgt, // 3:マッチングステップの高さ
		stpwdt, // 4:マッチングステップの幅
		blkhgt, // 5:マッチングブロックの高さ
		blkwdt, // 6:マッチングブロックの幅
		imghgtblk, // 7:視差ブロック画像の高さ
		imgwdtblk, // 8:視差ブロック画像の幅
		crstthr, // 9:コントラスト閾値
		crstofs, // 10:コントラストオフセット
		grdcrct, // 11:階調補正モードステータス 0:オフ 1:オン
		cv::ocl::KernelArg::ReadOnlyNoSize(imgref),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmp),
		cv::ocl::KernelArg::ReadWrite(imgrefbrt),
		cv::ocl::KernelArg::ReadWrite(imgcmpbrt),
		cv::ocl::KernelArg::ReadWrite(blkrefcrst),
		cv::ocl::KernelArg::ReadWrite(blkcmpcrst)
	);

	// globalWorkSize : 行いたい処理の総数
	// localWorkSize : 並列に行いたい処理の数
	globalSizeBrightnessContrast[0] = (size_t)imgref.cols;
	globalSizeBrightnessContrast[1] = (size_t)imgref.rows;

	// カーネルを実行する
	// bool cv::ocl::Kernel::run(int dims, size_t globalsize[], size_t localsize[], bool sync, const Queue & q = Queue())
	//
	// clEnqueueTask
	// ret = clEnqueueNDRangeKernel(command_queue, kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	// ret = clEnqueueReadBuffer(command_queue, matrixRMemObj, CL_TRUE, 0, matrixRMemSize, matrixR, 0, NULL, NULL)
	// 
	success = kernelObjectBrightnessContrast.run(2, globalSizeBrightnessContrast, NULL, true);

	if (success == false) {
		OutputDebugString(_T("FALSE : kernel.run()\n"));
	}


}


/// <summary>
/// ブロック輝度とコントラストを算出するカーネルプログラム
/// </summary>
static char* kernelGetBlockBrightnessContrast16U = (char*)"__kernel void kernelGetBlockBrightnessContrast16U(\n\
	int imghgt, int imgwdt,\n\
	int stphgt, int	stpwdt, int blkhgt, int blkwdt,\n\
	int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,\n\
	__global short* imgref, int imgref_step, int imgref_offset,\n\
	__global short* imgcmp, int imgcmp_step, int imgcmp_offset,\n\
	__global int* imgrefbrt, int imgrefbrt_step, int imgrefbrt_offset,\n\
	int height, int width,\n\
	__global int* imgcmpbrt, int imgcmpbrt_step, int imgcmpbrt_offset,\n\
	int imgcmpbrt_hgt, int imgcmpbrt_wdt,\n\
	__global int* blkrefcrst, int blkrefcrst_step, int blkrefcrst_offset,\n\
	int blkrefcrst_hgt, int blkrefcrst_wdt,\n\
	__global int* blkcmpcrst, int blkcmpcrst_step, int blkcmpcrst_offset,\n\
	int blkcmpcrst_hgt, int blkcmpcrst_wdt)\n\
{\n\
	int x = get_global_id(0);\n\
	int y = get_global_id(1);\n\
	if (x >= width || y >= height) {\n\
		return;\n\
	}\n\
	int jpx = y;\n\
	int ipx = x;\n\
	int remwdt = imgwdt - ipx - blkwdt;\n\
	if (remwdt < 0 || jpx % stphgt != 0) {\n\
		return;\n\
	}\n\
	int sumr = 0;\n\
	int sumc = 0;\n\
	int Lsumr = 0;\n\
	int Lminr = 4095;\n\
	int Lmaxr = 0;\n\
	int Lsumc = 0;\n\
	int Lminc = 4095;\n\
	int Lmaxc = 0;\n\
	int jpxe = jpx + blkhgt;\n\
	int ipxe = ipx + blkwdt;\n\
	int idx = jpx * imgwdt + ipx;\n\
	for (int j = jpx; j < jpxe; j++) {\n\
		int idxj = j * imgwdt;\n\
		for (int i = ipx; i < ipxe; i++) {\n\
			int rpx = imgref[idxj + i];\n\
			int cpx = imgcmp[idxj + i];\n\
			sumr += rpx;\n\
			sumc += cpx;\n\
			int xrpx = rpx * rpx;\n\
			int xcpx = cpx * cpx;\n\
			if (grdcrct == 1) {\n\
				rpx = xrpx / 4095;\n\
				cpx = xcpx / 4095;\n\
			}\n\
			Lsumr += rpx;\n\
			if (Lminr > rpx) {\n\
				Lminr = rpx;\n\
			}\n\
			if (Lmaxr < rpx) {\n\
				Lmaxr = rpx;\n\
			}\n\
			Lsumc += cpx;\n\
			if (Lminc > cpx) {\n\
				Lminc = cpx;\n\
			}\n\
			if (Lmaxc < cpx) {\n\
				Lmaxc = cpx;\n\
			}\n\
			}\n\
			}\n\
	imgrefbrt[idx] = sumr;\n\
	imgcmpbrt[idx] = sumc;\n\
	int blkcnt = blkhgt * blkwdt;\n\
	int crstr = 0;\n\
	int crstc = 0;\n\
	int deltaLr = Lmaxr - Lminr;\n\
	int deltaLc = Lmaxc - Lminc;\n\
	if (crstthr > 0 && deltaLr >= 48 && Lsumr > 0) {\n\
		crstr = (deltaLr * 1000 - crstofs * 16) * blkcnt / Lsumr;\n\
	}\n\
	if (crstthr > 0 && deltaLc >= 48 && Lsumc > 0) {\n\
		crstc = (deltaLc * 1000 - crstofs * 16) * blkcnt / Lsumc;\n\
	}\n\
	blkrefcrst[idx] = crstr;\n\
	blkcmpcrst[idx] = crstc;\n\
}";

/// <summary>
/// OpenCLコンテキストの初期化フラグ
/// </summary>
static bool openCLBrightnessContrastContextInit16U = false;

/// <summary>
/// OpenCLコンテキスト
/// </summary>
static cv::ocl::Context contextBrightnessContrast16U;

/// <summary>
/// カーネルプログラム
/// </summary>
static cv::ocl::Program kernelProgramBrightnessContrast16U;

/// <summary>
/// カーネルオブジェクト
/// </summary>
static cv::ocl::Kernel kernelObjectBrightnessContrast16U;

/// <summary>
/// glaobalWorkSize
/// </summary>
static size_t globalSizeBrightnessContrast16U[2];


/// <summary>
/// 画像のブロック輝度とコントラストを取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">視差ブロックの高さ(IN)</param>
/// <param name="stpwdt">視差ブロックの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="imgref">入力基準画像データUMat(IN)</param>
/// <param name="imgcmp">入力比較画像データUMat(IN)</param>
/// <param name="imgrefbrt">比較画像のブロック輝度UMat(OUT)</param>
/// <param name="imgcmpbrt">比較画像のブロック輝度UMat(OUT)</param>
/// <param name="blkrefcrst">基準ブロックコントラストUMat(OUT)</param>
/// <param name="blkcmpcrst">比較ブロックコントラストUMat(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getBlockBrightnessContrastOpenCL16U(int imghgt, int imgwdt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
	cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst)
{
	bool success;

	// OpenCLのコンテキストを初期化する
	if (openCLBrightnessContrastContextInit16U == false) {

		// コンテキストを生成する
		// * コンテキストContextを保持する？ <<<<<<　Context　スタティック
		// clCreateContext
		// context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
		success = contextBrightnessContrast16U.create(cv::ocl::Device::TYPE_GPU);

		if (success == false) {
			OutputDebugString(_T("FALSE : context.create()\n"));
		}

		// デバイスを選択する
		// clGetDeviceIDs
		// clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
		cv::ocl::Device(contextBrightnessContrast16U.device(0));

		// カーネルソースを生成する
		// clCreateProgramWithSource
		// command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
		// program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
		cv::ocl::ProgramSource programSource(kernelGetBlockBrightnessContrast16U);

		cv::String errMsg;

		cv::String bldOpt = "";

		// カーネルプログラムをビルドする
		// * Programプログラムを保持する <<<<<<
		// clBuildProgram
		// ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
		kernelProgramBrightnessContrast16U = contextBrightnessContrast16U.getProg(programSource, bldOpt, errMsg);

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
		kernelObjectBrightnessContrast16U.create("kernelGetBlockBrightnessContrast16U", kernelProgramBrightnessContrast16U);

		// OpenCL初期化フラグをセットする
		openCLBrightnessContrastContextInit16U = true;

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
	kernelObjectBrightnessContrast16U.args(
		imghgt, // 1:入力補正画像の高さ
		imgwdt, // 2:入力補正画像の幅
		stphgt, // 3:マッチングステップの高さ
		stpwdt, // 4:マッチングステップの幅
		blkhgt, // 5:マッチングブロックの高さ
		blkwdt, // 6:マッチングブロックの幅
		imghgtblk, // 7:視差ブロック画像の高さ
		imgwdtblk, // 8:視差ブロック画像の幅
		crstthr, // 9:コントラスト閾値
		crstofs, // 10:コントラストオフセット
		grdcrct, // 11:階調補正モードステータス 0:オフ 1:オン
		cv::ocl::KernelArg::ReadOnlyNoSize(imgref),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmp),
		cv::ocl::KernelArg::ReadWrite(imgrefbrt),
		cv::ocl::KernelArg::ReadWrite(imgcmpbrt),
		cv::ocl::KernelArg::ReadWrite(blkrefcrst),
		cv::ocl::KernelArg::ReadWrite(blkcmpcrst)
	);

	// globalWorkSize : 行いたい処理の総数
	// localWorkSize : 並列に行いたい処理の数
	globalSizeBrightnessContrast16U[0] = (size_t)imgref.cols;
	globalSizeBrightnessContrast16U[1] = (size_t)imgref.rows;

	// カーネルを実行する
	// bool cv::ocl::Kernel::run(int dims, size_t globalsize[], size_t localsize[], bool sync, const Queue & q = Queue())
	//
	// clEnqueueTask
	// ret = clEnqueueNDRangeKernel(command_queue, kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	// ret = clEnqueueReadBuffer(command_queue, matrixRMemObj, CL_TRUE, 0, matrixRMemSize, matrixR, 0, NULL, NULL)
	// 
	success = kernelObjectBrightnessContrast16U.run(2, globalSizeBrightnessContrast16U, NULL, true);

	if (success == false) {
		OutputDebugString(_T("FALSE : kernel.run()\n"));
	}


}


/// <summary>
/// SSDで視差値を算出するカーネルプログラム
/// </summary>
/// <remarks>最大探索幅 : ISC_IMG_DEPTH_MAX 512</remarks>
/// <remarks>SSD最大値 : 16581375 8ビット階調(255) 16x16画素ブロック</remarks>
/// <remarks>ブロック内の輝度差の最小値 : BLOCK_MIN_DELTA_BRIGHTNESS 3</remarks>
static char* kernelGetDisparityBySSD = (char*)"__kernel void kernelGetDisparityBySSD(\n\
	int imghgt, int imgwdt, int depth, int brkwdt, int extcnf, int crstthr, int minbrtrt,\n\
	int stphgt, int	stpwdt, int blkhgt, int blkwdt,\n\
	int imghgtblk, int imgwdtblk,\n\
	__global uchar* imgref, int imgref_step, int imgref_offset,\n\
	__global uchar* imgcmp, int imgcmp_step, int imgcmp_offset,\n\
	__global int* imgrefbrt, int imgrefbrt_step, int imgrefbrt_offset,\n\
	__global int* imgcmpbrt, int imgcmpbrt_step, int imgcmpbrt_offset,\n\
	__global int* blkrefcrst, int blkrefcrst_step, int blkrefcrst_offset,\n\
	__global int* blkcmpcrst, int blkcmpcrst_step, int blkcmpcrst_offset,\n\
	__global float* blkdsp, int blkdsp_step, int blkdsp_offset,\n\
	int height, int width)\n\
{\n\
	int x = get_global_id(0);\n\
	int y = get_global_id(1);\n\
	if (x >= width || y >= height) {\n\
		return;\n\
	}\n\
	int imgwdtdsp = imgwdt - brkwdt - blkwdt;\n\
	int jpx = y;\n\
	int ipx = x;\n\
	int remwdt = imgwdt - ipx - blkwdt;\n\
	if (ipx > imgwdtdsp || remwdt <= 0 || jpx > (imghgt - blkhgt) || jpx % stphgt != 0 || ipx % stpwdt != 0) {\n\
		return;\n\
		}\n\
	float pxdthr = 0.0f;\n\
	float pxdmin = 6.0f;\n\
	unsigned int sumthr;\n\
	int extmtcwdt = 0;\n\
	if (remwdt < depth) {\n\
		if (extcnf > 0) {\n\
			extmtcwdt = remwdt;\n\
			pxdthr = pxdmin + (float)extcnf * extmtcwdt / depth;\n\
	}\n\
		depth = remwdt;\n\
		}\n\
	int jblk = jpx / stphgt;\n\
	int iblk = ipx / stpwdt;\n\
	unsigned int ssd[512];\n\
	int blkcnt = blkhgt * blkwdt;\n\
	unsigned int maxsum = 255 * 255 * blkcnt;\n\
	unsigned int misum = maxsum;\n\
	int idx = jpx * imgwdt + ipx;\n\
	int bidx = jblk * imgwdtblk + iblk;\n\
	int disp = 0;\n\
	int jpxe = jpx + blkhgt;\n\
	int ipxe = ipx + blkwdt;\n\
	unsigned int sumr = imgrefbrt[idx];\n\
	unsigned int sumrr = 0;\n\
	int crst = blkrefcrst[idx];;\n\
	if (crst < crstthr) {\n\
		blkdsp[bidx] = 0.0f;\n\
		return;\n\
	}\n\
	pxdthr = pxdthr * sumr / blkcnt / 255;\n\
	sumthr = (unsigned int)(pxdthr * pxdthr * blkcnt);\n\
	for (int k = 0; k < depth; k++) {\n\
		int crstc = blkcmpcrst[idx + k];\n\
		if (crstc < crstthr) {\n\
			ssd[k] = maxsum;\n\
			continue;\n\
		}\n\
		unsigned int sumc = (unsigned int)imgcmpbrt[idx + k];\n\
		unsigned int minbrt;\n\
		unsigned int lowbrt;\n\
		if (sumc > sumr) {\n\
			minbrt = (sumc * minbrtrt) / 100;\n\
			lowbrt = sumr;\n\
		}\n\
		else {\n\
			minbrt = (sumr * minbrtrt) / 100;\n\
			lowbrt = sumc;\n\
		}\n\
		if (lowbrt < minbrt) {\n\
			ssd[k] = maxsum;\n\
			continue;\n\
		}\n\
		sumrr = 0;\n\
		unsigned int sumcc = 0;\n\
		unsigned int sumrc = 0;\n\
		for (int j = jpx; j < jpxe; j++) {\n\
			int idxj = j * imgwdt;\n\
			for (int i = ipx; i < ipxe; i++) {\n\
				int idxi = idxj + i;\n\
				unsigned int rfx = imgref[idxi];\n\
				unsigned int cpx = imgcmp[idxi + k];\n\
				sumrr += rfx * rfx;\n\
				sumcc += cpx * cpx;\n\
				sumrc += rfx * cpx;\n\
			}\n\
		}\n\
		unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
		ssd[k] = sumsq;\n\
		if (sumsq < misum) {\n\
			misum = sumsq;\n\
			disp = k;\n\
		}\n\
	}\n\
	if (disp < 1 || disp >= (depth - 1)	||\n\
		(extmtcwdt > 0 && misum > sumthr)) {\n\
		blkdsp[bidx] = 0.0f;\n\
	}\n\
	else {\n\
		if (ssd[disp - 1] == maxsum) {\n\
			unsigned int sumc = imgcmpbrt[idx + disp - 1];\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj + disp - 1;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgref[idxj + i];\n\
					unsigned int cpx = imgcmp[idxjdsp + i];\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
			ssd[disp - 1] = sumsq;\n\
		}\n\
		if (ssd[disp + 1] == maxsum) {\n\
			unsigned int sumc = imgcmpbrt[idx + disp + 1];\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj + disp + 1;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgref[idxj + i];\n\
					unsigned int cpx = imgcmp[idxjdsp + i];\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
			ssd[disp + 1] = sumsq;\n\
		}\n\
		int ssdprv = ssd[disp - 1];\n\
		int ssdcnt = ssd[disp];\n\
		int ssdnxt = ssd[disp + 1];\n\
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {\n\
			float sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);\n\
			blkdsp[bidx] = disp + sub;\n\
		}\n\
		else {\n\
			blkdsp[bidx] = 0.0f;\n\
		}\n\
	}\n\
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
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="imgref">入力基準画像データUMat(IN)</param>
/// <param name="imgcmp">入力比較画像データUMat(IN)</param>
/// <param name="imgrefbrt">入力基準画像のブロック輝度データUMat(IN)</param>
/// <param name="imgcmpbrt">入力比較画像のブロック輝度データUMat(IN)</param>
/// <param name="blkrefcrst">基準ブロックコントラストUMat(IN)</param>
/// <param name="blkcmpcrst">比較ブロックコントラストUMat(IN)</param>
/// <param name="blkdsp">マッチングの視差値UMat(OUT)</param>
void StereoMatching::getDisparityBySSDOpenCL(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst,
	cv::UMat blkdsp)
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
			msg += errMsg.c_str();
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
		brkwdt, // 4:マッチング探索打ち切り幅
		extcnf, // 5:拡張マッチング信頼限界
		crstthr, // 6:コントラスト閾値
		minbrtrt, // 7:マッチングブロック最低輝度比率(%)
		stphgt, // 8:マッチングステップの高さ
		stpwdt, // 9:マッチングステップの幅
		blkhgt, // 10:マッチングブロックの高さ
		blkwdt, // 11:マッチングブロックの幅
		imghgtblk, // 12:視差ブロック画像の高さ
		imgwdtblk, // 13:視差ブロック画像の幅
		cv::ocl::KernelArg::ReadOnlyNoSize(imgref),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmp),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgrefbrt),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmpbrt),
		cv::ocl::KernelArg::ReadOnlyNoSize(blkrefcrst),
		cv::ocl::KernelArg::ReadOnlyNoSize(blkcmpcrst),
		cv::ocl::KernelArg::ReadWrite(blkdsp)
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
/// SSDで視差値を算出するカーネルプログラム
/// </summary>
/// <remarks>最大探索幅 : ISC_IMG_DEPTH_MAX 512</remarks>
/// <remarks>SSD最大値 : 1676902500 12ビット階調(4095) 10x10画素ブロック</remarks>
/// <remarks>ブロック内の輝度差の最小値 : BLOCK_MIN_DELTA_BRIGHTNESS 3</remarks>
static char* kernelGetDisparityBySSD16U = (char*)"__kernel void kernelGetDisparityBySSD16U(\n\
	int imghgt, int imgwdt, int depth, int brkwdt, int extcnf, int crstthr, int minbrtrt,\n\
	int stphgt, int	stpwdt, int blkhgt, int blkwdt,\n\
	int imghgtblk, int imgwdtblk,\n\
	__global short* imgref, int imgref_step, int imgref_offset,\n\
	__global short* imgcmp, int imgcmp_step, int imgcmp_offset,\n\
	__global int* imgrefbrt, int imgrefbrt_step, int imgrefbrt_offset,\n\
	__global int* imgcmpbrt, int imgcmpbrt_step, int imgcmpbrt_offset,\n\
	__global int* blkrefcrst, int blkrefcrst_step, int blkrefcrst_offset,\n\
	__global int* blkcmpcrst, int blkcmpcrst_step, int blkcmpcrst_offset,\n\
	__global float* blkdsp, int blkdsp_step, int blkdsp_offset,\n\
	int height, int width)\n\
{\n\
	int x = get_global_id(0);\n\
	int y = get_global_id(1);\n\
	if (x >= width || y >= height) {\n\
		return;\n\
	}\n\
	int imgwdtdsp = imgwdt - brkwdt - blkwdt;\n\
	int jpx = y;\n\
	int ipx = x;\n\
	int remwdt = imgwdt - ipx - blkwdt;\n\
	if (ipx > imgwdtdsp || remwdt <= 0 || jpx > (imghgt - blkhgt) || jpx % stphgt != 0 || ipx % stpwdt != 0) {\n\
		return;\n\
	}\n\
	float pxdthr = 0.0f;\n\
	float pxdmin = 6.0f;\n\
	unsigned int sumthr;\n\
	int extmtcwdt = 0;\n\
	if (remwdt < depth) {\n\
		if (extcnf > 0) {\n\
			extmtcwdt = remwdt;\n\
			pxdthr = pxdmin + (float)extcnf * extmtcwdt / depth;\n\
		}\n\
		depth = remwdt;\n\
	}\n\
	int jblk = jpx / stphgt;\n\
	int iblk = ipx / stpwdt;\n\
	unsigned int ssd[512];\n\
	int blkcnt = blkhgt * blkwdt;\n\
	unsigned int maxsum = 4095 * 4095 * blkcnt;\n\
	unsigned int misum = maxsum;\n\
	int disp = 0;\n\
	int idx = jpx * imgwdt + ipx;\n\
	int bidx = jblk * imgwdtblk + iblk;\n\
	int jpxe = jpx + blkhgt;\n\
	int ipxe = ipx + blkwdt;\n\
	unsigned int sumr = imgrefbrt[idx];\n\
	unsigned int sumrr = 0;\n\
	int crst = blkrefcrst[idx];;\n\
	if (crst < crstthr) {\n\
		blkdsp[bidx] = 0.0f;\n\
		return;\n\
	}\n\
	pxdthr = pxdthr * sumr / blkcnt / 4095;\n\
	sumthr = (unsigned int)(pxdthr * pxdthr * blkcnt);\n\
	for (int k = 0; k < depth; k++) {\n\
		int crstc = blkcmpcrst[idx + k];\n\
		if (crstc < crstthr) {\n\
			ssd[k] = maxsum;\n\
			continue;\n\
		}\n\
		unsigned int sumc = (unsigned int)imgcmpbrt[idx + k];\n\
		unsigned int minbrt;\n\
		unsigned int lowbrt;\n\
		if (sumc > sumr) {\n\
			minbrt = (sumc * minbrtrt) / 100;\n\
			lowbrt = sumr;\n\
		}\n\
		else {\n\
			minbrt = (sumr * minbrtrt) / 100;\n\
			lowbrt = sumc;\n\
		}\n\
		if (lowbrt < minbrt) {\n\
			ssd[k] = maxsum;\n\
			continue;\n\
		}\n\
		sumrr = 0;\n\
		unsigned int sumcc = 0;\n\
		unsigned int sumrc = 0;\n\
		for (int j = jpx; j < jpxe; j++) {\n\
			int idxj = j * imgwdt;\n\
			for (int i = ipx; i < ipxe; i++) {\n\
				int idxi = idxj + i;\n\
				unsigned int rfx = imgref[idxi];\n\
				unsigned int cpx = imgcmp[idxi + k];\n\
				sumrr += rfx * rfx;\n\
				sumcc += cpx * cpx;\n\
				sumrc += rfx * cpx;\n\
			}\n\
		}\n\
		unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
		ssd[k] = sumsq;\n\
		if (sumsq < misum) {\n\
			misum = sumsq;\n\
			disp = k;\n\
		}\n\
	}\n\
	if (disp < 1 || disp >= (depth - 1)	||\n\
		(extmtcwdt > 0 && misum > sumthr)) {\n\
		blkdsp[bidx] = 0.0f;\n\
	}\n\
	else {\n\
		if (ssd[disp - 1] == maxsum) {\n\
			unsigned int sumc = imgcmpbrt[idx + disp - 1];\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj + disp - 1;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgref[idxj + i];\n\
					unsigned int cpx = imgcmp[idxjdsp + i];\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
			ssd[disp - 1] = sumsq;\n\
		}\n\
		if (ssd[disp + 1] == maxsum) {\n\
			unsigned int sumc = imgcmpbrt[idx + disp + 1];\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj + disp + 1;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgref[idxj + i];\n\
					unsigned int cpx = imgcmp[idxjdsp + i];\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
			ssd[disp + 1] = sumsq;\n\
		}\n\
		int ssdprv = ssd[disp - 1];\n\
		int ssdcnt = ssd[disp];\n\
		int ssdnxt = ssd[disp + 1];\n\
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {\n\
			float sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);\n\
			blkdsp[bidx] = disp + sub;\n\
		}\n\
		else {\n\
			blkdsp[bidx] = 0.0f;\n\
		}\n\
	}\n\
}";

/// <summary>
/// OpenCLコンテキストの初期化フラグ
/// </summary>
static bool openCLMatchingContextInit16U = false;

/// <summary>
/// OpenCLコンテキスト
/// </summary>
static cv::ocl::Context contextMatching16U;

/// <summary>
/// カーネルプログラム
/// </summary>
static cv::ocl::Program kernelProgramMatching16U;

/// <summary>
/// カーネルオブジェクト
/// </summary>
static cv::ocl::Kernel kernelObjectMatching16U;

/// <summary>
/// glaobalWorkSize
/// </summary>
static size_t globalSizeMatching16U[2];

/// <summary>
/// SSDにより視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="imgref">入力基準画像データUMat(IN)</param>
/// <param name="imgcmp">入力比較画像データUMat(IN)</param>
/// <param name="imgrefbrt">入力基準画像のブロック輝度データUMat(IN)</param>
/// <param name="imgcmpbrt">入力比較画像のブロック輝度データUMat(IN)</param>
/// <param name="blkrefcrst">基準ブロックコントラストUMat(IN)</param>
/// <param name="blkcmpcrst">比較ブロックコントラストUMat(IN)</param>
/// <param name="blkdsp">マッチングの視差値UMat(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getDisparityBySSDOpenCL16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst,
	cv::UMat blkdsp)
{

	bool success;

	// OpenCLのコンテキストを初期化する
	if (openCLMatchingContextInit16U == false) {

		// コンテキストを生成する
		// * コンテキストContextを保持する？ <<<<<<　Context　スタティック
		// clCreateContext
		// context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
		success = contextMatching16U.create(cv::ocl::Device::TYPE_GPU);

		if (success == false) {
			OutputDebugString(_T("FALSE : context.create()\n"));
		}

		// デバイスを選択する
		// clGetDeviceIDs
		// clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
		cv::ocl::Device(contextMatching16U.device(0));

		// カーネルソースを生成する
		// clCreateProgramWithSource
		// command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
		// program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
		cv::ocl::ProgramSource programSource(kernelGetDisparityBySSD16U);

		cv::String errMsg;

		cv::String bldOpt = "";

		// カーネルプログラムをビルドする
		// * Programプログラムを保持する <<<<<<
		// clBuildProgram
		// ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
		kernelProgramMatching16U = contextMatching16U.getProg(programSource, bldOpt, errMsg);

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
		kernelObjectMatching16U.create("kernelGetDisparityBySSD16U", kernelProgramMatching16U);

		// OpenCL初期化フラグをセットする
		openCLMatchingContextInit16U = true;

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
	kernelObjectMatching16U.args(
		imghgt, // 1:入力補正画像の高さ
		imgwdt, // 2:入力補正画像の幅
		depth, // 3:マッチング探索幅
		brkwdt, // 4:マッチング探索打ち切り幅
		extcnf, // 5:拡張マッチング信頼限界
		crstthr, // 6:コントラスト閾値
		minbrtrt, // 7:マッチングブロック最低輝度比率(%)
		stphgt, // 8:マッチングステップの高さ
		stpwdt, // 9:マッチングステップの幅
		blkhgt, // 10:マッチングブロックの高さ
		blkwdt, // 11:マッチングブロックの幅
		imghgtblk, // 12:視差ブロック画像の高さ
		imgwdtblk, // 13:視差ブロック画像の幅
		cv::ocl::KernelArg::ReadOnlyNoSize(imgref),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmp),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgrefbrt),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmpbrt),
		cv::ocl::KernelArg::ReadOnlyNoSize(blkrefcrst),
		cv::ocl::KernelArg::ReadOnlyNoSize(blkcmpcrst),
		cv::ocl::KernelArg::ReadWrite(blkdsp)
	);

	// globalWorkSize : 行いたい処理の総数
	// localWorkSize : 並列に行いたい処理の数
	globalSizeMatching16U[0] = (size_t)imgref.cols;
	globalSizeMatching16U[1] = (size_t)imgcmp.rows;

	// カーネルを実行する
	// bool cv::ocl::Kernel::run(int dims, size_t globalsize[], size_t localsize[], bool sync, const Queue & q = Queue())
	//
	// clEnqueueTask
	// ret = clEnqueueNDRangeKernel(command_queue, kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	// ret = clEnqueueReadBuffer(command_queue, matrixRMemObj, CL_TRUE, 0, matrixRMemSize, matrixR, 0, NULL, NULL)
	// 
	success = kernelObjectMatching16U.run(2, globalSizeMatching16U, NULL, true);

	if (success == false) {
		OutputDebugString(_T("FALSE : kernel.run()\n"));
	}

}


/// <summary>
/// 両方向マッチングで視差値を算出するカーネルプログラム
/// </summary>
/// <remarks>最大探索幅 ISC_IMG_DEPTH_MAX 512</remarks>
/// <remarks>SSD最大値 : 16581375 8ビット階調(255) 16x16画素ブロック</remarks>
/// <remarks>ブロック内の輝度差の最小値 : BLOCK_MIN_DELTA_BRIGHTNESS 3</remarks>
static char* kernelGetBothDisparityBySSD = (char*)"__kernel void kernelGetBothDisparityBySSD(\n\
	int imghgt, int imgwdt, int depth, int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt,\n\
	int imghgtblk, int imgwdtblk,\n\
	__global uchar* imgref, int imgref_step, int imgref_offset,\n\
	__global uchar* imgcmp, int imgcmp_step, int imgcmp_offset,\n\
	__global int* imgrefbrt, int imgrefbrt_step, int imgrefbrt_offset,\n\
	__global int* imgcmpbrt, int imgcmpbrt_step, int imgcmpbrt_offset,\n\
	__global int* blkrefcrst, int blkrefcrst_step, int blkrefcrst_offset,\n\
	__global int* blkcmpcrst, int blkcmpcrst_step, int blkcmpcrst_offset,\n\
	__global float* blkdsp, int blkdsp_step, int blkdsp_offset,\n\
	int height, int width,\n\
	__global float* blkbkdsp, int blkbkdsp_step, int blkbkdsp_offset,\n\
	int bkheight, int bkwidth)\n\
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
	unsigned int ssd[512];\n\
	unsigned int bk_ssd[512];\n\
	int blkcnt = blkhgt * blkwdt;\n\
	unsigned int maxsum = 255 * 255 * blkcnt;\n\
	unsigned int misum = maxsum;\n\
	unsigned int bk_misum = maxsum;\n\
	int disp = 0;\n\
	int bk_disp = 0;\n\
	int idx = jpx * imgwdt + ipx;\n\
	int bidx = jblk * imgwdtblk + iblk;\n\
	int jpxe = jpx + blkhgt;\n\
	int ipxe = ipx + blkwdt;\n\
	unsigned int sumr = imgrefbrt[idx];\n\
	unsigned int sumrr = 0;\n\
	unsigned int bk_sumr = imgcmpbrt[idx];\n\
	unsigned int bk_sumrr = 0;\n\
	int crst = blkrefcrst[idx];\n\
	int bk_crst = blkcmpcrst[idx];\n\
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
	if (crst >= crstthr) {\n\
		for (int k = 0; k < fr_depth; k++) {\n\
			int crstc = blkcmpcrst[idx + k];\n\
			if (crstc < crstthr) {\n\
				ssd[k] = maxsum;\n\
				continue;\n\
			}\n\
			unsigned int sumc = imgcmpbrt[idx + k];\n\
			unsigned int highbrt;\n\
			unsigned int minbrt;\n\
			unsigned int lowbrt;\n\
			if (sumc > sumr) {\n\
				highbrt = sumc;\n\
				lowbrt = sumr;\n\
			}\n\
			else {\n\
				highbrt = sumr;\n\
				lowbrt = sumc;\n\
			}\n\
			minbrt = (highbrt * minbrtrt) / 100;\n\
			if (lowbrt < minbrt) {\n\
				ssd[k] = maxsum;\n\
				continue;\n\
			}\n\
			sumrr = 0;\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
		for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
			for (int i = ipx; i < ipxe; i++) {\n\
					int idxi = idxj + i;\n\
					unsigned int rfx = imgref[idxi];\n\
					unsigned int cpx = imgcmp[idxi + k];\n\
					sumrr += rfx * rfx;\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc)\n\
				- (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
		ssd[k] = sumsq;\n\
		if (sumsq < misum) {\n\
			misum = sumsq;\n\
			disp = k;\n\
		}\n\
		}\n\
	}\n\
	if (bk_crst >= crstthr) {\n\
		for (int k = 0; k < bk_depth; k++) {\n\
			int bk_crstc = blkrefcrst[idx - k];\n\
			if (bk_crstc < crstthr) {\n\
				bk_ssd[k] = maxsum;\n\
				continue;\n\
			}\n\
			unsigned int bk_sumc = imgrefbrt[idx - k];\n\
			unsigned int highbrt;\n\
			unsigned int minbrt;\n\
			unsigned int lowbrt;\n\
			if (bk_sumc > bk_sumr) {\n\
				highbrt = bk_sumc;\n\
				lowbrt = bk_sumr;\n\
			}\n\
			else {\n\
				highbrt = bk_sumr;\n\
				lowbrt = bk_sumc;\n\
			}\n\
			minbrt = (highbrt * minbrtrt) / 100;\n\
			if (lowbrt < minbrt) {\n\
				bk_ssd[k] = maxsum;\n\
				continue;\n\
			}\n\
			bk_sumrr = 0;\n\
			unsigned int bk_sumcc = 0;\n\
			unsigned int bk_sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					int idxi = idxj + i;\n\
					unsigned int rfx = imgcmp[idxi];\n\
					unsigned int cpx = imgref[idxi - k];\n\
					bk_sumrr += rfx * rfx;\n\
					bk_sumcc += cpx * cpx;\n\
					bk_sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc)\n\
				- (bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;\n\
		bk_ssd[k] = bk_sumsq;\n\
		if (bk_sumsq < bk_misum) {\n\
			bk_misum = bk_sumsq;\n\
			bk_disp = k;\n\
		}\n\
	}\n\
	}\n\
	float sub;\n\
	int ssdprv;\n\
	int ssdcnt;\n\
	int ssdnxt;\n\
	if (fr_depth < 3 || disp < 1 || disp >= (depth - 1)) {\n\
		blkdsp[bidx] = 0.0f;\n\
	}\n\
	else {\n\
		if (ssd[disp - 1] == maxsum) {\n\
			unsigned int sumc = imgcmpbrt[idx + disp - 1];\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj + disp - 1;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgref[idxj + i];\n\
					unsigned int cpx = imgcmp[idxjdsp + i];\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
			ssd[disp - 1] = sumsq;\n\
		}\n\
		if (ssd[disp + 1] == maxsum) {\n\
			unsigned int sumc = imgcmpbrt[idx + disp + 1];\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj + disp + 1;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgref[idxj + i];\n\
					unsigned int cpx = imgcmp[idxjdsp + i];\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
			ssd[disp + 1] = sumsq;\n\
		}\n\
		ssdprv = ssd[disp - 1];\n\
		ssdcnt = ssd[disp];\n\
		ssdnxt = ssd[disp + 1];\n\
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {\n\
			sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt); \n\
			blkdsp[bidx] = disp + sub; \n\
		}\n\
		else {\n\
			blkdsp[bidx] = 0.0f;\n\
		}\n\
	}\n\
	if (bk_depth >= 3 && bk_disp >= 1 && bk_disp < (bk_depth - 1)) {\n\
		if (bk_ssd[bk_disp - 1] == maxsum) {\n\
			unsigned int bk_sumc = imgrefbrt[idx - (bk_disp - 1)];\n\
			unsigned int bk_sumcc = 0;\n\
			unsigned int bk_sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj - (bk_disp - 1);\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgcmp[idxj + i];\n\
					unsigned int cpx = imgref[idxjdsp + i];\n\
					bk_sumcc += cpx * cpx;\n\
					bk_sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) -\n\
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;\n\
			bk_ssd[bk_disp - 1] = bk_sumsq;\n\
		}\n\
		if (bk_ssd[bk_disp + 1] == maxsum) {\n\
			unsigned int bk_sumc = imgrefbrt[idx - (bk_disp + 1)];\n\
			unsigned int bk_sumcc = 0;\n\
			unsigned int bk_sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj - (bk_disp + 1);\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgcmp[idxj + i];\n\
					unsigned int cpx = imgref[idxjdsp + i];\n\
					bk_sumcc += cpx * cpx;\n\
					bk_sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) -\n\
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;\n\
			bk_ssd[bk_disp + 1] = bk_sumsq;\n\
	}\n\
		ssdprv = bk_ssd[bk_disp - 1];\n\
		ssdcnt = bk_ssd[bk_disp];\n\
		ssdnxt = bk_ssd[bk_disp + 1];\n\
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {\n\
			sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);\n\
		float bk_disp_sub = bk_disp + sub;\n\
		int bk_iblk = (int)((ipx - bk_disp_sub) / stpwdt);\n\
		blkbkdsp[jblk * imgwdtblk + bk_iblk] = bk_disp_sub;\n\
	}\n\
	}\n\
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
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="imgref">入力基準画像データUMat(IN)</param>
/// <param name="imgcmp">入力比較画像データUMat(IN)</param>
/// <param name="imgrefbrt">入力基準画像のブロック輝度データUMat(IN)</param>
/// <param name="imgcmpbrt">入力比較画像のブロック輝度データUMat(IN)</param>
/// <param name="blkrefcrst">基準ブロックコントラストUMat(IN)</param>
/// <param name="blkcmpcrst">比較ブロックコントラストUMat(IN)</param>
/// <param name="blkdsp">マッチングの視差値UMat(OUT)</param>
/// <param name="blkbkdsp">バックマッチングの視差値UMat(OUT)</param>
void StereoMatching::getBothDisparityBySSDOpenCL(int imghgt, int imgwdt, int depth,
	int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst,
	cv::UMat blkdsp, cv::UMat blkbkdsp)
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
		minbrtrt, // 5:マッチングブロック最低輝度比率(%)
		stphgt, // 6:マッチングステップの高さ
		stpwdt, // 7:マッチングステップの幅
		blkhgt, // 8:マッチングブロックの高さ
		blkwdt, // 9:マッチングブロックの幅
		imghgtblk, // 10:視差ブロック画像の高さ
		imgwdtblk, // 11:視差ブロック画像の幅
		cv::ocl::KernelArg::ReadOnlyNoSize(imgref),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmp),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgrefbrt),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmpbrt),
		cv::ocl::KernelArg::ReadOnlyNoSize(blkrefcrst),
		cv::ocl::KernelArg::ReadOnlyNoSize(blkcmpcrst),
		cv::ocl::KernelArg::ReadWrite(blkdsp),
		cv::ocl::KernelArg::ReadWrite(blkbkdsp)
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
/// 両方向マッチングで視差値を算出するカーネルプログラム
/// </summary>
/// <remarks>最大探索幅 ISC_IMG_DEPTH_MAX 512</remarks>
/// <remarks>SSD最大値 : 1676902500 12ビット階調(4095) 10x10画素ブロック</remarks>
/// <remarks>ブロック内の輝度差の最小値 : BLOCK_MIN_DELTA_BRIGHTNESS 3</remarks>
/// <remarks>12ビット階調対応</remarks>
static char *kernelGetBothDisparityBySSD16U = (char*)"__kernel void kernelGetBothDisparityBySSD16U(\n\
	int imghgt, int imgwdt, int depth, int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt,\n\
	int imghgtblk, int imgwdtblk,\n\
	__global short* imgref, int imgref_step, int imgref_offset,\n\
	__global short* imgcmp, int imgcmp_step, int imgcmp_offset,\n\
	__global int* imgrefbrt, int imgrefbrt_step, int imgrefbrt_offset,\n\
	__global int* imgcmpbrt, int imgcmpbrt_step, int imgcmpbrt_offset,\n\
	__global int* blkrefcrst, int blkrefcrst_step, int blkrefcrst_offset,\n\
	__global int* blkcmpcrst, int blkcmpcrst_step, int blkcmpcrst_offset,\n\
	__global float* blkdsp, int blkdsp_step, int blkdsp_offset,\n\
	int height, int width,\n\
	__global float* blkbkdsp, int blkbkdsp_step, int blkbkdsp_offset,\n\
	int bkheight, int bkwidth)\n\
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
	unsigned int ssd[512];\n\
	unsigned int bk_ssd[512];\n\
	int blkcnt = blkhgt * blkwdt;\n\
	unsigned int maxsum = 4095 * 4095 * blkcnt;\n\
	unsigned int misum = maxsum;\n\
	unsigned int bk_misum = maxsum;\n\
	int disp = 0;\n\
	int bk_disp = 0;\n\
	int idx = jpx * imgwdt + ipx;\n\
	int bidx = jblk * imgwdtblk + iblk;\n\
	int jpxe = jpx + blkhgt;\n\
	int ipxe = ipx + blkwdt;\n\
	unsigned int sumr = imgrefbrt[idx];\n\
	unsigned int sumrr = 0;\n\
	unsigned int bk_sumr = imgcmpbrt[idx];\n\
	unsigned int bk_sumrr = 0;\n\
	int crst = blkrefcrst[idx]; \n\
	int bk_crst = blkcmpcrst[idx]; \n\
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
	if (crst >= crstthr) {\n\
		for (int k = 0; k < fr_depth; k++) {\n\
			unsigned int crstc = blkcmpcrst[idx + k];\n\
			if (crstc < crstthr) {\n\
				ssd[k] = maxsum;\n\
				continue;\n\
			}\n\
			int sumc = imgcmpbrt[idx + k];\n\
			unsigned int highbrt;\n\
			unsigned int minbrt;\n\
			unsigned int lowbrt;\n\
			if (sumc > sumr) {\n\
				highbrt = sumc;\n\
				lowbrt = sumr;\n\
			}\n\
			else {\n\
				highbrt = sumr;\n\
				lowbrt = sumc;\n\
			}\n\
			minbrt = (highbrt * minbrtrt) / 100;\n\
			if (lowbrt < minbrt) {\n\
				ssd[k] = maxsum;\n\
				continue;\n\
			}\n\
			sumrr = 0;\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					int idxi = idxj + i;\n\
					unsigned int rfx = imgref[idxi];\n\
					unsigned int cpx = imgcmp[idxi + k];\n\
					sumrr += rfx * rfx;\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc)\n\
				- (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
			ssd[k] = sumsq;\n\
			if (sumsq < misum) {\n\
				misum = sumsq;\n\
				disp = k;\n\
			}\n\
		}\n\
	}\n\
	if (bk_crst >= crstthr) {\n\
		for (int k = 0; k < bk_depth; k++) {\n\
			int bk_crstc = blkrefcrst[idx - k];\n\
			if (bk_crstc < crstthr) {\n\
				bk_ssd[k] = maxsum;\n\
				continue;\n\
			}\n\
			unsigned int bk_sumc = imgrefbrt[idx - k];\n\
			unsigned int highbrt;\n\
			unsigned int minbrt;\n\
			unsigned int lowbrt;\n\
			if (bk_sumc > bk_sumr) {\n\
				highbrt = bk_sumc;\n\
				lowbrt = bk_sumr;\n\
			}\n\
			else {\n\
				highbrt = bk_sumr;\n\
				lowbrt = bk_sumc;\n\
			}\n\
			minbrt = (highbrt * minbrtrt) / 100;\n\
			if (lowbrt < minbrt) {\n\
				bk_ssd[k] = maxsum;\n\
				continue;\n\
			}\n\
			bk_sumrr = 0;\n\
			unsigned int bk_sumcc = 0;\n\
			unsigned int bk_sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					int idxi = idxj + i;\n\
					unsigned int rfx = imgcmp[idxi];\n\
					unsigned int cpx = imgref[idxi - k];\n\
					bk_sumrr += rfx * rfx;\n\
					bk_sumcc += cpx * cpx;\n\
					bk_sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc)\n\
				- (bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;\n\
			bk_ssd[k] = bk_sumsq;\n\
			if (bk_sumsq < bk_misum) {\n\
				bk_misum = bk_sumsq;\n\
				bk_disp = k;\n\
			}\n\
		}\n\
	}\n\
	float sub;\n\
	float ssdprv;\n\
	float ssdcnt;\n\
	float ssdnxt;\n\
	if (fr_depth < 3 || disp < 1 || disp >= (depth - 1)) {\n\
		blkdsp[bidx] = 0.0f;\n\
	}\n\
	else {\n\
		if (ssd[disp - 1] == maxsum) {\n\
			unsigned int sumc = imgcmpbrt[idx + disp - 1];\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj + disp - 1;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgref[idxj + i];\n\
					unsigned int cpx = imgcmp[idxjdsp + i];\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
				ssd[disp - 1] = sumsq;\n\
		}\n\
		if (ssd[disp + 1] == maxsum) {\n\
			unsigned int sumc = imgcmpbrt[idx + disp + 1];\n\
			unsigned int sumcc = 0;\n\
			unsigned int sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj + disp + 1;\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgref[idxj + i];\n\
					unsigned int cpx = imgcmp[idxjdsp + i];\n\
					sumcc += cpx * cpx;\n\
					sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int sumsq = (sumrr + sumcc - 2 * sumrc) - (sumr * sumr + sumc * sumc - 2 * sumr * sumc) / blkcnt;\n\
				ssd[disp + 1] = sumsq;\n\
		}\n\
		ssdprv = ssd[disp - 1];\n\
		ssdcnt = ssd[disp];\n\
		ssdnxt = ssd[disp + 1];\n\
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {\n\
			sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);\n\
			blkdsp[bidx] = disp + sub;\n\
		}\n\
		else {\n\
			blkdsp[bidx] = 0.0f;\n\
		}\n\
	}\n\
	if (bk_depth >= 3 && bk_disp >= 1 && bk_disp < (bk_depth - 1)) {\n\
		if (bk_ssd[bk_disp - 1] == maxsum) {\n\
			unsigned int bk_sumc = imgrefbrt[idx - (bk_disp - 1)];\n\
			unsigned int bk_sumcc = 0;\n\
			unsigned int bk_sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj - (bk_disp - 1);\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgcmp[idxj + i];\n\
					unsigned int cpx = imgref[idxjdsp + i];\n\
					bk_sumcc += cpx * cpx;\n\
					bk_sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) - \n\
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;\n\
			bk_ssd[bk_disp - 1] = bk_sumsq;\n\
		}\n\
		if (bk_ssd[bk_disp + 1] == maxsum) {\n\
			unsigned int bk_sumc = imgrefbrt[idx - (bk_disp + 1)];\n\
			unsigned int bk_sumcc = 0;\n\
			unsigned int bk_sumrc = 0;\n\
			for (int j = jpx; j < jpxe; j++) {\n\
				int idxj = j * imgwdt;\n\
				int idxjdsp = idxj - (bk_disp + 1);\n\
				for (int i = ipx; i < ipxe; i++) {\n\
					unsigned int rfx = imgcmp[idxj + i];\n\
					unsigned int cpx = imgref[idxjdsp + i];\n\
					bk_sumcc += cpx * cpx;\n\
					bk_sumrc += rfx * cpx;\n\
				}\n\
			}\n\
			unsigned int bk_sumsq = (bk_sumrr + bk_sumcc - 2 * bk_sumrc) - \n\
				(bk_sumr * bk_sumr + bk_sumc * bk_sumc - 2 * bk_sumr * bk_sumc) / blkcnt;\n\
			bk_ssd[bk_disp + 1] = bk_sumsq;\n\
		}\n\
		ssdprv = bk_ssd[bk_disp - 1];\n\
		ssdcnt = bk_ssd[bk_disp];\n\
		ssdnxt = bk_ssd[bk_disp + 1];\n\
		if (ssdprv >= ssdcnt && ssdnxt >= ssdcnt && (ssdprv + ssdnxt) > (2 * ssdcnt)) {\n\
			sub = (float)(ssdprv - ssdnxt) / (2 * ssdprv - 4 * ssdcnt + 2 * ssdnxt);\n\
			float bk_disp_sub = bk_disp + sub;\n\
			int bk_iblk = (int)((ipx - bk_disp_sub) / stpwdt);\n\
			blkbkdsp[jblk * imgwdtblk + bk_iblk] = bk_disp_sub;\n\
		}\n\
	}\n\
}";


/// <summary>
/// OpenCLコンテキストの初期化フラグ
/// </summary>
static bool openCLBothMatchingContextInit16U = false;

/// <summary>
/// OpenCLコンテキスト
/// </summary>
static cv::ocl::Context contextBothMatching16U;

/// <summary>
/// カーネルプログラム
/// </summary>
static cv::ocl::Program kernelProgramBothMatching16U;

/// <summary>
/// カーネルオブジェクト
/// </summary>
static cv::ocl::Kernel kernelObjectBothMatching16U;

/// <summary>
/// glaobalWorkSize
/// </summary>
static size_t globalSizeBothMatching16U[2];


/// <summary>
/// 両方向マッチングにより視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="imgref">入力基準画像データUMat(IN)</param>
/// <param name="imgcmp">入力比較画像データUMat(IN)</param>
/// <param name="imgrefbrt">入力基準画像のブロック輝度データUMat(IN)</param>
/// <param name="imgcmpbrt">入力比較画像のブロック輝度データUMat(IN)</param>
/// <param name="blkrefcrst">基準ブロックコントラストUMat(IN)</param>
/// <param name="blkcmpcrst">比較ブロックコントラストUMat(IN)</param>
/// <param name="blkdsp">マッチングの視差値UMat(OUT)</param>
/// <param name="blkbkdsp">バックマッチングの視差値UMat(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getBothDisparityBySSDOpenCL16U(int imghgt, int imgwdt, int depth,
	int crstthr, int minbrtrt, int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	cv::UMat imgref, cv::UMat imgcmp, cv::UMat imgrefbrt, cv::UMat imgcmpbrt, cv::UMat blkrefcrst, cv::UMat blkcmpcrst,
	cv::UMat blkdsp, cv::UMat blkbkdsp)
{

	bool success;

	// OpenCLのコンテキストを初期化する
	if (openCLBothMatchingContextInit16U == false) {

		// コンテキストを生成する
		// * コンテキストContextを保持する？ <<<<<<　Context　スタティック
		// clCreateContext
		// context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
		success = contextBothMatching16U.create(cv::ocl::Device::TYPE_GPU);

		if (success == false) {
			OutputDebugString(_T("FALSE : context.create()\n"));
		}

		// デバイスを選択する
		// clGetDeviceIDs
		// clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
		cv::ocl::Device(contextBothMatching16U.device(0));

		// カーネルソースを生成する
		// clCreateProgramWithSource
		// command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
		// program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
		cv::ocl::ProgramSource programSource(kernelGetBothDisparityBySSD16U);

		cv::String errMsg;

		cv::String bldOpt = "";

		// カーネルプログラムをビルドする
		// * Programプログラムを保持する <<<<<<
		// clBuildProgram
		// ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
		kernelProgramBothMatching16U = contextBothMatching16U.getProg(programSource, bldOpt, errMsg);

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
		kernelObjectBothMatching16U.create("kernelGetBothDisparityBySSD16U", kernelProgramBothMatching16U);

		// OpenCL初期化フラグをセットする
		openCLBothMatchingContextInit16U = true;

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
	kernelObjectBothMatching16U.args(
		imghgt, // 1:入力補正画像の高さ
		imgwdt, // 2:入力補正画像の幅
		depth, // 3:マッチング探索幅
		crstthr, // 4:コントラスト閾値
		minbrtrt, // 5:マッチングブロック最低輝度比率(%)
		stphgt, // 6:マッチングステップの高さ
		stpwdt, // 7:マッチングステップの幅
		blkhgt, // 8:マッチングブロックの高さ
		blkwdt, // 9:マッチングブロックの幅
		imghgtblk, // 10:視差ブロック画像の高さ
		imgwdtblk, // 11:視差ブロック画像の幅
		cv::ocl::KernelArg::ReadOnlyNoSize(imgref),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmp),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgrefbrt),
		cv::ocl::KernelArg::ReadOnlyNoSize(imgcmpbrt),
		cv::ocl::KernelArg::ReadOnlyNoSize(blkrefcrst),
		cv::ocl::KernelArg::ReadOnlyNoSize(blkcmpcrst),
		cv::ocl::KernelArg::ReadWrite(blkdsp),
		cv::ocl::KernelArg::ReadWrite(blkbkdsp)
	);

	// globalWorkSize : 行いたい処理の総数
	// localWorkSize : 並列に行いたい処理の数
	globalSizeBothMatching16U[0] = (size_t)imgref.cols;
	globalSizeBothMatching16U[1] = (size_t)imgcmp.rows;

	// カーネルを実行する
	// bool cv::ocl::Kernel::run(int dims, size_t globalsize[], size_t localsize[], bool sync, const Queue & q = Queue())
	//
	// clEnqueueTask
	// ret = clEnqueueNDRangeKernel(command_queue, kernel, workDim, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	// ret = clEnqueueReadBuffer(command_queue, matrixRMemObj, CL_TRUE, 0, matrixRMemSize, matrixR, 0, NULL, NULL)
	// 
	success = kernelObjectBothMatching16U.run(2, globalSizeBothMatching16U, NULL, true);

	if (success == false) {
		OutputDebugString(_T("FALSE : kernel.run()\n"));
	}

}


/// <summary>
/// ブロック輝度算出スレッド
/// </summary>
/// <param name="parg">バンド情報(IN)</param>
/// <returns>処理結果を返す</returns>
UINT  StereoMatching::blockBandThread(LPVOID parg)
{
	BNAD_THREAD_BLOCK_INFO* pBand = (BNAD_THREAD_BLOCK_INFO*)parg;

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

		// バンド内の比較画像のブロック輝度を取得する
		if (pBand->pimgref_16U == NULL) {
			getBlockBrightnessContrastInBand(pBand->imghgt, pBand->imgwdt,
				pBand->stphgt, pBand->stpwdt, pBand->blkhgt, pBand->blkwdt,
				pBand->imghgtblk, pBand->imgwdtblk,
				pBand->crstthr, pBand->crstofs, pBand->grdcrct,
				pBand->pimgref, pBand->pimgcmp, pBand->pimgrefbrt, pBand->pimgcmpbrt,
				pBand->pblkrefcrst, pBand->pblkcmpcrst,
				pBand->bandStart, pBand->bandEnd);
		}
		else {
			getBlockBrightnessContrastInBand16U(pBand->imghgt, pBand->imgwdt,
				pBand->stphgt, pBand->stpwdt, pBand->blkhgt, pBand->blkwdt,
				pBand->imghgtblk, pBand->imgwdtblk,
				pBand->crstthr, pBand->crstofs, pBand->grdcrct,
				pBand->pimgref_16U, pBand->pimgcmp_16U, pBand->pimgrefbrt, pBand->pimgcmpbrt,
				pBand->pblkrefcrst, pBand->pblkcmpcrst,
				pBand->bandStart, pBand->bandEnd);
		}

		// 視差平均化完了イベントを通知する
		SetEvent(pBand->doneEvent);
	}
	return 0;
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
			if (pBand->pimgref_16U == NULL) {
				getDisparityInBand(pBand->imghgt, pBand->imgwdt, pBand->depth, pBand->brkwdt, pBand->extcnf,
					pBand->crstthr, pBand->crstofs, pBand->grdcrct, pBand->minbrtrt,
					pBand->stphgt, pBand->stpwdt, pBand->blkhgt, pBand->blkwdt, pBand->imghgtblk, pBand->imgwdtblk,
					pBand->pimgref, pBand->pimgcmp, pBand->pimgrefbrt, pBand->pimgcmpbrt,
					pBand->pblkrefcrst, pBand->pblkcmpcrst, pBand->pblkdsp,
					pBand->bandStart, pBand->bandEnd);
			}
			else {
				getDisparityInBand16U(pBand->imghgt, pBand->imgwdt, pBand->depth, pBand->brkwdt, pBand->extcnf,
					pBand->crstthr, pBand->crstofs, pBand->grdcrct, pBand->minbrtrt,
				pBand->stphgt, pBand->stpwdt, pBand->blkhgt, pBand->blkwdt, pBand->imghgtblk, pBand->imgwdtblk,
					pBand->pimgref_16U, pBand->pimgcmp_16U, pBand->pimgrefbrt, pBand->pimgcmpbrt,
					pBand->pblkrefcrst, pBand->pblkcmpcrst, pBand->pblkdsp,
					pBand->bandStart, pBand->bandEnd);
			}
		}
		else {
			if (pBand->pimgref_16U == NULL) {
				getBothDisparityInBand(pBand->imghgt, pBand->imgwdt, pBand->depth,
					pBand->crstthr, pBand->crstofs, pBand->grdcrct, pBand->minbrtrt,
					pBand->stphgt, pBand->stpwdt, pBand->blkhgt, pBand->blkwdt, pBand->imghgtblk, pBand->imgwdtblk,
					pBand->pimgref, pBand->pimgcmp, pBand->pimgrefbrt, pBand->pimgcmpbrt,
					pBand->pblkrefcrst, pBand->pblkcmpcrst, pBand->pblkdsp, pBand->pblkbkdsp,
					pBand->bandStart, pBand->bandEnd);
		}
		else {
				getBothDisparityInBand16U(pBand->imghgt, pBand->imgwdt, pBand->depth,
					pBand->crstthr, pBand->crstofs, pBand->grdcrct, pBand->minbrtrt,
				pBand->stphgt, pBand->stpwdt, pBand->blkhgt, pBand->blkwdt, pBand->imghgtblk, pBand->imgwdtblk,
					pBand->pimgref_16U, pBand->pimgcmp_16U, pBand->pimgrefbrt, pBand->pimgcmpbrt,
					pBand->pblkrefcrst, pBand->pblkcmpcrst, pBand->pblkdsp, pBand->pblkbkdsp,
				pBand->bandStart, pBand->bandEnd);
		}
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
		// マッチングスレッド
		// バンド数分のスレッドを生成する
		for (int i = 0; i < numOfBands; i++) {
			// イベントを生成する
			// 自動リセット非シグナル状態
			bandInfo[i].startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			bandInfo[i].stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			bandInfo[i].doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			// スレッドを生成する
			bandInfo[i].bandThread = (HANDLE)_beginthreadex(0, 0, matchingBandThread, (LPVOID)&bandInfo[i], 0, 0);
			if (bandInfo[i].bandThread == 0) {
				// failed
			}
			else {
				SetThreadPriority(bandInfo[i].bandThread, THREAD_PRIORITY_NORMAL);
			}
		}

		// ブロックスレッド
		// バンド数分のスレッドを生成する
		for (int i = 0; i < numOfBands; i++) {
			// イベントを生成する
			// 自動リセット非シグナル状態
			bandBlockInfo[i].startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			bandBlockInfo[i].stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			bandBlockInfo[i].doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			// スレッドを生成する
			bandBlockInfo[i].bandThread = (HANDLE)_beginthreadex(0, 0, blockBandThread, (LPVOID)&bandBlockInfo[i], 0, 0);
			if (bandBlockInfo[i].bandThread == 0) {
				// failed
			}
			else {
				SetThreadPriority(bandBlockInfo[i].bandThread, THREAD_PRIORITY_NORMAL);
			}
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
		// マッチングスレッド
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

		// ブロックスレッド
		for (int i = 0; i < numOfBands; i++) {
			// 停止イベントを送信する
			SetEvent(bandBlockInfo[i].stopEvent);
			// 開始イベントを送信する
			SetEvent(bandBlockInfo[i].startEvent);
			// 受信スレッドの終了を待つ
			WaitForSingleObject(bandBlockInfo[i].bandThread, INFINITE);

			// スレッドオブジェクトを破棄する
			CloseHandle(bandBlockInfo[i].bandThread);

			// イベントオブジェクトを破棄する
			CloseHandle(bandBlockInfo[i].startEvent);
			CloseHandle(bandBlockInfo[i].stopEvent);
			CloseHandle(bandBlockInfo[i].doneEvent);
		}

	}
}


/// <summary>
/// バンド分割してブロック輝度とコントラストを取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
void StereoMatching::getBandBlockBrightnessContrast(int imghgt, int imgwdt, int stphgt, int stpwdt,
	int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct,
	unsigned char* pimgref, unsigned char* pimgcmp,	int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst)
{
	// 完了イベントの配列
	HANDLE doneevt[MAX_NUM_OF_BANDS];

	// バンドの高さライン数
	int bndhgt = imghgt / numOfBands;

	int i, n;

	for (i = 0, n = 0; i < numOfBands; i++, n += bndhgt) {
		// 入力補正画像の高さ
		bandBlockInfo[i].imghgt = imghgt;
		// 入力補正画像の幅
		bandBlockInfo[i].imgwdt = imgwdt;

		// コントラスト閾値
		bandBlockInfo[i].crstthr = crstthr;
		// コントラストオフセット
		bandBlockInfo[i].crstofs = crstofs;
		// 階調補正モードステータス
		bandBlockInfo[i].grdcrct = grdcrct;

		// マッチングステップの高さ
		bandBlockInfo[i].stphgt = stphgt;
		// マッチングステップの幅
		bandBlockInfo[i].stpwdt = stpwdt;
		// マッチングブロックの高さ
		bandBlockInfo[i].blkhgt = blkhgt;
		// マッチングブロックの幅
		bandBlockInfo[i].blkwdt = blkwdt;
		// 視差ブロック画像の高さ
		bandBlockInfo[i].imghgtblk = imghgtblk;
		// 視差ブロック画像の幅
		bandBlockInfo[i].imgwdtblk = imgwdtblk;

		// 入力基準画像データ
		bandBlockInfo[i].pimgref = pimgref;
		// 入力比較画像データ
		bandBlockInfo[i].pimgcmp = pimgcmp;
		// 入力基準画像データ（12ビット階調対応）
		bandBlockInfo[i].pimgref_16U = NULL;
		// 入力比較画像データ（12ビット階調対応）
		bandBlockInfo[i].pimgcmp_16U = NULL;

		// 基準画像のブロック輝度
		bandBlockInfo[i].pimgrefbrt = pimgrefbrt;
		// 比較画像のブロック輝度値総和
		bandBlockInfo[i].pimgcmpbrt = pimgcmpbrt;

		// 基準ブロックコントラスト
		bandBlockInfo[i].pblkrefcrst = pblkrefcrst;
		// 比較ブロックコントラスト
		bandBlockInfo[i].pblkcmpcrst = pblkcmpcrst;

		// バンド開始ライン位置（y座標）
		bandBlockInfo[i].bandStart = n;
		// バンド終了ライン位置（y座標）
		bandBlockInfo[i].bandEnd = n + bndhgt;

	}
	bandBlockInfo[i - 1].bandEnd = imghgt;

	DWORD st;

	for (i = 0; i < numOfBands; i++) {
		// 開始イベントを送信する
		SetEvent(bandBlockInfo[i].startEvent);
		// 完了イベントのハンドルを配列に格納する
		doneevt[i] = bandBlockInfo[i].doneEvent;
	}

	// 全ての完了イベントを待つ
	st = WaitForMultipleObjects(numOfBands, doneevt, TRUE, INFINITE);

	if (st == WAIT_OBJECT_0) {

	}

}


/// <summary>
/// バンド分割してブロック輝度とコントラストを取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)</param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(OUT)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(OUT)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(OUT)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getBandBlockBrightnessContrast16U(int imghgt, int imgwdt, int stphgt, int stpwdt,
	int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk, int crstthr, int crstofs, int grdcrct, 
	unsigned short* pimgref, unsigned short* pimgcmp, int *pimgrefbrt, int *pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst)
{
	// 完了イベントの配列
	HANDLE doneevt[MAX_NUM_OF_BANDS];

	// バンドの高さライン数
	int bndhgt = imghgt / numOfBands;

	int i, n;

	for (i = 0, n = 0; i < numOfBands; i++, n += bndhgt) {
		// 入力補正画像の高さ
		bandBlockInfo[i].imghgt = imghgt;
		// 入力補正画像の幅
		bandBlockInfo[i].imgwdt = imgwdt;

		// コントラスト閾値
		bandBlockInfo[i].crstthr = crstthr;
		// コントラストオフセット
		bandBlockInfo[i].crstofs = crstofs;
		// 階調補正モードステータス
		bandBlockInfo[i].grdcrct = grdcrct;

		// マッチングステップの高さ
		bandBlockInfo[i].stphgt = stphgt;
		// マッチングステップの幅
		bandBlockInfo[i].stpwdt = stpwdt;
		// マッチングブロックの高さ
		bandBlockInfo[i].blkhgt = blkhgt;
		// マッチングブロックの幅
		bandBlockInfo[i].blkwdt = blkwdt;
		// 視差ブロック画像の高さ
		bandBlockInfo[i].imghgtblk = imghgtblk;
		// 視差ブロック画像の幅
		bandBlockInfo[i].imgwdtblk = imgwdtblk;

		// 入力基準画像データ
		bandBlockInfo[i].pimgref = NULL;
		// 入力比較画像データ
		bandBlockInfo[i].pimgcmp = NULL;
		// 入力基準画像データ（12ビット階調対応）
		bandBlockInfo[i].pimgref_16U = pimgref;
		// 入力比較画像データ（12ビット階調対応）
		bandBlockInfo[i].pimgcmp_16U = pimgcmp;

		// 基準画像のブロック輝度
		bandBlockInfo[i].pimgrefbrt = pimgrefbrt;
		// 比較画像のブロック輝度値総和
		bandBlockInfo[i].pimgcmpbrt = pimgcmpbrt;

		// 基準ブロックコントラスト
		bandBlockInfo[i].pblkrefcrst = pblkrefcrst;
		// 比較ブロックコントラスト
		bandBlockInfo[i].pblkcmpcrst = pblkcmpcrst;

		// バンド開始ライン位置（y座標）
		bandBlockInfo[i].bandStart = n;
		// バンド終了ライン位置（y座標）
		bandBlockInfo[i].bandEnd = n + bndhgt;

	}
	bandBlockInfo[i - 1].bandEnd = imghgt;

	DWORD st;

	for (i = 0; i < numOfBands; i++) {
		// 開始イベントを送信する
		SetEvent(bandBlockInfo[i].startEvent);
		// 完了イベントのハンドルを配列に格納する
		doneevt[i] = bandBlockInfo[i].doneEvent;
	}

	// 全ての完了イベントを待つ
	st = WaitForMultipleObjects(numOfBands, doneevt, TRUE, INFINITE);

	if (st == WAIT_OBJECT_0) {

	}

}


/// <summary>
/// バンド分割して視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
void StereoMatching::getBandDisparity(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned char *pimgref, unsigned char *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float *pblkdsp, float *pblkbkdsp)
{
	// 完了イベントの配列
	HANDLE doneevt[MAX_NUM_OF_BANDS];

	// バンドの高さライン数
	int bndhgt = imghgt / numOfBands;

	int i, n;

	for (i = 0, n = 0; i < numOfBands; i++, n += bndhgt) {
		// 入力補正画像の高さ
		bandInfo[i].imghgt = imghgt;
		// 入力補正画像の幅
		bandInfo[i].imgwdt = imgwdt;
		// マッチング探索幅
		bandInfo[i].depth = depth;
		// マッチング探索打ち切り幅
		bandInfo[i].brkwdt = brkwdt;
		// 拡張マッチング信頼限界
		bandInfo[i].extcnf = extcnf;

		// コントラスト閾値
		bandInfo[i].crstthr = crstthr;
		// コントラストオフセット
		bandInfo[i].crstofs = crstofs;
		// 階調補正モードステータス
		bandInfo[i].grdcrct = grdcrct;

		// マッチングブロック最低輝度比率(%)
		bandInfo[i].minbrtrt = minbrtrt;

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
		// 入力基準画像データ（12ビット階調対応）
		bandInfo[i].pimgref_16U = NULL;
		// 入力比較画像データ（12ビット階調対応）
		bandInfo[i].pimgcmp_16U = NULL;

		// マッチングの視差値
		bandInfo[i].pblkdsp = pblkdsp;
		// バックマッチングの視差値
		bandInfo[i].pblkbkdsp = pblkbkdsp;

		// 基準ブロックコントラスト
		bandInfo[i].pblkrefcrst = pblkrefcrst;
		// 比較ブロックコントラスト
		bandInfo[i].pblkcmpcrst = pblkcmpcrst;

		// 基準画像のブロック輝度
		bandInfo[i].pimgrefbrt = pimgrefbrt;
		// 比較画像のブロック輝度
		bandInfo[i].pimgcmpbrt = pimgcmpbrt;

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


/// <summary>
/// バンド分割して視差を取得する
/// </summary>
/// <param name="imghgt">入力補正画像の高さ(IN)</param>
/// <param name="imgwdt">入力補正画像の幅(IN)</param>
/// <param name="depth">マッチング探索幅(IN)</param>
/// <param name="brkwdt">マッチング探索打ち切り幅(IN)</param>
/// <param name="extcnf">拡張マッチング信頼限界(IN)</param>
/// <param name="crstthr">コントラスト閾値(IN)</param>
/// <param name="crstofs">コントラストオフセット(IN)</param>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
/// <param name="minbrtrt">マッチングブロック最低輝度比率(%)(IN)</param>
/// <param name="stphgt">マッチングステップの高さ(IN)</param>
/// <param name="stpwdt">マッチングステップの幅(IN)</param>
/// <param name="blkhgt">マッチングブロックの高さ(IN)/param>
/// <param name="blkwdt">マッチングブロックの幅(IN)</param>
/// <param name="imghgtblk">視差ブロック画像の高さ(IN)</param>
/// <param name="imgwdtblk">視差ブロック画像の幅(IN)</param>
/// <param name="pimgref">入力基準画像データ(IN)</param>
/// <param name="pimgcmp">入力比較画像データ(IN)</param>
/// <param name="pimgrefbrt">基準画像のブロック輝度(IN)</param>
/// <param name="pimgcmpbrt">比較画像のブロック輝度(IN)</param>
/// <param name="pblkrefcrst">基準ブロックコントラスト(IN)</param>
/// <param name="pblkcmpcrst">比較ブロックコントラスト(IN)</param>
/// <param name="pblkdsp">マッチングの視差値(OUT)</param>
/// <param name="pblkbkdsp">バックマッチングの視差値(OUT)</param>
/// <remarks>12ビット階調対応</remarks>
void StereoMatching::getBandDisparity16U(int imghgt, int imgwdt, int depth, int brkwdt, int extcnf,
	int crstthr, int crstofs, int grdcrct, int minbrtrt,
	int stphgt, int stpwdt, int blkhgt, int blkwdt, int imghgtblk, int imgwdtblk,
	unsigned short *pimgref, unsigned short *pimgcmp, int* pimgrefbrt, int* pimgcmpbrt,
	int* pblkrefcrst, int* pblkcmpcrst, float *pblkdsp, float *pblkbkdsp)
{
	// 完了イベントの配列
	HANDLE doneevt[MAX_NUM_OF_BANDS];

	// バンドの高さライン数
	int bndhgt = imghgt / numOfBands;

	int i, n;

	for (i = 0, n = 0; i < numOfBands; i++, n += bndhgt) {
		// 入力補正画像の高さ
		bandInfo[i].imghgt = imghgt;
		// 入力補正画像の幅
		bandInfo[i].imgwdt = imgwdt;
		// マッチング探索幅
		bandInfo[i].depth = depth;
		// マッチング探索打ち切り幅
		bandInfo[i].brkwdt = brkwdt;
		// 拡張マッチング信頼限界
		bandInfo[i].extcnf = extcnf;

		// コントラスト閾値
		bandInfo[i].crstthr = crstthr;
		// コントラストオフセット
		bandInfo[i].crstofs = crstofs;
		// 階調補正モードステータス
		bandInfo[i].grdcrct = grdcrct;

		// マッチングブロック最低輝度比率(%)
		bandInfo[i].minbrtrt = minbrtrt;

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
		bandInfo[i].pimgref = NULL;
		// 入力比較画像データ
		bandInfo[i].pimgcmp = NULL;
		// 入力基準画像データ（12ビット階調対応）
		bandInfo[i].pimgref_16U = pimgref;
		// 入力比較画像データ（12ビット階調対応）
		bandInfo[i].pimgcmp_16U = pimgcmp;

		// マッチングの視差値
		bandInfo[i].pblkdsp = pblkdsp;
		// バックマッチングの視差値
		bandInfo[i].pblkbkdsp = pblkbkdsp;

		// 基準ブロックコントラスト
		bandInfo[i].pblkrefcrst = pblkrefcrst;
		// 比較ブロックコントラスト
		bandInfo[i].pblkcmpcrst = pblkcmpcrst;

		// 基準画像のブロック輝度
		bandInfo[i].pimgrefbrt = pimgrefbrt;
		// 比較画像のブロック輝度
		bandInfo[i].pimgcmpbrt = pimgcmpbrt;

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

