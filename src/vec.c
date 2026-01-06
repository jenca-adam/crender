#include "crender.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
cr_Vec3 cr_Vec3_copy(cr_Vec3 v) { return cr_Vec3_create(v.x, v.y, v.z); }
void cr_Matrix_dealloc(cr_Matrix *matrix) {
  if (!matrix || !matrix->valid)
    return;
  for (int i = 0; i < matrix->rows; i++) {
    free(matrix->m[i]);
  }
  free(matrix->m);
  *matrix = (cr_Matrix){0};
}
cr_Matrix cr_Matrix_empty(int rows, int cols) {
  cr_Matrix matrix;
  matrix.rows = rows;
  matrix.cols = cols;
  matrix.valid = true;
  matrix.m = malloc(rows * sizeof(cr_num *));
  for (int i = 0; i < rows; i++) {
    matrix.m[i] = calloc(cols, sizeof(cr_num));
    for (int j = 0; j < cols; j++) {
      matrix.m[i][j] = 0.0; // Initialize to zero
    }
  }
  return matrix;
}

cr_Matrix cr_Matrix_identity(int size) {
  cr_Matrix matrix = cr_Matrix_empty(size, size);
  for (int i = 0; i < size; i++) {
    matrix.m[i][i] = 1.0;
  }
  return matrix;
}
cr_Matrix cr_Matrix_clone(cr_Matrix m) {
  cr_Matrix copy = cr_Matrix_empty(m.rows, m.cols);
  for (int i = 0; i < m.rows; i++)
    for (int j = 0; j < m.cols; j++)
      copy.m[i][j] = m.m[i][j];
  return copy;
}
cr_Matrix cr_Matrix_matmul(cr_Matrix m1, cr_Matrix m2) {

  int rows = m1.rows, cols = m2.cols;
  cr_ASSERT(m1.valid && m2.valid, ": check for early deallocation");
  cr_ASSERT(rows && cols, "");
  cr_Matrix result = cr_Matrix_empty(rows, cols);

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      for (int k = 0; k < m1.cols; k++) {
        result.m[i][j] += m1.m[i][k] * m2.m[k][j];
      }
    }
  }
  return result;
}

cr_Matrix cr_Matrix_projection(cr_num near, cr_num fov, cr_num aspect) {
  cr_Matrix mat = cr_Matrix_empty(4, 4);
  cr_num f = 1.0f / tanf(fov * 0.5f);
  mat.m[0][0] = f / aspect;
  mat.m[1][1] = f;
  mat.m[2][2] = -1.0f;
  mat.m[2][3] = -2.0f * near;
  mat.m[3][2] = -1.0f;
  return mat;
}

