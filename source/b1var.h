/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 b1var.h: types for variables representation
*/


#ifndef _B1_VAR_
#define _B1_VAR_

#include "b1feat.h"
#include "b1itypes.h"
#include "b1types.h"
#include "b1id.h"
#include "b1tok.h"


typedef union
{
	// token record pointer
	const B1_TOKENDATA *token;
	// boolean value (used internally)
	uint8_t bval;
	// immediate string value
	B1_T_CHAR istr[B1_TYPE_STRING_IMM_MAX_LEN + 1];
	// RAM memory block descriptor (used to save strings longer than B1_TYPE_STRING_IMM_MAX_LEN)
	B1_T_MEM_BLOCK_DESC mem_desc;
	// 32-bit integer value
	int32_t i32val;
#ifdef B1_FEATURE_TYPE_SINGLE
	// single precision floating point value
	float sval;
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	// double precision floating point value
	double dval;
#endif
#ifdef B1_FEATURE_TYPE_SMALL
	int16_t i16val;
	uint16_t ui16val;
	uint8_t ui8val;
#endif
} B1_VAL;

typedef struct
{
	uint8_t type;
	B1_VAL value;
} B1_VAR;

typedef struct
{
	B1_ID id;
	B1_VAR var;
} B1_NAMED_VAR;

typedef struct
{
	B1_NAMED_VAR *var;
	B1_T_MEMOFFSET val_off;
} B1_VAR_REF;


extern B1_T_ERROR b1_var_str2var(const B1_T_CHAR *s, B1_VAR *var);
extern B1_T_ERROR b1_var_var2str(const B1_VAR *var, B1_T_CHAR *sbuf);
extern B1_T_ERROR b1_var_convert(B1_VAR *var, uint8_t otype);
extern B1_T_ERROR b1_var_init_empty(uint8_t type, uint8_t argnum, const B1_T_SUBSCRIPT *subs_bounds, B1_VAR *pvar);
extern B1_T_ERROR b1_var_array_get_data_ptr(B1_T_MEM_BLOCK_DESC arr_data_desc, uint8_t type, B1_T_MEMOFFSET offset, void **data);
extern B1_T_ERROR b1_var_create(B1_T_IDHASH name_hash, uint8_t type, uint8_t argnum, const B1_T_SUBSCRIPT *subs_bounds, B1_NAMED_VAR **var);
extern B1_T_ERROR b1_var_get(B1_NAMED_VAR *src_var, B1_VAR *dst_var, B1_VAR_REF *src_var_ref);
extern B1_T_ERROR b1_var_set(B1_VAR *src_var, const B1_VAR_REF *dst_var_ref);

#endif
