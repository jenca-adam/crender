#include "crender.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cr_Object cr_Object_new() {
  cr_REQUIRE_INIT cr_Object object = {0};
  object.vertices = (cr_Vec3_dynarr){0};
  object.uvs = (cr_Vec3_dynarr){0};
  object.normals = (cr_Vec3_dynarr){0};
  object.faces = (cr_Face_dynarr){0};
  object.valid = true;
  return object;
}
// TODO DYNAMIC ARRAYS!!!!
void cr_Object_add_vertex(cr_Object *object, cr_Vec3 vertex) {
  cr_DYNARR_PUSH(&object->vertices, vertex);
}
void cr_Object_add_uv(cr_Object *object, cr_Vec3 uv) {
  cr_DYNARR_PUSH(&object->uvs, uv);
}
void cr_Object_add_normal(cr_Object *object, cr_Vec3 normal) {
  cr_DYNARR_PUSH(&object->normals, normal);
}
void cr_Object_add_face(cr_Object *object, cr_Face face) {
  cr_DYNARR_PUSH(&object->faces, face);
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
bool read_face(FILE *fp, cr_Object *obj, cr_Face *face) {

  char vinfos[3][512];
  char buffer[2048];
  if (fgets(buffer, sizeof(buffer), fp)) {
    if (sscanf(buffer, "%s %s %s", vinfos[0], vinfos[1], vinfos[2]) != 3) {
      return false;
    }
  }
  *face = (cr_Face){0};
  for (int i = 0; i < 3; i++) {
    face->vs[i] = 0;
    face->vts[i] = 0;
    face->vns[i] = 0;
    read_face_vinfo(vinfos[i], &face->vs[i], &face->vts[i], &face->vns[i]);
    if (face->vs[i] > obj->vertices.count) {
      cr_ERROR("corrupted object file! vertex index %d out of bounds for a "
               "model with %zu vertices",
               face->vs[i], obj->vertices.count);
    }
    if (face->vts[i] > obj->uvs.count) {
      cr_ERROR("Corrupted object file! UV index %d out of bounds for a model "
               "with %zu UVs",
               face->vts[i], obj->uvs.count);
      if (face->vns[i] > obj->normals.count) {
        cr_ERROR("Corrupted object file! Vertex normal index %d out of bounds "
                 "for a model with %zu vertex normals",
                 face->vns[i], obj->normals.count);
      }
    }
  }
  return true;
}
cr_Object cr_Object_fromOBJ(char *fn) {

  FILE *fp = fopen(fn, "rb");

  if (!fp) {
    fprintf(stderr, "cr_Object_fromOBJ: fopen(%s) failed: %s\n", fn,
            strerror(errno));
    return (cr_Object){0};
  }
  cr_Object object = cr_Object_new();
  cr_Vec3 arg;
  cr_Face farg;
  char line_type[64];
  while (1) {
    if (fscanf(fp, "%s ", line_type) != 1) {
      break;
    }
    if (!strcmp(line_type, "v")) {
      arg = read_vec3(fp);
      if (arg.x == NAN) {
        break;
      }
      cr_Object_add_vertex(&object, arg);
    } else if (!strcmp(line_type, "vt")) {
      arg = read_vec3(fp);
      if (arg.x == NAN) {
        break;
      }
      cr_Object_add_uv(&object, arg);
    } else if (!strcmp(line_type, "vn")) {
      arg = read_vec3(fp);
      if (arg.x == NAN) {
        break;
      }
      cr_Object_add_normal(&object, arg);
    } else if (!strcmp(line_type, "f")) {
      cr_Face farg;
      if (!read_face(fp, &object, &farg)) {
        break;
      }
      cr_Object_add_face(&object, farg);
    }
  }
  if (!object.vertices.count) {
    fprintf(stderr, "cr_Object_fromOBJ: no vertices defined in object file %s",
            fn);
    cr_Object_dealloc(&object);
    return (cr_Object){0};
  }
  if (!object.uvs.count) {
    fprintf(stderr, "cr_Object_fromOBJ: object %s is not uv-mapped!", fn);
    cr_Object_dealloc(&object);
    return (cr_Object){0};
  }
  fclose(fp);
  return object;
}

bool cr_Face_gettri(cr_Face *face, cr_Object *obj, cr_FaceTriType tt,
                    cr_Triangle *tri) {
  switch (tt) {
  case VERTEX:
    if (!obj->vertices.count) {
      return false;
    }
    *tri = cr_Triangle_create(obj->vertices.items[face->vs[0] - 1],
                              obj->vertices.items[face->vs[1] - 1],
                              obj->vertices.items[face->vs[2] - 1]);
    break;

  case UV:
    if (!obj->uvs.count) {
      return false;
    }

    *tri = cr_Triangle_create(obj->uvs.items[face->vts[0] - 1],
                              obj->uvs.items[face->vts[1] - 1],
                              obj->uvs.items[face->vts[2] - 1]);
    break;
  case NORMAL:
    if (!obj->normals.count) {
      return false;
    }
    *tri = cr_Triangle_create(obj->normals.items[face->vns[0] - 1],
                              obj->normals.items[face->vns[1] - 1],
                              obj->normals.items[face->vns[2] - 1]);
    break;
  default:
    cr_UNREACHABLE("face_gettri");
  }
  return true;
}
void cr_Object_dealloc(cr_Object *object) {
  if (!object || !object->valid)
    return;
  cr_DYNARR_DEALLOC(object->vertices);
  cr_DYNARR_DEALLOC(object->uvs);
  cr_DYNARR_DEALLOC(object->normals);
  cr_DYNARR_DEALLOC(object->faces);
}
