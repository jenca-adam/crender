#ifndef _CORE_H
#define _CORE_H
#include"vec.h"
#include"tri.h"
#include "common.h"
#include<math.h>
double apow(double x, uint8_t y);
inline Vec3 trinterpolate(Triangle tri, Vec3 bary) {
  double bx = bary.x, by = bary.y, bz = bary.z;
  return (Vec3){tri.v0.x * bx + tri.v1.x * by + tri.v2.x * bz,
                tri.v0.y * bx + tri.v1.y * by + tri.v2.y * bz,
                tri.v0.z * bx + tri.v1.z * by + tri.v2.z * bz};
}
inline Vec3 barycentric(Vec3 v0, Vec3 v1, Vec3 v2, double px, double py,
                        double denom) {
  double x0 = v0.x, y0 = v0.y;
  double x1 = v1.x, y1 = v1.y;
  double x2 = v2.x, y2 = v2.y;
  if (fabs(denom) < 1e-12) {
    return (Vec3){-1.0, 1.0, 1.0};
  }

  double w1 = ((x2 - x0) * (py - y0) - (y2 - y0) * (px - x0)) * denom;

  double w2 = ((y1 - y0) * (px - x0) - (x1 - x0) * (py - y0)) * denom;

  return (Vec3){1.0 - w1 - w2, w1, w2};
}

inline double fmin3(double x, double y, double z) {
  return fmin(x, fmin(y, z));
}
inline double fmax3(double x, double y, double z) {
  return fmax(x, fmax(y, z));
}

inline Vec3 bary_precomp(double px, double py,
                         Vec3 base, Vec3 deltax, Vec3 deltay) {
  return Vec3_add(Vec3_add(base, Vec3_mul(deltax, px)), Vec3_mul(deltay, py));
}

#endif
