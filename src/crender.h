#ifndef _CRENDER_H
#define _CRENDER_H
#include "crender_cfg.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef CR_CFG_NUM_DOUBLE
#define CR_CFG_NUM_DOUBLE 0
#endif
#ifndef CR_CFG_NO_MULTITHREAD
#define CR_CFG_NO_MULTITHREAD 0
#endif
#ifndef CR_CFG_NO_BFCULL
#define CR_CFG_NO_BFCULL 0
#endif
#ifndef CR_CFG_NO_LOCK
#define CR_CFG_NO_LOCK 0
#endif
#if CR_CFG_NO_MULTITHREAD
#define CR_IFOMPLOCK(B)
#define CR_IFOMP(B)
typedef void omp_lock_t;
#else
#include <omp.h>
#define CR_IFOMP(B) B
#if CR_CFG_NO_LOCK
#define CR_IFOMPLOCK(B)
#else
#define CR_IFOMPLOCK(B) B
#endif
#endif
#if CR_CFG_NUM_DOUBLE
typedef double cr_num;
#define cr_NUM_FMT "%lf"
#define cr_NUM_INT_TYPE int64_t
#else
typedef float cr_num;
#define cr_NUM_FMT "%f"
#define cr_NUM_INT_TYPE int32_t
#endif
#define cr_NUM_INT_CAST(a, b) memcpy(&b, &a, sizeof(cr_NUM_INT_TYPE))
#define cr_ABORT(t, fmt, ...)                                                  \
  do {                                                                         \
    fprintf(stderr, "%s:%d [%s]" fmt "\n", __FILE__, __LINE__, t,              \
            ##__VA_ARGS__);                                                    \
    abort();                                                                   \
  } while (0)

#define cr_UNREACHABLE(fmt, ...) cr_ABORT("Unreachable", fmt, ##__VA_ARGS__)
#define cr_ERROR(fmt, ...) cr_ABORT("Error", fmt, ##__VA_ARGS__)
#define cr_ASSERT(a, fmt, ...)                                                 \
  if (!(a))                                                                    \
    cr_ABORT("Assertion `" #a "` failed", fmt, ##__VA_ARGS__);
#ifndef cr_EPSILON
#define cr_EPSILON 1e-2
#endif
#ifndef cr_VIEWPORT_DEPTH
#define cr_VIEWPORT_DEPTH 255
#endif
#ifndef cr_AMBIENT
#define cr_AMBIENT 0
#endif
#ifndef cr_SCHEDULE
#define cr_SCHEDULE dynamic
#endif
#define cr_Vec3_ADD_INPLACE3(a, dx, dy, dz)                                    \
  a.x += dx;                                                                   \
  a.y += dy;                                                                   \
  a.z += dz;
#define cr_Vec3_ADD_INPLACE(a, b) cr_Vec3_ADD_INPLACE3(a, b.x, b.y, b.z)

#define cr_Vec3_NEG_INPLACE(a)                                                 \
  a.x = -a.x;                                                                  \
  a.y = -a.y;                                                                  \
  a.z = -a.z
#ifndef cr_ENTITIES_INITIAL_CAPACITY
#define cr_ENTITIES_INITIAL_CAPACITY 8
#endif
#define _cr_Texture_draw_face_NAME(SHADING_MODE, SAMPLING_MODE,                \
                                   HAS_NORMAL_MAP)                             \
  _cr_Texture_draw_face_##SHADING_MODE##_##SAMPLING_MODE##_##HAS_NORMAL_MAP
#define _cr_Texture_draw_face_ARGS                                             \
  (cr_Linear_Texture texture, int width, int height, cr_Face *face,            \
   cr_Object *obj, cr_Texture *diffuse, cr_Texture *normal_map,                \
   cr_Texture *specular_map, cr_num *zbuffer, omp_lock_t *zbuffer_locks,       \
   cr_Vec3 light_dir, cr_Matrix transform, cr_Matrix world_transform,          \
   cr_Matrix inverse_transform, cr_num near_plane)

#define _cr_Texture_draw_face_DECL(SHADING_MODE, SAMPLING_MODE,                \
                                   HAS_NORMAL_MAP)                             \
  bool _cr_Texture_draw_face_NAME(SHADING_MODE, SAMPLING_MODE, HAS_NORMAL_MAP) \
      _cr_Texture_draw_face_ARGS
