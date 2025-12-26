#include "texture.h"
#include "common.h"
#include "core.h"
#include "display.h"
#include "obj.h"
#include <errno.h>
#include <math.h>
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

inline Vec3 Texture_getuv(Texture *texture, Vec3 uv) {
  return texture
      ->m[(int)fabs(texture->height - (clamp(uv.y, 0, 1) * texture->height))]
         [(int)fabs(clamp(uv.x, 0, 1) * texture->width)];
}
void Texture_draw_face(LinearTexture texture, int width, int height, Face *face,
                       Texture *diffuse, Texture *normal_map,
                       Texture *specular_map, double *zbuffer, Vec3 light_dir,
                       Matrix transform, Matrix world_transform,
                       Matrix inverse_transform, double near_plane,
                       shading_mode mode) {
  Vec3 l = Vec3_transform(light_dir, inverse_transform);
  Vec3 ldir = Vec3_normalized(l);
  Triangle raw_tri = Face_gettri(face, VERTEX);
  Triangle world_tri = Triangle_transform(raw_tri, world_transform);
  if (world_tri.v0.z > near_plane || world_tri.v1.z > near_plane ||
      world_tri.v2.z > near_plane) {
    return;
  }
  Vec3 n = Vec3_normalized(Vec3_cross(Vec3_sub(raw_tri.v2, raw_tri.v0),
                                      Vec3_sub(raw_tri.v1, raw_tri.v0)));
  double intensity = Vec3_dot(n, ldir);
  if (intensity < 0) {
    return;
  }
  Triangle uvs = Face_gettri(face, UV);
  Triangle vns = Face_gettri(face, NORMAL);
  Triangle tri = Triangle_transform(raw_tri, transform);
  double x0 = tri.v0.x, y0 = tri.v0.y, z0 = tri.v0.z;
  double x1 = tri.v1.x, y1 = tri.v1.y, z1 = tri.v1.z;
  double x2 = tri.v2.x, y2 = tri.v2.y, z2 = tri.v2.z;
  // Triangle_bary_precomp(&tri);
  double bary_denom = 1 / ((x2 - x0) * (y1 - y0) - (x1 - x0) * (y2 - y0));
  int minx = fmax(0, fmin3(x0, x1, x2));
  int maxx = fmin(width, fmax3(x0, x1, x2));
  int miny = fmax(0, fmin3(y0, y1, y2));
  int maxy = fmin(height, fmax3(y0, y1, y2));

  double tw = width;
  double th = height;
  Vec3 bbase = barycentric(tri.v0, tri.v1, tri.v2, minx, miny, bary_denom);
  Vec3 deltax = Vec3_sub(
      barycentric(tri.v0, tri.v1, tri.v2, minx + 1, miny, bary_denom), bbase);
  Vec3 deltay = Vec3_sub(
      barycentric(tri.v0, tri.v1, tri.v2, minx, miny + 1, bary_denom), bbase);
  for (int x = minx; x <= maxx; x++) {
    if (x >= tw) {
      break;
    }
    if (x < 0) {
      Vec3_ADD_INPLACE(bbase, deltax);
      continue;
    }
    Vec3 b = bbase;
    int calc_bary = 0;
    for (int y = miny; y <= maxy; y++) {
      if (y >= th) {
        break;
      }
      if (y < 0) {
        Vec3_ADD_INPLACE(b, deltay);
        continue;
      }
      if (b.x < -EPSILON || b.y < -EPSILON || b.z < -EPSILON) {
        Vec3_ADD_INPLACE(b, deltay);
        continue;
      }
      double z = z0 * b.x + z1 * b.y + z2 * b.z;
      int zbuffix = x + y * tw;
      if (zbuffer[zbuffix] < z) {
        double spec, specpow;
        Vec3 normal;
        zbuffer[zbuffix] = z;
        Vec3 uv = trinterpolate(uvs, b);
        int u = clamp((int)(uv.x * diffuse->width), 0, diffuse->width - 1);
        int v =
            clamp((int)((1 - uv.y) * diffuse->height), 0, diffuse->height - 1);
        Vec3 color = diffuse->m[v][u];
        if (!normal_map) {
          normal = Vec3_neg(trinterpolate(vns, b));
        } else {
          normal = (Vec3_normal_from_color(normal_map->m[v][u]));
        }

        double d = Vec3_dot(normal, ldir);
        if (mode == PHONG) {
          if (!specular_map) {
            specpow = 1;
          } else {
            specpow = specular_map->m[v][u].x;
          }

          /*Vec3 normal_times_d = Vec3_mul(normal, 2.0 * d);

          Vec3 reflected = (Vec3_sub(normal_times_d, light_dir));
          Vec3 r_normalized = Vec3_normalized(reflected);
          */
          spec = apow(fmax(d, 0.0), specpow);
          intensity = d + spec * .6;
          if (texture) {
            texture[(int)(th * (th - y - 1) + x)] =
                Vec3_phong(color, intensity, 0, 255);
          }
        } else {
          texture[(int)(th * (th - y - 1) + x)] =
              Vec3_pack_color(Vec3_mul(color, fmax(d, 0.0)));
        }
      }
      Vec3_ADD_INPLACE(b, deltay);
    }
    Vec3_ADD_INPLACE(bbase, deltax);
  }
}

void Texture_dealloc(Texture *texture) {
  for (int i = 0; i < texture->height; i++) {
    free(texture->m[i]);
  }
  free(texture->m);
  free(texture);
}
LinearTexture Texture_to_linear(Texture *texture) {
  LinearTexture t = malloc(texture->width * texture->height * sizeof(*t));
  for (int i = 0; i < texture->height; i++) {
    for (int j = 0; j < texture->width; j++) {
      t[(int)(texture->width * (texture->height - i - 1) + j)] =
          Vec3_pack_color(texture->m[i][j]);
    }
  }
  return t;
}
