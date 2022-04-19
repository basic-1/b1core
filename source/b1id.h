/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 b1id.c: definitions and types for identifiers (variable and function names)
*/


#ifndef _B1_IDENT_
#define _B1_IDENT_

#include <stdint.h>

#include "b1feat.h"
#include "b1itypes.h"
#include "b1types.h"


#define B1_IDENT_FLAGS_BUSY ((uint8_t)0x1)
#define B1_IDENT_FLAGS_IDTYPE_FN ((uint8_t)0x2)
#define B1_IDENT_FLAGS_ARGNUM_MASK ((uint8_t)0x1C)
#define B1_IDENT_FLAGS_ARGNUM_SHIFT ((uint8_t)0x2)

#define B1_IDENT_GET_FLAGS_ARGNUM(FLAGS) ((((uint8_t)(FLAGS)) & B1_IDENT_FLAGS_ARGNUM_MASK) >> B1_IDENT_FLAGS_ARGNUM_SHIFT)

#define B1_IDENT_TEST_FLAGS_BUSY(FLAGS) (((uint8_t)(FLAGS)) & B1_IDENT_FLAGS_BUSY)
#define B1_IDENT_TEST_FLAGS_IDTYPE_VAR(FLAGS) (!B1_IDENT_TEST_FLAGS_IDTYPE_FN(FLAGS))
#define B1_IDENT_TEST_FLAGS_IDTYPE_FN(FLAGS) (((uint8_t)(FLAGS)) & B1_IDENT_FLAGS_IDTYPE_FN)

#define B1_IDENT_FLAGS_SET_VAR(ARGNUM) ((B1_IDENT_FLAGS_BUSY) | ((((uint8_t)(ARGNUM)) << B1_IDENT_FLAGS_ARGNUM_SHIFT) & B1_IDENT_FLAGS_ARGNUM_MASK))
#define B1_IDENT_FLAGS_SET_VAR_FREE ((uint8_t)0x0)

#define B1_IDENT_FLAGS_FN_BLTIN ((uint8_t)0x20)

#define B1_IDENT_TEST_FLAGS_FN_BLTIN(FLAGS) (((uint8_t)(FLAGS)) & B1_IDENT_FLAGS_FN_BLTIN)

#define B1_IDENT_FLAGS_SET_FN(ARGNUM, BLTIN) ((B1_IDENT_FLAGS_BUSY) | ((((uint8_t)(ARGNUM)) << B1_IDENT_FLAGS_ARGNUM_SHIFT) & B1_IDENT_FLAGS_ARGNUM_MASK) | ((BLTIN) ? B1_IDENT_FLAGS_FN_BLTIN : (uint8_t)0))

#define B1_ID_STMT_UNKNOWN ((uint8_t)0xFF)
#define B1_ID_STMT_ABSENT ((uint8_t)0xFE)
#ifdef B1_FEATURE_STMT_DATA_READ
#define B1_ID_STMT_DATA ((uint8_t)0x0)
#endif
#ifdef B1_FEATURE_FUNCTIONS_USER
#define B1_ID_STMT_DEF ((uint8_t)0x1)
#endif
#define B1_ID_STMT_DIM ((uint8_t)0x2)
#define B1_ID_STMT_ELSE ((uint8_t)0x3)
#define B1_ID_STMT_ELSEIF ((uint8_t)0x4)
#define B1_ID_STMT_END ((uint8_t)0x5)
#ifdef B1_FEATURE_STMT_ERASE
#define B1_ID_STMT_ERASE ((uint8_t)0x6)
#endif
#define B1_ID_STMT_FOR ((uint8_t)0x7)
#define B1_ID_STMT_GOSUB ((uint8_t)0x8)
#define B1_ID_STMT_GOTO ((uint8_t)0x9)
#define B1_ID_STMT_IF ((uint8_t)0xA)
#define B1_ID_STMT_INPUT ((uint8_t)0xB)
#define B1_ID_STMT_LET ((uint8_t)0xC)
#define B1_ID_STMT_NEXT ((uint8_t)0xD)
#define B1_ID_STMT_ON ((uint8_t)0xE)
#define B1_ID_STMT_OPTION ((uint8_t)0xF)
#define B1_ID_STMT_PRINT ((uint8_t)0x10)
#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
#define B1_ID_STMT_RANDOMIZE ((uint8_t)0x11)
#endif
#endif
#ifdef B1_FEATURE_STMT_DATA_READ
#define B1_ID_STMT_READ ((uint8_t)0x12)
#endif
#define B1_ID_STMT_REM ((uint8_t)0x13)
#ifdef B1_FEATURE_STMT_DATA_READ
#define B1_ID_STMT_RESTORE ((uint8_t)0x14)
#endif
#define B1_ID_STMT_RETURN ((uint8_t)0x15)
#define B1_ID_STMT_SET ((uint8_t)0x16)
#ifdef B1_FEATURE_STMT_WHILE_WEND
#define B1_ID_STMT_WEND ((uint8_t)0x17)
#define B1_ID_STMT_WHILE ((uint8_t)0x18)
#endif
#ifdef B1_FEATURE_STMT_BREAK_CONTINUE
#define B1_ID_STMT_BREAK ((uint8_t)0x19)
#define B1_ID_STMT_CONTINUE ((uint8_t)0x1A)
#endif
#ifdef B1_FEATURE_STMT_STOP
#define B1_ID_STMT_STOP ((uint8_t)0x1B)
#endif


typedef struct
{
	uint16_t name_hash;
	uint8_t flags;
#ifdef B1_FEATURE_DEBUG
	B1_T_CHAR name[B1_MAX_IDENTIFIER_LEN + 1];
#endif
} B1_ID;


extern int b1_id_cmp_hashes(const void *hash1, const void *hash2);
extern uint8_t b1_id_get_stmt_by_hash(B1_T_IDHASH hash);
extern B1_T_IDHASH b1_id_calc_hash(const B1_T_CHAR *data, B1_T_INDEX data_size);

#endif
