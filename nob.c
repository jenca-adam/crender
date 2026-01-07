#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#define X_BOOL_PARAMS_DOC(X)                                                   \
  X(DEBUG, "build with debugging information")                                 \
  X(EXPAND_MACRO,                                                              \
    "expand _cr_Texture_draw_face_XXX macros for debugging purposes")          \
  X(O0, "build without optimizations")                                         \
  X(SINGLETHREAD,                                                              \
    "build without multithreading. removes dependency on OpenMP")              \
  X(UNSAFE, "don't use mutex locks. will result in a performance boost, but "     \
            "will cause artifacts")\
  X(STATIC, "also link statically.")
#define X_BOOL_PARAMS(X)                                                       \
  X(DEBUG)                                                                     \
  X(EXPAND_MACRO)                                                              \
  X(O0)                                                                        \
  X(SINGLETHREAD)                                                              \
  X(UNSAFE)\
  X(STATIC)
#define TARGET_VALUES_VA(X, ...)                                               \
  X(LINUX, __VA_ARGS__)                                                        \
  X(WIN_MINGW32, __VA_ARGS__)                                                  \
  X(WIN_MSVC, __VA_ARGS__)
#define TARGET_VALUES_DOC_VA(X, ...)                                           \
  X(LINUX, "Linux", __VA_ARGS__)                                               \
  X(WIN_MINGW32, "Windows (MinGW)", __VA_ARGS__)                               \
  X(WIN_MSVC, "Windows (Microsoft Visual C)", __VA_ARGS__)
#define TARGET_VALUES(X)                                                       \
  X(LINUX)                                                                     \
  X(WIN_MINGW32)                                                               \
  X(WIN_MSVC)

#define FLOAT_VALUES_VA(X, ...)                                                \
  X(F32, __VA_ARGS__)                                                          \
  X(F64, __VA_ARGS__)
#define FLOAT_VALUES_DOC_VA(X, ...)                                            \
  X(F32, "Use 32-bit floats", __VA_ARGS__)                                     \
  X(F64, "Use 64-bit floats", __VA_ARGS__)

#define FLOAT_VALUES(X)                                                        \
  X(F32)                                                                       \
  X(F64)
#define BOOL_VALUES_VA(X, ...)                                                 \
  X(UNSET, __VA_ARGS__)                                                        \
  X(SET, __VA_ARGS__)
#define BOOL_VALUES(X)                                                         \
  X(UNSET)                                                                     \
  X(SET)

#define CURRENT_PLATFORM LINUX
#if defined(__MINGW32__) || defined(__MINGW64__)
#undef CURRENT_PLATFORM
#define CURRENT_PLATFORM WIN_MINGW32
#elif defined(_MSC_VER)
#undef CURRENT_PLATFORM
#define CURRENT_PLATFORM WIN_MSVC
#endif
#ifdef _WINDOWS_
// michealsoft binbows
static int ascii_tolower(int c) {
  return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
}

