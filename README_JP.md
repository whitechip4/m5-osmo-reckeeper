# Osmo RecKeeper

M5StickC Plus2 を使用して DJI Osmo360 アクションカメラの録画開始/停止を制御する BLE リモコン。

## 概要

M5StickC Plus2 から DJI Osmo360 アクションカメラをREC/STOPするリモコン用ファームウェア。
外力により録画が停止した場合、自動で再録画を叩く機能がメイン。
BLEOsmo360の録画が自動で止まってしまう症状があったため、それの対応用。

### 主な機能

- **録画制御**: 録画開始/停止を切り替え
- **Rec Keep モード**: 外部要因で録画が停止した場合、自動録画再開

## 使用デバイス

- **M5StickC Plus2** - ESP32-PICO-V3-02 搭載マイコンモジュール
- **M5 Unit GPS v1.1 (AT6668)**: GPS Module(optional for dashboard)

## 必要環境

### ソフトウェア

- **ESP-IDF v5.5.3** - Espressif の公式フレームワーク
- **M5Unified** - M5stick制御用

### 推奨開発環境

- **OS**: Ubuntu 22.04 LTS 以降
- **ターゲットデバイス**: `/dev/ttyACM0`

## プロジェクト構成

```
osmo-reckeeper/
├── main/                    # メインソースコード (C/C++)
├── components/             # M5Stack 公式ライブラリ (git submodule)
│   ├── M5Unified/          # ハードウェア抽象化レイヤー (v0.2.13)
│   └── M5GFX/             # LCD/グラフィック描画ライブラリ (v0.2.19)
├── reference/             # DJI 公式デモ (git submodule)
│   └── OsmoDemo/          # 参考実装 エージェントで読み込む用
├── tools/                 # ビルド・フラッシュ・モニタスクリプト
│   ├── build.sh          # ビルドのみ
│   ├── flash.sh          # ビルドとフラッシュ
│   └── monitor.sh        # シリアルモニタ
├── CMakeLists.txt        # ルートビルド設定
└── sdkconfig             # ESP-IDF 設定ファイル
```

## セットアップ手順

### 1. ESP-IDF 環境構築

ESP-IDF v5.5.3 をインストールしていない場合は、公式ドキュメントに従ってセットアップしてください：

```bash
# ESP-IDF のダウンロードとインストール
git clone --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf
git checkout v5.5.3
./install.sh esp32
```

### 2. リポジトリのクローンとサブモジュール初期化

```bash
# リポジトリをクローン
git clone <repository-url>
cd osmo-reckeeper

# サブモジュールを初期化して更新
git submodule update --init --recursive
```

### 3. ESP-IDF 環境変数の設定

```bash
# ESP-IDF 環境変数を有効化
. ~/esp/esp-idf/export.sh
```

## ビルドと書き込み

### ビルドのみ

```bash
./tools/build.sh
```

### ビルドとフラッシュ

```bash
./tools/flash.sh
```

### シリアルモニタ

```bash
./tools/monitor.sh
```

終了するには `Ctrl+]` を押してください。

### 手動ビルド（ESP-IDF コマンド）

```bash
# ターゲット設定
idf.py set-target esp32

# ビルド
idf.py build

# フラッシュ
idf.py -p /dev/ttyACM0 flash

# モニタ
idf.py -p /dev/ttyACM0 monitor

# ビルド+フラッシュ+モニタ（一括実行）
idf.py -p /dev/ttyACM0 flash monitor
```

## 使用方法

### ボタン操作

| ボタン | 操作 | 機能 |
|--------|------|------|
| **Btn A** | 単押し | 録画開始/停止の切り替え |
| **Btn B** | 単押し | Rec Keep モードの ON/OFF 切り替え |
| **PWR** | 単押し後離す | デバイスをリセット |
| **PWR** | 3秒長押し | 電源 OFF（カウントダウン中に離すとリセット） |

### Rec Keep モード

Rec Keep モードが ON の時、外部要因（SDカードエラー等）で録画が停止した場合、自動的に録画を再開します。本デバイスからの停止操作では再開しません。

### LCD 表示

- **READY**: `PUSH TO Pairing` - ペアリング待機中
- **ペアリング中**: `Finding Device... (デバイスID)`
- **録画停止中**: `■STOP`（緑色）/ `Press to start`
- **録画中**: `●REC`（赤色）/ 録画時間（秒）
- **Rec Keep 状態**: 右上に `RK: ON`（赤色）/ `RK: OFF`（緑色）を常時表示
- **PWR ボタン案内**: `Press 3s to PWR OFF`（黄色）/ `Release to reset`（白色）
- **PWR カウントダウン**: `3`→`2`→`1`（赤色）/ `Release to reset`（白色）
- **電源 OFF**: `BYE`（白色）

## 参考資料

- [DJI Osmo GPS Controller Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo) - DJI 公式参考実装（`reference/OsmoDemo/` にサブモジュールとして含まれています）
- [M5Unified](https://github.com/m5stack/M5Unified) - M5Stack 公式ハードウェア抽象化レイヤー
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/) - Espressif 公式ドキュメント

## ライセンス

**MIT License**

本プロジェクトは独自実装ですが、DJI プロトコルの使用については DJI の EULA に準拠してください。

### 依存ライブラリのライセンス

- **M5Unified**: MIT License
- **M5GFX**: MIT License
- **ESP-IDF**: Apache 2.0 License

## Development Tools

* Claude Code
* GLM4.7
