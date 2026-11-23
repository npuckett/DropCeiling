// Drop Ceiling V2 node: the firmware every V2 board runs. One binary for all boards.
//
// A board's node number n lives in flash (NVS), written once at the bench over USB serial:
//   node 3
// Everything else follows from n, as dropCeilingV2-DEV's layout/wiring.json lists it per controller Kn:
//   MAC 02:DC:20:27:00:0n, IP 10.42.0.(200+n)/24, Art-Net universe n, short name DC2-Kn.
// Node 0 means not yet assigned: DHCP, universe 0, and a MAC made from the chip's own, so two new
// boards on one network never clash.
//
// Firmware items from dropCeilingV2-DEV docs/decisions/D0004-wiring.md:
//   FW1 identity from one node number      FW2 static address
//   FW3 universe n                         FW4 16-bit levels (res 16), 8-bit kept (res 8, the default)
//   FW5 fade on loss of signal             FW6 health in ArtPollReply, hardware watchdog
// FW7 is a bench test with a live driver, not firmware. FW8 (light sensors) is not here yet.
//
// Serial console, 115200 baud, one command a line:
//   show                 one JSON line: identity, network, health, levels
//   node <0-54>          set the node number, then restart
//   res <8|16>           Art-Net resolution, then restart
//   net <static|dhcp>    addressing, then restart
//   subnet <a.b.c>       first three octets for the static address, then restart (default 10.42.0)
//   reboot
//   help
//
// Status over the network: send "show" in a UDP packet to port 6455 and the board answers with the same JSON line
// `show` prints here, to whoever asked. Read only: settings change over USB, never over the network. The tower's
// board tests (dropCeilingV2-DEV ops/boards) poll it beside Art-Net's ArtPoll, which carries a shorter report.

#include <DropCeiling.h>
#include <Preferences.h>
#include <stdarg.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>

static const uint8_t FW_MAJOR = 2;
static const uint8_t FW_MINOR = 1;
static const char FW_VERSION[] = "2.1.1";

// Loss of signal, from dropCeilingV2-DEV engine/DESIGN.md: after 10 s of silence fade to 0 over 30 s.
// Off is dark, so this reads as closed and not broken. Frames after a loss, or after a restart, are
// faded in over 2 s rather than jumped to.
static const uint32_t LOSS_TIMEOUT_MS = 10000;
static const uint32_t LOSS_FADE_MS = 30000;
static const uint32_t RESUME_FADE_MS = 2000;

// The loop resets the watchdog every pass. A loop stuck this long restarts the processor; the board takes
// up from what the DAC holds. 15 s, because a failed DHCP renewal (net dhcp only) blocks the loop about 8 s.
static const uint32_t WATCHDOG_MS = 15000;
// 200 + n must stay a host address below .255.
static const uint8_t MAX_NODE = 54;
// A board whose DAC or network did not start restarts after this long, loudly, rather than sit dark.
static const uint32_t NOT_READY_RESTART_MS = 30000;
// The status port: Art-Net's 6454 plus one.
static const uint16_t STATUS_PORT = 6455;

Preferences prefs;
DropCeilingConfig config;
DropCeiling *ceiling = nullptr;

uint8_t node = 0;
uint8_t resolution = 8;
bool useDhcp = false;
uint8_t subnet[3] = {10, 42, 0};
char shortName[18];
char longName[64];
char chipId[18];
char consoleLine[64];
uint8_t consoleLength = 0;
uint32_t lastReport = 0;
EthernetUDP statusUdp;
bool statusOpen = false;
char statusLine[768];
// The longest pass through loop(), in ms: since start, and since the last status query. A long pass is what drops
// or bunches frames, so a load test wants to see it.
uint32_t loopMaxMs = 0;
uint32_t loopMaxSinceMs = 0;

const char *resetCause() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "PWR";
    case ESP_RST_EXT: return "EXT";
    case ESP_RST_SW: return "SW";
    case ESP_RST_PANIC: return "PANIC";
    case ESP_RST_INT_WDT: return "IWDT";
    case ESP_RST_TASK_WDT: return "TWDT";
    case ESP_RST_WDT: return "WDT";
    case ESP_RST_BROWNOUT: return "BROWN";
    case ESP_RST_DEEPSLEEP: return "SLEEP";
    case ESP_RST_SDIO: return "SDIO";
