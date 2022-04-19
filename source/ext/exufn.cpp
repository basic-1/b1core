/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exufn.cpp: implementing user-defined functions data cache
*/


#include <map>

extern "C"
{
#include "b1fn.h"
#include "b1err.h"
}


#ifdef B1_FEATURE_FUNCTIONS_USER
static std::map<B1_T_IDHASH, B1_UDEF_FN> b1_ex_ufn_ufns;


extern "C" B1_T_ERROR b1_ex_ufn_init()
{
	b1_ex_ufn_ufns.clear();

	return B1_RES_OK;
}

extern "C" B1_T_ERROR b1_ex_ufn_get(B1_T_IDHASH name_hash, uint8_t alloc_new, B1_UDEF_FN **fn)
{
	auto fn_it = b1_ex_ufn_ufns.find(name_hash);
	
	if(fn_it == b1_ex_ufn_ufns.end())
	{
		if(alloc_new)
		{
			*fn = &b1_ex_ufn_ufns[name_hash];
		}

		return B1_RES_EUNKIDENT;
	}

	*fn = &fn_it->second;

	return B1_RES_OK;
}
#endif