#ifndef _VEC_H
#define _VEC_H
#include<stdint.h>
#include<math.h>
#include "common.h"
#define Vec3_ADD_INPLACE(a,b) a.x+=b.x; a.y+=b.y; a.z+=b.z
typedef struct Vec2{
	num x;
	num y;
} Vec2;
typedef struct Vec3{
	num x;
	num y;
	num z;
} Vec3;
typedef struct Matrix{
	int rows;
	int cols;
	num **m;
} Matrix;
inline Vec2 Vec2_create(num x, num y) { return (Vec2){x, y}; }

inline Vec3 Vec3_create(num x, num y, num z) {
  return (Vec3){x, y, z};
}

inline Vec3 Vec3_cross(Vec3 v1, Vec3 v2) {
  return Vec3_create(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                     v1.x * v2.y - v1.y * v2.x);
}

inline num Vec3_dot(Vec3 v1, Vec3 v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vec3 Vec3_add(Vec3 v1, Vec3 v2) {
  return Vec3_create(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

inline Vec3 Vec3_neg(Vec3 v1) { return Vec3_create(-v1.x, -v1.y, -v1.z); }

inline Vec3 Vec3_sub(Vec3 v1, Vec3 v2) {
  return Vec3_create(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline Vec3 Vec3_mul(Vec3 v1, num a) {
  return Vec3_create(v1.x * a, v1.y * a, v1.z * a);
}

inline Vec3 Vec3_div(Vec3 v1, num a) {
  return Vec3_create(v1.x / a, v1.y / a, v1.z / a);
}

inline num Vec3_length(Vec3 v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vec3 Vec3_normalized(Vec3 v) { return Vec3_div(v, Vec3_length(v)); }

inline Vec3 Vec3_from_matrix(Matrix mat) {
  return Vec3_create(mat.m[0][0] / mat.m[3][0], mat.m[1][0] / mat.m[3][0],
                     mat.m[2][0] / mat.m[3][0]);
}

inline Vec3 Vec3_from_matrix3(Matrix mat) {
  return Vec3_create(mat.m[0][0], mat.m[1][0], mat.m[2][0]);
}
inline uint32_t _pack(int r, int g, int b) {
  return 0xff | b << 8 | g << 16 | r << 24;
}
inline uint32_t Vec3_phong(Vec3 v1, num a, num lo, num hi){
    return _pack(clamp(v1.x * a + AMBIENT, lo, hi),
                     clamp(v1.y * a + AMBIENT, lo, hi),
                     clamp(v1.z * a + AMBIENT, lo, hi)
            );
}
inline uint32_t Vec3_pack_color(Vec3 v){
    return _pack(v.x, v.y, v.z);
}

inline Vec3 Vec3_normal_from_color(Vec3 color) {
  return (Vec3){-(color.x/(num)127.5)+1, -(color.y/(num)127.5)+1, -(color.z/(num)127.5)+1};
}

Matrix Matrix_empty(int rows, int cols);
Matrix Matrix_identity(int size);
Matrix Matrix_matmul(Matrix m1, Matrix m2);
Matrix Matrix_projection(num camz);
Matrix Matrix_viewport(num x, num y, num w, num h, num d);
Matrix Matrix_from_vector(Vec3 v);
Matrix Matrix_rotz(num theta);
Matrix Matrix_roty(num theta);
Matrix Matrix_rotx(num theta);
Matrix Matrix_inverse(Matrix matrix);
Matrix Matrix_translation(Vec3 v);
void Matrix_dealloc(Matrix mat);
Vec3 Vec3_copy(Vec3 v);
Vec3 Vec3_transform(Vec3 v, Matrix mat);
Vec3 Vec3_transform3(Vec3 v, Matrix mat);
void Vec3_set_item(Vec3 v, int i, num a);
num Vec3_get_item(Vec3 v, int i);
#endif
