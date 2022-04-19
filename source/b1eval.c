/*
 BASIC1 interpreter
 Copyright (c) 2021-2022 Nikolay Pletnev
 MIT license

 b1eval.c: expressions evaluation
*/


#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "b1ex.h"
#include "b1tok.h"
#include "b1rpn.h"
#include "b1fn.h"
#include "b1eval.h"
#include "b1int.h"
#include "b1err.h"


// converts C string to numeric value, can return value of any numeric type (integer, fractional)
// input string should be placed in b1_tmp_buf global variable, the function uses the variable too so do not reuse the string after
// the function call
// accepts numeric data type specificators (e.g.: 10%, 10!, 10.1!, etc.)
// doesn't check input string for errors (so returns unspecified values if input string doesn't represent a correct numeric value)
B1_T_ERROR b1_eval_get_numeric_value(B1_VAR *var)
{
	B1_T_INDEX len;
	B1_T_CHAR c;
	uint8_t type;
#ifdef B1_FEATURE_HEX_NUM
	B1_T_INDEX i;
	int32_t hv;
	uint8_t neg;
#endif

	len = *b1_tmp_buf;
	if(len == 0 || len == (B1_TMP_BUF_LEN - 1))
	{
		return B1_RES_EINVNUM;
	}

#ifdef B1_FEATURE_HEX_NUM
	i = 2;
	neg = 0;
	c = *(b1_tmp_buf + 1);
	if(c == B1_T_C_MINUS || c == B1_T_C_PLUS)
	{
		if(c == B1_T_C_MINUS)
		{
			neg++;
		}
		i++;
	}
#endif

	// get type specifier
	c = *(b1_tmp_buf + len);

#ifdef B1_FEATURE_HEX_NUM
	if(len >= i && *(b1_tmp_buf + i - 1) == B1_T_C_0 && (*(b1_tmp_buf + i) == (B1_T_CHAR)'X' || *(b1_tmp_buf + i) == (B1_T_CHAR)'x'))
	{
		type = B1_TYPE_INT;
		if(c == B1_T_C_PERCENT)
		{
			len--;
		}

		if(len <= i || len > 10)
		{
			return B1_RES_EINVNUM;
		}

		hv = 0;

		for(; ++i <= len;)
		{
			hv <<= (uint8_t)4;

			c = *(b1_tmp_buf + i);

			if(B1_T_ISDIGIT(c))
			{
				hv |= (uint8_t)(c - B1_T_C_0);
			}
			else
			if(c >= (B1_T_CHAR)'A' && c <= (B1_T_CHAR)'F')
			{
				hv |= (uint8_t)(c - ((B1_T_CHAR)'A' - (B1_T_CHAR)10));
			}
			else
			if(c >= (B1_T_CHAR)'a' && c <= (B1_T_CHAR)'f')
			{
				hv |= (uint8_t)(c - ((B1_T_CHAR)'a' - (B1_T_CHAR)10));
			}
			else
			{
				return B1_RES_EINVNUM;
			}
		}

		if(neg)
		{
			hv = -hv;
		}

		(*var).type = B1_TYPE_SET(B1_TYPE_INT, 0);
		(*var).value.i32val = hv;

		return B1_RES_OK;
	}
#endif

	// get type specificator
#ifdef B1_FEATURE_TYPE_SINGLE
	if(c == B1_T_C_EXCLAMATION)
	{
		type = B1_TYPE_SINGLE;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(c == B1_T_C_NUMBER)
	{
		type = B1_TYPE_DOUBLE;
	}
	else
#endif
	if(c == B1_T_C_PERCENT)
	{
		type = B1_TYPE_INT;
	}
	else
	{
		len++;
		// default numeric type for values without type specificator
#ifdef B1_FEATURE_TYPE_SINGLE
		type = B1_TYPE_SINGLE;
#elif defined(B1_FEATURE_TYPE_DOUBLE)
		type = B1_TYPE_DOUBLE;
#else
		type = B1_TYPE_INT;
#endif
	}

	*(b1_tmp_buf + len) = B1_T_C_STRTERM;

	if(type == B1_TYPE_INT)
	{
		(*var).type = B1_TYPE_SET(B1_TYPE_INT, 0);
		return b1_t_strtoi32(b1_tmp_buf + 1, &(*var).value.i32val);
	}
#ifdef B1_FEATURE_TYPE_SINGLE
	if(type == B1_TYPE_SINGLE)
	{
		(*var).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
		return b1_t_strtosingle(b1_tmp_buf + 1, &(*var).value.sval);
	}
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(type == B1_TYPE_DOUBLE)
	{
		(*var).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
		return b1_t_strtodouble(b1_tmp_buf + 1, &(*var).value.dval);
	}
#endif

	return B1_RES_ETYPMISM;
}

// converts numeric or string represented with B1_TYPE_RPNREC_PTR record to immediate value (B1_TYPE_STRING, B1_TYPE_SINGLE, etc.)
static B1_T_ERROR b1_eval_get_value(B1_T_INDEX value_index)
{
	B1_T_ERROR err;
	B1_VAR *var;
	B1_TOKENDATA td;

	err = B1_RES_OK;

	var = b1_rpn_eval + value_index;

	if(B1_TYPE_TEST_RPNREC_PTR((*var).type))
	{
		err = b1_tok_get((*(*var).value.token).offset, B1_TOK_COPY_VALUE, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(td.type & B1_TOKEN_TYPE_QUOTEDSTR)
		{
			err = b1_var_str2var(b1_tmp_buf, var);
		}
		else
		if(td.type & B1_TOKEN_TYPE_NUMERIC)
		{
			err = b1_eval_get_numeric_value(var);
		}
		else
		{
			return B1_RES_ESYNTAX;
		}
	}

	return err;
}

B1_T_ERROR b1_eval_neg(B1_VAR *pvar, uint8_t optype, uint8_t abs)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		if(!abs || (*pvar).value.sval < 0.0f)
		{
			(*pvar).value.sval = -((*pvar).value.sval);
		}
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		if(!abs || (*pvar).value.dval < 0.0)
		{
			(*pvar).value.dval = -((*pvar).value.dval);
		}
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		if(!abs || (*pvar).value.i32val < 0)
		{
			(*pvar).value.i32val = -((*pvar).value.i32val);
		}
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		if(!abs || (*pvar).value.i16val < 0)
		{
			(*pvar).value.i16val = -((*pvar).value.i16val);
		}
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		if(!abs)
		{
			(*pvar).value.ui16val = -((*pvar).value.ui16val);
		}
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		if(!abs)
		{
			(*pvar).value.ui8val = -((*pvar).value.ui8val);
		}
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_not(B1_VAR *pvar, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar).value.i32val = ~((*pvar).value.i32val);
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar).value.i16val = ~((*pvar).value.i16val);
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar).value.ui16val = ~((*pvar).value.ui16val);
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar).value.ui8val = ~((*pvar).value.ui8val);
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

