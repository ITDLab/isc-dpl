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
 * @file SelfCalibration.cpp
 * @brief Implementation of self calibration
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 *
 * @details
 * - 自律的に補正量を求め、ステレオカメラの平行化を行う
 *
*/

#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <math.h>
#include <functional>

#include "SelfCalibration.h"


// メッシュの最大数
#define EPIPOLAR_POINT_MAX_NUM 5000

// 画像サイズ
#define ISC_IMG_HEIGHT_MAX 720
#define ISC_IMG_WIDTH_MAX 1280

#define IMG_WIDTH_VM 752
#define IMG_WIDTH_XC 1280


// 輝度最小値
// ブロック内の輝度の最大値がこの値未満の場合
// コントラストをゼロにする
#define BLOCK_BRIGHTNESS_MAX 20

// コントラストオフセット
// 入力された画像サイズによってオフセットを選択する
// コントラスト⊿L/Lmeanを1000倍して評価値とする
#define CONTRAST_OFFSET_VM (1.8 * 1000)
#define CONTRAST_OFFSET_XC (1.2 * 1000)

static int contrastOffset = (int)CONTRAST_OFFSET_XC;

// 回転調整勾配幅
// 回転角から調整値を求めるときに勾配幅を使用する
// 調整値 = tan(回転角) * 勾配幅 * 16
#define ROTATION_ADJUST_SLOPE_WIDTH_XC 256
#define ROTATION_ADJUST_SLOPE_WIDTH_VM 376

static int rotationAdjustSlopeWidth = ROTATION_ADJUST_SLOPE_WIDTH_XC;

// 輝度エッジ閾値
#define BRIGHTNESS_EDGE_THRESHOLD 3

// 階調補正モードのコントラスト補正値
#define CONTRAST_CORRECTION_VALUE (0.5)

// 階調補正モードの輝度値補正値
#define BRIGHTNESS_CORRECTION_VALUE (1.25)

// バックグラウンド処理スレッド優先度
#define RARALLELIZING_THREAD_PRIORITY THREAD_PRIORITY_NORMAL

// #define RARALLELIZING_THREAD_PRIORITY THREAD_PRIORITY_BELOW_NORMAL
// #define RARALLELIZING_THREAD_PRIORITY THREAD_PRIORITY_LOWEST
// #define RARALLELIZING_THREAD_PRIORITY THREAD_PRIORITY_IDLE


// //////////////////////
// メッシュ生成
// //////////////////////

/// <summary>
/// 補正画像の高さ
/// </summary>
static int imageHeight;

/// <summary>
/// 補正画像の幅
/// </summary>
static int imageWidth;

/// <summary>
/// メッシュサイズ高さ
/// </summary>
static int epipolarMeshSizeHeight;

/// <summary>
/// メッシュサイズ幅
/// </summary>
static int epipolarMeshSizeWidth;

/// <summary>
/// メッシュ中心x座標
/// </summary>
static int epipolarMeshCenterX;

/// <summary>
/// メッシュ中心y座標
/// </summary>
static int epipolarMeshCenterY;

/// <summary>
/// メッシュ右個数
/// </summary>
static int epipolarMeshNumberRight;

/// <summary>
/// メッシュ左個数
/// </summary>
static int epipolarMeshNumberLeft;

/// <summary>
/// メッシュ上個数
/// </summary>
static int epipolarMeshNumberUpper;

/// <summary>
/// メッシュ下個数
/// </summary>
static int epipolarMeshNumberLower;

/// <summary>
/// 領域座標Top
/// </summary>
static int epipolarMeshRegionTop;

/// <summary>
/// 領域座標Bottom
/// </summary>
static int epipolarMeshRegionBottom;

/// <summary>
/// 領域座標Left
/// </summary>
static int epipolarMeshRegionLeft;

/// <summary>
/// 領域座標Right
/// </summary>
static int epipolarMeshRegionRight;

/// <summary>
/// メッシュ左上x座標
/// </summary>
static int *epipolarMeshRegionX1;

/// <summary>
/// メッシュ左上y座標
/// </summary>
static int *epipolarMeshRegionY1;

/// <summary>
/// メッシュ数
/// </summary>
static int epipolarMeshCount = 0;


// ////////////////////////////
// メッシュパターン強度
// ////////////////////////////

/// <summary>
/// メッシュコントラスト
/// </summary>
static int *epipolarMeshContrast;

/// <summary>
/// メッシュ輝度
/// </summary>
static int *epipolarMeshBrightness;

/// <summary>
/// メッシュ輝度エッジ比率（縦横）
/// </summary>
static double *epipolarMeshEdgeRatioCross;

/// <summary>
/// メッシュ輝度エッジ比率（斜め）
/// </summary>
static double *epipolarMeshEdgeRatioDiagonal;

/// <summary>
/// メッシュ最小輝度値
/// </summary>
static int epipolarMeshMinBrightness = 20;

/// <summary>
/// メッシュ最大輝度値
/// </summary>
static int epipolarMeshMaxBrightness = 200;

/// <summary>
/// メッシュ最小コントラスト
/// </summary>
static int epipolarMeshMinContrast = 1000;

/// <summary>
/// メッシュ最小エッジ比率
/// </summary>
static double epipolarMeshMinEdgeRatio = 40.0;

/// <summary>
/// メッシュパターン強度
/// </summary>
static bool *epipolarMeshTextureStrength;

/// <summary>
/// 階調補正モード 0:オフ 1:オン
/// </summary>
static int gradationCorrectionMode;

/// <summary>
/// コントラスト補正値
/// </summary>
static double contrastCorrect;

/// <summary>
/// 輝度値補正値
/// </summary>
static double brightnessCorrect;



// ///////////////////////////
// メッシュ探索
// ///////////////////////////

/// <summary>
/// メッシュ最大変位高さ
/// </summary>
static int epipolarMeshMaxDisplacementHeight = 5;

/// <summary>
/// メッシュ最小変位幅
/// </summary>
static int epipolarMeshMaxDisplacementWidth = 150;

/// <summary>
/// メッシュ最小一致率
/// </summary>
static double epipolarMeshMinMtchRatio = 98.0;

/// <summary>
/// メッシュ探索範囲高さ
/// </summary>
static int epipolarSearchSpanHeight = 5;

/// <summary>
/// メッシュ探索範囲幅
/// </summary>
static int epipolarSearchSpanWidth = 150;

/// <summary>
/// メッシュ探索領域左上x座標
/// </summary>
static int *epipolarSearchRegionX1;

/// <summary>
/// メッシュ探索領域左上y座標
/// </summary>
static int *epipolarSearchRegionY1;

/// <summary>
/// メッシュパターン一致率
/// </summary>
static double *epipolarMatchRatio;

/// <summary>
/// メッシュパターン一致
/// </summary>
static bool *epipolarMeshMatching;

/// <summary>
/// 一致領域左上サブピクセルx座標(比較画像)
/// </summary>
static double *epipolarMatchRegionSubX1;

/// <summary>
/// 一致領域左上サブピクセルy座標(比較画像)
/// </summary>
static double *epipolarMatchRegionSubY1;


// /////////////////////////////////////
// メッシュズレ量計算
// /////////////////////////////////////

/// <summary>
/// メッシュパターン一致カウント
/// </summary>
static int epipolarMeshMatchCount = 0;

/// <summary>
/// メッシュパターン一致カウント（最新結果）
/// </summary>
static int epipolarCurrentMatchNumber = 0;

/// <summary>
/// メッシュパターン一致インデックス
/// </summary>
static int *epipolarMeshMatchIndex;

/// <summary>
/// メッシュパターン縦方向差分
/// </summary>
static double *epipolarMeshDifferenceY;

/// <summary>
/// メッシュパターン一致縦方向差分
/// </summary>
static double epipolarMatchingDifference = 0.0;

/// <summary>
/// メッシュパターン一致回転
/// </summary>
static double epipolarMatchingRotation = 0.0;

/// <summary>
/// メッシュパターン一致縦方向差分標準偏差
/// </summary>
static double epipolarMatchingStDev = 0.0;

/// <summary>
/// メッシュパターン一致縦方向差分（最新結果）
/// </summary>
static double epipolarCurrentMatchingDifference = 0.0;

/// <summary>
/// メッシュパターン一致回転（最新結果）
/// </summary>
static double epipolarCurrentMatchingRotation = 0.0;

/// <summary>
/// メッシュパターン一致縦方向差分標準偏差（最新結果）
/// </summary>
static double epipolarCurrentMatchingStDev = 0.0;

// //////////////////////////////////
// メッシュズレ量判定
// //////////////////////////////////

/// <summary>
/// メッシュ最小パターン一致数
/// </summary>
static int epipolarMeshMinMatchNumber = 10;

/// <summary>
/// メッシュ最小パターン縦変位偏差
/// </summary>
static double epipolarMeshMaxDiffDeviation = 0.15;

/// <summary>
/// メッシュ変位量平均化フレーム数
/// </summary>
static int epipolarMeshAveragingFrameNumber = 20;

/// <summary>
/// 平均フレームカウント
/// </summary>
static int epipolarAverageFrameCount = 0;

/// <summary>
/// 平均縦変位量
/// </summary>
static double epipolarAverageDifference = 0.0;

/// <summary>
/// 平均回転量
/// </summary>
static double epipolarAverageRotation = 0.0;

/// <summary>
/// 縦変位量標準偏差
/// </summary>
static double epipolarDifferenceDeviation = 0.0;

/// <summary>
/// 校正判定基準フレーム数
/// </summary>
static int epipolarMeshCriteriaFrameCount = 100;

/// <summary>
/// 校正判定基準縦方向ズレ量
/// </summary>
static double epipolarMeshCriteriaDifference = 0.1;

/// <summary>
/// 校正判定基準回転ズレ量
/// </summary>
static double epipolarMeshCriteriaRotation = 0.001;

/// <summary>
/// 校正判定基準標準偏差
/// </summary>
static double epipolarMeshCriteriaDeviation = 0.25;

/// <summary>
/// 現在の上下シフト調整幅
/// </summary>
static double currentVerticalDifference = 0.0;

/// <summary>
/// 現在の上下シフト調整量
/// </summary>
static int currentVerticalShiftValue = 0;

/// <summary>
/// 現在の回転調整幅
/// </summary>
static double currentRotationDifference = 0.0;

/// <summary>
/// 現在の回転調整量
/// </summary>
static int currentRotationValue = 0;

/// <summary>
/// 校正判定回転補正 0:しない 1:する
/// </summary>
static int epipolarMeshCorrectRotate = 0;

/// <summary>
/// 補正量の自動保存 0:しない 1:する
/// </summary>
static int epipolarMeshCorrectAutoSave = 0;


// ////////////////////////////////////////
// エピポーラ線平行化バックグラウンド処理
// ////////////////////////////////////////

/// <summary>
/// 基準画像
/// </summary>
static unsigned char *pImageRef;

/// <summary>
/// 比較画像
/// </summary>
static unsigned char *pImageCmp;

/// <summary>
/// エピポーラ線平行化実行中フラグ
/// </summary>
static bool parallelizingRun = false;

/// <summary>
/// エピポーラ線平行化処理開始フラグ
/// </summary>
static bool parallelizingStart = false;

/// <summary>
/// エピポーラ線平行化処理終了フラグ
/// </summary>
static bool parallelizingStop = false;

