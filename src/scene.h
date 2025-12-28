#ifndef _SCENE_H
#define _SCENE_H
#include<stdbool.h>
#include<stddef.h>
#include<omp.h>
#include "obj.h"
#include "texture.h"
#include "common.h"
#define ENTITIES_INITIAL_CAPACITY 8
typedef struct TextureSet{
   Texture diffuse;
  Texture normal_map;
 Texture specular_map; 
} TextureSet;
typedef struct Entity{
  Object *ob;
  Matrix transform;
  Matrix inverse_transform;
  TextureSet ts;
  const char *name;
} Entity;

typedef struct SceneSettings{
  shading_mode mode;
  size_t render_width;
  size_t render_height;
  num render_depth; 
  num cam_z;
  num fov;
  num near_plane;
  Vec3 light_dir;
  bool use_normal_map;
} SceneSettings;
typedef struct Entities {
  size_t count;
  size_t capacity;
  Entity **items;
} Entities;
typedef struct Scene{
  SceneSettings settings;
  SceneSettings __internal_settings_cache;
  Entities entities;
  size_t buffer_size;
  LinearTexture framebuffer;
  num *zbuffer;
  omp_lock_t *zbuffer_locks;
  Matrix projection;
  Matrix viewport;
  Matrix world_transform;
  Matrix inverse_world_transform;
  
} Scene;
Entity Entity_create(void);
bool Entity_load_dir(Entity *e, char *dirname);
void Entity_set_transform(Entity *e, Matrix transform);
void Entity_reset_transform(Entity *e);
void Entity_add_transform(Entity *e, Matrix transform);
void Entity_translate(Entity *e, Vec3 delta) ;
void Entity_translate_world_space(Entity *e, Vec3 delta);
void Entity_rotate(Entity *e, Vec3 thetas);
Scene Scene_create(SceneSettings settings);
void Scene_add_entity(Scene *s, Entity *e);
void Scene_remove_entity(Scene *s, Entity *e);
void Scene_rebuild_transform(Scene *s);
int Scene_init(Scene *s);
int Scene_update_settings(Scene *s);
int Scene_resize(Scene *s, size_t new_width, size_t new_height);
void Scene_dealloc(Scene *s);
void Scene_reset_buffers(Scene *s);
void Scene_render(Scene *s, int num_threads);
#endif
