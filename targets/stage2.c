#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "../config/build_config.h"
#include "../nob.h"
#define BUILD_DIR "./build"
#define UTILS_DIR "./utils"
#define SRC_DIR "./src"

#define BUILD_DIR_LIB BUILD_DIR "/lib"
#define BUILD_DIR_INCLUDE BUILD_DIR "/include"
#define BUILD_DIR_BIN BUILD_DIR "/bin"
#define CC "cc"
#define AR "ar"
#ifdef __WIN32__
#define CP "copy"
#else
#define CP "cp"
#endif
#ifdef TARGET_LINUX
#include "target_linux.h"
#elif defined(TARGET_WIN_MINGW32) // elifdef is a C23 extension
#include "target_mingw.h"
#elif defined(TARGET_WIN_MSVC)
#error "MSVC target not yet supported"
#define QUIT
#else
#error "Invalid target."
#define QUIT
#endif
#ifndef PARAM_SINGLETHREAD
#define LDLIBS "-lm", "-fopenmp"
#else
#define LDLIBS "-lm"
#endif
#define THIRDPARTY "./thirdparty"
#ifdef PARAM_STATIC_LIB
#define PREFERRED_CRENDER "-l:libcrender" STATLIB_SUFFIX
#else
#define PREFERRED_CRENDER "-lcrender"
#endif
#ifdef TARGET_WIN_MINGW32
#define GETOPT (THIRDPARTY "/wingetopt/getopt.c")
#endif
#ifdef PARAM_ASAN
#define ASAN "-fsanitize=address"
#endif
#define SDL THIRDPARTY "/SDL2"
#ifdef TARGET_LINUX
#define SDL_LIB SDL "/linux"
#else
#define SDL_LIB SDL "/win"
#endif
#ifndef QUIT
struct util {
  const char *source;
  const char *name;
};

