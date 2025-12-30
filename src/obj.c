#include "crender.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cr_Object *cr_Object_new() {
  cr_Object *object = malloc(sizeof(cr_Object));
  object->vertices = NULL;
  object->uvs = NULL;
  object->normals = NULL;
  object->faces = NULL;
  object->nv = 0;
  object->nuv = 0;
  object->nn = 0;
  object->nf = 0;
  object->valid = true;
  return object;
}
// TODO DYNAMIC ARRAYS!!!!
void cr_Object_add_vertex(cr_Object *object, cr_Vec3 vertex) {
  object->vertices =
      realloc(object->vertices, (object->nv + 1) * sizeof(cr_Vec3));
  object->vertices[object->nv] = vertex;
  object->nv++;
}
void cr_Object_add_uv(cr_Object *object, cr_Vec3 uv) {
  object->uvs = realloc(object->uvs, (object->nuv + 1) * sizeof(cr_Vec3));
  object->uvs[object->nuv] = uv;
  object->nuv++;
}
void cr_Object_add_normal(cr_Object *object, cr_Vec3 normal) {
  object->normals =
      realloc(object->normals, (object->nn + 1) * sizeof(cr_Vec3));
  object->normals[object->nn] = normal;
  object->nn++;
}
void cr_Object_add_face(cr_Object *object, cr_Face *face) {
  object->faces = realloc(object->faces, (object->nf + 1) * sizeof(cr_Face *));
  object->faces[object->nf] = face;
  object->nf++;
}
cr_Vec3 read_vec3(FILE *fp) {
  char buffer[1024];
  cr_num x = 0;
  cr_num y = 0;
  cr_num z = 0;
  if (fgets(buffer, sizeof(buffer), fp)) {
    sscanf(buffer, cr_NUM_FMT " " cr_NUM_FMT " " cr_NUM_FMT, &x, &y, &z);
    return cr_Vec3_create(x, y, z);
  }
  return cr_Vec3_create(NAN, NAN, NAN);
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
cr_Face *read_face(FILE *fp, cr_Object *obj) {
  char **vinfos = malloc(3 * sizeof(char *));
  for (int i = 0; i < 3; i++) {
    vinfos[i] = calloc(512, sizeof(char));
  }
  cr_Face *face = malloc(sizeof(cr_Face));
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
    if (vs[i] > obj->nv) {
      cr_ERROR("corrupted object file! vertex index %d out of bounds for a "
               "model with %d vertices",
               vs[i], obj->nv);
    }
    if (vts[i] > obj->nuv) {
      cr_ERROR("Corrupted object file! UV index %d out of bounds for a model "
               "with %d UVs",
               vts[i], obj->nuv);
      if (vns[i] > obj->nn) {
        cr_ERROR("Corrupted object file! Vertex normal index %d out of bounds "
                 "for a model with %d vertex normals",
                 vns[i], obj->nn);
      }
    }
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
cr_Object *cr_Object_fromOBJ(char *fn) {

  FILE *fp = fopen(fn, "rb");

  if (!fp) {
    fprintf(stderr, "cr_Object_fromOBJ: fopen(%s) failed: %s\n", fn,
            strerror(errno));
    return NULL;
  }
  cr_Object *object = cr_Object_new();
  cr_Vec3 arg;
  cr_Face *farg;
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
      cr_Object_add_vertex(object, arg);
    } else if (!strcmp(line_type, "vt")) {
      arg = read_vec3(fp);
      if (arg.x == NAN) {
        break;
      }
      cr_Object_add_uv(object, arg);
    } else if (!strcmp(line_type, "vn")) {
      arg = read_vec3(fp);
      if (arg.x == NAN) {
        break;
      }
      cr_Object_add_normal(object, arg);
    } else if (!strcmp(line_type, "f")) {
      farg = read_face(fp, object);
      if (!farg) {
        break;
      }
      cr_Object_add_face(object, farg);
    }
  }
  if (!object->vertices) {
    cr_ERROR("no vertices defined in object file %s", fn);
  }
  if (!object->uvs) {
    cr_ERROR("object is not uv-mapped!");
  }
  free(line_type);
  fclose(fp);
  return object;
}

bool cr_Face_gettri(cr_Face *face, cr_FaceTriType tt, cr_Triangle *tri) {
  switch (tt) {
  case VERTEX:
    if (!face->parent->vertices) {
      return false;
    }
    *tri = cr_Triangle_create(face->parent->vertices[face->vs[0] - 1],
                              face->parent->vertices[face->vs[1] - 1],
                              face->parent->vertices[face->vs[2] - 1]);
    break;

  case UV:
    if (!face->parent->uvs) {
      return false;
    }

    *tri = cr_Triangle_create(face->parent->uvs[face->vts[0] - 1],
                              face->parent->uvs[face->vts[1] - 1],
                              face->parent->uvs[face->vts[2] - 1]);
    break;
  case NORMAL:
    if (!face->parent->normals) {
      return false;
    }
    *tri = cr_Triangle_create(face->parent->normals[face->vns[0] - 1],
                              face->parent->normals[face->vns[1] - 1],
                              face->parent->normals[face->vns[2] - 1]);
    break;
  default:
    cr_UNREACHABLE("face_gettri");
  }
  return true;
}
void cr_Face_dealloc(cr_Face *face) {
  free(face->vs);
  free(face->vts);
  free(face->vns);
  free(face);
}
void cr_Object_dealloc(cr_Object *object) {
  if (!object || !object->valid)
    return;
  free(object->vertices);
  free(object->uvs);
  free(object->normals);
  for (int i = 0; i < object->nf; i++) {
    cr_Face_dealloc(object->faces[i]);
  }
  free(object->faces);
  free(object);
}