#define _cr_Texture_draw_face_DECLH(...)                                       \
  _cr_Texture_draw_face_DECL(__VA_ARGS__);
#define _cr_Texture_draw_face_FORALL1(XMACRO)                                  \
  XMACRO(GOURAUD)                                                              \
  XMACRO(PHONG)
#define _cr_Texture_draw_face_FORALL2(XMACRO)                                  \
  XMACRO(FLOOR)                                                                \
  XMACRO(CLOSEST)                                                              \
  XMACRO(LINEAR)
#define _cr_Texture_draw_face_FORALL3(XMACRO)                                  \
  XMACRO(0)                                                                    \
  XMACRO(1)
#define _cr_Texture_draw_face_FORALL(                                          \
    XMACRO) /* too lazy to make a cartesian product*/                          \
  XMACRO(GOURAUD, FLOOR, 0)                                                    \
  XMACRO(GOURAUD, FLOOR, 1)                                                    \
  XMACRO(GOURAUD, CLOSEST, 0)                                                  \
  XMACRO(GOURAUD, CLOSEST, 1)                                                  \
  XMACRO(GOURAUD, LINEAR, 0)                                                   \
  XMACRO(GOURAUD, LINEAR, 1)                                                   \
  XMACRO(PHONG, FLOOR, 0)                                                      \
  XMACRO(PHONG, FLOOR, 1)                                                      \
  XMACRO(PHONG, CLOSEST, 0)                                                    \
  XMACRO(PHONG, CLOSEST, 1)                                                    \
  XMACRO(PHONG, LINEAR, 0)                                                     \
  XMACRO(PHONG, LINEAR, 1)
#define _cr_Texture_draw_face_HANDLE(SHADING, SAMPLING, NORMAL)                \
  return _cr_Texture_draw_face_NAME(SHADING, SAMPLING, NORMAL);
#define _cr_Texture_draw_face_NORMAL_CASES(SHADING, SAMPLING)                  \
  if (has_normal_map) {                                                        \
    _cr_Texture_draw_face_HANDLE(SHADING, SAMPLING, 1)                         \
  } else {                                                                     \
    _cr_Texture_draw_face_HANDLE(SHADING, SAMPLING, 0)                         \
  }

#define _cr_Texture_draw_face_SAMPLING_CASES(SHADING)                          \
  case FLOOR:                                                                  \
    _cr_Texture_draw_face_NORMAL_CASES(SHADING, FLOOR);                        \
    break;                                                                     \
  case CLOSEST:                                                                \
    _cr_Texture_draw_face_NORMAL_CASES(SHADING, CLOSEST);                      \
    break;                                                                     \
  case LINEAR:                                                                 \
    _cr_Texture_draw_face_NORMAL_CASES(SHADING, LINEAR);                       \
    break;

#define _cr_Texture_draw_face_SHADING_CASES                                    \
  case GOURAUD:                                                                \
    switch (sampling_mode) { _cr_Texture_draw_face_SAMPLING_CASES(GOURAUD) }   \
    break;                                                                     \
  case PHONG:                                                                  \
    switch (sampling_mode) { _cr_Texture_draw_face_SAMPLING_CASES(PHONG) }     \
    break;

#define _cr_Texture_draw_face_SELECT                                           \
  switch (shading_mode) { _cr_Texture_draw_face_SHADING_CASES }                \
  return false;
#define CR_CFG_ABI_TAG ((CR_CFG_NUM_DOUBLE) | (CR_CFG_NO_MULTITHREAD << 1))
#define cr_INIT_CRENDER(...)                                                   \
  cr_ASSERT(_cr_abi_tag == CR_CFG_ABI_TAG,                                     \
            ": please recompile libcrender with correct crender_cfg.h!");      \
  cr_crender_initted = true;
#define cr_REQUIRE_INIT                                                        \
  cr_ASSERT(cr_crender_initted,                                                \
            ": crender not initted! use the macro cr_INIT_CRENDER().");
#define cr_DYNARR_BASE_CAPACITY 8
#define cr_DYNARR_PUSH(arr, what)                                              \
  do {                                                                         \
                                                                               \
    if ((arr)->count + 1 > (arr)->capacity) {                                  \
      if ((arr)->capacity == 0) {                                              \
        (arr)->capacity = cr_DYNARR_BASE_CAPACITY;                             \
      } else {                                                                 \
        (arr)->capacity *= 2;                                                  \
      }                                                                        \
      (arr)->items =                                                           \
          realloc((arr)->items, (arr)->capacity * sizeof(*(arr)->items));      \
      cr_ASSERT((arr)->items != NULL, "")                                      \
    }                                                                          \
    (arr)->items[(arr)->count++] = what;                                       \
  } while (0)