cr_Matrix cr_Matrix_viewport(cr_num x, cr_num y, cr_num w, cr_num h, cr_num d) {
  cr_Matrix mat = cr_Matrix_identity(4);
  mat.m[0][3] = x + w / 2.0;
  mat.m[1][3] = y + h / 2.0;
  mat.m[2][3] = d / 2.0;
  mat.m[2][2] = d / 2.0;
  mat.m[0][0] = w / 2.0;
  mat.m[1][1] = h / 2.0;
  return mat;
}
cr_Matrix cr_Matrix_translation(cr_Vec3 v) {
  cr_Matrix mat = cr_Matrix_identity(4);
  mat.m[0][3] = v.x;
  mat.m[1][3] = v.y;
  mat.m[2][3] = v.z;
  return mat;
}
cr_Matrix cr_Matrix_rotation(cr_Vec3 thetas) {
  cr_Matrix rotz = cr_Matrix_rotz(thetas.z);
  cr_Matrix roty = cr_Matrix_roty(thetas.y);
  cr_Matrix rotx = cr_Matrix_rotx(thetas.x);
  cr_Matrix rotzy = cr_Matrix_matmul(rotz, roty);
  cr_Matrix rot = cr_Matrix_matmul(rotzy, rotx);
  cr_Matrix_dealloc(&rotz);
  cr_Matrix_dealloc(&roty);
  cr_Matrix_dealloc(&rotx);
  cr_Matrix_dealloc(&rotzy);
  return rot;
}
cr_Matrix cr_Matrix_from_vector(cr_Vec3 v) {
  cr_Matrix mat = cr_Matrix_empty(4, 1);
  mat.m[0][0] = v.x;
  mat.m[1][0] = v.y;
  mat.m[2][0] = v.z;
  mat.m[3][0] = 1.0;
  return mat;
}
cr_Matrix cr_Matrix_from_vectors(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2) {
  cr_Matrix mat = cr_Matrix_empty(3, 3);
  mat.m[0][0] = v0.x;
  mat.m[0][1] = v0.y;
  mat.m[0][2] = v0.z;
  mat.m[1][0] = v1.x;
  mat.m[1][1] = v1.y;
  mat.m[1][2] = v1.z;
  mat.m[2][0] = v2.x;
  mat.m[2][1] = v2.y;
  mat.m[2][2] = v2.z;
  return mat;
}
cr_Matrix cr_Matrix_from_vectors4(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2) {
  cr_Matrix mat = cr_Matrix_identity(4);
  mat.m[0][0] = v0.x;
  mat.m[0][1] = v0.y;
  mat.m[0][2] = v0.z;
  mat.m[1][0] = v1.x;
  mat.m[1][1] = v1.y;
  mat.m[1][2] = v1.z;
  mat.m[2][0] = v2.x;
  mat.m[2][1] = v2.y;
  mat.m[2][2] = v2.z;
  return mat;
}
cr_Matrix cr_Matrix_from_vectors_col(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2) {
  cr_Matrix mat = cr_Matrix_empty(3, 3);
  mat.m[0][0] = v0.x;
  mat.m[1][0] = v0.y;
  mat.m[2][0] = v0.z;
  mat.m[0][1] = v1.x;
  mat.m[1][1] = v1.y;
  mat.m[2][1] = v1.z;
  mat.m[0][2] = v2.x;
  mat.m[1][2] = v2.y;
  mat.m[2][2] = v2.z;
  return mat;
}
cr_Matrix cr_Matrix_from_vectors_col4(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2) {
  cr_Matrix mat = cr_Matrix_identity(4);
  mat.m[0][0] = v0.x;
  mat.m[1][0] = v0.y;
  mat.m[2][0] = v0.z;
  mat.m[0][1] = v1.x;
  mat.m[1][1] = v1.y;
  mat.m[2][1] = v1.z;
  mat.m[0][2] = v2.x;
  mat.m[1][2] = v2.y;
  mat.m[2][2] = v2.z;
  return mat;
}

cr_Matrix cr_Matrix_rotz(cr_num theta) {
  cr_Matrix mat = cr_Matrix_identity(4);
  mat.m[0][0] = cos(theta);
  mat.m[0][1] = -sin(theta);
  mat.m[1][0] = sin(theta);
  mat.m[1][1] = cos(theta);
  return mat;
}

cr_Matrix cr_Matrix_roty(cr_num theta) {
  cr_Matrix mat = cr_Matrix_identity(4);
  mat.m[0][0] = cos(theta);
  mat.m[2][0] = -sin(theta);
  mat.m[0][2] = sin(theta);
  mat.m[2][2] = cos(theta);
  return mat;
}

cr_Matrix cr_Matrix_rotx(cr_num theta) {
  cr_Matrix mat = cr_Matrix_identity(4);
  mat.m[1][1] = cos(theta);
  mat.m[1][2] = -sin(theta);
  mat.m[2][1] = sin(theta);
  mat.m[2][2] = cos(theta);
  return mat;
}

cr_Matrix cr_Matrix_get_minor(cr_Matrix matrix, int row, int col) {
  int n = matrix.rows;
  cr_Matrix minor = cr_Matrix_empty(n - 1, n - 1);

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

cr_num cr_Matrix_determinant(cr_Matrix matrix) {
  int n = matrix.rows;
  if (n == 1) {
    return matrix.m[0][0];
  }

  cr_num det = 0.0;
  for (int j = 0; j < n; j++) {
    cr_Matrix minor = cr_Matrix_get_minor(matrix, 0, j);
    cr_num cofactor = matrix.m[0][j] * cr_Matrix_determinant(minor);
    cr_Matrix_dealloc(&minor);
    if (j % 2 != 0) {
      cofactor = -cofactor;
    }
    det += cofactor;
  }
  return det;
}

cr_Matrix cr_Matrix_adjugate(cr_Matrix matrix) {
  int n = matrix.rows;
  if (n != matrix.cols) {
    return cr_Matrix_empty(0, 0); // Return an empty matrix
  }

  cr_Matrix adjugate = cr_Matrix_empty(n, n);

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      cr_Matrix minor = cr_Matrix_get_minor(matrix, i, j);
      cr_num cofactor = cr_Matrix_determinant(minor);
      cr_Matrix_dealloc(&minor);
      if ((i + j) % 2 != 0) {
        cofactor = -cofactor;
      }
      adjugate.m[j][i] = cofactor;
    }
  }
  return adjugate;
}

