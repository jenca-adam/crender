#ifndef _TRI_H
#define _TRI_H
#include"vec.h"
typedef struct Triangle{
	Vec3 v0;
	Vec3 v1;
	Vec3 v2;
} Triangle;

Triangle Triangle_transform(Triangle tri, Matrix transform);

Triangle Triangle_transform3(Triangle tri, Matrix transform);
Triangle Triangle_transform4(Triangle tri, Matrix transform, Vec3 *ws);
Triangle Triangle_create(Vec3 v0, Vec3 v1, Vec3 v2);
#endif
