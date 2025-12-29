#include "crender.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
cr_Texture cr_Texture_alloc(int width, int height) {
  cr_Texture texture;
  texture.width = width;
  texture.height = height;
  texture.m = malloc(sizeof(cr_Vec3) * width * height);
  if (!texture.m) {
    perror("cr_Texture_alloc: malloc() failed");
  }
  return texture;
}
cr_Texture cr_Texture_create(int width, int height, cr_Vec3 color) {
  cr_Texture texture = cr_Texture_alloc(width, height);
  if (!texture.m)
    return texture;
  for (int i = 0; i < width * height; i++) {
    texture.m[i] = color;
  }
  return texture;
}

void cr_Texture_writePPM(cr_Texture texture, char *fn) {
  if (!texture.m) {
    return;
  }
  FILE *fp = fopen(fn, "wb");
  if (!fp) {
    perror("cr_Texture_writePPM: fopen() failed");
    return;
  }
  fprintf(fp, "P6\n%d %d\n255\n", texture.width, texture.height);
  for (int i = 0; i < texture.width * texture.height; i++) {
    cr_Vec3 color = texture.m[i];
    fputc(color.x, fp);
    fputc(color.y, fp);
    fputc(color.z, fp);
  }
  fclose(fp);
}
cr_Texture cr_Texture_readPPM(char *fn_raw, char *dirname) {
  char *fn;
  if (dirname) {
    fn = malloc(strlen(fn_raw) + strlen(dirname) + 2);
    snprintf(fn, strlen(fn_raw) + strlen(dirname) + 2, "%s/%s", dirname,
             fn_raw);
  } else {
    fn = fn_raw;
  }

  FILE *fp = fopen(fn, "rb");
  char mode[3];
  int width;
  int height;
  unsigned long lmaxn; // convert everything to 8-bit RGB anyway
  if (!fp) {
    fprintf(stderr, "cr_Texture_readPPM: fopen(%s) failed: %s\n", fn,
            strerror(errno));
    if (dirname)
      free(fn);
    return (cr_Texture){0};
  }
  if (dirname)
    free(fn);
  if (!fgets(mode, sizeof(mode), fp)) {
    fprintf(stderr, "cr_Texture_readPPM: format error: can't read mode\n");
    return (cr_Texture){0};
  }
  if (strcmp(mode, "P6")) {
    fprintf(stderr,
            "cr_Texture_readPPM: format error: mode %s is not supported\n",
            mode);
    return (cr_Texture){0};
  }
  if (fscanf(fp, "\n%d %d\n%lu\n", &width, &height, &lmaxn) != 3) {
    fprintf(stderr, "cr_Texture_readPPM: format error: can't read header\n");
    return (cr_Texture){0};
  }
  if (lmaxn != 255) {
    fprintf(stderr,
            "cr_Texture_readPPM: format error: can only read 24-bit colors\n");
    return (cr_Texture){0};
  }
  cr_Texture texture = cr_Texture_alloc(width, height);
  if (!texture.m) {
    perror("cr_Texture_readPPM: can't allocate");
    return (cr_Texture){0};
  }
  for (int i = 0; i < width * height; i++) {

    unsigned char rgb[3];
    fread(&rgb, 1, 3, fp);

    if (feof(fp)) {
      fprintf(stderr, "cr_Texture_readPPM: format error: EOF\n");
      cr_Texture_dealloc(texture);
      return (cr_Texture){0};
    }
    texture.m[i] = cr_Vec3_create(rgb[0], rgb[1], rgb[2]);
  }

  return texture;
}
cr_Texture cr_Texture_readPAM(char *fn) {
  FILE *fp = fopen(fn, "rb");
  char mode[3];
  int width;
  int height;
  short depth;
  unsigned long lmaxn;
  if (!fp) {

    perror("cr_Texture_readPAM: fopen() failed");
    abort();
  }
  if (!fgets(mode, sizeof(mode), fp)) {
    perror("cr_Texture_readPAM: format error");
    abort();
  }
  if (strcmp(mode, "P7")) {
    errno = ENOTSUP;
    perror("cr_Texture_readPAM: format error");
    abort();
  }

  char *tupltype = malloc(32 * sizeof(char));
  if (fscanf(
          fp,
          "\nWIDTH %d\nHEIGHT %d\nDEPTH %hd\nMAXVAL %lu\nTUPLTYPE %s\nENDHDR\n",
          &width, &height, &depth, &lmaxn, tupltype) != 5) {
    free(tupltype);
    errno = EINVAL;
    perror("cr_Texture_readPAM: format error");
    abort();
  }
  if (strcmp(tupltype, "RGB_ALPHA")) {
    free(tupltype);
    errno = ENOTSUP;
    perror("cr_Texture_readPAM: can only read RGB_ALPHA pams");
    abort();
  }
  free(tupltype);
  if (depth != 4) {
    perror("cr_Texture_readPAM: expected depth 4");
    abort();
  }
  if (lmaxn != 255) {
    errno = ENOTSUP;
    perror("cr_Texture_readPAM: expected maxval 255");
    abort();
  }
  cr_Texture texture =
      cr_Texture_create(width, height, cr_Vec3_create(0, 0, 0));
  if (!texture.m) {
    abort();
  }
  for (int i = 0; i < height; i++) {
    unsigned char rgb[4];
    fread(&rgb, 1, 4, fp);

    if (feof(fp)) {
      cr_Texture_dealloc(texture);
      perror("cr_Texture_readPAM: format error");
      abort();
    }
    texture.m[i] = cr_Vec3_create(rgb[0], rgb[1], rgb[2]);
  }
  return texture;
}
cr_Texture cr_Texture_read(char *fn) {
  if (strcmp(fn + strlen(fn) - 4, ".ppm") == 0) {
    return cr_Texture_readPPM(fn, NULL);
  } else if (strcmp(fn + strlen(fn) - 4, ".pam") == 0) {
    return cr_Texture_readPAM(fn);
  } else {
    errno = ENOTSUP;
    perror("cr_Texture_read: unknown file format");
    abort();
  }
}