#define cr_DYNARR_REMOVE_SWAP_LAST(arr, index)                                 \
  do {                                                                         \
    cr_ASSERT((index) < (arr)->count, ": remove index out of range");          \
    (arr)->items[index] = (arr)->items[--(arr)->count];                        \
  } while (0)
#define cr_DYNARR_REMOVE_KEEP_ORDER(arr, index)                                \
  do {                                                                         \
    cr_ASSERT((index) < (arr)->count, ": remove index out of range");          \
    if (index < (arr)->count - 1) {                                            \
      memmove(&(arr)->items[index], &(arr)->items[index + 1],                  \
              ((arr)->count - 1 - index) * sizeof(*(arr)->items));             \
    }                                                                          \
    (arr)->count--;                                                            \
  } while (0)
#define cr_DYNARR_FROMARR(target, source, new_count)                           \
  do {                                                                         \
    (target)->count = new_count;                                               \
    (target)->capacity = cr_DYNARR_BASE_CAPACITY;                              \
    while ((target)->capacity < (target)->count) {                             \
      (target)->capacity *= 2;                                                 \
    }                                                                          \
    (target)->items = calloc((target)->capacity, sizeof(*(target)->items));    \
    memcpy((target)->items, source, new_count * sizeof(*source));              \
  } while (0)
#define cr_DYNARR_DEALLOC(arr) free(arr.items)
extern int _cr_abi_tag;
extern bool cr_crender_initted;
typedef enum cr_ShadingMode {
  PHONG = 0,   // slower, specular highlights
  GOURAUD = 1, // faster, no specular highlights
} cr_ShadingMode;
typedef enum cr_SamplingMode {
  FLOOR = 0,   // worst, fastest
  CLOSEST = 1, // marginally better and equally marginally slower
  LINEAR = 2,  // best, slowest
} cr_SamplingMode;

typedef struct cr_Vec2 {
  cr_num x;
  cr_num y;
} cr_Vec2;
typedef struct cr_Vec3 {
  cr_num x;
  cr_num y;
  cr_num z;
} cr_Vec3;
typedef struct cr_Vec4 {
  cr_num x, y, z, w;
} cr_Vec4;

typedef struct cr_Matrix {
  cr_num **m;
  int rows;
  int cols;
  bool valid;
} cr_Matrix;
static inline cr_num cr_clamp(cr_num a, cr_num lo, cr_num hi) {
  return fminf(fmaxf(a, lo), hi);
}
static inline cr_num cr_clamplo(cr_num a, cr_num lo) { return fmaxf(a, lo); }
static inline cr_Vec2 cr_Vec2_create(cr_num x, cr_num y) {
  return (cr_Vec2){x, y};
}

static inline cr_Vec3 cr_Vec3_create(cr_num x, cr_num y, cr_num z) {
  return (cr_Vec3){x, y, z};
}

static inline cr_Vec3 cr_Vec3_cross(cr_Vec3 v1, cr_Vec3 v2) {
  return cr_Vec3_create(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                        v1.x * v2.y - v1.y * v2.x);
}