cr_Matrix cr_Matrix_inverse4(cr_Matrix matrix) {
  cr_Matrix inv = cr_Matrix_empty(4, 4);
  cr_num **m = matrix.m;
  cr_num det = cr_Matrix_determinant(matrix);
  if (det == 0) {
    cr_Matrix_dealloc(&inv);
    return (cr_Matrix){0};
  }

  cr_num inv_det = 1.0 / det;
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
cr_Matrix cr_Matrix_inverse(cr_Matrix matrix) {
  int n = matrix.rows;

  if (n != matrix.cols) {
    return cr_Matrix_empty(0, 0); // Return an empty matrix
  }
  if (n == 4) {
    return cr_Matrix_inverse4(matrix);
  }
  cr_num det = cr_Matrix_determinant(matrix);
  if (fabs(det) < 1e-9) {
    return cr_Matrix_empty(0, 0); // Return an empty matrix
  }

  cr_Matrix adjugate = cr_Matrix_adjugate(matrix);
  cr_Matrix inverse = cr_Matrix_empty(n, n);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      inverse.m[i][j] = adjugate.m[i][j] / det;
    }
  }
  cr_Matrix_dealloc(&adjugate);
  return inverse;
}

cr_Matrix cr_Matrix_model_view(cr_Vec3 eye, cr_Vec3 center, cr_Vec3 up) {
  cr_Vec3 a = cr_Vec3_normalized(cr_Vec3_sub(eye, center));
  cr_Vec3 b = cr_Vec3_normalized(cr_Vec3_cross(up, a));
  cr_Vec3 c = cr_Vec3_normalized(cr_Vec3_cross(a, b));
  cr_Matrix m = cr_Matrix_from_vectors4(b, c, a);
  cr_Matrix trans = cr_Matrix_translation(cr_Vec3_neg(eye));
  cr_Matrix view = cr_Matrix_matmul(m, trans);
  return view;
}
cr_Vec3 cr_Vec3_transform(cr_Vec3 v, cr_Matrix m) {
  cr_num x = v.x, y = v.y, z = v.z;
  cr_num w = 1.0;

  cr_num nx = m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z + m.m[0][3] * w;
  cr_num ny = m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z + m.m[1][3] * w;
  cr_num nz = m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z + m.m[2][3] * w;
  cr_num nw = m.m[3][0] * x + m.m[3][1] * y + m.m[3][2] * z + m.m[3][3] * w;

  return cr_Vec3_create(nx / nw, ny / nw, nz / nw);
}

cr_Vec3 cr_Vec3_transform3(cr_Vec3 v, cr_Matrix mat) {
  cr_Matrix vmat = cr_Matrix_from_vector(v);
  cr_Matrix result = cr_Matrix_matmul(mat, vmat);
  cr_Vec3 out = cr_Vec3_from_matrix3(result);
  cr_Matrix_dealloc(&vmat);
  cr_Matrix_dealloc(&result);
  return out;
}

cr_Vec3 cr_Vec3_transform_dir(cr_Vec3 v, cr_Matrix mat) {
  cr_Vec3 out;
  if (mat.rows < 3 || mat.cols < 3)
    return v;
  out.x = v.x * mat.m[0][0] + v.y * mat.m[0][1] + v.z * mat.m[0][2];
  out.y = v.x * mat.m[1][0] + v.y * mat.m[1][1] + v.z * mat.m[1][2];
  out.z = v.x * mat.m[2][0] + v.y * mat.m[2][1] + v.z * mat.m[2][2];
  return out;
}

cr_Vec4 cr_Vec4_transform(cr_Vec3 v, cr_Matrix m) {
  cr_num x = v.x, y = v.y, z = v.z, w = 1.0;

  return (cr_Vec4){
      m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z + m.m[0][3] * w,
      m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z + m.m[1][3] * w,
      m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z + m.m[2][3] * w,
      m.m[3][0] * x + m.m[3][1] * y + m.m[3][2] * z + m.m[3][3] * w};
}

void cr_Vec3_setItem(cr_Vec3 *v, int i, cr_num a) {
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
void cr_Matrix_print(cr_Matrix m) {
  for (int i = 0; i < m.rows; i++) {
    for (int j = 0; j < m.cols; j++) {
      printf("%.2f, ", m.m[i][j]);
    }
    printf("\n");
  }
}
cr_num cr_Vec3_dot_(cr_Vec3 v1, cr_Vec3 v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
