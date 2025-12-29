#include "crender.h"
#define cr_TRIANGLE_MAP(tri, func, ...)                                        \
  cr_Triangle_create(func(tri.v0, __VA_ARGS__), func(tri.v1, __VA_ARGS__),     \
                     func(tri.v2, __VA_ARGS__))
cr_Triangle cr_Triangle_transform(cr_Triangle tri, cr_Matrix transform) {
  return cr_TRIANGLE_MAP(tri, cr_Vec3_transform, transform);
}
cr_Triangle cr_Triangle_transform3(cr_Triangle tri, cr_Matrix transform) {
  return cr_TRIANGLE_MAP(tri, cr_Vec3_transform3, transform);
}
cr_Triangle cr_Triangle_transform4(cr_Triangle tri, cr_Matrix transform,
                                   cr_Vec3 *ws) {
  cr_Triangle out;
  cr_Vec4 v40, v41, v42;
  v40 = cr_Vec4_transform(tri.v0, transform);
  v41 = cr_Vec4_transform(tri.v1, transform);
  v42 = cr_Vec4_transform(tri.v2, transform);
  out.v0 = (cr_Vec3){v40.x / v40.w, v40.y / v40.w, v40.z / v40.w};
  out.v1 = (cr_Vec3){v41.x / v41.w, v41.y / v41.w, v41.z / v41.w};
  out.v2 = (cr_Vec3){v42.x / v42.w, v42.y / v42.w, v42.z / v42.w};
  *ws = (cr_Vec3){v40.w, v41.w, v42.w};
  return out;
}
cr_Triangle cr_Triangle_create(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2) {
  cr_Triangle triangle;

  triangle.v0 = v0;
  triangle.v1 = v1;
  triangle.v2 = v2;
  return triangle;
}
#undef cr_TRIANGLE_MAP // not needed anywhere else
