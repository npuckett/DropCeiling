// Desktop tests for the parts of the library that need no hardware: levels, the loss and resume fades,
// ArtDmx parsing in 8 and 16 bits, and the ArtPollReply layout. Run: extras/tests/run.sh
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "DropCeilingArtNet.h"
#include "DropCeilingLevels.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static size_t artdmx(uint8_t *p, uint16_t universe, const uint8_t *data, uint16_t n) {
  memcpy(p, "Art-Net\0", 8);
  p[8] = 0x00; p[9] = 0x50; p[10] = 0; p[11] = 14; p[12] = 1; p[13] = 0;
  p[14] = universe & 0xFF; p[15] = universe >> 8; p[16] = n >> 8; p[17] = n & 0xFF;
  memcpy(p + 18, data, n);
  return 18 + n;
}

static void levels() {
  CHECK(DropCeilingLevels::fromByte(0) == 0);
  CHECK(DropCeilingLevels::fromByte(255) == 65535);
  CHECK(DropCeilingLevels::toCode(DropCeilingLevels::fromByte(255), 10) == 1023);
  CHECK(DropCeilingLevels::toCode(DropCeilingLevels::fromByte(0), 10) == 0);
  // A byte lands within one code of the old map(v, 0, 255, 0, 1023).
  for (int v = 0; v < 256; v++) {
    int old = v * 1023 / 255;
    int now = DropCeilingLevels::toCode(DropCeilingLevels::fromByte(v), 10);
    CHECK(now - old >= 0 && now - old <= 1);
  }
  // FW4: DAC code = value >> 6.
  CHECK(DropCeilingLevels::toCode(0xFFFF, 10) == 1023);
  CHECK(DropCeilingLevels::toCode(0x0040, 10) == 1);
  CHECK(DropCeilingLevels::toCode(0x003F, 10) == 0);
  CHECK(DropCeilingLevels::lerp(0, 1000, 0, 100) == 0);
  CHECK(DropCeilingLevels::lerp(0, 1000, 50, 100) == 500);
  CHECK(DropCeilingLevels::lerp(1000, 0, 50, 100) == 500);
  CHECK(DropCeilingLevels::lerp(0, 1000, 100, 100) == 1000);
  CHECK(DropCeilingLevels::lerp(0, 1000, 5, 0) == 1000);
  CHECK(DropCeilingLevels::lerp(65535, 0, 1, 2) == 32768 || DropCeilingLevels::lerp(65535, 0, 1, 2) == 32767);
}

