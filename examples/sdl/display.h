#pragma once
#ifndef _DISPLAY_H
#define _DISPLAY_H
#include <SDL2/SDL.h>
#define cr_STRIP_SYMS
#include <crender.h>
typedef struct Display {
  int width;
  int height;
  int render_width;
  int render_height;
  int block_width;
  int block_height;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *fbuf;
} Display;
int initDisplay(int width, int height, int render_width, int render_height,
                const char *title);
void cleanupDisplay();
void setScreenPixel(int i, int j, Vec3 color);
void updateDisplay(Linear_Texture pixels);
void clearDisplay(Vec3 color);
int renderTexture(Texture texture);
extern Display *display;
#endif
