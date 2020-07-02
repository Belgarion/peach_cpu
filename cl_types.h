#ifndef CL_TYPES_H
#define CL_TYPES_H

#include <stdint.h>
#include <stdio.h>

typedef uint64_t ulong;
typedef uint32_t uint;
typedef uint8_t uchar;
#define global
#define __kernel
#define __constant
#define __global

#define bitselect(a, b, c)	((a) ^ ((c) & ((b) ^ (a))))
//#define rotate(x, y)		(((x) << (y)) | ((x) >> (32 - (y))))
#define clz(x) __builtin_clz(x)
int atomic_xchg(volatile int *p, int val);
uint rotate(uint x, uint y);
ulong rotate(ulong x, ulong y);

#endif
