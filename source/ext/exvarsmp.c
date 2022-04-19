/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exvarsmp.c: simple variables cache for microcontrollers
*/


#include <stdlib.h>

#include "b1var.h"
#include "b1err.h"


#define B1_MAX_VAR_NUM (256 / sizeof(B1_NAMED_VAR))


// variables store
static B1_NAMED_VAR b1_ex_var_vars[B1_MAX_VAR_NUM];
static B1_NAMED_VAR *b1_ex_var_free_var;


B1_T_ERROR b1_ex_var_init()
{
	B1_T_INDEX i;
	B1_NAMED_VAR *var;

	var = b1_ex_var_vars;
	b1_ex_var_free_var = var;

	for(i = 0; i < B1_MAX_VAR_NUM; i++, var++)
	{
		(*var).id.flags = 0;
	}

	return B1_RES_OK;
}

// creates new variable, fills B1_ID structure only (hash, arg. num)
B1_T_ERROR b1_ex_var_alloc(B1_T_IDHASH name_hash, B1_NAMED_VAR **var)
{
	B1_T_INDEX i;

	for(i = 0; i < B1_MAX_VAR_NUM; i++)
	{
		if(B1_IDENT_TEST_FLAGS_BUSY(b1_ex_var_vars[i].id.flags))
		{
			if(name_hash == b1_ex_var_vars[i].id.name_hash)
			{
				if(var != NULL)
				{
					*var = b1_ex_var_vars + i;
				}

				return B1_RES_EIDINUSE;
			}
		}
		else
		if(b1_ex_var_free_var == NULL)
		{
			b1_ex_var_free_var = b1_ex_var_vars + i;
		}
	}

	if(b1_ex_var_free_var == NULL)
	{
		return B1_RES_ENOMEM;
	}

	*var = b1_ex_var_free_var;
	b1_ex_var_free_var = NULL;

	return B1_RES_OK;
}

// frees memory occupied by B1_NAMED_VAR structure
B1_T_ERROR b1_ex_var_free(B1_NAMED_VAR *var)
{
	b1_ex_var_free_var = var;
	(*b1_ex_var_free_var).id.flags = B1_IDENT_FLAGS_SET_VAR_FREE;
	return B1_RES_OK;
}
