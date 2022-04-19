/*
 BASIC1 interpreter
 Copyright (c) 2021 Nikolay Pletnev
 MIT license

 b1dbg.c: functions for debugging
*/

#include "b1feat.h"


#ifdef B1_FEATURE_DEBUG
#include <string.h>
#include <stdlib.h>

#include "b1.h"
#include "b1ex.h"
#include "b1err.h"
#include "b1dbg.h"


B1_T_INDEX b1_dbg_breakpoints_num;
B1_T_PROG_LINE_CNT b1_dbg_breakpoints[B1_MAX_BREAKPOINT_NUM];

#ifdef B1_FEATURE_RPN_CACHING
uint8_t b1_dbg_rpn_caching_enabled;
#endif


static uint8_t b1_dbg_copy_str(const B1_T_CHAR *str, B1_T_CHAR **sbuf, B1_T_INDEX *buflen)
{
	B1_T_INDEX len;

	len = (B1_T_INDEX)str[0];
	len = len >= *buflen ? *buflen - 1 : len;

	memcpy(*sbuf, str + 1, len * B1_T_CHAR_SIZE);
	*sbuf += len;
	*buflen -= len;

	**sbuf = B1_T_C_STRTERM;

	return (*buflen == 1) ? (uint8_t)1 : (uint8_t)0;
}

static B1_T_ERROR b1_dbg_var_to_str(B1_VAR *var)
{
	B1_T_ERROR err;

	if(!B1_TYPE_TEST_STRING((*var).type))
	{
		err = b1_var_convert(var, B1_TYPE_STRING);
		if(err != B1_RES_OK)
		{
			return err;
		}
	}

	err = b1_var_var2str(var, b1_tmp_buf);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

B1_T_ERROR b1_dbg_get_var_dump(const B1_NAMED_VAR *var, B1_T_CHAR *sbuf, B1_T_INDEX buflen)
{
	B1_T_ERROR err;
	B1_VAR tmpvar;
	B1_T_INDEX i;
	uint8_t type, dimnum;
	B1_T_SUBSCRIPT *arrdata;
	B1_T_MEMOFFSET ai, arrsize;
	B1_T_MEM_BLOCK_DESC desc;
	void *data;

	type = B1_TYPE_GET((*var).var.type);
	desc = (*var).var.value.mem_desc;
	dimnum = B1_IDENT_GET_FLAGS_ARGNUM((*var).id.flags);

	// copy variable name
	if(b1_dbg_copy_str((*var).id.name, &sbuf, &buflen))
	{
		return B1_RES_OK;
	}

	if(b1_dbg_copy_str(_DBG_TYPE_OPBR, &sbuf, &buflen))
	{
		return B1_RES_OK;
	}

	// copy variable type
	for(i = 0; i < B1_TYPE_COUNT; i++)
	{
		if(b1_t_types[i] == type)
		{
			if(b1_dbg_copy_str(b1_t_type_names[i], &sbuf, &buflen))
			{
				return B1_RES_OK;
			}

			break;
		}
	}

	// copy variable value
	if(dimnum == 0)
	{
		if(b1_dbg_copy_str(_DBG_TYPE_CLBR, &sbuf, &buflen))
		{
			return B1_RES_OK;
		}

		tmpvar = (*var).var;

		if(tmpvar.type == B1_TYPE_SET(B1_TYPE_STRING, 0))
		{
			tmpvar.type = B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_REF_FLAG);
		}

		err = b1_dbg_var_to_str(&tmpvar);
		if(err != B1_RES_OK)
		{
			return err;
		}

		b1_dbg_copy_str(b1_tmp_buf, &sbuf, &buflen);
	}
	else
	{
		// array
		// get array descriptor data
		if(desc == B1_T_MEM_BLOCK_DESC_INVALID)
		{
			// the descriptor can be invalid if a memory allocation error
			// occurred during array variable creation
			if(b1_dbg_copy_str(_DBG_TYPE_CLBR, &sbuf, &buflen))
			{
				return B1_RES_OK;
			}

			b1_dbg_copy_str(_DBG_INVALID, &sbuf, &buflen);

			return B1_RES_OK;
		}

		err = b1_ex_mem_access(desc, 0, 0, B1_EX_MEM_READ, (void **)&arrdata);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// check array subscripts
		arrsize = 1;
		for(i = 0; i < dimnum; i++)
		{
			// get lbound
			ai = *arrdata++;
			// get dimension size
			ai = (*arrdata++ - ai) + 1;
			arrsize *= ai;

			if(b1_dbg_copy_str(_DBG_DELIM, &sbuf, &buflen))
			{
				return B1_RES_OK;
			}

			tmpvar.type = B1_TYPE_SET(B1_TYPE_INT, 0);
			tmpvar.value.i32val = (int32_t)ai;

			err = b1_dbg_var_to_str(&tmpvar);
			if(err != B1_RES_OK)
			{
				return err;
			}

			if(b1_dbg_copy_str(b1_tmp_buf, &sbuf, &buflen))
			{
				return B1_RES_OK;
			}
		}
		desc = *((B1_T_MEM_BLOCK_DESC *)arrdata);

		if(b1_dbg_copy_str(_DBG_TYPE_CLBR, &sbuf, &buflen))
		{
			return B1_RES_OK;
		}

		// invalid memory block descriptor means non-allocated array
		if(desc == B1_T_MEM_BLOCK_DESC_INVALID)
		{
			b1_dbg_copy_str(_DBG_UNALLOC, &sbuf, &buflen);
		}
		else
		{
			ai = 0;
			while(1)
			{
				err = b1_var_array_get_data_ptr(desc, type, ai, &data);
				if(err != B1_RES_OK)
				{
					return err;
				}

				// copy array value to output variabe
#ifdef B1_FEATURE_TYPE_SINGLE
				if(type == B1_TYPE_SINGLE)
				{
					tmpvar.type = B1_TYPE_SET(B1_TYPE_SINGLE, 0);
					tmpvar.value.sval = *((float *)data);
				}
				else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
				if(type == B1_TYPE_DOUBLE)
				{
					tmpvar.type = B1_TYPE_SET(B1_TYPE_DOUBLE, 0);
					tmpvar.value.dval = *((double *)data);
				}
				else
#endif

				if(type == B1_TYPE_INT)
				{
					tmpvar.type = B1_TYPE_SET(B1_TYPE_INT, 0);
					tmpvar.value.i32val = *((int32_t *)data);
				}
				else
#ifdef B1_FEATURE_TYPE_SMALL
				if(type == B1_TYPE_INT16)
				{
					tmpvar.type = B1_TYPE_SET(B1_TYPE_INT16, 0);
					tmpvar.value.i16val = *((int16_t *)data);
				}
				else
				if(type == B1_TYPE_WORD)
				{
					tmpvar.type = B1_TYPE_SET(B1_TYPE_WORD, 0);
					tmpvar.value.ui16val = *((uint16_t *)data);
				}
				else
				if(type == B1_TYPE_BYTE)
				{
					tmpvar.type = B1_TYPE_SET(B1_TYPE_BYTE, 0);
					tmpvar.value.ui8val = *((uint8_t *)data);
				}
				else
#endif

				{
					tmpvar.type = B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_REF_FLAG);
					tmpvar.value.mem_desc = *((B1_T_MEM_BLOCK_DESC *)data);
				}

				err = b1_dbg_var_to_str(&tmpvar);
				if(err != B1_RES_OK)
				{
					return err;
				}

				if(b1_dbg_copy_str(b1_tmp_buf, &sbuf, &buflen))
				{
					return B1_RES_OK;
				}

				ai++;
				if(ai == arrsize)
				{
					break;
				}

				if(b1_dbg_copy_str(_DBG_DELIM, &sbuf, &buflen))
				{
					return B1_RES_OK;
				}
			}
		}
	}

	return B1_RES_OK;
}

