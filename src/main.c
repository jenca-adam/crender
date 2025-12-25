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
#define WIDTH 1200
#define HEIGHT 1200
#define RW 1200
#define RH 1200
#define DEPTH 1024
#define CAM_Z 3
#define ROTATE 1
int main() {
  const int width = WIDTH;
  const int height = HEIGHT;
  const double cam_z = CAM_Z;

  Vec3 light_dir = Vec3_create(0, 0, -1);
  Vec3 bg = Vec3_create(0, 0, 0);
  Vec3 fg = Vec3_create(255, 255, 0);
  Matrix projection = Matrix_projection(cam_z);
  Matrix viewport = Matrix_viewport(width / 8., height / 8., width * 3 / 4.,
                                    height * 3 / 4., DEPTH);
  Matrix projection_x_viewport = Matrix_matmul(viewport, projection);

  Texture *texture = Texture_create(WIDTH, HEIGHT, bg);
  Texture *obj_texture = Texture_readPPM("obj_texture.ppm");
  Texture *normal_map = Texture_readPPM("normal.ppm");
  Texture *specular_map = Texture_readPPM("specular.ppm");
  Object *object = Object_fromOBJ("obj.obj");

  if (!initDisplay(RW, RH, WIDTH, HEIGHT, "renderer")) {
    return 1;
  };
  if (!object) {
    return 1;
  }
  printf("%d\n", object->nf);

  double *zbuffer = malloc(width * height * sizeof(double));
  LinearTexture framebuffer = malloc(width * height * sizeof(uint32_t));
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
  float deg_rot = 0;
  Uint32 frame_time = SDL_GetTicks();
  while (running) {
    float dt = (SDL_GetTicks() - frame_time) / 1000.0;
    frame_time = SDL_GetTicks(); // last time we measured

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
        break;
      }
    }

    deg_rot += dt * ROTATE;
    Matrix rot = Matrix_roty(deg_rot);
    Matrix inverse = Matrix_inverse(rot);
    Matrix transform = Matrix_matmul(projection_x_viewport, rot);

    clearDisplay(bg);
    memset(zbuffer, '\x00', width * height * sizeof(double));
    memset(framebuffer, '\x00', width * height * sizeof(uint32_t));
    for (int fi = 0; fi < object->nf; fi++) {
      Face *face = object->faces[fi];

      Texture_draw_face(framebuffer, width, height, face, obj_texture,
                        normal_map, specular_map, zbuffer, light_dir, transform,
                        rot, inverse);
    }

    Matrix_dealloc(rot);
    Matrix_dealloc(inverse);
    Matrix_dealloc(transform);

    updateDisplay(framebuffer);

    // --- FPS counter update ---
    Uint32 fps_current_time = SDL_GetTicks();
    float fps = 1.0 / ((fps_current_time - frame_time) / 1000.0);
    printf("FPS:%f\r", fps);
    fflush(stdout);
    nframes++;
  }
  return 0;
}
