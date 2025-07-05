#ifndef _COLOR_H
#define _COLOR_H
typedef struct Color{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} Color;

Color *Color_create(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
#endif
