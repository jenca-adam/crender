/* ----- available targets ----- */
#define TARGET_LINUX
// #define TARGET_WIN_MINGW32
/* ----- build  parameters ----- */
#define PARAM_FLOAT32 // use 32-bit floats instead of 64-bit floats
// #define PARAM_FLOAT64 // use 64-bit floats instead of 32-bit floats. not tested.
// #define PARAM_DEBUG // build with -g3
// #define PARAM_EXPAND_MACRO // expand _cr_Texture_draw_face_XXX macros for debugging purposes
// #define PARAM_O0 // build without optimizations
// #define PARAM_SINGLETHREAD // build without multithreading. removes dependency on OpenMP
// #define PARAM_UNSAFE // Don't use mutexes. Will result in a performance boost, but will cause artifacts.
