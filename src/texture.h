#ifndef _TEXTURE_H
#define _TEXTURE_H
#include"vec.h"
#include"tri.h"
#include"obj.h"
typedef struct Texture{
	int width;
	int height;
	Vec3 **m;
} Texture;

Texture *Texture_create(int width, int height, Vec3 color);
Texture *Texture_readPPM(char *fn);
Texture *Texture_readPAM(char *fn);
Texture *Texture_read(char *fn);
void Texture_draw_face(Texture *texture,Face *face, Texture *diffuse, Texture *normal_map, Texture *specular_map,  double *zbuffer, Vec3 light_dir, Matrix transform, Matrix world_transform, Matrix inverse_transform); 
void Texture_writePPM(Texture *texture, char *fn);
void Texture_dealloc(Texture *texture);
#endif