static inline cr_Vec3 cr_Texture_getuv(const cr_Texture t, cr_Vec3 uv) {
  int x = (int)(fabs(cr_clamp(uv.x, 0.0, 1.0)) * (t.width - 1));
  int y = (int)(fabs(cr_clamp((1 - uv.y), 0.0, 1.0)) * (t.height - 1));
  return t.m[y * t.width + x];
}
static inline cr_Vec3 _interp_correct(cr_Vec3 v0, cr_Vec3 v1, cr_Vec3 v2,
                                      cr_Vec3 b, cr_num w0, cr_num w1,
                                      cr_num w2) {
  cr_num bxw = b.x * w0, byw = b.y * w1, bzw = b.z * w2;
  cr_num iw = w0 * b.x + w1 * b.y + w2 * b.z;
  return (cr_Vec3){
      (v0.x * bxw + v1.x * byw + v2.x * bzw) / iw,
      (v0.y * bxw + v1.y * byw + v2.y * bzw) / iw,
      (v0.z * bxw + v1.z * byw + v2.z * bzw) / iw,
  };
}

cr_num apow(cr_num x, uint8_t n) {
  cr_num p[8];
  p[0] = x;
  for (int i = 1; i < 8; i++)
    p[i] = p[i - 1] * p[i - 1];

  cr_num r = 1.0;
  for (int i = 0; i < 8; i++)
    if (n & (1 << i))
      r *= p[i];

  return r;
}
bool cr_Texture_draw_face(cr_Linear_Texture texture, int width, int height,
                          cr_Face *face, cr_Texture diffuse,
                          cr_Texture normal_map, cr_Texture specular_map,
                          cr_num *zbuffer, omp_lock_t *zbuffer_locks,
                          cr_Vec3 light_dir, cr_Matrix transform,
                          cr_Matrix world_transform,
                          cr_Matrix inverse_transform, cr_num near_plane,
                          cr_shading_mode mode) {
  cr_Vec3 l = cr_Vec3_transform_dir(light_dir, inverse_transform);
  cr_Vec3 ldir = cr_Vec3_normalized(l);
  cr_Triangle raw_tri = cr_Face_gettri(face, VERTEX);
  cr_Triangle world_tri = cr_Triangle_transform(raw_tri, world_transform);
  if (world_tri.v0.z > near_plane || world_tri.v1.z > near_plane ||
      world_tri.v2.z > near_plane) {
    return false;
  }
  cr_Vec3 n =
      cr_Vec3_normalized(cr_Vec3_cross(cr_Vec3_sub(raw_tri.v2, raw_tri.v0),
                                       cr_Vec3_sub(raw_tri.v1, raw_tri.v0)));
  cr_num intensity = cr_Vec3_dot(n, ldir);
#ifndef NO_BFCULL
  if (intensity < -cr_EPSILON) {
    return false;
  }
#endif
  cr_Triangle uvs = cr_Face_gettri(face, UV);
  cr_Triangle vns = cr_Face_gettri(face, NORMAL);
  cr_Vec3 ws;
  cr_Triangle tri = cr_Triangle_transform4(raw_tri, transform, &ws);
  cr_num x0 = tri.v0.x, y0 = tri.v0.y;
  cr_num x1 = tri.v1.x, y1 = tri.v1.y;
  cr_num x2 = tri.v2.x, y2 = tri.v2.y;
  cr_num bary_denom = 1 / ((x2 - x0) * (y1 - y0) - (x1 - x0) * (y2 - y0));
  int minx = fmax(0, cr_fmin3(x0, x1, x2));
  int maxx = fmin(width, cr_fmax3(x0, x1, x2));
  int miny = fmax(0, cr_fmin3(y0, y1, y2));
  int maxy = fmin(height, cr_fmax3(y0, y1, y2));
  cr_num iw0 = 1.0f / ws.x;
  cr_num iw1 = 1.0f / ws.y;
  cr_num iw2 = 1.0f / ws.z;
  cr_num tw = width;
  cr_num th = height;
  if (maxx < 0 || maxy < 0 || minx > tw || miny > th)
    return false;
  cr_Vec3 bbase =
      cr_barycentric(tri.v0, tri.v1, tri.v2, minx, miny, bary_denom);
  cr_Vec3 deltax = cr_Vec3_sub(
      cr_barycentric(tri.v0, tri.v1, tri.v2, minx + 1, miny, bary_denom),
      bbase);
  cr_Vec3 deltay = cr_Vec3_sub(
      cr_barycentric(tri.v0, tri.v1, tri.v2, minx, miny + 1, bary_denom),
      bbase);
  for (int y = miny; y <= maxy; y++) {
    if (y >= th) {
      break;
    }
    if (y < 0) {
      cr_Vec3_ADD_INPLACE(bbase, deltay);
      continue;
    }
    cr_Vec3 b = bbase;
    int calc_bary = 0;
    for (int x = minx; x <= maxx; x++) {
      if (x >= tw) {
        break;
      }
      if (x < 0) {
        cr_Vec3_ADD_INPLACE(b, deltax);
        continue;
      }
      if (b.x < -cr_EPSILON || b.y < -cr_EPSILON || b.z < -cr_EPSILON) {
        cr_Vec3_ADD_INPLACE(b, deltax);
        continue;
      }
      cr_num iz = iw0 * b.x + iw1 * b.y + iw2 * b.z;
      cr_num z = iz;
      // cr_num z = z0 * b.x + z1 * b.y + z2 * b.z;
      int zbuffix = x + y * tw;
#ifndef NO_MULTITHREAD
      omp_lock_t *lock = &zbuffer_locks[zbuffix];
      omp_set_lock(lock);
#endif
      if (zbuffer[zbuffix] < z) {
        cr_num spec, specpow;
        cr_Vec3 normal;
        zbuffer[zbuffix] = z;
        // cr_Vec3 uv = trinterpolate(uvs, b);
        cr_Vec3 uv = _interp_correct(uvs.v0, uvs.v1, uvs.v2, b, iw0, iw1, iw2);
        cr_Vec3 color = cr_Texture_getuv(diffuse, uv);
        if (!normal_map.m) {
          normal = cr_Vec3_neg(
              _interp_correct(vns.v0, vns.v1, vns.v2, b, iw0, iw1, iw2));
        } else {
          normal =
              (cr_Vec3_normal_from_color(cr_Texture_getuv(normal_map, uv)));
        }
        cr_num d = cr_Vec3_dot(normal, ldir);
        if (mode == PHONG) { // if there's no specular map,
                             // phong shading is useless(?)
          if (!specular_map.m) {
            specpow = 2;
          } else {
            specpow = cr_Texture_getuv(specular_map, uv).x;
          }

          /*cr_Vec3 normal_times_d = cr_Vec3_mul(normal, 2.0 * d);

          cr_Vec3 reflected = (cr_Vec3_sub(normal_times_d, light_dir));
          cr_Vec3 r_normalized = cr_Vec3_normalized(reflected);
          */
          spec = apow(fmax(d, 0.0), specpow);
          intensity = d + spec * .6;
          if (texture) {
            texture[(int)(tw * (th - y - 1) + x)] =
                cr_Vec3_phong(color, intensity, 0, 255);
          }
        } else {
          texture[(int)(tw * (th - y - 1) + x)] =
              cr_Vec3_pack_color(cr_Vec3_mul(color, fmax(d, 0.0)));
        }
      }
#ifndef NO_MULTITHREAD
      omp_unset_lock(lock);
#endif
      cr_Vec3_ADD_INPLACE(b, deltax);
    }
    cr_Vec3_ADD_INPLACE(bbase, deltay);
  }
  return true;
}

void cr_Texture_dealloc(cr_Texture texture) {
  if (!texture.m)
    return;
  free(texture.m);
}
cr_Linear_Texture cr_Texture_to_linear(cr_Texture texture) {
  cr_Linear_Texture t = malloc(texture.width * texture.height * sizeof(*t));
  for (int i = 0; i < texture.width * texture.height; i++) {
    t[(int)i] = cr_Vec3_pack_color(texture.m[i]);
  }
  return t;
}
