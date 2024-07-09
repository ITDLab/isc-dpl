このサンプルは、DPL LibraryをPythonで使うための例です。
DPL Libraryの呼び出しには、ctypesを使用しています。


1.作業用フォルダを作成
例）
> mkdir dpl_test

２．仮想環境の作成（オプション）
例）
dpl_test>python -m venv .venv

作成した仮想環境 .venv ディレクトリにある Scripts\activate.bat を実行します
例）
dpl_test>.venv\Scripts\activate.bat
(.venv) dpl_test>

３．パッケージのインストール
環境に合わせて必要なパッケージをインストールします
例）
(.venv) dpl_test>python -m pip install --upgrade pip
(.venv) dpl_test>python -m pip install numpy
(.venv) dpl_test>python -m pip install opencv-python
(.venv) dpl_test>python -m pip install opencv-contrib-python

4.サンプルコードをコピー
サンプルコードをコピーします
例）
(.venv) dpl_test>　ここに　dpl_test_main.py　を配置します
(.venv) dpl_test>　isc_dpl　フォルダをコピーします

下記の配置となります
(.venv) dpl_test>
		|- dpl_test_main.py
		|
		|-isc_dpl
			|- __init__.py
			|- isc_dpl_if.py
			|- isc_dpl_utility_if.py

5.DLL及び設定ファイルをコピー
例）
(.venv) dpl_test>　ここに　DLL及び設定ファイルをコピーします

注意：
SDKの提供するファイルもコピーしてください
ファイルは、以下です。

　VM:ISCLibvm.dll　ISCSDKLibvm200.dll
　XC:ISCLibxc.dll　ISCSDKLibxc.dll

６．実行
例）
(.venv) dpl_test>python dpl_test_main.py 1 1 1

usage：
dpl_test_main.py 「カメラタイプ」 「視差取得選択」 「表示内容選択」

カメラタイプ：　０=VM　1=XC
視差取得選択：　０=カメラ出力の視差　１=ソフトウェアマッチング出力の視差(表示内容選択は強制的に１となります)
表示内容選択：　０=カメラデータ　1=データ処理結果

例２）
DPC_gui　の保存機能によって保存されたファイルを再生します

(.venv) dpl_test>python dpl_test_main_file_if.py C:\isc-dpl\data\20240709_141700.dat　0

usage：
dpl_test_main_file_if.py 「ファイル名」 「表示内容選択」

ファイル名：再生データ　ファイルフルパス
表示内容選択：　０=カメラデータ　1=データ処理結果

[LICENSE]
This software is licensed under the Apache 2.0 LICENSE.

