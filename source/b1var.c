/*
 BASIC1 interpreter
 Copyright (c) 2021 Nikolay Pletnev
 MIT license

 b1var.c: managing variables
*/


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "b1ex.h"
#include "b1var.h"
#include "b1fn.h"
#include "b1int.h"
#include "b1err.h"


static B1_T_ERROR b1_var_put_str_to_mem(const B1_T_CHAR *s, B1_T_MEM_BLOCK_DESC *mem_desc)
{
	B1_T_ERROR err;
	void *data;
	B1_T_INDEX len;

	len = (*s + 1) * B1_T_CHAR_SIZE;

	err = b1_ex_mem_alloc(len, mem_desc, &data);
	if(err != B1_RES_OK)
	{
		return err;
	}

	memcpy(data, s, len);

	b1_ex_mem_release(*mem_desc);

	return B1_RES_OK;
}

// creates variable from string buffer
B1_T_ERROR b1_var_str2var(const B1_T_CHAR *s, B1_VAR *var)
{
	B1_T_ERROR err;
	B1_T_INDEX len;

	len = *s;

	if(len > B1_TYPE_STRING_IMM_MAX_LEN)
	{
		(*var).type = B1_TYPE_SET(B1_TYPE_STRING, 0);
		err = b1_var_put_str_to_mem(s, &(*var).value.mem_desc);
		if(err != B1_RES_OK)
		{
			return err;
		}
	}
	else
	{
		len++;
		len *= B1_T_CHAR_SIZE;
		(*var).type = B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_STRING_IMM_FLAG);
		memcpy((*var).value.istr, s, len);
	}

	return B1_RES_OK;
}

// moves string data from variable to the specified temp. buffer (b1_tmp_buf or b1_tmp_buf1), frees memory used by the source string
B1_T_ERROR b1_var_var2str(const B1_VAR *var, B1_T_CHAR *sbuf)
{
	B1_T_ERROR err;
	B1_T_MEM_BLOCK_DESC desc;
	const void *data;
	uint8_t type;

	data = NULL;
	desc = B1_T_MEM_BLOCK_DESC_INVALID;
	type = (*var).type;

	if(B1_TYPE_TEST_STRING(type))
	{
		if(B1_TYPE_TEST_STRING_IMM(type))
		{
			data = (*var).value.istr;
		}
		else
		{
			desc = (*var).value.mem_desc;

			// check the memory descriptor here because this function can be called
			// for temporary variables representing array elements
			if(desc == B1_T_MEM_BLOCK_DESC_INVALID)
			{
				sbuf[0] = 0;
			}
			else
 			{
				err = b1_ex_mem_access(desc, 0, 0, B1_EX_MEM_READ, (void **)&data);
				if(err != B1_RES_OK)
				{
					return err;
				}
			}
		}

		if(data != NULL)
		{
			memcpy(sbuf, data, (*((const B1_T_CHAR *)data) + 1) * B1_T_CHAR_SIZE);

			if(desc != B1_T_MEM_BLOCK_DESC_INVALID && !B1_TYPE_TEST_REF(type))
			{
				b1_ex_mem_free(desc);
			}
		}
	}
	else
	{
		return B1_RES_EWARGTYPE;
	}

	return B1_RES_OK;
}

#ifdef B1_FEATURE_TYPE_SMALL
static B1_T_ERROR b1_var_convert_int16(B1_VAR* var, uint8_t otype)
{
	B1_T_ERROR err;
	switch (otype)
	{
#ifdef B1_FEATURE_TYPE_DOUBLE
		case B1_TYPE_DOUBLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
			(*var).value.dval = (double)(*var).value.i16val;
			return B1_RES_OK;
#endif
#ifdef B1_FEATURE_TYPE_SINGLE
		case B1_TYPE_SINGLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
			(*var).value.sval = (float)(*var).value.i16val;
			return B1_RES_OK;
#endif
	case B1_TYPE_INT:
		(*var).type = B1_TYPE_SET(B1_TYPE_INT, 0);
		(*var).value.i32val = (int32_t)(*var).value.i16val;
		return B1_RES_OK;
	case B1_TYPE_WORD:
		(*var).type = B1_TYPE_SET(B1_TYPE_WORD, 0);
		(*var).value.ui16val = (uint16_t)(*var).value.i16val;
		return B1_RES_OK;
	case B1_TYPE_BYTE:
		(*var).type = B1_TYPE_SET(B1_TYPE_BYTE, 0);
		(*var).value.ui8val = (uint8_t)(*var).value.i16val;
		return B1_RES_OK;
	case B1_TYPE_STRING:
		err = b1_t_i32tostr((*var).value.i16val, b1_tmp_buf, B1_TMP_BUF_LEN);
		if (err != B1_RES_OK)
		{
			return err;
		}
		return b1_var_str2var(b1_tmp_buf, var);
	}

	return B1_RES_ETYPMISM;
}

