# ArtNet Ethernet Control Examples

Use these with the Arduino sketch in `examples/EthernetArtNet/`.

The board listens for ArtNet on UDP port 6454. The default static fallback IP is `192.168.1.200`, and ArtPoll discovery is supported.

## Python Controller

`controller.py` is a small PySide6 desktop controller with 8 sliders, ArtPoll discovery, all-zero/all-max buttons, and a wave preset.

Install dependencies in a Python environment:

```bash
pip install PySide6 netifaces
```

Run it:

```bash
python controller.py
```

## TouchDesigner Module

`touchdesigner/td_artnet_script.py` is a module intended to be pasted into a Text DAT named `text_artnet`.

Basic use from TouchDesigner Python:

```python
artnet = op('text_artnet').module
artnet.connect()
artnet.send_dmx([0, 128, 255, 0, 0, 0, 0, 0])
```
