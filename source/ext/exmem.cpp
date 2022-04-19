/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exmem.cpp: memory management functions for interpreter (using C++ new and
 delete operators)
*/


#include <cstdint>
#include <cstdlib>

extern "C"
{
#include "b1itypes.h"
#include "b1err.h"
}


extern "C" B1_T_ERROR b1_ex_mem_init()
{
	return B1_RES_OK;
}

extern "C" B1_T_ERROR b1_ex_mem_alloc(B1_T_MEMOFFSET size, B1_T_MEM_BLOCK_DESC *mem_desc, void **data)
{
	*mem_desc = new uint8_t[size];

	if(*mem_desc == NULL)
	{
		return B1_RES_ENOMEM;
	}
	
	if(data != NULL)
	{
		*data = (void *)*mem_desc;
	}

	return B1_RES_OK;
}

extern "C" B1_T_ERROR b1_ex_mem_access(const B1_T_MEM_BLOCK_DESC mem_desc, B1_T_MEMOFFSET offset, B1_T_INDEX size, uint8_t options, void **data)
{
	size;
	options;
	*data = (void *)(((uint8_t *)mem_desc) + offset);
	return B1_RES_OK;
}

extern "C" B1_T_ERROR b1_ex_mem_release(const B1_T_MEM_BLOCK_DESC mem_desc)
{
	mem_desc;
	return B1_RES_OK;
}

extern "C" B1_T_ERROR b1_ex_mem_free(const B1_T_MEM_BLOCK_DESC mem_desc)
{
	delete[] (const uint8_t *)mem_desc;
	return B1_RES_OK;
}
