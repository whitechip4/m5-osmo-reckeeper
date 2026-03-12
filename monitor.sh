#!/bin/bash

if [ -z "$IDF_PATH" ]; then
    echo "エラー: ESP-IDF環境が設定されていません"
    echo "実行: . ~/esp/esp-idf/export.sh"
    exit 1
fi

echo "M5StickC Plus2 Serial Monitor (終了: Ctrl+])"
idf.py -p /dev/ttyACM0 monitor
