/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exrndsmp.c: random values generator
*/


// include "b1types.h" for B1_FRACTIONAL_TYPE_EXISTS macro
#include "b1types.h"


#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
#include <stdlib.h>
#include <time.h>


B1_T_RAND_SEED b1_ex_rnd_get_next_seed()
{
	int seed;

	seed = rand();
	return (B1_T_RAND_SEED)((((float)seed) / (float)RAND_MAX) * ((float)B1_T_RAND_SEED_MAX_VALUE));
}

void b1_ex_rnd_randomize(uint8_t init)
{
	srand(init ?
		(unsigned int)0 :
		((unsigned int)time(NULL) * rand())
		);
}
#endif
#endif
