#ifndef _OBJ_H
#define _OBJ_H
#include "vec.h"
#include "tri.h"
struct Object;
typedef struct Face{
	int *vs;
	int *vts;
	int *vns;
	struct Object *parent;
} Face;
typedef struct Object{
	Vec3 *vertices;
	Vec3 *uvs;
	Vec3 *normals;
	Face **faces;
	int nv;
	int nuv;
	int nn;
	int nf;
} Object;
typedef enum FACE_TRI_TYPE{
	VERTEX = 0,
	UV = 1,
	NORMAL = 2,
} FACE_TRI_TYPE;
Object *Object_new();
void Object_addVertex(Object *object, Vec3 vertex);
void Object_addUV(Object *object, Vec3 uv);
void Object_addNormal(Object *object, Vec3 normal);
void Object_addFace(Object *object, Face *face);
void Object_dealloc(Object *object);
Object *Object_fromOBJ(char *fn, char *dirname);

Triangle Face_gettri(Face *face, FACE_TRI_TYPE tt);
void Face_dealloc(Face *face);
#endif
