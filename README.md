[![USB Audio Build and Test](https://github.com/atomic14/esp32-usb-uac-experiments/actions/workflows/usb-audio.yml/badge.svg)](https://github.com/atomic14/esp32-usb-uac-experiments/actions/workflows/usb-audio.yml)

## 🎯 Overview

This repository contains a complete audio processing ecosystem with both hardware and software components:

- **Protocols**: USB Audio Class (UAC) and Web Serial communication

## 📁 Project Structure

### Hardware Projects

#### [`usb-audio/`](./usb-audio/)
ESP32-S3 implementation of a USB Audio Class (UAC) device that appears as a standard USB microphone to the host computer. Provides both input (microphone) and output (speaker) capabilities.

**Key Features:**
- USB Audio Class 2.0 compliant
- PDM microphone input
- I2S audio output
- Volume and mute controls
- Plug-and-play USB device

## 🚀 Quick Start

### Hardware Setup

- Flash the `usb-audio` firmware to your ESP32-S3
- The device will appear as a standard USB Speaker


## 🔧 Hardware Requirements

### ESP32-S3 Development Board
- ESP32-S3-DevKitC-1 or compatible
- USB-C connection for programming and communication

### Audio Components
- PCM5102 or compatible DAC circuit
- 2 - 150 ohm resistors if you want to use with headphones and not line level output

### Pin Connections
- GPIO 13: I2S Data Out
- GPIO 14: I2S Bit Clock
- GPIO 21: I2S Word Select
- GPIO 12: Amplifier Enable(idk, i don't use this one)

## 🛠️ Development

### Building Firmware

#### USB-Audio (ESP-IDF)
```bash
cd usb-audio
idf.py build
idf.py flash
```

## 🔍 Troubleshooting

### Common Issues

1. **USB connection issues**: Check USB cable and drivers
2. **Audio quality**: Verify all connections and power supply

### Debug Features
- Real-time packet statistics
- CRC error monitoring
- Connection status indicators
- Detailed logging output

## 📄 License

This project is open source. Please check individual project directories for specific license information.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.

## 📚 Documentation

Each project contains detailed documentation in its respective README file:
- [Serial-Mic Documentation](./serial-mic/README.md)
- [USB-Audio Documentation](./usb-audio/README.md)
- [Frontend Documentation](./frontend/README.md)
- [3D Visualizer Documentation](./3d-visualiser/README.md)