static B1_T_ERROR b1_var_convert_word(B1_VAR* var, uint8_t otype)
{
	B1_T_ERROR err;
	switch (otype)
	{
#ifdef B1_FEATURE_TYPE_DOUBLE
		case B1_TYPE_DOUBLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
			(*var).value.dval = (double)(*var).value.ui16val;
			return B1_RES_OK;
#endif
#ifdef B1_FEATURE_TYPE_SINGLE
		case B1_TYPE_SINGLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
			(*var).value.sval = (float)(*var).value.ui16val;
			return B1_RES_OK;
#endif
	case B1_TYPE_INT:
			(*var).type = B1_TYPE_SET(B1_TYPE_INT, 0);
			(*var).value.i32val = (int32_t)(*var).value.ui16val;
			return B1_RES_OK;
	case B1_TYPE_INT16:
			(*var).type = B1_TYPE_SET(B1_TYPE_INT16, 0);
			(*var).value.i16val = (int16_t)(*var).value.ui16val;
			return B1_RES_OK;
	case B1_TYPE_BYTE:
			(*var).type = B1_TYPE_SET(B1_TYPE_BYTE, 0);
			(*var).value.ui8val = (uint8_t)(*var).value.ui16val;
			return B1_RES_OK;
	case B1_TYPE_STRING:
			err = b1_t_i32tostr((*var).value.ui16val, b1_tmp_buf, B1_TMP_BUF_LEN);
			if (err != B1_RES_OK)
			{
				return err;
			}
			return b1_var_str2var(b1_tmp_buf, var);
	}

	return B1_RES_ETYPMISM;
}

static B1_T_ERROR b1_var_convert_byte(B1_VAR* var, uint8_t otype)
{
	B1_T_ERROR err;
	switch (otype)
	{
#ifdef B1_FEATURE_TYPE_DOUBLE
		case B1_TYPE_DOUBLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
			(*var).value.dval = (double)(*var).value.ui8val;
			return B1_RES_OK;
#endif
#ifdef B1_FEATURE_TYPE_SINGLE
		case B1_TYPE_SINGLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
			(*var).value.sval = (float)(*var).value.ui8val;
			return B1_RES_OK;
#endif
	case B1_TYPE_INT:
			(*var).type = B1_TYPE_SET(B1_TYPE_INT, 0);
			(*var).value.i32val = (int32_t)(*var).value.ui8val;
			return B1_RES_OK;
	case B1_TYPE_INT16:
			(*var).type = B1_TYPE_SET(B1_TYPE_INT16, 0);
			(*var).value.i16val = (int16_t)(*var).value.ui8val;
			return B1_RES_OK;
	case B1_TYPE_WORD:
			(*var).type = B1_TYPE_SET(B1_TYPE_WORD, 0);
			(*var).value.ui16val = (uint16_t)(*var).value.ui8val;
			return B1_RES_OK;
	case B1_TYPE_STRING:
		err = b1_t_i32tostr((*var).value.ui8val, b1_tmp_buf, B1_TMP_BUF_LEN);
		if (err != B1_RES_OK)
		{
			return err;
		}
		return b1_var_str2var(b1_tmp_buf, var);
	}

	return B1_RES_ETYPMISM;
}
#endif

