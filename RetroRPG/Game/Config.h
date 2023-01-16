#pragma once

// Engine configuration
#define MAX_FILE_EXTENSION 8          // Maximum file extension string length
#define MAX_FILE_NAME      128        // Maximum file name string length
#define MAX_FILE_PATH      256        // Maximum file path string length

// Data caches
#define BMPCACHE_SLOTS 1024           // Maximum number of bitmaps in cache
#define MESHCACHE_SLOTS 1024          // Maximum number of meshes in cache

// Wavefront object parser
#define OBJ_MAX_NAME 256              // Wavefront object maximum name string length
#define OBJ_MAX_LINE 1024             // Wavefront object maximum file line length
#define OBJ_MAX_PATH 256              // Wavefront object maximum path string length

// Bitmap manipulator
#define BMP_MIPMAPS 32                // Maximum number of mipmaps per bitmap

// Renderer configuration
#define RENDERER_NEAR_DEFAULT 1.0f    // Default near clipping distance
#define RENDERER_FAR_DEFAULT 32768.0f // Default far clipping distance
#define RENDERER_FOV_DEFAULT 65.0f    // Default field of view
#define RENDERER_3DFRUSTRUM 1         // Use a 3D frustrum to clip triangles
#define RENDERER_2DFRAME 0            // Use a 2D frame to clip triangles

#define RENDERER_INTRASTER 0          // Enable fixed point or floating point rasterizing

#define TRILIST_MAX 50000             // Maximum number of triangles in display list
#define VERLIST_MAX 150000            // Maximum number of vertexes in transformation buffer

#define USE_SIMD 1                    // Use generic compiler support for SIMD instructions

// Performance optimizations
#define USE_SSE2 1                    // Use Intel SSE2 instructions