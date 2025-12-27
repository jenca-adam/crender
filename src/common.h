#ifndef _COMMON_H
#define _COMMON_H
#define EPSILON 1e-2
#define VIEWPORT_DEPTH 255
#define AMBIENT 0
#define clamp(a, lo, hi) (a > hi) ? hi : ((a < lo) ? lo : a)

typedef enum shading_mode {
   PHONG = 0,
   GOURAUD = 1, 
} shading_mode;
typedef float num;
#endif
