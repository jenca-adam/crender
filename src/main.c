#include "common.h"
#include "display.h"
#include "scene.h"
#include <stdio.h>
#define WIN_WIDTH 800
#define WIN_HEIGHT 800
#define RENDER_SAMPLE .5f
#define DEPTH 0
#define CAM_Z 3
#define ROTATEY 1
#define ROTATEX 1
#define ROTATEZ 1
#define N_THREADS 14
#define WET_RUN 1
#define TX 0
#define TY 0
#define TZ 2
#define NEAR_PLANE (CAM_Z)

int main(int argc, char *argv[]) {
  int width = WIN_WIDTH * RENDER_SAMPLE;
  int height = WIN_HEIGHT * RENDER_SAMPLE;
  const num cam_z = CAM_Z;
  Vec3 bg = {0, 0, 0};
  if (argc < 2) {
    fprintf(stderr, "missing object directory (e.g. obj/head)\n");
    return 1;
  }
  char *dirname = argv[argc - 1];
  Vec3 light_dir = {0, 0, -1};
  SceneSettings settings = {
      .mode = PHONG,
      .render_width = width,
      .render_height = height,
      .render_depth = DEPTH,
      .cam_z = cam_z,
      .fov = 1,
      .near_plane = NEAR_PLANE,
      .light_dir = light_dir,
      .use_normal_map = true,
  };
  Scene scene = Scene_create(settings);
  Entity main_entity = Entity_create();
  if (!Entity_load_dir(&main_entity, dirname)) {
    return 1;
  }

  Scene_add_entity(&scene, &main_entity);

  if (!initDisplay(WIN_WIDTH, WIN_HEIGHT, width, height, "renderer")) {
    return 1;
  }
  Scene_init(&scene);
  int running = 1;
  Vec3 delta_rot = {0, 0, 0};
  Vec3 delta_trans = {0, 0, 0};
  uint32_t frame_time = SDL_GetTicks();
  while (running) {
    float dt = (SDL_GetTicks() - frame_time) / 1000.0;
    printf("FPS:%5.2f\r", 1 / dt);
    fflush(stdout);
    frame_time = SDL_GetTicks();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
        break;
      } else if (event.type == SDL_WINDOWEVENT &&
                 event.window.event == SDL_WINDOWEVENT_RESIZED) {
        // Scene_resize(&scene, event.window.data1 * RENDER_SAMPLE,
        //              event.window.data2 * RENDER_SAMPLE);

        display->render_width = scene.settings.render_width =
            event.window.data1 * RENDER_SAMPLE;
        display->render_height = scene.settings.render_height =
            event.window.data2 * RENDER_SAMPLE;
        // Scene_update_settings(&scene);
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_LEFT:
          delta_rot.y = -ROTATEY;
          break;
        case SDLK_RIGHT:
          delta_rot.y = ROTATEY;
          break;
        case SDLK_UP:
          delta_trans.z = -TZ;
          break;
        case SDLK_DOWN:
          delta_trans.z = TZ;
          break;
        case 'w':
          delta_rot.x = -ROTATEX;
          break;
        case 's':
          delta_rot.x = ROTATEX;
          break;
        case 'a':
          delta_rot.z = ROTATEZ;
          break;
        case 'd':
          delta_rot.z = -ROTATEZ;
          break;
        case 'm':
          scene.settings.mode = scene.settings.mode == PHONG ? GOURAUD : PHONG;
          break;
        case 'n':
          scene.settings.use_normal_map = !scene.settings.use_normal_map;
          break;
        default:
          break;
        }
      } else if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
        case SDLK_LEFT:
        case SDLK_RIGHT:
          delta_rot.y = 0;
          break;
        case 'w':
        case 's':
          delta_rot.x = 0;
          break;
        case 'a':
        case 'd':
          delta_rot.z = 0;
          break;
        case SDLK_UP:
        case SDLK_DOWN:
          delta_trans.z = 0;
        default:
          break;
        }
      }
    }
    Entity_rotate(&main_entity, Vec3_mul(delta_rot, dt));
    Entity_translate_world_space(&main_entity, Vec3_mul(delta_trans, dt));
    Scene_reset_buffers(&scene);
    Scene_render(&scene, N_THREADS);
    clearDisplay(bg);
    updateDisplay(scene.framebuffer);
  }
  Scene_dealloc(&scene);
  cleanupDisplay();
  return 0;
}
