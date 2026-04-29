# DropCeiling Project Context

This repository contains the standalone Arduino library extracted from the Drop Ceiling Controller project. It is intended to continue as the reusable Arduino API for an 8-channel 0-10V controller board based on ESP32 Feather hardware and an I2C DAC.

## Origin

The library was started inside the source project at:

```text
/Users/npmac/Documents/GitHub/Drop-Ceiling-Controller/firmware/DropCeiling
```

It was copied into this repo so the Arduino library can evolve independently while preserving standard Arduino library structure:

```text
DropCeiling/
  library.properties
  keywords.txt
  src/
  examples/
  README.md
  PROJECT_CONTEXT.md
```

The original reference firmware remains in the controller repo at:

```text
/Users/npmac/Documents/GitHub/Drop-Ceiling-Controller/firmware/DropCeilingController/DropCeilingController.ino
```

That sketch is the behavioral reference for Ethernet ArtNet, DAC output, TFT visualization, and ArtPoll discovery.

## Hardware Context

The target controller is an 8-channel ArtNet or local-input to 0-10V converter.

Main hardware:

| Part | Role |
| --- | --- |
| Adafruit ESP32-S3 TFT Feather | Main controller and optional TFT UI |
| Adafruit WIZ5500 Ethernet FeatherWing | Wired Ethernet ArtNet input |
| Adafruit DAC6578 / DACx578 family | 8-channel I2C DAC output |
| Op amp output stage | Scales DAC output to 0-10V |

Default pins and addresses currently match the custom controller board/reference sketch:

| Setting | Default |
| --- | --- |
| I2C SDA | 42 |
| I2C SCL | 41 |
| WIZ5500 CS | 10 |
| DAC I2C address | `0x4C` |
| DAC resolution | 10-bit, max value 1023 |
| Output channels | 8 |

The library should also compile on ESP32 Feather variants without the built-in TFT. Display support is optional and auto-disabled when TFT pin defaults are not available.

## Library Goals

The public API centers on one `DropCeiling` datatype:

```cpp
#include <DropCeiling.h>

DropCeiling ceiling(DropCeilingMode_Ethernet);

void setup() {
  ceiling.setup();
}

void loop() {
  ceiling.read();
}
```

All input modes converge on the same output API:

```cpp
ceiling.voltageWrite(1, 128);   // 0-255 logical value
ceiling.voltageWrite(2, 5.0f);  // 0.0-10.0V logical value
```

Public channel numbers are 1-8. Internally, ArtNet start-channel offsets remain 0-indexed to match DMX packet layout.

## Implemented Modes

| Mode | Status | Notes |
| --- | --- | --- |
| Ethernet ArtNet | Compile-verified | WIZ5500 + `EthernetUDP`; DHCP with static fallback |
| WiFi ArtNet | Compile-verified | ESP32 `WiFiUDP`; same ArtNet parser as Ethernet |
| BLE | Compile-verified | One writable 8-byte characteristic for channels 1-8 |
| Serial CSV | Compile-verified | Newline-terminated CSV, values clamped 0-255 |
| Standalone | Compile-verified | Startup defaults plus direct `voltageWrite()` calls |

Hardware behavior still needs bench validation on the actual controller board.

## ArtNet Compatibility Contract

The ArtNet implementation must remain compatible with the controller tools in the original repo:

```text
/Users/npmac/Documents/GitHub/Drop-Ceiling-Controller/software/controller.py
/Users/npmac/Documents/GitHub/Drop-Ceiling-Controller/software/touchdesigner/td_artnet_script.py
```

Compatibility details:

| Item | Value |
| --- | --- |
| UDP port | 6454 |
| Header | `Art-Net\0` |
| ArtDmx opcode | `0x5000` little-endian |
| ArtPoll opcode | `0x2000` little-endian |
| ArtPollReply opcode | `0x2100` little-endian |
| Protocol version | 14 |
| Default universe | 0 |
| Default start channel | 0, meaning DMX channel 1 |
| ArtPollReply size | 239 bytes |
| Node short name | `DropCeiling` |
| Node long name | `Drop Ceiling 0-10V Controller` |