static B1_T_ERROR b1_var_convert_int(B1_VAR *var, uint8_t otype)
{
	B1_T_ERROR err;
	switch(otype)
	{
#ifdef B1_FEATURE_TYPE_DOUBLE
		case B1_TYPE_DOUBLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
			(*var).value.dval = (double)(*var).value.i32val;
			return B1_RES_OK;
#endif
#ifdef B1_FEATURE_TYPE_SINGLE
		case B1_TYPE_SINGLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
			(*var).value.sval = (float)(*var).value.i32val;
			return B1_RES_OK;
#endif
#ifdef B1_FEATURE_TYPE_SMALL
		case B1_TYPE_INT16:
			(*var).type = B1_TYPE_SET(B1_TYPE_INT16, 0);
			(*var).value.i16val = (int16_t)(*var).value.i32val;
			return B1_RES_OK;
		case B1_TYPE_WORD:
			(*var).type = B1_TYPE_SET(B1_TYPE_WORD, 0);
			(*var).value.ui16val = (uint16_t)(*var).value.i32val;
			return B1_RES_OK;
		case B1_TYPE_BYTE:
			(*var).type = B1_TYPE_SET(B1_TYPE_BYTE, 0);
			(*var).value.ui8val = (uint8_t)(*var).value.i32val;
			return B1_RES_OK;
#endif
		case B1_TYPE_STRING:
			err = b1_t_i32tostr((*var).value.i32val, b1_tmp_buf, B1_TMP_BUF_LEN);
			if(err != B1_RES_OK)
			{
				return err;
			}
			return b1_var_str2var(b1_tmp_buf, var);
	}

	return B1_RES_ETYPMISM;
}

#ifdef B1_FEATURE_TYPE_DOUBLE
static B1_T_ERROR b1_var_convert_double(B1_VAR *var, uint8_t otype)
{
	B1_T_ERROR err;

	switch(otype)
	{
#ifdef B1_FEATURE_TYPE_SINGLE
		case B1_TYPE_SINGLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
			(*var).value.sval = (float)(*var).value.dval;
			return B1_RES_OK;
#endif
		case B1_TYPE_INT:
#ifdef B1_FEATURE_TYPE_SMALL
		case B1_TYPE_INT16:
		case B1_TYPE_WORD:
		case B1_TYPE_BYTE:
#endif
			// convert floating point value to integer with rounding
			(*var).type = B1_TYPE_SET(otype, 0);
			(*var).value.i32val = (int32_t)((*var).value.dval + (((*var).value.dval < 0.0) ? -0.5 : 0.5));
#ifdef B1_FEATURE_TYPE_SMALL
			switch (otype)
			{
				case B1_TYPE_INT16:
					(*var).value.i16val = (int16_t)(*var).value.i32val;
					break;
				case B1_TYPE_WORD:
					(*var).value.ui16val = (uint16_t)(*var).value.i32val;
					break;
				case B1_TYPE_BYTE:
					(*var).value.ui8val = (uint8_t)(*var).value.i32val;
					break;
			}
#endif
			return B1_RES_OK;
		case B1_TYPE_STRING:
			err = b1_t_doubletostr((*var).value.dval, b1_tmp_buf, B1_TMP_BUF_LEN, b1_int_print_zone_width - 2);
			if(err != B1_RES_OK)
			{
				return err;
			}
			return b1_var_str2var(b1_tmp_buf, var);
	}

	return B1_RES_ETYPMISM;
}
#endif

