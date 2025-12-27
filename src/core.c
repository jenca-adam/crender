#include "core.h"
num apow(num x, uint8_t n) {
  num p[8];
  p[0] = x;
  for (int i = 1; i < 8; i++)
    p[i] = p[i - 1] * p[i - 1];

  num r = 1.0;
  for (int i = 0; i < 8; i++)
    if (n & (1 << i))
      r *= p[i];

  return r;
}
