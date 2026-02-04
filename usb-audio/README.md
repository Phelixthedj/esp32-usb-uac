# ESP32-S3 USB Audio Device

A complete USB Audio Class 2.0 implementation for ESP32-S3 that provides high-quality audio input/output capabilities. This device appears as a standard USB microphone and speaker to any host computer.

## 🎯 Features

- ✅ **USB Audio Class 2.0** - Full compliance with USB audio standards
- ✅ **High-Quality Audio** - 48kHz, 16-bit stereo audio
- ✅ **Plug-and-Play** - Works with Windows, macOS, and Linux
- ✅ **Logarithmic Volume Control** - Natural volume scaling that matches human hearing
- ✅ **Configurable Default Volume** - Set startup volume via menuconfig (no reflash needed)
- ✅ **Amplifier Control** - Proper power sequencing and timing
- ✅ **ESP-IDF v5.5.2** - Latest framework compatibility

## 🚀 Quick Start

### Prerequisites
- ESP32-S3 development board
- PDM microphone module (optional)
- I2S audio amplifier/speaker (optional)
- ESP-IDF v5.5.2 development environment
- USB-C cable

### Hardware Connections

```
ESP32-S3    | PDM Mic    | I2S Amplifier
------------|------------|---------------
GPIO 9      | CLK        | -
GPIO 10     | LR (GND)   | -
GPIO 11     | DATA       | -
GPIO 13     | -          | DOUT
GPIO 14     | -          | BCLK
GPIO 21     | -          | WS/LRC
GPIO 12     | -          | SD_MODE (Enable)
```

### Software Setup

1. **Clone and Build**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/esp32-usb-audio.git
   cd esp32-usb-audio
   idf.py build
   ```

2. **Configure Default Volume** (Optional):
   ```bash
   idf.py menuconfig
   # Navigate to: ESP32 USB Audio Configuration → Default volume factor
   # Set value (10 = -20dB, 5 = -26dB, 1 = -40dB)
   ```

3. **Flash to Device**:
   ```bash
   idf.py flash
   ```

4. **Monitor** (Optional):
   ```bash
   idf.py monitor
   ```

## 🎵 Usage

1. Connect the ESP32-S3 to your computer via USB
2. The device will appear as "ESP32 UAC Speaker" in your audio devices
3. Select it as your default audio output device
4. Adjust volume using your system's volume controls
5. Audio will play through the connected I2S amplifier

## ⚙️ Configuration Options

### Default Volume Factor
- **Location**: `ESP32 USB Audio Configuration → Default volume factor`
- **Range**: 1-100 (represents 0.01-1.0 linear gain)
- **Default**: 10 (-20dB, prevents clipping)
- **Effect**: Sets the initial volume level when device connects

### Audio Parameters
- **Sample Rate**: 48kHz (configurable via sdkconfig)
- **Bit Depth**: 16-bit
- **Channels**: Mono input, Mono output
- **USB Class**: Audio Class 2.0

## 🔧 Technical Details

### Audio Processing Pipeline
1. **USB Audio Reception** → 48kHz 16-bit PCM
2. **Volume Scaling** → Logarithmic gain control
3. **Clipping Prevention** → Hard limiting at ±32k
4. **I2S Transmission** → Standard I2S format
5. **Amplifier Control** → Proper power sequencing

### Volume Control Algorithm
```c
// Logarithmic mapping for perceptual volume control
float db = -40.0f + (volume - 1) * 20.0f / 99.0f;
volume_factor = powf(10.0f, db / 20.0f);
```

### USB Descriptors
- **Device Class**: Composite (MISC)
- **Audio Control**: Interface 0
- **Audio Streaming**: Interface 1
- **Endpoint**: Isochronous OUT (EP 0x01)
- **Feedback**: Isochronous IN (EP 0x81)

## 🐛 Troubleshooting

### Device Not Recognized
- Check USB cable and connections
- Verify ESP32-S3 is properly powered
- Try different USB port/host computer

### No Audio Output
- Confirm I2S connections (DOUT, BCLK, WS)
- Check amplifier power and enable pin
- Verify volume is not muted

### Distortion Issues
- Reduce default volume factor in menuconfig
- Check I2S timing (try toggling WS inversion)
- Ensure clean power supply to ESP32-S3

### Windows Compatibility
- Device uses USB Audio Class 2.0
- Compatible with Windows 10/11
- Appears in Device Manager and Sound settings

## 📋 Requirements

- **ESP-IDF**: v5.5.2
- **Chip**: ESP32-S3
- **Flash**: Minimum 4MB
- **RAM**: ~50KB for audio buffers
- **USB**: USB 2.0 Full Speed

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

## 🙏 Acknowledgments

- Based on Espressif's `usb_device_uac` component
- TinyUSB stack for USB implementation
- ESP-IDF framework and community
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
. ./export.sh

# Build and flash
cd usb-audio
idf.py build
idf.py flash
```

