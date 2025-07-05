#include "color.h"
#include "core.h"
#include "display.h"
#include "obj.h"
#include "texture.h"
#include "tri.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define WIDTH 256
#define HEIGHT 256
#define RW 1024
#define RH 1024
#define DEPTH 1024
#define CAM_Z 3
#define ROTATE 90
int main() {
  const int width = WIDTH;
  const int height = HEIGHT;
  const double cam_z = CAM_Z;

  Vec3 light_dir = Vec3_create(0, 0, -1);
  Vec3 bg = Vec3_create(0, 0, 0);
  Vec3 fg = Vec3_create(255, 255, 0);
  Matrix projection = Matrix_projection(cam_z);
  Matrix viewport = Matrix_viewport(width / 8, height / 8, width * 3 / 4,
                                    height * 3 / 4, DEPTH);
  Matrix projection_x_viewport = Matrix_matmul(viewport, projection);

  Texture *texture = Texture_create(WIDTH, HEIGHT, bg);
  Texture *obj_texture = Texture_readPPM("obj_texture.ppm");
  Texture *normal_map = Texture_readPPM("normal.ppm");
  Texture *specular_map = Texture_readPPM("specular.ppm");
  Object *object = Object_fromOBJ("obj.obj");

  initDisplay(RW, RH, WIDTH, HEIGHT, "renderer");
  if (!object) {
    return 1;
  }
  printf("%d\n", object->nf);

  double *zbuffer = malloc(width * height * sizeof(double));
  for (int i = 0; i < width * height; i++) {
    zbuffer[i] = -INT_MAX;
  }
#if 0
  printf("render start\n");
#pragma omp parallel for
  for (int fi = 0; fi < object->nf; fi++) {
    Face *face = object->faces[fi];

    Texture_draw_face(texture, face, obj_texture, normal_map, specular_map,
                      zbuffer, light_dir, transform, inverse);
  }
  renderTexture(texture);
  printf("render end\n");
  Texture_writePPM(texture, "out2.ppm");
#endif // 0
  SDL_Event event;
  Matrix rot, transform, inverse;
  int running = 1;
  int nframes = 0;
  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
        break;
      }
    }

    Matrix rot = Matrix_roty(ROTATE + ((float)nframes) / 10.f);

    Matrix inverse = Matrix_inverse(rot);
    Matrix transform = Matrix_matmul(projection_x_viewport, rot);
    printf("frame %d\n", nframes);
    nframes++;
    clearDisplay(bg);
    memset(zbuffer, '\x00', width * height * sizeof(double));
    for (int fi = 0; fi < object->nf; fi++) {
      Face *face = object->faces[fi];

      Texture_draw_face(NULL, face, obj_texture, normal_map, specular_map,
                        zbuffer, light_dir, transform, rot, inverse);
    }
    Matrix_dealloc(rot);
    Matrix_dealloc(inverse);
    Matrix_dealloc(transform);

    updateDisplay();
    // renderTexture(texture);
  }
  Texture_dealloc(texture);
  Texture_dealloc(obj_texture);
  Texture_dealloc(specular_map);
  Texture_dealloc(normal_map);
  Object_dealloc(object);
  free(zbuffer);
  Matrix_dealloc(projection);
  Matrix_dealloc(viewport);
}
