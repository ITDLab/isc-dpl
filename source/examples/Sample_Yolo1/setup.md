# How to set up your environment  

****
## 概要  
****
YOLOの準備方法を説明します  

****
## 1.準備  
****
Githubにある、[AlexeyAB/darknet](https://github.com/AlexeyAB/darknet)を参照します  
”Requirements for Windows, Linux and macOS”の指定に従います  

### 注意事項  
- Visual Studio では、　英語の言語パッケージの導入が必要です  
- 環境変数に以下を設定します（設定しなくても、Build可能な様子ですが、念のため設定しておきます）  
    - CUDA_BIN_PATH  
      例）C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v10.2  
    - CUDA_TOOLKIT_ROOT_DIR  
      例）C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v10.2  
    - VCPKG_DEFAULT_TRIPLET  
      x64-windows  
    - VCPKG_ROOT  
      vcpkgのインストール先\vcpkg　＜＝　vcpkgをインストール予定のフォルダです
    - OpenCV_DIR  
      例） C:\opencv\build  
    - CUDAインストール時にzlibwapi.dll　を入手しておきます  
      zlibwapi.dll　は、実行フォルダへコピーしておきます  
      入手先は、CUDAのInstallation Guide　にあります  
****
## 2.VCPKGのインストール
****
適当な作業をフォルダを作成し、そこで行います  
- git clone　https://github.com/Microsoft/vcpkg.git  
- cd vcpkg（vcpkgのインストール先をカレントとします…）  
- Bootstarap-vcpkg.bat  
- vcpkg integrate install  
- vcpkg install curl  
- vcpkg install curl:x86-windows  
- vcpkg install curl:x64-windows  
- vcpkg install curl:x64-windows-static  
****
## 3.YOLOのBuild
****
“How to compile on Windows (using vcpkg)”　に従います  
### 2023/5/31現在  
- Set-ExecutionPolicy unrestricted -Scope CurrentUser -Force  
- git clone https://github.com/AlexeyAB/darknet  
- cd darknet  
- .\build.ps1 -UseVCPKG -EnableOPENCV -EnableCUDA -EnableCUDNN  
### build.ps1　でエラーになる場合  
    - build.ps1　内の981,982行目  
    Write-Host "Removing folder $build_folder" -ForegroundColor Yellow  
    Remove-Item -Force -Recurse -ErrorAction SilentlyContinue $build_folder にあるbuild_folder　が未定義です
    release_build_folder　の間違いと思われるので、修正します  
    ただし、修正してもフォルダ build_release　が残るので、手動で削除します  
****
## 4.動作確認
****
- weightファイルをダウンロードする  
Github [AlexeyAB/darknet](https://github.com/AlexeyAB/darknet)　のlinkより入手可能です  

- 実行  
darknetフォルダにdarknet.exe がありますので、これを使用します  

darknet.exe detector test cfg/coco.data cfg/yolov4.cfg yolov4.weights -thresh 0.25 -ext_output data/dog.jpg  
****
## 5.DLLのBuild
****
”How to use Yolo as DLL and SO libraries”　に従います  
Visual Studio　にて、build\darknet\yolo_cpp_dll.sln　を使用してBuildします  
### 注意事項
2023/5/31現在は、Debug Buildに失敗します  
Property -> CUDA C/C++ -> Device　の設定がReleaseと違っています  
この設定を、自分の使用しているデバイスに合わせて設定します  
また、Debug/Releaseとも、同じフォルダに出力するので、設定を変更し、Build後にコピーしておきます  
****
## 6.サンプルプロジェクトのBuildのための準備
****
- 環境変数に以下を設定します  
    - YOLO_DIR：  
     例）D:\AlexeyAB\darknet
- 5.の手順で作成した yolo_cpp_dll.dll/lib　をプロジェクトの出力フォルダにコピーします  
****
## 7.動作確認環境
****
以下の環境で動作確認しました  
- darknet   (2023/5/31 master)
- CMake 3.23.2  
- CUDA 11.7  
- CUDNN 8.4.1  
- OPenCV 4.7.0  
- Visual Studio 2022  
- 使用PC:
    - Windows 10/64  
    - NVIDIA GeFroce GTX1080  

*end of document.*  