static void fader() {
  uint16_t start[8] = {1000, 0, 0, 0, 0, 0, 0, 0};
  uint16_t target[8] = {3000, 0, 0, 0, 0, 0, 0, 0};
  DropCeilingFader f;
  f.lossTimeoutMs = 10000;
  f.lossFadeMs = 30000;
  f.lossLevel = 0;
  f.resumeFadeMs = 2000;

  // Boot holds what the DAC had, even with a target set.
  f.begin(start, 0);
  CHECK(f.update(target, 500)[0] == 1000);
  CHECK(f.signal() == DropCeilingSignal_Waiting);

  // First frame: fade from 1000 to 3000 over 2 s, then follow.
  f.frame(1000);
  CHECK(f.signal() == DropCeilingSignal_Live);
  CHECK(f.update(target, 1000)[0] == 1000);
  CHECK(f.update(target, 2000)[0] == 2000);
  CHECK(f.update(target, 3000)[0] == 3000);
  target[0] = 5000;
  CHECK(f.update(target, 3001)[0] == 5000);  // live: follows exactly

  // Steady frames keep it live, never steps.
  for (uint32_t t = 3100; t < 20000; t += 100) {
    f.frame(t);
    CHECK(f.update(target, t)[0] == 5000);
  }
  // Silence: nothing for 10 s, then a 30 s fade to 0.
  uint32_t last = 19900;
  CHECK(f.update(target, last + 9999)[0] == 5000);
  CHECK(f.signal() == DropCeilingSignal_Live);
  CHECK(f.update(target, last + 10000)[0] == 5000);
  CHECK(f.signal() == DropCeilingSignal_Lost);
  CHECK(f.losses() == 1);
  CHECK(f.update(target, last + 25000)[0] == 2500);
  CHECK(f.update(target, last + 40000)[0] == 0);
  CHECK(f.update(target, last + 90000)[0] == 0);

  // Frames again: fade up from 0 over 2 s, no jump.
  uint32_t back = last + 100000;
  f.frame(back);
  CHECK(f.update(target, back)[0] == 0);
  CHECK(f.update(target, back + 1000)[0] == 2500);
  CHECK(f.update(target, back + 2000)[0] == 5000);

  // Loss in the middle of a resume fade starts from where it got to.
  f.frame(back + 3000);
  CHECK(f.update(target, back + 13000)[0] == 5000);
  CHECK(f.signal() == DropCeilingSignal_Lost);

  // No frame ever after boot: hold, then fade at the timeout.
  DropCeilingFader g;
  g.lossTimeoutMs = 10000; g.lossFadeMs = 30000; g.lossLevel = 0; g.resumeFadeMs = 2000;
  uint16_t held[8] = {6000, 6000, 6000, 6000, 6000, 6000, 6000, 6000};
  g.begin(held, 0);
  CHECK(g.update(target, 9999)[3] == 6000);
  CHECK(g.update(target, 10000)[3] == 6000);
  CHECK(g.signal() == DropCeilingSignal_Lost);
  CHECK(g.update(target, 25000)[3] == 3000);

  // millis() wraps after 49.7 days: unsigned differences carry across it.
  DropCeilingFader w;
  w.lossTimeoutMs = 10000; w.lossFadeMs = 30000; w.lossLevel = 0; w.resumeFadeMs = 0;
  uint32_t near = 0xFFFFFFFFu - 5000;
  w.begin(held, near);
  w.frame(near);
  CHECK(w.update(target, near + 9000)[0] == 5000);
  CHECK(w.signal() == DropCeilingSignal_Live);
  CHECK(w.update(target, near + 10000)[0] == 5000);
  CHECK(w.signal() == DropCeilingSignal_Lost);

  // Turned off: the output is the target, silence holds.
  DropCeilingFader off;
  off.begin(held, 0);
  CHECK(off.update(target, 999999)[0] == 5000);
}

static void parse() {
  DropCeilingConfig config;
  config.artnetUniverse = 3;
  uint16_t lv[8] = {0};
  uint8_t pkt[600];
  uint8_t data8[8] = {0, 1, 2, 128, 254, 255, 7, 9};

  size_t n = artdmx(pkt, 3, data8, 8);
  DropCeilingArtNet::ParseResult r = DropCeilingArtNet::parsePacket(pkt, n, config, lv);
  CHECK(r.type == DropCeilingArtNet::PacketDmx && r.changed);
  CHECK(lv[0] == 0 && lv[1] == 257 && lv[3] == 128 * 257 && lv[5] == 65535 && lv[7] == 9 * 257);
  r = DropCeilingArtNet::parsePacket(pkt, n, config, lv);
  CHECK(r.type == DropCeilingArtNet::PacketDmx && !r.changed);

  // Another board's universe is ignored.
  n = artdmx(pkt, 4, data8, 8);
  CHECK(DropCeilingArtNet::parsePacket(pkt, n, config, lv).type == DropCeilingArtNet::PacketIgnored);

  // A short frame changes only the channels it carries. A length past the packet is clipped.
  uint16_t before = lv[4];
  uint8_t two[2] = {50, 60};
  n = artdmx(pkt, 3, two, 2);
  r = DropCeilingArtNet::parsePacket(pkt, n, config, lv);
  CHECK(lv[0] == 50 * 257 && lv[1] == 60 * 257 && lv[4] == before);
  pkt[16] = 0x02; pkt[17] = 0x00;  // claims 512 slots, carries 2
  r = DropCeilingArtNet::parsePacket(pkt, n, config, lv);
  CHECK(r.type == DropCeilingArtNet::PacketDmx && lv[4] == before);

  // 512 slots, as most consoles send.
  uint8_t full[512] = {0};
  full[0] = 200; full[7] = 100;
  n = artdmx(pkt, 3, full, 512);
  DropCeilingArtNet::parsePacket(pkt, n, config, lv);
  CHECK(lv[0] == 200 * 257 && lv[7] == 100 * 257);

  // 16-bit: slots 2c-1 high, 2c low.
  config.artnetResolution = 16;
  uint8_t data16[16];
  for (int c = 0; c < 8; c++) { data16[2 * c] = 0x12 + c; data16[2 * c + 1] = 0x34; }
  data16[14] = 0xFF; data16[15] = 0xFF;
  n = artdmx(pkt, 3, data16, 16);
  DropCeilingArtNet::parsePacket(pkt, n, config, lv);
  CHECK(lv[0] == 0x1234 && lv[6] == 0x1834 && lv[7] == 0xFFFF);
  CHECK(DropCeilingLevels::toCode(lv[0], 10) == (0x1234 >> 6));
  // An 8-slot frame in 16-bit mode reaches only channels 1-4.
  uint16_t ch5 = lv[4];
  n = artdmx(pkt, 3, data8, 8);
  DropCeilingArtNet::parsePacket(pkt, n, config, lv);
  CHECK(lv[0] == ((0 << 8) | 1) && lv[4] == ch5);

  // Start channel offsets the slots.
  config.artnetResolution = 8;
  config.artnetStartChannel = 2;
  n = artdmx(pkt, 3, data8, 8);
  DropCeilingArtNet::parsePacket(pkt, n, config, lv);
  CHECK(lv[0] == 2 * 257 && lv[5] == 9 * 257);

  // Not Art-Net, and truncated packets.
  CHECK(DropCeilingArtNet::parsePacket((const uint8_t *)"hello world......", 17, config, lv).type == DropCeilingArtNet::PacketIgnored);
  CHECK(DropCeilingArtNet::parsePacket(pkt, 12, config, lv).type == DropCeilingArtNet::PacketIgnored);

  // ArtPoll
  uint8_t poll[14] = {'A', 'r', 't', '-', 'N', 'e', 't', 0, 0x00, 0x20, 0, 14, 0, 0};
  CHECK(DropCeilingArtNet::parsePacket(poll, 14, config, lv).type == DropCeilingArtNet::PacketPoll);
}

