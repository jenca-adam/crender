#ifndef _CORE_H
#define _CORE_H
#include"vec.h"
#include"tri.h"
#include "common.h"
#include<math.h>
num apow(num x, uint8_t y);
static inline Vec3 trinterpolate(Triangle tri, Vec3 bary) {
  num bx = bary.x, by = bary.y, bz = bary.z;
  return (Vec3){tri.v0.x * bx + tri.v1.x * by + tri.v2.x * bz,
                tri.v0.y * bx + tri.v1.y * by + tri.v2.y * bz,
                tri.v0.z * bx + tri.v1.z * by + tri.v2.z * bz};
}
static inline Vec3 barycentric(Vec3 v0, Vec3 v1, Vec3 v2, num px, num py,
                        num denom) {
  num x0 = v0.x, y0 = v0.y;
  num x1 = v1.x, y1 = v1.y;
  num x2 = v2.x, y2 = v2.y;
  if (fabs(denom) < 1e-12) {
    return (Vec3){-1.0, 1.0, 1.0};
  }

  num w1 = ((x2 - x0) * (py - y0) - (y2 - y0) * (px - x0)) * denom;

  num w2 = ((y1 - y0) * (px - x0) - (x1 - x0) * (py - y0)) * denom;

  return (Vec3){(num)1.0 - w1 - w2, w1, w2};
}

static inline num fmin3(num x, num y, num z) {
  return fmin(x, fmin(y, z));
}
static inline num fmax3(num x, num y, num z) {
  return fmax(x, fmax(y, z));
}

static inline Vec3 bary_precomp(num px, num py,
                         Vec3 base, Vec3 deltax, Vec3 deltay) {
  return Vec3_add(Vec3_add(base, Vec3_mul(deltax, px)), Vec3_mul(deltay, py));
}

#endif
