#define cr_STRIP_SYMS
#include "display.h"
#include <crender.h>
#include <stdio.h>
#include <stdbool.h>
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
#define TX 2
#define TY 2
#define TZ 2
#define NEAR_PLANE (CAM_Z)
void usage(void){
    printf("Commands: \n\
Arrow keys left/right: rotate on the y axis\n\
Arrow keys up/down: move on the z axis\n\
w/s: rotate on the x axis\n\
a/d: rotate on the z axis\n\
m: switch Phong/Gouraud shading\n\
n: switch normal map/interpolated normals\n\
t: swap diffuse texture\n");
}

int main(int argc, char *argv[]) {
  int width = WIN_WIDTH * RENDER_SAMPLE;
  int height = WIN_HEIGHT * RENDER_SAMPLE;
  const num cam_z = CAM_Z;
  Vec3 bg = {0, 0, 0};
  if (argc < 2) {
    fprintf(stderr, "missing at least 1 object directory (e.g. obj/head)\n");
    return 1;
  }
  char *dirname = argv[1];
char *dirname2 = argc>=3?argv[2]:dirname;
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
  Entity other_entity = Entity_create();
  if (!Entity_load_dir(&other_entity, dirname2)){
    return 1;
  }

  Scene_add_entity(&scene, &main_entity);
  Scene_add_entity(&scene, &other_entity);
  if (!initDisplay(WIN_WIDTH, WIN_HEIGHT, width, height, "renderer")) {
    return 1;
  }
  Scene_init(&scene);
  usage();
  int running = 1;
  Vec3 delta_rot = {0, 0, 0};
  Vec3 delta_trans = {0, 0, 0};
  Texture other_texture = Texture_create(1,1, (Vec3){255,0,0});
  Texture main_texture = main_entity.ts.diffuse; 
  Entity_translate_world_space(&other_entity, (Vec3){2, 0, -5});
  Entity_rotate(&other_entity, (Vec3){0,-M_PI/2,0});
  bool using_other = false;
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
        case 'h':
          delta_trans.x = -TX;
          break;
        case 'l':
          delta_trans.x = TX;
          break;
        case 'j':
          delta_trans.y = -TY;
          break;
        case 'k':
          delta_trans.y = TY;
          break;
        case 'm':
          scene.settings.mode = scene.settings.mode == PHONG ? GOURAUD : PHONG;
          break;
        case 'n':
          scene.settings.use_normal_map = !scene.settings.use_normal_map;
          break;
        case 't':
          using_other = !using_other;
          Entity_attach_texture(&main_entity, TextureSetIndex_diffuse, using_other?other_texture:main_texture);
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
        case 'h':
        case 'l':
          delta_trans.x = 0;
          break;
        case 'j':
        case 'k':
          delta_trans.y = 0;
          break;
        case SDLK_UP:
        case SDLK_DOWN:
          delta_trans.z = 0;
        default:
          break;
        }
      }
    }
    Entity_rotate_world_space(&main_entity, Vec3_mul(delta_rot, dt));
    Entity_translate_world_space(&main_entity, Vec3_mul(delta_trans, dt));
    Scene_reset_buffers(&scene);
    Scene_render(&scene, N_THREADS);
    clearDisplay(bg);
    updateDisplay(scene.framebuffer);
  }
  if (!Scene_uses_texture(&scene, other_texture)){
    Texture_dealloc(other_texture);
  } // todo: maybe implement an arena ?? that's by far the cleanest solution to all my problems
  Entity_dealloc(main_entity);
  Scene_dealloc(&scene);
  cleanupDisplay();
  return 0;
}
