# Osmo RecKeeper

BLE remote control for starting/stopping recording on DJI Osmo360 action camera using M5StickC Plus2.

## Overview

Firmware for M5StickC Plus2 to remotely control REC/STOP on DJI Osmo360 action camera. The main feature is automatically restarting recording when it stops due to external factors. This was developed to address an issue where the Osmo360's recording would stop automatically.

### Key Features

- **Recording Control**: Toggle recording start/stop
- **Rec Keep Mode**: Automatically resume recording when stopped by external factors

## Hardware Requirements

- **M5StickC Plus2** - Microcontroller module with ESP32-PICO-V3-02
- **M5 Unit GPS v1.1 (AT6668)**: GPS Module(optional for dashboard)

## Software Requirements

- **ESP-IDF v5.5.3** - Official Espressif framework
- **M5Unified** - M5Stick control library

### Recommended Development Environment

- **OS**: Ubuntu 22.04 LTS or later
- **Target Device**: `/dev/ttyACM0`

## Project Structure

```
osmo-reckeeper/
├── main/                    # Main source code (C/C++)
├── components/              # M5Stack official libraries (git submodule)
│   ├── M5Unified/           # Hardware abstraction layer (v0.2.13)
│   └── M5GFX/              # LCD/graphics drawing library (v0.2.19)
├── reference/              # DJI official demo (git submodule)
│   └── OsmoDemo/           # Reference implementation for agent analysis
├── tools/                  # Build, flash, and monitor scripts
│   ├── build.sh           # Build only
│   ├── flash.sh           # Build and flash
│   └── monitor.sh         # Serial monitor
├── CMakeLists.txt         # Root build configuration
└── sdkconfig              # ESP-IDF configuration file
```

## Setup Instructions

### 1. ESP-IDF Environment Setup

If you haven't installed ESP-IDF v5.5.3, follow the official documentation to set it up:

```bash
# Download and install ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf
git checkout v5.5.3
./install.sh esp32
```

### 2. Clone Repository and Initialize Submodules

```bash
# Clone repository
git clone <repository-url>
cd osmo-reckeeper

# Initialize and update submodules
git submodule update --init --recursive
```

### 3. Set ESP-IDF Environment Variables

```bash
# Enable ESP-IDF environment variables
. ~/esp/esp-idf/export.sh
```

## Build & Flash

### Build Only

```bash
./tools/build.sh
```

### Build and Flash

```bash
./tools/flash.sh
```

### Serial Monitor

```bash
./tools/monitor.sh
```

Press `Ctrl+]` to exit.

### Manual Build (ESP-IDF Commands)

```bash
# Set target
idf.py set-target esp32

# Build
idf.py build

# Flash
idf.py -p /dev/ttyACM0 flash

# Monitor
idf.py -p /dev/ttyACM0 monitor

# Build+flash+monitor (all at once)
idf.py -p /dev/ttyACM0 flash monitor
```

## Usage

### Button Operations

| Button | Operation | Function |
|--------|-----------|----------|
| **Btn A** | Single press | Toggle recording start/stop |
| **Btn B** | Single press | Toggle Rec Keep mode ON/OFF |
| **PWR** | Press and release | Reset device |
| **PWR** | Long press (3s) | Power OFF (release during countdown to reset) |

### Rec Keep Mode

When Rec Keep Mode is ON, recording will automatically resume if it stops due to external factors (e.g., SD card error). Recording will NOT resume when stopped manually from this device.

### LCD Display

- **READY**: `PUSH TO Pairing` - Waiting for pairing
- **Pairing**: `Finding Device... (device ID)`
- **Recording Stopped**: `■STOP` (green) / `Press to start`
- **Recording**: `●REC` (red) / Recording time (seconds)
- **Rec Keep Status**: `RK: ON` (red) / `RK: OFF` (green) displayed in top-right corner
- **PWR Button Guide**: `Press 3s to PWR OFF` (yellow) / `Release to reset` (white)
- **PWR Countdown**: `3`→`2`→`1` (red) / `Release to reset` (white)
- **Power OFF**: `BYE` (white)

## References

- [DJI Osmo GPS Controller Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo) - DJI official reference implementation (included as submodule in `reference/OsmoDemo/`)
- [M5Unified](https://github.com/m5stack/M5Unified) - M5Stack official hardware abstraction layer
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/) - Official Espressif documentation

## License

**MIT License**

This project is an original implementation. Please comply with DJI's EULA regarding the use of DJI protocols.

### Dependency Library Licenses

- **M5Unified**: MIT License
- **M5GFX**: MIT License
- **ESP-IDF**: Apache 2.0 License

## Development Tools

- Claude Code
- GLM4.7
