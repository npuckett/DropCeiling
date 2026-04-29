const NUM_CHANNELS = 8;
const BAUD_RATE = 115200;

let serial;
let sliders = [];
let valueLabels = [];
let statusLabel;
let lastLine = "";

function setup() {
  noCanvas();
  serial = new DropCeilingWebSerial();

  const app = select("#app");
  const panel = createDiv().addClass("panel").parent(app);
  createElement("h1", "DropCeiling Serial Controller").parent(panel);
  statusLabel = createP("Disconnected").addClass("status").parent(panel);

  const actions = createDiv().addClass("actions").parent(panel);
  createButton("Connect Serial").mousePressed(connectSerial).parent(actions);
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

async function connectSerial() {
  try {
    await serial.connect(BAUD_RATE);
    statusLabel.html(`Connected at ${BAUD_RATE} baud`);
    sendChannels();
  } catch (error) {
    statusLabel.html(`Serial error: ${error.message}`);
  }
}

function currentValues() {
  return sliders.map((slider) => Number(slider.value()));
}

async function sendChannels() {
  if (!serial.connected()) {
    return;
  }

  const line = `${currentValues().join(",")}\n`;
  if (line === lastLine) {
    return;
  }
  lastLine = line;
  await serial.writeLine(line);
}

function setAll(value) {
  for (let channel = 0; channel < NUM_CHANNELS; channel++) {
    sliders[channel].value(value);
    valueLabels[channel].html(value);
  }
  sendChannels();
}

window.addEventListener("beforeunload", () => {
  if (serial && serial.connected()) {
    serial.disconnect();
  }
});