#if ESP_IDF_VERSION_MAJOR >= 5
    case ESP_RST_USB: return "USB";
    case ESP_RST_JTAG: return "JTAG";
#endif
    default: return "OTHER";
  }
}

const char *signalName(DropCeilingSignal s) {
  switch (s) {
    case DropCeilingSignal_Live: return "LIVE";
    case DropCeilingSignal_Lost: return "LOST";
    default: return "WAIT";
  }
}

void loadSettings() {
  prefs.begin("dc2", true);
  node = prefs.getUChar("node", 0);
  if (node > MAX_NODE) {
    node = 0;
  }
  resolution = prefs.getUChar("res", 8);
  useDhcp = prefs.getBool("dhcp", false);
  if (prefs.getBytesLength("subnet") == 3) {
    prefs.getBytes("subnet", subnet, 3);
  }
  prefs.end();
  if (resolution != 16) {
    resolution = 8;
  }
}

void deriveIdentity() {
  uint8_t chip[6];
  esp_efuse_mac_get_default(chip);
  snprintf(chipId, sizeof(chipId), "%02X:%02X:%02X:%02X:%02X:%02X", chip[0], chip[1], chip[2], chip[3], chip[4], chip[5]);

  // Locally administered MACs (02:...). Assigned boards use the plan's; new ones borrow the chip's tail.
  uint8_t mac[6] = {0x02, 0xDC, 0x20, 0x27, 0x00, node};
  if (node == 0) {
    mac[2] = chip[2];
    mac[3] = chip[3];
    mac[4] = chip[4];
    mac[5] = chip[5];
  }
  memcpy(config.mac, mac, 6);

  config.useDhcp = node == 0 || useDhcp;
  // Unassigned boards fall back to .150-.199 by their chip, so two new boards rarely clash without DHCP.
  config.staticIp = IPAddress(subnet[0], subnet[1], subnet[2], node == 0 ? 150 + chip[5] % 50 : 200 + node);
  config.staticSubnet = IPAddress(255, 255, 255, 0);
  config.staticGateway = IPAddress(subnet[0], subnet[1], subnet[2], 1);
  config.staticDns = IPAddress(subnet[0], subnet[1], subnet[2], 1);
  // DHCP is tried for 8 s before the static fallback. The watchdog starts after setup().
  config.dhcpTimeoutMs = 8000;

  config.artnetUniverse = node;
  config.artnetStartChannel = 0;
  config.artnetResolution = resolution;

  if (node == 0) {
    snprintf(shortName, sizeof(shortName), "DC2-new-%02X%02X", chip[4], chip[5]);
  } else {
    snprintf(shortName, sizeof(shortName), "DC2-K%u", node);
  }
  snprintf(longName, sizeof(longName), "Drop Ceiling V2 %s, 8 x 0-10V, fw %s, %u-bit", shortName, FW_VERSION, resolution);
  config.nodeShortName = shortName;
  config.nodeLongName = longName;
  config.firmwareVersionHi = FW_MAJOR;
  config.firmwareVersionLo = FW_MINOR;

  config.lossTimeoutMs = LOSS_TIMEOUT_MS;
  config.lossFadeMs = LOSS_FADE_MS;
  config.lossLevel = 0;
  config.resumeFadeMs = RESUME_FADE_MS;
  config.nodeReportFn = writeReport;
}

// The node report, written as each ArtPollReply is built, so its frame count is exact at the moment of the poll.
// Read by the tower's board tests (dropCeilingV2-DEV ops/boards): key=value, space separated, then the state.
void writeReport(char *out, size_t size) {
  if (ceiling == nullptr) {
    snprintf(out, size, "starting");
    return;
  }
  snprintf(out, size, "up=%llu fr=%lu e=%lu rs=%s r=%u %s", uptimeS(),
           static_cast<unsigned long>(ceiling->frames()), static_cast<unsigned long>(ceiling->dacErrors()), resetCause(),
           resolution, ceiling->dacReady() ? signalName(ceiling->signal()) : "NODAC");
}

// 64-bit, so it never wraps: millis() would every 49.7 days, and the tower reads a smaller uptime as a restart.
unsigned long long uptimeS() {
  return static_cast<unsigned long long>(esp_timer_get_time() / 1000000LL);
}

