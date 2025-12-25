#include "tri.h"
#include "core.h"
#include <stdlib.h>
#define TRIANGLE_MAP(tri, func, ...)                                           \
  Triangle_create(func(tri.v0, __VA_ARGS__), func(tri.v1, __VA_ARGS__),        \
                  func(tri.v2, __VA_ARGS__))
Triangle Triangle_transform(Triangle tri, Matrix transform) {
  return TRIANGLE_MAP(tri, Vec3_transform, transform);
}
Triangle Triangle_transform3(Triangle tri, Matrix transform) {
  return TRIANGLE_MAP(tri, Vec3_transform3, transform);
}
Triangle Triangle_create(Vec3 v0, Vec3 v1, Vec3 v2) {
  Triangle triangle;

  triangle.v0 = v0;
  triangle.v1 = v1;
  triangle.v2 = v2;
  return triangle;
}
