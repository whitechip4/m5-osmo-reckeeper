#!/bin/bash
set -e

# Setup ESP-IDF environment if not already set
if [ -z "$IDF_PATH" ]; then
    echo "ESP-IDF environment not set, sourcing export.sh..."
    . ~/esp/esp-idf/export.sh
fi

# Enable CCache for faster builds
export IDF_CCACHE_ENABLE=1
export CCACHE_COMPILERCHECK=content
export USE_CCACHE=1

echo "M5StickC Plus2 Build (with CCache)"
idf.py build
echo "Build complete!"