bool dump_crender_cfg_h(char *path) {
  FILE *fp = fopen(path, "wb");
  if (!fp) {
    nob_log(NOB_ERROR, "can't dump crender_cfg.h: %s", strerror(errno));
    return false;
  }
  fprintf(fp, "#ifndef _CRENDER_CFG_H\n#define _CRENDER_CFG_H\n");
  fprintf(fp, "#define CR_CFG_NO_BFCULL 1\n");
#ifdef FLOAT_F64
  fprintf(fp, "#define CR_CFG_NUM_DOUBLE 1\n");
#endif
#ifdef PARAM_SINGLETHREAD
  fprintf(fp, "#define CR_CFG_NO_MULTITHREAD 1\n");
#endif
#ifdef PARAM_UNSAFE
  fprintf(fp, "#define CR_CFG_NO_LOCK 1\n");
#endif
  fprintf(fp, "#endif");
  fclose(fp);
  return true;
}
int main(void) {
  Cmd cmd = {0};
  Cmd dynlib_cmd = {0};
  Cmd statlib_cmd = {0};
  Procs procs = {0};
  if (!mkdir_if_not_exists(BUILD_DIR) || !mkdir_if_not_exists(BUILD_DIR_BIN) ||
      !mkdir_if_not_exists(BUILD_DIR_LIB) ||
      !mkdir_if_not_exists(BUILD_DIR_INCLUDE)) {
    return 1;
  }

  if (!dump_crender_cfg_h(SRC_DIR "/crender_cfg.h")) {
    return 1;
  }

  // expand macro
#ifdef PARAM_EXPAND_MACRO
  cmd_append(&cmd, CC, "-E", "-P", "-dD", SRC_DIR "/texture.c", "-o",
             SRC_DIR "/texture_expanded.c");
  cmd_run(&cmd);
#define EXPANDED "_expanded"
#else
#define EXPANDED ""
#endif
  const char *sources[] = {"abi_tag", "obj", "scene", ("texture" EXPANDED),
                           "tri",     "vec"};
  const struct util utils[] = {{.source = "crbake.c", .name = "crbake"}};
  cmd_append(&dynlib_cmd, CC, "-shared", "-fPIC", "-o",
             (BUILD_DIR_LIB "/libcrender" DYNLIB_SUFFIX), LDLIBS, );

  cmd_append(&statlib_cmd, AR, "-rv",
             (BUILD_DIR_LIB "/libcrender" STATLIB_SUFFIX));
#ifdef PARAM_DEBUG
  cmd_append(&dynlib_cmd, DEBUG_FLAG);
#endif
  for (size_t i = 0; i < ARRAY_LEN(sources); i++) {
    const char *source = sources[i];
    char *src_file = temp_sprintf(SRC_DIR"/%s.c", source);
    char *obj_file = temp_sprintf(SRC_DIR"/%s.o", source);
    cmd_append(&dynlib_cmd, obj_file);
    cmd_append(&statlib_cmd, obj_file);
#ifdef CLANG_FORMAT
    cmd_append(&cmd, CLANG_FORMAT, "-i", src_file);
    cmd_run(&cmd);
#endif
    cmd_append(&cmd, CC, "-Wall", "-Wextra", "-Wpedantic", "-Wshadow",
               "-Wstrict-aliasing", "-march=native", "-ffast-math",
               "-funroll-loops", "-I", INCLUDES, "-I", THIRDPARTY, "-fPIC",
               "-c", src_file, "-o", obj_file, LDLIBS);
#ifdef ASAN
    cmd_append(&cmd, ASAN);
#endif
#ifdef PARAM_DEBUG
    cmd_append(&cmd, DEBUG_FLAG);
#endif
#ifdef PARAM_O0
    cmd_append(&cmd, "-O0");
#else
    cmd_append(&cmd, "-O3");
#endif
    cmd_run(&cmd, .async = &procs);
  }
#ifdef CLANG_FORMAT
  cmd_append(&cmd, CLANG_FORMAT, "-i", SRC_DIR "/crender.h");
  cmd_run(&cmd, .async = &procs);
  if (!procs_flush(&procs)) {
    return 1;
  }
#endif
  cmd_append(&cmd, CP, SRC_DIR "/crender.h", BUILD_DIR_INCLUDE "/crender.h");
  cmd_run(&cmd, .async = &procs);
  cmd_append(&cmd, CP, SRC_DIR "/crender_cfg.h",
             BUILD_DIR_INCLUDE "/crender_cfg.h");
  cmd_run(&cmd, .async = &procs);
  if (!procs_flush(&procs)) {
    return 1;
  }

  cmd_run(&dynlib_cmd, .async = &procs);
#ifdef PARAM_STATIC_LIB
  cmd_run(&statlib_cmd, .async = &procs);
#endif
  if (!procs_flush(&procs)) {
    return 1;
  }

  for (size_t i = 0; i < ARRAY_LEN(utils); i++) {
    const struct util u = utils[i];
    char *src_file = malloc(sizeof(UTILS_DIR) + strlen(u.source) + 2);
    strcat(src_file, (UTILS_DIR "/"));
    strcat(src_file, u.source);
    char *out_file = malloc(sizeof(BUILD_DIR_BIN) + strlen(u.name) + 2);
    strcat(out_file, (BUILD_DIR_BIN "/"));
    strcat(out_file, u.name);
#ifdef CLANG_FORMAT
    cmd_append(&cmd, CLANG_FORMAT, "-i", src_file);
    cmd_run(&cmd);
#endif
    cmd_append(&cmd, CC, src_file, "-o", out_file, "-Wall", "-Wextra",
               "-Wpedantic", LDLIBS, (PREFERRED_CRENDER), "-I",
               (BUILD_DIR_INCLUDE), "-I", INCLUDES, "-I", THIRDPARTY, "-L",
               (BUILD_DIR_LIB));
#ifdef GETOPT
    cmd_append(&cmd, GETOPT);
#endif
#ifdef PARAM_DEBUG
    cmd_append(&cmd, DEBUG_FLAG);
#endif
    cmd_run(&cmd, .async = &procs);
  }
  cmd_append(&cmd, CC, "examples/sdl/main.c", "examples/sdl/display.c", "-o",
             "examples/sdl/renderer", "-Wall", "-Wextra", "-Wpedantic",
             "-I", (BUILD_DIR_INCLUDE), "-I", INCLUDES,
             "-I", THIRDPARTY, ("-L" BUILD_DIR_LIB), ("-L" SDL_LIB),
             ("-I" SDL));
  cmd_append(&cmd, (PREFERRED_CRENDER), "-lSDL2", LDLIBS EXAMPLE_EXTRA_FLAGS);
#ifdef PARAM_SDL_EXAMPLE_NO_VSYNC
  cmd_append(&cmd, "-DNO_VSYNC");
#endif
#ifdef PARAM_STATIC_EXAMPLE
  cmd_append(&cmd, "-static", "-static-libgcc");
#endif
  cmd_run(&cmd);
  if (!procs_flush(&procs)) {
    return 1;
  }

  return 0;
}
#endif
