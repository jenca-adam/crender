#include "color.h"
#include <stdlib.h>
Color *Color_create(unsigned char r, unsigned char g, unsigned char b,
                    unsigned char a) {
  Color *color = (Color *)malloc(sizeof(color));
  color->r = r;
  color->g = g;
  color->b = b;
  color->a = a;
  return color;
}
