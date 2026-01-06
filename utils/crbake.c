#include<crender.h>
#include<argp.h>

const char *argp_program_version = "crbake 1.0";
const char *argp_program_bug_address = "<jenca.adam@gmail.com>";
static char doc[] = "A combined baking utility for crender";
static char args_doc[] = "[FILE] ...";
static struct argp_option options[] = {
    { "output", 'o', "FILE", 0, "Write output to FILE instead of the standard output", 0},
     { 0, 0, 0, 0, "Modes", 0},
    { "normal-map" , 'n', "OBJECT", 0, "Bake object space normal map for OBJECT from a tangent space normal map", 1},
    { "object", 'O', NULL, 0, "Bake object file without changing normals", 1},
    { "smooth-object", 's', NULL, 0, "Bake object file with smooth normals",1},
    { "flat-object", 'f', NULL, 0, "Bake object file with flat normals",1}, 
    {0}
};
struct args {
    enum {UNSPECIFIED = 0, NORMALS, OBJECT, OBJECT_SMOOTH, OBJECT_FLAT} mode;
    union {struct {char *object_file;};} mode_args;
    char *input; 
    char *output;
};

static error_t parse_options(int key, char *arg, struct argp_state *state){
    struct args *args = (struct args*)state->input;
    switch(key){
        case 'n':
            if(args->mode!=UNSPECIFIED){
                goto mode_fail;
            }
            args->mode = NORMALS;
            args->mode_args.object_file = arg;
            break;
        case 'O':
            if (args->mode!=UNSPECIFIED){
                goto mode_fail;
            }
            args->mode = OBJECT;
            break; 
        case 's':
            if (args->mode!=UNSPECIFIED){
                goto mode_fail;
            }
            args->mode = OBJECT_SMOOTH;
            break; 
        case 'f':
            if (args->mode!=UNSPECIFIED){
                goto mode_fail;
            }
            args->mode = OBJECT_FLAT;
            break; 
        case 'o':
            args->output = arg;
            break;
        case ARGP_KEY_ARG:
            args->input = arg;
            break;
        case ARGP_KEY_NO_ARGS:
            argp_usage(state);
            break;
        case ARGP_KEY_END:
mode_fail:
            if(args->mode==UNSPECIFIED){
                argp_error(state, "Exactly one mode flag must be provided");
                
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
struct argp argp = {options,parse_options, args_doc, doc, NULL, NULL, NULL};

int crbake_normal_map(char *infile, char *objfile, char *outfile){
    cr_ASSERT(infile!=NULL, "");
    cr_ASSERT(objfile!=NULL, "");
    cr_ASSERT(outfile!=NULL, "");
    cr_Texture normal_map = cr_Texture_readPPM(infile);
    if (!normal_map.valid) return 1;
    cr_Object object = cr_Object_readOBJ(objfile, NONE);
    if (!object.valid) return 1;
    cr_Texture baked = cr_Texture_bake_object_space_normal_map(&normal_map, &object);
    cr_Texture_writePPM(&baked,outfile);
    return 0;
}
int crbake_object(char *infile, char *outfile, cr_NormalPrecompMode mode){
    cr_ASSERT(infile!=NULL, "");
    cr_ASSERT(outfile!=NULL, "");
    cr_Object object = cr_Object_readOBJ(infile, mode);
    if (!object.valid) return 1;
    cr_Object_writeOBJ(&object, outfile);
    return 0;
}
int main(int argc, char *argv[]){
    cr_INIT_CRENDER();

    struct args args = {0, NULL, 0, "/dev/stdout"}; // TODO: a cross-platform way of doing this?? 
    argp_parse(&argp, argc, argv, 0, 0, &args);
    switch(args.mode){
        case NORMALS:
            return crbake_normal_map(args.input, args.mode_args.object_file, args.output);
        case OBJECT:
            return crbake_object(args.input, args.output, NONE);
        case OBJECT_FLAT:
            return crbake_object(args.input, args.output, FLAT);
        case OBJECT_SMOOTH:
            return crbake_object(args.input, args.output, SMOOTH);
        default:
            cr_UNREACHABLE("mode");
    }

}
