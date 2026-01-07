#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "../nob.h"
#include "../config/build_config.h"
#define BUILD_DIR "./build"
#define UTILS_DIR "./utils"
#define SRC_DIR "./src"
#define INCLUDES "/usr/include"
#define BUILD_DIR_LIB BUILD_DIR"/lib"
#define BUILD_DIR_INCLUDE BUILD_DIR"/include"
#define BUILD_DIR_BIN BUILD_DIR/"bin"
#define CC ""
#ifdef TARGET_LINUX
#include "target_linux.h"
#elif defined(TARGET_WIN_MINGW32) //elifdef is a C23 extension
#include "target_mingw.c"
#endif
#ifndef PARAM_SINGLETHREAD
#define LDLIBS "-lm", "-fopenmp"
#else
#define LDLIBS "-lm"
#endif
const char *sources[] = {"abi_tag", "obj", "scene", "texture", "tri", "vec"};
int main(int argc, char **argv){
   
    Cmd cmd = {0};
    Procs procs = {0};
    for (size_t i=0; i<ARRAY_LEN(sources); i++){
        const char *source = sources[i];
        char *src_file = strdup(source);
        strcat(src_file, ".c");
        char *obj_file = strdup(source);
        strcat(obj_file, ".o");
        obj_file[strlen(obj_file)-1]='o';
#ifdef CLANG_FORMAT
        cmd_append(&cmd, CLANG_FORMAT, "-i", src_file);
        cmd_run(&cmd);
#endif
        cmd_append(&cmd, CC, "-Wall", "-Wextra", "-pedantic", "-march=native", "-ffast-math", "-funroll-loops", ("-I"INCLUDES), "-fPIC", "-c", src_file, "-o", obj_file, LDLIBS);
        cmd_run(&cmd, .async = &procs); 
    }
    if(!procs_flush(&procs)){
        return 1;
    }
    return 0;
}
