# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# プロジェクト概要 (Project Overview)

M5StickC Plus2 を使用して DJI Osmo360 アクションカメラの録画開始/停止を制御する BLE リモコン。

## 目的

* M5StickC Plus2 のボタン押下で Osmo360 の Rec/Stop を制御
* BLE 経由で録画状態を取得し、LCD に表示（REC/STOP）
* 初回起動時に BLE ペアリングを実行

## ハードウェア

* **MCU**: ESP32-PICO-V3-02 (M5StickC Plus2)
* **ターゲットデバイス**: /dev/ttyACM0

## 開発環境

* **フレームワーク**: ESP-IDF v5.4.2
* **開発OS**: Ubuntu
* **言語**: C/C++
* **M5Stick用 ライブラリ**: M5Stack 公式ライブラリを使用する事
* **参考実装**: https://github.com/dji-sdk/Osmo-GPS-Controller-Demo (Submodule として使用可能)

## プロジェクト構成

ESP-IDF 標準プロジェクト構成に従う:
```
main/           - メインソースコード
CMakeLists.txt  - ビルド設定
sdkconfig       - ESP-IDF 設定ファイル
```

## ビルドとデプロイ

### 作成するスクリプト

* **build.sh** - ビルドのみを行う
* **flash.sh** - ビルドとフラッシュを実行
* **monitor.sh** - デバッグ用シリアルモニタ起動

### ESP-IDF コマンド (参考)

```bash
# ビルド
idf.py build

# フラッシュ
idf.py -p /dev/ttyACM0 flash

# モニタ
idf.py -p /dev/ttyACM0 monitor

# ビルド+フラッシュ+モニタ
idf.py -p /dev/ttyACM0 flash monitor
```

## 動作フロー

1. **起動時**: スイッチ押下で Osmo360 と BLE ペアリング開始
2. **ペアリング中**: LCD に進捗と結果を表示
3. **ペアリング完了後**: 録画状態をポーリングし、LCD に "REC" または "STOP" を表示
4. **ボタン押下**: BLE 経由で録画開始/停止コマンドを送信

## 言語とコミュニケーション

* **対話**: 日本語を使用
* **ソースコードコメント**: 英語で記述
* **CLAUDE.md**: 日本語で記述
* **git** 取捨選択は人間が行うため、自動でコミット、プッシュしないこと。

## その他
* 各種ソースはGoogleスタイルガイドに従う事
* ソースコードはシンプルに、指示された事以外の無駄な機能は追加しないこと
* 各種変更を加える度にBuildが通るかを確認する事
  * また、影響範囲を確認し、不要になったコード等があった場合は適宜削除する事
* Submoduleで使っているものなどの中身を書き換えないこと
* Submoduleやライブラリをインストールする際はバージョンを固定すること。最新の安定板を選択すること。
* Wifi情報など機密に関わるものなどは.envなどに格納し、.gitignoreに適宜追加すること
* ビルド失敗の際は場当たり的対処をせず、周辺ソースコードまで読み込み、変更による副作用が少ない修正を行う事。
* コード変更後は変更したコードにclang-formatを使用し、フォーマットが正しくされた状態で成果物を出すこと
