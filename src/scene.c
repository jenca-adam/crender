#include "crender.h"
#include <assert.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
cr_Entity cr_Entity_create() { return (cr_Entity){0}; }

bool cr_Entity_load_dir(cr_Entity *e, char *dirname) {
  cr_Texture diffuse = cr_Texture_readPPM("diffuse.ppm", dirname);
  cr_Object *ob = cr_Object_fromOBJ("obj.obj", dirname);
  if (!ob || !diffuse.m) {
    return false;
  }
  e->name = dirname;
  e->ob = ob;
  e->ts.diffuse = diffuse;
  e->ts.normal_map = cr_Texture_readPPM("normal.ppm", dirname);
  e->ts.specular_map = cr_Texture_readPPM("specular.ppm", dirname);
  e->transform = cr_Matrix_identity(4);
  e->inverse_transform = cr_Matrix_identity(4);
  return true;
}

void cr_Entity_detach_texture(cr_Entity *e, size_t index) {
  assert(index < cr_TextureSetIndex_max);
  e->ts.textures[index] = (cr_Texture){0};
}

void cr_Entity_attach_texture(cr_Entity *e, size_t index, cr_Texture texture) {
  assert(index < cr_TextureSetIndex_max);
  cr_Entity_detach_texture(e, index);
  e->ts.textures[index] = texture;
}

bool cr_Entity_uses_texture(cr_Entity *e, cr_Texture texture) {
  for (size_t i = 0; i < cr_TextureSetIndex_max; i++) {
    if (e->ts.textures[i].m == texture.m) {
      return true;
    }
  }
  return false;
}

void cr_Entity_set_transform(cr_Entity *e, cr_Matrix transform) {
  cr_Matrix inv = cr_Matrix_inverse(transform);
  cr_Matrix_dealloc(e->transform);
  cr_Matrix_dealloc(e->inverse_transform);
  e->transform = transform;
  e->inverse_transform = inv;
}
void cr_Entity_reset_transform(cr_Entity *e) {
  cr_Entity_set_transform(e, cr_Matrix_identity(4));
}
void cr_Entity_add_transform(cr_Entity *e, cr_Matrix transform) {
  cr_Matrix new_transform = cr_Matrix_matmul(e->transform, transform);
  cr_Entity_set_transform(e, new_transform);
}
void cr_Entity_translate(cr_Entity *e, cr_Vec3 delta) {
  cr_Matrix translation = cr_Matrix_translation(delta);
  cr_Entity_add_transform(e, translation);
  cr_Matrix_dealloc(translation);
}

void cr_Entity_translate_world_space(cr_Entity *e, cr_Vec3 delta) {
  cr_Matrix translation = cr_Matrix_translation(delta);
  cr_Matrix ws_translation =
      cr_Entity_get_world_space_transform(e, translation);
  cr_Entity_add_transform(e, ws_translation);
  cr_Matrix_dealloc(translation);
  cr_Matrix_dealloc(ws_translation);
}

void cr_Entity_rotate(cr_Entity *e, cr_Vec3 thetas) {
  cr_Matrix rotation = cr_Matrix_rotation(thetas);
  cr_Entity_add_transform(e, rotation);
  cr_Matrix_dealloc(rotation);
}

void cr_Entity_rotate_world_space(cr_Entity *e, cr_Vec3 thetas) {
  cr_Matrix rotation = cr_Matrix_rotation(thetas);
  cr_Matrix ws_rotation = cr_Entity_get_world_space_transform(e, rotation);
  cr_Entity_add_transform(e, rotation);
  cr_Matrix_dealloc(rotation);
  cr_Matrix_dealloc(ws_rotation);
}

cr_Matrix cr_Entity_get_world_space_transform(cr_Entity *e,
                                              cr_Matrix transform) {
  cr_Matrix m = cr_Matrix_matmul(e->inverse_transform, transform);
  cr_Matrix n = cr_Matrix_matmul(m, e->transform);
  cr_Matrix_dealloc(m);
  return n;
}
cr_Scene cr_Scene_create(cr_SceneSettings settings) {
  cr_Scene sc = {0};
  sc.settings = settings;
  sc.__internal_settings_cache.render_width = 0;
  sc.__internal_settings_cache.render_height = 0;
  sc.framebuffer = NULL;
  sc.zbuffer = NULL;
  sc.zbuffer_locks = NULL;
  return sc;
}

void cr_Scene_add_entity(cr_Scene *s, cr_Entity *e) {
  if (s->entities.count + 1 > s->entities.capacity) {

    if (s->entities.capacity == 0) {
      s->entities.capacity = cr_ENTITIES_INITIAL_CAPACITY;
    }
    while (s->entities.count + 1 > s->entities.capacity) {
      s->entities.capacity *= 2;
    }
    s->entities.items =
        realloc(s->entities.items, s->entities.capacity * sizeof(cr_Entity *));
  }
  s->entities.items[s->entities.count++] = e;
}
void cr_Scene_remove_entity(cr_Scene *s, cr_Entity *e) {
  for (size_t i = 0; i < s->entities.count; i++) {
    if (s->entities.items[i] == e) {
      s->entities.items[i] = s->entities.items[s->entities.count - 1];
      s->entities.count--;
    }
  }
}
bool cr_Scene_uses_texture(cr_Scene *s, cr_Texture t) {
  for (size_t i = 0; i < s->entities.count; i++) {
    if (cr_Entity_uses_texture(s->entities.items[i], t)) {
      return true;
    }
  }
  return false;
}
void cr_Scene_rebuild_transform(cr_Scene *s) {
  cr_SceneSettings settings = s->settings;
  cr_num aspect = (cr_num)settings.render_width / settings.render_height;
  s->projection = cr_Matrix_projection(settings.cam_z, settings.fov, aspect);
  s->viewport =
      cr_Matrix_viewport(0, 0, settings.render_width, settings.render_height,
                         settings.render_depth);
  s->world_transform = cr_Matrix_matmul(s->viewport, s->projection);
}

