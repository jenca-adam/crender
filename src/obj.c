#include "obj.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Object *Object_new() {
  Object *object = malloc(sizeof(Object));
  object->vertices = NULL;
  object->uvs = NULL;
  object->normals = NULL;
  object->faces = NULL;
  object->nv = 0;
  object->nuv = 0;
  object->nn = 0;
  object->nf = 0;
  return object;
}
void Object_addVertex(Object *object, Vec3 vertex) {
  object->vertices = realloc(object->vertices, (object->nv + 1) * sizeof(Vec3));
  object->vertices[object->nv] = vertex;
  object->nv++;
}
void Object_addUV(Object *object, Vec3 uv) {
  object->uvs = realloc(object->uvs, (object->nuv + 1) * sizeof(Vec3));
  object->uvs[object->nuv] = uv;
  object->nuv++;
}
void Object_addNormal(Object *object, Vec3 normal) {
  object->normals = realloc(object->normals, (object->nn + 1) * sizeof(Vec3));
  object->normals[object->nn] = normal;
  object->nn++;
}
void Object_addFace(Object *object, Face *face) {
  object->faces = realloc(object->faces, (object->nf + 1) * sizeof(Face *));
  object->faces[object->nf] = face;
  object->nf++;
}
Vec3 read_vec3(FILE *fp) {
  char buffer[1024];
  double x = 0;
  double y = 0;
  double z = 0;
  if (fgets(buffer, sizeof(buffer), fp)) {
    sscanf(buffer, "%lf %lf %lf", &x, &y, &z);
    return Vec3_create(x, y, z);
  }
  return Vec3_create(NAN, NAN, NAN);
}
void read_face_vinfo(char *vinfo, int *vindex, int *uvindex, int *nindex) {
  size_t size = strlen(vinfo);
  FILE *stream = fmemopen(vinfo, size, "rb");
  for (int i = 0; i < 3;) {
    int q;
    if (fscanf(stream, "%d", &q) == 1) {
      switch (i) {
      case 0:
        *vindex = q;
        break;
      case 1:
        *uvindex = q;
        break;
      case 2:
        *nindex = q;
        break;
      }
      // ignore slashes
      int c;
      c = fgetc(stream);
      while (c == '/') {
        c = fgetc(stream);
        i++;
      }
      if (c == EOF) {
        break;
      }

      fseek(stream, -1, SEEK_CUR); // go back (maybe ungetc  would be better?)

    } else {
      break;
    }
  }
  fclose(stream);
}
Face *read_face(FILE *fp, Object *obj) {
  char **vinfos = malloc(3 * sizeof(char *));
  for (int i = 0; i < 3; i++) {
    vinfos[i] = calloc(512, sizeof(char));
  }
  Face *face = malloc(sizeof(Face));
  face->parent = obj;
  char buffer[2048];
  if (fgets(buffer, sizeof(buffer), fp)) {
    if (sscanf(buffer, "%s %s %s", vinfos[0], vinfos[1], vinfos[2]) != 3) {
      for (int i = 0; i < 3; i++) {
        free(vinfos[i]);
      }
      free(vinfos);
      free(face);
      return NULL;
    }
  }
  int *vs = malloc(3 * sizeof(int));
  int *vts = malloc(3 * sizeof(int));
  int *vns = malloc(3 * sizeof(int));
  for (int i = 0; i < 3; i++) {
    vs[i] = 0;
    vts[i] = 0;
    vns[i] = 0;
    read_face_vinfo(vinfos[i], &vs[i], &vts[i], &vns[i]);
  }
  face->vs = vs;
  face->vts = vts;
  face->vns = vns;
  for (int i = 0; i < 3; i++) {
    free(vinfos[i]);
  }
  free(vinfos);

  return face;
}
Object *Object_fromOBJ(char *fn) {

  FILE *fp = fopen(fn, "rb");
  if (!fp) {
    perror("Object_fromOBJ: fopen() fail");
    return NULL;
  }

  Object *object = Object_new();
  Vec3 arg;
  Face *farg;
  char *line_type = malloc(64 * sizeof(char));
  while (1) {
    if (fscanf(fp, "%s ", line_type) != 1) {
      break;
    }
    if (!strcmp(line_type, "v")) {
      arg = read_vec3(fp);
      if (arg.x == NAN) {
        break;
      }
      Object_addVertex(object, arg);
    } else if (!strcmp(line_type, "vt")) {
      arg = read_vec3(fp);
      if (arg.x == NAN) {
        break;
      }
      Object_addUV(object, arg);
    } else if (!strcmp(line_type, "vn")) {
      arg = read_vec3(fp);
      if (arg.x == NAN) {
        break;
      }
      Object_addNormal(object, arg);
    } else if (!strcmp(line_type, "f")) {
      farg = read_face(fp, object);
      if (!farg) {
        break;
      }
      Object_addFace(object, farg);
    }
  }
  free(line_type);
  fclose(fp);
  return object;
}

Triangle Face_gettri(Face *face, FACE_TRI_TYPE tt) {
  switch (tt) {
  case VERTEX:
    return Triangle_create(face->parent->vertices[face->vs[0] - 1],
                           face->parent->vertices[face->vs[1] - 1],
                           face->parent->vertices[face->vs[2] - 1]);

  case UV:
    return Triangle_create(face->parent->uvs[face->vts[0] - 1],
                           face->parent->uvs[face->vts[1] - 1],
                           face->parent->uvs[face->vts[2] - 1]);
  case NORMAL:

    return Triangle_create(face->parent->normals[face->vns[0] - 1],
                           face->parent->normals[face->vns[1] - 1],
                           face->parent->normals[face->vns[2] - 1]);
  default:
    return Triangle_create(Vec3_create(0, 0, 0), Vec3_create(0, 0, 0),
                           Vec3_create(0, 0, 0));
  }
}
void Face_dealloc(Face *face) {
  free(face->vs);
  free(face->vts);
  free(face->vns);
  free(face);
}
void Object_dealloc(Object *object) {
  free(object->vertices);
  free(object->uvs);
  free(object->normals);
  for (int i = 0; i < object->nf; i++) {
    Face_dealloc(object->faces[i]);
  }
  free(object->faces);
  free(object);
}
