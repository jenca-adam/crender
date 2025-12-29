#include "display.h"
extern Display *display = NULL;
void _display_error(char *errmsg) {
  fprintf(stderr, "%s: %s\n", errmsg, SDL_GetError());
}

int initDisplay(int width, int height, int render_width, int render_height,
                const char *title) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    _display_error("initDisplay: SDL_Init failure");
    return 0;
  }
  SDL_Window *window =
      SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window) {
    _display_error("initDisplay: SDL_CreateWindow failure");
    SDL_Quit();
    return 0;
  }
  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    _display_error("initDisplay: SDL_CreateRenderer failure");
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }
  display = (Display *)malloc(sizeof(Display));
  display->width = width;
  display->height = height;
  display->render_width = render_width;
  display->render_height = render_height;
  display->block_width = width / render_width;
  display->block_height = height / render_height;
  display->window = window;
  display->renderer = renderer;
  /*display->fbuf = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
      display->render_width, display->render_height);*/
  return 1;
}

void cleanupDisplay() {
  if (!display) {
    return;
  }

  if (display->renderer)
    SDL_DestroyRenderer(display->renderer);
  if (display->window)
    SDL_DestroyWindow(display->window);
  SDL_DestroyTexture(display->fbuf);
  SDL_Quit();
  free(display);
  display = NULL;
}
void clearDisplay(Vec3 color) {

  SDL_SetRenderDrawColor(display->renderer, color.x, color.y, color.z, 255);
  SDL_RenderClear(display->renderer);
}

void updateDisplay(Linear_Texture pixels) {
  SDL_Texture *fbuf = SDL_CreateTexture(
      display->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
      display->render_width, display->render_height);
  SDL_UpdateTexture(fbuf, NULL, pixels,
                    display->render_width * sizeof(uint32_t));
  SDL_RenderCopy(display->renderer, fbuf, NULL, NULL);
  SDL_RenderPresent(display->renderer);
}

void setScreenPixel(int i, int j, Vec3 color) {
  // printf("D: %d %d (%f %f %f)\n", i, j, color.x, color.y, color.z);
  SDL_Rect rect = {j * display->block_width, i * display->block_height,
                   display->block_width, display->block_height};
  SDL_SetRenderDrawColor(display->renderer, color.x, color.y, color.z, 255);
  SDL_RenderFillRect(display->renderer, &rect);
}
int renderTexture(Texture texture) {
  if (!display) {
    fprintf(stderr, "renderTexture: display not initialised\n");
    return 0;
  }
  Linear_Texture t = Texture_to_linear(texture);
  updateDisplay(t);
  free(t);
  return 1;
}
