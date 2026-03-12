#!/bin/bash
set -e

if [ -z "$IDF_PATH" ]; then
    echo "エラー: ESP-IDF環境が設定されていません"
    echo "実行: . ~/esp/esp-idf/export.sh"
    exit 1
fi

echo "M5StickC Plus2 Build & Flash"
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyACM0 flash
echo "完了! ./monitor.sh でモニタリングできます"