static char *strstr_no_case(const char *h, const char *n) {
  if (!*n) return (char *)h;
  for (; *h; h++) {
    const char *h2 = h, *n2 = n;
    while (*h2 && *n2 &&
           ascii_tolower(*h2) == ascii_tolower(*n2)) {
      h2++; n2++;
    }
    if (!*n2) return (char *)h;
  }
  return NULL;
}
#define strcmp_no_case stricmp
#else
#define strstr_no_case strcasestr
#define strcmp_no_case strcasecmp
#endif
enum bool_param { KEEP = -1, UNSET = 0, SET = 1 };
void delete_entire_dir(const char *dirname);
bool delete_one_file(Nob_Walk_Entry entry){
    char *start = entry.data;
    switch(entry.type){
        case FILE_DIRECTORY:
            if(strcmp(start, entry.path)!=0){
                delete_entire_dir(entry.path);
                *entry.action = WALK_SKIP; 
            }
            break;
        default:
            delete_file(entry.path);
            break;
    }
    return true;
}
void delete_entire_dir(const char *dirname){
    walk_dir(dirname, delete_one_file, (void*)dirname);
    delete_file(dirname);
}
typedef struct {
#define COMMA(x) x,
  enum { TARGET_KEEP = -1, TARGET_VALUES(COMMA) } PTARGET;
  enum { FLOAT_KEEP = -1, FLOAT_VALUES(COMMA) } PFLOAT;
#define BOOL_PARAM_DECL(b) enum bool_param PPARAM_##b;
  X_BOOL_PARAMS(BOOL_PARAM_DECL)
} config;
bool load_config(const char *fn, config *cnf) {
  FILE *fp = fopen(fn, "rb");
  *cnf = (config){CURRENT_PLATFORM, F32, 0, 0, 0, 0, 0};
  if (!fp) {
    nob_log(NOB_INFO, "couldn't load config, creating a default one: %s",
            strerror(errno));
    return false;
  }
  while (!feof(fp)) {
    char line[4096];
    char _[4096];
#define LOAD_CONFIG_CASE(val, param)                                           \
  else if (sscanf(line, "#define " #param "_" #val "%s", _)) {                 \
    (cnf)->P##param = val;                                                     \
  }
#define LOAD_CONFIG(param, VALUES_VA) VALUES_VA(LOAD_CONFIG_CASE, param)
#define BOOL_PARAM_LOAD_CONFIG(param)                                          \
  else if (sscanf(line, "#define PARAM_" #param "%s", _)) {                    \
    (cnf)->PPARAM_##param = SET;                                               \
  }

    if (fgets(line, sizeof(line), fp)) {
      if (0) {
      }
      LOAD_CONFIG(TARGET, TARGET_VALUES_VA)
      LOAD_CONFIG(FLOAT, FLOAT_VALUES_VA)
      X_BOOL_PARAMS(BOOL_PARAM_LOAD_CONFIG)
    } else {
      break;
    }
  }
  return true;
}
void merge_configs(config *c1, config *c2) {
#define MERGE_CONFIG(param, keep)                                              \
  if ((c2)->P##param != keep)                                                  \
    (c1)->P##param = (c2)->P##param;
#define BOOL_PARAM_MERGE_CONFIG(param) MERGE_CONFIG(PARAM_##param, KEEP)
  MERGE_CONFIG(TARGET, TARGET_KEEP);
  MERGE_CONFIG(FLOAT, FLOAT_KEEP);
  X_BOOL_PARAMS(BOOL_PARAM_MERGE_CONFIG);
}
void print_config(config cnf) {
#define PRINT_CASE(val)                                                        \
  case val:                                                                    \
    s = #val;                                                                  \
    break;
#define NONBOOL_PARAM_PRINT(param, VALUES)                                     \
  do {                                                                         \
    char *s;                                                                   \
    switch (cnf.P##param) {                                                    \
      VALUES(PRINT_CASE);                                                      \
    default:                                                                   \
      s = "KEEP";                                                              \
      break;                                                                   \
    }                                                                          \
    nob_log(NOB_INFO, #param "=%s", s);                                        \
  } while (0);
#define BOOL_PARAM_PRINT(b) NONBOOL_PARAM_PRINT(PARAM_##b, BOOL_VALUES)
  nob_log(NOB_INFO, "======= Config =======");
  NONBOOL_PARAM_PRINT(TARGET, TARGET_VALUES);
  NONBOOL_PARAM_PRINT(FLOAT, FLOAT_VALUES);
  X_BOOL_PARAMS(BOOL_PARAM_PRINT);
  nob_log(NOB_INFO, "======================");
}

int clean(void){
   delete_entire_dir("build");
   delete_entire_dir("config");
   delete_file("src/crender_cfg.h");
   delete_file("nob");
   delete_file("nob.old");
   return 0;
}
bool dump_config(char *fn, config *cnf) {
  FILE *fp = fopen(fn, "wb");
  if (!fp) {
    nob_log(NOB_ERROR, "Couldn't dump config file: %s", strerror(errno));
    return false;
  }
#define DUMP_CONFIG_CASE(val, doc, param)                                      \
  if ((cnf)->P##param != val) {                                                \
    fprintf(fp, "// ");                                                        \
  }                                                                            \
  fprintf(fp, "#define " #param "_" #val " //" doc "\n");
#define NONBOOL_PARAM_DUMP_CONFIG(param, VALUES_DOC_VA)                        \
  VALUES_DOC_VA(DUMP_CONFIG_CASE, param)
#define BOOL_PARAM_DUMP_CONFIG(param, doc)                                     \
  if ((cnf)->PPARAM_##param != SET) {                                          \
    fprintf(fp, "// ");                                                        \
  }                                                                            \
  fprintf(fp, "#define PARAM_" #param " //" doc "\n");
  fprintf(fp, "/* ----- available targets ----- */\n");
  NONBOOL_PARAM_DUMP_CONFIG(TARGET, TARGET_VALUES_DOC_VA);
  fprintf(fp, "/* -----   float sizes     ----- */\n");
  NONBOOL_PARAM_DUMP_CONFIG(FLOAT, FLOAT_VALUES_DOC_VA);
  fprintf(fp, "/* ----- build parameters  ----- */\n");
  X_BOOL_PARAMS_DOC(BOOL_PARAM_DUMP_CONFIG);
  fclose(fp);
  nob_log(NOB_INFO, "config saved successfully");
  return true;
}
int main(int argc, char **argv) {
  GO_REBUILD_URSELF(argc, argv);
  Cmd cmd = {0};
#define BOOL_PARAM_DEFAULT(b) KEEP,
  config cnf = {TARGET_KEEP, FLOAT_KEEP, X_BOOL_PARAMS(BOOL_PARAM_DEFAULT)};
  config loaded_cnf;
   if (!mkdir_if_not_exists("config")) {
    return 1;
  }
  shift(argv, argc);
  while (argc > 0) {
    char *arg = shift(argv, argc);
    if (0) {
    } // so that we can have else ifs
#define BOOL_PARAM_PARSE(b)                                                    \
  else if (strstr_no_case((arg), #b "=") == arg) {                                 \
    char *v = (arg) + sizeof(#b);                                              \
    (cnf).PPARAM_##b = (bool)atoi(v); /*make it either UNSET or SET*/          \
  }
#define PARSE_CASE(val, param)                                                 \
  else if (strcmp_no_case(v, #val) == 0) {                                         \
    (cnf).P##param = val;                                                      \
  }
#define NONBOOL_PARAM_PARSE(param, VALUES_VA)                                  \
  else if (strstr_no_case((arg), #param "=") == arg) {                             \
    char *v = (arg) + sizeof(#param);                                          \
    if (0) {                                                                   \
    }                                                                          \
    VALUES_VA(PARSE_CASE, param)                                               \
    else {                                                                     \
      nob_log(NOB_WARNING, "Invalid value for " #param ": %s. Ignoring", v);   \
    }                                                                          \
  }
    if(strcmp_no_case(arg, "clean")==0){
        return clean();
    }
    X_BOOL_PARAMS(BOOL_PARAM_PARSE)
    NONBOOL_PARAM_PARSE(TARGET, TARGET_VALUES_VA)
    NONBOOL_PARAM_PARSE(FLOAT, FLOAT_VALUES_VA)
  }
  load_config("./config/build_config.h", &loaded_cnf);
  merge_configs(&loaded_cnf, &cnf);
  print_config(loaded_cnf);
  if (!dump_config("./config/build_config.h", &loaded_cnf)) {
    return 1;
  }
  const char *stage2 = "./targets/stage2";
  cmd_append(&cmd, NOB_REBUILD_URSELF(stage2, "./targets/stage2.c"));
  if (!cmd_run(&cmd))
    return 1;
  cmd_append(&cmd, stage2);
  if (!cmd_run(&cmd))
      return 1;
}
