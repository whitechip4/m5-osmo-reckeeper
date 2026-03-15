# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

M5StickC Plus2 を使用して DJI Osmo360 アクションカメラの録画開始/停止を制御する BLE リモコン。

## Context & Purpose

### 目的

* M5StickC Plus2 のボタン押下で Osmo360 の Rec/Stop を制御
* BLE 経由で録画状態を取得し、LCD に表示（REC/STOP）


### 動作フロー

1. **起動時**: スイッチ押下で Osmo360 と BLE ペアリング開始
2. **ペアリング中**: LCD に進捗と結果を表示
3. **ペアリング完了後**: 録画状態をポーリングし、LCD に "REC" または "STOP" を表示
4. **ボタンA押下**: BLE 経由で録画開始/停止コマンドを送信
5. **ボタンB押下**: Rec KeepモードのON/OFF切り替え
6. **Rec Keep Mode ON時**: 外部要因で録画停止を検知した場合、3秒後に自動録画再開
   * 本デバイスからの停止操作では再開しない
7. **PWRボタン単押し後離す**: デバイスをリセット
8. **PWRボタン3秒長押し**: 電源OFF
   - 押下直後: 案内表示（"Press 3s to PWR OFF"/"Release to reset"）
   - カウントダウン中（3→2→1）に離すとリセット
   - 3秒間押し続けると "BYE" 表示後に電源OFF

## Tech Stack & Architecture

### ハードウェア

* **MCU**: ESP32-PICO-V3-02 (M5StickC Plus2)
- **GPS MODULE**: M5 Unit GPS v1.1 (AT6668)
* **ターゲットデバイス**: /dev/ttyACM0

### 開発環境

* **フレームワーク**: ESP-IDF v5.5.3 (インストール先: `~/esp/esp-idf/`)
* **開発OS**: Ubuntu
* **言語**: C/C++
* **M5Stick用 ライブラリ**: M5Stack 公式ライブラリを使用する事
  * M5Unified: M5StickC Plus2 のハードウェア抽象化。LCDやIMUなど、M5Stickの機能を使う場合はこちらを利用し、独自でドライバを書かないこと。
  * M5GFX: LCD/グラフィック描画ライブラリ　必要であれば使用。M5Unifiedを優先。
* **参考実装**: https://github.com/dji-sdk/Osmo-GPS-Controller-Demo リモコン動作全般
- **参考実装2** https://github.com/m5stack/TinyGPSPlus GPS取得
* **ライブラリバージョン**:
  * M5Unified v0.2.13
  * M5GFX v0.2.19 (M5Unified の依存)

### コードアーキテクチャ

シングルスレッドのイベントループ方式（FreeRTOS タスクはシステムタスクのみ）:

### プロジェクト構成

ESP-IDF 標準プロジェクト構成に従う:
```
main/           - メインソースコード (C/C++)
  main.c         - エントリーポイントとアプリケーション本体 (C)
  m5_wrapper.cpp - M5Unified C++ APIのCラッパー (C++)
  dji_protocol.c - DJIプロトコル高レベルAPI (C)
  ble_simple.c   - BLE通信処理 (C)
  storage.c      - NVSストレージ処理 (C)
  config.h       - ユーザーが調整可能な定数を定義
  CMakeLists.txt - コンポーネント依存設定 (M5Unified を PRIV_REQUIRES)
components/     - 使用ライブラリ (git submodule)
  M5Unified/     - ハードウェア抽象化レイヤー
  M5GFX/        - LCD/グラフィック描画ライブラリ
reference/      - DJI 公式デモ (git submodule、WebSearchが出来ないとき用の参考実装)
tools/          - ビルド・フラッシュ・モニタスクリプト
CMakeLists.txt  - ルートビルド設定 (EXTRA_COMPONENT_DIRS で components/ を指定)
sdkconfig       - ESP-IDF 設定ファイル
```

### M5Unified C++ラッパー

M5UnifiedはC++ライブラリですが、メインコードはCで記述されています。
そのため、`m5_wrapper.cpp`でC++ APIのCラッパー関数を提供しています：

**基本関数**:
- `M5_begin()` - 初期化と画面回転設定（`setRotation(3)`）
- `M5_update()` - ボタン状態更新
- `M5_display_width()`, `M5_display_height()` - 画面サイズ取得
- `M5_display_fillScreen()` - 画面クリア
- `M5_display_setTextColor()` - テキスト色設定
- `M5Display_setTextDatum()` - テキスト基準点設定（中央揃え等）
- `M5_display_setTextSize()` - テキストサイズ設定
- `M5_display_drawString()` - 文字列描画

**グラフィック描画関数**:
- `M5_display_fillRect(x, y, w, h, color)` - 四角形描画（STOPアイコン用）
- `M5_display_fillCircle(x, y, r, color)` - 円描画（RECアイコン用）

**ボタン関数**:
- `M5_BtnA_wasPressed()`, `M5_BtnB_wasPressed()` - ボタン押下検出（立ち上がりエッジ）
- `M5_BtnPWR_wasPressed()`, `M5_BtnPWR_isPressed()` - PWRボタン検出

**電源管理関数**:
- `M5_Power_restart()` - デバイスリセット
- `M5_Power_off()` - 電源OFF

## Development Workflow

### ビルドキャッシュ

ccacheを利用しビルド時間を短縮する。ESP-IDFはデフォルトでccacheをサポートしている。
ビルド時に自動的にccacheが使用される（`~/.espressif/python_env/`配下の設定による）。

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

### 録画時間取得

