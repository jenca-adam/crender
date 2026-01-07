#include <crender.h>
#include <stdio.h>
#include <unistd.h>
#ifdef __WIN32__
#include <wingetopt/getopt.h>
#endif
enum Mode { UNSPECIFIED = 0, NORMALS, OBJECT, OBJECT_SMOOTH, OBJECT_FLAT };

struct args {
  enum Mode mode;
  char *object_file; // only for NORMALS mode
  char *input;
  char *output;
};

void print_usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s [options] [FILE]\n"
          "A combined baking utility for crender\n\n"
          "Options:\n"
          "  -o FILE          Write output to FILE instead of stdout\n"
          "Modes (exactly one required):\n"
          "  -n OBJECT        Bake object space normal map for OBJECT from a "
          "tangent space normal map\n"
          "  -O               Bake object file without changing normals\n"
          "  -s               Bake object file with smooth normals\n"
          "  -f               Bake object file with flat normals\n",
          prog);
}

int crbake_normal_map(char *infile, char *objfile, char *outfile) {
  cr_ASSERT(infile != NULL, "");
  cr_ASSERT(objfile != NULL, "");
  cr_ASSERT(outfile != NULL, "");
  cr_Texture normal_map = cr_Texture_readPPM(infile);
  if (!normal_map.valid)
    return 1;
  cr_Object object = cr_Object_readOBJ(objfile, NONE);
  if (!object.valid)
    return 1;
  cr_Texture baked =
      cr_Texture_bake_object_space_normal_map(&normal_map, &object);
  cr_Texture_writePPM(&baked, outfile);
  return 0;
}

int crbake_object(char *infile, char *outfile, cr_NormalPrecompMode mode) {
  cr_ASSERT(infile != NULL, "");
  cr_ASSERT(outfile != NULL, "");
  cr_Object object = cr_Object_readOBJ(infile, mode);
  if (!object.valid)
    return 1;
  cr_Object_writeOBJ(&object, outfile);
  return 0;
}

int main(int argc, char *argv[]) {
  cr_INIT_CRENDER();

  struct args args = {UNSPECIFIED, NULL, NULL,
                      "/dev/stdout"}; // cross-platform issue noted
  int opt;

  while ((opt = getopt(argc, argv, "o:n:Osf")) != -1) {
    switch (opt) {
    case 'o':
      args.output = optarg;
      break;
    case 'n':
      if (args.mode != UNSPECIFIED) {
        fprintf(stderr, "Error: Only one mode can be specified\n");
        print_usage(argv[0]);
        return 1;
      }
      args.mode = NORMALS;
      args.object_file = optarg;
      break;
    case 'O':
      if (args.mode != UNSPECIFIED) {
        fprintf(stderr, "Error: Only one mode can be specified\n");
        print_usage(argv[0]);
        return 1;
      }
      args.mode = OBJECT;
      break;
    case 's':
      if (args.mode != UNSPECIFIED) {
        fprintf(stderr, "Error: Only one mode can be specified\n");
        print_usage(argv[0]);
        return 1;
      }
      args.mode = OBJECT_SMOOTH;
      break;
    case 'f':
      if (args.mode != UNSPECIFIED) {
        fprintf(stderr, "Error: Only one mode can be specified\n");
        print_usage(argv[0]);
        return 1;
      }
      args.mode = OBJECT_FLAT;
      break;
    default: /* '?' */
      print_usage(argv[0]);
      return 1;
    }
  }

  // Remaining argument is input file
  if (optind < argc) {
    args.input = argv[optind];
  } else {
    fprintf(stderr, "Error: Input file not specified\n");
    print_usage(argv[0]);
    return 1;
  }

  if (args.mode == UNSPECIFIED) {
    fprintf(stderr, "Error: Exactly one mode flag must be provided\n");
    print_usage(argv[0]);
    return 1;
  }

  switch (args.mode) {
  case NORMALS:
    return crbake_normal_map(args.input, args.object_file, args.output);
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