#ifdef B1_FEATURE_TYPE_SINGLE
static B1_T_ERROR b1_var_convert_single(B1_VAR *var, uint8_t otype)
{
	B1_T_ERROR err;

	switch(otype)
	{
#ifdef B1_FEATURE_TYPE_DOUBLE
		case B1_TYPE_DOUBLE:
			(*var).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
			(*var).value.dval = (double)(*var).value.sval;
			return B1_RES_OK;
#endif
		case B1_TYPE_INT:
#ifdef B1_FEATURE_TYPE_SMALL
		case B1_TYPE_INT16:
		case B1_TYPE_WORD:
		case B1_TYPE_BYTE:
#endif
			// convert floating point value to integer with rounding
			(*var).type = B1_TYPE_SET(otype, 0);
			(*var).value.i32val = (int32_t)((*var).value.sval + (((*var).value.sval < 0.0f) ? -0.5f : 0.5f));
#ifdef B1_FEATURE_TYPE_SMALL
			switch(otype)
			{
				case B1_TYPE_INT16:
					(*var).value.i16val = (int16_t)(*var).value.i32val;
					break;
				case B1_TYPE_WORD:
					(*var).value.ui16val = (uint16_t)(*var).value.i32val;
					break;
				case B1_TYPE_BYTE:
					(*var).value.ui8val = (uint8_t)(*var).value.i32val;
					break;
			}
#endif
			return B1_RES_OK;
		case B1_TYPE_STRING:
			err = b1_t_singletostr((*var).value.sval, b1_tmp_buf, B1_TMP_BUF_LEN, b1_int_print_zone_width - 2);
			if(err != B1_RES_OK)
			{
				return err;
			}
			return b1_var_str2var(b1_tmp_buf, var);
	}

	return B1_RES_ETYPMISM;
}
#endif

B1_T_ERROR b1_var_convert(B1_VAR *var, uint8_t otype)
{
	uint8_t type;

	otype = B1_TYPE_GET(otype);
	type = B1_TYPE_GET((*var).type);

	if(otype == B1_TYPE_ANY || otype == type)
	{
		return B1_RES_OK;
	}

	switch(type)
	{
#ifdef B1_FEATURE_TYPE_DOUBLE
		case B1_TYPE_DOUBLE:
			return b1_var_convert_double(var, otype);
#endif
#ifdef B1_FEATURE_TYPE_SINGLE
		case B1_TYPE_SINGLE:
			return b1_var_convert_single(var, otype);
#endif
		case B1_TYPE_INT:
			return b1_var_convert_int(var, otype);
#ifdef B1_FEATURE_TYPE_SMALL
		case B1_TYPE_INT16:
			return b1_var_convert_int16(var, otype);
		case B1_TYPE_WORD:
			return b1_var_convert_word(var, otype);
		case B1_TYPE_BYTE:
			return b1_var_convert_byte(var, otype);
#endif
		default:
			return B1_RES_ETYPMISM;
	}

	return B1_RES_OK;
}

B1_T_ERROR b1_var_init_empty(uint8_t type, uint8_t argnum, const B1_T_SUBSCRIPT *subs_bounds, B1_VAR *pvar)
{
	B1_T_ERROR err;
	uint8_t i;
	B1_T_MEM_BLOCK_DESC mem_desc;
	B1_T_SUBSCRIPT *data;

	if(argnum)
	{
		argnum *= 2;

		// empty array
		err = b1_ex_mem_alloc(argnum * ((uint8_t)sizeof(B1_T_SUBSCRIPT)) + ((uint8_t)sizeof(B1_T_MEM_BLOCK_DESC)), &mem_desc, (void **)&data);
		if(err != B1_RES_OK)
		{
			return err;
		}

		for(i = 0; i < argnum; i++)
		{
			if(subs_bounds != NULL)
			{
				*(data + i) = *(subs_bounds + i);
			}
			else
			{
				*(data + i) = (i & 1) ? (uint8_t)(B1_DEF_SUBSCRIPT_UBOUND) : b1_opt_base_val;
			}
		}

		// not allocated at the moment
		*((B1_T_MEM_BLOCK_DESC *)(data + i)) = B1_T_MEM_BLOCK_DESC_INVALID;

		b1_ex_mem_release(mem_desc);

		(*pvar).type = B1_TYPE_SET(type, 0);
		(*pvar).value.mem_desc = mem_desc;
	}
	else
#ifdef B1_FEATURE_TYPE_SINGLE
	if(type == B1_TYPE_SINGLE)
	{
		(*pvar).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
		(*pvar).value.sval = 0.0f;
	}
	else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
	if(type == B1_TYPE_DOUBLE)
	{
		(*pvar).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
		(*pvar).value.dval = 0.0;
	}
	else
#endif
	if(type == B1_TYPE_INT)
	{
		(*pvar).type = B1_TYPE_SET(B1_TYPE_INT, 0);
		(*pvar).value.i32val = 0;
	}
	else
#ifdef B1_FEATURE_TYPE_SMALL
	if(type == B1_TYPE_INT16)
	{
		(*pvar).type = B1_TYPE_SET(B1_TYPE_INT16, 0);
		(*pvar).value.i16val = 0;
	}
	else
	if(type == B1_TYPE_WORD)
	{
		(*pvar).type = B1_TYPE_SET(B1_TYPE_WORD, 0);
		(*pvar).value.ui16val = 0;
	}
	else
	if(type == B1_TYPE_BYTE)
	{
		(*pvar).type = B1_TYPE_SET(B1_TYPE_BYTE, 0);
		(*pvar).value.ui8val = 0;
	}
	else
#endif
	if(type == B1_TYPE_STRING)
	{
		// initialize with empty string
		(*pvar).type = B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_STRING_IMM_FLAG);
		(*pvar).value.istr[0] = 0;
	}
	else
	if(type == B1_TYPE_BOOL)
	{
		(*pvar).type = B1_TYPE_SET(B1_TYPE_BOOL, 0);
		(*pvar).value.bval = 0;
	}

	return B1_RES_OK;
}

