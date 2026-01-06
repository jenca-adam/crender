#include "crender.h"
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
cr_Texture cr_Texture_alloc(int width, int height) {
  cr_REQUIRE_INIT cr_Texture texture;
  texture.width = width;
  texture.height = height;
  texture.m = malloc(sizeof(cr_Vec3) * width * height);
  texture.valid = true;
  if (!texture.m) {
    fprintf(stderr, "cr_Texture_alloc: malloc() failed: %s", strerror(errno));
  }
  return texture;
}
cr_Texture cr_Texture_create(int width, int height, cr_Vec3 color) {
  cr_Texture texture = cr_Texture_alloc(width, height);
  if (!texture.m)
    return texture;
  for (int i = 0; i < width * height; i++) {
    texture.m[i] = color;
  }
  return texture;
}

bool cr_Texture_writePPM(cr_Texture *texture, char *fn) {
  if (!texture->valid) {
    return false;
  }
  FILE *fp = fopen(fn, "wb");
  if (!fp) {
    fprintf(stderr, "cr_Texture_writePPM: fopen(%s) failed: %s", fn,
            strerror(errno));
    return false;
  }
  fprintf(fp, "P6\n%d %d\n255\n", texture->width, texture->height);
  for (int i = 0; i < texture->width * texture->height; i++) {
    cr_Vec3 color = texture->m[i];
    fputc(color.x, fp);
    fputc(color.y, fp);
    fputc(color.z, fp);
  }
  fclose(fp);
  return true;
}
cr_Texture cr_Texture_readPPM(char *fn) {
  FILE *fp = fopen(fn, "rb");
  char mode[3];
  int width;
  int height;
  unsigned long lmaxn; // convert everything to 8-bit RGB anyway
  if (!fp) {
    fprintf(stderr, "cr_Texture_readPPM: fopen(%s) failed: %s\n", fn,
            strerror(errno));
    return (cr_Texture){0};
  }
  if (!fgets(mode, sizeof(mode), fp)) {
    fprintf(stderr, "cr_Texture_readPPM: format error: can't read mode\n");
    return (cr_Texture){0};
  }
  if (strcmp(mode, "P6")) {
    fprintf(stderr,
            "cr_Texture_readPPM: format error: mode %s is not supported\n",
            mode);
    return (cr_Texture){0};
  }
  if (fscanf(fp, "\n%d %d\n%lu\n", &width, &height, &lmaxn) != 3) {
    fprintf(stderr, "cr_Texture_readPPM: format error: can't read header\n");
    return (cr_Texture){0};
  }
  if (lmaxn != 255) {
    fprintf(stderr,
            "cr_Texture_readPPM: format error: can only read 24-bit colors\n");
    return (cr_Texture){0};
  }
  cr_Texture texture = cr_Texture_alloc(width, height);
  if (!texture.m) {
    return (cr_Texture){0};
  }
  for (int i = 0; i < width * height; i++) {

    unsigned char rgb[3];
    fread(&rgb, 1, 3, fp);

    if (feof(fp)) {
      fprintf(stderr, "cr_Texture_readPPM: format error: EOF\n");
      cr_Texture_dealloc(&texture);
      return (cr_Texture){0};
    }
    texture.m[i] = cr_Vec3_create(rgb[0], rgb[1], rgb[2]);
  }

  return texture;
}

cr_Texture cr_Texture_read(char *fn) {
  if (strcmp(fn + strlen(fn) - 4, ".ppm") == 0) {
    return cr_Texture_readPPM(fn);
  } else {
    fprintf(stderr, "cr_Texture_read: unknown file format");
    return (cr_Texture){0};
  }
}

static inline cr_Vec3 cr_Texture_getuv_CLOSEST(const cr_Texture *restrict t,
                                               cr_Vec3 uv) {
  int x = (int)((cr_clamp(uv.x, 0.0, 1.0)) * (t->width - 1) + .5f);
  int y = (int)((cr_clamp((1 - uv.y), 0.0, 1.0)) * (t->height - 1) + .5f);
  return t->m[y * t->width + x];
}

