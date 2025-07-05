#include "core.h"
#include <math.h>
#include <stdlib.h>
Vec3 barycentric(Vec3 v0, Vec3 v1, Vec3 v2, double x, double y) {
  Vec3 a1 = Vec3_create(v2.x - v0.x, v1.x - v0.x, v0.x - x);
  Vec3 a2 = Vec3_create(v2.y - v0.y, v1.y - v0.y, v0.y - y);
  Vec3 u = Vec3_cross(a1, a2);
  if (fabs(u.z) < 1) {
    return Vec3_create(-1, 1, 1);
  }
  Vec3 out = Vec3_create(1 - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
  return out;
}
Vec3 trinterpolate(Triangle tri, Vec3 bary) {
  Vec3 c0 = Vec3_mul(tri.v0, bary.x);
  Vec3 c1 = Vec3_mul(tri.v1, bary.y);

  Vec3 c2 = Vec3_mul(tri.v2, bary.z);
  Vec3 c12 = Vec3_add(c1, c2);
  Vec3 out = Vec3_add(c0, c12);
  return out;
}
double fmin3(double x, double y, double z) { return fmin(x, fmin(y, z)); }
double fmax3(double x, double y, double z) { return fmax(x, fmax(y, z)); }
double clamp(double a, double lo, double hi) {
  return (a > hi) ? hi : ((a < lo) ? lo : a);
}
