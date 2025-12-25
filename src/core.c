#include "core.h"
#include <math.h>
#include <stdlib.h>
inline Vec3 barycentric(Vec3 v0, Vec3 v1, Vec3 v2, double px, double py) {
  double x0 = v0.x, y0 = v0.y;
  double x1 = v1.x, y1 = v1.y;
  double x2 = v2.x, y2 = v2.y;

  double denom = 1 / ((x2 - x0) * (y1 - y0) - (x1 - x0) * (y2 - y0));

  if (fabs(denom) < 1e-12) {
    return (Vec3){-1.0, 1.0, 1.0};
  }

  double w1 = ((x2 - x0) * (py - y0) - (y2 - y0) * (px - x0)) * denom;

  double w2 = ((y1 - y0) * (px - x0) - (x1 - x0) * (py - y0)) * denom;

  return (Vec3){1.0 - w1 - w2, w1, w2};
}
inline Vec3 trinterpolate(Triangle tri, Vec3 bary) {
  return (Vec3){tri.v0.x * bary.x + tri.v1.x * bary.y + tri.v2.x * bary.z,
                tri.v0.y * bary.x + tri.v1.y * bary.y + tri.v2.y * bary.z,
                tri.v0.z * bary.x + tri.v1.z * bary.y + tri.v2.z * bary.z};
}
double fmin3(double x, double y, double z) { return fmin(x, fmin(y, z)); }
double fmax3(double x, double y, double z) { return fmax(x, fmax(y, z)); }
double clamp(double a, double lo, double hi) {
  return (a > hi) ? hi : ((a < lo) ? lo : a);
}
