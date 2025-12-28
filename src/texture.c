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
Texture Texture_alloc(int width, int height) {
  Texture texture;
  texture.width = width;
  texture.height = height;
  texture.m = malloc(sizeof(Vec3) * width * height);
  if (!texture.m) {
    perror("Texture_alloc: malloc() failed");
  }
  return texture;
}
Texture Texture_create(int width, int height, Vec3 color) {
  Texture texture = Texture_alloc(width, height);
  if (!texture.m)
    return texture;
  for (int i = 0; i < width * height; i++) {
    texture.m[i] = color;
  }
  return texture;
}

void Texture_writePPM(Texture texture, char *fn) {
  if (!texture.m) {
    return;
  }
  FILE *fp = fopen(fn, "wb");
  if (!fp) {
    perror("Texture_writePPM: fopen() failed");
    return;
  }
  fprintf(fp, "P6\n%d %d\n255\n", texture.width, texture.height);
  for (int i = 0; i < texture.width * texture.height; i++) {
    Vec3 color = texture.m[i];
    fputc(color.x, fp);
    fputc(color.y, fp);
    fputc(color.z, fp);
  }
  fclose(fp);
}
Texture Texture_readPPM(char *fn_raw, char *dirname) {
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
    fprintf(stderr, "Texture_readPPM: fopen(%s) failed: %s\n", fn,
            strerror(errno));
    if (dirname)
      free(fn);
    return (Texture){0};
  }
  if (dirname)
    free(fn);
  if (!fgets(mode, sizeof(mode), fp)) {
    fprintf(stderr, "Texture_readPPM: format error: can't read mode\n");
    return (Texture){0};
  }
  if (strcmp(mode, "P6")) {
    fprintf(stderr, "Texture_readPPM: format error: mode %s is not supported\n",
            mode);
    return (Texture){0};
  }
  if (fscanf(fp, "\n%d %d\n%lu\n", &width, &height, &lmaxn) != 3) {
    fprintf(stderr, "Texture_readPPM: format error: can't read header\n");
    return (Texture){0};
  }
  if (lmaxn != 255) {
    fprintf(stderr,
            "Texture_readPPM: format error: can only read 24-bit colors\n");
    return (Texture){0};
  }
  Texture texture = Texture_alloc(width, height);
  if (!texture.m) {
    perror("Texture_readPPM: can't allocate");
    return (Texture){0};
  }
  for (int i = 0; i < width * height; i++) {

    unsigned char rgb[3];
    fread(&rgb, 1, 3, fp);

    if (feof(fp)) {
      fprintf(stderr, "Texture_readPPM: format error: EOF\n");
      Texture_dealloc(texture);
      return (Texture){0};
    }
    texture.m[i] = Vec3_create(rgb[0], rgb[1], rgb[2]);
  }

  return texture;
}
Texture Texture_readPAM(char *fn) {
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
  Texture texture = Texture_create(width, height, Vec3_create(0, 0, 0));
  if (!texture.m) {
    abort();
  }
  for (int i = 0; i < height; i++) {
    unsigned char rgb[4];
    fread(&rgb, 1, 4, fp);

    if (feof(fp)) {
      Texture_dealloc(texture);
      perror("Texture_readPAM: format error");
      abort();
    }
    texture.m[i] = Vec3_create(rgb[0], rgb[1], rgb[2]);
  }
  return texture;
}
Texture Texture_read(char *fn) {
  if (strcmp(fn + strlen(fn) - 4, ".ppm") == 0) {
    return Texture_readPPM(fn, NULL);
  } else if (strcmp(fn + strlen(fn) - 4, ".pam") == 0) {
    return Texture_readPAM(fn);
  } else {
    errno = ENOTSUP;
    perror("Texture_read: unknown file format");
    abort();
  }
}

static inline Vec3 Texture_getuv(const Texture t, Vec3 uv) {
  int x = (int)(fabs(clamp(uv.x, 0.0, 1.0)) * (t.width - 1));
  int y = (int)(fabs(clamp((1 - uv.y), 0.0, 1.0)) * (t.height - 1));
  return t.m[y * t.width + x];
}
inline Vec3 _interp_correct(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 b, num w0, num w1,
                            num w2) {
  num bxw = b.x * w0, byw = b.y * w1, bzw = b.z * w2;
  num iw = w0 * b.x + w1 * b.y + w2 * b.z;
  return (Vec3){
      (v0.x * bxw + v1.x * byw + v2.x * bzw) / iw,
      (v0.y * bxw + v1.y * byw + v2.y * bzw) / iw,
      (v0.z * bxw + v1.z * byw + v2.z * bzw) / iw,
  };
}

