/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exrndsmp.c: simplified random values generator
*/


// include "b1types.h" for B1_FRACTIONAL_TYPE_EXISTS macro
#include "b1types.h"
#include "b1int.h"


#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS


static B1_T_RAND_SEED b1_fn_rand_seed;


B1_T_RAND_SEED b1_ex_rnd_get_next_seed()
{
	b1_fn_rand_seed ^= (b1_fn_rand_seed << 13);
	b1_fn_rand_seed ^= (b1_fn_rand_seed >> 9);
	b1_fn_rand_seed ^= (b1_fn_rand_seed << 7);
	return b1_fn_rand_seed;
}

void b1_ex_rnd_randomize(uint8_t init)
{
	b1_fn_rand_seed += init ? (B1_T_RAND_SEED)0xFFFF : (B1_T_RAND_SEED)(b1_int_print_curr_pos + 17);
}
#endif
#endif
