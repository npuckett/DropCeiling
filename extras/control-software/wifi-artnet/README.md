# ArtNet WiFi Control Examples

Use these with the Arduino sketch in `examples/WiFiArtNet/`.

ArtNet over WiFi uses the same ArtDmx and ArtPoll packet structure as wired Ethernet. These files are copies of the Ethernet ArtNet controller examples so they can be found from the WiFi input mode folder.

Before using this mode, set WiFi credentials in the Arduino sketch:

```cpp
ceiling.setWiFiCredentials("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
```

The computer running the controller software and the DropCeiling board must be on the same network or otherwise able to route UDP port 6454.

## Python Controller

Install dependencies:

```bash
pip install PySide6 netifaces
```

Run it:

```bash
python controller.py
```

## TouchDesigner Module

Paste `touchdesigner/td_artnet_script.py` into a Text DAT named `text_artnet`.

```python
artnet = op('text_artnet').module
artnet.connect()
artnet.send_dmx([0, 128, 255, 0, 0, 0, 0, 0])
```