// creates variable if it does not exist, if the variable exists the function just returns pointer to its B1_NAMED_VAR structure in var argument
// variable. returns B1_RES_OK code if variabe was created and B1_RES_EIDINUSE if it existed
B1_T_ERROR b1_var_create(B1_T_IDHASH name_hash, uint8_t type, uint8_t argnum, const B1_T_SUBSCRIPT *subs_bounds, B1_NAMED_VAR **var)
{
	B1_T_ERROR err;
	B1_NAMED_VAR *newvar;
#ifdef B1_FEATURE_CHECK_KEYWORDS
	B1_FN *fn;
#endif

	if(argnum > B1_MAX_VAR_DIM_NUM)
	{
		return B1_RES_EWSUBSCNT;
	}

	err = b1_ex_var_alloc(name_hash, &newvar);

	if(err == B1_RES_OK)
	{
#ifdef B1_FEATURE_CHECK_KEYWORDS
		if(b1_id_get_stmt_by_hash(name_hash) != B1_ID_STMT_UNKNOWN ||
#ifdef B1_FEATURE_FUNCTIONS_USER
			b1_fn_get_params(name_hash, 0, &fn) != B1_RES_EUNKIDENT
#else
			b1_fn_get_params(name_hash, &fn) != B1_RES_EUNKIDENT
#endif
			)
		{
			b1_ex_var_free(name_hash);
			return B1_RES_ERESKWRD;
		}
#endif

		(*newvar).id.name_hash = name_hash;
		(*newvar).id.flags = B1_IDENT_FLAGS_SET_VAR(argnum);

		err = b1_var_init_empty(B1_TYPE_GET(type), argnum, subs_bounds, &(*newvar).var);
		if(err != B1_RES_OK)
		{
			return err;
		}
	}
	else
	if(err == B1_RES_EIDINUSE && B1_IDENT_GET_FLAGS_ARGNUM((*newvar).id.flags) != argnum)
	{
		return B1_RES_EWSUBSCNT;
	}

	if(var != NULL)
	{
		*var = newvar;
	}

	return err;
}

static uint8_t b1_var_get_type_size(uint8_t type)
{
	return
#ifdef B1_FEATURE_TYPE_SINGLE
		(type == B1_TYPE_SINGLE) ?	(uint8_t)sizeof(float) :
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
		(type == B1_TYPE_DOUBLE) ?	(uint8_t)sizeof(double) :
#endif
		(type == B1_TYPE_INT) ?	(uint8_t)sizeof(int32_t) :
#ifdef B1_FEATURE_TYPE_SMALL
		(type == B1_TYPE_INT16) ?	(uint8_t)sizeof(int16_t) :
		(type == B1_TYPE_WORD) ?	(uint8_t)sizeof(uint16_t) :
		(type == B1_TYPE_BYTE) ?	(uint8_t)sizeof(uint8_t) :
#endif
									(uint8_t)sizeof(B1_T_MEM_BLOCK_DESC);
}

