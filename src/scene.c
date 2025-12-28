#include "scene.h"
#include <assert.h>
#include <float.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
Entity Entity_create() { return (Entity){0}; }

bool Entity_load_dir(Entity *e, char *dirname) {
  Texture diffuse = Texture_readPPM("diffuse.ppm", dirname);
  Object *ob = Object_fromOBJ("obj.obj", dirname);
  if (!ob || !diffuse.m) {
    return false;
  }
  e->name = dirname;
  e->ob = ob;
  e->ts.diffuse = diffuse;
  e->ts.normal_map = Texture_readPPM("normal.ppm", dirname);
  e->ts.specular_map = Texture_readPPM("specular.ppm", dirname);
  e->transform = Matrix_identity(4);
  e->inverse_transform = Matrix_identity(4);
  return true;
}

void Entity_set_transform(Entity *e, Matrix transform) {
  Matrix inv = Matrix_inverse(transform);
  Matrix_dealloc(e->transform);
  Matrix_dealloc(e->inverse_transform);
  e->transform = transform;
  e->inverse_transform = inv;
}
void Entity_reset_transform(Entity *e) {
  Entity_set_transform(e, Matrix_identity(4));
}
void Entity_add_transform(Entity *e, Matrix transform) {
  Matrix new_transform = Matrix_matmul(e->transform, transform);
  Entity_set_transform(e, new_transform);
}
void Entity_translate(Entity *e, Vec3 delta) {
  Matrix translation = Matrix_translation(delta);
  Entity_add_transform(e, translation);
  Matrix_dealloc(translation);
}

void Entity_translate_world_space(Entity *e, Vec3 delta) {
  Matrix translation = Matrix_translation(delta);
  Matrix m = Matrix_matmul(e->inverse_transform, translation);
  Matrix n = Matrix_matmul(m, e->transform);
  Entity_add_transform(e, n);
  Matrix_dealloc(translation);
  Matrix_dealloc(m);
  Matrix_dealloc(n);
}
void Entity_rotate(Entity *e, Vec3 thetas) {

  Matrix rot = Matrix_rotation(thetas);
  Entity_add_transform(e, rot);
  Matrix_dealloc(rot);
}
Scene Scene_create(SceneSettings settings) {
  Scene sc = {0};
  sc.settings = settings;
  sc.__internal_settings_cache.render_width = 0;
  sc.__internal_settings_cache.render_height = 0;
  sc.framebuffer = NULL;
  sc.zbuffer = NULL;
  sc.zbuffer_locks = NULL;
  return sc;
}

void Scene_add_entity(Scene *s, Entity *e) {
  if (s->entities.count + 1 > s->entities.capacity) {

    if (s->entities.capacity == 0) {
      s->entities.capacity = ENTITIES_INITIAL_CAPACITY;
    }
    while (s->entities.count + 1 > s->entities.capacity) {
      s->entities.capacity *= 2;
    }
    s->entities.items =
        realloc(s->entities.items, s->entities.capacity * sizeof(Entity *));
  }
  s->entities.items[s->entities.count++] = e;
}
void Scene_remove_entity(Scene *s, Entity *e) {
  for (size_t i = 0; i < s->entities.count; i++) {
    if (s->entities.items[i]->name == e->name) {
      s->entities.items[i] = s->entities.items[s->entities.count - 1];
      s->entities.count--;
    }
  }
}
void Scene_rebuild_transform(Scene *s) {
  SceneSettings settings = s->settings;
  num aspect = (num)settings.render_width / settings.render_height;
  s->projection = Matrix_projection(settings.cam_z, settings.fov, aspect);
  s->viewport = Matrix_viewport(0, 0, settings.render_width,
                                settings.render_height, settings.render_depth);
  s->world_transform = Matrix_matmul(s->viewport, s->projection);
}

int Scene_init(Scene *s) { return Scene_update_settings(s); }
int Scene_update_settings(Scene *s) {
  return Scene_resize(s, s->settings.render_width, s->settings.render_height);
}

int Scene_resize(Scene *s, size_t new_width, size_t new_height) {
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
  num *zbuffer = malloc(res * sizeof(*s->zbuffer));
  assert(zbuffer != NULL);
  for (size_t i = 0; i < res; i++) {
    zbuffer[i] = -FLT_MAX;
  }
  s->zbuffer = zbuffer;
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
  Scene_rebuild_transform(s);
  return 1;
}
void Scene_reset_buffers(Scene *s) {
  for (size_t i = 0; i < s->buffer_size; i++) {
    s->zbuffer[i] = -FLT_MAX;
    s->framebuffer[i] = 0;
  }
}
static inline bool __SceneSettings_eq(SceneSettings a, SceneSettings b) {
  return (
      a.mode == b.mode && a.render_width == b.render_width &&
      a.render_height == b.render_height && a.render_depth == b.render_depth &&
      a.cam_z == b.cam_z && a.fov == b.fov && a.near_plane == b.near_plane &&
      a.light_dir.x == b.light_dir.x && a.light_dir.y == b.light_dir.y &&
      a.light_dir.z == b.light_dir.z && a.use_normal_map == b.use_normal_map);

  ;
}
void Scene_render(Scene *s, int num_threads) {
  if (!__SceneSettings_eq(s->settings, s->__internal_settings_cache)) {
    Scene_update_settings(s);
  }

  LinearTexture framebuffer = s->framebuffer;
  Vec3 light_dir = s->settings.light_dir;
  shading_mode mode = s->settings.mode;
  num near_plane = s->settings.near_plane;
  num *zbuffer = s->zbuffer;
  omp_lock_t *zbuffer_locks = s->zbuffer_locks;
  num rw = s->settings.render_width, rh = s->settings.render_height;
  bool use_normal_map = s->settings.use_normal_map;
  omp_set_num_threads(num_threads);
  for (size_t i = 0; i < s->entities.count; i++) {
    Entity entity = *s->entities.items[i];
    Object *ob = entity.ob;
    Texture diffuse = entity.ts.diffuse;
    Texture normal_map = use_normal_map ? entity.ts.normal_map : (Texture){0};
    Texture specular_map = entity.ts.specular_map;
    Matrix t = Matrix_matmul(s->world_transform, entity.transform);
#ifndef NO_MULTITHREAD
#pragma omp parallel for schedule(SCHEDULE)
#endif
    for (int fi = 0; fi < ob->nf; fi++) {
      Face *face = ob->faces[fi];
      Texture_draw_face(framebuffer, rw, rh, face, diffuse, normal_map,
                        specular_map, zbuffer, zbuffer_locks, light_dir, t,
                        entity.transform, entity.inverse_transform, near_plane,
                        mode);
    }
    Matrix_dealloc(t);
  }
}
void Entity_dealloc(Entity e) {
  Object_dealloc(e.ob);
  Matrix_dealloc(e.transform);
  Matrix_dealloc(e.inverse_transform);
  Texture_dealloc(e.ts.diffuse);
  Texture_dealloc(e.ts.normal_map);
  Texture_dealloc(e.ts.specular_map);
}
void Scene_dealloc(Scene *s) {
  free(s->framebuffer);
  free(s->zbuffer);
  for (size_t i = 0; i < s->buffer_size; i++) {
    omp_destroy_lock(&s->zbuffer_locks[i]);
  }
  free(s->zbuffer_locks);
  for (size_t i = 0; i < s->entities.count; i++) {
    Entity_dealloc(*s->entities.items[i]);
  }
  free(s->entities.items);
  Matrix_dealloc(s->projection);
  Matrix_dealloc(s->viewport);
  Matrix_dealloc(s->world_transform);
}