int cr_Scene_init(cr_Scene *s) { return cr_Scene_update_settings(s); }
int cr_Scene_update_settings(cr_Scene *s) {
  return cr_Scene_resize(s, s->settings.render_width,
                         s->settings.render_height);
}

int cr_Scene_resize(cr_Scene *s, size_t new_width, size_t new_height) {
  size_t res = new_width * new_height, old_res = s->buffer_size;
  s->buffer_size = res;
  s->__internal_settings_cache = s->settings;
  s->settings.render_width = new_width;
  s->settings.render_height = new_height;
  if (s->framebuffer)
    free(s->framebuffer);
  s->framebuffer = calloc(res, sizeof(*s->framebuffer));
  assert(s->framebuffer != NULL);
  if (s->zbuffer)
    free(s->zbuffer);
  cr_num *zbuffer = malloc(res * sizeof(*s->zbuffer));
  assert(zbuffer != NULL);
  for (size_t i = 0; i < res; i++) {
    zbuffer[i] = -FLT_MAX;
  }
  s->zbuffer = zbuffer;
#ifndef NO_MULTITHREAD
  if (s->zbuffer_locks) {
    for (size_t i = 0; i < old_res; i++) {
      omp_destroy_lock(&s->zbuffer_locks[i]);
    }
    free(s->zbuffer_locks);
  }
  s->zbuffer_locks = malloc(res * sizeof(*s->zbuffer_locks));
  assert(s->zbuffer_locks != NULL);
  for (size_t i = 0; i < res; i++) {
    omp_init_lock(&s->zbuffer_locks[i]);
  }
#endif
  cr_Scene_rebuild_transform(s);
  return 1;
}
void cr_Scene_reset_buffers(cr_Scene *s) {
  for (size_t i = 0; i < s->buffer_size; i++) {
    s->zbuffer[i] = -FLT_MAX;
    s->framebuffer[i] = 0;
  }
}
static inline bool __cr_SceneSettings_eq(cr_SceneSettings a,
                                         cr_SceneSettings b) {
  return (
      a.mode == b.mode && a.render_width == b.render_width &&
      a.render_height == b.render_height && a.render_depth == b.render_depth &&
      a.cam_z == b.cam_z && a.fov == b.fov && a.near_plane == b.near_plane &&
      a.light_dir.x == b.light_dir.x && a.light_dir.y == b.light_dir.y &&
      a.light_dir.z == b.light_dir.z && a.use_normal_map == b.use_normal_map);

  ;
}
void cr_Scene_render(cr_Scene *s, int num_threads) {
  if (!__cr_SceneSettings_eq(s->settings, s->__internal_settings_cache)) {
    cr_Scene_update_settings(s);
  }

  cr_Linear_Texture framebuffer = s->framebuffer;
  cr_Vec3 light_dir = s->settings.light_dir;
  cr_shading_mode mode = s->settings.mode;
  cr_num near_plane = s->settings.near_plane;
  cr_num *zbuffer = s->zbuffer;
  omp_lock_t *zbuffer_locks = s->zbuffer_locks;
  cr_num rw = s->settings.render_width, rh = s->settings.render_height;
  bool use_normal_map = s->settings.use_normal_map;
#ifndef NO_MULTITHREAD
  omp_set_num_threads(num_threads);
#endif
  for (size_t i = 0; i < s->entities.count; i++) {
    cr_Entity entity = *s->entities.items[i];
    cr_Object *ob = entity.ob;
    cr_Texture diffuse = entity.ts.diffuse;
    cr_Texture normal_map =
        use_normal_map ? entity.ts.normal_map : (cr_Texture){0};
    cr_Texture specular_map = entity.ts.specular_map;
    cr_Matrix t = cr_Matrix_matmul(s->world_transform, entity.transform);
#ifndef NO_MULTITHREAD
#pragma omp parallel for schedule(cr_SCHEDULE)
#endif
    for (int fi = 0; fi < ob->nf; fi++) {
      cr_Face *face = ob->faces[fi];
      cr_Texture_draw_face(framebuffer, rw, rh, face, diffuse, normal_map,
                           specular_map, zbuffer, zbuffer_locks, light_dir, t,
                           entity.transform, entity.inverse_transform,
                           near_plane, mode);
    }
    cr_Matrix_dealloc(t);
  }
}
void cr_Entity_dealloc(cr_Entity e) {
  cr_Object_dealloc(e.ob);
  cr_Matrix_dealloc(e.transform);
  cr_Matrix_dealloc(e.inverse_transform);
  cr_Texture_dealloc(e.ts.diffuse);
  cr_Texture_dealloc(e.ts.normal_map);
  cr_Texture_dealloc(e.ts.specular_map);
}
void cr_Scene_dealloc(cr_Scene *s) {
  free(s->framebuffer);
  free(s->zbuffer);
#ifndef NO_MULTITHREAD
  for (size_t i = 0; i < s->buffer_size; i++) {
    omp_destroy_lock(&s->zbuffer_locks[i]);
  }
  free(s->zbuffer_locks);
#endif
  free(s->entities.items);
  cr_Matrix_dealloc(s->projection);
  cr_Matrix_dealloc(s->viewport);
  cr_Matrix_dealloc(s->world_transform);
}
