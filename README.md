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

## Drop Ceiling V2 node

`examples/DropCeilingV2Node` is the firmware every Drop Ceiling V2 board runs, one binary for all boards. It
carries the firmware items from dropCeilingV2-DEV's `docs/decisions/D0004-wiring.md`:

| Item | What the node does |
| --- | --- |
| FW1 | Node number `n` in flash (NVS), set once over USB serial with `node <n>`. Everything below derives from it |
| FW2 | Static `10.42.0.(200+n)/24`, MAC `02:DC:20:27:00:0n`. `net dhcp` and `subnet a.b.c` change this for a bench |
| FW3 | Art-Net universe `n`, start slot 1, short name `DC2-Kn` |
| FW4 | `res 16`: two slots per channel, high byte then low, DAC code = value >> 6. `res 8` (default): one slot per channel |
| FW5 | After 10 s without a frame, fade to 0 over 30 s. Frames after a loss or a restart fade in over 2 s |
| FW6 | Health in ArtPollReply (version, MAC, node report `up= fr= e= rs= r=` and `LIVE`/`WAIT`/`LOST`), the fuller status line over UDP (below), and a 15 s task watchdog |

A restart of the processor no longer drops the outputs: the library reads back what the DAC holds and starts from there.
Node 0 means unassigned: DHCP, universe 0, and a MAC made from the chip's own.

Serial console at 115200 baud: `show` prints one JSON line with identity, network, health and the DAC codes. `node`,
`res`, `net` and `subnet` write flash and restart.

The same line over the network: send `show` in a UDP packet to port 6455 and the board answers with it. Beside the
fields above it carries `tempC` (the chip), `heapFree` and `heapMin`, `loopMaxMs` and `loopMaxSinceMs` (the longest
pass through the loop since start and since the last query: a long pass is what drops or bunches frames), and
`levels`, the 16-bit output after any fade. It is read only; settings change over USB alone.

The screen shows the board's name as large as it fits, its state in colour (`LIVE` green, `WAIT` amber, `LOST`,
`NO LINK` or `NO DAC` red, worst first), its address, universe, firmware and resolution, and the eight output levels as
bars. It is redrawn at most every 100 ms and only where something changed, so it costs the Art-Net loop little. FW7 (a dead board against a live driver) is a bench test, and FW8
(light sensors) is not built.

### Levels

Every input now works in 16-bit levels, 0-65535. A byte `v` becomes `v * 257`, so 255 still reaches full scale and a
byte lands within one DAC code of the old `map(v, 0, 255, 0, 1023)`. `writeLevel()`, `getLevel()`, `getOutputLevel()`
and `getCode()` expose them. Loss-of-signal fades are off unless `config.lossTimeoutMs` is set, so the other examples
behave as before.

### Tests

`extras/tests/run.sh` builds and runs desktop tests of the level maths, the fades, ArtDmx parsing in both resolutions
and the ArtPollReply layout. CI runs them and compiles every example.

## Project Context

See `PROJECT_CONTEXT.md` for the hardware background, source-repo history, compatibility notes, and next engineering steps.
