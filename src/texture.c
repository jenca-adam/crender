#include "texture.h"
#include "common.h"
#include "core.h"
#include "display.h"
#include "obj.h"
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
Texture *Texture_alloc(int width, int height) {
  Texture *texture = (Texture *)malloc(sizeof(Texture));
  texture->width = width;
  texture->height = height;
  texture->m = malloc(sizeof(Vec3 *) * width * height);
  if (!texture->m) {
    free(texture);
    perror("Texture_create: malloc() failed");
    return NULL;
  }
  for (int i = 0; i < height; i++) {
    texture->m[i] = malloc(width * sizeof(Vec3));
  }
  return texture;
}
Texture *Texture_create(int width, int height, Vec3 color) {
  Texture *texture = (Texture *)malloc(sizeof(Texture));
  texture->width = width;
  texture->height = height;
  texture->m = malloc(sizeof(Vec3 *) * width * height);
  if (!texture->m) {
    free(texture);
    perror("Texture_create: malloc() failed");
    return NULL;
  }
  for (int i = 0; i < height; i++) {
    texture->m[i] = malloc(width * sizeof(Vec3));
    if (!texture->m[i]) {
      free(texture->m);
      free(texture);
      perror("Texture_create: calloc() failed");
      return NULL;
    }
    for (int j = 0; j < width; j++) {
      texture->m[i][j] = color;
    }
  }
  return texture;
}

void Texture_writePPM(Texture *texture, char *fn) {
  if (!texture) {
    return;
  }
  FILE *fp = fopen(fn, "wb");
  if (!fp) {
    perror("Texture_writePPM: fopen() failed");
    return;
  }
  fprintf(fp, "P6\n%d %d\n255\n", texture->width, texture->height);
  for (int i = 0; i < texture->height; i++) {
    for (int j = 0; j < texture->width; j++) {
      Vec3 color = texture->m[i][j];
      fputc(color.x, fp);
      fputc(color.y, fp);
      fputc(color.z, fp);
    }
  }
  fclose(fp);
}
Texture *Texture_readPPM(char *fn) {
  FILE *fp = fopen(fn, "rb");
  char mode[3];
  int width;
  int height;
  unsigned long lmaxn; // convert everything to 8-bit RGB anyway
  if (!fp) {
    perror("Texture_readPPM: fopen() failed");
    abort();
  }
  if (!fgets(mode, sizeof(mode), fp)) {

    perror("Texture_readPPM: format error");
    abort();
  }
  if (strcmp(mode, "P6")) {
    errno = ENOTSUP;
    perror("Texture_readPPM: format error");
    abort();
  }
  if (fscanf(fp, "\n%d %d\n%lu\n", &width, &height, &lmaxn) != 3) {
    errno = EINVAL;
    perror("Texture_readPPM: format error");
    abort();
  }
  if (lmaxn != 255) {
    errno = ENOTSUP;
    perror("Texture_readPPM: format error");
    abort();
  }
  Texture *texture = Texture_alloc(width, height);
  if (!texture) {
    abort();
  }
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      unsigned char rgb[3];
      fread(&rgb, 1, 3, fp);

      if (feof(fp) && (i < height - 1) && (j < width - 1)) {
        perror("Texture_readPPM: format error");
        abort();
      }
      texture->m[i][j] = Vec3_create(rgb[0], rgb[1], rgb[2]);
    }
  }
  return texture;
}
Texture *Texture_readPAM(char *fn) {
  FILE *fp = fopen(fn, "rb");
  char mode[3];
  int width;
  int height;
  short depth;
  unsigned long lmaxn;
  if (!fp) {

    perror("Texture_readPAM: fopen() failed");
    abort();
  }
  if (!fgets(mode, sizeof(mode), fp)) {
    perror("Texture_readPAM: format error");
    abort();
  }
  if (strcmp(mode, "P7")) {
    errno = ENOTSUP;
    perror("Texture_readPAM: format error");
    abort();
  }

  char *tupltype = malloc(32 * sizeof(char));
  if (fscanf(
          fp,
          "\nWIDTH %d\nHEIGHT %d\nDEPTH %hd\nMAXVAL %lu\nTUPLTYPE %s\nENDHDR\n",
          &width, &height, &depth, &lmaxn, tupltype) != 5) {
    free(tupltype);
    errno = EINVAL;
    perror("Texture_readPAM: format error");
    abort();
  }
  if (strcmp(tupltype, "RGB_ALPHA")) {
    free(tupltype);
    errno = ENOTSUP;
    perror("Texture_readPAM: can only read RGB_ALPHA pams");
    abort();
  }
  free(tupltype);
  if (depth != 4) {
    perror("Texture_readPAM: expected depth 4");
    abort();
  }
  if (lmaxn != 255) {
    errno = ENOTSUP;
    perror("Texture_readPAM: expected maxval 255");
    abort();
  }
  Texture *texture = Texture_create(width, height, Vec3_create(0, 0, 0));
  if (!texture) {
    abort();
  }
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      unsigned char rgb[4];
      fread(&rgb, 1, 4, fp);

      if (feof(fp) && (i < height - 1) && (j < width - 1)) {
        Texture_dealloc(texture);
        perror("Texture_readPAM: format error");
        abort();
      }
      texture->m[i][j] = Vec3_create(rgb[0], rgb[1], rgb[2]);
    }
  }
  return texture;
}
Texture *Texture_read(char *fn) {
  if (strcmp(fn + strlen(fn) - 4, ".ppm") == 0) {
    return Texture_readPPM(fn);
  } else if (strcmp(fn + strlen(fn) - 4, ".pam") == 0) {
    return Texture_readPAM(fn);
  } else {
    errno = ENOTSUP;
    perror("Texture_read: unknown file format");
    abort();
  }
}

