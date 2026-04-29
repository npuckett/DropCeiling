const SERVICE_UUID = "7b4f0001-3f3a-4f6f-9b0d-84e8f64f0f10";
const CHARACTERISTIC_UUID = "7b4f0002-3f3a-4f6f-9b0d-84e8f64f0f10";
const NUM_CHANNELS = 8;

let ble;
let channelCharacteristic = null;
let isConnected = false;
let sliders = [];
let valueLabels = [];
let statusLabel;

function setup() {
  noCanvas();
  ble = new p5ble();

  const app = select("#app");
  const panel = createDiv().addClass("panel").parent(app);
  createElement("h1", "DropCeiling BLE Controller").parent(panel);
  statusLabel = createP("Disconnected").addClass("status").parent(panel);

  const actions = createDiv().addClass("actions").parent(panel);
  createButton("Connect").mousePressed(connectBoard).parent(actions);
  createButton("All Zero").mousePressed(() => setAll(0)).parent(actions);
  createButton("All Max").mousePressed(() => setAll(255)).parent(actions);
  createButton("Send").mousePressed(sendChannels).parent(actions);

  for (let channel = 0; channel < NUM_CHANNELS; channel++) {
    const row = createDiv().addClass("row").parent(panel);
    createSpan(`CH ${channel + 1}`).parent(row);
    const slider = createSlider(0, 255, 0, 1).parent(row);
    const label = createSpan("0").parent(row);
    slider.input(() => {
      label.html(slider.value());
      sendChannels();
    });
    sliders.push(slider);
    valueLabels.push(label);
  }
}

function connectBoard() {
  statusLabel.html("Scanning for DropCeiling...");
  ble.connect(SERVICE_UUID, gotCharacteristics);
}

function gotCharacteristics(error, characteristics) {
  if (error) {
    statusLabel.html(`Connection error: ${error}`);
    return;
  }

  channelCharacteristic = characteristics.find((characteristic) => {
    return characteristic.uuid.toLowerCase() === CHARACTERISTIC_UUID;
  }) || characteristics[0];

  isConnected = true;
  statusLabel.html("Connected");
  sendChannels();
}

function currentValues() {
  return sliders.map((slider) => Number(slider.value()));
}

function sendChannels() {
  if (!isConnected || channelCharacteristic === null) {
    return;
  }

  const payload = Uint8Array.from(currentValues());
  ble.write(channelCharacteristic, payload);
}

function setAll(value) {
  for (let channel = 0; channel < NUM_CHANNELS; channel++) {
    sliders[channel].value(value);
    valueLabels[channel].html(value);
  }
  sendChannels();
}
