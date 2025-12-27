#ifndef _TEXTURE_H
#define _TEXTURE_H
#include"vec.h"
#include"tri.h"
#include"obj.h"
#include<stdint.h>
#include<omp.h>
typedef struct Texture{
	int width;
	int height;
	Vec3 **m;
} Texture;
typedef uint32_t *LinearTexture;
Texture *Texture_create(int width, int height, Vec3 color);
Texture *Texture_readPPM(char *fn, char *dirname);
Texture *Texture_readPAM(char *fn);
Texture *Texture_read(char *fn);
void Texture_draw_face(LinearTexture texture, int width, int height, Face *face, Texture *diffuse, Texture *normal_map, Texture *specular_map,  double *zbuffer, omp_lock_t *zbuffer_locks, Vec3 light_dir, Matrix transform, Matrix world_transform, Matrix inverse_transform, double near_plane, shading_mode mode); 
void Texture_writePPM(Texture *texture, char *fn);
void Texture_dealloc(Texture *texture);
LinearTexture Texture_to_linear(Texture *texture);
#endif