Vec3 Texture_getuv(Texture *texture, Vec3 uv) {
  return texture
      ->m[(int)fabs(texture->height - clamp(uv.y, 0, 1) * texture->height)]
         [(int)fabs(clamp(uv.x, 0, 1) * texture->width)];
}
void Texture_draw_face(Texture *texture, Face *face, Texture *diffuse,
                       Texture *normal_map, Texture *specular_map,
                       double *zbuffer, Vec3 light_dir, Matrix transform,
                       Matrix world_transform, Matrix inverse_transform) {
  Vec3 l = Vec3_transform(light_dir, inverse_transform);
  Vec3 ldir = Vec3_normalized(l);
  Triangle world_tri = Face_gettri(face, VERTEX);
  Vec3 carg1 = Vec3_sub(world_tri.v2, world_tri.v0);
  Vec3 carg2 = Vec3_sub(world_tri.v1, world_tri.v0);
  Vec3 crossed = Vec3_cross(carg1, carg2);
  Vec3 n = Vec3_normalized(crossed);
  double intensity = Vec3_dot(n, ldir);
  if (intensity < 0) {
    return;
  }
  Vec3 c = Vec3_create(255, 255, 255);
  Triangle uvs = Face_gettri(face, UV);
  Triangle vns = Face_gettri(face, NORMAL);
  Triangle tri = Triangle_transform(world_tri, transform);

  int minx = fmin3(tri.v0.x, tri.v1.x, tri.v2.x);
  int maxx = fmax3(tri.v0.x, tri.v1.x, tri.v2.x);
  int miny = fmin3(tri.v0.y, tri.v1.y, tri.v2.y);
  int maxy = fmax3(tri.v0.y, tri.v1.y, tri.v2.y);

  Vec3 b;
  double tw, th;
  if (texture) {
    tw = texture->width;
    th = texture->height;
  } else {
    tw = display->render_width;
    th = display->render_height;
  }

  for (int x = minx; x <= maxx; x++) {
    if (x >= tw) {
      continue;
    }
    if (x < 0) {
      continue;
    }
    for (int y = miny; y <= maxy; y++) {
      if (y >= th) {
        continue;
      }
      if (y < 0) {
        continue;
      }
      b = barycentric(tri.v0, tri.v1, tri.v2, x, y);
      if (b.x < -EPSILON || b.y < -EPSILON || b.z < -EPSILON) {
        continue;
      }
      double z = tri.v0.z * b.x + tri.v1.z * b.y + tri.v2.z * b.z;
      int zbuffix = x + y * tw;
      if (zbuffer[zbuffix] < z) {
        double spec, specpow;
        Vec3 normal;
        zbuffer[zbuffix] = z;
        Vec3 uv = trinterpolate(uvs, b);

        Vec3 color = Texture_getuv(diffuse, uv);
        if (!normal_map) {
          normal = Vec3_neg(trinterpolate(vns, b));
        } else {
          normal = (Vec3_normal_from_color(Texture_getuv(normal_map, uv)));
        }
        if (!specular_map) {
          specpow = 1;
        } else {
          specpow = Texture_getuv(specular_map, uv).x;
        }
        Vec3 normal_transformed = Vec3_transform(normal, inverse_transform);
        Vec3 normal_transformed_normalized =
            Vec3_normalized(normal_transformed);
        double d = Vec3_dot(normal, ldir);

        Vec3 normal_times_d = Vec3_mul(normal, 2.0 * d);

        Vec3 reflected = (Vec3_sub(normal_times_d, light_dir));
        Vec3 r_normalized = Vec3_normalized(reflected);
        spec = pow(fmax(-r_normalized.z, 0.0), specpow);
        intensity = d + spec * .6;
        if (texture) {
          texture->m[(int)th - y - 1][x] = Vec3_phong(color, intensity, 0, 255);
        } else {
          setScreenPixel(th - y - 1, x, Vec3_phong(color, intensity, 0, 255));
        }
      }
    }
  }
}

void Texture_dealloc(Texture *texture) {
  for (int i = 0; i < texture->height; i++) {
    free(texture->m[i]);
  }
  free(texture->m);
  free(texture);
}
