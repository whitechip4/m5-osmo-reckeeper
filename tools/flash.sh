#!/bin/bash
set -e

# Setup ESP-IDF environment if not already set
if [ -z "$IDF_PATH" ]; then
    echo "ESP-IDF environment not set, sourcing export.sh..."
    . ~/esp/esp-idf/export.sh
fi

echo "M5StickC Plus2 Build & Flash"
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyACM0 flash
echo "完了! ./monitor.sh でモニタリングできます"
