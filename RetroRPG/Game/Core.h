#pragma once

//#pragma warning(disable : 4100)// TODO: delete
//#pragma warning(disable : 4242)// TODO: delete
//#pragma warning(disable : 4244)// TODO: delete
//#pragma warning(disable : 4267)// TODO: delete
//#pragma warning(disable : 4296)// TODO: delete
//#pragma warning(disable : 4324)// TODO: delete
//#pragma warning(disable : 4365)// TODO: delete
//#pragma warning(disable : 4458)// TODO: delete
//#pragma warning(disable : 4505)// TODO: delete
#pragma warning(disable : 4514)// TODO: delete
//#pragma warning(disable : 4706)// TODO: delete
#pragma warning(disable : 4820)// TODO: delete
//#pragma warning(disable : 5039)// TODO: delete
#pragma warning(disable : 5045)// TODO: delete
//#pragma warning(disable : 5219)// TODO: delete

#define  _CRT_SECURE_NO_WARNINGS // TODO: delete

#include "Config.h"

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

typedef unsigned long long Handle;

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif 

// Handful macros
#define cmmax(a,b)             ((a)>(b)?(a):(b))                                   // Return the maximum of two values
#define cmmin(a,b)             ((a)<(b)?(a):(b))                                   // Return the minimum of two values
#define cmabs(a)               ((a)<0.0f?(-(a)):(a))                               // Return the absolute value
#define cmbound(v, vmin, vmax) ((v)>(vmax)?(vmax):((v)<(vmin)?(vmin):(v)))         // Limit a value between two bounds (included)
#define cmsgn(a)               ((a)<0.0f?-1.0f:1.0f)                               // Return the sign (+1.0 or -1.0) of a value
#define cmthr(a, t)            ((a)<(-t)?-1.0f:((a)>(t)?1.0f:0.0f))                // Compare a value to a threshold and return +1.0 (above), -1.0 (below) or 0.0f (inside)
#define cmmod(a, m)            (((a) % (m) + (m)) % (m))                           // Perform a non-signed modulo (integer)
#define cmmodf(a, m)           (fmodf((fmodf((a), (m)) + (m)), (m)))               // Perform a non-signed modulo (float)

#define randf()                (((float) rand() / (float) (RAND_MAX >> 1)) - 1.0f) // Compute a float random pseudo number
#define d2r                    (const float) (M_PI / 180.0f)                       // Mutiply constant to convert degrees in radiants
#define r2d                    (const float) (180.0f / M_PI)                       // Mutiply constant to convert radiants in degrees

// Global string & math functions
namespace Global
{
	void toLower(char* txt); // Convert in place null terminated C string to lower case
	void toUpper(char* txt); // Convert in place null terminated C string to upper case

	void getFileExtention(char* ext, const int extSize, const char* path); // Return a file extension from a path
	void getFileName(char* name, const int nameSize, const char* path);    // Return a file name from a path
	void getFileDirectory(char* dir, int dirSize, const char* path);       // Return a directory name from a path

	int log2i32(int n); // Compute the log2 of a 32bit integer
};

// Endianness conversion macros
#define FROM_U16(x) (x = ((uint8_t *) &(x))[0] + (((uint8_t *) &(x))[1] << 8))
#define FROM_S16(x) FROM_U16(x)
#define FROM_U32(x) (x = ((uint8_t *) &x)[0] + (((uint8_t *) &x)[1] << 8) + (((uint8_t *) &x)[2] << 16) + (((uint8_t *) &x)[3] << 24))
#define FROM_S32(x) FROM_U32(x)

#define TO_U16(x) (\
		((uint8_t *) &(x))[0] = ((uint16_t) (x)) & 0xFF, \
		((uint8_t *) &(x))[1] = ((uint16_t) (x)) >> 8    \
	)

#define TO_S16(x) (\
		((uint8_t *) &(x))[0] = ((int16_t) (x)) & 0xFF, \
		((uint8_t *) &(x))[1] = ((int16_t) (x)) >> 8    \
	)

#define TO_U32(x) (\
		((uint8_t *) &(x))[0] = ((uint32_t) (x)) & 0xFF, \
		((uint8_t *) &(x))[1] = ((uint32_t) (x)) >> 8,   \
		((uint8_t *) &(x))[2] = ((uint32_t) (x)) >> 16,  \
		((uint8_t *) &(x))[3] = ((uint32_t) (x)) >> 24   \
	)

#define TO_S32(x) (\
		((uint8_t *) &(x))[0] = ((int32_t) (x)) & 0xFF, \
		((uint8_t *) &(x))[1] = ((int32_t) (x)) >> 8,   \
		((uint8_t *) &(x))[2] = ((int32_t) (x)) >> 16,  \
		((uint8_t *) &(x))[3] = ((int32_t) (x)) >> 24   \
	)

// Compilers missing - intrinsics and maths functions
#ifdef _MSC_VER
#include <intrin.h>
extern "C" int __builtin_ffs(int x);
#endif