class DropCeilingWebSerial {
  constructor() {
    this.port = null;
    this.writer = null;
    this.encoder = new TextEncoder();
  }

  supported() {
    return "serial" in navigator;
  }

  async connect(baudRate = 115200) {
    if (!this.supported()) {
      throw new Error("Web Serial is not supported in this browser");
    }

    this.port = await navigator.serial.requestPort();
    await this.port.open({ baudRate });
    this.writer = this.port.writable.getWriter();
  }

  async writeLine(line) {
    if (this.writer === null) {
      return;
    }
    await this.writer.write(this.encoder.encode(line));
  }

  async disconnect() {
    if (this.writer !== null) {
      this.writer.releaseLock();
      this.writer = null;
    }
    if (this.port !== null) {
      await this.port.close();
      this.port = null;
    }
  }

  connected() {
    return this.port !== null && this.writer !== null;
  }
}