static B1_T_ERROR b1_var_array_alloc(B1_T_MEM_BLOCK_DESC arrdesc, uint8_t type, uint8_t argnum, B1_T_MEMOFFSET size, B1_T_MEM_BLOCK_DESC *arrdatadesc)
{
	B1_T_ERROR err;
	uint8_t size1;
	B1_T_MEMOFFSET mem_size;
	void *data;
	B1_T_INDEX max1, i;

	size1 = b1_var_get_type_size(type);
	max1 = ((B1_T_INDEX)(B1_MAX_STRING_LEN + 1)) / size1;
	mem_size = size * size1;

	err = b1_ex_mem_alloc(mem_size, arrdatadesc, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	for(size = 0, i = 0; size != mem_size; size += size1, i++)
	{
		if(i == max1)
		{
			i = 0;
		}

		if(i == 0)
		{
			b1_ex_mem_release(*arrdatadesc);
			err = b1_ex_mem_access(*arrdatadesc, size, max1 * size1, B1_EX_MEM_WRITE, &data);
			if(err != B1_RES_OK)
			{
				b1_ex_mem_free(*arrdatadesc);
				return err;
			}
		}

#ifdef B1_FEATURE_TYPE_SINGLE
		if(type == B1_TYPE_SINGLE)
		{
			*(((float *)data) + i) = 0.0f;
		}
		else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
		if(type == B1_TYPE_DOUBLE)
		{
			*(((double *)data) + i) = 0.0;
		}
		else
#endif
		if(type == B1_TYPE_INT)
		{
			*(((int32_t *)data) + i) = 0;
		}
		else
#ifdef B1_FEATURE_TYPE_SMALL
		if(type == B1_TYPE_INT16)
		{
			*(((int16_t *)data) + i) = 0;
		}
		else
		if(type == B1_TYPE_WORD)
		{
			*(((uint16_t *)data) + i) = 0;
		}
		else
		if(type == B1_TYPE_BYTE)
		{
			*(((uint8_t *)data) + i) = 0;
		}
		else
#endif
		if(type == B1_TYPE_STRING)
		{
			// initialize with empty string
			*(((B1_T_MEM_BLOCK_DESC *)data) + i) = B1_T_MEM_BLOCK_DESC_INVALID;
		}
	}

	b1_ex_mem_release(*arrdatadesc);

	err = b1_ex_mem_access(arrdesc, argnum * (uint8_t)2 * (uint8_t)sizeof(B1_T_SUBSCRIPT), sizeof(B1_T_MEM_BLOCK_DESC), B1_EX_MEM_WRITE, &data);
	if(err != B1_RES_OK)
	{
		b1_ex_mem_free(*arrdatadesc);
		return err;
	}

	*((B1_T_MEM_BLOCK_DESC *)data) = *arrdatadesc;

	b1_ex_mem_release(arrdesc);

	return B1_RES_OK;
}

// returns pointer to an array element in read/write mode
B1_T_ERROR b1_var_array_get_data_ptr(B1_T_MEM_BLOCK_DESC arr_data_desc, uint8_t type, B1_T_MEMOFFSET offset, void **data)
{
	uint8_t size1;

	size1 = b1_var_get_type_size(type);

	return b1_ex_mem_access(arr_data_desc, offset * size1, size1, B1_EX_MEM_READ | B1_EX_MEM_WRITE, data);
}

// copies value from named variable to temp. stack variable (dst_var must point on a temp. stack variable that can consist of
// the variable itself and its subscripts), fills B1_VAR_REF structure if src_var_ref parameter is not NULL
// the function always returns references to string data if src_var is a string variable (B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_REF_FLAG))
B1_T_ERROR b1_var_get(B1_NAMED_VAR *src_var, B1_VAR *dst_var, B1_VAR_REF *src_var_ref)
{
	B1_T_ERROR err;
	uint8_t i, argnum, type;
	B1_T_SUBSCRIPT *arrdata, subs, ubound;
	B1_T_MEMOFFSET arrsize, offset;
	B1_T_MEM_BLOCK_DESC arrdatadesc;
	void *data;

	if(src_var_ref)
	{
		(*src_var_ref).var = src_var;
		(*src_var_ref).val_off = 0;
	}

	argnum = B1_IDENT_GET_FLAGS_ARGNUM((*src_var).id.flags);
	type = B1_TYPE_GET((*src_var).var.type);

	arrsize = 1;
	offset = 0;

	if(argnum == 0)
	{
		*dst_var = (*src_var).var;
		if((*dst_var).type == B1_TYPE_SET(B1_TYPE_STRING, 0))
		{
			(*dst_var).type = B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_REF_FLAG);
		}
	}
	else
	{
		// get array descriptor data
		err = b1_ex_mem_access((*src_var).var.value.mem_desc, 0, 0, B1_EX_MEM_READ, (void **)&arrdata);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// check array subscripts
		arrdata += argnum * (uint8_t)2;
		arrdatadesc = *((B1_T_MEM_BLOCK_DESC *)arrdata);

		for(i = argnum; i != 0;)
		{
			i--;

			if(!B1_TYPE_TEST_INT((*(dst_var + i)).type))
			{
				return B1_RES_ETYPMISM;
			}

			subs = (B1_T_SUBSCRIPT)(*(dst_var + i)).value.i32val;

			arrdata--;
			ubound = *arrdata;

			// ubound
			if(subs > ubound)
			{
				return B1_RES_ESUBSRANGE;
			}

			arrdata--;

			// lbound
			if(subs < *arrdata)
			{
				return B1_RES_ESUBSRANGE;
			}

			// now subs is not a subscript value but zero-based offset
			offset += (((B1_T_MEMOFFSET)subs) - *arrdata) * arrsize;
			arrsize *= ((B1_T_MEMOFFSET)ubound) - *arrdata + 1;
		}

		// invalid memory block descriptor means non-allocated array
		if(!src_var_ref && arrdatadesc == B1_T_MEM_BLOCK_DESC_INVALID)
		{
			// just return initial value even without memory allocation
			err = b1_var_init_empty(type, 0, NULL, dst_var);
			if(err != B1_RES_OK)
			{
				return err;
			}
		}
		else
		{
			// allocated array
			// return value according to subscripts
			if(arrdatadesc == B1_T_MEM_BLOCK_DESC_INVALID)
			{
				// allocate memory for array values
				err = b1_var_array_alloc((*src_var).var.value.mem_desc, type, argnum, arrsize, &arrdatadesc);
				if(err != B1_RES_OK)
				{
					return err;
				}
			}

			// return value
			err = b1_var_array_get_data_ptr(arrdatadesc, type, offset, &data);
			if(err != B1_RES_OK)
			{
				return err;
			}

			// copy array value to output variabe
#ifdef B1_FEATURE_TYPE_SINGLE
			if(type == B1_TYPE_SINGLE)
			{
				(*dst_var).type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
				(*dst_var).value.sval = *((float *)data);
			}
			else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
			if(type == B1_TYPE_DOUBLE)
			{
				(*dst_var).type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
				(*dst_var).value.dval = *((double *)data);
			}
			else
#endif
			if(type == B1_TYPE_INT)
			{
				(*dst_var).type = B1_TYPE_SET(B1_TYPE_INT, 0);
				(*dst_var).value.i32val = *((int32_t *)data);
			}
			else
#ifdef B1_FEATURE_TYPE_SMALL
			if(type == B1_TYPE_INT16)
			{
				(*dst_var).type = B1_TYPE_SET(B1_TYPE_INT16, 0);
				(*dst_var).value.i16val = *((int16_t *)data);
			}
			else
			if(type == B1_TYPE_WORD)
			{
				(*dst_var).type = B1_TYPE_SET(B1_TYPE_WORD, 0);
				(*dst_var).value.ui16val = *((uint16_t *)data);
			}
			else
			if(type == B1_TYPE_BYTE)
			{
				(*dst_var).type = B1_TYPE_SET(B1_TYPE_BYTE, 0);
				(*dst_var).value.ui8val = *((uint8_t *)data);
			}
			else
#endif
			{
				(*dst_var).type = B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_REF_FLAG);
				(*dst_var).value.mem_desc = *((B1_T_MEM_BLOCK_DESC *)data);
			}

			// return var reference
			if(src_var_ref)
			{
				(*src_var_ref).val_off = offset;
			}
		}
	}

	return B1_RES_OK;
}

