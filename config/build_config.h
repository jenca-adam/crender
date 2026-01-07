/* ----- available targets ----- */
// #define TARGET_LINUX //Linux
#define TARGET_WIN_MINGW32 //Windows (MinGW)
// #define TARGET_WIN_MSVC //Windows (Microsoft Visual C)
/* -----   float sizes     ----- */
#define FLOAT_F32 //Use 32-bit floats
// #define FLOAT_F64 //Use 64-bit floats
/* ----- build parameters  ----- */
#define PARAM_DEBUG //build with debugging information
// #define PARAM_EXPAND_MACRO //expand _cr_Texture_draw_face_XXX macros for debugging purposes
// #define PARAM_O0 //build without optimizations
#define PARAM_SINGLETHREAD //build without multithreading. removes dependency on OpenMP
// #define PARAM_UNSAFE //don't use mutex locks. will result in a performance boost, but will cause artifacts
#define PARAM_STATIC //also link statically.
