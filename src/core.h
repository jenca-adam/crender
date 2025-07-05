#ifndef _CORE_H
#define _CORE_H
#include"vec.h"
#include"tri.h"
Vec3 barycentric(Vec3 v0, Vec3 v1, Vec3 v2, double x, double y);
Vec3 trinterpolate(Triangle tri, Vec3 bary);
double fmin3(double x, double y, double z);
double fmax3(double x, double y, double z);
double clamp(double a, double lo, double hi);
#endif