* **基本**: カメラからのBLE通知（`record_time`フィールド）を使用
* **フォールバック**: カメラからの`record_time`が常に0の場合、内部タイマー（`esp_timer_get_time()`）を使用して録画開始時からの経過時間を計算
  * 録画開始時に`s_recording_start_time`に開始時刻を記録
  * `dji_get_recording_time()`で経過時間を計算して返す（マイクロ秒→秒変換）
  * 録画停止時にタイマーをリセット


### 実装アーキテクチャ

シングルスレッドのイベントループ方式を採用。
FreeRTOS のタスクはシステムタスクのみを使用し、アプリケーション固有のタスクは作成しない。
ボタン処理と BLE イベントは app_main() 内で polling する。

### 永続化

* **Rec Keep Mode状態**: NVS（非揮発ストレージ）に保存
  * 保存タイミング: 録画開始/停止時
  * 復元タイミング: 起動時

### PWRボタン処理

* **検出方法**: `M5_BtnPWR_wasPressed()` で立ち上がりエッジ検出
* **カウントダウン処理**: 3秒間のカウントダウン（3→2→1）を表示
  * 途中でボタンを離す: リセット（`M5_Power_restart()`）
  * 3秒間押し続ける: 電源OFF（`M5_Power_off()`）
* **表示内容**:
  * 押下直後: `"Press 3s to PWR OFF"`（黄色・大）/ `"Release to reset"`（白色・小）
  * カウントダウン中: `"3"`→`"2"`→`"1"`（赤色・大）/ `"Release to reset"`（白色・小）
  * 電源OFF時: `"BYE"`（白色・大）

## UI/UX Design

### LCD表示仕様

* **画面向き**: 左に90度回転（横向き）: `setRotation(3)`
* **優先度**: 他処理に影響しない範囲で更新
* **表示位置**: メイン表示は上下左右中央揃え（`top_center`使用）。入りきらない場合は2行にして 1行目タイトル：2行目サブタイトルのような形でフォントサイズ差をつけ見やすくすること
* **常時表示**: 右上にRec Keepモード状態（ON/OFF）を目視できるサイズで小さめで表示。描画色はONなら赤色、OFFなら緑色とする。
* **グラフィック描画**: フォントでサポートされない文字は図形描画で対応
  * 四角形（STOP）: `M5_display_fillRect(x, y, w, h, color)`
  * 円形（REC）: `M5_display_fillCircle(x, y, r, color)`

#### 状態別表示内容

| 状態 | 表示内容 |
|------|----------|
| READY | `PUSH TO Pairing` |
| ペアリング中 | `Finding Device... (デバイスID)` |
| 録画停止中 | `■STOP`（緑色） / `Press to start` |
| 録画中 | `●REC`（赤色） / 録画時間 |
| 再録画待機中(Rec Keep Mode発動時) | `Restar Recording...` |
| PWRボタン案内 | `Press 3s to PWR OFF`（黄色） / `Release to reset`（白色・小） |
| PWRカウントダウン | `3`→`2`→`1`（赤色・大） / `Release to reset`（白色・小） |
| PWR電源OFF | `BYE`（白色） |

* フォントが無いものについては図形描画で対応する

## 開発手順

デバイスが関わるため、小さく機能を区切って動作確認を行い、開発を進めていく。完了したら完了したことを明記すること。
参考にするライブラリをSubmoduleとしてreferenceフォルダに入れ、ローカルでコードやドキュメントを参照すること。

✅ **完了**: 1. LCDディスプレイに文字を表示して動作確認する事
✅ **完了**: 2. 1.が完了後、BLEの動作確認をすること
✅ **完了**: 3. 2.が完了後、Osmo360との通信確立し、ペアリングが出来ることを確認すること
✅ **完了**: 4. 3.が完了後、再生信号を送り、実際に再生されているかどうかをポーリングで確認する事
✅ **完了**: 5. 4.が完了後、停止信号を送り、停止を確認すること
✅ **完了**: 6. 5.が完了後、Rec Keepモードを追加する事
✅ **完了**: 7. 6.が完了後、PWRボタンの実装を行う事
✅ **完了**: 8. 7.が完了後、LCDを仕様にのっとった実装にする事
   - グラフィック描画によるアイコン表示（四角・円）
   - 録画時間の内部タイマーフォールバック実装
   - 画面中央揃え表示の実装
✅ **完了**: 9. バッテリー表示 左上にバッテリー表示を追加する事。バッテリー表示は%表示とする事
✅ **完了**: 10. GPUのデータを取得する事。データ取得周期はRefelenceのリポジトリを参考にする事。
✅ **完了**: 11. GPSのデータを録画時にOsmo360に送信すること。Protocolや使用方法はRefelenceのリポジトリのドキュメントとサンプルプログラムを参考にすること。
✅ **完了**: 12. Wifi等の使っていないペリフェラルの電源を落として省電力化する事
✅ **完了**: 13. 大きくなったネストを改善すること、ベタ書きを関数化して特にmain内の可読性を向上させること。ネストは基本2段まで、最大でも3段までの許容とする。コンテクストや役割毎にソース分割や関数化を行い、基本的にmainの中では関数の呼び出しだけを行う形にして可読性を向上させること。
✅ **完了**: 14. LCDをダブルバッファ・または部分更新にして、画面のチラツキを減らすこと

### 実装完了状態

すべての開発手順が完了しています。プロジェクトは以下の機能を備えています：

- ✅ BLE接続とDJI Osmo360ペアリング
- ✅ 録画開始/停止制御
- ✅ Rec Keepモード（外部停止時の自動録画再開）
- ✅ PWRボタンによるリセット/電源OFF
- ✅ LCD表示（グラフィック描画によるアイコン、録画時間表示）
- ✅ NVSによる設定永続化
- ✅ GPSデータをOSMO360に送信
