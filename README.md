# isc-dpl
Data processing library for ISC stereo cameras.

****
## Outline 
****
isc-dplは、ISCシリーズのステレオカメラに対応したデータ処理ライブラリと、サンプルプログラムを提供するものです。  
データ処理ライブラリには、現在は3つのライブラリと１つのサポートライブラリが含まれています。  

 - Software Stereo Matching  
    ステレオマッチングを行います  

 - Disparity Filter  
    カメラ本体、又はSoftware Stereo Matchingの出力する視差に対して平均化、補間処理を行います  

 - Self Calibration  
    ソフトウェアによるステレオ平行化を行います  

 - Frame Decoder  
    カメラから取得するデータの展開などを行います（サポートライブラリ）  

サンプルプログラムを実際のステレオカメラと接続して使用することで、データ処理ライブラリの動作検証が可能となります。  

なお、データ処理ライブラリは、それぞれ以下の名称です。

| Library                  | module(class)      | DLL                    |  
| :-------------------     | :---------------   | :-------------------   |  
| Software Stereo Matching | IscStereoMatching  | IscStereoMatching.dll  |  
| Disparity Filter	       | IscDisparityFilter | IscDisparityFilter.dll |  
| Self Calibration	       | IscSelfCalibration | IscSelfCalibration.dll |  
| Frame Decoder	           | IscFrameDecoder    | IscFrameDecoder.dll    |  
    
注意  
*Software Stereo Matchingのアルゴリズムは、ISCシリーズのステレオカメラ本体実装と同一のものではありません*  

****
## Requirements for Windows  
****
- Windows 10(x64)/11  
- Visual Studio 2022 (MFCを使用します C++ MFCを選択してインストールしてください)  
- OpenCV 4.8.0 (これ以外のバージョンも動作可能ですが、その場合はbuildの設定を調整してください)  
- ISC Stereo Camrea  
    - ISC100VM: FPGA(0x75)  
    - ISC100XC: FPGA(0x22)  
- ISC Stereo Camera SDK
    - ISC100VM: 2.3.2
    - ISC100XC: 2.2.2

