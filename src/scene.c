#include "scene.h"

Entity Entity_new(){
    return (Entity){0};
}

bool Entity_load_dir(Entity *e, char *dirname){
    Texture diffuse = Texture_readPPM("diffuse.ppm", dirname);
    Object *ob = Object_fromOBJ("obj.obj", dirname);
    if (!ob||!diffuse.m){
        return false;
    }
    e->ob = ob;
    e->ts.diffuse = diffuse;
    e->ts.normal_map = Texture_readPPM("normal.ppm", dirname);
    e->ts.specular_map = Texture_readPPM("specular.ppm", dirname);
    return true;
}
