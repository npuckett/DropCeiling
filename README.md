# DropCeiling Arduino Library

DropCeiling is an Arduino library for the 8-channel Drop Ceiling 0-10V controller. It allows control via Art Net over Ethernet or WiFi, Serial, BLE, or as a standalone controller.



## Supported Modes

| Mode | Constructor value | Input format |
| --- | --- | --- |
| Ethernet ArtNet | `DropCeilingMode_Ethernet` | ArtDmx over WIZ5500 UDP port 6454 |
| WiFi ArtNet | `DropCeilingMode_WiFi` | ArtDmx over ESP32 WiFi UDP port 6454 |
| BLE | `DropCeilingMode_Bluetooth` | One writable 8-byte BLE characteristic |
| Serial | `DropCeilingMode_Serial` | Newline-terminated CSV values |
| Standalone | `DropCeilingMode_Standalone` | Config defaults and direct API writes |

Public channel numbers are 1-8. ArtNet start-channel configuration remains 0-indexed to match DMX packet offsets.

## Basic Use

```cpp
#include <DropCeiling.h>

DropCeiling ceiling(DropCeilingMode_Ethernet);

void setup() {
  Serial.begin(115200);
  ceiling.setup();
}

void loop() {
  ceiling.read();
}
```

Manual output writes use the same output path as all input modes:

```cpp
ceiling.voltageWrite(1, 128);   // channel 1, logical 0-255
ceiling.voltageWrite(2, 5.0f);  // channel 2, 0.0-10.0 volts
```

## Configuration

Use `DropCeilingConfig` when you need to override defaults:

```cpp
DropCeilingConfig config;
config.artnetUniverse = 0;
config.i2cSdaPin = 42;
config.i2cSclPin = 41;
config.displayMode = DropCeilingDisplay_Auto;

DropCeiling ceiling(DropCeilingMode_Ethernet, config);
```

The defaults preserve the original controller sketch behavior: DHCP with static fallback, WIZ5500 CS pin 10, DAC6578 address `0x4C`, 10-bit DAC output, ArtNet universe 0, 8 channels, and a 3-second live-signal timeout.

## BLE Contract

The BLE mode creates one service and one characteristic. The characteristic accepts exactly one 8-byte payload, where byte 0 controls output 1 and byte 7 controls output 8.

Defaults:

| Item | UUID/name |
| --- | --- |
| Device name | `DropCeiling` |
| Service UUID | `7b4f0001-3f3a-4f6f-9b0d-84e8f64f0f10` |
| Characteristic UUID | `7b4f0002-3f3a-4f6f-9b0d-84e8f64f0f10` |

This shape is designed for Web Bluetooth clients, including p5.ble.js.

## Serial CSV

Serial mode reads newline-terminated CSV strings:

```text
0,128,255,0,0,0,0,0
```

Values are clamped to 0-255. By default, a short line updates only the channels provided.

## Examples

- `examples/EthernetArtNet`
- `examples/WiFiArtNet`
- `examples/BluetoothInput`
- `examples/SerialInput`
- `examples/Standalone`

From the repository root, compile an example with:

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3_tft --libraries .. examples/EthernetArtNet
```

## Control Software

Non-Arduino controller examples are included under `extras/control-software/`. Keeping them in `extras/` preserves the standard Arduino library structure while packaging examples for ArtNet Ethernet, ArtNet WiFi, BLE, Serial CSV, and Standalone modes.

## Project Context

See `PROJECT_CONTEXT.md` for the hardware background, source-repo history, compatibility notes, and next engineering steps.
