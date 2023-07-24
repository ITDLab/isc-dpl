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

６．実行
例）
(.venv) dpl_test>python dpl_test_main.py 1 1

usage：
dpl_test_main.py 「カメラタイプ」 「表示内容選択」

カメラタイプ：　０=VM　1=XC
表示内容選択：　０=カメラデータ　1=データ処理結果

[LICENSE]
This software is licensed under the Apache 2.0 LICENSE.

