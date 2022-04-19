/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 b1id.c: hashes for identifiers (variable and function names)
*/

#include <stdlib.h>

#include "b1id.h"


static const uint8_t b1_id_stmts[] =
{
#ifdef B1_FEATURE_STMT_ERASE
	B1_ID_STMT_ERASE,
#endif
#ifdef B1_FEATURE_STMT_BREAK_CONTINUE
	B1_ID_STMT_BREAK,
#endif
	B1_ID_STMT_RETURN,
	B1_ID_STMT_PRINT,
	B1_ID_STMT_OPTION,
#ifdef B1_FEATURE_STMT_DATA_READ
	B1_ID_STMT_DATA,
#endif
	B1_ID_STMT_NEXT,
#ifdef B1_FEATURE_STMT_DATA_READ
	B1_ID_STMT_RESTORE,
#endif
#ifdef B1_FEATURE_STMT_STOP
	B1_ID_STMT_STOP,
#endif
#ifdef B1_FEATURE_STMT_WHILE_WEND
	B1_ID_STMT_WHILE,
#endif
	B1_ID_STMT_GOSUB,
	B1_ID_STMT_END,
#ifdef B1_FEATURE_STMT_BREAK_CONTINUE
	B1_ID_STMT_CONTINUE,
#endif
	B1_ID_STMT_SET,
	B1_ID_STMT_GOTO,
#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
	B1_ID_STMT_RANDOMIZE,
#endif
#endif
#ifdef B1_FEATURE_STMT_WHILE_WEND
	B1_ID_STMT_WEND,
#endif
	B1_ID_STMT_INPUT,
	B1_ID_STMT_ELSEIF,
	B1_ID_STMT_IF,
#ifdef B1_FEATURE_FUNCTIONS_USER
	B1_ID_STMT_DEF,
#endif
	B1_ID_STMT_DIM,
	B1_ID_STMT_ON,
#ifdef B1_FEATURE_STMT_DATA_READ
	B1_ID_STMT_READ,
#endif
	B1_ID_STMT_LET,
	B1_ID_STMT_FOR,
	B1_ID_STMT_REM,
	B1_ID_STMT_ELSE,
};

static const B1_T_IDHASH b1_id_stmt_hashes[] =
{
#ifdef B1_FEATURE_STMT_ERASE
	0x22d,
#endif
#ifdef B1_FEATURE_STMT_BREAK_CONTINUE
	0xbb3,
#endif
	0x104e,
	0x10ac,
	0x129e,
#ifdef B1_FEATURE_STMT_DATA_READ
	0x2710,
#endif
	0x31fd,
#ifdef B1_FEATURE_STMT_DATA_READ
	0x3b4c,
#endif
#ifdef B1_FEATURE_STMT_STOP
	0x480a,
#endif
#ifdef B1_FEATURE_STMT_WHILE_WEND
	0x5087,
#endif
	0x6119,
	0x65af,
#ifdef B1_FEATURE_STMT_BREAK_CONTINUE
	0x68be,
#endif
	0x7c57,
	0x7edc,
#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
	0x7f56,
#endif
#endif
#ifdef B1_FEATURE_STMT_WHILE_WEND
	0x80af,
#endif
	0x8f40,
	0xab4b,
	0xba4b,
#ifdef B1_FEATURE_FUNCTIONS_USER
	0xbe83,
#endif
	0xceeb,
	0xd400,
#ifdef B1_FEATURE_STMT_DATA_READ
	0xd4d2,
#endif
	0xe092,
	0xef5d,
	0xf7b2,
	0xf7fa,
};


int b1_id_cmp_hashes(const void *hash1, const void *hash2)
{
	B1_T_IDHASH h1, h2;

	h1 = *((const B1_T_IDHASH *)hash1);
	h2 = *((const B1_T_IDHASH *)hash2);

	return	h1 < h2 ? -1 :
		h1 == h2 ? 0 : 1;
}

uint8_t b1_id_get_stmt_by_hash(B1_T_IDHASH hash)
{
	const B1_T_IDHASH *hash_ptr;

	hash_ptr = bsearch(&hash, b1_id_stmt_hashes, sizeof(b1_id_stmt_hashes) / sizeof(B1_T_IDHASH), sizeof(B1_T_IDHASH), b1_id_cmp_hashes);

	return hash_ptr != NULL ? b1_id_stmts[hash_ptr - b1_id_stmt_hashes] : (uint8_t)B1_ID_STMT_UNKNOWN;
}

// calculates BASIC identifier hash (for 16 bit B1_T_IDHASH type)
B1_T_IDHASH b1_id_calc_hash(const B1_T_CHAR *data, B1_T_INDEX data_size)
{
	uint8_t b;
	B1_T_IDHASH hash;
	B1_T_INDEX i;

	hash = 0xFFFF;

	do
	{
		for(i = 0; i < data_size; i++)
		{
			b = *(((const uint8_t *)data) + i);

			if(B1_T_ISALPHA(b))
			{
				b = B1_T_TOLOWER(b);
			}
			else
			if(B1_T_ISCSTRTERM(b))
			{
				continue;
			}

			hash += b;
			b ^= (uint8_t)(hash >> 8);
			hash <<= 5;
			hash += b;
		}

	} while((uint8_t)(hash >> 8) == 0);

	return hash;
}