****
# How to build and Run  
****
- OpenCV  
    - [OpenCV official site](https://opencv.org/releases/)よりダウンロードしてください  
      システムの環境変数に次の値を設定してください  
      OpenCV_DIR = C:\opencv\build (実際の環境に合わせてください)

- build  
    - Visual Studioを使用し、プロジェクトをbuildします  
      ソリューションファイルは、build\windows\isc_dpl_all.sln です  
      実行するプロジェクトは、DPC_guiです  
    - OpenCVのバージョンが4.8.0以外の場合は、リンクしているバージョンを変更してください  
      以下のようにLinkを定義していますので、全て変更してください  
      ```
            #ifdef _DEBUG  
            #pragma comment (lib,"opencv_world480d")  
            #else  
            #pragma comment (lib,"opencv_world480")  
            #endif  
      ```  
    
      設定の必要なファイルは、以下です  
      - source\apps\DPC_gui\DPC_guiDlg.cpp  
      - source\modules\IscDplMainControl\src\isc_main_control_impl.cpp  
      - source\modules\IscCameraControl\src\isc_camera_control.cpp  
      - source\modules\IscDataProcessingControl\src\isc_data_processing_control.cpp  
      - source\modules\IscFrameDecoder\src\isc_framedecoder_interface.cpp  
      - source\modules\IscDisparityFilter\src\isc_disparityfilter_interface.cpp  
      - source\modules\IscStereoMatching\src\isc_stereomatching_interface.cpp  
      - source\modules\IscSelfCalibration\src\isc_selftcalibration_interface.cpp  
      - source\modules\VmSdkWrapper\src\vm_sdk_wrapper.cpp  
      - source\modules\XcSdkWrapper\src\xc_sdk_wrapper.cpp  
      - source\modules\K4aSdkWrapper\src\k4a_sdk_wrapper.cpp  
     
- run  
    - OpenCVのDLLを実行フォルダへコピーします（ファイル名はバージョンによります）  
        - Debug Build: opencv_world480d.dll  
        - Release Build: opencv_world480.dll  
    - ISCステレオカメラ用のDLLを実行フォルダへコピーします  
      DLLは、ISC Stereo Camera入手時に同梱されるUSBに含まれています  
      それぞれ必要なDLLの名称は以下です
        - ISC100VM: ISCLibvm.dll, ISCSDKLibvm200.dll  
        - ISC100XC: ISCLibxc.dll, ISCSDKLibxc.dll
    - ISC100XCを使用する場合は、USB 3.0 to FIFO Bridge Chip FT601(FTDI*)のApplication Library(FTD3XX.dll)を実行フォルダへコピーします  
      FTD3XX.dll は、ISC Stereo Cameraに同梱されるSDKに含まれています  
      または、[FTDI official site](https://ftdichip.com/drivers/d3xx-drivers/)より入手可能です  
    - 実行に必要なパラメータファイルを実行フォルダへコピーします  
      source\apps\ParameterFiles に含まれています

****
## Project structure
****
プロジェクト一覧

|                       | Project                   | exe/dll                       | Content |  
|:-------------------   | :---------------          | :-------------------          | :------------------- |  
| Application           |                           |                               | |  
|                       | DPC_gui                   | DPC_gui.exe                   | |  
| Examples              |                           |                               | |  
|                       | Sample_OpenCV1            | Sample_OpenCV1.exe            | OpenCVを使用するサンプル|  
|                       | Sample_OpenCV2            | Sample_OpenCV2.exe            | OpenCVを使用するサンプル ファイルより再生します|  
|                       | Sample_Yolo1              | Sample_Yolo1.exe              | Yoloを使用するサンプル|  
|                       | Sample_Python1            |                               | Pythonで使用するサンプル|  
| Library               | ISC DPL                   |                               | カメラの制御及びデータ処理を行うライブラリ群です|  
|                       | IscDpl                    | IscDpl.dll                    | インターフェース用DLLです|  
|                       | IscDplC                   | IscDplC.dll                   | インターフェース用DLLです(Extern C)|  
|                       | IscDplMainControl         | IscDplMainControl.dll         | 全体の制御、データの受け渡しを行います|  
|                       | IScCameraControl          | IscCameraControl.dll          | 実カメラの制御及びカメラデータのファイル読み書きを行います|  
|                       | VmSdkWrapper              | VmSdkWrapper.dll              | SDKとのインターフェースです|  
|                       | XcSdkWrapper              | XcSdkWrapper.dll              | SDKとのインターフェースです|  
|                       | IscDataProcessingControl  | IscDataProcessingControl.dll  | データ処理ライブラリの呼び出しを行います|  
|                       | IscStereoMatching         | IscStereoMatching.dll         | ソフトウェアによるステレオマッチングを行います|  
|                       | IscFrameDecoder           | IscFrameDecoder.dll           | カメラデータの展開を行います|  
|                       | IscDataProcessingControl  | IscDataProcessingControl.dll  | データ処理ライブラリの呼び出しを行います|  
|                       | IscSelfCalibration        | IscSelfCalibration.dll        | ソフトウェアによるステレオ平行化を行います|  
| SDK                   |                           |                               | |  
|                       |ISC SDK(VM/XC)             |                               | それぞれのカメラに対応したSDKです|  

****
プロジェクトの全体構成を下図に示します  
<img src="./res/data_processing_lib-Overall-Structure.drawio.png" width="609px">

****
## Manuals
****
[DPC_gui操作説明書](./docs/DPC_gui_Operation_Manual.pdf)  
[isc-dplモジュール説明書](./docs/isc-dpl_Manual.pdf)  
[データ処理ライブラリ説明書](./docs/Library_Manual-A.pdf)  
[セルフキャリブレーションライブラリ説明書](./docs/Library_Manual-B.pdf)  
[IscDpl API Manual](https://itdlab.github.io/isc-dpl-docs/index.html)  

****
## License  
****
This software is licensed under the Apache 2.0 LICENSE.

> Copyright 2023 ITD Lab Corp. All Rights Reserved.  
>    
> Licensed under the Apache License, Version 2.0 (the "License");  
> you may not use this file except in compliance with the License.  
> You may obtain a copy of the License at  
>    
> http://www.apache.org/licenses/LICENSE-2.0  
>    
> Unless required by applicable law or agreed to in writing, software  
> distributed under the License is distributed on an "AS IS" BASIS,  
> WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
> See the License for the specific language governing permissions and  
> limitations under the License.  
    
****  

****  
## Other Libraries  
****  
- OpenCV  
OpenCV 4.5.0 and higher versions are licensed under the Apache 2 License.  
https://opencv.org/license/

- FTDI  
[Future Technology Devices International Limited](https://ftdichip.com/)

*end of document.*  
