#include "DropCeilingBLE.h"

DropCeilingBLEInput::DropCeilingBLEInput()
  : _hasPending(false), _clientConnected(false)
#if defined(ARDUINO_ARCH_ESP32)
    , _server(nullptr), _characteristic(nullptr)
#endif
{
  memset(_pendingValues, 0, sizeof(_pendingValues));
}

bool DropCeilingBLEInput::begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) {
  _config = config;
  memcpy(_pendingValues, currentValues, DROPCEILING_CHANNELS);

#if !defined(ARDUINO_ARCH_ESP32)
  return false;
#else
  BLEDevice::init(_config.bleDeviceName);
  _server = BLEDevice::createServer();
  _server->setCallbacks(this);

  BLEService *service = _server->createService(_config.bleServiceUuid);
  _characteristic = service->createCharacteristic(
    _config.bleChannelsUuid,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY
  );
  _characteristic->setCallbacks(this);
  _characteristic->addDescriptor(new BLE2902());
  _characteristic->setValue(_pendingValues, DROPCEILING_CHANNELS);

  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(_config.bleServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();
  return true;
#endif
}

bool DropCeilingBLEInput::read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) {
  changed = false;
  if (!_hasPending) {
    return false;
  }

  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    if (values[channel] != _pendingValues[channel]) {
      values[channel] = _pendingValues[channel];
      changed = true;
    }
  }
  _hasPending = false;

#if defined(ARDUINO_ARCH_ESP32)
  if (_characteristic != nullptr && _config.bleNotifyOnChange) {
    _characteristic->setValue(values, DROPCEILING_CHANNELS);
    _characteristic->notify();
  }
#endif

  return true;
}

bool DropCeilingBLEInput::connected() const {
  return _clientConnected;
}

IPAddress DropCeilingBLEInput::localIP() const {
  return IPAddress(0, 0, 0, 0);
}

#if defined(ARDUINO_ARCH_ESP32)
void DropCeilingBLEInput::onWrite(BLECharacteristic *characteristic) {
  auto payload = characteristic->getValue();
  size_t length = payload.length();
  if (length < DROPCEILING_CHANNELS) {
    return;
  }

  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    _pendingValues[channel] = static_cast<uint8_t>(payload[channel]);
  }
  _hasPending = true;
}

void DropCeilingBLEInput::onConnect(BLEServer *server) {
  (void)server;
  _clientConnected = true;
}

void DropCeilingBLEInput::onDisconnect(BLEServer *server) {
  (void)server;
  _clientConnected = false;
  BLEDevice::startAdvertising();
}
#endif
