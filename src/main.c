#include "color.h"
#include "core.h"
#include "display.h"
#include "obj.h"
#include "texture.h"
#include "tri.h"
#include <limits.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define WIDTH 1200
#define HEIGHT 1200
#define RW 1200
#define RH 1200
#define DEPTH 1024
#define CAM_Z 3
#define ROTATE 1
#define MULTITHREAD 1
#define N_THREADS 14
#define SCHEDULE dynamic
#define WET_RUN 1
#define TX 0
#define TY 0
#define TZ -2
#define NEAR_PLANE (CAM_Z)
#define BF_CULL

int main(int argc, char *argv[]) {
  const int width = WIDTH;
  const int height = HEIGHT;
  const num cam_z = CAM_Z;
  if (argc < 2) {
    fprintf(stderr, "missing object directory (e.g. obj/head)\n");
    return 1;
  }
  char *dirname = argv[argc - 1];
  Vec3 light_dir = Vec3_create(0, 0, -1);
  Vec3 bg = Vec3_create(0, 0, 0);
  Vec3 fg = Vec3_create(255, 255, 0);
  Matrix projection = Matrix_projection(cam_z);
  Matrix viewport = Matrix_viewport(width / 8., height / 8., width * 3 / 4.,
                                    height * 3 / 4., DEPTH);
  Matrix projection_x_viewport = Matrix_matmul(viewport, projection);
  Matrix_dealloc(projection);
  Matrix_dealloc(viewport);
  Texture texture = Texture_create(WIDTH, HEIGHT, bg);
  Texture obj_texture = Texture_readPPM("obj_texture.ppm", dirname);
  Texture normal_map = Texture_readPPM("normal.ppm", dirname);
  Texture specular_map = Texture_readPPM("specular.ppm", dirname);
  Object *object = Object_fromOBJ("obj.obj", dirname);
  if (!object || !texture.m || !obj_texture.m) {
    return 1;
  }

  if (!initDisplay(RW, RH, WIDTH, HEIGHT, "renderer")) {
    return 1;
  };
  printf("%d\n", object->nf);

  num *zbuffer = malloc(width * height * sizeof(num));
  omp_lock_t *zbuffer_locks = malloc(width * height * sizeof(omp_lock_t));
  LinearTexture framebuffer = malloc(width * height * sizeof(uint32_t));
  for (int i = 0; i < width * height; i++) {
    zbuffer[i] = -DBL_MAX;
    omp_init_lock(&zbuffer_locks[i]);
  }

  SDL_Event event;
  Matrix rot, transform, inverse;
  int running = 1;
  int nframes = 0;
  float deg_rot = 0;
  float rotate = 0;
  int use_normal_map = 1;
  shading_mode mode = PHONG;
  Vec3 translate_vec = {0, 0, 0};
  Vec3 tdelta = {0, 0, 0};
  Uint32 frame_time = SDL_GetTicks();
  while (running) {
    float dt = (SDL_GetTicks() - frame_time) / 1000.0;
    frame_time = SDL_GetTicks(); // last time we measured

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
        break;
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_LEFT:
          rotate = -ROTATE;
          break;
        case SDLK_RIGHT:
          rotate = ROTATE;
          break;
        case SDLK_UP:
          tdelta.z = TZ * dt;
          break;
        case SDLK_DOWN:
          tdelta.z = -TZ * dt;
          break;
        case 'm':
          mode = mode == PHONG ? GOURAUD : PHONG;
          break;
        case 'n':
          use_normal_map = !use_normal_map;
          break;
        default:
          break;
        }
      } else if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
        case SDLK_LEFT:
        case SDLK_RIGHT:
          rotate = 0;
          break;
        case SDLK_UP:
        case SDLK_DOWN:
          tdelta.z = 0;
          break;
        default:
          break;
        }
      }
    }

    deg_rot += dt * rotate;
    Vec3_ADD_INPLACE(translate_vec, tdelta);
    Matrix trans = Matrix_translation(translate_vec);
    Matrix rot = Matrix_roty(deg_rot);
    Matrix inverse = Matrix_inverse(rot);
    Matrix tot = Matrix_matmul(trans, rot);
    Matrix transform = Matrix_matmul(projection_x_viewport, tot);

    clearDisplay(bg);
    for (int i = 0; i < width * height; i++) {
      zbuffer[i] = -DBL_MAX;
    }
    memset(framebuffer, '\x00', width * height * sizeof(uint32_t));

    int drawn = 0;
#if MULTITHREAD
#pragma omp parallel for num_threads(N_THREADS) schedule(SCHEDULE)
#endif
    for (int fi = 0; fi < object->nf; fi++) {
      Face *face = object->faces[fi];

      drawn += Texture_draw_face(framebuffer, width, height, face, obj_texture,
                                 use_normal_map ? normal_map : (Texture){0},
                                 specular_map, zbuffer, zbuffer_locks,
                                 light_dir, transform,

                                 tot, inverse, NEAR_PLANE, mode);
    }

    Matrix_dealloc(rot);
    Matrix_dealloc(tot);
    Matrix_dealloc(inverse);
    Matrix_dealloc(transform);
    Matrix_dealloc(trans);
#if WET_RUN
    updateDisplay(framebuffer);
#endif
    // --- FPS counter update ---
    Uint32 fps_current_time = SDL_GetTicks();
    float fps = 1.0 / ((fps_current_time - frame_time) / 1000.0);
    printf("FPS:%f; model_z: %lf; faces: %d                   \r", fps,
           translate_vec.z, drawn);
    fflush(stdout);
    nframes++;
  }
  cleanupDisplay();
  Texture_dealloc(texture);
  Texture_dealloc(obj_texture);
  Texture_dealloc(normal_map);
  Texture_dealloc(specular_map);
  Object_dealloc(object);
  Matrix_dealloc(projection_x_viewport);
  free(zbuffer);
  free(framebuffer);
  free(zbuffer_locks);
  return 0;
}
