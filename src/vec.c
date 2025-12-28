#include "vec.h"
#include "common.h"
#include "core.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

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
  matrix.m = malloc(rows * sizeof(num *));
  for (int i = 0; i < rows; i++) {
    matrix.m[i] = calloc(cols, sizeof(num));
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

Matrix Matrix_projection(num camz) {
  Matrix mat = Matrix_identity(4);
  mat.m[3][2] = -1.0 / camz;
  return mat;
}

Matrix Matrix_viewport(num x, num y, num w, num h, num d) {
  Matrix mat = Matrix_identity(4);
  mat.m[0][3] = x + w / 2.0;
  mat.m[1][3] = y + h / 2.0;
  mat.m[2][3] = d / 2.0;
  mat.m[2][2] = d / 2.0;
  mat.m[0][0] = w / 2.0;
  mat.m[1][1] = h / 2.0;
  return mat;
}
Matrix Matrix_translation(Vec3 v) {
  Matrix mat = Matrix_identity(4);
  mat.m[0][3] = v.x;
  mat.m[1][3] = v.y;
  mat.m[2][3] = v.z;
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

Matrix Matrix_rotz(num theta) {
  Matrix mat = Matrix_identity(4);
  mat.m[0][0] = cos(theta);
  mat.m[0][1] = -sin(theta);
  mat.m[1][0] = sin(theta);
  mat.m[1][1] = cos(theta);
  return mat;
}

Matrix Matrix_roty(num theta) {
  Matrix mat = Matrix_identity(4);
  mat.m[0][0] = cos(theta);
  mat.m[2][0] = -sin(theta);
  mat.m[0][2] = sin(theta);
  mat.m[2][2] = cos(theta);
  return mat;
}

Matrix Matrix_rotx(num theta) {
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

num Matrix_determinant(Matrix matrix) {
  int n = matrix.rows;
  if (n == 1) {
    return matrix.m[0][0];
  }

  num det = 0.0;
  for (int j = 0; j < n; j++) {
    Matrix minor = Matrix_get_minor(matrix, 0, j);
    num cofactor = matrix.m[0][j] * Matrix_determinant(minor);
    Matrix_dealloc(minor);
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
      num cofactor = Matrix_determinant(minor);
      Matrix_dealloc(minor);
      if ((i + j) % 2 != 0) {
        cofactor = -cofactor;
      }
      adjugate.m[j][i] = cofactor;
    }
  }
  return adjugate;
}

Matrix Matrix_inverse4(Matrix matrix) {
  Matrix inv = Matrix_empty(4, 4);
  num **m = matrix.m;
  num det = Matrix_determinant(matrix);
  if (det == 0) {
    return inv;
  }

  num inv_det = 1.0 / det;
  inv.m[0][0] = (m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                 m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) +
                 m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1])) *
                inv_det;

  inv.m[0][1] = -(m[0][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                  m[0][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) +
                  m[0][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1])) *
                inv_det;

  inv.m[0][2] = (m[0][1] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) -
                 m[0][2] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) +
                 m[0][3] * (m[1][1] * m[3][2] - m[1][2] * m[3][1])) *
                inv_det;

  inv.m[0][3] = -(m[0][1] * (m[1][2] * m[2][3] - m[1][3] * m[2][2]) -
                  m[0][2] * (m[1][1] * m[2][3] - m[1][3] * m[2][1]) +
                  m[0][3] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])) *
                inv_det;

  inv.m[1][0] = -(m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                  m[1][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                  m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0])) *
                inv_det;

  inv.m[1][1] = (m[0][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                 m[0][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                 m[0][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0])) *
                inv_det;

  inv.m[1][2] = -(m[0][0] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) -
                  m[0][2] * (m[1][0] * m[3][3] - m[1][3] * m[3][0]) +
                  m[0][3] * (m[1][0] * m[3][2] - m[1][2] * m[3][0])) *
                inv_det;

  inv.m[1][3] = (m[0][0] * (m[1][2] * m[2][3] - m[1][3] * m[2][2]) -
                 m[0][2] * (m[1][0] * m[2][3] - m[1][3] * m[2][0]) +
                 m[0][3] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])) *
                inv_det;

  inv.m[2][0] = (m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
                 m[1][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                 m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])) *
                inv_det;

  inv.m[2][1] = -(m[0][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
                  m[0][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                  m[0][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])) *
                inv_det;

  inv.m[2][2] = (m[0][0] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) -
                 m[0][1] * (m[1][0] * m[3][3] - m[1][3] * m[3][0]) +
                 m[0][3] * (m[1][0] * m[3][1] - m[1][1] * m[3][0])) *
                inv_det;

  inv.m[2][3] = -(m[0][0] * (m[1][1] * m[2][3] - m[1][3] * m[2][1]) -
                  m[0][1] * (m[1][0] * m[2][3] - m[1][3] * m[2][0]) +
                  m[0][3] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])) *
                inv_det;

  inv.m[3][0] = -(m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
                  m[1][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) +
                  m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])) *
                inv_det;

  inv.m[3][1] = (m[0][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
                 m[0][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) +
                 m[0][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])) *
                inv_det;

  inv.m[3][2] = -(m[0][0] * (m[1][1] * m[3][2] - m[1][2] * m[3][1]) -
                  m[0][1] * (m[1][0] * m[3][2] - m[1][2] * m[3][0]) +
                  m[0][2] * (m[1][0] * m[3][1] - m[1][1] * m[3][0])) *
                inv_det;

  inv.m[3][3] = (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                 m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                 m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])) *
                inv_det;
  return inv;
}
Matrix Matrix_inverse(Matrix matrix) {
  int n = matrix.rows;

  if (n != matrix.cols) {
    return Matrix_empty(0, 0); // Return an empty matrix
  }
  if (n == 4) {
    return Matrix_inverse4(matrix);
  }
  num det = Matrix_determinant(matrix);
  if (fabs(det) < 1e-9) {
    return Matrix_empty(0, 0); // Return an empty matrix
  }

  Matrix adjugate = Matrix_adjugate(matrix);
  Matrix inverse = Matrix_empty(n, n);
  Matrix_dealloc(adjugate);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      inverse.m[i][j] = adjugate.m[i][j] / det;
    }
  }

  return inverse;
}

Vec3 Vec3_transform(Vec3 v, Matrix m) {
  num x = v.x, y = v.y, z = v.z;
  num w = 1.0;

  num nx = m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z + m.m[0][3] * w;
  num ny = m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z + m.m[1][3] * w;
  num nz = m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z + m.m[2][3] * w;
  num nw = m.m[3][0] * x + m.m[3][1] * y + m.m[3][2] * z + m.m[3][3] * w;

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
Vec4 Vec4_transform(Vec3 v, Matrix m) {
  num x = v.x, y = v.y, z = v.z, w = 1.0;

  return (Vec4){m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z + m.m[0][3] * w,
                m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z + m.m[1][3] * w,
                m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z + m.m[2][3] * w,
                m.m[3][0] * x + m.m[3][1] * y + m.m[3][2] * z + m.m[3][3] * w};
}

void Vec3_setItem(Vec3 *v, int i, num a) {
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