// Appends to statusLine; the line is cut short rather than overrun.
size_t statusAppend(size_t at, const char *format, ...) {
  if (at >= sizeof(statusLine)) {
    return at;
  }
  va_list args;
  va_start(args, format);
  int n = vsnprintf(statusLine + at, sizeof(statusLine) - at, format, args);
  va_end(args);
  return n < 0 ? at : at + static_cast<size_t>(n);
}

// One JSON line: identity, network, health, levels. The same over USB serial and over UDP.
const char *formatStatus() {
  IPAddress ip = ceiling->localIP();
  const uint8_t *m = config.mac;
  size_t at = 0;
  at = statusAppend(at, "{\"fw\":\"%s\",\"node\":%u,\"name\":\"%s\",\"chipMac\":\"%s\"", FW_VERSION, node, shortName, chipId);
  at = statusAppend(at, ",\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\"", m[0], m[1], m[2], m[3], m[4], m[5]);
  at = statusAppend(at, ",\"ip\":\"%u.%u.%u.%u\",\"dhcp\":%s,\"universe\":%u,\"res\":%u", ip[0], ip[1], ip[2], ip[3],
                    ceiling->ethernet().usingDhcp() ? "true" : "false", config.artnetUniverse, resolution);
  at = statusAppend(at, ",\"ethernet\":%s,\"link\":%s,\"dac\":%s", ceiling->ethernet().hardwareFound() ? "true" : "false",
                    ceiling->ethernet().linkUp() ? "true" : "false", ceiling->dacReady() ? "true" : "false");
  at = statusAppend(at, ",\"signal\":\"%s\",\"frames\":%lu,\"losses\":%lu,\"dacErrors\":%lu,\"uptimeS\":%llu,\"reset\":\"%s\"",
                    signalName(ceiling->signal()), static_cast<unsigned long>(ceiling->frames()),
                    static_cast<unsigned long>(ceiling->losses()), static_cast<unsigned long>(ceiling->dacErrors()),
                    uptimeS(), resetCause());
  at = statusAppend(at, ",\"tempC\":%.1f,\"heapFree\":%lu,\"heapMin\":%lu,\"loopMaxMs\":%lu,\"loopMaxSinceMs\":%lu",
                    temperatureRead(), static_cast<unsigned long>(ESP.getFreeHeap()),
                    static_cast<unsigned long>(ESP.getMinFreeHeap()), static_cast<unsigned long>(loopMaxMs),
                    static_cast<unsigned long>(loopMaxSinceMs));
  at = statusAppend(at, ",\"levels\":[");
  for (uint8_t c = 1; c <= DROPCEILING_CHANNELS; c++) {
    at = statusAppend(at, "%s%u", c == 1 ? "" : ",", ceiling->getOutputLevel(c));
  }
  at = statusAppend(at, "],\"codes\":[");
  for (uint8_t c = 1; c <= DROPCEILING_CHANNELS; c++) {
    at = statusAppend(at, "%s%u", c == 1 ? "" : ",", ceiling->getCode(c));
  }
  statusAppend(at, "]}");
  return statusLine;
}

void show() {
  Serial.println(formatStatus());
}

// Answers "show" on the status port. One query per loop pass at most, so a flood cannot starve Art-Net.
void serveStatus() {
  if (!statusOpen || statusUdp.parsePacket() <= 0) {
    return;
  }
  char ask[8] = {0};
  statusUdp.read(reinterpret_cast<uint8_t *>(ask), sizeof(ask) - 1);
  while (statusUdp.available() > 0) {
    statusUdp.read();
  }
  if (strncmp(ask, "show", 4) != 0) {
    return;
  }
  const char *line = formatStatus();
  loopMaxSinceMs = 0;
  if (statusUdp.beginPacket(statusUdp.remoteIP(), statusUdp.remotePort())) {
    statusUdp.write(reinterpret_cast<const uint8_t *>(line), strlen(line));
    statusUdp.write(static_cast<uint8_t>('\n'));
    statusUdp.endPacket();
  }
}

void restart(const char *why) {
  Serial.printf("restarting: %s\n", why);
  Serial.flush();
  delay(100);
  ESP.restart();
}

