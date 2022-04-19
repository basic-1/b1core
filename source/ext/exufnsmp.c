/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exufnsmp.c: implementing user-defined functions cache for microcontrollers
*/


#include "b1fn.h"
#include "b1err.h"


#ifdef B1_FEATURE_FUNCTIONS_USER
#define B1_MAX_UDEF_FN_NUM ((uint8_t)7)


static B1_UDEF_FN b1_ex_ufn_fns[B1_MAX_UDEF_FN_NUM];


extern B1_T_ERROR b1_ex_ufn_init()
{
	B1_T_INDEX i;
        B1_UDEF_FN *fn;

	fn = b1_ex_ufn_fns;

	for(i = 0; i < B1_MAX_UDEF_FN_NUM; i++, fn++)
	{
		(*fn).fn.id.flags = 0;
	}

	return B1_RES_OK;
}

extern B1_T_ERROR b1_ex_ufn_get(B1_T_IDHASH name_hash, uint8_t alloc_new, B1_UDEF_FN **fn)
{
	B1_T_INDEX i;

	for(i = 0; i < B1_MAX_UDEF_FN_NUM; i++)
	{
		*fn = b1_ex_ufn_fns + i;

		if(B1_IDENT_TEST_FLAGS_BUSY((**fn).fn.id.flags))
		{
			if((**fn).fn.id.name_hash == name_hash)
			{
				return B1_RES_OK;
			}
		}
		else
		{
			return B1_RES_EUNKIDENT;
		}
	}

	return alloc_new ? B1_RES_EMANYDEF : B1_RES_EUNKIDENT;
}
#endif
