# Standalone Mode

Standalone mode does not require external control software. Use the Arduino sketch in `examples/Standalone/` and write values directly from firmware:

```cpp
ceiling.voltageWrite(1, 128);
ceiling.voltageWrite(2, 5.0f);
```

This folder exists so every DropCeiling input mode has a matching control-software entry. Future standalone pattern editors or preset generators can live here without changing the Arduino library structure.