static inline cr_Vec3 cr_Texture_getuv_FLOOR(const cr_Texture *restrict t,
                                             cr_Vec3 uv) {
  int x = (int)((cr_clamp(uv.x, 0.0, 1.0)) * (t->width - 1));
  int y = (int)((cr_clamp((1 - uv.y), 0.0, 1.0)) * (t->height - 1));
  return t->m[y * t->width + x];
}
static inline cr_Vec3 cr_Texture_getuv_LINEAR(const cr_Texture *restrict t,
                                              cr_Vec3 uv) {
  size_t w = t->width, h = t->height;
  cr_num u = cr_clamp(uv.x, 0.0, 1.0);
  cr_num v = 1.0 - (cr_clamp(uv.y, 0.0, 1.0));
  cr_num x = u * (w - 1);
  cr_num y = v * (h - 1);
  int x0 = (int)x;
  int y0 = (int)y;
  int x1 = x0 + ((size_t)x0 + 1 < w);
  int y1 = y0 + ((size_t)y0 + 1 < h);
  cr_num tx = x - x0;
  cr_num ty = y - y0;
  const cr_Vec3 *restrict row0 = &t->m[y0 * w];
  const cr_Vec3 *restrict row1 = &t->m[y1 * w];
  cr_Vec3 c00 = row0[x0];
  cr_Vec3 c10 = row0[x1];
  cr_Vec3 c01 = row1[x0];
  cr_Vec3 c11 = row1[x1];
  cr_num w00 = (1 - tx) * (1 - ty);
  cr_num w10 = tx * (1 - ty);
  cr_num w01 = (1 - tx) * ty;
  cr_num w11 = tx * ty;
  return (cr_Vec3){c00.x * w00 + c10.x * w10 + c01.x * w01 + c11.x * w11,
                   c00.y * w00 + c10.y * w10 + c01.y * w01 + c11.y * w11,
                   c00.z * w00 + c10.z * w10 + c01.z * w01 + c11.z * w11};
}

cr_Vec3 cr_Texture_getuv(const cr_Texture *t, cr_Vec3 uv,
                         cr_SamplingMode sampling_mode) {
  switch (sampling_mode) {
  case FLOOR:
    return cr_Texture_getuv_FLOOR(t, uv);
  case CLOSEST:
    return cr_Texture_getuv_CLOSEST(t, uv);
  case LINEAR:
    return cr_Texture_getuv_LINEAR(t, uv);
  default:
    cr_UNREACHABLE("Texture_getuv");
  }
}
static inline cr_Vec3 _interp_correct(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2,
                                      cr_Vec3 b, cr_num w0, cr_num w1,
                                      cr_num w2) {
  cr_num bxw = b.x * w0, byw = b.y * w1, bzw = b.z * w2;
  cr_num iw = w0 * b.x + w1 * b.y + w2 * b.z;
  return (cr_Vec3){
      (v0.x * bxw + v1.x * byw + v2.x * bzw) / iw,
      (v0.y * bxw + v1.y * byw + v2.y * bzw) / iw,
      (v0.z * bxw + v1.z * byw + v2.z * bzw) / iw,
  };
}

cr_num apow(cr_num x, uint8_t n) {
  cr_num p[8];
  p[0] = x;
  cr_num r = 1.0;
  for (int i = 1; i < 8; i++) {
    p[i] = p[i - 1] * p[i - 1];
    r *= 1 + (!!((n >> i) & 1)) * (p[i] - 1);
  }
  return r;
}

