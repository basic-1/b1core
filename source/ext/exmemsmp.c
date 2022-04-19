/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exmemsmp.c: memory management functions for interpreter (allocate memory
 within static block, can be useful for microcontrollers)
*/


#include <stdlib.h>

#include "b1itypes.h"
#include "b1err.h"


#define B1_EX_MEM_MEMORY_SIZE 512
#define B1_EX_MEM_MAX_BLOCK_SIZE 255

#define B1_EX_MEM_BLOCK_HDR_SIZE 2

#define B1_EX_MEM_WRITE_HEADER_BUSY(HEADER_PTR, BUSY) *((uint16_t *)(HEADER_PTR)) &= ((BUSY) ? 0xFFFF : 0x7FFF)
#define B1_EX_MEM_WRITE_HEADER(HEADER_PTR, BUSY, SIZE) *((uint16_t *)(HEADER_PTR)) = ((BUSY) ? 0x8000 : 0) | ((uint16_t)(SIZE))

#define B1_EX_MEM_READ_HEADER_BUSY(HEADER_PTR) (((uint8_t)((*((uint16_t *)(HEADER_PTR))) >> 8)) & (uint8_t)0x80)
#define B1_EX_MEM_READ_HEADER_SIZE(HEADER_PTR) (*((uint16_t *)(HEADER_PTR)) & 0x7FFF)

#define B1_EX_MEM_READ_HEADER(HEADER_PTR, BUSY, SIZE) \
do { \
	BUSY = B1_EX_MEM_READ_HEADER_BUSY(HEADER_PTR); \
	SIZE = B1_EX_MEM_READ_HEADER_SIZE(HEADER_PTR); \
} while (0)


static uint8_t b1_ex_mem_data_buffer[B1_EX_MEM_MEMORY_SIZE];


B1_T_ERROR b1_ex_mem_init()
{
	B1_T_MEMOFFSET i;

	if((B1_EX_MEM_MEMORY_SIZE) / ((B1_EX_MEM_MAX_BLOCK_SIZE) / 2 + (B1_EX_MEM_BLOCK_HDR_SIZE)) > 0)
	{
		i = (B1_EX_MEM_MEMORY_SIZE) / ((B1_EX_MEM_MAX_BLOCK_SIZE) / 2 + (B1_EX_MEM_BLOCK_HDR_SIZE));
		while(i > 0)
		{
			i--;
			B1_EX_MEM_WRITE_HEADER(b1_ex_mem_data_buffer + (i * ((B1_EX_MEM_MAX_BLOCK_SIZE) / 2 + (B1_EX_MEM_BLOCK_HDR_SIZE))), 0 , (B1_EX_MEM_MAX_BLOCK_SIZE) / 2);
		}
	}

	if((B1_EX_MEM_MEMORY_SIZE) % ((B1_EX_MEM_MAX_BLOCK_SIZE) / 2 + (B1_EX_MEM_BLOCK_HDR_SIZE)) > 0)
	{
		B1_EX_MEM_WRITE_HEADER(b1_ex_mem_data_buffer + (((B1_EX_MEM_MEMORY_SIZE) / ((B1_EX_MEM_MAX_BLOCK_SIZE) / 2 + (B1_EX_MEM_BLOCK_HDR_SIZE))) * ((B1_EX_MEM_MAX_BLOCK_SIZE) / 2 + (B1_EX_MEM_BLOCK_HDR_SIZE))), 0, (B1_EX_MEM_MEMORY_SIZE) % ((B1_EX_MEM_MAX_BLOCK_SIZE) / 2 + (B1_EX_MEM_BLOCK_HDR_SIZE)) - (B1_EX_MEM_BLOCK_HDR_SIZE));
	}
	
	return B1_RES_OK;
}

B1_T_ERROR b1_ex_mem_alloc(B1_T_MEMOFFSET size, B1_T_MEM_BLOCK_DESC *mem_desc, void **data)
{
	uint8_t b, bnew, *ptr;
	B1_T_MEMOFFSET sz, sznew;

	if(size > B1_EX_MEM_MAX_BLOCK_SIZE)
	{
		return B1_RES_ENOMEM;
	}

	ptr = b1_ex_mem_data_buffer;

	while(1)
	{
		B1_EX_MEM_READ_HEADER(ptr, b, sz);

		if(b || sz < size)
		{
			sz += (B1_EX_MEM_BLOCK_HDR_SIZE);
			ptr += sz;

			if(ptr - b1_ex_mem_data_buffer >= (B1_EX_MEM_MEMORY_SIZE))
			{
				return B1_RES_ENOMEM;
			}

			// if both current and next blocks are empty aggregate them
			B1_EX_MEM_READ_HEADER(ptr, bnew, sznew);
			if(!b && !bnew)
			{
				ptr -= sz;
				B1_EX_MEM_WRITE_HEADER(ptr, 0, sz + sznew);
			}

			continue;
		}

		// free block is found
		sznew = size + (B1_EX_MEM_BLOCK_HDR_SIZE);
		if(sz >= sznew)
		{
			// divide found block on two smaller blocks
			B1_EX_MEM_WRITE_HEADER(ptr + sznew, 0, sz - sznew);
			sz = size;
		}

		B1_EX_MEM_WRITE_HEADER(ptr, 1, sz);
		ptr += B1_EX_MEM_BLOCK_HDR_SIZE;
		// B1_T_MEM_BLOCK_DESC is pointer
		*mem_desc = ptr;
		if(data != NULL)
		{
			*data = ptr;
		}

		break;
	}

	return B1_RES_OK;
}

B1_T_ERROR b1_ex_mem_access(const B1_T_MEM_BLOCK_DESC mem_desc, B1_T_MEMOFFSET offset, B1_T_INDEX size, uint8_t options, void **data)
{
	uint8_t b;
	const uint8_t *ptr;

	size;
	options;

	ptr = ((const uint8_t *)mem_desc) - (B1_EX_MEM_BLOCK_HDR_SIZE);

	b = B1_EX_MEM_READ_HEADER_BUSY(ptr);

	if(b)
	{
		*data = (void *)(((uint8_t *)mem_desc) + offset);
		return B1_RES_OK;
	}

	return B1_RES_EINVMEMBLK;
}

B1_T_ERROR b1_ex_mem_release(const B1_T_MEM_BLOCK_DESC mem_desc)
{
	mem_desc;
	return B1_RES_OK;
}

B1_T_ERROR b1_ex_mem_free(const B1_T_MEM_BLOCK_DESC mem_desc)
{
	uint8_t b;
	const uint8_t *ptr;

	ptr = ((const uint8_t *)mem_desc) - (B1_EX_MEM_BLOCK_HDR_SIZE);

	b = B1_EX_MEM_READ_HEADER_BUSY(ptr);

	if(b)
	{
		B1_EX_MEM_WRITE_HEADER_BUSY(ptr, 0);
		return B1_RES_OK;
	}

	return B1_RES_EINVMEMBLK;
}
