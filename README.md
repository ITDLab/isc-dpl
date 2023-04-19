# isc-dpl
Data processing library for ISC stereo cameras.

****
## Outline 
****
isc-dplは、ISCシリーズのステレオカメラに対応したデータ処理ライブラリと、その使用方法を提示するサンプルプロジェクトにより構成されています。  

プロジェクトには、２つのデータ処理ライブラリが含まれています。  

 - Soft Matching  
　ステレオマッチングを行います。  

 - Frame Decoder  
　視差の平均化、補間処理を行います。  

これらのライブラリのソースコードを提供すると共に、実際のステレオカメラと接続して使用する方法を提示します。  

なお、本プロジェクトにおいては、それぞれ以下のモジュール名で提供されています。

| Library              | module(class)    | DLL                  |  
| :------------------- | :--------------- | :------------------- |  
| Soft Stereo Matching | IscBlockMatching | IscBlockMatching.dll |  
| Frame Decoder	       | IscFrameDecoder  | IscFrameDecoder.dll  |  
    
注意  
*Soft Matchingのアルゴリズムは、ISCシリーズのステレオカメラ実装と同一のものではありません*  

****
## Requirements for Windows  
****
- Windows 10(x64)/11  
- Visual Studio 2022  
- OpenCV 4.5.0 and higher versions  
- ISC Stereo Camrea  
    - ISC100VM: FPGA(0x73~)  
    - ISC100XC: FPGA(0x22~)  

****
# How to build  
****
- OpenCV  
    - [OpenCV official site](https://opencv.org/releases/)よりダウンロードしてください  
      システムの環境変数に次の値を設定してください  
      OpenCV_DIR = C:\opencv\build (your path)

- build  
    - Visual Studioを使用し、プロジェクトをbuildします  
      ソリューションファイルは、build\windows\isc_dpl_all.sln　です。  

- run  
    - 実行フォルダにISCステレオカメラ用のDLLをコピーします  
      それぞれ以下に含まれます  
        - ISC100VM: 3rdparty\ITDLab\vm\2.4.0.0\bin  
        - ISC100XC: 3rdparty\ITDLab\xc\2.2.4.0\bin

****
## Project structure
****
![Diagram](./res/data_processing_lib-Overall-Structure.drawio.png)
- DPC_gui 表示及び各種の制御を行うGUIです  
    
- ISC DPL カメラの制御及びデータ処理を行うライブラリ群です  
    + IscDpl インターフェース用DLLです  
    + IscDplMainControl 全体の制御、データの受け渡しを行います  
    + IScCameraControl 実カメラの制御及びカメラデータのファイル読み書きを行います  
        + VmSdkWrapper  
        + XcSdkWrapper  
    + IscDataProcessingControl  データ処理ライブラリの呼び出しを行います
        + IscBlockMatching ステレオマッチングを行います  
        + IscFrameDecoder 視差の平均化、補間処理を行います  
    
- ISC SDK(VM/XC) それぞれのカメラに対応したSDKです  

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
*end of document.*  
