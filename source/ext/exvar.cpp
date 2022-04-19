/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exvar.cpp: implementing variables cache
*/


#include <cstdlib>
#include <map>

extern "C"
{
#include "b1var.h"
#include "b1err.h"
}


static std::map<B1_T_IDHASH, B1_NAMED_VAR> b1_ex_vars;


extern "C" B1_T_ERROR b1_ex_var_init()
{
	b1_ex_vars.clear();

	return B1_RES_OK;
}

// allocates memory for B1_NAMED_VAR structure or returns pointer to existing one (indicating this with B1_RES_EIDINUSE return code)
extern "C" B1_T_ERROR b1_ex_var_alloc(B1_T_IDHASH name_hash, B1_NAMED_VAR **var)
{
	auto var_it = b1_ex_vars.find(name_hash);
	
	if(var_it == b1_ex_vars.end())
	{
		*var = &b1_ex_vars[name_hash];
		return B1_RES_OK;
	}

	*var = &var_it->second;
	return B1_RES_EIDINUSE;
}

// frees memory occupied by B1_NAMED_VAR structure
extern "C" B1_T_ERROR b1_ex_var_free(B1_T_IDHASH name_hash)
{
	auto var_it = b1_ex_vars.find(name_hash);

	if(var_it != b1_ex_vars.end())
	{
		b1_ex_vars.erase(var_it);
	}

	return B1_RES_OK;
}

#ifdef B1_FEATURE_INIT_FREE_MEMORY
extern "C" B1_T_ERROR b1_ex_var_enum(B1_NAMED_VAR **var)
{
	auto var_it = b1_ex_vars.begin();

	if(*var != NULL)
	{
		var_it = b1_ex_vars.find((*var)->id.name_hash);
		if(var_it != b1_ex_vars.end())
		{
			var_it++;
		}
	}

	if(var_it == b1_ex_vars.end())
	{
		*var = NULL;
	}
	else
	{
		*var = &var_it->second;
	}

	return B1_RES_OK;
}
#endif