B1_T_ERROR b1_eval_add(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		(*pvar1).value.sval += (*(pvar1 + 1)).value.sval;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		(*pvar1).value.dval += (*(pvar1 + 1)).value.dval;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar1).value.i32val += (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar1).value.i16val += (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar1).value.ui16val += (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar1).value.ui8val += (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_strcat(B1_VAR *pvar1, uint8_t optype)
{
	B1_T_ERROR err;
	B1_T_INDEX len;

	err = B1_RES_OK;

	if(optype == B1_TYPE_STRING)
	{
		// string concatenation
		// get the first string
		err = b1_var_var2str(pvar1, b1_tmp_buf);
		if(err == B1_RES_OK)
		{
			// get the second string
			err = b1_var_var2str(pvar1 + 1, b1_tmp_buf1);
			if(err == B1_RES_OK)
			{
				// calculate new string length
				len = *b1_tmp_buf1 + *b1_tmp_buf;

				if(len > B1_MAX_STRING_LEN)
				{
					err = B1_ESTRLONG;
				}
				else
				{
					// concatenate strings
					memcpy(b1_tmp_buf + *b1_tmp_buf + 1, b1_tmp_buf1 + 1, (*b1_tmp_buf1) * B1_T_CHAR_SIZE);
					*b1_tmp_buf = len;

					// create variable from the resulting string
					err = b1_var_str2var(b1_tmp_buf, pvar1);
				}
			}
		}
	}
	else
	{
		err = B1_RES_EWARGTYPE;
	}

	return err;
}

B1_T_ERROR b1_eval_sub(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		(*pvar1).value.sval -= (*(pvar1 + 1)).value.sval;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		(*pvar1).value.dval -= (*(pvar1 + 1)).value.dval;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar1).value.i32val -= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar1).value.i16val -= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar1).value.ui16val -= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar1).value.ui8val -= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_mul(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		(*pvar1).value.sval *= (*(pvar1 + 1)).value.sval;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		(*pvar1).value.dval *= (*(pvar1 + 1)).value.dval;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar1).value.i32val *= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar1).value.i16val *= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar1).value.ui16val *= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar1).value.ui8val *= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_div(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		(*pvar1).value.sval /= (*(pvar1 + 1)).value.sval;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		(*pvar1).value.dval /= (*(pvar1 + 1)).value.dval;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		if((*(pvar1 + 1)).value.i32val == 0 || ((*(pvar1 + 1)).value.i32val == -1 && (*pvar1).value.i32val == INT32_MIN))
		{
			return B1_RES_EIDIVZERO;
		}

		(*pvar1).value.i32val /= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		if((*(pvar1 + 1)).value.i16val == 0 || ((*(pvar1 + 1)).value.i16val == ((int16_t)-1) && (*pvar1).value.i16val == INT16_MIN))
		{
			return B1_RES_EIDIVZERO;
		}

		(*pvar1).value.i16val /= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		if((*(pvar1 + 1)).value.ui16val == 0)
		{
			return B1_RES_EIDIVZERO;
		}

		(*pvar1).value.ui16val /= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		if((*(pvar1 + 1)).value.ui8val == 0)
		{
			return B1_RES_EIDIVZERO;
		}

		(*pvar1).value.ui8val /= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_mod(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		(*pvar1).value.sval = fmodf((*pvar1).value.sval, (*(pvar1 + 1)).value.sval);
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		(*pvar1).value.dval = fmod((*pvar1).value.dval, (*(pvar1 + 1)).value.dval);
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		if((*(pvar1 + 1)).value.i32val == 0 || ((*(pvar1 + 1)).value.i32val == -1 && (*pvar1).value.i32val == INT32_MIN))
		{
			return B1_RES_EIDIVZERO;
		}

		(*pvar1).value.i32val %= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		if((*(pvar1 + 1)).value.i16val == 0 || ((*(pvar1 + 1)).value.i16val == ((int16_t)-1) && (*pvar1).value.i16val == INT16_MIN))
		{
			return B1_RES_EIDIVZERO;
		}

		(*pvar1).value.i16val %= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		if((*(pvar1 + 1)).value.ui16val == 0)
		{
			return B1_RES_EIDIVZERO;
		}

		(*pvar1).value.ui16val %= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		if((*(pvar1 + 1)).value.ui8val == 0)
		{
			return B1_RES_EIDIVZERO;
		}
		
		(*pvar1).value.ui8val %= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_and(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar1).value.i32val &= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar1).value.i16val &= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar1).value.ui16val &= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar1).value.ui8val &= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_or(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar1).value.i32val |= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar1).value.i16val |= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar1).value.ui16val |= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar1).value.ui8val |= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_xor(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar1).value.i32val ^= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar1).value.i16val ^= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar1).value.ui16val ^= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar1).value.ui8val ^= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_shr(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar1).value.i32val >>= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar1).value.i16val >>= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar1).value.ui16val >>= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar1).value.ui8val >>= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_shl(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		return B1_RES_ETYPMISM;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		(*pvar1).value.i32val <<= (*(pvar1 + 1)).value.i32val;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		(*pvar1).value.i16val <<= (*(pvar1 + 1)).value.i16val;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		(*pvar1).value.ui16val <<= (*(pvar1 + 1)).value.ui16val;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		(*pvar1).value.ui8val <<= (*(pvar1 + 1)).value.ui8val;
	}
	else