// moves value from temp. stack to a named variable identified by dst_var_ref argument
B1_T_ERROR b1_var_set(B1_VAR *src_var, const B1_VAR_REF *dst_var_ref)
{
	B1_T_ERROR err;
	uint8_t type, argnum, access;
	B1_T_MEM_BLOCK_DESC arrdesc, dstdesc;
	void *data;

	argnum = B1_IDENT_GET_FLAGS_ARGNUM((*(*dst_var_ref).var).id.flags);
	type = B1_TYPE_GET((*(*dst_var_ref).var).var.type);
	dstdesc = (*(*dst_var_ref).var).var.value.mem_desc;

	err = b1_var_convert(src_var, type);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(argnum == 0)
	{
		if((*(*dst_var_ref).var).var.type == B1_TYPE_SET(B1_TYPE_STRING, 0))
		{
			// free memory occupied by string dst_var_ref points to
			b1_ex_mem_free(dstdesc);
		}

		(*(*dst_var_ref).var).var = *src_var;
	}
	else
	{
		// array value
		err = b1_ex_mem_access(dstdesc, argnum * (uint8_t)2 * (uint8_t)sizeof(B1_T_SUBSCRIPT), sizeof(B1_T_MEM_BLOCK_DESC), B1_EX_MEM_READ, &data);
		if(err != B1_RES_OK)
		{
			return err;
		}

		arrdesc = *((B1_T_MEM_BLOCK_DESC *)data);

		err = b1_var_array_get_data_ptr(arrdesc, type, (*dst_var_ref).val_off, &data);
		if(err != B1_RES_OK)
		{
			return err;
		}

		access = 0;

#ifdef B1_FEATURE_TYPE_SINGLE
		if(type == B1_TYPE_SINGLE)
		{
			*((float *)data) = (*src_var).value.sval;
		}
		else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
		if(type == B1_TYPE_DOUBLE)
		{
			*((double *)data) = (*src_var).value.dval;
		}
		else
#endif
		if(type == B1_TYPE_INT)
		{
			*((int32_t *)data) = (*src_var).value.i32val;
		}
		else
#ifdef B1_FEATURE_TYPE_SMALL
		if(type == B1_TYPE_INT16)
		{
			*((int16_t *)data) = (*src_var).value.i16val;
		}
		else
		if(type == B1_TYPE_WORD)
		{
			*((uint16_t *)data) = (*src_var).value.ui16val;
		}
		else
		if(type == B1_TYPE_BYTE)
		{
			*((uint8_t *)data) = (*src_var).value.ui8val;
		}
		else
#endif
		{
			if(*((B1_T_MEM_BLOCK_DESC *)data) != B1_T_MEM_BLOCK_DESC_INVALID)
			{
				// free memory occupied by string dst_var_ref points to
				b1_ex_mem_free(*((B1_T_MEM_BLOCK_DESC *)data));
			}

			if(B1_TYPE_TEST_STRING_IMM((*src_var).type))
			{
				access = 1;

				data = (*src_var).value.istr;

				err = b1_var_put_str_to_mem((B1_T_CHAR *)data, &dstdesc);
				if(err != B1_RES_OK)
				{
					return err;
				}
			}
			else
			{
				dstdesc = (*src_var).value.mem_desc;
			}

			if(access)
			{
				err = b1_var_array_get_data_ptr(arrdesc, type, (*dst_var_ref).val_off, &data);
				if(err != B1_RES_OK)
				{
					return err;
				}
			}

			*((B1_T_MEM_BLOCK_DESC *)data) = dstdesc;
		}

		b1_ex_mem_release(arrdesc);
	}

	return B1_RES_OK;
}