The current output mapping preserves the reference sketch behavior: input value 0-255 maps to DAC value 0-1023 for the 10-bit DAC6578 variant.

## BLE Contract

BLE is designed for Web Bluetooth clients, including p5.ble.js.

Defaults:

| Item | Value |
| --- | --- |
| Device name | `DropCeiling` |
| Service UUID | `7b4f0001-3f3a-4f6f-9b0d-84e8f64f0f10` |
| Characteristic UUID | `7b4f0002-3f3a-4f6f-9b0d-84e8f64f0f10` |
| Payload | 8 bytes, one byte per channel |

Byte 0 controls output 1. Byte 7 controls output 8.

## Dependencies

Install the ESP32 core and Arduino libraries before compiling examples:

```bash
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli lib install Ethernet "Adafruit GFX Library" "Adafruit ST7735 and ST7789 Library" "Adafruit BusIO"
arduino-cli config set library.enable_unsafe_install true
arduino-cli lib install --git-url https://github.com/adafruit/Adafruit_DACX578.git
```

The Adafruit DACX578 library is installed from GitHub because it may not be available through the normal Library Manager index.

## Compile Verification

From this repo root, compile examples with the parent directory as the Arduino library search path:

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3_tft --libraries .. examples/EthernetArtNet
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3_tft --libraries .. examples/WiFiArtNet
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3_tft --libraries .. examples/BluetoothInput
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3_tft --libraries .. examples/SerialInput
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3_tft --libraries .. examples/Standalone
```

At the time of extraction, all five examples compiled successfully with Arduino CLI 1.4.1 and ESP32 core 3.3.8.

## Control Software Examples

Non-Arduino controller examples live in `extras/control-software/` so the package remains compatible with Arduino Library Manager.

| Input mode | Folder | Notes |
| --- | --- | --- |
| Ethernet ArtNet | `extras/control-software/artnet-ethernet/` | Python/PySide controller and TouchDesigner module copied from the original controller repo |
| WiFi ArtNet | `extras/control-software/wifi-artnet/` | Same ArtNet controller software as Ethernet, used over a WiFi network |
| BLE | `extras/control-software/bluetooth-p5/` | p5.js sketch using p5.ble and the 8-byte BLE characteristic |
| Serial CSV | `extras/control-software/serial-p5-webserial/` | p5.js sketch using a local Web Serial helper to send 8 CSV values |
| Standalone | `extras/control-software/standalone/` | No external software required; notes for direct firmware control |

## Next Work

Recommended next steps:

1. Upload `examples/EthernetArtNet` to the ESP32-S3 TFT Feather with the WIZ5500 and DAC attached.
2. Confirm the Python controller from the original repo discovers the device via ArtPoll.
3. Confirm TouchDesigner ArtNet sends still update all eight outputs.
4. Measure DAC output for logical values 0, 128, and 255 before the op amp stage.
5. Confirm the TFT startup screen, IP display, live signal state, and 8-channel bars match the reference sketch.
6. Test WiFi ArtNet on the same network as the controller software.
7. Test BLE writes from a Web Bluetooth or p5.ble.js page using an 8-byte `Uint8Array`.
8. Test Serial CSV from Arduino Serial Monitor, Python, TouchDesigner, and WebSerial.
9. Decide whether to replace the original monolithic controller sketch with a wrapper around this library once hardware behavior is confirmed.

## Known Caveats

- This extraction is compile-verified, not hardware-verified.
- Display auto-detection is compile-time and conservative. On non-TFT ESP32 boards, explicitly set `ceiling.config().displayMode = DropCeilingDisplay_Disabled` if needed.
- BLE accepts payloads with at least 8 bytes and uses the first 8 bytes. Short writes are ignored.
- Serial input defaults to partial updates, so a short CSV line updates only the channels provided.
- The output channel count is fixed at 8 in this version.
- CircuitPython support is intentionally out of scope for this Arduino library repo.