void handleCommand(char *line) {
  char *cmd = strtok(line, " ");
  char *arg = strtok(nullptr, " ");
  if (cmd == nullptr) {
    return;
  }
  if (strcmp(cmd, "show") == 0) {
    show();
  } else if (strcmp(cmd, "node") == 0 && arg != nullptr) {
    int n = atoi(arg);
    if (n < 0 || n > MAX_NODE || (n == 0 && strcmp(arg, "0") != 0)) {
      Serial.printf("error: node is 0 to %u\n", MAX_NODE);
      return;
    }
    prefs.begin("dc2", false);
    prefs.putUChar("node", static_cast<uint8_t>(n));
    prefs.end();
    restart("node set");
  } else if (strcmp(cmd, "res") == 0 && arg != nullptr) {
    int r = atoi(arg);
    if (r != 8 && r != 16) {
      Serial.println("error: res is 8 or 16");
      return;
    }
    prefs.begin("dc2", false);
    prefs.putUChar("res", static_cast<uint8_t>(r));
    prefs.end();
    restart("resolution set");
  } else if (strcmp(cmd, "net") == 0 && arg != nullptr) {
    if (strcmp(arg, "static") != 0 && strcmp(arg, "dhcp") != 0) {
      Serial.println("error: net is static or dhcp");
      return;
    }
    prefs.begin("dc2", false);
    prefs.putBool("dhcp", strcmp(arg, "dhcp") == 0);
    prefs.end();
    restart("addressing set");
  } else if (strcmp(cmd, "subnet") == 0 && arg != nullptr) {
    int a, b, c;
    if (sscanf(arg, "%d.%d.%d", &a, &b, &c) != 3 || a < 1 || a > 223 || b < 0 || b > 255 || c < 0 || c > 255) {
      Serial.println("error: subnet is a.b.c, as in 10.42.0");
      return;
    }
    uint8_t s[3] = {static_cast<uint8_t>(a), static_cast<uint8_t>(b), static_cast<uint8_t>(c)};
    prefs.begin("dc2", false);
    prefs.putBytes("subnet", s, 3);
    prefs.end();
    restart("subnet set");
  } else if (strcmp(cmd, "reboot") == 0) {
    restart("asked");
  } else {
    Serial.println("commands: show | node <0-54> | res <8|16> | net <static|dhcp> | subnet <a.b.c> | reboot");
  }
}

void readConsole() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      consoleLine[consoleLength] = '\0';
      handleCommand(consoleLine);
      consoleLength = 0;
    } else if (consoleLength < sizeof(consoleLine) - 1) {
      consoleLine[consoleLength++] = c;
    } else {
      consoleLength = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  loadSettings();
  deriveIdentity();

  ceiling = new DropCeiling(DropCeilingMode_Ethernet, config);
  bool ok = ceiling->setup();
  Serial.printf("DropCeiling V2 node %s fw %s: %s\n", shortName, FW_VERSION, ok ? "ready" : "NOT READY");
  if (!ceiling->dacReady()) {
    Serial.println("ERROR: DAC did not answer on I2C");
  }
  if (!ceiling->ethernet().hardwareFound()) {
    Serial.println("ERROR: no W5500 Ethernet found");
  }
  if (ceiling->ethernet().hardwareFound()) {
    statusOpen = statusUdp.begin(STATUS_PORT) == 1;
  }
  show();

  esp_task_wdt_config_t wdt = {};
  wdt.timeout_ms = WATCHDOG_MS;
  wdt.idle_core_mask = 0;
  wdt.trigger_panic = true;
  if (esp_task_wdt_reconfigure(&wdt) != ESP_OK) {
    esp_task_wdt_init(&wdt);
  }
  esp_task_wdt_add(nullptr);
}

void loop() {
  static uint32_t passStart = millis();
  esp_task_wdt_reset();
  ceiling->read();
  readConsole();
  serveStatus();

  uint32_t pass = millis() - passStart;
  passStart = millis();
  if (pass > loopMaxMs) {
    loopMaxMs = pass;
  }
  if (pass > loopMaxSinceMs) {
    loopMaxSinceMs = pass;
  }

  uint32_t now = millis();
  if (now - lastReport >= 1000) {
    lastReport = now;
    if (!ceiling->ready() && now > NOT_READY_RESTART_MS) {
      restart(ceiling->dacReady() ? "network not ready" : "DAC not ready");
    }
  }
}
