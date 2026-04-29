# p5.js BLE Control Example

Use this browser sketch with the Arduino sketch in `examples/BluetoothInput/`.

The board advertises as `DropCeiling` and exposes one writable 8-byte characteristic. Each byte controls one output channel, with byte 0 mapped to output 1 and byte 7 mapped to output 8.

Default BLE values:

| Item | Value |
| --- | --- |
| Service UUID | `7b4f0001-3f3a-4f6f-9b0d-84e8f64f0f10` |
| Characteristic UUID | `7b4f0002-3f3a-4f6f-9b0d-84e8f64f0f10` |
| Payload | 8 bytes, 0-255 |

## Run

Web Bluetooth generally requires a secure context. Localhost is accepted by Chrome-based browsers.

From this folder:

```bash
python3 -m http.server 8000
```

Open:

```text
http://localhost:8000
```

Click Connect, choose the DropCeiling board, and move the sliders.
