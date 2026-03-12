# design-rough.md

人間のデザインをここに書く。これをClaudeに解釈させCLAUDE.mdの基にする

# 仕様

## やること
* M5StickC Plus2 を用いて、DJI Osmo360のRec/Stopをする
* ディスプレイには、REC/Stopの状態を表示する。この状態はOsmoからBLEで取得する

## 機材
* M5StickC Plus2を使用
  * MCUはESP32-PICO-V3-02

## 開発環境
* esp-idfを使用
* M5Stickの機能の使用には、M5Unified、M5GFXなどM5のライブラリを使用すること。独自ライブラリを作成しないこと。
* Ubuntu 24.04上で開発する
* https://github.com/dji-sdk/Osmo-GPS-Controller-Demo を参照、またはSubmoduleとして使用する

## 動作
* 起動後、スイッチを押すとOsmo360とのペアリング動作をすること。LCDに進行状態とペアリング成功可否を表示すること。
* ペアリング成功後、録画状態を取得し続け、LCD画面に文字で表示する事。 RECまたはSTOPと表示する事。
* ペアリング成功後、ボタンを押された場合にBLE経由で録画開始/録画停止の指令をOsmo360に送る事

## ビルドとフラッシュ
* ターゲットは/dev/ttyACM0
* ビルドコマンドやフラッシュ方法は別のmarkdownに記述し、ユーザーが実行できるようにすること
* ビルドしてデータのフラッシュまでを行う flash.shを作成する事
* デバッグ用のシリアルをモニタリングするmonitor.shを作成する事

## プロジェクト構成
* ESP-IDF 標準プロジェクト構成に従う
  - `main/` - メインソースコードディレクトリ
  - `CMakeLists.txt` - ビルド設定ファイル
  - `sdkconfig` - ESP-IDF 設定ファイル

## その他
* 対話には全て日本語を使う事
* ソースコードへのコメントは英語にすること
* CLAUDE.mdの記述には日本語を用いる事