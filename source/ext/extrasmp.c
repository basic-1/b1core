/*
 BASIC1 interpreter
 Copyright (c) 2022 Nikolay Pletnev
 MIT license

 extrasmp.c: for special functions (that are not present in all C libraries)
*/


#include <stdint.h>

#include "b1feat.h"


#ifdef B1_FEATURE_TYPE_SINGLE
extern float fmodf(float x, float y)
{
	return (float)((int32_t)x % (int32_t)y);
}
#endif