void cr_Texture_dealloc_ref(cr_Texture *texture) {
  if (!texture->m)
    return;
  free(texture->m);
}
void cr_Texture_dealloc(cr_Texture *texture) {
  if (!texture || !texture->valid)
    return;
  free(texture->m);
}
cr_Linear_Texture cr_Texture_to_linear(cr_Texture texture) {
  cr_Linear_Texture t = calloc(texture.width * texture.height, sizeof(*t));
  for (int i = 0; i < texture.width * texture.height; i++) {
    t[(int)i] = cr_Vec3_pack_color(texture.m[i]);
  }
  return t;
}
cr_Texture cr_Texture_bake_object_space_normal_map(cr_Texture *in,
                                                   cr_Object *object) {
  cr_REQUIRE_INIT;
  cr_ASSERT(in->valid, "invalid texture");
  cr_Texture out = cr_Texture_alloc(in->width, in->height);
  cr_Vec3 *vertex_tangents = malloc(sizeof(cr_Vec3) * object->vertices.count);
  cr_Object_compute_vertex_tangents(object, vertex_tangents);
  for (size_t i = 0; i < object->faces.count; i++) {
    // run a super simple version of the rasterization algorithm, it doesn't
    // have to be particularly fast
    cr_Face *face = &object->faces.items[i];
    cr_Triangle uvs, vs, vns;
    if (!cr_Face_gettri(face, object, VERTEX, &vs)) {
      continue; // just in case
    }
    if (!cr_Face_gettri(face, object, UV, &uvs)) {
      continue;
    }
    if (!cr_Face_gettri(face, object, NORMAL, &vns)) {
      cr_Vec3 n = cr_Vec3_normalized(
          cr_Vec3_cross(cr_Vec3_sub(vs.v2, vs.v0), cr_Vec3_sub(vs.v1, vs.v0)));
      vns = (cr_Triangle){n, n, n};
    }
    cr_Triangle scaled_uvs = {{uvs.v0.x * in->width, uvs.v0.y * in->height, 0},
                              {uvs.v1.x * in->width, uvs.v1.y * in->height, 0},
                              {uvs.v2.x * in->width, uvs.v2.y * in->height, 0}};
    int minx = fmaxf(
        0, cr_fmin3(scaled_uvs.v0.x, scaled_uvs.v1.x, scaled_uvs.v2.x) - 1);
    int maxx =
        fminf(in->width - 1,
              cr_fmax3(scaled_uvs.v0.x, scaled_uvs.v1.x, scaled_uvs.v2.x) + 1);
    int miny = fmaxf(
        0, cr_fmin3(scaled_uvs.v0.y, scaled_uvs.v1.y, scaled_uvs.v2.y) - 1);
    int maxy =
        fminf(in->height - 1,
              cr_fmax3(scaled_uvs.v0.y, scaled_uvs.v1.y, scaled_uvs.v2.y) + 1);

    cr_num bary_denom = 1 / ((scaled_uvs.v2.x - scaled_uvs.v0.x) *
                                 (scaled_uvs.v1.y - scaled_uvs.v0.y) -
                             (scaled_uvs.v1.x - scaled_uvs.v0.x) *
                                 (scaled_uvs.v2.y - scaled_uvs.v0.y));
    cr_Triangle vt = {vertex_tangents[face->vs[0] - 1],
                      vertex_tangents[face->vs[1] - 1],
                      vertex_tangents[face->vs[2] - 1]};
    for (int y = miny; y <= maxy; y++) {
      for (int x = minx; x <= maxx; x++) {
        cr_Vec3 bary = cr_barycentric(scaled_uvs.v0, scaled_uvs.v1,
                                      scaled_uvs.v2, x, y, bary_denom);
        if (bary.x < -cr_EPSILON || bary.y < -cr_EPSILON ||
            bary.z < -cr_EPSILON)
          continue;
        cr_Vec3 normal = cr_trinterpolate(vns, bary);
        cr_Vec3 tangent = cr_trinterpolate(vt, bary);
        tangent = cr_Vec3_normalized(cr_Vec3_sub(
            tangent, cr_Vec3_mul(normal, cr_Vec3_dot(tangent, normal))));
        cr_Vec3 bitangent = cr_Vec3_normalized(cr_Vec3_cross(normal, tangent));
        cr_Matrix tbn_mat =
            cr_Matrix_from_vectors_transposed(tangent, bitangent, normal);
        cr_Vec3 tangent_space_normal =
            cr_Vec3_normal_from_color(cr_Texture_getuv_LINEAR(
                in, (cr_Vec3){(cr_num)x / in->width, (cr_num)y / in->height,
                              0})); // or index in.m directly?
        cr_Vec3 object_space_normal = cr_Vec3_normalized(
            cr_Vec3_transform3(tangent_space_normal, tbn_mat));
        out.m[out.width * (out.height - y - 1) + x] =
            cr_Vec3_normal_as_color(object_space_normal);
      }
    }
  }
  return out;
}

