#ifndef IPADDRESS_STUB_H
#define IPADDRESS_STUB_H
#include <stdint.h>
class IPAddress {
public:
  IPAddress() : b{0, 0, 0, 0} {}
  IPAddress(uint8_t a, uint8_t c, uint8_t d, uint8_t e) : b{a, c, d, e} {}
  uint8_t operator[](int i) const { return b[i]; }
private:
  uint8_t b[4];
};
#endif
