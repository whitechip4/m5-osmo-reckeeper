#!/bin/bash
set -e

# Setup ESP-IDF environment if not already set
if [ -z "$IDF_PATH" ]; then
    echo "ESP-IDF environment not set, sourcing export.sh..."
    . ~/esp/esp-idf/export.sh
fi

echo "M5StickC Plus2 Build"
idf.py build
echo "Build complete!"
