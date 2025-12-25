#include "common.h"
#include "core.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
inline Vec2 Vec2_create(double x, double y) { return (Vec2){x, y}; }

inline Vec3 Vec3_create(double x, double y, double z) {
  return (Vec3){x, y, z};
}

inline Vec3 Vec3_cross(Vec3 v1, Vec3 v2) {
  return Vec3_create(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                     v1.x * v2.y - v1.y * v2.x);
}

inline double Vec3_dot(Vec3 v1, Vec3 v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vec3 Vec3_add(Vec3 v1, Vec3 v2) {
  return Vec3_create(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

inline Vec3 Vec3_neg(Vec3 v1) { return Vec3_create(-v1.x, -v1.y, -v1.z); }

inline Vec3 Vec3_sub(Vec3 v1, Vec3 v2) {
  return Vec3_create(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline Vec3 Vec3_mul(Vec3 v1, double a) {
  return Vec3_create(v1.x * a, v1.y * a, v1.z * a);
}

inline Vec3 Vec3_phong(Vec3 v1, double a, double lo, double hi) {
  return Vec3_create(clamp(v1.x * a + AMBIENT, lo, hi),
                     clamp(v1.y * a + AMBIENT, lo, hi),
                     clamp(v1.z * a + AMBIENT, lo, hi));
}

inline Vec3 Vec3_div(Vec3 v1, double a) {
  return Vec3_create(v1.x / a, v1.y / a, v1.z / a);
}

inline double Vec3_length(Vec3 v) {
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
inline uint32_t Vec3_pack_color(Vec3 vec) {
  return 0xff | (int)vec.z << 8 | (int)vec.y << 16 | (int)vec.x << 24;
}
Vec3 Vec3_normal_from_color(Vec3 color) {
  Vec3 n_0to1 = Vec3_div(color, 127.5);
  Vec3 n_minus1to1 = Vec3_create(-n_0to1.x + 1, -n_0to1.y + 1, -n_0to1.z + 1);
  return n_minus1to1;
}

Vec3 Vec3_copy(Vec3 v) { return Vec3_create(v.x, v.y, v.z); }
void Matrix_dealloc(Matrix matrix) {
  for (int i = 0; i < matrix.rows; i++) {
    free(matrix.m[i]);
  }
  free(matrix.m);
}
Matrix Matrix_empty(int rows, int cols) {
  Matrix matrix;
  matrix.rows = rows;
  matrix.cols = cols;
  matrix.m = malloc(rows * sizeof(double *));
  for (int i = 0; i < rows; i++) {
    matrix.m[i] = calloc(cols, sizeof(double));
    for (int j = 0; j < cols; j++) {
      matrix.m[i][j] = 0.0; // Initialize to zero
    }
  }
  return matrix;
}

Matrix Matrix_identity(int size) {
  Matrix matrix = Matrix_empty(size, size);
  for (int i = 0; i < size; i++) {
    matrix.m[i][i] = 1.0;
  }
  return matrix;
}

Matrix Matrix_matmul(Matrix m1, Matrix m2) {
  int rows = m1.rows, cols = m2.cols;
  Matrix result = Matrix_empty(rows, cols);

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      for (int k = 0; k < m1.cols; k++) {
        result.m[i][j] += m1.m[i][k] * m2.m[k][j];
      }
    }
  }
  return result;
}

Matrix Matrix_projection(double camz) {
  Matrix mat = Matrix_identity(4);
  mat.m[3][2] = -1.0 / camz;
  return mat;
}

Matrix Matrix_viewport(double x, double y, double w, double h, double d) {
  Matrix mat = Matrix_identity(4);
  mat.m[0][3] = x + w / 2.0;
  mat.m[1][3] = y + h / 2.0;
  mat.m[2][3] = d / 2.0;
  mat.m[2][2] = d / 2.0;
  mat.m[0][0] = w / 2.0;
  mat.m[1][1] = h / 2.0;
  return mat;
}

Matrix Matrix_from_vector(Vec3 vec3) {
  Matrix mat = Matrix_empty(4, 1);
  mat.m[0][0] = vec3.x;
  mat.m[1][0] = vec3.y;
  mat.m[2][0] = vec3.z;
  mat.m[3][0] = 1.0;
  return mat;
}

Matrix Matrix_rotz(double theta) {
  Matrix mat = Matrix_identity(4);
  mat.m[0][0] = cos(theta);
  mat.m[0][1] = -sin(theta);
  mat.m[1][0] = sin(theta);
  mat.m[1][1] = cos(theta);
  return mat;
}

Matrix Matrix_roty(double theta) {
  Matrix mat = Matrix_identity(4);
  mat.m[0][0] = cos(theta);
  mat.m[2][0] = -sin(theta);
  mat.m[0][2] = sin(theta);
  mat.m[2][2] = cos(theta);
  return mat;
}

Matrix Matrix_rotx(double theta) {
  Matrix mat = Matrix_identity(4);
  mat.m[1][1] = cos(theta);
  mat.m[1][2] = -sin(theta);
  mat.m[2][1] = sin(theta);
  mat.m[2][2] = cos(theta);
  return mat;
}

Matrix Matrix_get_minor(Matrix matrix, int row, int col) {
  int n = matrix.rows;
  Matrix minor = Matrix_empty(n - 1, n - 1);

  int minor_row = 0, minor_col;
  for (int i = 0; i < n; i++) {
    if (i == row)
      continue;
    minor_col = 0;
    for (int j = 0; j < n; j++) {
      if (j == col)
        continue;
      minor.m[minor_row][minor_col] = matrix.m[i][j];
      minor_col++;
    }
    minor_row++;
  }
  return minor;
}

double Matrix_determinant(Matrix matrix) {
  int n = matrix.rows;
  if (n == 1) {
    return matrix.m[0][0];
  }

  double det = 0.0;
  for (int j = 0; j < n; j++) {
    Matrix minor = Matrix_get_minor(matrix, 0, j);
    double cofactor = matrix.m[0][j] * Matrix_determinant(minor);
    if (j % 2 != 0) {
      cofactor = -cofactor;
    }
    det += cofactor;
  }
  return det;
}

Matrix Matrix_adjugate(Matrix matrix) {
  int n = matrix.rows;
  if (n != matrix.cols) {
    return Matrix_empty(0, 0); // Return an empty matrix
  }

  Matrix adjugate = Matrix_empty(n, n);

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      Matrix minor = Matrix_get_minor(matrix, i, j);
      double cofactor = Matrix_determinant(minor);
      if ((i + j) % 2 != 0) {
        cofactor = -cofactor;
      }
      adjugate.m[j][i] = cofactor;
    }
  }
  return adjugate;
}

Matrix Matrix_inverse(Matrix matrix) {
  int n = matrix.rows;
  if (n != matrix.cols) {
    return Matrix_empty(0, 0); // Return an empty matrix
  }

  double det = Matrix_determinant(matrix);
  if (fabs(det) < 1e-9) {
    return Matrix_empty(0, 0); // Return an empty matrix
  }

  Matrix adjugate = Matrix_adjugate(matrix);
  Matrix inverse = Matrix_empty(n, n);

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      inverse.m[i][j] = adjugate.m[i][j] / det;
    }
  }

  return inverse;
}

Vec3 Vec3_transform(Vec3 v, Matrix m) {
  double x = v.x, y = v.y, z = v.z;
  double w = 1.0;

  double nx = m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z + m.m[0][3] * w;
  double ny = m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z + m.m[1][3] * w;
  double nz = m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z + m.m[2][3] * w;
  double nw = m.m[3][0] * x + m.m[3][1] * y + m.m[3][2] * z + m.m[3][3] * w;

  return Vec3_create(nx / nw, ny / nw, nz / nw);
}

Vec3 Vec3_transform3(Vec3 v, Matrix mat) {
  Matrix vmat = Matrix_from_vector(v);
  Matrix result = Matrix_matmul(mat, vmat);
  Vec3 out = Vec3_from_matrix3(result);
  Matrix_dealloc(vmat);
  Matrix_dealloc(result);
  return out;
}

void Vec3_setItem(Vec3 *v, int i, double a) {
  switch (i) {
  case 0:
    v->x = a;
    break;
  case 1:
    v->y = a;
    break;
  case 2:
    v->z = a;
    break;
  default:
    break;
  }
}
