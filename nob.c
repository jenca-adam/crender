#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#include "config/build_config.h"

int main(int argc, char **argv){
    GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    if (!mkdir_if_not_exists("build")){
        return 1;
    }
    const char *stage2 = "./targets/stage2";
    cmd_append(&cmd, NOB_REBUILD_URSELF(stage2, "./targets/stage2.c"));
    if (!cmd_run(&cmd)) return 1;
}
