# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

M5StickC Plus2 を使用して DJI Osmo360 アクションカメラの録画開始/停止を制御する BLE リモコン。

## Context & Purpose

### 目的

* M5StickC Plus2 のボタン押下で Osmo360 の Rec/Stop を制御
* BLE 経由で録画状態を取得し、LCD に表示（REC/STOP）
* 初回起動時に BLE ペアリングを実行

### 動作フロー

1. **起動時**: スイッチ押下で Osmo360 と BLE ペアリング開始
2. **ペアリング中**: LCD に進捗と結果を表示
3. **ペアリング完了後**: 録画状態をポーリングし、LCD に "REC" または "STOP" を表示
4. **ボタン押下**: BLE 経由で録画開始/停止コマンドを送信

## Tech Stack & Architecture

### ハードウェア

* **MCU**: ESP32-PICO-V3-02 (M5StickC Plus2)
* **ターゲットデバイス**: /dev/ttyACM0

### 開発環境

* **フレームワーク**: ESP-IDF v5.5.3 (インストール先: `~/esp/esp-idf/`)
* **開発OS**: Ubuntu
* **言語**: C/C++
* **M5Stick用 ライブラリ**: M5Stack 公式ライブラリを使用する事
  * M5Unified: M5StickC Plus2 のハードウェア抽象化。LCDやIMUなど、M5Stickの機能を使う場合はこちらを利用し、独自でドライバを書かないこと。
  * M5GFX: LCD/グラフィック描画ライブラリ　必要であれば使用。M5Unifiedを優先。
* **参考実装**: https://github.com/dji-sdk/Osmo-GPS-Controller-Demo (Submodule として使用可能)
* **ライブラリバージョン**:
  * M5Unified v0.2.13
  * M5GFX v0.2.19 (M5Unified の依存)

### コードアーキテクチャ

シングルスレッドのイベントループ方式（FreeRTOS タスクはシステムタスクのみ）:

### プロジェクト構成

ESP-IDF 標準プロジェクト構成に従う:
```
main/           - メインソースコード (C++)
  main.cpp       - エントリーポイント
  CMakeLists.txt - コンポーネント依存設定 (M5Unified を PRIV_REQUIRES)
components/     - M5Stack 公式ライブラリ (git submodule)
  M5Unified/     - ハードウェア抽象化レイヤー
  M5GFX/        - LCD/グラフィック描画ライブラリ
reference/      - DJI 公式デモ (git submodule、参考実装)
tools/          - ビルド・フラッシュ・モニタスクリプト
CMakeLists.txt  - ルートビルド設定 (EXTRA_COMPONENT_DIRS で components/ を指定)
sdkconfig       - ESP-IDF 設定ファイル
```

## Development Workflow

### ビルド・デプロイスクリプト

`tools/` ディレクトリ以下のスクリプトを使用（ESP-IDF 環境は自動でソースされます）:

* **tools/build.sh** - ビルドのみ
* **tools/flash.sh** - ビルドとフラッシュ
* **tools/monitor.sh** - シリアルモニタ起動 (終了: Ctrl+])

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

## Conventions & Standards

### 言語とコミュニケーション

* **対話**: 日本語を使用
* **ソースコードコメント**: 英語で記述
* **CLAUDE.md**: 日本語で記述

### コーディング規約

* 各種ソースはGoogleスタイルガイドに従う事
* コード変更後は変更したコードにclang-formatを使用し、フォーマットが正しくされた状態で成果物を出すこと

### Git ルール

* 取捨選択は人間が行うため、自動でコミット、プッシュしないこと

## Development Guidelines

### ソースコード方針

* ソースコードはシンプルに、指示された事以外の無駄な機能は追加しないこと

### 変更時のルール

* 各種変更を加える度にBuildが通るかを確認する事。
* ビルドメッセージの確認でトークンを消費しすぎないように適宜grepやtailを使って行数を100行程度に絞って読むこと。
* 影響範囲を確認し、不要になったコード等があった場合は適宜削除する事
* ビルド失敗の際は場当たり的対処をせず、周辺ソースコードまで読み込み、変更による副作用が少ない修正を行う事

### 依存管理

* **Submodule**: `reference/OsmoDemo` - 中身を書き換えないこと
* **components/M5Unified**, **components/M5GFX** - git submodule で管理される M5Stack 公式ライブラリ
  * バージョンを固定して運用すること (.gitmodules 参照)
  * 中身を直接書き換えないこと

### 機密情報管理

* Wifi情報など機密に関わるものなどは.envなどに格納し、.gitignoreに適宜追加すること

### Arduino IDE は使用不可
 
BLE ライブラリの互換性問題により、Arduino IDE では DJI R SDK プロトコルが動作しない。
必ず ESP-IDF を使用すること。
 
### BLE ライブラリ

DJI 公式デモの BLE 層を参考に実装する。

### DJI R SDK プロトコル

BLE GATT 経由でカメラと通信する独自プロトコル。
フレーム構造・CRC計算・コマンドID等の詳細は
`reference/OsmoDemo/docs/protocol.md` を参照。
`reference/OsmoDemo/`のソースコードで使用できるモジュールはそのままインポートして使い、無理に独自実装しない。


### 実装アーキテクチャ

シングルスレッドのイベントループ方式を採用。
FreeRTOS のタスクはシステムタスクのみを使用し、アプリケーション固有のタスクは作成しない。
ボタン処理と BLE イベントは app_main() 内で polling する。

## 開発手順

デバイスが関わるため、小さく機能を区切って動作確認を行い、開発を進めていく。リファクタリングは最後に行う事。

 1. LCDディスプレイに文字を表示して動作確認する事
 2. 1.が完了後、BLEの動作確認をすること
 3. 2.が完了後、Osmo360との通信確立し、ペアリングが出来ることを確認すること
 4. 3.が完了後、再生信号を送り、実際に再生されているかどうかをポーリングで確認する事
 5. 4.が完了後、停止信号を送り、停止を確認すること