/// <summary>
/// スレッドオブジェクトのポインタ
/// </summary>
static HANDLE parallelizingThread;

/// <summary>
/// スレッド実行開始イベントのハンドル 
/// </summary>
static HANDLE startParallelizingEvent;



/// <summary>
/// オブジェクトを生成する
/// </summary>
SelfCalibration::SelfCalibration()
{
}


/// <summary>
/// オブジェクトを破棄する
/// </summary>
SelfCalibration::~SelfCalibration()
{
}


/// <summary>
/// セルフキャリブレーションを初期化する
/// </summary>
/// <param name="imghgt">補正画像の高さ(IN)</param>
/// <param name="imgwdt">補正画像の幅(IN)</param>
void SelfCalibration::initialize(int imghgt, int imgwdt)
{
	// 基準画像
	pImageRef = (unsigned char *)malloc(imghgt * imgwdt * sizeof(unsigned char));
	// 比較画像
	pImageCmp = (unsigned char *)malloc(imghgt * imgwdt * sizeof(unsigned char));

	// メッシュ左上x座標
	epipolarMeshRegionX1 = (int *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(int));
	// メッシュ左上y座標
	epipolarMeshRegionY1 = (int *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(int));

	// メッシュコントラスト
	epipolarMeshContrast = (int *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(int));
	memset(epipolarMeshContrast, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(int));
	// メッシュ輝度
	epipolarMeshBrightness = (int *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(int));
	memset(epipolarMeshBrightness, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(int));
	// メッシュ輝度エッジ比率（縦横）
	epipolarMeshEdgeRatioCross = (double *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	memset(epipolarMeshEdgeRatioCross, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	// メッシュ輝度エッジ比率（斜め）
	epipolarMeshEdgeRatioDiagonal = (double *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	memset(epipolarMeshEdgeRatioDiagonal, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	// メッシュパターン強度
	epipolarMeshTextureStrength = (bool *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(bool));
	memset(epipolarMeshTextureStrength, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(bool));

	// メッシュ探索領域左上x座標
	epipolarSearchRegionX1 = (int *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(int));
	// メッシュ探索領域左上y座標
	epipolarSearchRegionY1 = (int *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(int));

	// メッシュパターン一致率
	epipolarMatchRatio = (double *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	memset(epipolarMeshEdgeRatioDiagonal, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	// メッシュパターン一致
	epipolarMeshMatching = (bool *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(bool));
	memset(epipolarMeshMatching, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(bool));

	// 一致領域左上サブピクセルx座標(比較画像)
	epipolarMatchRegionSubX1 = (double *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	memset(epipolarMatchRegionSubX1, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	// 一致領域左上サブピクセルy座標(比較画像)
	epipolarMatchRegionSubY1 = (double *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	memset(epipolarMatchRegionSubY1, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(double));

	// メッシュパターン一致インデックス
	epipolarMeshMatchIndex = (int *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(int));
	memset(epipolarMeshMatchIndex, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(int));
	// メッシュパターン縦方向差分
	epipolarMeshDifferenceY = (double *)malloc(EPIPOLAR_POINT_MAX_NUM * sizeof(double));
	memset(epipolarMeshDifferenceY, 0x00, EPIPOLAR_POINT_MAX_NUM * sizeof(double));

	// ステレオ平行化スレッドを生成する
	createParallelizingThread();

	if (imgwdt == IMG_WIDTH_VM) {
		contrastOffset = (int)CONTRAST_OFFSET_VM;
		rotationAdjustSlopeWidth = ROTATION_ADJUST_SLOPE_WIDTH_VM;
		// レジスタ読み書き関数を設定する
		setRegisterFunction(0);
	}
	else {
		contrastOffset = (int)CONTRAST_OFFSET_XC;
		rotationAdjustSlopeWidth = ROTATION_ADJUST_SLOPE_WIDTH_XC;
		// レジスタ読み書き関数を設定する
		setRegisterFunction(1);
	}

}


/// <summary>
/// セルフキャリブレーションを終了する
/// </summary>
void SelfCalibration::finalize()
{

	// ステレオ平行化スレッドを削除する
	deleteParallelizingThread();

	free(pImageRef);
	free(pImageCmp);

	free(epipolarMeshRegionX1);
	free(epipolarMeshRegionY1);

	free(epipolarMeshContrast);
	free(epipolarMeshBrightness);
	free(epipolarMeshEdgeRatioCross);
	free(epipolarMeshEdgeRatioDiagonal);
	free(epipolarMeshTextureStrength);

	free(epipolarSearchRegionX1);
	free(epipolarSearchRegionY1);
	free(epipolarMatchRatio);
	free(epipolarMeshMatching);
	free(epipolarMatchRegionSubX1);
	free(epipolarMatchRegionSubY1);

	free(epipolarMeshMatchIndex);
	free(epipolarMeshDifferenceY);


}


/// <summary>
/// メッシュパラメータを設定する
/// </summary>
/// <param name="imghgt">補正画像の高さ(IN)</param>
/// <param name="imgwdt">補正画像の幅(IN)</param>
/// <param name="mshhgt">メッシュサイズ高さ(IN)</param>
/// <param name="mshwdt">メッシュサイズ幅(IN)</param>
/// <param name="mshcntx">メッシュ中心x座標(IN)</param>
/// <param name="mshcnty">メッシュ中心y座標(IN)</param>
/// <param name="mshnrgt">メッシュ右個数(IN)</param>
/// <param name="mshnlft">メッシュ左個数(IN)</param>
/// <param name="mshnupr">メッシュ上個数(IN)</param>
/// <param name="mshnlwr">メッシュ下個数(IN)</param>
/// <param name="rgntop">領域座標Top(IN)</param>
/// <param name="rgnbtm">領域座標Bottom(IN)</param>
/// <param name="rgnlft">領域座標Left(IN)</param>
/// <param name="rgnrgt">領域座標Right(IN)</param>
/// <param name="srchhgt">対応点探索領域の高さ(IN)</param>
/// <param name="srchwdt">対応点探索領域の幅(IN)</param>
void SelfCalibration::setMeshParameter(int imghgt, int imgwdt, 
	int mshhgt, int mshwdt, int mshcntx, int mshcnty,
	int mshnrgt, int mshnlft, int mshnupr, int mshnlwr, 
	int rgntop, int rgnbtm, int rgnlft, int rgnrgt, int srchhgt, int srchwdt)
{

	// 平行化処理を中断する
	bool stflg = suspendParallelizing();

	// 補正画像の高さ
	imageHeight = imghgt;
	// 補正画像の幅
	imageWidth = imgwdt;

	// メッシュサイズ高さ
	epipolarMeshSizeHeight = mshhgt;
	// メッシュサイズ幅
	epipolarMeshSizeWidth = mshwdt;
	// メッシュ中心x座標
	epipolarMeshCenterX = mshcntx;
	// メッシュ中心y座標
	epipolarMeshCenterY = mshcnty;
	// メッシュ右個数
	epipolarMeshNumberRight = mshnrgt;
	// メッシュ左個数
	epipolarMeshNumberLeft = mshnlft;
	// メッシュ上個数
	epipolarMeshNumberUpper = mshnupr;
	// メッシュ下個数
	epipolarMeshNumberLower = mshnlwr;
	// 領域座標Top
	epipolarMeshRegionTop = rgntop;
	// 領域座標Bottom
	epipolarMeshRegionBottom = rgnbtm;
	// 領域座標Left
	epipolarMeshRegionLeft = rgnlft;
	// 領域座標Right
	epipolarMeshRegionRight = rgnrgt;
	// メッシュ探索範囲高さ
	epipolarSearchSpanHeight = srchhgt;
	// メッシュ探索範囲幅
	epipolarSearchSpanWidth = srchwdt;

	// エピポーラメッシュ座標を設定する
	setEpipolarMeshCoodinate();

	// 平均キューをリセットする
	resetAverageQueue();

	// 平行化処理を再開する
	resumeParallelizing(stflg);


}


/// <summary>
/// メッシュ閾値を設定する
/// </summary>
/// <param name="minbrgt">最小平均輝度(IN)</param>
/// <param name="maxbrgt">最大平均輝度(IN)</param>
/// <param name="mincrst">最小コントラスト(IN)</param>
/// <param name="minedgrt">最小エッジ比率(IN)</param>
/// <param name="maxdphgt">最大変位高さIN)</param>
/// <param name="maxdpwdt">最大変位幅(IN)</param>
/// <param name="minmtcrt">最小一致率(IN)</param>
void SelfCalibration::setMeshThreshold(int minbrgt, int maxbrgt, int mincrst, 
	double minedgrt, int maxdphgt, int maxdpwdt, double minmtcrt)
{

	// 平行化処理を中断する
	bool stflg = suspendParallelizing();

	// メッシュ最小輝度値
	epipolarMeshMinBrightness = minbrgt;
	// メッシュ最大輝度値
	epipolarMeshMaxBrightness = maxbrgt;
	// メッシュ最小コントラスト
	epipolarMeshMinContrast = mincrst;
	// メッシュ最小エッジ比率
	epipolarMeshMinEdgeRatio = minedgrt;
	// メッシュ最大変位高さ
	epipolarMeshMaxDisplacementHeight = maxdphgt;
	// メッシュ最小変位幅
	epipolarMeshMaxDisplacementWidth = maxdpwdt;
	// メッシュ最小一致率
	epipolarMeshMinMtchRatio = minmtcrt;

	// 平均キューをリセットする
	resetAverageQueue();

	// 平行化処理を再開する
	resumeParallelizing(stflg);

}


/// <summary>
/// 動作モードを設定する
/// </summary>
/// <param name="grdcrct">階調補正モードステータス 0:オフ 1:オン(IN)</param>
void SelfCalibration::setOperationMode(int grdcrct)
{
	// 階調補正モード
	gradationCorrectionMode = grdcrct;

	// 階調補正モードの場合、メッシュ最小コントラストを下げる
	// コントラストの調整値を設定する
	if (gradationCorrectionMode == 0) {
		contrastCorrect = 1.0;
		brightnessCorrect = 1.0;
	}
	else {
		contrastCorrect = CONTRAST_CORRECTION_VALUE;
		brightnessCorrect = BRIGHTNESS_CORRECTION_VALUE;
	}

}


/// <summary>
/// ズレ量平均パラメータを設定する
/// </summary>
/// <param name="minmshn">最小メッシュ特徴点数(IN)</param>
/// <param name="maxdifstd">最大垂直ズレ標準偏差(IN)</param>
/// <param name="avefrmn">ズレ量平均フレーム数(IN)</param>
void SelfCalibration::setAveragingParameter(int minmshn, double maxdifstd, int avefrmn)
{
	// 平行化処理を中断する
	bool stflg = suspendParallelizing();

	// 最小メッシュ特徴点数
	epipolarMeshMinMatchNumber = minmshn;
	// 最大垂直ズレ標準偏差
	epipolarMeshMaxDiffDeviation = maxdifstd;
	// ズレ量平均フレーム数
	epipolarMeshAveragingFrameNumber = avefrmn;

	// 平均キューをリセットする
	resetAverageQueue();

	// 平行化処理を再開する
	resumeParallelizing(stflg);

}


/// <summary>
/// 平均ズレ量の判定基準を設定する
/// </summary>
/// <param name="calccnt">ズレ量算出回数(IN)</param>
/// <param name="crtrdiff">垂直ズレ量の判定基準(IN)</param>
/// <param name="crtrrot">回転ズレ量の判定基準(IN)</param>
/// <param name="crtrdev">標準偏差の判定基準(IN)</param>
/// <param name="crctrot">回転ズレ補正 0:しない 1:する(IN)</param>
/// <param name="crctsv">補正量の自動保存 0:しない 1:する(IN)</param>
void SelfCalibration::setCriteria(int calccnt, double crtrdiff, double crtrrot, double crtrstd, int crctrot, int crctsv)
{
	// 平行化処理を中断する
	bool stflg = suspendParallelizing();

	// ズレ量算出回数
	epipolarMeshCriteriaFrameCount = calccnt;
	// 校正判定基準縦方向ズレ量
	epipolarMeshCriteriaDifference = crtrdiff;
	// 校正判定基準回転ズレ量
	epipolarMeshCriteriaRotation = crtrrot;
	// 校正判定基準標準偏差
	epipolarMeshCriteriaDeviation = crtrstd;
	// 校正判定回転補正 0:しない 1:する
	epipolarMeshCorrectRotate = crctrot;
	// 補正量の自動保存 0:しない 1:する
	epipolarMeshCorrectAutoSave = crctsv;

	// 平均キューをリセットする
	resetAverageQueue();

	// 平行化処理を再開する
	resumeParallelizing(stflg);

}


/// <summary>
/// メッシュ座標を取得する
/// </summary>
/// <param name="mshrgnx">メッシュx座標の配列ポインタ(OUT)</param>
/// <param name="mshrgny">メッシュy座標の配列ポインタ(OUT)</param>
/// <param name="srchrgnx">メッシュ探索領域x座標の配列ポインタ(OUT)</param>
/// <param name="srchrgny">メッシュ探索領域y座標の配列ポインタ(OUT)</param>
/// <returns>メッシュの数を返す</returns>
int SelfCalibration::getMeshCoordinate(int **mshrgnx, int **mshrgny, int **srchrgnx, int **srchrgny)
{
	// メッシュ座標の配列
	*mshrgnx = epipolarMeshRegionX1;
	*mshrgny = epipolarMeshRegionY1;

	// メッシュ探索領域座標の配列
	*srchrgnx = epipolarSearchRegionX1;
	*srchrgny = epipolarSearchRegionY1;

	// メッシュの数
	return epipolarMeshCount;
}


/// <summary>
/// メッシュのパターン強度を取得する
/// </summary>
/// <param name="mshstrgt">パターン強度判定の配列ポインタ(OUT)</param>
/// <param name="mshbrgt">メッシュ輝度の配列ポインタ(OUT)</param>
/// <param name="mshcrst">メッシュコントラストの配列ポインタ(OUT)</param>
/// <param name="mshedgx">メッシュ輝度エッジ比率（縦横）の配列ポインタ(OUT)</param>
/// <param name="mshedgd">メッシュ輝度エッジ比率（斜め）の配列ポインタ(OUT)</param>
void SelfCalibration::getMeshTextureStrength(bool **mshstrgt, int **mshbrgt, int **mshcrst,
	double **mshedgx, double **mshedgd)
{

	// 平行化処理を中断する
	bool stflg = suspendParallelizing();

	*mshstrgt = epipolarMeshTextureStrength;
	*mshbrgt = epipolarMeshBrightness;
	*mshcrst = epipolarMeshContrast;
	*mshedgx = epipolarMeshEdgeRatioCross;
	*mshedgd = epipolarMeshEdgeRatioDiagonal;

	// 平行化処理を再開する
	resumeParallelizing(stflg);

}


/// <summary>
/// 対応点領域サブピクセル座標を取得する
/// </summary>
/// <param name="mshmtch">メッシュパターン一致判定の配列ポインタ(OUT)</param>
/// <param name="mtchrt">パターン一致率の配列ポインタ(OUT)</param>
/// <param name="mtchsubx">一致領域サブピクセルx座標の配列ポインタ(OUT)</param>
/// <param name="mtchsuby">一致領域サブピクセルy座標の配列ポインタ(OUT)</param>
/// <returns>パターン一致したメッシュの数を返す</returns>
int SelfCalibration::getMatchSubpixelCoordinate(bool **mshmtc, double **mtcrt, double **mtcsubx, double **mtcsuby)
{

	// 平行化処理を中断する
	bool stflg = suspendParallelizing();

	*mshmtc = epipolarMeshMatching;
	*mtcrt = epipolarMatchRatio;
	*mtcsubx = epipolarMatchRegionSubX1;
	*mtcsuby = epipolarMatchRegionSubY1;

	int mtchcnt = epipolarMeshMatchCount;

	// 平行化処理を再開する
	resumeParallelizing(stflg);

	return mtchcnt;
}


/// <summary>
/// 現在のフレームの平行化処理結果をクリアする
/// </summary>
void SelfCalibration::clearCurrentMeshDifference()
{

	epipolarCurrentMatchingDifference = 0.0;
	epipolarCurrentMatchingRotation = 0.0;
	epipolarCurrentMatchingStDev = 0.0;

	epipolarCurrentMatchNumber = 0;

	// 平均キューをリセットする
	resetAverageQueue();

}


/// <summary>
/// 現在のフレームの平行化処理結果を返す
/// </summary>
/// <param name="difvrt">左右画像の縦方向差分(OUT)</param>
/// <param name="difrot">左右画像の回転差分(OUT)</param>
/// <param name="difvstd">左右画像の縦方向差分の標準偏差(OUT)</param>
/// <returns>パターン一致したメッシュの数を返す</returns>
int SelfCalibration::getCurrentMeshDifference(double *difvrt, double *difrot, double *difvstd)
{

	*difvrt = epipolarCurrentMatchingDifference;
	*difrot = epipolarCurrentMatchingRotation;
	*difvstd = epipolarCurrentMatchingStDev;

	return epipolarCurrentMatchNumber;
}


/// <summary>
/// 最新フレームの左右画像のメッシュのズレ量を取得する
/// </summary>
/// <param name="mshdif">メッシュごとの縦方向差分の配列ポインタ(OUT)</param>
/// <param name="difvrt">左右画像の縦方向差分(OUT)</param>
/// <param name="difrot">左右画像の回転差分(OUT)</param>
/// <param name="difvstd">左右画像の縦方向差分の標準偏差(OUT)</param>
void SelfCalibration::getMeshDifference(double **mshdif, double *difvrt, double *difrot, double *difvstd)
{

	// 平行化処理を中断する
	bool stflg = suspendParallelizing();

	*mshdif = epipolarMeshDifferenceY;
	*difvrt = epipolarMatchingDifference;
	*difrot = epipolarMatchingRotation;
	*difvstd = epipolarMatchingStDev;

	// 平行化処理を再開する
	resumeParallelizing(stflg);

}


/// <summary>
/// 左右画像のフレーム平均ズレ量を取得する
/// </summary>
/// <param name="avedvrt">上下ズレ差（画素）(OUT)</param>
/// <param name="avedrot">回転ズレ差（ラジアン）(OUT)</param>
/// <param name="avedvstd">上下ズレ差標準偏差（画素）(OUT)</param>
/// <returns>平均フレームカウントを返す</returns>
int SelfCalibration::getAverageDifference(double *avedvrt, double *avedrot, double *avedvstd)
{
	*avedvrt = epipolarAverageDifference;
	*avedrot = epipolarAverageRotation;
	*avedvstd = epipolarDifferenceDeviation;

	return epipolarAverageFrameCount;
}


/// <summary>
/// 現在の左右画像のズレ補正量を取得する
/// </summary>
/// <param name="vrtdif">上下ズレ補正量（画素）(OUT)</param>
/// <param name="vrtval">上下並進レジスタ値（1/16画素）(OUT)</param>
/// <param name="rotdif">回転ズレ補正量（ラジアン）(OUT)</param>
/// <param name="rotval">回転レジスタ値（1/(256 or 376 * 16)勾配画素）(OUT)</param>
void  SelfCalibration::getCurrentCorrectValue(double *vrtdif, int *vrtval, double *rotdif, int *rotval)
{

	*vrtdif = currentVerticalDifference;
	*vrtval = currentVerticalShiftValue;
	*rotdif = currentRotationDifference;
	*rotval = currentRotationValue;

}


/// <summary>
/// 最新のズレ補正量をカメラに保存する
/// </summary>
void SelfCalibration::saveLatestCorrectValue()
{
	saveAdjustmentValue(currentVerticalShiftValue, currentRotationValue);
}


/// <summary>
/// 左右画像を平行にする
/// </summary>
/// <param name="pimgref">基準補正画像(IN)</param>
/// <param name="pimgcmp">比較補正画像(IN)</param>
void SelfCalibration::parallelize(unsigned char *pimgref, unsigned char *pimgcmp)
{
	epipolarParallelizingBackground(pimgref, pimgcmp);

}


/// <summary>
/// 平行化処理を開始する
/// </summary>
void SelfCalibration::start()
{
	resumeParallelizing(true);
}


/// <summary>
/// 平行化処理を停止する
/// </summary>
void SelfCalibration::stop()
{
	suspendParallelizing();
}


/// <summary>
/// 平行化処理の停止を要求する
/// </summary>
void SelfCalibration::requestStop()
{
	resumeParallelizing(false);
}


/// <summary>
/// 平行化処理ステータスを取得する
/// </summary>
/// <param name="pprcstat">処理中ステータス(OUT)</param>
/// <returns>実行中ステータスを返す</returns>
bool SelfCalibration::getStatus(bool *pprcstat)
{
	*pprcstat = parallelizingStart;

	return parallelizingRun;
}


/// <summary>
/// エピポーラ線を平行化する
/// </summary>
/// <param name="pimgref">基準補正画像(IN)</param>
/// <param name="pimgcmp">比較補正画像(IN)</param>
void SelfCalibration::parallelizeEpipolarLine(unsigned char *pimgref, unsigned char *pimgcmp)
{

	// エピポーラメッシュのコントラストを取得する
	getEpipolarMeshContrast(pimgref);
	// メッシュパターンを探索する
	searchEpipolarMesh(pimgref, pimgcmp);
	// 左右画像のメッシュのズレ量を計算する
	calculateEpipolarMeshDifference();

	// 最新結果
	// 現在のメッシュ一致数をセットする
	epipolarCurrentMatchNumber = epipolarMeshMatchCount;
	// メッシュパターン一致縦方向差分（最新結果表示用）
	epipolarCurrentMatchingDifference = epipolarMatchingDifference;
	// メッシュパターン一致回転（最新結果表示用）
	epipolarCurrentMatchingRotation = epipolarMatchingRotation;
	// メッシュパターン一致縦方向差分標準偏差（最新結果表示用）
	epipolarCurrentMatchingStDev = epipolarMatchingStDev;

	// メッシュの平均ズレ量を取得する
	double diffave; // 平均縦変位量
	double rotave; // 平均回転量
	double avedev; // 縦変位量偏差

	int frmcnt = getAverageDifference(&diffave, &rotave, &avedev);

	// エピポーラ線を校正する
	correctDifference(frmcnt, diffave, rotave, avedev);

}


/// <summary>
/// エピポーラメッシュ座標を設定する
/// </summary>
void SelfCalibration::setEpipolarMeshCoodinate()
{

	// 展開後のメッシュの数
	int nmesh = 0;

	// メッシュ上限数到達フラグ
	bool numlimit = false;

	// メッシュ座標位置（ソート用）
	int *meshpos = epipolarMeshMatchIndex;

	// メッシュサイズ
	int mshhgt = epipolarMeshSizeHeight; // 高さ
	int mshwdt = epipolarMeshSizeWidth; // 幅

	// メッシュの中心座標
	int centerX = epipolarMeshCenterX;
	int centerY = epipolarMeshCenterY;

	// メッシュの個数
	int mshrgtn = epipolarMeshNumberRight; // 右側の個数
	int mshlftn = epipolarMeshNumberLeft; // 左側の個数
	int mshuprn = epipolarMeshNumberUpper; // 上側の個数
	int mshlwrn = epipolarMeshNumberLower; // 下側の個数

	// 領域座標
	int dispImageTop = epipolarMeshRegionTop;
	int dispImageBottom = epipolarMeshRegionBottom;
	int dispImageRight = epipolarMeshRegionRight;
	int dispImageLeft = epipolarMeshRegionLeft;

	// 中心座標からメッシュを展開する
	// 4象限　0:右下 1:右上 2:左上 3:左下
	for (int k = 0; k < 4; k++) {
		int hztn;
		int vrtn;
		int di;
		int dj;

		// メッシュが重ならないように中心座標をずらす
		//                || 
		//       左上(2)  ||  右上(1)　
		// ---------------++-----------------
		// ---------------+X-----------------
		//       左下(3)  ||  右下(0)
		//                ||
		int cntOfsX;
		int cntOfsY;
		// 右下・右上
		if (k == 0 || k == 1) {
			hztn = mshrgtn;
			di = 1;
			cntOfsX = 0;
		}
		// 左上・左下
		else {
			hztn = mshlftn;
			di = -1;
			cntOfsX = -1;
		}
		// 右下・左下
		if (k == 0 || k == 3) {
			vrtn = mshlwrn;
			dj = 1;
			cntOfsY = 0;
		}
		// 右上・左上
		else {
			vrtn = mshuprn;
			dj = -1;
			cntOfsY = -1;
		}

		int topOfs = centerY + cntOfsY;
		int btmOfs = topOfs + dj * (mshhgt - 1);

		for (int j = 0; j < vrtn; j++) {
			int lftOfs = centerX + cntOfsX;
			int rgtOfs = lftOfs + di * (mshwdt - 1);

			for (int i = 0; i < hztn; i++) {

				int lftPos = lftOfs;
				int rgtPos = rgtOfs;
				int topPos = topOfs;
				int btmPos = btmOfs;

				// 2点の座標を領域座標に変換する
				// メッシュの左上をleft、topにする
				// (lftPos, topPos)
				//   +------------+ 
				//   |            |
				//   |            |
				//   +------------+
				//             (rgtPos,btmPos)
				if (k == 2 || k == 3) {
					lftPos = rgtOfs;
					rgtPos = lftOfs;
				}
				if (k == 1 || k == 2) {
					topPos = btmOfs;
					btmPos = topOfs;
				}


				// メッシュ左上座標)
				epipolarMeshRegionX1[nmesh] = lftPos;
				epipolarMeshRegionY1[nmesh] = topPos;

				// メッシュ座標位置を設定する
				meshpos[nmesh] = topPos * imageWidth + lftPos;
				nmesh++;

				if (nmesh >= EPIPOLAR_POINT_MAX_NUM) {
					numlimit = true;
					break;
				}

				lftOfs = rgtOfs + di;
				rgtOfs = lftOfs + di * (mshwdt - 1);
				// 左右領域をはみ出した場合に打ち切る
				if (rgtOfs < dispImageLeft || rgtOfs > dispImageRight) {
					break;
				}
			}
			if (numlimit == true) {
				break;
			}

			topOfs = btmOfs + dj;
			btmOfs = topOfs + dj * (mshhgt - 1);
			// 領域をはみ出した場合に打ち切る
			if (btmOfs < dispImageTop || btmOfs > dispImageBottom) {
				break;
			}
		}
		if (numlimit == true) {
			break;
		}

	}

	// メッシュ数を設定する
	epipolarMeshCount = nmesh;

	// 座標を表示画面の左上から右下へ向かって並べる
	int cdx;
	int cdy;
	int pos;

	for (int i = 0; i < epipolarMeshCount - 1; i++) {
		for (int j = i + 1; j < epipolarMeshCount; j++) {
			if (meshpos[i] > meshpos[j]) {
				cdx = epipolarMeshRegionX1[i];
				cdy = epipolarMeshRegionY1[i];
				pos = meshpos[i];

				epipolarMeshRegionX1[i] = epipolarMeshRegionX1[j];
				epipolarMeshRegionY1[i] = epipolarMeshRegionY1[j];
				meshpos[i] = meshpos[j];
				epipolarMeshRegionX1[j] = cdx;
				epipolarMeshRegionY1[j] = cdy;
				meshpos[j] = pos;

			}
		}
	}

	// メッシュ探索領域を設定する
	for (int i = 0; i < epipolarMeshCount; i++) {
		// メッシュ探索領域左上座標
		epipolarSearchRegionX1[i] = epipolarMeshRegionX1[i];
		epipolarSearchRegionY1[i] = epipolarMeshRegionY1[i] - epipolarSearchSpanHeight;

		// 一致領域サブピクセル座標を初期化する
		epipolarMeshMatching[i] = false;
		epipolarMatchRatio[i] = 0.0;
		epipolarMatchRegionSubX1[i] = 0.0;
		epipolarMatchRegionSubY1[i] = 0.0;
	}

}


/// <summary>
/// メッシュパターンを探索する
/// </summary>
/// <param name="pimgref">基準補正画像(IN)</param>
/// <param name="pimgcmp">比較補正画像(IN)</param>
void SelfCalibration::searchEpipolarMesh(unsigned char *pimgref, unsigned char *pimgcmp)
{

	//
	// 基準補正画像    
	// メッシュ
	// left: epipolarMeshRegionX1
	// top: epipolarMeshRegionY1
	// +-----+
	// |     |
	// |     |
	// +-----+
	//
	// 比較補正左画像
	// 探索領域
	// left: epipolarSearchRegionX1
	// top: epipolarSearchRegionY1
    // +--------------+
	// |              |    
	// |              |    
	// |              |    
	// +--------------+
	//

	// メッシュパターン一致カウントを初期化する
	epipolarMeshMatchCount = 0;
	
	// メッシュサイズ
	int mshwdt = epipolarMeshSizeWidth;
	int mshhgt = epipolarMeshSizeHeight;
	// 探索領域サイズ
	int srchwdt = epipolarMeshSizeWidth + epipolarSearchSpanWidth;
	int srchhgt = epipolarMeshSizeHeight + epipolarSearchSpanHeight * 2;

	for (int i = 0; i < epipolarMeshCount; i++) {
		// パターン強度がある場合
		if (epipolarMeshTextureStrength[i] == true) {

			// メッシュ座標を取得する
			int lftMshPos = epipolarMeshRegionX1[i];
			int rgtMshPos = lftMshPos + mshwdt - 1;
			int topMshPos = epipolarMeshRegionY1[i];
			int btmMshPos = topMshPos + mshhgt - 1;

			// 対応点の探索座標を取得する
			int lftSrchPos = epipolarSearchRegionX1[i];
			int rgtSrchPos = lftSrchPos + srchwdt - 1;
			if (rgtSrchPos >= imageWidth) {
				rgtSrchPos = imageWidth - 1;
			}
			int topSrchPos = epipolarSearchRegionY1[i];
			int btmSrchPos = topSrchPos + srchhgt - 1;

			// 一致領域のサブピクセル座標を取得する
			double mtchRt;
			double topMtchSubPos;
			double btmMtchSubPos;
			double lftMtchSubPos;
			double rgtMtchSubPos;
			getEpipolarMatchPosition(pimgref, pimgcmp,
				topMshPos, btmMshPos, lftMshPos, rgtMshPos, topSrchPos, btmSrchPos, lftSrchPos, rgtSrchPos,
				&mtchRt, &topMtchSubPos, &btmMtchSubPos, &lftMtchSubPos, &rgtMtchSubPos);

			// メッシュパターン一致率
			epipolarMatchRatio[i] = mtchRt;

			// メッシュパターン一致
			if (mtchRt > epipolarMeshMinMtchRatio) {
				epipolarMeshMatching[i] = true;
				epipolarMeshMatchIndex[epipolarMeshMatchCount] = i;
				epipolarMeshMatchCount++;

			}
			else {
				epipolarMeshMatching[i] = false;
			}

			// 一致領域左上サブピクセルx座標(比較画像)
			epipolarMatchRegionSubX1[i] = lftMtchSubPos;
			// 一致領域左上サブピクセルy座標(比較画像)
			epipolarMatchRegionSubY1[i] = topMtchSubPos;

		}
	}

}


/// <summary>
/// 左右画像のメッシュのズレ量を計算する
/// </summary>
void SelfCalibration::calculateEpipolarMeshDifference()
{
	double aveydiff; // 左右のエピポーラ点のy座標差の平均
	double varydiff; // 水平性分散値
	getVarianceVerticalDifference(0.0, &aveydiff, &varydiff);

	double minave = aveydiff;
	double minvar = varydiff;
	double minrot = 0.0;

	// 回転ズレの算出には2個所以上必要
	// 回転させて全てのポイントの高さのズレが同じなる回転角を求める
	if (epipolarMeshMatchCount > 1) {

		int i;

		// 0.0001ラジアン刻み±0.01ラジアン（0.0057度刻み±0.57度）
		// 反時計方向へ最小分散値まで回転させる
		// 0.0001ラジアン刻み
		double radunit = 0.0001;
		int radrange = 100;

		for (i = 1; i <= radrange; i++) {
			getVarianceVerticalDifference(radunit * i, &aveydiff, &varydiff);
			if (minvar > varydiff) {
				minvar = varydiff;
				minave = aveydiff;
				minrot = i * radunit;
			}
			else {
				break;
			}
		}
		// 時計方向へ最小分散値まで回転させる
		if (i == 1) {
			for (i = -1; i >= (-1 * radrange); i--) {
				getVarianceVerticalDifference(radunit * i, &aveydiff, &varydiff);
				if (minvar > varydiff) {
					minvar = varydiff;
					minave = aveydiff;
					minrot = i * radunit;
				}
				else {
					break;
				}
			}
		}

	}

	// メッシュパターン一致縦方向差分
	epipolarMatchingDifference = minave;
	// メッシュパターン一致回転
	epipolarMatchingRotation = minrot;
	// メッシュパターン一致縦方向差分標準偏差
	epipolarMatchingStDev = sqrt(minvar);
	// 最小分散値の回転位置のメッシュの上下ズレを保存する
	getVarianceVerticalDifference(minrot, &aveydiff, &varydiff);

	// 時間平均を求める
	if (epipolarMeshMatchCount >= epipolarMeshMinMatchNumber &&
		epipolarMatchingStDev <= epipolarMeshMaxDiffDeviation) {
		putAverageQueue(minave, minrot);
		epipolarAverageFrameCount = getAverageQueue(epipolarMeshAveragingFrameNumber,
			&epipolarAverageDifference, &epipolarAverageRotation, &epipolarDifferenceDeviation);
	}

	return;
}


/// <summary>
/// 左右の垂直ズレ差の分散を取得する
/// </summary>
/// <param name="raddiff">回転角（ラジアン）(IN)</param>
/// <param name="paveydiff">左右画像のy座標との差の平均(OUT)</param>
/// <param name="pvarydiff">左右画像のy座標との差の分散(OUT)</param>
void SelfCalibration::getVarianceVerticalDifference(double raddiff, double *paveydiff, double *pvarydiff)
{

	//
	// 縦方向ズレ
	// 右画像                     左画像
	// +->---------+-----------+  +->---------+-----------+  
	// |           |           |  |           |           |   A -
	// v           |           |  v           |           |   |
	// +-----------+-----------+  +-----------+-----------+ ----  
	// |           |           |  |           |           |   |
	// |           |           |  |           |           |   V +
	// +-----------+-----------+  +-----------+-----------+  
	//
	// 回転ズレ
	// 右画像                     左画像
	// +->---------+-----------+  +->---------+-----------+  
	// |           |           |  |           |           |   A +
	// v           |           |  v           |           |   |
	// +-----------+-----------+  +-----------+-----------+ ----  
	// |           |           |  |           |           |   |
	// |           |           |  |           |           |   V -
	// +-----------+-----------+  +-----------+-----------+  
	//                            + : 反時計回り
	//

	// 回転の中心座標を求める
	int cx = imageWidth / 2; // 回転の中心x座標
	int cy = imageHeight / 2; // 回転の中心y座標

	double ave_ydiff = 0.0; // y座標差の平均
	double dev_yddiff = 0.0; // y座標差の分散

	if (epipolarMeshMatchCount == 0) {
		*paveydiff = ave_ydiff;
		*pvarydiff = dev_yddiff;
		return;
	}

	double sum_ydiff = 0; // y座標差の合計
	double sum_sqydiff = 0; // y座標差二乗の合計

	// 右カメラのy座標との差の平均を求める
	for (int i = 0; i < epipolarMeshMatchCount; i++) {
		int idx = epipolarMeshMatchIndex[i];
		// 左画像の回転後のy座標
		double ly2 = (epipolarMatchRegionSubX1[idx] - cx) * sin(raddiff) + (epipolarMatchRegionSubY1[idx] - cy) * cos(raddiff) + cy;
		double ydiff = (ly2 - epipolarMeshRegionY1[idx]);

		// メッシュの上下ズレを保存する
		epipolarMeshDifferenceY[idx] = ydiff;

		sum_ydiff += ydiff;
		sum_sqydiff += (ydiff * ydiff);
	}

	// 平均
	ave_ydiff = sum_ydiff / epipolarMeshMatchCount;
	// 分散
	dev_yddiff = (sum_sqydiff - epipolarMeshMatchCount * ave_ydiff * ave_ydiff) / epipolarMeshMatchCount;

	*paveydiff = ave_ydiff;
	*pvarydiff = dev_yddiff;
}


// 探索SADマップ
static long sumij[100][500];

/// <summary>
/// 一致領域の座標を取得する
/// </summary>
/// <param name="pimgref">基準補正画像(IN)</param>
/// <param name="pimgcmp">比較補正画像(IN)</param>
/// <param name="topMshPos">メッシュ上座標(IN)</param>
/// <param name="btmMshPos">メッシュ下座標(IN)</param>
/// <param name="lftMshPos">メッシュ左座標(IN)</param>
/// <param name="rgtMshPos">メッシュ右座標(IN)</param>
/// <param name="topSrchPos">探索領域上座標(IN)</param>
/// <param name="btmSrchPos">探索領域下座標(IN)</param>
/// <param name="lftSrchPos">探索領域左座標(IN)</param>
/// <param name="rgtSrchPos">探索領域右座標(IN)</param>
/// <param name="mtchRt">パターン一致率(OUT)</param>
/// <param name="topMtchPos">一致領域サブピクセル上座標(OUT)</param>
/// <param name="btmMtchPos">一致領域サブピクセル下座標(OUT)</param>
/// <param name="lftMtchPos">一致領域サブピクセル左座標(OUT)</param>
/// <param name="rgtMtchPos">一致領域サブピクセル右座標(OUT)</param>
void SelfCalibration::getEpipolarMatchPosition(unsigned char *pimgref, unsigned char *pimgcmp,
	int topMshPos, int btmMshPos, int lftMshPos, int rgtMshPos,
	int topSrchPos, int btmSrchPos, int lftSrchPos, int rgtSrchPos,
	double *pMtchRt, double *ptopMtchSubPos, double *pbtmMtchSubPos, double *plftMtchSubPos, double *prgtMtchSubPos)
{
	int di, dj, midi, midj, bx, by;
	int djj, dii, midii, midjj;

	// 探索シフト幅を求める 
	int srchjs = topSrchPos - topMshPos;
	int srchje = btmSrchPos - btmMshPos;
	int srchis = lftSrchPos - lftMshPos;
	int srchie = rgtSrchPos - rgtMshPos;

	long misij = 255 * epipolarMeshSizeHeight * epipolarMeshSizeWidth;
	long max_sij = misij;

	// 真っ直ぐ横方向へ探索する
	for (dii = 0, di = srchis; di <= srchie; dii++, di++) {
		long sum = 0;

		for (int jj = topMshPos; jj <= btmMshPos; jj++) {
			for (int ii = lftMshPos; ii <= rgtMshPos; ii++) {
				int rfx = pimgref[jj * imageWidth + ii];
				int cpx = pimgcmp[(jj) * imageWidth + (ii + di)];
				sum += abs(rfx - cpx);
			}
		}

		if (sum < misij) {
			misij = sum;
			midi = di;
			midii = dii;
		}
	}

	// 上下左右に探索する
	// 横方向は±10幅
	misij = max_sij;

	for (djj = 0, dj = srchjs; dj <= srchje; djj++, dj++) {
		for (dii = 0, di = midi - 10; di <= midi + 10; dii++, di++) {
			long sum = 0;

			for (int jj = topMshPos; jj <= btmMshPos; jj++) {
				for (int ii = lftMshPos; ii <= rgtMshPos; ii++) {
					int rfx = pimgref[jj * imageWidth + ii];
					int cpx = pimgcmp[(jj + dj) * imageWidth + (ii + di)];
					sum += abs(rfx - cpx);
				}
			}

			sumij[djj][dii] = sum;

			if (sum < misij) {
				misij = sum;
				midi = di;
				midj = dj;
				midii = dii;
				midjj = djj;
			}
		}
	}

	int topMtchPos = topMshPos + midj;
	int btmMtchPos = btmMshPos + midj;
	int lftMtchPos = lftMshPos + midi;
	int rgtMtchPos = rgtMshPos + midi;

	// サブピクセル座標を求める
	double mtchRt = 0.0; // パターン一致率
	double topMtchSubPos = (double)topMtchPos;
	double btmMtchSubPos = (double)btmMtchPos;
	double lftMtchSubPos = (double)lftMtchPos;
	double rgtMtchSubPos = (double)rgtMtchPos;

	// 変位幅に入っている場合はサブピクセル座標を求める
	if (midj >= (-1) * epipolarMeshMaxDisplacementHeight && midj <= epipolarMeshMaxDisplacementHeight && 
		midi >= 0 && midi <= epipolarMeshMaxDisplacementWidth) {

		by = 0;
		if (sumij[midjj - 1][midii] < sumij[midjj + 1][midii]) {
			by--;
		}
		topMtchPos = topMtchPos + by;
		btmMtchPos = btmMtchPos + by;

		bx = 0;
		if (sumij[midjj][midii - 1] < sumij[midjj][midii + 1]) {
			bx--;
		}
		lftMtchPos = lftMtchPos + bx;
		rgtMtchPos = rgtMtchPos + bx;

		getSubPixelMatchPosition(pimgref, pimgcmp,
			topMshPos, btmMshPos, lftMshPos, rgtMshPos,
			topMtchPos, btmMtchPos, lftMtchPos, rgtMtchPos,
			&mtchRt, &topMtchSubPos, &btmMtchSubPos, &lftMtchSubPos, &rgtMtchSubPos);

	}

	*pMtchRt = mtchRt;
	*ptopMtchSubPos = topMtchSubPos;
	*pbtmMtchSubPos = btmMtchSubPos;
	*plftMtchSubPos = lftMtchSubPos;
	*prgtMtchSubPos = rgtMtchSubPos;

}


// 0.1精度の座標を求めるためのイメージの一時バッファ
static long a_calibration[300][300];
static long b_calibration[300][300];

/// <summary>
/// 一致領域のサブピクセル座標を取得する
/// </summary>
/// <param name="pimgref">基準補正画像(IN)</param>
/// <param name="pimgcmp">比較補正画像(IN)</param>
/// <param name="topMshPos">メッシュ上座標(IN)</param>
/// <param name="btmMshPos">メッシュ下座標(IN)</param>
/// <param name="lftMshPos">メッシュ左座標(IN)</param>
/// <param name="rgtMshPos">メッシュ右座標(IN)</param>
/// <param name="topMtchPos">探索領域上座標(IN)</param>
/// <param name="btmMtchPos">探索領域下座標(IN)</param>
/// <param name="lftMtchPos">探索領域左座標(IN)</param>
/// <param name="rgtMtchPos">探索領域右座標(IN)</param>
/// <param name="mtchRt">パターン一致率(OUT)</param>
/// <param name="ptopMtchSubPos">一致領域サブピクセル上座標(OUT)</param>
/// <param name="pbtmMtchSubPos">一致領域サブピクセル下座標(OUT)</param>
/// <param name="plftMtchSubPos">一致領域サブピクセル左座標(OUT)</param>
/// <param name="prgtMtchSubPos">一致領域サブピクセル右座標(OUT)</param>
void SelfCalibration::getSubPixelMatchPosition(unsigned char *pimgref, unsigned char *pimgcmp,
	int topMshPos, int btmMshPos, int lftMshPos, int rgtMshPos,
	int topMtchPos, int btmMtchPos, int lftMtchPos, int rgtMtchPos,
	double *mtchRt, double *ptopMtchSubPos, double *pbtmMtchSubPos, double *plftMtchSubPos, double *prgtMtchSubPos)
{
	int i, j, ais, ajs, aie, aje, bis, bjs, bie, bje;
	int imx, jmx, ii, jj;
	long sum, sumin;
	double bdi, bdj, midi, midj;

	long subij[11][11];

	ajs = topMshPos; // メッシュ座標上　
	aje = btmMshPos; // メッシュ座標下
	ais = lftMshPos; // メッシュ座標左　
	aie = rgtMshPos; // メッシュ座標右

	imx = aie - ais + 1; // メッシュ幅
	jmx = aje - ajs + 1; // メッシュ高さ

	// メッシュをコピーする
	for (j = ajs; j <= aje; j++) {
		for (i = ais; i <= aie; i++) {
			a_calibration[j - ajs][i - ais] = pimgref[j * imageWidth + i];
		}
	}

	// サブピクセル探索開始座標
	bjs = topMtchPos; // 一致領域上
	bje = btmMtchPos; // 一致領域下
	bis = lftMtchPos; // 一致領域左 
	bie = rgtMtchPos; // 一致領域右

	sumin = jmx * imx * 255;
	long max_sij = sumin;
	midi = midj = 0.0;

	//
	// 1画素内で最小値を求める
	// 4画素に0.1単位の重みを付けて探索する
	//
	//  [j][i]   | [j][i+1]
	// ----------+-----------
	//  [j+1][i] | [j+1][i+1]
	//
	for (bdj = 0.0, jj = 0; bdj < 1.01; bdj += 0.1, jj++) {
		for (bdi = 0.0, ii = 0; bdi < 1.01; bdi += 0.1, ii++) {
			for (j = bjs; j <= bje; j++) {
				for (i = bis; i <= bie; i++) {
					b_calibration[j - bjs][i - bis] =
						(long)((1.0 - bdi)*(1.0 - bdj)*(double)pimgcmp[j * imageWidth + i]
							+ bdi * (1.0 - bdj)*(double)pimgcmp[j * imageWidth + (i + 1)]
							+ (1.0 - bdi)*bdj*(double)pimgcmp[(j + 1) * imageWidth + i]
							+ bdi * bdj*(double)pimgcmp[(j + 1) * imageWidth + (i + 1)]);
				}
			}
			sum = 0;
			for (j = 0; j < jmx; j++) {
				for (i = 0; i < imx; i++) {
					sum += abs(a_calibration[j][i] - b_calibration[j][i]);
				}
			}

			subij[jj][ii] = sum;
			if (sum < sumin) {
				sumin = sum;
				midi = bdi;
				midj = bdj;
			}
		}
	}

	// パターン一致率を算出する
	double mtchrt = (1.0 - (double)sumin / max_sij) * 100;

	*mtchRt = mtchrt;
	*ptopMtchSubPos = (double)topMtchPos + midj;
	*pbtmMtchSubPos = (double)btmMtchPos + midj;
	*plftMtchSubPos = (double)lftMtchPos + midi;
	*prgtMtchSubPos = (double)rgtMtchPos + midi;


}


/// <summary>
/// エピポーラメッシュのコントラストを取得する
/// </summary>
/// <param name="pimgbuf">補正画像(IN)</param>
/// <param name="mshbrgt">メッシュ輝度(OUT)</param>
/// <param name="mshcrst">メッシュコントラスト(OUT)</param>
void SelfCalibration::getEpipolarMeshContrast(unsigned char *pimgbuf)
{

	// ブロック輝度最大値
	int bgtmax = BLOCK_BRIGHTNESS_MAX;

	// コントラストオフセット
	int crstofs = contrastOffset;

	int mshwdt = epipolarMeshSizeWidth;
	int mshhgt = epipolarMeshSizeHeight;

	for (int i = 0; i < epipolarMeshCount; i++) {

		// パターン強度を初期化する
		epipolarMeshTextureStrength[i] = true;

		// 特徴点メッシュの座標を取得する
		int lftPos = epipolarMeshRegionX1[i];
		int rgtPos = lftPos + mshwdt - 1;
		int topPos = epipolarMeshRegionY1[i];
		int btmPos = topPos + mshhgt - 1;

		// 輝度総和
		int Lsum = 0;
		// メッシュ内輝度最小値（初期値256階調最大輝度値）
		int Lmin = 255;
		// メッシュ内輝度最大値（初期値256階調最大輝度値）
		int Lmax = 0;

		// 輝度エッジカウント（縦横）
		int edgcntvh = 0;
		// 輝度エッジカウント（斜め）
		int edgcntdg = 0;

		// メッシュ領域
		int pxlcnt = 0;
		for (int jj = topPos; jj <= btmPos; jj++) {
			for (int ii = lftPos; ii <= rgtPos; ii++) {
				int brgt = pimgbuf[jj * imageWidth + ii];

				int brgtnxth = pimgbuf[jj * imageWidth + ii + 1]; // 右 
				int brgtnxtv = pimgbuf[(jj + 1) * imageWidth + ii]; // 下
				int brgtnxtdu = pimgbuf[(jj - 1) * imageWidth + ii + 1]; // 斜め上 
				int brgtnxtdd = pimgbuf[(jj + 1) * imageWidth + ii + 1]; // 斜め下

				int diffh = abs(brgt - brgtnxth);
				int diffv = abs(brgt - brgtnxtv);
				if (diffh >= BRIGHTNESS_EDGE_THRESHOLD && diffv >= BRIGHTNESS_EDGE_THRESHOLD) {
					edgcntvh++;
				}

				int diffdu = abs(brgt - brgtnxtdu);
				int diffdd = abs(brgt - brgtnxtdd);
				if (diffdu >= BRIGHTNESS_EDGE_THRESHOLD && diffdd >= BRIGHTNESS_EDGE_THRESHOLD) {
					edgcntdg++;
				}

				Lsum += brgt;
				if (Lmin > brgt) {
					Lmin = brgt;
				}
				else if (Lmax < brgt) {
					Lmax = brgt;
				}
				pxlcnt++;
			}
		}

		// 輝度エッジ比率を求める
		double edgrtvh = (double)edgcntvh / pxlcnt * 100;
		epipolarMeshEdgeRatioCross[i] = edgrtvh;
		double edgrtdg = (double)edgcntdg / pxlcnt * 100;
		epipolarMeshEdgeRatioDiagonal[i] = edgrtdg;

		// 平均輝度を
		double Lave = Lsum / pxlcnt;

		// コントラストを求める
		double crst = 0;
		if (Lmax >= bgtmax) {
			crst = ((Lmax - Lmin) * 1000 - crstofs) / Lave;
		}

		// メッシュ輝度
		epipolarMeshBrightness[i] = (int)Lave;

		// メッシュコントラスト
		epipolarMeshContrast[i] = (int)crst;

		// パターン強度
		if ((int)Lave < epipolarMeshMinBrightness || Lave > (epipolarMeshMaxBrightness * brightnessCorrect) ||
			crst < (epipolarMeshMinContrast * contrastCorrect) ||
			edgrtvh < epipolarMeshMinEdgeRatio || edgrtdg < epipolarMeshMinEdgeRatio) {
			epipolarMeshTextureStrength[i] = false;
		}
	}

}


/// <summary>
/// カメラから現在の調整量を取得する
/// </summary>
void  SelfCalibration::getCurrentDifference()
{
	// 現在のズレ量を取得する
	int curvsft;
	double curdvrt;
	int currot;
	double curdrot;

	getVerticalDifference(&curvsft, &curdvrt);
	getRotationalDifference(&currot, &curdrot);

	currentVerticalShiftValue = curvsft;
	currentVerticalDifference = curdvrt;
	currentRotationValue = currot;
	currentRotationDifference = curdrot;


}


/// <summary>
/// エピポーラ線を校正する
/// </summary>
/// <param name="frmcnt">平均フレーム数(IN)</param>
/// <param name="dvrt">平均上下ズレ量(IN)</param>
/// <param name="drot">平均回転ズレ量(IN)</param>
/// <param name="stdev">上下ズレ標準偏差(IN)</param>
void  SelfCalibration::correctDifference(int frmcnt, double dvrt, double drot, double stdev)
{
	constexpr bool output_debug_msg = false;


	// ズレ量を判定する
	// 平均回数が一定以上
	// ズレ量標準偏差が判定以下
	// 平均ズレ量が許容範囲を超えた場合
	// 平均回転量が許容範囲を超えた場合
	if (frmcnt > epipolarMeshCriteriaFrameCount &&
		stdev < epipolarMeshCriteriaDeviation &&
		(dvrt > epipolarMeshCriteriaDifference || dvrt < (-1.0) * epipolarMeshCriteriaDifference ||
			(epipolarMeshCorrectRotate == 1 &&
			(drot > epipolarMeshCriteriaRotation || drot < (-1.0) * epipolarMeshCriteriaRotation)))
		) {

		char msg[256] = { };
		
		if (output_debug_msg) {
			sprintf_s(msg, "[INFO]---Update Epipolar Line---\n");
			printf(msg);
		}

		// 現在の補正量を取得する
		getCurrentDifference();

		if (output_debug_msg) {
			sprintf_s(msg, "	[INFO]Current V-Shift=%d V-diff=%.3f Rot=%d Rot-Diff=%.3f\n",
				currentVerticalShiftValue, currentVerticalDifference, currentRotationValue, currentRotationDifference);
			printf(msg);
		}

		int curvsft = currentVerticalShiftValue;
		double curdvrt = currentVerticalDifference;
		int currot = currentRotationValue;
		double curdrot = currentRotationDifference;

		int adjvsft;
		int adjrot;

		// 上下差から補正量を計算する
		// 右画像基準で上下差は下方向プラス
		// ズレ量をレジスタ値へ換算する
		adjvsft = (int)(dvrt * 16 + 0.5);
		// ズレ量を補正するにレジスタ値を求める
		adjvsft = curvsft - adjvsft;
		// 左右画像の上下シフト量を設定する
		setVerticalDifference(adjvsft);

		if (output_debug_msg) {
			sprintf_s(msg, "	[INFO]New adjvsft=%d\n", adjvsft);
			printf(msg);
		}

		// 回転補正をする場合
		if (epipolarMeshCorrectRotate == 1) {
			// 回転ズレから補正量を計算する
			// 右画像基準で回転ズレは反時計回りプラス
			// ズレ量をレジスタ値へ換算する
			adjrot = (int)(tan(drot) * rotationAdjustSlopeWidth * 16 + 0.5);
			// ズレ量を補正するにレジスタ値を求める
			adjrot = currot - adjrot;
			// 左右画像の回転量を設定する
			setRotationalDifference(adjrot);

			if (output_debug_msg) {
				sprintf_s(msg, "	[INFO]New adjrot=%d\n", adjrot);
				printf(msg);
			}

		}

		// 現在の補正量を取得する
		getCurrentDifference();

		if (output_debug_msg) {
			sprintf_s(msg, "	[INFO]Current V-Shift=%d V-diff=%.3f Rot=%d Rot-Diff=%.3f\n",
				currentVerticalShiftValue, currentVerticalDifference, currentRotationValue, currentRotationDifference);
			printf(msg);
		}

		// 並進、回転基準位置を保存する
		if (epipolarMeshCorrectAutoSave == 1) {
			saveAdjustmentValue(currentVerticalShiftValue, currentRotationValue);

			if (output_debug_msg) {
				sprintf_s(msg, "	[INFO]New V-Shift=%d Rot=%d\n", currentVerticalShiftValue, currentRotationValue);
				printf(msg);
			}
		}

		// 平均キューをリセットする
		resetAverageQueue();

	}

}


// ////////////////////////////////////////
// カメラレジスタ読み出し書き込み
// ////////////////////////////////////////

// レジスタ読み書き関数
static int(*readRegFunc)(int, int *);
static int(*writeRegFunc)(int, int);

// 校正量レジスタ
#define COR_R_TH 0 // 基準画像回転
#define COR_R_SFT_I 1 // 基準画像横シフト
#define COR_R_SFT_J 2 // 基準画像縦シフト
#define COR_L_TH 3 // 比較画像回転
#define COR_L_SFT_I 4 // 比較画像横シフト
#define COR_L_SFT_J 5 // 比較画像縦シフト
#define S_VERTICAL 6 // 基準位置レジスタVertical
#define S_ROTATE 7 // 基準位置レジスタRotate
#define EEPROM_BASE_ADDR 8

#define COR_R_TH_VM 0x00 
#define COR_R_SFT_I_VM 0x01
#define COR_R_SFT_J_VM 0x02
#define COR_L_TH_VM 0x03
#define COR_L_SFT_I_VM 0x04
#define COR_L_SFT_J_VM 0x05
#define S_VERTICAL_VM 0x21
#define S_ROTATE_VM 0x22
#define EEPROM_BASE_ADDR_VM 0x0080

#define COR_R_TH_XC 0x30 // 基準画像回転
#define COR_R_SFT_I_XC 0x31 // 基準画像横シフト
#define COR_R_SFT_J_XC 0x32 // 基準画像縦シフト
#define COR_L_TH_XC 0x33 // 比較画像回転
#define COR_L_SFT_I_XC 0x34 // 比較画像横シフト
#define COR_L_SFT_J_XC 0x35 // 比較画像縦シフト
#define S_VERTICAL_XC 0x61 // 基準位置レジスタVertical
#define S_ROTATE_XC 0x62 // 基準位置レジスタRotate
#define EEPROM_BASE_ADDR_XC 0x00F0

static int RegAddr[2][9] = {
	{
		COR_R_TH_VM,
		COR_R_SFT_I_VM,
		COR_R_SFT_J_VM,
		COR_L_TH_VM,
		COR_L_SFT_I_VM,
		COR_L_SFT_J_VM,
		S_VERTICAL_VM,
		S_ROTATE_VM,
		EEPROM_BASE_ADDR_VM
	},
	{
		COR_R_TH_XC,
		COR_R_SFT_I_XC,
		COR_R_SFT_J_XC,
		COR_L_TH_XC,
		COR_L_SFT_I_XC,
		COR_L_SFT_J_XC,
		S_VERTICAL_XC,
		S_ROTATE_XC,
		EEPROM_BASE_ADDR_XC
	}
};


/// <summary>
/// レジスタ読み書き関数を設定する
/// </summary>
/// <param name="type">FPGAタイプ 0:VM 1:XC(IN)</param>
void  SelfCalibration::setRegisterFunction(int type)
{
	if (type == 0) {
		readRegFunc = &readRegisterForVM;
		writeRegFunc = &writeRegisterForVM;
	}
	else {
		readRegFunc = &readRegisterForXC;
		writeRegFunc = &writeRegisterForXC;
	}

}


/// <summary>
/// 左右画像の上下シフト量を設定する
/// </summary>
/// <param name="adjvsft">上下並進レジスタ値 1/16画素(IN)</param>
void  SelfCalibration::setVerticalDifference(int adjvsft)
{
	int r_v_sft = 0;
	int l_v_sft = 0;

	if (adjvsft > 0) {
		r_v_sft = adjvsft;
	}
	else {
		l_v_sft = (-1) * adjvsft;
	}

	// 上下シフト量を書き込む
	(*writeRegFunc)(COR_R_SFT_J, r_v_sft);
	(*writeRegFunc)(COR_L_SFT_J, l_v_sft);

}


/// <summary>
/// 左右画像の回転量を設定する
/// </summary>
/// <param name="adjrot">回転レジスタ値 1/(256 * 16)勾配画素(IN)</param>
void SelfCalibration::setRotationalDifference(int adjrot)
{

	int r_rot = 0;
	int l_rot = 0;

	if (adjrot > 0) {
		r_rot = adjrot;
	}
	else {
		l_rot = (-1) * adjrot;
	}

	// 回転補正量を書き込む
	(*writeRegFunc)(COR_R_TH, r_rot);
	(*writeRegFunc)(COR_L_TH, l_rot);

}


/// <summary>
/// 左右画像の上下シフト量を取得する
/// </summary>
/// <param name="pvsft">上下並進レジスタ値 1/16画素(OUT)</param>
/// <param name="pdvrt">上下ズレ差 画素(OUT)</param>
void  SelfCalibration::getVerticalDifference(int *pvsft, double *pdvrt)
{
	// 現在の補正量を取得する
	int nRet;

	// 現在の上下シフト量を取得する
	// 基準位置レジスタは左画像が下がるとプラス
	int r_v_sft = 0;
	int l_v_sft = 0;
	int vsft;

	// 右画像基準
	// 右画像                     左画像
	// +-----------+-----------+  +-----------+-----------+  
	// |           |           |  |           |           |   A -
	// |           |           |  |           |           |   |
	// +-----------+-----------+  +-----------+-----------+ ----  
	// |           |           |  |           |           |   |
	// |           |           |  |           |           |   V +
	// +-----------+-----------+  +-----------+-----------+  
	//
	nRet = (*readRegFunc)(COR_R_SFT_J, &r_v_sft);
	nRet = (*readRegFunc)(COR_L_SFT_J, &l_v_sft);
	vsft = r_v_sft - l_v_sft;

	// 基準位置座標レジスタの値と同じ
	*pvsft = vsft;

	// 上下シフト量（画素）
	*pdvrt = (double)vsft / 16;

}


/// <summary>
///  左右画像の回転量を取得する
/// </summary>
/// <param name="prot">回転レジスタ値 1/(256 * 16)勾配画素(OUT)</param>
/// <param name="drot">回転ズレ差 ラジアン(OUT)</param>
void SelfCalibration::getRotationalDifference(int *prot, double *drot)
{
	// 現在の補正量を取得する
	int nRet;

	// 現在の回転量を取得する
	// rot 右画像が時計回りに回転するとプラス
	// 相対的に左画像が反時計回りに回転する
	int r_rot = 0;
	int l_rot = 0;
	int rot;

	// 右画像基準
	// 回転ズレ
	// 右画像                     左画像
	// +-----------+-----------+  +-----------+-----------+  
	// |           |           |  |           |           |   A +
	// |           |           |  |           |           |   |
	// +-----------+-----------+  +-----------+-----------+ ----  
	// |           |           |  |           |           |   |
	// |           |           |  |           |           |   V -
	// +-----------+-----------+  +-----------+-----------+  
	//                            + : 反時計回り
	nRet = (*readRegFunc)(COR_R_TH, &r_rot);
	nRet = (*readRegFunc)(COR_L_TH, &l_rot);
	rot = r_rot - l_rot;

	// 基準位置レジスタの値と同じ
	*prot = rot;

	// 回転量（ラジアン）
	*drot = atan((double)rot / (rotationAdjustSlopeWidth * 16));

}


#define EEPROM_TABLE 0x0001
#define EEPROM_SYSTEM_REG 0x0002
#define EEPROM_AUTOCALIB_REG 0x0004

/// <summary>
/// レジスタの値をEEPROMに保存する
/// </summary>
/// <returns>処理結果を返す</returns>
int SelfCalibration::saveEEPROMCalib()
{

	int	nRet;
	int val;

	nRet = (*writeRegFunc)(EEPROM_BASE_ADDR, EEPROM_AUTOCALIB_REG);

	if (nRet == 0) {

		while (true) {
			nRet = (*readRegFunc)(EEPROM_BASE_ADDR, &val);

			if (nRet != 0) {
				break;
			}
			if ((val & EEPROM_AUTOCALIB_REG) == 0) {
				break;
			}
			Sleep(1);
		}
	}

	return nRet;
}


/// <summary>
/// エピポーラ線校正値をカメラに保存する
/// </summary>
/// <param name="vsft">上下並進レジスタ値 1/16画素（符号反転）(IN)</param>
/// <param name="rot">回転レジスタ値 1/(256 * 16)勾配画素(IN)</param>
void SelfCalibration::saveAdjustmentValue(int vsft, int rot)
{

	// 補正量を計算する
	// 基準位置レジスタは符号付き
	int adj_vsft = (vsft) & 0x01ff;
	int adj_th = rot & 0x01ff;

	// 並進基準位置レジスタ
	(*writeRegFunc)(S_VERTICAL, adj_vsft);
	// 回転基準位置レジスタ
	(*writeRegFunc)(S_ROTATE, adj_th);

	// EEPROMに保存する
	saveEEPROMCalib();

}

using GetCameraRegDataMethod = std::function<int(unsigned char*, unsigned char*, int, int)>;
using SetCameraRegDataMethod = std::function<int(unsigned char*, int)>;

GetCameraRegDataMethod GetCameraRegData = NULL;
SetCameraRegDataMethod SetCameraRegData = NULL;

/// <summary>
/// Call Back関数を設定する
/// </summary>
/// <param name="func_get_camera_reg">読み込み関数</param>
/// <param name="func_set_camera_reg">書き込み関数</param>
/// <returns>処理結果を返す</returns>
void SelfCalibration::SetCallbackFunc(std::function<int(unsigned char*, unsigned char*, int, int)> func_get_camera_reg, std::function<int(unsigned char*, int)> func_set_camera_reg)
{
	GetCameraRegData = func_get_camera_reg;
	SetCameraRegData = func_set_camera_reg;

	return;
}


#define USB_WRITE_CMD_SIZE_VM		5
#define USB_WRITE_RDC_SIZE_VM		3
#define USB_READ_DATA_SIZE_VM		16

/// <summary>
/// レジスタから値を読み出す
/// </summary>
/// <param name="addridx">レジスタアドレス番号</param>
/// <param name="pval">値</param>
/// <returns>処理結果を返す</returns>
int SelfCalibration::readRegisterForVM(int addridx, int *pval)
{

	int		nRet;
	int		nValue;

	int addr = RegAddr[0][addridx];

	BYTE wbuf[USB_WRITE_CMD_SIZE_VM] = { 0 };
	BYTE rbuf[USB_READ_DATA_SIZE_VM] = { 0 };

	wbuf[0] = 0xF1;
	wbuf[1] = addr >> 8 & 0xFF;
	wbuf[2] = addr & 0xFF;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	nRet = GetCameraRegData(wbuf, rbuf, USB_WRITE_RDC_SIZE_VM, USB_READ_DATA_SIZE_VM);

	if (nRet == 0) {
		nValue = rbuf[6] << 8 | rbuf[7];
		*pval = nValue;
	}

	Sleep(20);

	return nRet;
}


/// <summary>
/// レジスタに値を書き込む
/// </summary>
/// <param name="addridx">レジスタアドレス番号</param>
/// <param name="val">値</param>
/// <returns>処理結果を返す</returns>
int SelfCalibration::writeRegisterForVM(int addridx, int val)
{

	int addr = RegAddr[0][addridx];

	BYTE wbuf[USB_WRITE_CMD_SIZE_VM] = { 0 };

	wbuf[0] = 0xF0;
	wbuf[1] = addr >> 8 & 0xFF;
	wbuf[2] = addr & 0xFF;
	wbuf[3] = val >> 8 & 0xFF;
	wbuf[4] = val & 0xFF;

	int nRet = SetCameraRegData(wbuf, USB_WRITE_CMD_SIZE_VM);

	Sleep(20);

	return nRet;
}


#define USB_WRITE_CMD_SIZE_XC 8
#define USB_WRITE_RDC_SIZE_XC 8
#define USB_READ_DATA_SIZE_XC 8

/// <summary>
/// レジスタから値を読み出す
/// </summary>
/// <param name="addridx">レジスタアドレス番号</param>
/// <param name="pval">値</param>
/// <returns>処理結果を返す</returns>
int SelfCalibration::readRegisterForXC(int addridx, int *pval)
{

	int		nRet;
	int		nValue;

	int addr = RegAddr[1][addridx];

	BYTE wbuf[USB_WRITE_CMD_SIZE_XC] = { 0 };
	BYTE rbuf[USB_READ_DATA_SIZE_XC] = { 0 };

	wbuf[0] = 0xF1;
	wbuf[1] = addr >> 8 & 0xFF;
	wbuf[2] = addr & 0xFF;
	wbuf[3] = 0x00;
	wbuf[4] = 0x00;

	nRet = GetCameraRegData(wbuf, rbuf, USB_WRITE_RDC_SIZE_XC, USB_READ_DATA_SIZE_XC);

	if (nRet == 0) {
		nValue = rbuf[6] << 8 | rbuf[7];
		*pval = nValue;
	}

	Sleep(20);

	return nRet;
}


/// <summary>
/// レジスタに値を書き込む
/// </summary>
/// <param name="addridx">レジスタアドレス番号</param>
/// <param name="val">値</param>
/// <returns>処理結果を返す</returns>
int SelfCalibration::writeRegisterForXC(int addridx, int val)
{

	int addr = RegAddr[1][addridx];

	BYTE wbuf[USB_WRITE_CMD_SIZE_XC] = { 0 };

	wbuf[0] = 0xF0;
	wbuf[1] = addr >> 8 & 0xFF;
	wbuf[2] = addr & 0xFF;
	wbuf[3] = val >> 8 & 0xFF;
	wbuf[4] = val & 0xFF;

	int nRet = SetCameraRegData(wbuf, USB_WRITE_CMD_SIZE_XC);

	Sleep(20);

	return nRet;
}



// ////////////////////////////////////////
// エピポーラ線平行化バックグラウンド処理
// ////////////////////////////////////////

/// <summary>
/// エピポーラ線平行化スレッドを生成する
/// </summary>
void SelfCalibration::createParallelizingThread()
{
	// 開始イベントを生成する
	// 自動リセット非シグナル状態
	startParallelizingEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// スレッドを生成する
	parallelizingThread = (HANDLE)_beginthreadex(0, 0, parallelizingThreadFunction, NULL, 0, 0);

	SetThreadPriority(parallelizingThread, RARALLELIZING_THREAD_PRIORITY);

}


/// <summary>
/// エピポーラ線平行化スレッドを削除する
/// </summary>
void SelfCalibration::deleteParallelizingThread()
{
	// 停止フラグを設定する
	parallelizingStop = true;
	// 開始イベントを送信する
	SetEvent(startParallelizingEvent);
	// 受信スレッドの終了を待つ
	WaitForSingleObject(parallelizingThread, INFINITE);

	// スレッドオブジェクトを破棄する
	CloseHandle(parallelizingThread);

	// イベントオブジェクトを破棄する
	CloseHandle(startParallelizingEvent);


}


/// <summary>
/// エピポーラ線平行化処理を中断する
/// </summary>
/// <returns>現在のステータスを返す</returns>
bool SelfCalibration::suspendParallelizing()
{
	bool stflg = parallelizingRun;

	if (stflg == true) {
		parallelizingRun = false;

		while (parallelizingStart == true) {
			Sleep(100);
		}
	}

	return stflg;
}


/// <summary>
/// エピポーラ線平行化処理を再開する
/// </summary>
/// <param name="stflg">戻すステータス(IN)</param>
void SelfCalibration::resumeParallelizing(bool stflg)
{
	parallelizingRun = stflg;
}


/// <summary>
/// エピポーラ線平行化スレッド関数
/// </summary>
/// <param name="parg">スレッド引数（未使用）(IN)</param>
/// <returns>処理結果を返す</returns>
UINT SelfCalibration::parallelizingThreadFunction(LPVOID parg)
{
	DWORD st;

	while (1) {
		// 実行開始イベントを待つ
		st = WaitForSingleObject(startParallelizingEvent, INFINITE);

		// 停止フラグをチェックする
		if (parallelizingStop == true) {
			break;
		}

		// エピポーラ線を平行化する
		parallelizeEpipolarLine(pImageRef, pImageCmp);

		// 開始フラグをリセットする
		parallelizingStart = false;

	}

	return 0;
}


/// <summary>
/// エピポーラ線平行化処理をバックグラウンド実行する
/// </summary>
/// <param name="pimgref">基準補正画像(IN)</param>
/// <param name="pimgcmp">比較補正画像(IN)</param>
void SelfCalibration::epipolarParallelizingBackground(unsigned char *pimgref, unsigned char *pimgcmp)
{

	// 開始フラグをチェックする
	if (parallelizingRun == true && parallelizingStart == false) {

		// 開始フラグをセットする
		parallelizingStart = true;

		// 画像をコピーする
		memcpy(pImageRef, pimgref, imageHeight * imageWidth);
		memcpy(pImageCmp, pimgcmp, imageHeight * imageWidth);

		// 開始イベントを送信する
		SetEvent(startParallelizingEvent);

	}

}


// ////////////////////////////////////////
// 移動平均キュー処理
// ////////////////////////////////////////

#define EPIPOLAR_AVERAGE_QUEUE_SIZE 200

// 縦差分量の移動平均キュー 
static double diffQueue[EPIPOLAR_AVERAGE_QUEUE_SIZE];
static int diffQueueIndex;
static bool diffQueueWrap;
static int diffQueueCount;

// 回転量の移動平均キュー
static double rotQueue[EPIPOLAR_AVERAGE_QUEUE_SIZE];
static int rotQueueIndex;
static bool rotQueueWrap;
static int rotQueueCount;

/// <summary>
/// 平均キューをリセットする
/// </summary>
void SelfCalibration::resetAverageQueue()
{
	clearQueue(&diffQueueIndex, &diffQueueWrap, &diffQueueCount);
	clearQueue(&rotQueueIndex, &rotQueueWrap, &rotQueueCount);
}


/// <summary>
/// 平均キューへ書き込む
/// </summary>
/// <param name="diffval">縦差分量(IN)</param>
/// <param name="rotval">回転量(IN)</param>
void SelfCalibration::putAverageQueue(double diffval, double rotval)
{
	putQueue(&diffQueueIndex, &diffQueueWrap, &diffQueueCount, diffQueue, EPIPOLAR_AVERAGE_QUEUE_SIZE, diffval);
	putQueue(&rotQueueIndex, &rotQueueWrap, &rotQueueCount, rotQueue, EPIPOLAR_AVERAGE_QUEUE_SIZE, rotval);
}


/// <summary>
/// 平均キューから平均を算出する
/// </summary>
/// <param name="avewdt">平均幅(IN)</param>
/// <param name="diffave">平均縦差分量(OUT)</param>
/// <param name="rotave">平均回転量(OUT)</param>
/// <param name="diffstdev">縦差分量標準偏差(OUT)</param>
/// <returns>書き込み回数を返す</returns>
int SelfCalibration::getAverageQueue(int avewdt, double *diffave, double *rotave, double *diffstdev)
{
	double max;
	double min;
	double stdev;

	int cnt = getMovingAverageQueue(diffQueueIndex, diffQueueWrap, diffQueueCount, diffQueue, EPIPOLAR_AVERAGE_QUEUE_SIZE,
		avewdt, diffave, &min, &max, diffstdev);
	getMovingAverageQueue(rotQueueIndex, rotQueueWrap, rotQueueCount, rotQueue, EPIPOLAR_AVERAGE_QUEUE_SIZE,
		avewdt, rotave, &min, &max, &stdev);

	return cnt;
}


/// <summary>
/// 移動平均算出キューをクリアする
/// </summary>
/// <param name="pidx">現在の書き込み位置のインデックス(OUT)</param>
/// <param name="pwarp">キューが一巡したことを示すフラグ(OUT)</param>
/// <param name="pcnt">キューへの書き込み回数(OUT)</param>
void SelfCalibration::clearQueue(int *pidx, bool *pwarp, int *pcnt)
{
	*pidx = 0;
	*pwarp = false;
	*pcnt = 0;
}


/// <summary>
/// 移動平均算出キューに値を書き込む
/// </summary>
/// <param name="pidx">現在の書き込み位置のインデックス(IN/OUT)</param>
/// <param name="pwarp">キューが一巡したことを示すフラグ(IN/OUT)</param>
/// <param name="pcnt">キューへの書き込み回数(IN/OUT)</param>
/// <param name="pqueue">キュー(OUT)</param>
/// <param name="qsize">キューのサイズ(IN)</param>
/// <param name="value">キューに書き込む値(IN)</param>
void SelfCalibration::putQueue(int *pidx, bool *pwarp, int *pcnt, double *pqueue, int qsize, double value)
{
	int qindex = *pidx;

	pqueue[qindex] = value;

	qindex++;

	if (qindex < qsize) {
		*pidx = qindex;
	}
	else {
		*pwarp = true;
		*pidx = 0;
	}

	(*pcnt)++;
}


/// <summary>
/// 移動平均算出キューから指定した数の平均値を取り出す
/// </summary>
/// <param name="idx">現在の書き込み位置のインデックス(IN)</param>
/// <param name="warp">キューが一巡したことを示すフラグ(IN)</param>
/// <param name="cnt">キューへの書き込み回数(IN)</param>
/// <param name="pqueue">キュー(IN)</param>
/// <param name="qsize">キューのサイズ(IN)</param>
/// <param name="width">平均算出のサイズ(IN)</param>
/// <param name="pave">平均値(OUT)</param>
/// <param name="pmin">平均算出のサイズ内に最小値(OUT)</param>
/// <param name="pmax">平均算出のサイズ内に最大値(OUT)</param>
/// <param name="pstddev">標準偏差(OUT)</param>
/// <returns>キューへの書き込み回数を返す</returns>
int SelfCalibration::getMovingAverageQueue(int idx, bool warp, int cnt, double *pqueue, int qsize,
	int width, double *pave, double *pmin, double *pmax, double *pstddev)
{
	int endidx, startidx;
	int i, j;
	double sum = 0.0;
	double sqsum = 0.0;
	double average;
	double value;
	double valmax = 0.0;
	double valmin = 0.0;
	double sqave = 0.0;
	double stddev = 0.0;

	endidx = idx;
	startidx = endidx - width;
	if (startidx < 0) {
		if (warp == false) {
			startidx = 0;
		}
		else {
			startidx = qsize + startidx;
			endidx = qsize + endidx;
		}
	}

	if ((endidx - startidx) > 0) {

		for (i = startidx; i < endidx; i++) {
			j = i % qsize;
			value = pqueue[j];
			if (i == startidx) {
				valmax = value;
				valmin = value;
			}
			else {
				if (value > valmax) {
					valmax = value;
				}
				if (value < valmin) {
					valmin = value;
				}
			}
			sum += value;
			sqsum += value * value;
		}
		average = sum / (endidx - startidx);
		// 標準偏差を求める
		sqave = sqsum / (endidx - startidx);
		stddev = sqrt(sqave - average * average);
	}

	*pave = average;
	*pmax = valmax;
	*pmin = valmin;
	*pstddev = stddev;

	return cnt;
}

