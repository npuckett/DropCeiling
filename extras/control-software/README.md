# DropCeiling Control Software Examples

These examples are packaged under `extras/` so the repository remains a valid Arduino library for Arduino Library Manager. The Arduino library code stays in `src/`, Arduino sketches stay in `examples/`, and non-Arduino controller software lives here.

| Input mode | Folder | Use with Arduino example |
| --- | --- | --- |
| ArtNet over Ethernet | `artnet-ethernet/` | `examples/EthernetArtNet/` |
| ArtNet over WiFi | `wifi-artnet/` | `examples/WiFiArtNet/` |
| BLE | `bluetooth-p5/` | `examples/BluetoothInput/` |
| Serial CSV | `serial-p5-webserial/` | `examples/SerialInput/` |
| Standalone | `standalone/` | `examples/Standalone/` |

The ArtNet Ethernet and WiFi controller examples use the same packet format. The only difference is the network path between the controller computer and the board.