static void pollReply() {
  DropCeilingConfig config;
  config.artnetUniverse = 2;
  config.nodeShortName = "DC2-K2";
  config.nodeLongName = "Drop Ceiling V2 DC2-K2";
  config.firmwareVersionHi = 2;
  config.firmwareVersionLo = 0;
  uint8_t mac[6] = {0x02, 0xDC, 0x20, 0x27, 0x00, 0x02};
  memcpy(config.mac, mac, 6);
  config.useDhcp = false;
  config.nodeReport = "up=12 fr=345 e=0 rs=PWR r=8 LIVE";
  uint8_t reply[239];
  CHECK(DropCeilingArtNet::buildPollReply(reply, sizeof(reply), config, IPAddress(10, 42, 0, 202), true, 7) == 239);
  CHECK(memcmp(reply, "Art-Net\0", 8) == 0 && reply[8] == 0x00 && reply[9] == 0x21);
  CHECK(reply[10] == 10 && reply[13] == 202);
  CHECK(reply[14] == 0x36 && reply[15] == 0x19);  // 6454 little-endian
  CHECK(reply[16] == 2 && reply[17] == 0);
  CHECK(strcmp((const char *)&reply[26], "DC2-K2") == 0);
  CHECK(strcmp((const char *)&reply[108], "#0001 [0007] up=12 fr=345 e=0 rs=PWR r=8 LIVE") == 0);
  CHECK(reply[173] == 1 && reply[174] == 0x80 && reply[182] == 0x80 && reply[190] == 2);
  CHECK(memcmp(&reply[201], mac, 6) == 0);
  CHECK(reply[207] == 10 && reply[210] == 202 && reply[211] == 1);
  CHECK(reply[212] == 0x0C);
  CHECK(DropCeilingArtNet::buildPollReply(reply, 100, config, IPAddress(), false, 0) == 0);
  // The report never overruns its 64 bytes.
  char longText[200];
  memset(longText, 'x', sizeof(longText) - 1);
  longText[sizeof(longText) - 1] = 0;
  config.nodeReport = longText;
  DropCeilingArtNet::buildPollReply(reply, sizeof(reply), config, IPAddress(10, 42, 0, 202), false, 12345);
  CHECK(reply[171] == 0 && reply[172] == 0 && reply[182] == 0);
}

int main() {
  levels();
  fader();
  parse();
  pollReply();
  if (failures) {
    printf("%d failed\n", failures);
    return 1;
  }
  printf("all passed\n");
  return 0;
}