bool Texture_draw_face(LinearTexture texture, int width, int height, Face *face,
                       Texture diffuse, Texture normal_map,
                       Texture specular_map, num *zbuffer,
                       omp_lock_t *zbuffer_locks, Vec3 light_dir,
                       Matrix transform, Matrix world_transform,
                       Matrix inverse_transform, num near_plane,
                       shading_mode mode) {
  Vec3 l = Vec3_transform(light_dir, inverse_transform);
  Vec3 ldir = Vec3_normalized(l);
  Triangle raw_tri = Face_gettri(face, VERTEX);
  Triangle world_tri = Triangle_transform(raw_tri, world_transform);
  if (world_tri.v0.z > near_plane || world_tri.v1.z > near_plane ||
      world_tri.v2.z > near_plane) {
    return false;
  }
  Vec3 n = Vec3_normalized(Vec3_cross(Vec3_sub(raw_tri.v2, raw_tri.v0),
                                      Vec3_sub(raw_tri.v1, raw_tri.v0)));
  num intensity = Vec3_dot(n, ldir);
#ifdef BF_CULL
  if (intensity < -EPSILON) {
    return false;
  }
#endif
  Triangle uvs = Face_gettri(face, UV);
  Triangle vns = Face_gettri(face, NORMAL);
  Vec3 ws;
  Triangle tri = Triangle_transform4(raw_tri, transform, &ws);
  num x0 = tri.v0.x, y0 = tri.v0.y, z0 = tri.v0.z;
  num x1 = tri.v1.x, y1 = tri.v1.y, z1 = tri.v1.z;
  num x2 = tri.v2.x, y2 = tri.v2.y, z2 = tri.v2.z;
  // Triangle_bary_precomp(&tri);
  num bary_denom = 1 / ((x2 - x0) * (y1 - y0) - (x1 - x0) * (y2 - y0));
  int minx = fmax(0, fmin3(x0, x1, x2));
  int maxx = fmin(width, fmax3(x0, x1, x2));
  int miny = fmax(0, fmin3(y0, y1, y2));
  int maxy = fmin(height, fmax3(y0, y1, y2));
  num iw0 = 1.0f / ws.x;
  num iw1 = 1.0f / ws.y;
  num iw2 = 1.0f / ws.z;
  num tw = width;
  num th = height;
  if (maxx < 0 || maxy < 0 || minx > tw || miny > th)
    return false;
  Vec3 bbase = barycentric(tri.v0, tri.v1, tri.v2, minx, miny, bary_denom);
  Vec3 deltax = Vec3_sub(
      barycentric(tri.v0, tri.v1, tri.v2, minx + 1, miny, bary_denom), bbase);
  Vec3 deltay = Vec3_sub(
      barycentric(tri.v0, tri.v1, tri.v2, minx, miny + 1, bary_denom), bbase);
  for (int y = miny; y <= maxy; y++) {
    if (y >= th) {
      break;
    }
    if (y < 0) {
      Vec3_ADD_INPLACE(bbase, deltay);
      continue;
    }
    Vec3 b = bbase;
    int calc_bary = 0;
    for (int x = minx; x <= maxx; x++) {
      if (x >= tw) {
        break;
      }
      if (x < 0) {
        Vec3_ADD_INPLACE(b, deltax);
        continue;
      }
      if (b.x < -EPSILON || b.y < -EPSILON || b.z < -EPSILON) {
        Vec3_ADD_INPLACE(b, deltax);
        continue;
      }
      num iz = iw0 * b.x + iw1 * b.y + iw2 * b.z;
      num z = iz;
      // num z = z0 * b.x + z1 * b.y + z2 * b.z;
      int zbuffix = x + y * tw;
      omp_lock_t *lock = &zbuffer_locks[zbuffix];
      omp_set_lock(lock);
      if (zbuffer[zbuffix] < z) {
        num spec, specpow;
        Vec3 normal;
        zbuffer[zbuffix] = z;
        // Vec3 uv = trinterpolate(uvs, b);
        Vec3 uv = _interp_correct(uvs.v0, uvs.v1, uvs.v2, b, iw0, iw1, iw2);
        Vec3 color = Texture_getuv(diffuse, uv);
        if (!normal_map.m) {
          normal = Vec3_neg(
              _interp_correct(vns.v0, vns.v1, vns.v2, b, iw0, iw1, iw2));
        } else {
          normal = (Vec3_normal_from_color(Texture_getuv(normal_map, uv)));
        }

        num d = Vec3_dot(normal, ldir);
        if (mode == PHONG) { // if there's no specular map,
                             // phong shading is useless(?)
          if (!specular_map.m) {
            specpow = 2;
          } else {
            specpow = Texture_getuv(specular_map, uv).x;
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
      omp_unset_lock(lock);
      Vec3_ADD_INPLACE(b, deltax);
    }
    Vec3_ADD_INPLACE(bbase, deltay);
  }
  return true;
}

void Texture_dealloc(Texture texture) {
  if (!texture.m)
    return;
  free(texture.m);
}
LinearTexture Texture_to_linear(Texture texture) {
  LinearTexture t = malloc(texture.width * texture.height * sizeof(*t));
  for (int i = 0; i < texture.width * texture.height; i++) {
    t[(int)i] = Vec3_pack_color(texture.m[i]);
  }
  return t;
}