static inline cr_num cr_Vec3_dot(cr_Vec3 v1, cr_Vec3 v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

static inline cr_Vec3 cr_Vec3_add(cr_Vec3 v1, cr_Vec3 v2) {
  return cr_Vec3_create(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

static inline cr_Vec3 cr_Vec3_neg(cr_Vec3 v1) {
  return cr_Vec3_create(-v1.x, -v1.y, -v1.z);
}

static inline cr_Vec3 cr_Vec3_sub(cr_Vec3 v1, cr_Vec3 v2) {
  return cr_Vec3_create(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

static inline cr_Vec3 cr_Vec3_mul(cr_Vec3 v1, cr_num a) {
  return cr_Vec3_create(v1.x * a, v1.y * a, v1.z * a);
}

static inline cr_Vec3 cr_Vec3_div(cr_Vec3 v1, cr_num a) {
  return cr_Vec3_create(v1.x / a, v1.y / a, v1.z / a);
}

static inline cr_num cr_Vec3_length(cr_Vec3 v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline cr_Vec3 cr_Vec3_normalized(cr_Vec3 v) {
  return cr_Vec3_div(v, cr_Vec3_length(v));
}

static inline cr_Vec3 cr_Vec3_from_matrix(cr_Matrix mat) {
  return cr_Vec3_create(mat.m[0][0] / mat.m[3][0], mat.m[1][0] / mat.m[3][0],
                        mat.m[2][0] / mat.m[3][0]);
}

static inline cr_Vec3 cr_Vec3_from_matrix3(cr_Matrix mat) {
  return cr_Vec3_create(mat.m[0][0], mat.m[1][0], mat.m[2][0]);
}
static inline uint32_t cr__pack(int r, int g, int b) {
  return 0xff | b << 8 | g << 16 | r << 24;
}
static inline uint32_t cr_Vec3_phong(cr_Vec3 v1, cr_num a, cr_num lo,
                                     cr_num hi) {
  return cr__pack(
      cr_clamp(v1.x * a + cr_AMBIENT, lo,
               hi), // TODO: get rid of cr_AMBIENT, pass it as a parameter?
      cr_clamp(v1.y * a + cr_AMBIENT, lo, hi),
      cr_clamp(v1.z * a + cr_AMBIENT, lo, hi));
}
static inline uint32_t cr_Vec3_pack_color(cr_Vec3 v) {
  return cr__pack(v.x, v.y, v.z);
}

static inline cr_Vec3 cr_Vec3_normal_from_color(cr_Vec3 color) {
  return (cr_Vec3){-(color.x / (cr_num)127.5) + 1,
                   -(color.y / (cr_num)127.5) + 1,
                   -(color.z / (cr_num)127.5) + 1};
}
static inline cr_Vec3 cr_Vec3_normal_as_color(cr_Vec3 normal) {
  return (cr_Vec3){(1 - normal.x) * (cr_num)127.5,
                   (1 - normal.y) * (cr_num)127.5,
                   (1 - normal.z) * (cr_num)127.5};
}

static inline void cr_Vec3_normalize(cr_Vec3 *v) {
  cr_num l = cr_Vec3_length(*v);
  v->x /= l;
  v->y /= l;
  v->z /= l;
}
cr_Vec3 cr_Vec3_transform_dir(cr_Vec3 v, cr_Matrix mat);
void cr_Matrix_print(cr_Matrix m);
cr_Matrix cr_Matrix_empty(int rows, int cols);
cr_Matrix cr_Matrix_identity(int size);
cr_Matrix cr_Matrix_matmul(cr_Matrix m1, cr_Matrix m2);
cr_Matrix cr_Matrix_transpose(cr_Matrix m);
cr_Matrix cr_Matrix_projection(cr_num camz, cr_num fov, cr_num aspect);
cr_Matrix cr_Matrix_viewport(cr_num x, cr_num y, cr_num w, cr_num h, cr_num d);
cr_Matrix cr_Matrix_from_vector(cr_Vec3 v);
cr_Matrix cr_Matrix_from_vectors(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2);
cr_Matrix cr_Matrix_from_vectors4(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2);
cr_Matrix cr_Matrix_from_vectors_col(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2);
cr_Matrix cr_Matrix_rotz(cr_num theta);
cr_Matrix cr_Matrix_roty(cr_num theta);
cr_Matrix cr_Matrix_rotx(cr_num theta);
cr_Matrix cr_Matrix_inverse(cr_Matrix matrix);
cr_Matrix cr_Matrix_translation(cr_Vec3 v);
cr_Matrix cr_Matrix_rotation(cr_Vec3 v);
cr_Matrix cr_Matrix_inverse_clean(cr_Matrix matrix);
cr_Matrix cr_Matrix_model_view(cr_Vec3 eye, cr_Vec3 center, cr_Vec3 up);
cr_Matrix cr_Matrix_clone(cr_Matrix m);
void cr_Matrix_dealloc(cr_Matrix *mat);
cr_Vec3 cr_Vec3_copy(cr_Vec3 v);
cr_Vec3 cr_Vec3_transform(cr_Vec3 v, cr_Matrix mat);
cr_Vec3 cr_Vec3_transform3(cr_Vec3 v, cr_Matrix mat);
void cr_Vec3_set_item(cr_Vec3 v, int i, cr_num a);
cr_num cr_Vec3_get_item(cr_Vec3 v, int i);
cr_Vec4 cr_Vec4_transform(cr_Vec3 v, cr_Matrix m);

typedef struct cr_Face {
  size_t vs[3]; // non-triangular faces are not supported
  size_t vts[3];
  size_t vns[3];
} cr_Face;
typedef struct cr_Vec3_dynarr {
  cr_Vec3 *items;
  size_t count;
  size_t capacity;
} cr_Vec3_dynarr;
typedef struct cr_Face_dynarr {
  cr_Face *items;
  size_t count;
  size_t capacity;
} cr_Face_dynarr;
typedef struct cr_Object {
  cr_Vec3_dynarr vertices;
  cr_Vec3_dynarr uvs;
  cr_Vec3_dynarr normals;
  cr_Face_dynarr faces;
  bool valid;
} cr_Object;
typedef enum cr_FaceTriType {
  VERTEX = 0,
  UV = 1,
  NORMAL = 2,
} cr_FaceTriType;
typedef enum cr_NormalPrecompMode {
  NONE = 0,
  FLAT = 1,
  SMOOTH = 2
} cr_NormalPrecompMode;
cr_Object cr_Object_new(void);
void cr_Object_add_vertex(cr_Object *object, cr_Vec3 vertex);
void cr_Object_add_uv(cr_Object *object, cr_Vec3 uv);
void cr_Object_add_normal(cr_Object *object, cr_Vec3 normal);
void cr_Object_add_face(cr_Object *object, cr_Face face);
void cr_Object_compute_vertex_tangents(cr_Object *object,
                                       cr_Vec3 *out_tangents);

void cr_Object_compute_smooth_normals(cr_Object *object, cr_Vec3 *out_normals);
void cr_Object_compute_flat_normals(cr_Object *object, cr_Vec3 *out_normals);
void cr_Object_dealloc(cr_Object *object);
bool cr_Object_writeOBJ(cr_Object *object, char *fn);
cr_Object cr_Object_readOBJ(char *fn, cr_NormalPrecompMode precompute_mode);

typedef struct cr_Triangle {
  cr_Vec3 v0;
  cr_Vec3 v1;
  cr_Vec3 v2;
} cr_Triangle;

cr_Triangle cr_Triangle_transform(cr_Triangle tri, cr_Matrix transform);

cr_Triangle cr_Triangle_transform3(cr_Triangle tri, cr_Matrix transform);
cr_Triangle cr_Triangle_transform4(cr_Triangle tri, cr_Matrix transform,
                                   cr_Vec3 *ws);
cr_Vec3 cr_Triangle_get_tangent(cr_Triangle *vs, cr_Triangle *uvs);
static inline cr_Vec3 cr_Triangle_get_normal(cr_Triangle vs) {
  cr_Vec3 e1 = cr_Vec3_sub(vs.v1, vs.v0), e2 = cr_Vec3_sub(vs.v2, vs.v0);
  return cr_Vec3_normalized(cr_Vec3_cross(e1, e2));
}
static inline cr_num cr_Triangle_get_area(cr_Triangle t) {
  return cr_Vec3_length(cr_Vec3_sub(t.v1, t.v0)) *
         cr_Vec3_length(cr_Vec3_sub(t.v2, t.v0)) * 0.5f;
}
cr_Triangle cr_Triangle_create(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2);

bool cr_Face_gettri(cr_Face *face, cr_Object *obj, cr_FaceTriType tt,
                    cr_Triangle *tri);

typedef struct cr_Texture {
  cr_Vec3 *m;
  int width;
  int height;
  bool valid;
} cr_Texture;
typedef uint32_t *cr_Linear_Texture;
cr_Texture cr_Texture_create(int width, int height, cr_Vec3 color);
cr_Texture cr_Texture_readPPM(char *fn);
cr_Texture cr_Texture_readPAM(char *fn);
cr_Texture cr_Texture_read(char *fn);
bool cr_Texture_writePPM(cr_Texture *texture, char *fn);
void cr_Texture_dealloc(cr_Texture *texture);
cr_Linear_Texture cr_Texture_to_linear(cr_Texture texture);
cr_Texture cr_Texture_bake_object_space_normal_map(cr_Texture *in,
                                                   cr_Object *object);
static inline cr_Vec3 cr_trinterpolate(cr_Triangle tri, cr_Vec3 bary) {
  cr_num bx = bary.x, by = bary.y, bz = bary.z;
  return (cr_Vec3){tri.v0.x * bx + tri.v1.x * by + tri.v2.x * bz,
                   tri.v0.y * bx + tri.v1.y * by + tri.v2.y * bz,
                   tri.v0.z * bx + tri.v1.z * by + tri.v2.z * bz};
}
static inline cr_Vec3 cr_barycentric(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2,
                                     cr_num px, cr_num py, cr_num denom) {
  cr_num x0 = v0.x, y0 = v0.y;
  cr_num x1 = v1.x, y1 = v1.y;
  cr_num x2 = v2.x, y2 = v2.y;
  if (fabs(denom) < 1e-12) {
    return (cr_Vec3){-1.0, 1.0, 1.0};
  }

  cr_num w1 = ((x2 - x0) * (py - y0) - (y2 - y0) * (px - x0)) * denom;

  cr_num w2 = ((y1 - y0) * (px - x0) - (x1 - x0) * (py - y0)) * denom;

  return (cr_Vec3){(cr_num)1.0 - w1 - w2, w1, w2};
}

static inline cr_num cr_fmin3(cr_num x, cr_num y, cr_num z) {
  return fmin(x, fmin(y, z));
}
static inline cr_num cr_fmax3(cr_num x, cr_num y, cr_num z) {
  return fmax(x, fmax(y, z));
}

typedef enum cr_TextureSetIndex {
  cr_TextureSetIndex_diffuse = 0,
  cr_TextureSetIndex_normal_map = 1,
  cr_TextureSetIndex_specular_map = 2,
  cr_TextureSetIndex_max = 3
} cr_TextureSetIndex;

typedef union cr_TextureSet {
  struct {
    cr_Texture *diffuse;
    cr_Texture *normal_map;
    cr_Texture *specular_map;
  };
  cr_Texture *textures[cr_TextureSetIndex_max];
} cr_TextureSet;
typedef struct cr_Entity {
  cr_Object *ob;
  cr_Matrix transform;
  cr_Matrix inverse_transform;
  cr_TextureSet ts;
  bool valid;
} cr_Entity;

typedef struct cr_Camera {
  cr_num fov;
  cr_num near_plane;
  cr_Vec3 eye;
  cr_Vec3 center;
  cr_Vec3 up;
} cr_Camera;

typedef struct cr_SceneSettings {
  cr_ShadingMode shading_mode;
  cr_SamplingMode sampling_mode;
  size_t render_width;
  size_t render_height;
  cr_num render_depth;
  cr_Camera camera;
  cr_Vec3 light_dir;
  bool use_normal_map;

} cr_SceneSettings;

typedef struct cr_Entities {
  size_t count;
  size_t capacity;
  cr_Entity **items;
} cr_Entities;

typedef struct cr_Scene {
  cr_SceneSettings settings;
  cr_SceneSettings __internal_settings_cache;
  cr_Entities entities;
  size_t buffer_size;
  cr_Linear_Texture framebuffer;
  cr_num *zbuffer;
  omp_lock_t *zbuffer_locks;
  cr_Matrix projection;
  cr_Matrix viewport;
  cr_Matrix model_view;
  cr_Matrix inverse_model_view;
  cr_Matrix world_transform;
  cr_Texture default_texture;
  bool valid;
} cr_Scene;
cr_Entity cr_Entity_create(cr_Object *ob);
void cr_Entity_dealloc(cr_Entity *e);
void cr_Entity_detach_texture(cr_Entity *e, size_t index);
void cr_Entity_attach_texture(cr_Entity *e, size_t index, cr_Texture *texture);
void cr_Entity_set_transform(cr_Entity *e, cr_Matrix transform);
void cr_Entity_reset_transform(cr_Entity *e);
void cr_Entity_add_transform(cr_Entity *e, cr_Matrix transform);
void cr_Entity_translate(cr_Entity *e, cr_Vec3 delta);
void cr_Entity_translate_world_space(cr_Entity *e, cr_Vec3 delta);
void cr_Entity_rotate(cr_Entity *e, cr_Vec3 thetas);
void cr_Entity_rotate_world_space(cr_Entity *e, cr_Vec3 thetas);
cr_Matrix cr_Entity_get_world_space_transform(cr_Entity *e,
                                              cr_Matrix transform);
cr_Scene cr_Scene_create(cr_SceneSettings settings);
void cr_Scene_add_entity(cr_Scene *s, cr_Entity *e);
void cr_Scene_remove_entity(cr_Scene *s, cr_Entity *e);
void cr_Scene_rebuild_transform(cr_Scene *s);
int cr_Scene_init(cr_Scene *s);
int cr_Scene_update_settings(cr_Scene *s);
int cr_Scene_resize(cr_Scene *s, size_t new_width, size_t new_height);
void cr_Scene_dealloc(cr_Scene *s);
void cr_Scene_reset_buffers(cr_Scene *s);
void cr_Scene_render(cr_Scene *s, int num_threads);

bool cr_Entity_uses_texture(cr_Entity *e, cr_Texture *t);
bool cr_Scene_uses_texture(cr_Scene *s, cr_Texture *t);

typedef bool(*cr_Texture_draw_face_tp) _cr_Texture_draw_face_ARGS;
_cr_Texture_draw_face_FORALL(_cr_Texture_draw_face_DECLH)

// end of the interesting part

#ifdef cr_STRIP_SYMS
#define EPSILON cr_EPSILON
#define VIEWPORT_DEPTH cr_VIEWPORT_DEPTH
#define AMBIENT cr_AMBIENT
#define SCHEDULE cr_SCHEDULE
#define ENTITIES_INITIAL_CAPACITY cr_ENTITIES_INITIAL_CAPACITY
#define ABORT cr_ABORT
#define ERROR cr_ERROR
#define UNREACHABLE cr_UNREACHABLE
#define ASSERT cr_ASSERT
#define INIT_CRENDER cr_INIT_CRENDER
#define DYNARR_PUSH cr_DYNARR_PUSH
#define DYNARR_REMOVE_SWAP_LAST cr_DYNARR_REMOVE_SWAP_LAST
#define DYNARR_REMOVE_KEEP_ORDER cr_DYNARR_REMOVE_KEEP_ORDER
#define DYNARR_FROMARR cr_DYNARR_FROMARR
#define DYNARR_DEALLOC cr_DYNARR_DEALLOC
#define clamp cr_clamp
#define clamplo cr_clamplo
#define Vec3_ADD_INPLACE3 cr_Vec3_ADD_INPLACE3
#define Vec3_ADD_INPLACE cr_Vec3_ADD_INPLACE
#define Vec3_NEG_INPLACE cr_Vec3_NEG_INPLACE
#define ShadingMode cr_ShadingMode
#define FaceTriType cr_FaceTriType
#define SamplingMode cr_SamplingMode
#define num cr_num
#define NUM_FMT cr_NUM_FMT
#define Linear_Texture cr_Linear_Texture
#define Vec2 cr_Vec2
#define Vec3 cr_Vec3
#define Vec4 cr_Vec4
#define Matrix cr_Matrix
#define Face cr_Face
#define NormalPrecompMode cr_NormalPrecompMode
#define Object cr_Object
#define Triangle cr_Triangle
#define Texture cr_Texture
#define TextureSet cr_TextureSet
#define TextureSetIndex cr_TextureSetIndex
#define TextureSetIndex_diffuse cr_TextureSetIndex_diffuse
#define TextureSetIndex_normal_map cr_TextureSetIndex_normal_map
#define TextureSetIndex_max cr_TextureSetIndex_max
#define Entity cr_Entity
#define SceneSettings cr_SceneSettings
#define Entities cr_Entities
#define Scene cr_Scene
#define Vec2_create cr_Vec2_create
#define Vec3_create cr_Vec3_create
#define Vec3_cross cr_Vec3_cross
#define Vec3_dot cr_Vec3_dot
#define Vec3_add cr_Vec3_add
#define Vec3_neg cr_Vec3_neg
#define Vec3_sub cr_Vec3_sub
#define Vec3_mul cr_Vec3_mul
#define Vec3_div cr_Vec3_div
#define Vec3_length cr_Vec3_length
#define Vec3_normalized cr_Vec3_normalized
#define Vec3_from_matrix cr_Vec3_from_matrix
#define Vec3_from_matrix3 cr_Vec3_from_matrix3
#define Vec3_phong cr_Vec3_phong
#define Vec3_pack_color cr_Vec3_pack_color
#define Vec3_normal_from_color cr_Vec3_normal_from_color
#define Vec3_normal_as_color cr_Vec3_normal_as_color
#define Vec3_normalize cr_Vec3_normalize
#define trinterpolate cr_trinterpolate
#define barycentric cr_barycentric
#define fmin3 cr_fmin3
#define fmax3 cr_fmax3
#define Vec3_transform_dir cr_Vec3_transform_dir
#define Matrix_print cr_Matrix_print
#define Matrix_empty cr_Matrix_empty
#define Matrix_identity cr_Matrix_identity
#define Matrix_matmul cr_Matrix_matmul
#define Matrix_projection cr_Matrix_projection
#define Matrix_viewport cr_Matrix_viewport
#define Matrix_from_vector cr_Matrix_from_vector
#define Matrix_from_vectors cr_Matrix_from_vectors
#define Matrix_from_vectors4 cr_Matrix_from_vectors4
#define Matrix_from_vectors_col cr_Matrix_from_vectors_col
#define Matrix_rotz cr_Matrix_rotz
#define Matrix_roty cr_Matrix_roty
#define Matrix_rotx cr_Matrix_rotx
#define Matrix_inverse cr_Matrix_inverse
#define Matrix_translation cr_Matrix_translation
#define Matrix_rotation cr_Matrix_rotation
#define Matrix_model_view cr_Matrix_model_view
#define Matrix_inverse_clean cr_Matrix_inverse_clean
#define Matrix_dealloc cr_Matrix_dealloc
#define Matric_clone cr_Matrix_clone
#define Vec3_copy cr_Vec3_copy
#define Vec3_transform cr_Vec3_transform
#define Vec3_transform3 cr_Vec3_transform3
#define Vec3_set_item cr_Vec3_set_item
#define Vec3_get_item cr_Vec3_get_item
#define Vec4_transform cr_Vec4_transform
#define Object_new cr_Object_new
#define Object_add_vertex cr_Object_add_vertex
#define Object_add_uv cr_Object_add_uv
#define Object_add_normal cr_Object_add_normal
#define Object_add_face cr_Object_add_face
#define Object_compute_vertex_tangents cr_Object_compute_vertex_tangents
#define Object_compute_smooth_normals cr_Object_compute_vertex_normals
#define Object_compute_flat_normals cr_Object_compute_vertex_normals
#define Object_dealloc cr_Object_dealloc
#define Object_writeOBJ cr_Object_writeOBJ
#define Object_readOBJ cr_Object_readOBJ
#define Triangle_transform cr_Triangle_transform
#define Triangle_transform3 cr_Triangle_transform3
#define Triangle_transform4 cr_Triangle_transform4
#define Triangle_create cr_Triangle_create
#define Triangle_get_tangent cr_Triangle_get_tangent
#define Triangle_get_normal cr_Triangle_get_normal
#define Triangle_get_area cr_Triangle_get_area
#define Face_gettri cr_Face_gettri
#define Face_dealloc cr_Face_dealloc
#define Texture_create cr_Texture_create
#define Texture_readPPM cr_Texture_readPPM
#define Texture_readPAM cr_Texture_readPAM
#define Texture_read cr_Texture_read
#define Texture_writePPM cr_Texture_writePPM
#define Texture_dealloc cr_Texture_dealloc
#define Texture_to_linear cr_Texture_to_linear
#define Texture_bake_object_space_normal_map                                   \
  cr_Texture_bake_object_space_normal_map
#define Entity_create cr_Entity_create
#define Entity_dealloc cr_Entity_dealloc
#define Entity_detach_texture cr_Entity_detach_texture
#define Entity_attach_texture cr_Entity_attach_texture
#define Entity_set_transform cr_Entity_set_transform
#define Entity_reset_transform cr_Entity_reset_transform
#define Entity_add_transform cr_Entity_add_transform
#define Entity_translate cr_Entity_translate
#define Entity_translate_world_space cr_Entity_translate_world_space
#define Entity_rotate cr_Entity_rotate
#define Entity_rotate_world_space cr_Entity_rotate_world_space
#define Entity_get_world_space_transform cr_Entity_get_world_space_transform
#define Scene_create cr_Scene_create
#define Scene_add_entity cr_Scene_add_entity
#define Scene_remove_entity cr_Scene_remove_entity
#define Scene_rebuild_transform cr_Scene_rebuild_transform
#define Scene_init cr_Scene_init
#define Scene_update_settings cr_Scene_update_settings
#define Scene_resize cr_Scene_resize
#define Scene_dealloc cr_Scene_dealloc
#define Scene_reset_buffers cr_Scene_reset_buffers
#define Scene_render cr_Scene_render
#define Scene_uses_texture cr_Scene_uses_texture
#define Entity_uses_texture cr_Entity_uses_texture
#endif
#endif
