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

* **フレームワーク**: ESP-IDF
* **OS**: Ubuntu 24.04
* **言語**: C/C++
* **M5Stack ライブラリ**: M5Unified、M5GFX（M5Stack 公式ライブラリを使用。独自ライブラリは作成しない）
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
