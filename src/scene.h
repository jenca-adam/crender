#ifndef _SCENE_H
#define _SCENE_H
#include<stdbool.h>
#include "obj.h"
#include "texture.h"
typedef struct TextureSet{
   Texture diffuse;
  Texture normal_map;
 Texture specular_map; 
} TextureSet;
typedef struct Entity{
  Object *ob;
  TextureSet ts;
} Entity;

Entity Entity_new();
bool Entity_load_dir(Entity *e, char *dirname);
#endif
