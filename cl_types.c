#include "cl_types.h"
int atomic_xchg(volatile int *p, int val) {
	int old = *p;
	*p = val;
	return old;
}

uint rotate(uint x, uint y) {
	return (((x) << (y) | ((x) >>  (32 - (y)))));
}

ulong rotate(ulong x, ulong y) {
	return (((x) << (y) | ((x) >>  (64 - (y)))));
}
