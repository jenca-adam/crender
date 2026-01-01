#include<crender.h>
#include<argp.h>

const char *argp_program_version = "crbake 1.0";
const char *argp_program_bug_address = "<jenca.adam@gmail.com>";
static char doc[] = "A combined baking utility for crender";
static char args_doc[] = "[FILE] ...";
static struct argp_option options[] = {
    { "output", 'o', "FILE", OPTION_ARG_OPTIONAL, "Write output to FILE instead of out.ppm", 0},
     { 0, 0, 0, 0, "Modes", 0},
    { "normal-map" , 'n', "OBJECT", OPTION_ARG_OPTIONAL, "Bake object space normal map from a tangent space normal map", 1},
    {0}
};
struct args {
    enum {UNSPECIFIED = 0, NORMALS} mode;
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
    outfile = outfile?outfile:"out.ppm";
    cr_Texture normal_map = cr_Texture_readPPM(infile);
    cr_Object object = cr_Object_fromOBJ(objfile);
    cr_Texture baked = cr_Texture_bake_object_space_normal_map(&normal_map, &object);
    cr_Texture_writePPM(&baked,outfile);
    return 0;
}
int main(int argc, char *argv[]){
    cr_INIT_CRENDER();
    struct args args = {0, NULL, 0, "out.ppm"};
    argp_parse(&argp, argc, argv, 0, 0, &args);
    switch(args.mode){
        case NORMALS:
            return crbake_normal_map(args.input, args.mode_args.object_file, args.output);
            break;
        default:
            cr_UNREACHABLE("mode");
    }

}