static int b1_dbg_cmp_line_cnt(const void *lcnt1, const void *lcnt2)
{
	B1_T_IDHASH l1, l2;

	l1 = *((const B1_T_PROG_LINE_CNT *)lcnt1);
	l2 = *((const B1_T_PROG_LINE_CNT *)lcnt2);

	return	l1 < l2 ? -1 :
		l1 == l2 ? 0 : 1;
}

B1_T_INDEX b1_dbg_check_breakpoint(B1_T_PROG_LINE_CNT line_cnt)
{
	B1_T_PROG_LINE_CNT *line_cnt_ptr;

	line_cnt_ptr = (B1_T_PROG_LINE_CNT *)bsearch(&line_cnt, b1_dbg_breakpoints, b1_dbg_breakpoints_num, sizeof(B1_T_PROG_LINE_CNT), b1_dbg_cmp_line_cnt);
	return (line_cnt_ptr == NULL) ? (B1_T_INDEX)B1_MAX_BREAKPOINT_NUM : (B1_T_INDEX)(line_cnt_ptr - b1_dbg_breakpoints);
}

B1_T_ERROR b1_dbg_add_breakpoint(B1_T_PROG_LINE_CNT line_cnt)
{
	if(b1_dbg_check_breakpoint(line_cnt) == B1_MAX_BREAKPOINT_NUM)
	{
		if(b1_dbg_breakpoints_num == (B1_T_INDEX)(B1_MAX_BREAKPOINT_NUM - 1))
		{
			return B1_RES_EMANYBRKPNT;
		}

		b1_dbg_breakpoints[b1_dbg_breakpoints_num++] = line_cnt;
		qsort(b1_dbg_breakpoints, b1_dbg_breakpoints_num, sizeof(B1_T_PROG_LINE_CNT), b1_dbg_cmp_line_cnt);
	}

	return B1_RES_OK;
}

B1_T_ERROR b1_dbg_remove_breakpoint(B1_T_PROG_LINE_CNT line_cnt)
{
	B1_T_INDEX brk_off;
	B1_T_PROG_LINE_CNT *line_cnt_ptr;

	brk_off = b1_dbg_check_breakpoint(line_cnt);
	if(brk_off != B1_MAX_BREAKPOINT_NUM)
	{
		line_cnt_ptr = b1_dbg_breakpoints + brk_off;
		b1_dbg_breakpoints_num--;
		memmove(line_cnt_ptr, line_cnt_ptr + 1, b1_dbg_breakpoints_num - brk_off);
	}

	return B1_RES_OK;
}

B1_T_ERROR b1_dbg_remove_all_breakpoints()
{
	b1_dbg_breakpoints_num = 0;

	return B1_RES_OK;
}

B1_T_ERROR b1_dbg_get_break_line_cnt(B1_T_PROG_LINE_CNT *line_cnt)
{
	B1_T_ERROR err;
	B1_T_PROG_LINE_CNT prev_line_cnt;
	const B1_T_CHAR *prev_prg_line;


	prev_line_cnt = b1_curr_prog_line_cnt;
	prev_prg_line = b1_progline;

	err = (b1_curr_prog_line_offset == 0) ?
		b1_ex_prg_get_prog_line(b1_next_line_num) :
		B1_RES_OK;

	if(err == B1_RES_OK)
	{
		*line_cnt = b1_curr_prog_line_cnt;
	}

	b1_progline = prev_prg_line;
	b1_curr_prog_line_cnt = prev_line_cnt;

	return err;
}

#endif