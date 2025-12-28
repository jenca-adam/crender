#include "tri.h"
#define TRIANGLE_MAP(tri, func, ...)                                           \
  Triangle_create(func(tri.v0, __VA_ARGS__), func(tri.v1, __VA_ARGS__),        \
                  func(tri.v2, __VA_ARGS__))
Triangle Triangle_transform(Triangle tri, Matrix transform) {
  return TRIANGLE_MAP(tri, Vec3_transform, transform);
}
Triangle Triangle_transform3(Triangle tri, Matrix transform) {
  return TRIANGLE_MAP(tri, Vec3_transform3, transform);
}
Triangle Triangle_transform4(Triangle tri, Matrix transform, Vec3 *ws) {
  Triangle out;
  Vec4 v40, v41, v42;
  v40 = Vec4_transform(tri.v0, transform);
  v41 = Vec4_transform(tri.v1, transform);
  v42 = Vec4_transform(tri.v2, transform);
  out.v0 = (Vec3){v40.x / v40.w, v40.y / v40.w, v40.z / v40.w};
  out.v1 = (Vec3){v41.x / v41.w, v41.y / v41.w, v41.z / v41.w};
  out.v2 = (Vec3){v42.x / v42.w, v42.y / v42.w, v42.z / v42.w};
  *ws = (Vec3){v40.w, v41.w, v42.w};
  return out;
}
Triangle Triangle_create(Vec3 v0, Vec3 v1, Vec3 v2) {
  Triangle triangle;

  triangle.v0 = v0;
  triangle.v1 = v1;
  triangle.v2 = v2;
  return triangle;
}
