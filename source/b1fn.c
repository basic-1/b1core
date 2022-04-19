/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 b1fn.c: built-in and user defined functions
*/


#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "b1ex.h"
#include "b1tok.h"
#include "b1fn.h"
#include "b1eval.h"
#include "b1int.h"
#include "b1err.h"


// global data
// user defined functions RPN store
#ifdef B1_FEATURE_FUNCTIONS_USER
B1_T_INDEX b1_fn_udef_fn_rpn_off = 0;
B1_RPNREC b1_fn_udef_fn_rpn[B1_MAX_UDEF_FN_RPN_LEN];
// user defined function call stack
B1_UDEF_CALL b1_fn_udef_call_stack[B1_MAX_UDEF_CALL_NEST_DEPTH];
#endif


#ifdef B1_FEATURE_FUNCTIONS_STANDARD
static B1_T_ERROR b1_fn_bltin_iif(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_striif(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_len(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_asc(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_chr(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_str(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_val(B1_VAR *parg1);
#endif

#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
static B1_T_ERROR b1_fn_bltin_abs(B1_VAR *parg1);
#ifdef B1_FRACTIONAL_TYPE_EXISTS
static B1_T_ERROR b1_fn_bltin_int(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_rnd(B1_VAR *parg1);
#endif
static B1_T_ERROR b1_fn_bltin_sgn(B1_VAR *parg1);
#endif

#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
static B1_T_ERROR b1_fn_bltin_atn(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_cos(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_exp(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_log(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_pi(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_sin(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_sqr(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_tan(B1_VAR *parg1);
#endif

#ifdef B1_FEATURE_FUNCTIONS_STRING
static B1_T_ERROR b1_fn_bltin_mid(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_instr(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_ltrim(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_rtrim(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_left(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_right(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_lset(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_rset(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_ucase(B1_VAR *parg1);
static B1_T_ERROR b1_fn_bltin_lcase(B1_VAR *parg1);
#endif


// array of built-in functions definitions, sorted by name hash value (to use binary search)
#if defined(B1_FEATURE_FUNCTIONS_STANDARD) || defined(B1_FEATURE_FUNCTIONS_MATH_BASIC) || defined(B1_FEATURE_FUNCTIONS_MATH_EXTRA) || defined(B1_FEATURE_FUNCTIONS_STRING)
static const B1_BLTIN_FN b1_fn_bltin[B1_FN_BLTIN_COUNT] =
{
#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
	{{{0x5af, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_FP_HIGH_PREC}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_log},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
	{{{0x1924, B1_IDENT_FLAGS_SET_FN(0, 1)}, {0}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_rnd},
#endif
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0x21bd, B1_IDENT_FLAGS_SET_FN(2, 1)}, {B1_TYPE_STRING, B1_TYPE_INT}, B1_TYPE_STRING}, b1_fn_bltin_right},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
	{{{0x290a, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_ANY}, B1_TYPE_ANY}, b1_fn_bltin_abs},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0x2c0d, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_STRING}, B1_TYPE_STRING}, b1_fn_bltin_ltrim},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0x4346, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_STRING}, B1_TYPE_STRING}, b1_fn_bltin_ucase},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0x444e, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_STRING}, B1_TYPE_STRING}, b1_fn_bltin_lcase},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0x46b6, B1_IDENT_FLAGS_SET_FN(3, 1)}, {B1_TYPE_STRING, B1_TYPE_INT, B1_TYPE_ANY}, B1_TYPE_STRING}, b1_fn_bltin_mid},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STANDARD
	{{{0x4866, B1_IDENT_FLAGS_SET_FN(3, 1)}, {B1_TYPE_BOOL, B1_TYPE_STRING, B1_TYPE_STRING}, B1_TYPE_STRING}, b1_fn_bltin_striif},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STANDARD
	{{{0x4bfe, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_ANY}, B1_TYPE_STRING}, b1_fn_bltin_str},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0x4c4e, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_STRING}, B1_TYPE_STRING}, b1_fn_bltin_rtrim},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STANDARD
	{{{0x621d, B1_IDENT_FLAGS_SET_FN(3, 1)}, {B1_TYPE_BOOL, B1_TYPE_ANY, B1_TYPE_ANY}, B1_TYPE_ANY}, b1_fn_bltin_iif},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
	{{{0x6370, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_FP_HIGH_PREC}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_cos},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0x6d27, B1_IDENT_FLAGS_SET_FN(2, 1)}, {B1_TYPE_STRING, B1_TYPE_INT}, B1_TYPE_STRING}, b1_fn_bltin_rset},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STANDARD
	{{{0x6d38, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_STRING}, B1_TYPE_INT}, b1_fn_bltin_asc},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
	{{{0x71b5, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_FP_HIGH_PREC}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_atn},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
	{{{0x77af, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_FP_HIGH_PREC}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_int},
#endif
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
	{{{0x834a, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_ANY}, B1_TYPE_ANY}, b1_fn_bltin_sgn},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
	{{{0x8b0a, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_FP_HIGH_PREC}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_sin},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
	{{{0x917c, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_FP_HIGH_PREC}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_exp},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0x9628, B1_IDENT_FLAGS_SET_FN(2, 1)}, {B1_TYPE_STRING, B1_TYPE_INT}, B1_TYPE_STRING}, b1_fn_bltin_lset},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
	{{{0xae97, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_FP_HIGH_PREC}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_sqr},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
	{{{0xd787, B1_IDENT_FLAGS_SET_FN(0, 1)}, {0}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_pi},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STANDARD
	{{{0xdfc8, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_STRING}, B1_TYPE_INT}, b1_fn_bltin_len},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0xeaa3, B1_IDENT_FLAGS_SET_FN(2, 1)}, {B1_TYPE_STRING, B1_TYPE_INT}, B1_TYPE_STRING}, b1_fn_bltin_left},
#endif
#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
	{{{0xf009, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_FP_HIGH_PREC}, B1_TYPE_FP_HIGH_PREC}, b1_fn_bltin_tan},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STANDARD
	{{{0xf7c3, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_STRING}, B1_TYPE_ANY}, b1_fn_bltin_val},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STANDARD
	{{{0xfae3, B1_IDENT_FLAGS_SET_FN(1, 1)}, {B1_TYPE_INT}, B1_TYPE_STRING}, b1_fn_bltin_chr},
#endif
#ifdef B1_FEATURE_FUNCTIONS_STRING
	{{{0xff2d, B1_IDENT_FLAGS_SET_FN(3, 1)}, {B1_TYPE_ANY, B1_TYPE_STRING, B1_TYPE_STRING}, B1_TYPE_INT}, b1_fn_bltin_instr},
#endif
};
#endif


#ifdef B1_FEATURE_FUNCTIONS_STANDARD
static B1_T_ERROR b1_fn_bltin_iif(B1_VAR *parg1)
{
	*parg1 = *(parg1 + ((*parg1).value.bval ? (uint8_t)1 : (uint8_t)2));

	if(B1_TYPE_TEST_STRING((*parg1).type))
	{
		return B1_RES_ETYPMISM;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_striif(B1_VAR *parg1)
{
	uint8_t arg_index;
	B1_T_ERROR err;

	// argument to free
	arg_index = ((*parg1).value.bval ? (uint8_t)2 : (uint8_t)1);
	err = b1_fn_bltin_len(parg1 + arg_index);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// argument to return
	arg_index = (arg_index == 1) ? (uint8_t)2 : (uint8_t)1;

	*parg1 = *(parg1 + arg_index);

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_len(B1_VAR *parg1)
{
	B1_T_ERROR err;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	(*parg1).type = B1_TYPE_SET(B1_TYPE_INT, 0);
	(*parg1).value.i32val = *b1_tmp_buf;

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_asc(B1_VAR *parg1)
{
	B1_T_ERROR err;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(*b1_tmp_buf == 0)
	{
		return B1_RES_EINVARG;
	}

	(*parg1).type = B1_TYPE_SET(B1_TYPE_INT, 0);
	(*parg1).value.i32val = *(b1_tmp_buf + 1);

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_chr(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_CHAR buf[2];

	buf[0] = 1;
	buf[1] = (B1_T_CHAR)(*parg1).value.i32val;

	err = b1_var_str2var(buf, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_str(B1_VAR *parg1)
{
	B1_T_ERROR err;
	uint8_t ppz;

	// b1_var_convert uses b1_int_print_zone_width gobal variable to get max. print field width when formatting foating point values
	// set it to 12 (b1_var_convert reserves two positions for unary minus and value delimiter so max. value length is 10 charcters)
	ppz = b1_int_print_zone_width;
	b1_int_print_zone_width = 12;

	err = b1_var_convert(parg1, B1_TYPE_STRING);

	b1_int_print_zone_width = ppz;

	return err;
}

static B1_T_ERROR b1_fn_bltin_val(B1_VAR *parg1)
{
	B1_T_ERROR err;
	uint8_t type;
	const B1_T_CHAR *s;
	B1_TOKENDATA td;
	B1_T_INDEX len;

	type = B1_TYPE_GET((*parg1).type);
	switch(type)
	{
#ifdef B1_FEATURE_TYPE_SINGLE
		case B1_TYPE_SINGLE:
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
		case B1_TYPE_DOUBLE:
#endif
		case B1_TYPE_INT:
#ifdef B1_FEATURE_TYPE_SMALL
		case B1_TYPE_INT16:
		case B1_TYPE_WORD:
		case B1_TYPE_BYTE:
#endif
			return B1_RES_OK;
		case B1_TYPE_STRING:
			break;
		default:
			return B1_RES_ETYPMISM;
	}

	err = b1_var_var2str(parg1, b1_tmp_buf1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	len = *b1_tmp_buf1;

	if(len == (B1_TMP_BUF_LEN - 1))
	{
		return B1_RES_EINVNUM;
	}

	*(b1_tmp_buf1 + len + 1) = 0;

	s = b1_progline;
	b1_progline = b1_tmp_buf1;
	// copy value to b1_tmp_buf
	err = b1_tok_get(1, B1_TOK_ALLOW_UNARY_OPER | B1_TOK_COPY_VALUE, &td);
	b1_progline = s;
	
	if(err != B1_RES_OK)
	{
		return err;
	}

	len = td.length;

	if(len == 0 || !(td.type & B1_TOKEN_TYPE_NUMERIC))
	{
		return B1_RES_EINVNUM;
	}

	s = b1_progline;
	b1_progline = b1_tmp_buf1;
	err = b1_tok_get(td.offset + len, 0, &td);
	b1_progline = s;

	if(err != B1_RES_OK)
	{
		return err;
	}

	if(td.length != 0)
	{
		return B1_RES_EINVNUM;
	}

	// the function converts value copied to b1_tmp_buf variable with b1_tok_get call
	return b1_eval_get_numeric_value(parg1);
}
#endif

#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
static B1_T_ERROR b1_fn_bltin_abs(B1_VAR *parg1)
{
	return b1_eval_neg(parg1, (*parg1).type, 1);
}

#ifdef B1_FRACTIONAL_TYPE_EXISTS
static B1_T_ERROR b1_fn_bltin_int(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).value.dval = floor((*parg1).value.dval);
#else
	(*parg1).value.sval = floorf((*parg1).value.sval);
#endif
	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_rnd(B1_VAR *parg1)
{
	B1_T_RAND_SEED seed;

	seed = b1_ex_rnd_get_next_seed();
	if(seed == B1_T_RAND_SEED_MAX_VALUE)
	{
		seed--;
	}

#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
	(*parg1).value.dval = ((double)seed) / ((double)B1_T_RAND_SEED_MAX_VALUE);
#else
	(*parg1).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
	(*parg1).value.sval = ((float)seed) / ((float)B1_T_RAND_SEED_MAX_VALUE);
#endif

	return B1_RES_OK;
}
#endif

static B1_T_ERROR b1_fn_bltin_sgn(B1_VAR *parg1)
{
	uint8_t type;

	type = B1_TYPE_GET((*parg1).type);
	switch(type)
	{
#ifdef B1_FEATURE_TYPE_SINGLE
		case B1_TYPE_SINGLE:
			(*parg1).value.sval = ((*parg1).value.sval < 0.0f) ? -1.0f : (((*parg1).value.sval == 0.0f) ? 0.0f : 1.0f);
			break;
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
		case B1_TYPE_DOUBLE:
			(*parg1).value.dval = ((*parg1).value.dval < 0.0) ? -1.0 : (((*parg1).value.dval == 0.0) ? 0.0 : 1.0);
			break;
#endif
		case B1_TYPE_INT:
			(*parg1).value.i32val = (int32_t)(((*parg1).value.i32val < 0) ? (int8_t)-1 : (((*parg1).value.i32val == 0) ? (int8_t)0 : (int8_t)1));
			break;
#ifdef B1_FEATURE_TYPE_SMALL
		case B1_TYPE_INT16:
			(*parg1).value.i16val = (int16_t)(((*parg1).value.i16val < 0) ? (int8_t)-1 : (((*parg1).value.i16val == 0) ? (int8_t)0 : (int8_t)1));
			break;
		case B1_TYPE_WORD:
			(*parg1).value.ui16val = (uint16_t)(((*parg1).value.ui16val == 0) ? (uint8_t)0 : (uint8_t)1);
			break;
		case B1_TYPE_BYTE:
			(*parg1).value.ui8val = (uint8_t)(((*parg1).value.ui8val == 0) ? (uint8_t)0 : (uint8_t)1);
			break;
#endif
		default:
			return B1_RES_ETYPMISM;
	}

	return B1_RES_OK;
}
#endif

#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
static B1_T_ERROR b1_fn_bltin_atn(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).value.dval = atan((*parg1).value.dval);
#else
	(*parg1).value.sval = atanf((*parg1).value.sval);
#endif
	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_cos(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).value.dval = cos((*parg1).value.dval);
#else
	(*parg1).value.sval = cosf((*parg1).value.sval);
#endif
	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_exp(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).value.dval = exp((*parg1).value.dval);
#else
	(*parg1).value.sval = expf((*parg1).value.sval);
#endif
	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_log(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).value.dval = log((*parg1).value.dval);
#else
	(*parg1).value.sval = logf((*parg1).value.sval);
#endif
	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_pi(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
	(*parg1).value.dval = 3.14159265358979323846;
#else
	(*parg1).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
	(*parg1).value.sval = (float)3.14159265358979323846;
#endif
	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_sin(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).value.dval = sin((*parg1).value.dval);
#else
	(*parg1).value.sval = sinf((*parg1).value.sval);
#endif
	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_sqr(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).value.dval = sqrt((*parg1).value.dval);
#else
	(*parg1).value.sval = sqrtf((*parg1).value.sval);
#endif
	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_tan(B1_VAR *parg1)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	(*parg1).value.dval = tan((*parg1).value.dval);
#else
	(*parg1).value.sval = tanf((*parg1).value.sval);
#endif
	return B1_RES_OK;
}
#endif

#ifdef B1_FEATURE_FUNCTIONS_STRING
// MID$(<input_string>, <start_position>, [<length>])
static B1_T_ERROR b1_fn_bltin_mid(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX start, len;

	start = 0;

	if(B1_TYPE_GET((*(parg1 + 2)).type) != B1_TYPE_NULL)
	{
		err = b1_var_convert(parg1 + 2, B1_TYPE_INT);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if((*(parg1 + 2)).value.i32val < 0)
		{
			return B1_RES_EINVARG;
		}

		len = (B1_T_INDEX)(*(parg1 + 2)).value.i32val;

		// start here stands for <length> parameter presence
		start++;
	}

	// call b1_var_var2str after b1_var_convert function call because b1_var_convert can use b1_tmp_buf variable too
	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// set length to the entire string length id the last parameter is not present
	if(!start)
	{
		len = *b1_tmp_buf;
	}

	if((*(parg1 + 1)).value.i32val <= 0)
	{
		return B1_RES_EINVARG;
	}
	start = (B1_T_INDEX)(*(parg1 + 1)).value.i32val;

	if(start > *b1_tmp_buf)
	{
		len = 0;
	}

	if(len == 0)
	{
		start = 1;
	}
	else
	{
		if(*b1_tmp_buf - start  + 1 < len)
		{
			len = *b1_tmp_buf - start + 1;
		}
	}

	*(b1_tmp_buf + start - 1) = len;

	return b1_var_str2var(b1_tmp_buf + start - 1, parg1);
}

// INSTR([<start_position>], <string>, <string_to_search>)
static B1_T_ERROR b1_fn_bltin_instr(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX start, len;

	start = 1;

	if(B1_TYPE_GET((*parg1).type) != B1_TYPE_NULL)
	{
		err = b1_var_convert(parg1, B1_TYPE_INT);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if((*parg1).value.i32val <= 0)
		{
			return B1_RES_EINVARG;
		}

		start = (B1_T_INDEX)(*parg1).value.i32val;
	}

	err = b1_var_var2str(parg1 + 1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	err = b1_var_var2str(parg1 + 2, b1_tmp_buf1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	(*parg1).type = B1_TYPE_SET(B1_TYPE_INT, 0);
	(*parg1).value.i32val = 0;

	if(*b1_tmp_buf == 0)
	{
		return B1_RES_OK;
	}

	if(*b1_tmp_buf1 == 0)
	{
		(*parg1).value.i32val = start;
		return B1_RES_OK;
	}

	if(start > *b1_tmp_buf)
	{
		return B1_RES_EINVARG;
	}

	len = *b1_tmp_buf - start + 1;
	if(len < *b1_tmp_buf1)
	{
		return B1_RES_OK;
	}

	len -= *b1_tmp_buf1;
	len++;
	while(len-- != 0)
	{
#ifdef B1_FEATURE_LOCALES
		if(b1_t_strcmp_l(b1_tmp_buf1, b1_tmp_buf + start, *b1_tmp_buf1) == 0)
#else
		if(b1_t_strcmpi(b1_tmp_buf1, b1_tmp_buf + start, *b1_tmp_buf1) == 0)
#endif
		{
			(*parg1).value.i32val = start;
			return B1_RES_OK;
		}
		start++;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_ltrim(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX start;
	B1_T_CHAR c;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	start = 1;

	if(*b1_tmp_buf != 0)
	{
		for(; start <= *b1_tmp_buf; start++)
		{
			c = *(b1_tmp_buf + start);
			if(!B1_T_ISBLANK(c))
			{
				break;
			}
		}

		*(b1_tmp_buf + start - 1) = *b1_tmp_buf - start + 1;
	}

	err = b1_var_str2var(b1_tmp_buf + start - 1, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_rtrim(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX len;
	B1_T_CHAR c;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	len = *b1_tmp_buf;
	for(; len != 0; len--)
	{
		c = *(b1_tmp_buf + len);
		if(!B1_T_ISBLANK(c))
		{
			break;
		}
	}

	*b1_tmp_buf = len;

	err = b1_var_str2var(b1_tmp_buf, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_left(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX len;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if((*(parg1 + 1)).value.i32val < 0)
	{
		return B1_RES_EINVARG;
	}

	len = (B1_T_INDEX)(*(parg1 + 1)).value.i32val;

	if(len < *b1_tmp_buf)
	{
		*b1_tmp_buf = len;
	}

	err = b1_var_str2var(b1_tmp_buf, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_right(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX len, start;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if((*(parg1 + 1)).value.i32val < 0)
	{
		return B1_RES_EINVARG;
	}

	start = 0;
	len = (B1_T_INDEX)(*(parg1 + 1)).value.i32val;

	if(len < *b1_tmp_buf)
	{
		start = *b1_tmp_buf - len;
		*(b1_tmp_buf + start) = len;
	}

	err = b1_var_str2var(b1_tmp_buf + start, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_lset(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX len, i;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if((*(parg1 + 1)).value.i32val < 0)
	{
		return B1_RES_EINVARG;
	}

	len = (B1_T_INDEX)(*(parg1 + 1)).value.i32val;

	for(i = *b1_tmp_buf; i < len; )
	{
		i++;
		*(b1_tmp_buf + i) = B1_T_C_SPACE;
	}

	*b1_tmp_buf = len;

	err = b1_var_str2var(b1_tmp_buf, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_rset(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX len, old_len, spnum;
	B1_T_CHAR c;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if((*(parg1 + 1)).value.i32val < 0)
	{
		return B1_RES_EINVARG;
	}

	len = (B1_T_INDEX)(*(parg1 + 1)).value.i32val;
	old_len = *b1_tmp_buf;

	if(old_len < len)
	{
		spnum = len - old_len;

		while(old_len != 0)
		{
			c = *(b1_tmp_buf + old_len);
			*(b1_tmp_buf + old_len + spnum) = c;
			old_len--;
		}

		while(old_len < spnum)
		{
			old_len++;
			*(b1_tmp_buf + old_len) = B1_T_C_SPACE;
		}
	}

	*b1_tmp_buf = len;

	err = b1_var_str2var(b1_tmp_buf, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_ucase(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX len;
	B1_T_CHAR c;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	for(len = *b1_tmp_buf; len != 0; len--)
	{
		c = *(b1_tmp_buf + len);
#ifdef B1_FEATURE_LOCALES
		*(b1_tmp_buf + len) = b1_t_toupper_l(c);
#else
		*(b1_tmp_buf + len) = B1_T_TOUPPER(c);
#endif
	}

	err = b1_var_str2var(b1_tmp_buf, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_fn_bltin_lcase(B1_VAR *parg1)
{
	B1_T_ERROR err;
	B1_T_INDEX len;
	B1_T_CHAR c;

	err = b1_var_var2str(parg1, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	for(len = *b1_tmp_buf; len != 0; len--)
	{
		c = *(b1_tmp_buf + len);
#ifdef B1_FEATURE_LOCALES
		*(b1_tmp_buf + len) = b1_t_tolower_l(c);
#else
		*(b1_tmp_buf + len) = B1_T_TOLOWER(c);
#endif
	}

	err = b1_var_str2var(b1_tmp_buf, parg1);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}
#endif

#ifdef B1_FEATURE_FUNCTIONS_USER
B1_T_ERROR b1_fn_get_params(B1_T_IDHASH name_hash, uint8_t alloc_new, B1_FN **fn_ptr)
#else
B1_T_ERROR b1_fn_get_params(B1_T_IDHASH name_hash, B1_FN **fn_ptr)
#endif
{
#if defined(B1_FEATURE_FUNCTIONS_STANDARD) || defined(B1_FEATURE_FUNCTIONS_MATH_BASIC) || defined(B1_FEATURE_FUNCTIONS_MATH_EXTRA) || defined(B1_FEATURE_FUNCTIONS_STRING)
	*fn_ptr = (B1_FN *)bsearch(&name_hash, b1_fn_bltin, B1_FN_BLTIN_COUNT, sizeof(B1_BLTIN_FN), b1_id_cmp_hashes);
	if(*fn_ptr != NULL)
	{
		return B1_RES_OK;
	}
#endif

#ifdef B1_FEATURE_FUNCTIONS_USER
	return b1_ex_ufn_get(name_hash, alloc_new, (B1_UDEF_FN **)fn_ptr);
#else
	return B1_RES_EUNKIDENT;
#endif
}
