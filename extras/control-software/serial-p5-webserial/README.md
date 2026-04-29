# p5.js Serial Control Example

Use this browser sketch with the Arduino sketch in `examples/SerialInput/`.

The sketch sends newline-terminated CSV strings at 115200 baud:

```text
0,128,255,0,0,0,0,0
```

The Arduino library clamps values to 0-255. By default, all eight sliders are sent every time a value changes.

## Run

Web Serial requires a Chrome-based browser and a secure context. Localhost is accepted.

From this folder:

```bash
python3 -m http.server 8000
```

Open:

```text
http://localhost:8000
```

Click Connect Serial, choose the board serial port, and move the sliders.
