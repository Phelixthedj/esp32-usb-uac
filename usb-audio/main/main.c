/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "usb_device_uac.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
#include <math.h>
#include "tusb.h"
#include "uac_descriptors.h"

// Define missing macros
#define AUDIO_PROTOCOL_2_0 0x20
#define ITF_NUM_TOTAL 2  // Control + Streaming interface


#define SPEAKER_I2S_DOUT  13
#define SPEAKER_I2S_BCLK  14
#define SPEAKER_I2S_LRC   21
#define SPEAKER_SD_MODE   12

#define MIC_I2S_CLK  9
#define MIC_I2S_LR   10
#define MIC_I2S_DATA 11

static i2s_chan_handle_t rx;
static i2s_chan_handle_t tx;

static bool is_muted = false;
// volume scaling factor (0.01 to 1.0 for -40dB to 0dB range)
static float volume_factor = CONFIG_DEFAULT_VOLUME_FACTOR_INT / 100.0f;  // Configurable default volume

static esp_err_t usb_uac_device_output_cb(uint8_t *buf, size_t len, void *arg)
{
    if (!tx) {
        return ESP_FAIL;
    }
    int16_t *samples = (int16_t *)buf;
    for (size_t i = 0; i < len / 2; i++) {
        if (is_muted) {
            samples[i] = 0;
            continue;
        }
        int32_t sample = samples[i];
        // Apply volume scaling (volume_factor is 0.01 to 1.0)
        sample = (int32_t)(sample * volume_factor);
        // Ensure no overflow
        if (sample > 32767) {
            sample = 32767;
        } else if (sample < -32768) {
            sample = -32768;
        }
        samples[i] = (int16_t)sample;
    }
    size_t total_bytes_written = 0;
    while (total_bytes_written < len) {
        size_t bytes_written = 0;
        i2s_channel_write(tx, (uint8_t *)buf + total_bytes_written, len - total_bytes_written, &bytes_written, portMAX_DELAY);
        total_bytes_written += bytes_written;
    }
    return ESP_OK;
}

static esp_err_t usb_uac_device_input_cb(uint8_t *buf, size_t len, size_t *bytes_read, void *arg)
{
    if (!rx) {
        return ESP_FAIL;
    }
    return i2s_channel_read(rx, buf, len, bytes_read, portMAX_DELAY);
}

static void usb_uac_device_set_mute_cb(uint32_t mute, void *arg)
{
    is_muted = mute;
}
static void usb_uac_device_set_volume_cb(uint32_t _volume, void *arg)
{
    // _volume ranges from 0 to 100 (Windows volume control)
    // Map to logarithmic volume curve for better perceptual scaling
    if (_volume == 0) {
        volume_factor = 0.001f;  // Very low volume (~ -60dB)
    } else {
        // Logarithmic mapping: _volume 1-100 maps to ~ -40dB to 0dB
        // Using: factor = 10^((volume-1) * (0 - (-40)) / (100-1) / 20)
        // This gives smooth logarithmic volume control
        float db = -40.0f + (_volume - 1) * (0.0f - (-40.0f)) / 99.0f;
        volume_factor = powf(10.0f, db / 20.0f);
    }
}

static void usb_uac_device_init(void)
{
    uac_device_config_t config = {
        .output_cb = usb_uac_device_output_cb,
        .input_cb = usb_uac_device_input_cb,
        .set_mute_cb = usb_uac_device_set_mute_cb,
        .set_volume_cb = usb_uac_device_set_volume_cb,
        .cb_ctx = NULL,
        .spk_itf_num = 1,  // Audio streaming interface
        .mic_itf_num = -1, // No microphone
    };
    /* Init UAC device, UAC related configurations can be set by the menuconfig */
    ESP_ERROR_CHECK(uac_device_init(&config));
}

// USB Device Descriptor
static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A, // Espressif VID
    .idProduct          = 0x4000,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// Configuration Descriptor
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_AUDIO_DEVICE_DESC_LEN)

static uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Audio descriptor for speaker
    TUD_AUDIO_SPEAK_DESCRIPTOR(0, 4, 0x01, 0x81)
};

// String descriptors
static char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
    "Espressif",                   // 1: Manufacturer
    "ESP32 UAC Speaker",           // 2: Product
    "123456",                      // 3: Serials
    "UAC Speaker Control",         // 4: Audio Control Interface
    "UAC Speaker Streaming",       // 5: Audio Streaming Interface
};

static uint16_t _desc_str[32];

uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations
    return desc_configuration;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    uint8_t chr_count;

    if ( index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 2;
    }
    else
    {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

        const char* str = string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);
        if ( chr_count > 31 ) chr_count = 31;

        // Convert ASCII string into UTF-16
        for(uint8_t i=0; i<chr_count; i++)
        {
            _desc_str[1+i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

    return _desc_str;
}

void init_pdm_rx(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, NULL, &rx);

    i2s_pdm_rx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(CONFIG_UAC_SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = MIC_I2S_CLK,      // PDM clock
            // QUESTION - what about the LR clock pin? No longer relevant? Do we ties it high or low?
            .din = MIC_I2S_DATA,     // PDM data
            .invert_flags = { .clk_inv = false },
        },
    };
    pdm_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO; // single mic

    i2s_channel_init_pdm_rx_mode(rx, &pdm_cfg);
    i2s_channel_enable(rx);
}

static void init_pcm_tx(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(CONFIG_UAC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                    I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,   // set this if your amp needs MCLK
            .bclk = SPEAKER_I2S_BCLK,
            .ws   = SPEAKER_I2S_LRC,
            .dout = SPEAKER_I2S_DOUT,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = true,      // WS inversion for amplifier compatibility
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx));
}

void app_main(void)
{
    init_pdm_rx();
    init_pcm_tx();
    
    // Wait for I2S to stabilize before enabling amplifier
    vTaskDelay(pdMS_TO_TICKS(100));
    
    usb_uac_device_init();

    // enable the amplifier with proper initialization
    gpio_reset_pin(SPEAKER_SD_MODE);
    gpio_set_direction(SPEAKER_SD_MODE, GPIO_MODE_OUTPUT);
    gpio_set_level(SPEAKER_SD_MODE, 0);  // Start disabled
    vTaskDelay(pdMS_TO_TICKS(50));       // Wait for discharge
    gpio_set_level(SPEAKER_SD_MODE, 1);  // Enable
    vTaskDelay(pdMS_TO_TICKS(200));      // Wait for startup

    // tie the mic LR clock to GND
    gpio_reset_pin(MIC_I2S_LR);
    gpio_set_direction(MIC_I2S_LR, GPIO_MODE_OUTPUT);
    gpio_set_level(MIC_I2S_LR, 0);

    // Nothing to do here - the USB audio device will take care of everything
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