#endif
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

#ifndef B1_FRACTIONAL_TYPE_EXISTS
static int32_t b1_eval_pow_int32(int32_t base, int32_t exp)
{
	int32_t power;

	if(base == 0)
	{
		return 0;
	}

	if(exp < 0)
	{
		return 0;
	}

	power = 1;
	while(exp != 0)
	{
		if(exp & 1)
		{
			power *= base;
		}

		exp >>= 1;
		base *= base;
	}

	return power;
}
#endif

static B1_T_ERROR b1_eval_pow(B1_VAR *pvar1, uint8_t optype)
{
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(B1_TYPE_TEST_NUMERIC(optype))
	{
#ifdef B1_FEATURE_TYPE_SINGLE
		if(optype == B1_TYPE_SINGLE)
		{
			(*pvar1).value.dval = (double)(*pvar1).value.sval;
			(*(pvar1 + 1)).value.dval = (double)(*(pvar1 + 1)).value.sval;
		}
		else
#endif
		if(optype == B1_TYPE_INT)
		{
			(*pvar1).value.dval = (double)(*pvar1).value.i32val;
			(*(pvar1 + 1)).value.dval = (double)(*(pvar1 + 1)).value.i32val;
		}
#ifdef B1_FEATURE_TYPE_SMALL
		else
		if(optype == B1_TYPE_INT16)
		{
			(*pvar1).value.dval = (double)(*pvar1).value.i16val;
			(*(pvar1 + 1)).value.dval = (double)(*(pvar1 + 1)).value.i16val;
		}
		else
		if(optype == B1_TYPE_WORD)
		{
			(*pvar1).value.dval = (double)(*pvar1).value.ui16val;
			(*(pvar1 + 1)).value.dval = (double)(*(pvar1 + 1)).value.ui16val;
		}
		else
		if(optype == B1_TYPE_BYTE)
		{
			(*pvar1).value.dval = (double)(*pvar1).value.ui8val;
			(*(pvar1 + 1)).value.dval = (double)(*(pvar1 + 1)).value.ui8val;
		}
#endif
		(*pvar1).value.dval = pow((*pvar1).value.dval, (*(pvar1 + 1)).value.dval);

#ifdef B1_FEATURE_TYPE_SINGLE
		if(optype == B1_TYPE_SINGLE)
		{
			(*pvar1).value.sval = (float)(*pvar1).value.dval;
		}
		else
#endif
		if(optype == B1_TYPE_INT)
		{
			(*pvar1).value.i32val = (int32_t)(*pvar1).value.dval;
		}
#ifdef B1_FEATURE_TYPE_SMALL
		else
		if(optype == B1_TYPE_INT16)
		{
			(*pvar1).value.i16val = (int16_t)(*pvar1).value.dval;
		}
		else
		if(optype == B1_TYPE_WORD)
		{
			(*pvar1).value.ui16val = (uint16_t)(*pvar1).value.dval;
		}
		else
		if(optype == B1_TYPE_BYTE)
		{
			(*pvar1).value.ui8val = (uint8_t)(*pvar1).value.dval;
		}
#endif
	}
#elif defined(B1_FEATURE_TYPE_SINGLE)
	if(B1_TYPE_TEST_NUMERIC(optype))
	{
		if(optype == B1_TYPE_INT)
		{
			(*pvar1).value.sval = (float)(*pvar1).value.i32val;
			(*(pvar1 + 1)).value.sval = (float)(*(pvar1 + 1)).value.i32val;
		}
#ifdef B1_FEATURE_TYPE_SMALL
		else
		if(optype == B1_TYPE_INT16)
		{
			(*pvar1).value.sval = (float)(*pvar1).value.i16val;
			(*(pvar1 + 1)).value.sval = (float)(*(pvar1 + 1)).value.i16val;
		}
		else
		if(optype == B1_TYPE_WORD)
		{
			(*pvar1).value.sval = (float)(*pvar1).value.ui16val;
			(*(pvar1 + 1)).value.sval = (float)(*(pvar1 + 1)).value.ui16val;
		}
		else
		if(optype == B1_TYPE_BYTE)
		{
			(*pvar1).value.sval = (float)(*pvar1).value.ui8val;
			(*(pvar1 + 1)).value.sval = (float)(*(pvar1 + 1)).value.ui8val;
		}
#endif
		(*pvar1).value.sval = powf((*pvar1).value.sval, (*(pvar1 + 1)).value.sval);
		
		if(optype == B1_TYPE_INT)
		{
			(*pvar1).value.i32val = (int32_t)(*pvar1).value.sval;
		}
#ifdef B1_FEATURE_TYPE_SMALL
		else
		if(optype == B1_TYPE_INT16)
		{
			(*pvar1).value.i16val = (int16_t)(*pvar1).value.sval;
		}
		else
		if(optype == B1_TYPE_WORD)
		{
			(*pvar1).value.ui16val = (uint16_t)(*pvar1).value.sval;
		}
		else
		if(optype == B1_TYPE_BYTE)
		{
			(*pvar1).value.ui8val = (uint8_t)(*pvar1).value.sval;
		}
#endif
	}
#else
	if(B1_TYPE_TEST_NUMERIC(optype))
	{
#ifdef B1_FEATURE_TYPE_SMALL
		if(optype == B1_TYPE_INT16)
		{
			(*pvar1).value.i32val = (int32_t)(*pvar1).value.i16val;
			(*(pvar1 + 1)).value.i32val = (int32_t)(*(pvar1 + 1)).value.i16val;
		}
		else
		if(optype == B1_TYPE_WORD)
		{
			(*pvar1).value.i32val = (int32_t)(*pvar1).value.ui16val;
			(*(pvar1 + 1)).value.i32val = (int32_t)(*(pvar1 + 1)).value.ui16val;
		}
		else
		if(optype == B1_TYPE_BYTE)
		{
			(*pvar1).value.i32val = (int32_t)(*pvar1).value.ui8val;
			(*(pvar1 + 1)).value.i32val = (int32_t)(*(pvar1 + 1)).value.ui8val;
		}
#endif
		(*pvar1).value.i32val = b1_eval_pow_int32((*pvar1).value.i32val, (*(pvar1 + 1)).value.i32val);
#ifdef B1_FEATURE_TYPE_SMALL
		if(optype == B1_TYPE_INT16)
		{
			(*pvar1).value.i16val = (int16_t)(*pvar1).value.i32val;
		}
		else
		if(optype == B1_TYPE_WORD)
		{
			(*pvar1).value.ui16val = (uint16_t)(*pvar1).value.i32val;
		}
		else
		if(optype == B1_TYPE_BYTE)
		{
			(*pvar1).value.ui8val = (uint8_t)(*pvar1).value.i32val;
		}
#endif
	}
#endif
	else
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

B1_T_ERROR b1_eval_cmp(B1_VAR *pvar1, B1_T_CHAR c, B1_T_CHAR c1, uint8_t optype)
{
	B1_T_ERROR err;
	uint8_t cmp, cmp_res;
	int8_t scmp;

	scmp = 1;

	// 1 - eq needed
	// 2 - lt needed
	// 4 - eq set
	// 8 - lt set

	switch(c)
	{
		// '=' comparison operation
		case B1_T_C_EQ:
			cmp = (1 | 4);
			break;
		// '<', '<=' and '<>' operations
		case B1_T_C_LT:
			cmp =	B1_T_ISCSTRTERM(c1)	?	(uint8_t)(2 | 8) :
					(c1 == B1_T_C_EQ)	?	(uint8_t)(1 | 2 | 4 | 8) :
											(uint8_t)1;
			break;
		// '>' and '>=' operations
		default:
			cmp = B1_T_ISCSTRTERM(c1) ? (uint8_t)(1 | 2) : (uint8_t)2;
	}

#ifdef B1_FEATURE_TYPE_SINGLE
	if(optype == B1_TYPE_SINGLE)
	{
		// perform eq operation if needed
		if((cmp & 1) && ((*pvar1).value.sval == (*(pvar1 + 1)).value.sval)) scmp = 0;
		// perform lt operation if needed
		if((cmp & 2) && ((*pvar1).value.sval < (*(pvar1 + 1)).value.sval)) scmp = -1;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(optype == B1_TYPE_DOUBLE)
	{
		// perform eq operation if needed
		if((cmp & 1) && ((*pvar1).value.dval == (*(pvar1 + 1)).value.dval)) scmp = 0;
		// perform lt operation if needed
		if((cmp & 2) && ((*pvar1).value.dval < (*(pvar1 + 1)).value.dval)) scmp = -1;
	}
	else
#endif
	if(optype == B1_TYPE_INT)
	{
		if((cmp & 1) && ((*pvar1).value.i32val == (*(pvar1 + 1)).value.i32val)) scmp = 0;
		if((cmp & 2) && ((*pvar1).value.i32val < (*(pvar1 + 1)).value.i32val)) scmp = -1;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(optype == B1_TYPE_INT16)
	{
		if((cmp & 1) && ((*pvar1).value.i16val == (*(pvar1 + 1)).value.i16val)) scmp = 0;
		if((cmp & 2) && ((*pvar1).value.i16val < (*(pvar1 + 1)).value.i16val)) scmp = -1;
	}
	else
	if(optype == B1_TYPE_WORD)
	{
		if((cmp & 1) && ((*pvar1).value.ui16val == (*(pvar1 + 1)).value.ui16val)) scmp = 0;
		if((cmp & 2) && ((*pvar1).value.ui16val < (*(pvar1 + 1)).value.ui16val)) scmp = -1;
	}
	else
	if(optype == B1_TYPE_BYTE)
	{
		if((cmp & 1) && ((*pvar1).value.ui8val == (*(pvar1 + 1)).value.ui8val)) scmp = 0;
		if((cmp & 2) && ((*pvar1).value.ui8val < (*(pvar1 + 1)).value.ui8val)) scmp = -1;
	}
	else
#endif
	if(optype == B1_TYPE_STRING)
	{
		err = b1_var_var2str(pvar1, b1_tmp_buf);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// get the second string
		err = b1_var_var2str(pvar1 + 1, b1_tmp_buf1);
		if(err != B1_RES_OK)
		{
			return err;
		}

#ifdef B1_FEATURE_LOCALES
		scmp = b1_t_strcmp_l(b1_tmp_buf, b1_tmp_buf1 + 1, *b1_tmp_buf1);
#else
		scmp = b1_t_strcmpi(b1_tmp_buf, b1_tmp_buf1 + 1, *b1_tmp_buf1);
#endif
	}
	else
	{
		return B1_RES_EWARGTYPE;
	}

	cmp_res = 0;

	if(cmp & 1)
	{
		if(scmp == 0) cmp_res |= 1;
	}
	cmp >>= 1;

	if(cmp & 1)
	{
		if(scmp < 0) cmp_res |= 2;
	}
	cmp >>= 1;

	cmp_res = (cmp_res & cmp) || (cmp_res == 0 && cmp == 0);
	(*pvar1).type = B1_TYPE_SET(B1_TYPE_BOOL, 0);
	(*pvar1).value.bval = cmp_res;

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_un_op(B1_T_CHAR c, B1_VAR *arg, uint8_t optype)
{
	if(B1_TYPE_TEST_NUMERIC(optype))
	{
		if(B1_T_ISMINUS(c))
		{
			return b1_eval_neg(arg, optype, 0);
		}

		if(c == B1_T_C_EXCLAMATION)
		{
			return b1_eval_not(arg, optype);
		}
	}
	else
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_eval_bin_op(B1_T_CHAR c, B1_T_CHAR c1, B1_VAR *args, uint8_t optype)
{
	B1_T_ERROR err;

	if(B1_T_ISPLUS(c))
	{
		err = optype == B1_TYPE_STRING ?
			b1_eval_strcat(args, optype) :
			b1_eval_add(args, optype);
	}
	else
	if(B1_T_ISMINUS(c))
	{
		err = b1_eval_sub(args, optype);
	}
	else
	if(c == B1_T_C_ASTERISK)
	{
		err = b1_eval_mul(args, optype);
	}
	else
	if(c == B1_T_C_SLASH)
	{
		err = b1_eval_div(args, optype);
	}
	else
	if(c == B1_T_C_CARET)
	{
		err = b1_eval_pow(args, optype);
	}
	else
	// MOD operator
	if(c == B1_T_C_PERCENT)
	{
		err = b1_eval_mod(args, optype);
	}
	else
	// AND operator
	if(c == B1_T_C_AMPERSAND)
	{
		err = b1_eval_and(args, optype);
	}
	else
	// OR operator
	if(c == B1_T_C_PIPE)
	{
		err = b1_eval_or(args, optype);
	}
	else
	// XOR operator
	if(c == B1_T_C_TILDE)
	{
		err = b1_eval_xor(args, optype);
	}
	else
	// << operator
	if(c == B1_T_C_LT && c1 == B1_T_C_LT)
	{
		err = b1_eval_shl(args, optype);
	}
	else
	// >> operator
	if(c == B1_T_C_GT && c1 == B1_T_C_GT)
	{
		err = b1_eval_shr(args, optype);
	}
	else
	{
		err = b1_eval_cmp(args, c, c1, optype);
	}

	return err;
}

static uint8_t b1_eval_get_common_type(uint8_t type1, uint8_t type2)
{
	B1_T_INDEX i;
	uint8_t type;

	// start finding common type from the highest priority type
	for(i = 0; i < B1_TYPE_COUNT; i++)
	{
		type = b1_t_types[i];
		if(type == type1 || type == type2)
		{
			return type;
		}
	}

	return 0;
}

B1_T_ERROR b1_eval(uint8_t options, B1_VAR_REF *var_ref)
{
	B1_T_ERROR err;
	uint8_t tflags, type, argnum, arg, var_type;
	B1_T_INDEX i, tmptop;
	const B1_TOKENDATA *ptoken;
	B1_T_CHAR c, c1;
	B1_T_IDHASH name_hash;
	B1_NAMED_VAR *var;
	B1_VAR *var1;
	B1_FN *fn;
#ifdef B1_FEATURE_FUNCTIONS_USER
	B1_T_INDEX rpn_stack_ptr, rpn_end, argsbase;
#endif
#ifdef B1_FEATURE_DEBUG
	B1_T_INDEX id_off, id_len;
#endif

#ifdef B1_FEATURE_FUNCTIONS_USER
	rpn_stack_ptr = 0;
	rpn_end = 0;
	argsbase = 0;
#endif

	if(var_ref)
	{
		(*var_ref).var = NULL;
	}

	// temp stack top pointer (b1_rpn_eval)
	tmptop = 0;
	// current RPN record index
	i = 0;

	while(1)
	{
		// (B1_RPNREC_TYPE_IMM_VALUE, B1_RPNREC_TYPE_FNVAR, B1_RPNREC_TYPE_OPER (B1_RPNREC_OPER_LEFT_ASSOC and priority))
		tflags = (*(b1_rpn + i)).flags;

		// check for RPN end
		if(tflags == 0)
		{
			break;
		}

		var1 = b1_rpn_eval + tmptop;

#ifdef B1_FEATURE_MINIMAL_EVALUATION
		if(B1_RPNREC_TEST_SPEC_ARG(tflags))
		{
			if(	(tflags == B1_RPNREC_TYPE_SPEC_ARG_1 && !(*(var1 - 1)).value.bval) ||
				(tflags == B1_RPNREC_TYPE_SPEC_ARG_2)
				)
			{
				var1->type = B1_TYPE_SET(B1_TYPE_INT, 0);
				var1->value.i32val = 0;
				tmptop++;

				arg = (*(b1_rpn + i)).data.nestlevel;
				while(1)
				{
					i++;
					if(B1_RPNREC_TEST_SPEC_ARG((*(b1_rpn + i)).flags) && arg == (*(b1_rpn + i)).data.nestlevel)
					{
						break;
					}
				}
			}

			i++;
			continue;
		}
#endif

		if(B1_RPNREC_TEST_TYPES(tflags, B1_RPNREC_TYPE_FNVAR | B1_RPNREC_TYPE_FN_ARG))
		{
			var_type = (*(b1_rpn + i)).data.id.flags;
			name_hash = (*(b1_rpn + i)).data.id.hash;
#ifdef B1_FEATURE_DEBUG
			id_off = (*(b1_rpn + i)).data.id.offset;
			id_len = (*(b1_rpn + i)).data.id.length;
#endif
		}
		else
		if(B1_RPNREC_TEST_TYPES(tflags, B1_RPNREC_TYPE_OPER))
		{
			// the first and the second characters of the token
			c = (*(b1_rpn + i)).data.oper.c;
			c1 = (*(b1_rpn + i)).data.oper.c1;
		}
		else
		{
			// pointer to the current expression token structure
			ptoken = &(*(b1_rpn + i)).data.token;
		}

		if(B1_RPNREC_TEST_TYPES(tflags, B1_RPNREC_TYPE_IMM_VALUE))
		{
			// push the value to temp stack
			if(B1_MAX_RPN_EVAL_BUFFER_LEN == tmptop)
			{
				return B1_RES_ETMPSTKOVF;
			}

			if(B1_RPNREC_TEST_IMM_VALUE_NULL_ARG(tflags))
			{
				(*var1).type = B1_TYPE_SET(B1_TYPE_NULL, 0);
			}
			else
			{
				(*var1).type = B1_TYPE_SET(B1_TYPE_RPNREC_PTR, 0);
				(*var1).value.token = ptoken;

				err = b1_eval_get_value(tmptop);
				if(err != B1_RES_OK)
				{
					return err;
				}
			}
		}
		else
		{
			type = B1_TYPE_SET(B1_TYPE_INT, 0);

			// B1_RPNREC_TYPE_OPER, B1_RPNREC_TYPE_FNVAR or B1_RPNREC_TYPE_FN_ARG
			if(B1_RPNREC_TEST_TYPES(tflags, B1_RPNREC_TYPE_OPER))
			{
				// test for unary operator (0 - priority of unary -/+ and NOT operators)
				argnum = 1;
				type = B1_TYPE_GET((*(var1 - 1)).type);

				if(!B1_RPNREC_TEST_OPER_PRI(tflags, 0))
				{
					argnum++;
					type = b1_eval_get_common_type(type, B1_TYPE_GET((*(var1 - 2)).type));
				}

				// tflags = 0 means operator
				tflags = 0;
			}
#ifdef B1_FEATURE_FUNCTIONS_USER
			else
			if(B1_RPNREC_TEST_TYPES(tflags, B1_RPNREC_TYPE_FN_ARG))
			{
				// user function argument
				if(B1_MAX_RPN_EVAL_BUFFER_LEN == tmptop)
				{
					return B1_RES_ETMPSTKOVF;
				}

				// here argnum stands for function argument ordinal index not number of arguments
				argnum = B1_RPNREC_GET_FN_ARG_INDEX(tflags);
				tflags = 4;
			}
#endif
			else
			{
				// get function / variable number of arguments and their types
				// get argument count
				argnum = B1_RPNREC_GET_FNVAR_ARG_NUM(tflags);
				
				arg =	name_hash == B1_FN_TAB_FN_HASH ? B1_TYPE_SET(B1_TYPE_TAB_FN, 0) :
						name_hash == B1_FN_SPC_FN_HASH ? B1_TYPE_SET(B1_TYPE_SPC_FN, 0) : (uint8_t)0;

				// process special functions of PRINT statement
				if(arg != 0)
				{
					if(!(options & B1_EVAL_OPT_PRINT_FUNCTIONS))
					{
						return B1_RES_ERESWORD;
					}

					if(argnum != 1)
					{
						return B1_RES_EWRARGCNT;
					}

					var_type = arg;

					// tflags = 3 for TAB and SPC special functions
					tflags = 3;
				}
				else
				{	
					// tflags = 1 stands for function and tflags = 2 - variable
					tflags = 1;
					// test if the identifier is a function
#ifdef B1_FEATURE_FUNCTIONS_USER
					err = b1_fn_get_params(name_hash, 0, &fn);
#else
					err = b1_fn_get_params(name_hash, &fn);
#endif
					if(err == B1_RES_EUNKIDENT)
					{
						// not a function: variable subscripts are numerics (B1_TYPE_INT)
						// get/create variable (var_type here stands for variable type)
						err = b1_var_create(name_hash, var_type, argnum, NULL, &var);
						if(err == B1_RES_OK)
						{
#ifdef B1_FEATURE_DEBUG
							memcpy((*var).id.name + 1, b1_progline + id_off, id_len * B1_T_CHAR_SIZE);
							(*var).id.name[0] = (B1_T_CHAR)id_len;
#endif
							// the variable was created
							if(b1_opt_explicit_val)
							{
#ifdef B1_FEATURE_DEBUG
								// delete variable in case of debugging because the function
								// can be used for debug purpose so the error does not mean
								// the interpreter termination
								b1_int_var_mem_free(var);
								b1_ex_var_free(name_hash);
#endif
								return B1_RES_EUNKIDENT;
							}
						}
						else
						if(err == B1_RES_EIDINUSE)
						{
							err = B1_RES_OK;
						}

						tflags++;
					}
					else
					{
						// extract function argument types
						if(err == B1_RES_OK)
						{
							if(B1_IDENT_GET_FLAGS_ARGNUM((*fn).id.flags) != argnum)
							{
								err = B1_RES_EWRARGCNT;
							}
						}
					}

					if(err != B1_RES_OK)
					{
						return err;
					}
				}
			}

#ifdef B1_FEATURE_FUNCTIONS_USER
			if(tflags != 4)
			{
#endif
				if(argnum > tmptop)
				{
					// not enough operands or arguments in temp. stack
					return B1_RES_EWRARGCNT;
				}

				tmptop -= argnum;

				arg = argnum;

				while(arg != 0)
				{
					arg--;
					var1--;

					if(tflags == 1)
					{
						type = (*fn).argtypes[arg];
					}

					err = b1_var_convert(var1, type);
					if(err != B1_RES_OK)
					{
						return err;
					}
				}
#ifdef B1_FEATURE_FUNCTIONS_USER
			}
#endif

			// process operator
			if(tflags == 0)
			{
				// for operators all input and output types are the same
				type = B1_TYPE_GET((*var1).type);

				if(argnum == 1)
				{
					// unary - or +
					err = b1_eval_un_op(c, var1, type);
				}
				else
				{
					err = b1_eval_bin_op(c, c1, var1, type);
				}

				if(err != B1_RES_OK)
				{
					return err;
				}
			}
			else
			if(tflags == 1)
			{
				// function
#ifdef B1_FEATURE_FUNCTIONS_USER
				if(B1_IDENT_TEST_FLAGS_FN_BLTIN((*fn).id.flags))
				{
#endif
					// built-in function
					err = ((*((B1_BLTIN_FN *)fn)).fn_ptr)(var1);
					if(err != B1_RES_OK)
					{
						return err;
					}

					err = b1_var_convert(var1, (*fn).ret_type);
					if(err != B1_RES_OK)
					{
						return err;
					}
#ifdef B1_FEATURE_FUNCTIONS_USER
				}
				else
				{
					// user defined function
					if(rpn_stack_ptr == B1_MAX_UDEF_CALL_NEST_DEPTH)
					{
						return B1_RES_EUDEFOVF;
					}

					// save current program line
					b1_fn_udef_call_stack[rpn_stack_ptr].prev_line_cnt = b1_curr_prog_line_cnt;

					// save current RPN pointer (rpn and i)
					b1_fn_udef_call_stack[rpn_stack_ptr].prev_rpn = b1_rpn;
					b1_fn_udef_call_stack[rpn_stack_ptr].prev_rpn_end = rpn_end;
					b1_fn_udef_call_stack[rpn_stack_ptr].prev_rpn_index = i;

					b1_fn_udef_call_stack[rpn_stack_ptr].prev_argsbase = argsbase;

					b1_fn_udef_call_stack[rpn_stack_ptr].curr_arg_num = argnum;
					b1_fn_udef_call_stack[rpn_stack_ptr].curr_ret_type = (*((B1_UDEF_FN *)fn)).fn.ret_type;

					b1_rpn = b1_fn_udef_fn_rpn + (*((B1_UDEF_FN *)fn)).rpn_start_pos;
					rpn_end = (*((B1_UDEF_FN *)fn)).rpn_len;
					rpn_end--;
					argsbase = tmptop;
					i = 0;

					tmptop += argnum;

					// switch to DEF program line
					b1_curr_prog_line_cnt = (*((B1_UDEF_FN *)fn)).def_line_cnt - 1;
					err = b1_ex_prg_get_prog_line(B1_T_LINE_NUM_NEXT);
					if(err != B1_RES_OK)
					{
						return err;
					}

					rpn_stack_ptr++;

					continue;
				}
#endif
			}
			else
			if(tflags == 2)
			{
				// variable
				if(B1_MAX_RPN_EVAL_BUFFER_LEN == tmptop)
				{
					return B1_RES_ETMPSTKOVF;
				}

				// push the value to temp stack
#ifdef B1_FEATURE_FUNCTIONS_USER
				err = b1_var_get(var, var1, (rpn_stack_ptr != 0 || b1_rpn[i + 1].flags) ? NULL : var_ref);
#else
				err = b1_var_get(var, var1, b1_rpn[i + 1].flags ? NULL : var_ref);
#endif
				if(err != B1_RES_OK)
				{
					return err;
				}
			}
			else
			if(tflags == 3)
			{
				// PRINT function
				(*var1).type = var_type;
			}
#ifdef B1_FEATURE_FUNCTIONS_USER
			else
			{
				// argument of user defined function
				*var1 = *(b1_rpn_eval + argnum + argsbase);
				type = (*var1).type;
				if(type == B1_TYPE_SET(B1_TYPE_STRING, 0))
				{
					(*var1).type = B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_REF_FLAG);
				}
			}
#endif
		}

#ifdef B1_FEATURE_FUNCTIONS_USER
		while(rpn_stack_ptr != 0 && i == rpn_end)
		{
			rpn_stack_ptr--;

			b1_curr_prog_line_cnt = b1_fn_udef_call_stack[rpn_stack_ptr].prev_line_cnt - 1;

			b1_rpn = b1_fn_udef_call_stack[rpn_stack_ptr].prev_rpn;
			rpn_end = b1_fn_udef_call_stack[rpn_stack_ptr].prev_rpn_end;
			i = b1_fn_udef_call_stack[rpn_stack_ptr].prev_rpn_index;

			argsbase = b1_fn_udef_call_stack[rpn_stack_ptr].prev_argsbase;

			argnum = b1_fn_udef_call_stack[rpn_stack_ptr].curr_arg_num;
			type = b1_fn_udef_call_stack[rpn_stack_ptr].curr_ret_type;

			// free string arguments
			for(; argnum != 0; argnum--)
			{
				tmptop--;
				if((*(b1_rpn_eval + tmptop)).type == B1_TYPE_SET(B1_TYPE_STRING, 0))
				{
					b1_ex_mem_free((*(b1_rpn_eval + tmptop)).value.mem_desc);
				}
			}

			// restore previous program line
			err = b1_ex_prg_get_prog_line(B1_T_LINE_NUM_NEXT);
			if(err != B1_RES_OK)
			{
				return err;
			}

			err = b1_var_convert(var1, type);
			if(err != B1_RES_OK)
			{
				return err;
			}

			// move user function result
			*(b1_rpn_eval + tmptop) = *var1;
		}
#endif

		tmptop++;
		i++;
	}

	if(var_ref && (*var_ref).var == NULL)
	{
		return B1_RES_ENOTVAR;
	}

	return (tmptop == 1) ? B1_RES_OK : B1_RES_EWRARGCNT;
}
