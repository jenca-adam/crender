#ifndef _VEC_H
#define _VEC_H
typedef struct Vec2{
	double x;
	double y;
} Vec2;
typedef struct Vec3{
	double x;
	double y;
	double z;
} Vec3;
typedef struct Matrix{
	int rows;
	int cols;
	double **m;
} Matrix;
Matrix Matrix_empty(int rows, int cols);
Matrix Matrix_identity(int size);
Matrix Matrix_matmul(Matrix m1, Matrix m2);
Matrix Matrix_projection(double camz);
Matrix Matrix_viewport(double x, double y, double w, double h, double d);
Matrix Matrix_from_vector(Vec3 v);
Matrix Matrix_rotz(double theta);
Matrix Matrix_roty(double theta);
Matrix Matrix_rotx(double theta);
Matrix Matrix_inverse(Matrix matrix);
void Matrix_dealloc(Matrix mat);
Vec2 Vec2_create(double x, double y);
Vec3 Vec3_create(double x, double y, double z);
Vec3 Vec3_cross(Vec3 v1, Vec3 v2);
double Vec3_dot(Vec3 v1, Vec3 v2);
Vec3 Vec3_neg(Vec3 v1);
Vec3 Vec3_add(Vec3 v1, Vec3 v2);
Vec3 Vec3_sub(Vec3 v1, Vec3 v2);
Vec3 Vec3_mul(Vec3 v1, double a);
Vec3 Vec3_normal_from_color(Vec3 color);
Vec3 Vec3_phong(Vec3 v1, double a, double lo, double hi);
Vec3 Vec3_div(Vec3 v1, double a);
double Vec3_length(Vec3 v);
Vec3 Vec3_normalized(Vec3 v);
Vec3 Vec3_from_matrix(Matrix mat);
Vec3 Vec3_from_matrix3(Matrix mat);
Vec3 Vec3_copy(Vec3 v);
Vec3 Vec3_transform(Vec3 v, Matrix mat);
Vec3 Vec3_transform3(Vec3 v, Matrix mat);
void Vec3_set_item(Vec3 v, int i, double a);
double Vec3_get_item(Vec3 v, int i);
#endif