#### Configuration
```bash
# Configure project settings
idf.py menuconfig

# Key settings to configure:
# - USB Audio Class settings
# - Sample rate configuration
# - GPIO pin assignments
# - Power management options
```

### Verification
1. **Connect to computer** via USB-C
2. **Check device recognition** in system audio settings
3. **Test microphone input** in audio applications
4. **Test audio output** with speakers/headphones

## 🔧 Configuration

### USB Audio Settings
```c
// Sample rate configuration
#define CONFIG_UAC_SAMPLE_RATE 48000  // 8kHz to 48kHz

// Audio format settings
#define CONFIG_UAC_BITS_PER_SAMPLE 16
#define CONFIG_UAC_CHANNELS 1  // Mono
```

### GPIO Configuration
```c
// PDM microphone pins
#define MIC_I2S_CLK GPIO_NUM_9   // PDM clock
#define MIC_I2S_LR GPIO_NUM_10   // LR clock (tied to GND)
#define MIC_I2S_DATA GPIO_NUM_11 // PDM data

// I2S output pins
#define SPEAKER_I2S_DOUT GPIO_NUM_13  // Data out
#define SPEAKER_I2S_BCLK GPIO_NUM_14  // Bit clock
#define SPEAKER_I2S_LRC GPIO_NUM_21   // Word select
#define SPEAKER_SD_MODE GPIO_NUM_12   // Amplifier enable
```

### I2S Configuration
```c
// PDM input configuration
i2s_pdm_rx_config_t pdm_cfg = {
    .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(48000),
    .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_16BIT, 
        I2S_SLOT_MODE_MONO
    ),
    .gpio_cfg = {
        .clk = MIC_I2S_CLK,      // GPIO 9
        .din = MIC_I2S_DATA,     // GPIO 11
        .invert_flags = { .clk_inv = false },
    },
};

// I2S output configuration
i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_MONO
    ),
    .gpio_cfg = {
        .bclk = SPEAKER_I2S_BCLK,  // GPIO 14
        .ws = SPEAKER_I2S_LRC,     // GPIO 21
        .dout = SPEAKER_I2S_DOUT,  // GPIO 13
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
```

## 🛠️ Development

### Project Structure
```
usb-audio/
├── main/
│   ├── main.c              # Main application code
│   ├── CMakeLists.txt      # Build configuration
│   └── idf_component.yml   # Component dependencies
├── managed_components/     # ESP-IDF managed components
├── build/                  # Build output directory
├── CMakeLists.txt          # Project build configuration
├── sdkconfig              # Project configuration
└── README.md              # This file
```

### Dependencies
- **ESP-IDF**: Espressif IoT Development Framework
- **TinyUSB**: USB device stack
- **USB Audio Class**: UAC 2.0 implementation
- **I2S Driver**: ESP-IDF I2S interface

### Build Configuration
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(usb-audio)
```

### Key Components
- **USB Device Stack**: TinyUSB implementation
- **Audio Class Driver**: UAC 2.0 driver
- **I2S Driver**: ESP-IDF I2S interface
- **FreeRTOS**: Real-time operating system

## 🐛 Troubleshooting

### Common Issues

1. **Device not recognized**:
   - Check USB cable connection
   - Verify USB port functionality
   - Check device manager/system logs
   - Try different USB port

2. **No audio input**:
   - Check PDM microphone connections
   - Verify GPIO pin assignments
   - Test microphone with multimeter
   - Check power supply (3.3V)

3. **No audio output**:
   - Check I2S amplifier connections
   - Verify amplifier power supply
   - Test with different speakers
   - Check GPIO pin assignments

4. **Poor audio quality**:
   - Check sample rate configuration
   - Verify audio component specifications
   - Test with different components
   - Check power supply stability

5. **Build errors**:
   - Update ESP-IDF to latest version
   - Check component dependencies
   - Verify build configuration
   - Clean and rebuild project

### Debug Features
- **Serial Output**: Debug messages via USB CDC
- **LED Indicators**: Status indication
- **Audio Monitoring**: Real-time audio level monitoring
- **Performance Metrics**: CPU and memory usage

### Debug Commands
```bash
# Monitor serial output
idf.py monitor

# Check device status
idf.py monitor --print-filter="*:INFO"

# Flash with debug info
idf.py build --debug
idf.py flash
```

## 📄 License

This project is part of the microphone audio processing suite. See the main repository README for license information.

## 📚 References

- [USB Audio Class 2.0 Specification](https://www.usb.org/documents)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [TinyUSB Documentation](https://docs.tinyusb.org/)
- [I2S Audio Interface](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)