// I LOVE C

#define _cr_Texture_shader_PHONG(SAMPLING_MODE, ...)                           \
  specpow = cr_Texture_getuv_##SAMPLING_MODE(specular_map, uv).x;              \
  spec = apow(fmax(d, 0.0), specpow);                                          \
  cr_num intensity = d + spec * .6;                                            \
  texture[(int)(tw * (th - y - 1) + x)] =                                      \
      cr_Vec3_phong(color, intensity, 0, 255);

#define _cr_Texture_shader_GOURAUD(...)                                        \
  (void)spec;                                                                  \
  (void)specpow;                                                               \
  (void)specular_map;                                                          \
  texture[(int)(tw * (th - y - 1) + x)] =                                      \
      cr_Vec3_pack_color(cr_Vec3_mul(color, fmax(d, 0.0)));

#define _cr_Texture_draw_face_IMPL(SHADING_MODE, SAMPLING_MODE,                \
                                   HAS_NORMAL_MAP)                             \
  _cr_Texture_draw_face_DECL(SHADING_MODE, SAMPLING_MODE, HAS_NORMAL_MAP) {    \
    (void)zbuffer_locks;                                                       \
    cr_Vec3 l = cr_Vec3_transform_dir(light_dir, inverse_transform);           \
    cr_Vec3 ldir = cr_Vec3_normalized(l);                                      \
    cr_Triangle raw_tri, uvs, vns;                                             \
    cr_Face_gettri(face, obj, VERTEX, &raw_tri);                               \
    cr_Face_gettri(face, obj, UV, &uvs);                                       \
    cr_Face_gettri(face, obj, NORMAL,                                          \
                   &vns); /* all three should be present, if the user puts     \
                             some garbage here, that's on them*/               \
    cr_Triangle world_tri = cr_Triangle_transform(raw_tri, world_transform);   \
    if (world_tri.v0.z > near_plane || world_tri.v1.z > near_plane ||          \
        world_tri.v2.z > near_plane) {                                         \
      return false;                                                            \
    }                                                                          \
    cr_Vec3 n;                                                                 \
    if (!CR_CFG_NO_BFCULL) { /*  known at compile time, this will get          \
                          inlined*/                                            \
      n = cr_Vec3_normalized(                                                  \
          cr_Vec3_cross(cr_Vec3_sub(raw_tri.v2, raw_tri.v0),                   \
                        cr_Vec3_sub(raw_tri.v1, raw_tri.v0)));                 \
      cr_num intensity = cr_Vec3_dot(n, ldir);                                 \
      if (intensity < -1 - cr_EPSILON) {                                       \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
    cr_Vec3 ws;                                                                \
    cr_Triangle tri = cr_Triangle_transform4(raw_tri, transform, &ws);         \
    cr_num x0 = tri.v0.x, y0 = tri.v0.y;                                       \
    cr_num x1 = tri.v1.x, y1 = tri.v1.y;                                       \
    cr_num x2 = tri.v2.x, y2 = tri.v2.y;                                       \
    cr_num bary_denom = 1 / ((x2 - x0) * (y1 - y0) - (x1 - x0) * (y2 - y0));   \
    cr_num tw = width;                                                         \
    cr_num th = height;                                                        \
    int minx = fmax(0, cr_fmin3(x0, x1, x2));                                  \
    int maxx = fmin(width - 1, cr_fmax3(x0, x1, x2));                          \
    int miny = fmax(0, cr_fmin3(y0, y1, y2));                                  \
    int maxy = fmin(height - 1, cr_fmax3(y0, y1, y2));                         \
    cr_num iw0 = 1.0f / ws.x;                                                  \
    cr_num iw1 = 1.0f / ws.y;                                                  \
    cr_num iw2 = 1.0f / ws.z;                                                  \
                                                                               \
    if (maxx < 0 || maxy < 0 || minx > tw || miny > th)                        \
      return false;                                                            \
    cr_Vec3 bbase =                                                            \
        cr_barycentric(tri.v0, tri.v1, tri.v2, minx, miny, bary_denom);        \
    cr_Vec3 deltax = cr_Vec3_sub(                                              \
        cr_barycentric(tri.v0, tri.v1, tri.v2, minx + 1, miny, bary_denom),    \
        bbase);                                                                \
    cr_Vec3 deltay = cr_Vec3_sub(                                              \
        cr_barycentric(tri.v0, tri.v1, tri.v2, minx, miny + 1, bary_denom),    \
        bbase);                                                                \
    cr_num rayx = deltax.x != 0 ? (cr_num)1.0 / deltax.x : 0;                  \
    cr_num rayy = deltax.y != 0 ? (cr_num)1.0 / deltax.y : 0;                  \
    cr_num rayz = deltax.z != 0 ? (cr_num)1.0 / deltax.z : 0;                  \
    for (int y = miny; y <= maxy; y++) {                                       \
      cr_Vec3 b = bbase;                                                       \
      /* compute the bounds from barycentric coordinates*/                     \
      cr_num tx = (-b.x * rayx), ty = (-b.y * rayy), tz = (-b.z * rayz);       \
      cr_num tinx = cr_clamplo(copysign(tx, -b.x), 0);                         \
      cr_num tiny = cr_clamplo(copysign(ty, -b.y), 0);                         \
      cr_num tinz = cr_clamplo(copysign(tz, -b.z), 0);                         \
      int tin = ceilf(cr_fmax3(tinx, tiny, tinz));                             \
      cr_Vec3_ADD_INPLACE3(b, deltax.x * tin, deltax.y * tin, deltax.z * tin); \
      for (int x = minx + tin; x <= maxx; x++) {                               \
        cr_NUM_INT_TYPE ibx, iby, ibz;                                         \
        cr_NUM_INT_CAST(b.x, ibx);                                             \
        cr_NUM_INT_CAST(b.y, iby);                                             \
        cr_NUM_INT_CAST(b.z, ibz);                                             \
        cr_NUM_INT_TYPE inside = ibx | iby | ibz;                              \
        if (inside < 0) {                                                      \
          break;                                                               \
        }                                                                      \
        cr_num iz = iw0 * b.x + iw1 * b.y + iw2 * b.z;                         \
        cr_num z = iz;                                                         \
        int zbuffix = x + y * tw;                                              \
        CR_IFOMPLOCK(omp_lock_t * lock);                                       \
        CR_IFOMPLOCK(lock = &zbuffer_locks[zbuffix]);                          \
        CR_IFOMPLOCK(omp_set_lock(lock));                                      \
                                                                               \
        if (zbuffer[zbuffix] < z) {                                            \
          cr_num spec, specpow;                                                \
          cr_Vec3 normal;                                                      \
          zbuffer[zbuffix] = z;                                                \
          cr_Vec3 uv =                                                         \
              _interp_correct(uvs.v0, uvs.v1, uvs.v2, b, iw0, iw1, iw2);       \
          cr_Vec3 color = cr_Texture_getuv_##SAMPLING_MODE(diffuse, uv);       \
          if (!HAS_NORMAL_MAP) {                                               \
            normal = cr_Vec3_neg(                                              \
                _interp_correct(vns.v0, vns.v1, vns.v2, b, iw0, iw1, iw2));    \
          } else {                                                             \
            normal = (cr_Vec3_normal_from_color(                               \
                cr_Texture_getuv_##SAMPLING_MODE(normal_map, uv)));            \
          }                                                                    \
          cr_num d = cr_Vec3_dot(normal, ldir);                                \
          _cr_Texture_shader_##SHADING_MODE(SAMPLING_MODE, 0);                 \
        }                                                                      \
        CR_IFOMPLOCK(omp_unset_lock(lock));                                    \
        cr_Vec3_ADD_INPLACE(b, deltax);                                        \
      }                                                                        \
      cr_Vec3_ADD_INPLACE(bbase, deltay);                                      \
    }                                                                          \
    return true;                                                               \
  }

_cr_Texture_draw_face_FORALL(_cr_Texture_draw_face_IMPL)
