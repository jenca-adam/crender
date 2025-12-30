#define cr_STRIP_SYMS
#include "display.h"
#include <crender.h>
#include <stdbool.h>
#include <stdio.h>
#define WIN_WIDTH 800
#define WIN_HEIGHT 800
#define RENDER_SAMPLE 1.0f
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
void usage(void) {
  printf("-----------------------\n\
Commands: \n\
Arrow keys left/right: rotate on the y axis\n\
Arrow keys up/down: move on the z axis\n\
w/s: rotate on the x axis\n\
a/d: rotate on the z axis\n\
h/l: move on the x axis\n\
j/k: move on the y axis\n\
m: switch Phong/Gouraud shading\n\
n: switch normal map/interpolated normals\n\
b: switch floor/closest/bilinear texture sampling\n\
t: swap diffuse texture\n\
r: reset object transform\n\
-----------------------\n\
\n");
}
char *join_dirs(char *dirname, char *path) {
  size_t dlen = strlen(dirname);
  size_t len = dlen + strlen(path) + 2;
  char *buffer = malloc(len);
  if (dirname[dlen - 1] == '/' || path[0] == '/') {
    snprintf(buffer, len, "%s%s", dirname, path);
  } else {
    snprintf(buffer, len, "%s/%s", dirname, path);
  }
  return buffer;
}
bool load_object_from_dir(char *dirname, Object **object, Texture *diffuse,
                          Texture *normal_map, Texture *specular_map) {
  char *object_fname = join_dirs(dirname, "obj.obj");
  Object *ob = Object_fromOBJ(object_fname);
  free(object_fname);
  if (!ob) {
    return false;
  }
  char *diffuse_fname = join_dirs(dirname, "diffuse.ppm");
  *diffuse = Texture_read(diffuse_fname);
  free(diffuse_fname);
  if (!diffuse->m) {
    Object_dealloc(ob);
    return false;
  }
  *object = ob;
  char *normal_fname = join_dirs(dirname, "normal.ppm");
  *normal_map = Texture_read(normal_fname);
  free(normal_fname);
  char *specular_fname = join_dirs(dirname, "specular.ppm");
  *specular_map = Texture_read(specular_fname);
  free(specular_fname);
  return true;
}

int main(int argc, char *argv[]) {
  INIT_CRENDER();
  int width = WIN_WIDTH * RENDER_SAMPLE;
  int height = WIN_HEIGHT * RENDER_SAMPLE;
  const num cam_z = CAM_Z;
  Vec3 bg = {0, 0, 0};
  if (argc < 2) {
    fprintf(stderr, "provide object directory (e.g. obj/head)\n");
    return 1;
  }
  char *dirname = argv[1];
  char *dirname2 = argc >= 3 ? argv[2] : dirname;
  Vec3 light_dir = {0, 0, -1};
  SceneSettings settings = {
      .shading_mode = PHONG,
      .sampling_mode = FLOOR,
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
  Object *main_object;
  Texture main_diffuse, main_normal, main_specular;
  if (!load_object_from_dir(dirname, &main_object, &main_diffuse, &main_normal,
                            &main_specular)) {
    return 1;
  }
  Entity main_entity = Entity_create(main_object);
  main_entity.ts.diffuse = &main_diffuse;
  main_entity.ts.normal_map = &main_normal;
  main_entity.ts.specular_map = &main_specular;
  Scene_add_entity(&scene, &main_entity);
  if (!initDisplay(WIN_WIDTH, WIN_HEIGHT, width, height, "renderer")) {
    return 1;
  }
  Scene_init(&scene);
  usage();
  int running = 1;
  Vec3 delta_rot = {0, 0, 0};
  Vec3 delta_trans = {0, 0, 0};
  Texture other_texture = Texture_create(1, 1, (Vec3){255, 0, 0});
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
          scene.settings.shading_mode = scene.settings.shading_mode == PHONG ? GOURAUD : PHONG;
          break;
        case 'n':
          scene.settings.use_normal_map = !scene.settings.use_normal_map;
          break;
        case 'b':
          scene.settings.sampling_mode = (scene.settings.sampling_mode + 1)%3;
          break;
        case 't':
          using_other = !using_other;
          Entity_attach_texture(&main_entity, TextureSetIndex_diffuse,
                                using_other ? &other_texture : &main_diffuse);
          break;
        case 'r':
          Matrix transform = Matrix_identity(4);
          Entity_set_transform(&main_entity, transform);
          Matrix_dealloc(&transform);
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
  Texture_dealloc(&other_texture);
  Object_dealloc(main_object);
  Texture_dealloc(&main_diffuse);
  Texture_dealloc(&main_normal);
  Texture_dealloc(&main_specular);
  Entity_dealloc(&main_entity);
  Scene_dealloc(&scene);
  cleanupDisplay();
  return 0;
}
