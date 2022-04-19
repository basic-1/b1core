/*
 BASIC1 interpreter
 Copyright (c) 2021 Nikolay Pletnev
 MIT license

 b1int.c: the interpreter main file
*/


#include <stdio.h>
#include <string.h>

#include "b1ex.h"
#include "b1types.h"
#include "b1tok.h"
#include "b1rpn.h"
#include "b1fn.h"
#include "b1eval.h"
#include "b1int.h"
#include "b1dbg.h"
#include "b1err.h"


#define B1_INT_READ_VALUE_LAST ((uint8_t)0x1)
#define B1_INT_READ_VALUE_QSTRING ((uint8_t)0x2)
#define B1_INT_READ_VALUE_NUMERIC ((uint8_t)0x4)

#define B1_INT_ST_INPUT_PROMPT_PRINT ((uint8_t)0x0)
#define B1_INT_ST_INPUT_PROMPT_PRINTED ((uint8_t)0x1)
#define B1_INT_ST_INPUT_PROMPT_CUSTOM ((uint8_t)0x2)


// global data
// current statement state (some statements have execution states)
static uint8_t b1_int_curr_stmt_state;

// statement call stack (used with IF, GOSUB and FOR statements)
static B1_T_INDEX b1_int_stmt_stack_ptr;
static B1_INT_STMT_STK_REC b1_int_stmt_stack[B1_MAX_STMT_NEST_DEPTH];

// output device properties and current print position
static uint8_t b1_int_print_margin;
uint8_t b1_int_print_zone_width;
static uint8_t b1_int_print_zone_num;
uint8_t b1_int_print_curr_pos;

// echo input
uint8_t b1_int_input_echo;

#ifdef B1_FEATURE_STMT_STOP
uint8_t b1_int_exec_stop;
#endif

#ifdef B1_FEATURE_DEBUG
static uint8_t b1_int_continue_after_break;
#endif


// initializes or resets interpreter
B1_T_ERROR b1_int_reset()
{
#ifdef B1_FEATURE_INIT_FREE_MEMORY
	B1_T_ERROR err;
	B1_NAMED_VAR *var;

	var = NULL;
#endif

	b1_reset();

	b1_int_curr_stmt_state = 0;

#ifdef B1_FEATURE_INIT_FREE_MEMORY
	while(1)
	{
		b1_ex_var_enum(&var);

		if(var == NULL)
		{
			break;
		}

		err = b1_int_var_mem_free(var);
		if(err != B1_RES_OK)
		{
			return err;
		}
	}
#endif

	// clear variables cache
	b1_ex_var_init();

#ifdef B1_FEATURE_FUNCTIONS_USER
	// clear user functions
	b1_fn_udef_fn_rpn_off = 0;
	b1_ex_ufn_init();
#endif

	// reset statements call stack
	b1_int_stmt_stack_ptr = 0;

	// reset output device settings
	b1_int_print_margin = B1_DEF_PRINT_MARGIN;
	b1_int_print_zone_width = B1_DEF_PRINT_ZONE_WIDTH;
	b1_int_print_zone_num = (B1_DEF_PRINT_MARGIN) / (B1_DEF_PRINT_ZONE_WIDTH);

	// reset current print position
	b1_int_print_curr_pos = 0;

	b1_int_input_echo = 0;

#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
	b1_ex_rnd_randomize(1);
#endif
#endif

#ifdef B1_FEATURE_DEBUG
	b1_int_continue_after_break = 0;
	b1_dbg_breakpoints_num = 0;

#ifdef B1_FEATURE_RPN_CACHING
	b1_dbg_rpn_caching_enabled = 1;
#endif
#endif

#ifdef B1_FEATURE_STMT_STOP
	b1_int_exec_stop = 0;
#endif

	// initialize memory manager
	return b1_ex_mem_init();
}

// interpret LET statement
static B1_T_ERROR b1_int_st_let(B1_T_INDEX offset, B1_VAR_REF *dst_var_ref, const B1_T_CHAR **stop_tokens, B1_T_INDEX *continue_offset)
{
	B1_T_ERROR err;
	uint8_t stop;
	B1_T_INDEX right_exp_off;
	B1_TOKENDATA td;
	B1_VAR tmpvar;
	
	// look for assignment operator
	right_exp_off = offset;
	stop = 0;
	while(!stop)
	{
		err = b1_tok_get(right_exp_off, 0, &td);
		if(err != B1_RES_OK) return err;
		right_exp_off = td.offset;
		if(td.length == 0) return B1_RES_ESYNTAX;
		// only one stop token for LET statement ("=")
		stop = !b1_t_strcmpi(LET_STOP_TOKENS[0], b1_progline + right_exp_off, td.length);
		right_exp_off += td.length;
	}

	// build RPN for the right part of the expresssion
	err = b1_rpn_build(right_exp_off, stop_tokens, continue_offset);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// evaluate the expression part
	err = b1_eval(0, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// save the value
	tmpvar = b1_rpn_eval[0];

	// build RPN for the left part of the expression
	right_exp_off = 0;
	err = b1_rpn_build(offset, LET_STOP_TOKENS, &right_exp_off);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// no assignment operation found
	if(!right_exp_off)
	{
		return B1_RES_ESYNTAX;
	}

	// evaluate the expression on the left side of assignment operator
	err = b1_eval(0, dst_var_ref);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(tmpvar.type == B1_TYPE_SET(B1_TYPE_STRING, B1_TYPE_REF_FLAG))
	{
		err = b1_var_var2str(&tmpvar, b1_tmp_buf);
		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_var_str2var(b1_tmp_buf, &tmpvar);
		if(err != B1_RES_OK)
		{
			return err;
		}
	}


	// assign result of the second expression to the variable returned by the first expression
	// note: b1_var_set function frees memory used by destination variable before assigning new value
	err = b1_var_set(&tmpvar, dst_var_ref);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_print_char(B1_T_CHAR c)
{
	B1_T_ERROR err;

	err = b1_ex_io_print_char(c);
	if(err != B1_RES_OK)
	{
		return err;
	}

	b1_int_print_curr_pos++;
	if(b1_int_print_curr_pos == b1_int_print_margin)
	{
		err = b1_ex_io_print_margin_newline();
		if(err != B1_RES_OK)
		{
			return err;
		}
		b1_int_print_curr_pos = 0;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_print_newline()
{
	B1_T_ERROR err;

	err = b1_ex_io_print_newline();
	if(err != B1_RES_OK)
	{
		return err;
	}
	b1_int_print_curr_pos = 0;

	return B1_RES_OK;
}

// prints a string, if sdata is NULL the function prints N spaces where N = slen
static B1_T_ERROR b1_int_print_str(const B1_T_CHAR *sdata, B1_T_INDEX slen)
{
	B1_T_ERROR err;
	B1_T_CHAR c;

	for(; slen; slen--)
	{
		c = (sdata == NULL) ? B1_T_C_SPACE : (*sdata++);

		err = b1_int_print_char(c);
		if(err != B1_RES_OK)
		{
			return err;
		}
	}

	return B1_RES_OK;
}

// interpret PRINT statement
static B1_T_ERROR b1_int_st_print(B1_T_INDEX offset)
{
	B1_T_ERROR err;
	uint8_t numeric, next_print_zone, newline;
	B1_T_INDEX len;
	B1_T_CHAR *sdata;

	next_print_zone = 0;
	newline = 1;

	// process PRINT statement arguments
	for(; ; offset++)
	{
		// build RPN for every PRINT statement argument
		err = b1_rpn_build(offset, PRINT_STOP_TOKENS, &offset);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(next_print_zone)
		{
			// go to the next print zone
			// calculate next print zone number (zero based)
			next_print_zone = b1_int_print_curr_pos / b1_int_print_zone_width;
			len = next_print_zone * b1_int_print_zone_width;
			if(len != b1_int_print_curr_pos)
			{
				len += b1_int_print_zone_width;
				next_print_zone++;
			}

			if(next_print_zone >= b1_int_print_zone_num)
			{
				err = b1_int_print_newline();
				if(err != B1_RES_OK)
				{
					return err;
				}
			}
			else
			{
				if(b1_int_print_curr_pos < len)
				{
					len -= b1_int_print_curr_pos;
					err = b1_int_print_str(NULL, len);
					if(err != B1_RES_OK)
					{
						return err;
					}
				}
			}
		}

		// no RPN: empty expression was specified to b1_rpn_build function
		if(b1_rpn[0].flags == 0)
		{
			break;
		}

		newline = 1;

		// get next print zone flag
		next_print_zone = offset && B1_T_ISCOMMA(b1_progline[offset]);

		err = b1_eval(B1_EVAL_OPT_PRINT_FUNCTIONS, NULL);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// if the subexpression evaluation result is not NULL value print it
		if(!B1_TYPE_TEST_NULL(b1_rpn_eval[0].type))
		{
			sdata = NULL;

			numeric = B1_TYPE_TEST_NUMERIC(b1_rpn_eval[0].type);

			if(B1_TYPE_TEST_TAB_FN(b1_rpn_eval[0].type))
			{
				len = (uint8_t)b1_rpn_eval[0].value.i32val;
				// 4005
				if(len < 1) len = 1;

				len--;

				if(len > b1_int_print_margin)
				{
					len %= b1_int_print_margin;
				}

				if(b1_int_print_curr_pos > len)
				{
					err = b1_int_print_newline();
					if(err != B1_RES_OK)
					{
						return err;
					}
				}
				else
				{
					len -= b1_int_print_curr_pos;
				}
			}
			else
			if(B1_TYPE_TEST_SPC_FN(b1_rpn_eval[0].type))
			{
				len = (uint8_t)b1_rpn_eval[0].value.i32val;
			}
			else
			{
				// convert value to string
				err = b1_var_convert(b1_rpn_eval, B1_TYPE_STRING);
				if(err != B1_RES_OK)
				{
					return err;
				}

				// b1_var_var2str function frees memory used by string after copying it to output buffer
				err = b1_var_var2str(b1_rpn_eval, b1_tmp_buf);
				if(err != B1_RES_OK)
				{
					return err;
				}

				len = *b1_tmp_buf;
				sdata = b1_tmp_buf + 1;
			}

			if(numeric)
			{
				// add leading space if necessary
				if(!B1_T_ISMINUS(*sdata))
				{
					sdata--;
					*sdata = B1_T_C_SPACE;
					len++;
				}
				// add trailing space
				*(sdata + len) = B1_T_C_SPACE;
				len++;
			}

			if(len > b1_int_print_margin - b1_int_print_curr_pos)
			{
				err = b1_int_print_newline();
				if(err != B1_RES_OK)
				{
					return err;
				}
			}

			// print value
			err = b1_int_print_str(sdata, len);
			if(err != B1_RES_OK)
			{
				return err;
			}
		}

		if(!offset) break;
		newline = 0;
	}

	if(newline)
	{
		err = b1_int_print_newline();
		if(err != B1_RES_OK)
		{
			return err;
		}
	}

	return B1_RES_OK;
}

// the function reads user input and returns it as C string (terminated with zero character)
static B1_T_ERROR b1_int_input_line(B1_T_CHAR *sbuf, B1_T_INDEX buflen)
{
	B1_T_ERROR err;
	B1_T_CHAR c;
	B1_T_INDEX i;

	err = B1_RES_OK;

	for(i = 0; ; i++)
	{
		if(i == buflen)
		{
			return B1_RES_EBUFSMALL;
		}

		err = b1_ex_io_input_char(&c);
		if(err != B1_RES_OK || c == B1_T_C_LF)
		{
			break;
		}

		if(b1_int_input_echo)
		{
			err = b1_int_print_char(c);
			if(err != B1_RES_OK)
			{
				break;
			}
		}

		*(sbuf + i) = c;
	}

	*(sbuf + i) = 0;

	if(err == B1_RES_EEOF && i != 0)
	{
		err = B1_RES_OK;
	}

	if(err == B1_RES_OK)
	{
		if(b1_int_input_echo)
		{
			err = b1_int_print_newline();
		}

		b1_int_print_curr_pos = 0;
	}

	return err;
}

// reads the next value from (b1_progline + *offset) to b1_tmp_buf
static B1_T_ERROR b1_int_input_read_get_value(uint8_t allow_comments, B1_T_INDEX *offset, uint8_t *options)
{
	B1_T_ERROR err;
	B1_T_INDEX i, j;
	B1_T_CHAR c;
	B1_TOKENDATA td;
	uint8_t tokenfound;

	tokenfound = 0;
	*options = 0;

	err = b1_tok_get(*offset, B1_TOK_COPY_VALUE | B1_TOK_ALLOW_UNARY_OPER, &td);

	if(err == B1_RES_OK && td.length != 0 && (td.type & (B1_TOKEN_TYPE_QUOTEDSTR | B1_TOKEN_TYPE_NUMERIC)))
	{
		*options = (td.type & B1_TOKEN_TYPE_QUOTEDSTR) ? B1_INT_READ_VALUE_QSTRING : B1_INT_READ_VALUE_NUMERIC;
		*offset = td.offset + td.length;
		tokenfound++;
	}
	else
	{
		*b1_tmp_buf = 0;
	}

	for(i = 0, j = *b1_tmp_buf + 1; ; (*offset)++)
	{
		c = *(b1_progline + *offset);

		if(i == 0 && B1_T_ISBLANK(c))
		{
			if(tokenfound)
			{
				*(b1_tmp_buf + j++) = c;
			}

			continue;
		}

		if(i == 0 && B1_T_ISDBLQUOTE(c) && !(*options & B1_INT_READ_VALUE_QSTRING))
		{
			return B1_RES_ESYNTAX;
		}
		
		if(B1_T_ISCOMMA(c) || B1_T_ISCSTRTERM(c) || B1_T_ISCRLF(c) || (allow_comments && c == B1_T_C_APOSTROPHE))
		{
			if(!tokenfound)
			{
				j--;
				*b1_tmp_buf = (B1_T_CHAR)j;
			}

			if(B1_T_ISCOMMA(c))
			{
				(*offset)++;
			}
			else
			{
				*options |= B1_INT_READ_VALUE_LAST;
			}

			break;
		}

		i++;

		if(*options & B1_INT_READ_VALUE_QSTRING)
		{
			//repeat input
			return B1_RES_ESYNTAX;
		}
		else
		{
			*options = 0;
			tokenfound = 0;
			*(b1_tmp_buf + j++) = c;
		}
	}

	return B1_RES_OK;
}


static B1_T_ERROR b1_int_input_read_assign_value(uint8_t options, const B1_VAR_REF *var_ref, uint8_t *repeat_input)
{
	B1_T_ERROR err;
	uint8_t type;
	B1_T_INDEX len;
	B1_T_CHAR c;

	if(repeat_input != NULL) *repeat_input = 1;

	// trim trailing spaces because b1_int_input_read_get_value function can preserve them when reading values
	len = (B1_T_INDEX)*b1_tmp_buf;
	if(!(options & (B1_INT_READ_VALUE_QSTRING | B1_INT_READ_VALUE_NUMERIC)))
	{
		for(; ; len--)
		{
			c = b1_tmp_buf[len];
			if(!B1_T_ISBLANK(c) || len == 0)
			{
				*b1_tmp_buf = (B1_T_CHAR)len;
				break;
			}
		}
	}

	type = B1_TYPE_GET((*(*var_ref).var).var.type);
				
	if(B1_TYPE_TEST_NUMERIC(type))
	{
		if(!(options & B1_INT_READ_VALUE_NUMERIC))
		{
			return B1_RES_ESYNTAX;
		}

		err = b1_eval_get_numeric_value(b1_rpn_eval);
		if(err != B1_RES_OK)
		{
			// probably invalid numeric value, repeat input
			return err;
		}
	}
	else
	if(type == B1_TYPE_STRING)
	{
		err = b1_var_str2var(b1_tmp_buf, b1_rpn_eval);
		if(err != B1_RES_OK)
		{
			// don't repeat input, it should be no memory error
			if(repeat_input != NULL) *repeat_input = 0;
			return err;
		}
	}

	err = b1_var_convert(b1_rpn_eval, type);
	if(err != B1_RES_OK)
	{
		// in theory, the only possible error here is absence of memory
		if(repeat_input != NULL) *repeat_input = 0;
		return err;
	}

	// assign input value to variable
	// note: b1_var_set function frees memory used by destination variable before assigning new value
	err = b1_var_set(b1_rpn_eval, var_ref);
	if(err != B1_RES_OK)
	{
		// possibly it's memory error
		if(repeat_input != NULL) *repeat_input = 0;
		return err;
	}

	return B1_RES_OK;
}

// interpret INPUT statement: INPUT ["prompt",] var1, var2..., varN
static B1_T_ERROR b1_int_st_input(B1_T_INDEX offset)
{
	B1_T_ERROR err;
	uint8_t repeat_input, prompt, options;
	B1_T_INDEX init_offset, values_offset;
	const B1_T_CHAR *s;
	B1_VAR_REF var_ref;

	init_offset = offset;

	// input line repeat loop
	while(1)
	{
		offset = init_offset;
		prompt = B1_INT_ST_INPUT_PROMPT_PRINT;

		// process INPUT statement arguments
		for(; ; offset++)
		{
			// build RPN for every INPUT statement argument
			err = b1_rpn_build(offset, INPUT_STOP_TOKEN, &offset);
			if(err != B1_RES_OK)
			{
				return err;
			}
			if(b1_rpn[0].flags == 0)
			{
				// INPUT statement without arguments
				return B1_RES_ESYNTAX;
			}

			if(prompt == B1_INT_ST_INPUT_PROMPT_PRINT)
			{
				prompt = B1_INT_ST_INPUT_PROMPT_PRINTED;

				s = _PROMPT;

				// test for optional prompt string (single immediate string value in b1_rpn_build output)
				if(B1_RPNREC_TEST_TYPES(b1_rpn[0].flags, B1_RPNREC_TYPE_IMM_VALUE) && b1_rpn[1].flags == 0)
				{
					err = b1_eval(0, NULL);
					if(err != B1_RES_OK)
					{
						return err;
					}

					if(B1_TYPE_TEST_STRING(b1_rpn_eval[0].type))
					{
						// get custom prompt
						err = b1_var_var2str(b1_rpn_eval, b1_tmp_buf);
						if(err != B1_RES_OK)
						{
							return err;
						}

						s = b1_tmp_buf;

						prompt = B1_INT_ST_INPUT_PROMPT_CUSTOM;
					}
					else
					{
						// non-string constant or absent argument
						return B1_RES_ESYNTAX;
					}
				}

				// print prompt
				err = b1_int_print_str(s + 1, *s);
				if(err != B1_RES_OK)
				{
					return err;
				}

				// get user input
				values_offset = 0;
				err = b1_int_input_line(b1_tmp_buf1, B1_TMP_BUF_LEN);
				if(err != B1_RES_OK)
				{
					return err;
				}
			}

			options = 0;

			if(prompt != B1_INT_ST_INPUT_PROMPT_CUSTOM)
			{
				// evaluate the expression part
				err = b1_eval(0, &var_ref);
				if(err != B1_RES_OK)
				{
					return err;
				}

				// get next value
				repeat_input = 0;
				s = b1_progline;
				b1_progline = b1_tmp_buf1;
				err = b1_int_input_read_get_value(0, &values_offset, &options);
				b1_progline = s;

				if(err != B1_RES_OK)
				{
					repeat_input++;
					break;
				}

				err = b1_int_input_read_assign_value(options, &var_ref, &repeat_input);
				if(err != B1_RES_OK)
				{
					break;
				}
			}

			prompt = B1_INT_ST_INPUT_PROMPT_PRINTED;

			repeat_input = 1;

			if(options & B1_INT_READ_VALUE_LAST)
			{
				if(offset == 0)
				{
					repeat_input = 0;
				}

				break;
			}
			else
			{
				if(offset == 0)
				{
					break;
				}
			}
		}

		if(!repeat_input) break;
	}

	return err;
}

#ifdef B1_FEATURE_STMT_DATA_READ
static B1_T_ERROR b1_int_read_next_field(uint8_t *options)
{
	B1_T_ERROR err, err1;
	B1_T_PROG_LINE_CNT read_line_cnt;

	read_line_cnt = b1_curr_prog_line_cnt;

	// b1_data_curr_line_cnt == 0 stands for the initial state, b1_data_curr_line_offset == 0 means the need to
	// go to the next DATA statement line
	if(b1_data_curr_line_cnt == 0 || b1_data_curr_line_offset == 0)
	{
		err = b1_ex_prg_data_go_next(b1_data_curr_line_cnt == 0 ? B1_T_LINE_NUM_FIRST : B1_T_LINE_NUM_NEXT);
		if(err != B1_RES_OK)
		{
			return err;
		}
	}

	// switch to DATA line
	b1_curr_prog_line_cnt = b1_data_curr_line_cnt - 1;
	err = b1_ex_prg_get_prog_line(B1_T_LINE_NUM_NEXT);
	if(err != B1_RES_OK)
	{
		return err;
	}

	err1 = b1_int_input_read_get_value(1, &b1_data_curr_line_offset, options);
	if(err1 == B1_RES_OK && ((*options) & B1_INT_READ_VALUE_LAST))
	{
		// go to the next DATA statement line when reading the next value
		b1_data_curr_line_offset = 0;
	}

	// restore READ line
	b1_curr_prog_line_cnt = read_line_cnt - 1;
	err = b1_ex_prg_get_prog_line(B1_T_LINE_NUM_NEXT);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return err1;
}

static B1_T_ERROR b1_int_st_read(B1_T_INDEX offset)
{
	B1_T_ERROR err;
	uint8_t options;
	B1_VAR_REF var_ref;

	// process READ statement arguments
	for(; ; offset++)
	{
		// build RPN for every READ statement argument
		err = b1_rpn_build(offset, INPUT_STOP_TOKEN, &offset);
		if(err != B1_RES_OK)
		{
			return err;
		}
		if(b1_rpn[0].flags == 0)
		{
			// READ statement without arguments
			return B1_RES_ESYNTAX;
		}

		// evaluate the expression part
		err = b1_eval(0, &var_ref);
		if(err != B1_RES_OK)
		{
			return err;
		}

		options = 0;
		// read value
		err = b1_int_read_next_field(&options);
		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_int_input_read_assign_value(options, &var_ref, NULL);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(!offset) break;
	}

	return B1_RES_OK;
}

// RESTORE statement: sets next DATA stamtement line counter on requested
// line number, returns the line number in b1_next_line_num variable
static B1_T_ERROR b1_int_st_restore(B1_T_INDEX offset)
{
	B1_T_ERROR err;
	B1_TOKENDATA td;

	// get restore line number
	err = b1_tok_get_line_num(&offset);
	if(err != B1_RES_OK)
	{
		return err;
	}

	err = b1_tok_get(offset, 0, &td);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(td.length != 0)
	{
		return B1_RES_ESYNTAX;
	}

	return b1_ex_prg_data_go_next(b1_next_line_num == B1_T_LINE_NUM_ABSENT ? B1_T_LINE_NUM_FIRST : b1_next_line_num);
}
#endif

static B1_T_ERROR b1_int_st_dim_get_size(B1_T_INDEX *offset, uint8_t allow_TO_stop_word, B1_T_SUBSCRIPT *size)
{
	B1_T_ERROR err;

	// "TO" stop token must be the first in DIM_STOP_TOKENS array
	err = b1_rpn_build(*offset, allow_TO_stop_word ? DIM_STOP_TOKENS : (DIM_STOP_TOKENS + 1), offset);
	if(err != B1_RES_OK) return err;
	if(!offset || b1_rpn[0].flags == 0)
	{
		return B1_RES_ESYNTAX;
	}

	err = b1_eval(0, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	err = b1_var_convert(b1_rpn_eval, B1_TYPE_INT);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(b1_rpn_eval[0].value.i32val < B1_T_SUBSCRIPT_MIN_VALUE || b1_rpn_eval[0].value.i32val > B1_T_SUBSCRIPT_MAX_VALUE)
	{
		return B1_RES_ESUBSRANGE;
	}

	*size = (B1_T_SUBSCRIPT)b1_rpn_eval[0].value.i32val;

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_st_dim(B1_T_INDEX offset)
{
	B1_T_ERROR err;
	B1_T_CHAR c, ts_char;
	uint8_t stop, type, dimsnum;
	B1_TOKENDATA td;
	B1_T_INDEX len;
	B1_T_SUBSCRIPT bound1, subs_bounds[B1_MAX_VAR_DIM_NUM * 2], *subs_bnds_ptr;
	B1_T_IDHASH hash;
#ifdef B1_FEATURE_DEBUG
	B1_T_INDEX id_off, id_len;
	B1_NAMED_VAR *var;
#endif

	while(1)
	{
		// get variable name
		err = b1_tok_get(offset, B1_TOK_CALC_HASH, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(!(td.type & B1_TOKEN_TYPE_IDNAME))
		{
			return B1_RES_EINVTOK;
		}

		offset = td.offset;
		len = td.length;

#ifdef B1_FEATURE_DEBUG
		id_off = offset;
		id_len = len;
#endif
		offset += len;
		type = B1_TYPE_NULL;
		ts_char = b1_progline[offset - 1];

		dimsnum = 0;
		subs_bnds_ptr = subs_bounds;

		hash = b1_tok_id_hash;

		// DIM <var_name>[(subscript[,...])] [AS <type_name>][,...]
		err = b1_tok_get(offset, 0, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		offset = td.offset;
		len = td.length;

		if(len == 1 && b1_progline[offset] == B1_T_C_OPBRACK)
		{
			offset++;
			// get array dimensions
			while(1)
			{
				if(dimsnum == B1_MAX_VAR_DIM_NUM)
				{
					return B1_RES_EWSUBSCNT;
				}

				// get either dimension size or lbound
				err = b1_int_st_dim_get_size(&offset, 1, &bound1);
				if(err != B1_RES_OK)
				{
					return err;
				}

				// TO, comma or close bracket
				c = b1_progline[offset];

				if(c == B1_T_C_CLBRACK || B1_T_ISCOMMA(c))
				{
					*subs_bnds_ptr++ = b1_opt_base_val;
					*subs_bnds_ptr++ = bound1;
					dimsnum++;

					offset++;

					if(B1_T_ISCOMMA(c))
					{
						continue;
					}
					else
					{
						break;
					}
				}
				else
				{
					// get ubound
					offset += 2;
					*subs_bnds_ptr++ = bound1;
					err = b1_int_st_dim_get_size(&offset, 0, subs_bnds_ptr++);
					if(err != B1_RES_OK)
					{
						return err;
					}
					dimsnum++;

					c = b1_progline[offset];
					offset++;

					if(B1_T_ISCOMMA(c))
					{
						continue;
					}

					if(c == B1_T_C_CLBRACK)
					{
						break;
					}
				}
			}

			err = b1_tok_get(offset, 0, &td);
			if(err != B1_RES_OK)
			{
				return err;
			}

			offset = td.offset;
			len = td.length;
		}

		if(len == 0)
		{
			stop = 1;
		}
		else
		if(len == 1 && B1_T_ISCOMMA(b1_progline[offset]))
		{
			// continue parsing string
			stop = 0;
		}
		else
		{
			// try to get optional variable type
			if(!b1_t_strcmpi(_AS, b1_progline + offset, len))
			{
				err = b1_tok_get(offset + len, 0, &td);
				if(err != B1_RES_OK)
				{
					return err;
				}
				offset = td.offset;
				len = td.length;

				// get variable type
				err = b1_t_get_type_by_name(b1_progline + offset, len, &type);
				if(err != B1_RES_OK)
				{
					return err;
				}

				err = b1_tok_get(offset + len, 0, &td);
				if(err != B1_RES_OK)
				{
					return err;
				}

				offset = td.offset;
				len = td.length;

				if(len == 0)
				{
					stop = 1;
				}
				else
				if(len == 1 && B1_T_ISCOMMA(b1_progline[offset]))
				{
					// continue parsing string
					stop = 0;
				}
				else
				{
					return B1_RES_ESYNTAX;
				}
			}
			else
			{
				return B1_RES_ESYNTAX;
			}
		}

		err = b1_t_get_type_by_type_spec(ts_char, type, &type);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// create variable
#ifdef B1_FEATURE_DEBUG
		err = b1_var_create(hash, type, dimsnum, subs_bounds, &var);
		if(err != B1_RES_OK)
		{
			return err;
		}

		memcpy((*var).id.name + 1, b1_progline + id_off, id_len * B1_T_CHAR_SIZE);
		(*var).id.name[0] = (B1_T_CHAR)id_len;
#else
		err = b1_var_create(hash, type, dimsnum, subs_bounds, NULL);
		if(err != B1_RES_OK)
		{
			return err;
		}
#endif

		offset += len;
		if(stop) break;
	}

	return B1_RES_OK;
}

#if defined(B1_FEATURE_STMT_ERASE) || defined(B1_FEATURE_INIT_FREE_MEMORY)
B1_T_ERROR b1_int_var_mem_free(B1_NAMED_VAR *var)
{
	B1_T_ERROR err;
	uint8_t type, dimsnum;
	B1_T_MEMOFFSET size, mem_size;
	B1_T_MEM_BLOCK_DESC desc, arrdesc;
	const B1_T_MEM_BLOCK_DESC *arrdata;
	const B1_T_SUBSCRIPT *arrdescdata;
	B1_T_SUBSCRIPT sub;
	B1_T_INDEX i;

	// the variable exists, free it
	type = (*var).var.type;
	desc = (*var).var.value.mem_desc;
	dimsnum = B1_IDENT_GET_FLAGS_ARGNUM((*var).id.flags);

	if(dimsnum == 0)
	{
		// free string data
		if(type == B1_TYPE_SET(B1_TYPE_STRING, 0))
		{
			b1_ex_mem_free(desc);
		}
	}
	else
	{
		// free array data
#ifdef B1_FEATURE_INIT_FREE_MEMORY
		if(desc != B1_T_MEM_BLOCK_DESC_INVALID)
		{
#endif
			err = b1_ex_mem_access(desc, 0, 0, B1_EX_MEM_READ, (void **)&arrdescdata);
			if(err != B1_RES_OK)
			{
				return err;
			}

			// calc array size
			size = 1;
			while(dimsnum > 0)
			{
				dimsnum--;
				sub = *arrdescdata++;
				sub = *arrdescdata++ - sub;
				size *= ((B1_T_MEMOFFSET)sub) + 1;
			}
			arrdesc = *((B1_T_MEM_BLOCK_DESC *)arrdescdata);

			if(arrdesc != B1_T_MEM_BLOCK_DESC_INVALID)
			{
				// free array string data
				if(B1_TYPE_TEST_STRING(type))
				{
					mem_size = size * (uint8_t)sizeof(B1_T_MEM_BLOCK_DESC);

					for(size = 0, i = 0; size != mem_size; size += (uint8_t)sizeof(B1_T_MEM_BLOCK_DESC), i++)
					{
						if(i == ((B1_T_INDEX)(B1_MAX_STRING_LEN + 1)) / (uint8_t)sizeof(B1_T_MEM_BLOCK_DESC))
						{
							i = 0;
						}

						if(i == 0)
						{
							err = b1_ex_mem_access(arrdesc, size, (((B1_T_INDEX)(B1_MAX_STRING_LEN + 1)) / (uint8_t)sizeof(B1_T_MEM_BLOCK_DESC)) * (uint8_t)sizeof(B1_T_MEM_BLOCK_DESC), B1_EX_MEM_WRITE, (void **)&arrdata);
							if(err != B1_RES_OK)
							{
								return err;
							}
						}

						if(*(arrdata + i) != B1_T_MEM_BLOCK_DESC_INVALID)
						{
							b1_ex_mem_free(*(arrdata + i));
						}
					}
				}

				// free array data
				b1_ex_mem_free(arrdesc);
			}

			// free array descriptor data
			b1_ex_mem_free(desc);
#ifdef B1_FEATURE_INIT_FREE_MEMORY
		}
#endif
	}

	return B1_RES_OK;
}
#endif

#ifdef B1_FEATURE_STMT_ERASE
static B1_T_ERROR b1_int_st_erase(B1_T_INDEX offset)
{
	B1_T_ERROR err;
	uint8_t next, type;
	B1_TOKENDATA td;
	B1_T_INDEX len;
	B1_NAMED_VAR *var;

	while(1)
	{
		// get variable name
		err = b1_tok_get(offset, B1_TOK_CALC_HASH, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		type = td.type;
		offset = td.offset;
		len = td.length;
		if(len == 0)
		{
			return B1_RES_ESYNTAX;
		}

		if(!(type & B1_TOKEN_TYPE_IDNAME))
		{
			return B1_RES_EINVTOK;
		}

		offset += len;

		err = b1_tok_get(offset, 0, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		offset = td.offset;
		len = td.length;
		next = (len == 1) && B1_T_ISCOMMA(*(b1_progline + offset));
		if(!next && len != 0)
		{
			return B1_RES_ESYNTAX;
		}
		offset++;

		err = b1_ex_var_alloc(b1_tok_id_hash, &var);
		if(err != B1_RES_OK && err != B1_RES_EIDINUSE)
		{
			return err;
		}

		err = b1_int_var_mem_free(var);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// release variable memory
		b1_ex_var_free(b1_tok_id_hash);

		if(!next) break;
	}

	return B1_RES_OK;
}
#endif

// linen_index = 0 for simple GOTO statement, linen_index != 0 for ON ... GOTO statement
// sets b1_next_line_num to line number to move control to
static B1_T_ERROR b1_int_st_go(B1_T_INDEX offset, uint8_t linen_index)
{
	B1_T_ERROR err;
	B1_TOKENDATA td;
	B1_T_INDEX len;
	uint8_t	i;

	// get line number
	i = 1;

	while(1)
	{
		err = b1_tok_get_line_num(&offset);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(b1_next_line_num == B1_T_LINE_NUM_ABSENT)
		{
			return B1_RES_ESYNTAX;
		}

		if(i == linen_index)
		{
			break;
		}

		err = b1_tok_get(offset, 0, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		offset = td.offset;
		len = td.length;

		if(!linen_index)
		{
			if(len)
			{
				return B1_RES_ESYNTAX;
			}

			break;
		}

		if(!(td.type & B1_TOKEN_TYPE_OPERATION) && len != 1 && !B1_T_ISCOMMA(b1_progline[offset]))
		{
			return B1_RES_ESYNTAX;
		}

		offset += len;
		i++;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_st_if(B1_T_INDEX offset)
{
	B1_T_ERROR err;

	// build RPN for the IF statement condition
	err = b1_rpn_build(offset, IF_STOP_TOKENS, &offset);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// no THEN clause found
	if(!offset)
	{
		return B1_RES_ESYNTAX;
	}

	// evaluate the condition
	err = b1_eval(0, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(!B1_TYPE_TEST_BOOL(b1_rpn_eval[0].type))
	{
		return B1_RES_ETYPMISM;
	}

	// only one stop token for IF statement ("THEN")
	if(b1_rpn_eval[0].value.bval)
	{
		// add "THEN" keyword length
		b1_curr_prog_line_offset = offset + (B1_T_INDEX)4;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_st_on(B1_T_INDEX offset, uint8_t *result)
{
	B1_T_ERROR err;

	err = b1_rpn_build(offset, ON_STOP_TOKENS, &b1_curr_prog_line_offset);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// no GOTO (GOSUB) clause found
	if(b1_curr_prog_line_offset == 0)
	{
		return B1_RES_ESYNTAX;
	}

	// evaluate the condition
	err = b1_eval(0, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	err = b1_var_convert(b1_rpn_eval, B1_TYPE_INT);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(b1_rpn_eval[0].value.i32val > 0)
		*result = (uint8_t)b1_rpn_eval[0].value.i32val;
	else
		return B1_RES_EINVARG;

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_save_stmt_state(uint8_t new_state)
{
	B1_INT_STMT_STK_REC *stk_rec;

	if(b1_int_stmt_stack_ptr == B1_MAX_STMT_NEST_DEPTH)
	{
		return B1_RES_ESTSTKOVF;
	}

	stk_rec = b1_int_stmt_stack + b1_int_stmt_stack_ptr;
	b1_int_stmt_stack_ptr++;

	(*stk_rec).state = b1_int_curr_stmt_state;
	(*stk_rec).ret_line_cnt = b1_curr_prog_line_cnt;

	b1_int_curr_stmt_state = new_state;

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_restore_stmt_state()
{
	if(b1_int_stmt_stack_ptr == 0)
	{
		return B1_RES_ESTSTKUDF;
	}

	b1_int_stmt_stack_ptr--;

	// do no restore program line for IF statement
	if(!(b1_int_curr_stmt_state & B1_INT_STATE_IF))
	{
		b1_curr_prog_line_cnt = (*(b1_int_stmt_stack + b1_int_stmt_stack_ptr)).ret_line_cnt;
	}

	b1_int_curr_stmt_state = (*(b1_int_stmt_stack + b1_int_stmt_stack_ptr)).state;

	return B1_RES_OK;
}

// options = 1 stands for ON/OFF value represented with 0/1/2 values: 0 = OFF, 1 = ON, 2 = <no_value>
static B1_T_ERROR b1_int_st_option_set(B1_T_INDEX *offset, const B1_T_CHAR *s, uint8_t value_type, uint8_t options, uint8_t *value)
{
	B1_T_ERROR err;
	B1_TOKENDATA td;
	B1_T_INDEX len;
	uint16_t val;

	err = b1_tok_get(*offset, 0, &td);
	if(err != B1_RES_OK)
	{
		return err;
	}

	len = td.length;

	if(!b1_t_strcmpi(s, b1_progline + td.offset, len))
	{
		err = b1_tok_get(td.offset + len, B1_TOK_COPY_VALUE, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		len = td.length;
		*offset = td.offset;

		if(len != 0 && !(td.type & value_type))
		{
			return B1_RES_EINVARG;
		}

		if(value_type == B1_TOKEN_TYPE_NUMERIC)
		{
			if(len == 0 || len > 3)
			{
				return B1_RES_EINVARG;
			}

			b1_tmp_buf[len + 1] = 0;
			err = b1_t_strtoui16(b1_tmp_buf + 1, &val);
			if(err != B1_RES_OK)
			{
				return err;
			}

			if(val > UINT8_MAX)
			{
				return B1_RES_EINVARG;
			}

			*value = (uint8_t)val;
		}
		else
		if(value_type == B1_TOKEN_TYPE_IDNAME)
		{
			if(options & 0x1)
			{
				if(len == 0)
				{
					*value = 2;
				}
				else
				if(!b1_t_strcmpi(_ON, b1_tmp_buf + 1, len))
				{
					*value = 1;
				}
				else
				if(!b1_t_strcmpi(_OFF, b1_tmp_buf + 1, len))
				{
					*value = 0;
				}
				else
				{
					return B1_RES_EINVARG;
				}
			}
			else
			{
				*value = (uint8_t)len;
			}
		}
		else
		{
			return B1_RES_EINVARG;
		}
	}
	else
	{
		return B1_RES_EEOF;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_st_option(B1_T_INDEX offset)
{
	B1_T_ERROR err;
	uint8_t value;

	err = b1_int_st_option_set(&offset, _BASE, B1_TOKEN_TYPE_NUMERIC, 0, &value);
	if(err == B1_RES_OK)
	{
		if(value == 0)
		{
			b1_opt_base_val = 0;
		}
		else
		if(value == 1)
		{
			b1_opt_base_val = 1;
		}
		else
		{
			return B1_RES_EINVARG;
		}

		return B1_RES_OK;
	}
	else
	if(err != B1_RES_EEOF)
	{
		return err;
	}

	err = b1_int_st_option_set(&offset, _EXPLICIT, B1_TOKEN_TYPE_IDNAME, 1, &value);
	if(err == B1_RES_OK)
	{
		// omitted value corresponds to ON
		if(value == 0)
		{
			b1_opt_explicit_val = 0;
		}
		else
		if(value <= 2)
		{
			b1_opt_explicit_val = 1;
		}
		else
		{
			return B1_RES_EINVARG;
		}

		return B1_RES_OK;
	}
	else
	if(err == B1_RES_EEOF)
	{
		return B1_RES_EINVSTAT;
	}

	return B1_RES_OK;
}

#define B1_INT_ST_FOR_INTVAR_CREATE 0x0
#define B1_INT_ST_FOR_INTVAR_ACCESS 0x1
#define B1_INT_ST_FOR_INTVAR_FREE 0x2

static B1_T_ERROR b1_int_st_for_get_intvar(uint8_t var_hash_base, uint8_t var_op, uint8_t var_type, B1_VAR_REF *var_ref)
{
	B1_T_ERROR err;
	B1_NAMED_VAR *var;

	var_hash_base += b1_int_stmt_stack_ptr - 1;

	err = b1_var_create(var_hash_base, var_type, 0, NULL, &var);

	if(var_op == B1_INT_ST_FOR_INTVAR_CREATE)
	{
		if(err != B1_RES_OK)
		{
			return err;
		}

#ifdef B1_FEATURE_DEBUG
		memcpy((*var).id.name, _DBG_FORVAR, (((B1_T_INDEX)_DBG_FORVAR[0]) + 1) * B1_T_CHAR_SIZE);
#endif

		(*var_ref).var = var;

		return b1_var_set(b1_rpn_eval, var_ref);
	}

	if(err != B1_RES_EIDINUSE)
	{
		return err;
	}

	if(var_op == B1_INT_ST_FOR_INTVAR_ACCESS)
	{
		b1_rpn_eval[1] = (*var).var;
	}
	else
	if(var_op == B1_INT_ST_FOR_INTVAR_FREE)
	{
		b1_ex_var_free(var_hash_base);
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_st_for_start(B1_T_INDEX offset)
{
	//LET own1 = limit
	//LET own2 = increment
	//LET v = initial-value

	B1_T_ERROR err;
	uint8_t type;
	B1_VAR_REF var_ref;

	// get initial value of loop control variable
	err = b1_int_st_let(offset, &var_ref, FOR_STOP_TOKEN1, &offset);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// no "TO" keyword
	if(offset == 0)
	{
		return B1_RES_ESYNTAX;
	}

	(*(b1_int_stmt_stack + b1_int_stmt_stack_ptr - 1)).var = var_ref.var;

	// save control variable name hash and forbid using arrays and non-numeric variables as loop control variable
	if(B1_IDENT_GET_FLAGS_ARGNUM((*var_ref.var).id.flags) != 0)
	{
		return B1_RES_EFORSUBSVAR;
	}

	type = B1_TYPE_GET((*var_ref.var).var.type);
	
	if(!B1_TYPE_TEST_NUMERIC(type))
	{
		return B1_RES_ETYPMISM;
	}

	// skip "TO" keyword
	offset += 2;

	// build RPN for limit expression
	err = b1_rpn_build(offset, FOR_STOP_TOKEN2, &offset);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// evaluate limit
	err = b1_eval(0, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(!B1_TYPE_TEST_NUMERIC(b1_rpn_eval[0].type))
	{
		return B1_RES_ETYPMISM;
	}

	// create special variable for limit value
	err = b1_int_st_for_get_intvar(0, B1_INT_ST_FOR_INTVAR_CREATE, type, &var_ref);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// check for "STEP" keyword presence
	if(offset != 0)
	{
		// skip "STEP" keyword
		offset += 4;

		// build RPN for step expression
		err = b1_rpn_build(offset, NULL, &offset);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// step value expression should be the last in the line
		if(offset != 0)
		{
			return B1_RES_ESYNTAX;
		}

		// evaluate step value
		err = b1_eval(0, NULL);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// set step negative flag
		b1_rpn_eval[1] = b1_rpn_eval[0];

		err = b1_var_init_empty(B1_TYPE_GET(b1_rpn_eval[0].type), 0, NULL, b1_rpn_eval + 2);
		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_eval_cmp(b1_rpn_eval + 1, B1_T_C_LT, 0, B1_TYPE_GET(b1_rpn_eval[0].type));
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(b1_rpn_eval[1].value.bval)
		{
			b1_int_curr_stmt_state |= B1_INT_STATE_FOR_NEG_STEP;
		}
	}
	else
	{
		// step = 1
		b1_rpn_eval[0].type = type;
#ifdef B1_FEATURE_TYPE_SINGLE
		if(type == B1_TYPE_SINGLE)
			b1_rpn_eval[0].value.sval = 1.0f;
		else
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
		if(type == B1_TYPE_DOUBLE)
			b1_rpn_eval[0].value.dval = 1.0;
		else
#endif
		if(type == B1_TYPE_INT)
			b1_rpn_eval[0].value.i32val = 1;
#ifdef B1_FEATURE_TYPE_SMALL
		else
		if(type == B1_TYPE_INT16)
			b1_rpn_eval[0].value.i16val = 1;
		else
		if(type == B1_TYPE_WORD)
			b1_rpn_eval[0].value.ui16val = 1;
		else
		if(type == B1_TYPE_BYTE)
			b1_rpn_eval[0].value.ui8val = 1;
#endif
	}

	return b1_int_st_for_get_intvar(0x80, B1_INT_ST_FOR_INTVAR_CREATE, type, &var_ref);
}

static B1_T_ERROR b1_int_st_for_end()
{
	// dispose own1 (limit) and own2 (step) variables

	B1_T_ERROR err;

	// free internal loop variables
	err = b1_int_st_for_get_intvar(0, B1_INT_ST_FOR_INTVAR_FREE, 0, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	return b1_int_st_for_get_intvar(0x80, B1_INT_ST_FOR_INTVAR_FREE, 0, NULL);
}

static B1_T_ERROR b1_int_st_for_test()
{
	//IF (v-limit) * SGN (step) > 0 THEN line2

	B1_T_ERROR err;
	uint8_t negstep, stop;
	B1_INT_STMT_STK_REC *stk_rec;

#ifdef B1_FEATURE_TYPE_SMALL
	// extra loop exit condition for unsigned data types
	stop = (b1_int_curr_stmt_state & B1_INT_STATE_FOR_STOP);

	if(!stop)
	{
#endif
		negstep = b1_int_curr_stmt_state & B1_INT_STATE_FOR_NEG_STEP;

		stk_rec = b1_int_stmt_stack + b1_int_stmt_stack_ptr - 1;
		b1_rpn_eval[0] = (*(*stk_rec).var).var;

		// get limit
		err = b1_int_st_for_get_intvar(0, B1_INT_ST_FOR_INTVAR_ACCESS, 0, NULL);
		if (err != B1_RES_OK)
		{
			return err;
		}

		err = b1_eval_cmp(b1_rpn_eval, negstep ? B1_T_C_LT : B1_T_C_GT, 0, b1_rpn_eval[0].type);
		if (err != B1_RES_OK)
		{
			return err;
		}

		stop = b1_rpn_eval[0].value.bval;

#ifdef B1_FEATURE_TYPE_SMALL
	}
#endif

	if(stop)
	{
		err = b1_int_st_for_end();
		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_int_restore_stmt_state();
		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_ex_prg_for_go_next();
		if(err != B1_RES_OK)
		{
			return err;
		}
	}
	else
	{
#ifdef B1_FEATURE_TYPE_SMALL
		// check if the loop control variable has reached its limit value (is necessary for unsigned types)
		b1_rpn_eval[0] = (*(*stk_rec).var).var;

		err = b1_eval_cmp(b1_rpn_eval, B1_T_C_EQ, 0, b1_rpn_eval[0].type);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(b1_rpn_eval[0].value.bval)
		{
			b1_int_curr_stmt_state |= B1_INT_STATE_FOR_STOP;
		}
#endif
		// set program line counter to the first line of the loop body
		b1_curr_prog_line_cnt = (*stk_rec).ret_line_cnt;
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_int_st_for_next(B1_T_INDEX offset)
{
	//LET v = v + own2
	//GOTO line1

	B1_T_ERROR err;
	B1_VAR_REF var_ref;
	B1_TOKENDATA td;
	B1_NAMED_VAR *var;

	var = (*(b1_int_stmt_stack + b1_int_stmt_stack_ptr - 1)).var;

	// check NEXT statement control variable if specified
	if(!(b1_int_curr_stmt_state & B1_INT_STATE_FOR_NEXT_CHECKED))
	{
		b1_int_curr_stmt_state |= B1_INT_STATE_FOR_NEXT_CHECKED;
		
		err = b1_tok_get(offset, B1_TOK_CALC_HASH, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(td.length != 0)
		{
			if(!(td.type & B1_TOKEN_TYPE_IDNAME))
			{
				return B1_RES_EINVTOK;
			}

			if(b1_tok_id_hash != (*var).id.name_hash)
			{
				return B1_RES_ENXTWOFOR;
			}
		}
	}

	err = b1_int_st_for_get_intvar(0x80, B1_INT_ST_FOR_INTVAR_ACCESS, 0, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	b1_rpn_eval[0] = (*var).var;
	err = b1_eval_add(b1_rpn_eval, b1_rpn_eval[0].type);
	if(err != B1_RES_OK)
	{
		return err;
	}

	var_ref.var = var;
	var_ref.val_off = 0;
	err = b1_var_set(b1_rpn_eval, &var_ref);

	return err;
}

#ifdef B1_FEATURE_FUNCTIONS_USER
static B1_T_INDEX b1_int_find_hash(const B1_T_IDHASH *hashes, B1_T_INDEX hash_num, B1_T_IDHASH hash)
{
	B1_T_INDEX i;

	for(i = 0; i < hash_num; i++)
	{
		if(*(hashes + i) == hash)
		{
			break;
		}
	}

	return i;
}

// DEF <fn_name>[(<arg1_name[, arg2_name, ...argN_name]>)] = <expression>
static B1_T_ERROR b1_int_st_def(B1_T_INDEX offset, B1_T_PROG_LINE_CNT def_line_cnt)
{
	B1_T_ERROR err;
	B1_TOKENDATA td;
	B1_T_INDEX i, len;
	B1_T_CHAR c;
	B1_T_IDHASH arg_hashes[B1_MAX_FN_ARGS_NUM];
	uint8_t argnum, tflags;
	B1_UDEF_FN *fn;
	B1_RPNREC *rpnrec;

	// get function name
	err = b1_tok_get(offset, B1_TOK_CALC_HASH, &td);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(!(td.type & B1_TOKEN_TYPE_IDNAME))
	{
		return B1_RES_EINVTOK;
	}

	offset = td.offset;
	len = td.length;

	offset += len;

	// check for existence
	err = b1_fn_get_params(b1_tok_id_hash, 1, (B1_FN **)&fn);
	if(err == B1_RES_OK)
	{
		return B1_RES_EIDINUSE;
	}

	if(err != B1_RES_EUNKIDENT)
	{
		return err;
	}

	// create new user function
	(*fn).fn.id.name_hash = b1_tok_id_hash;
	b1_t_get_type_by_type_spec(b1_progline[offset - 1], B1_TYPE_NULL, &(*fn).fn.ret_type);

	err = b1_tok_get(offset, 0, &td);
	if(err != B1_RES_OK)
	{
		return err;
	}

	offset = td.offset;
	len = td.length;

	if(len != 1 || !(td.type & B1_TOKEN_TYPE_OPERATION))
	{
		return B1_RES_ESYNTAX;
	}

	c = b1_progline[offset];
	offset += len;

	argnum = 0;

	if(c == B1_T_C_OPBRACK)
	{
		// read function arguments
		while(1)
		{
			err = b1_tok_get(offset, B1_TOK_CALC_HASH, &td);
			if(err != B1_RES_OK)
			{
				return err;
			}

			offset = td.offset;
			len = td.length;

			if(!(td.type & B1_TOKEN_TYPE_IDNAME))
			{
				return B1_RES_ESYNTAX;
			}

			if(argnum == B1_MAX_FN_ARGS_NUM)
			{
				return B1_RES_EWRARGCNT;
			}

			// process function argument
			i = b1_int_find_hash(arg_hashes, argnum, b1_tok_id_hash);
			if(i != argnum)
			{
				return B1_RES_EIDINUSE;
			}

			offset += len;
			b1_t_get_type_by_type_spec(b1_progline[offset - 1], B1_TYPE_NULL, &(*fn).fn.argtypes[argnum]);

			arg_hashes[argnum] = b1_tok_id_hash;
			argnum++;

			err = b1_tok_get(offset, 0, &td);
			if(err != B1_RES_OK)
			{
				return err;
			}

			offset = td.offset;
			len = td.length;

			if(len != 1 || !(td.type & B1_TOKEN_TYPE_OPERATION))
			{
				return B1_RES_ESYNTAX;
			}

			c = b1_progline[offset];
			offset += len;

			if(c == B1_T_C_CLBRACK)
			{
				break;
			}

			if(!B1_T_ISCOMMA(c))
			{
				return B1_RES_ESYNTAX;
			}
		}

		err = b1_tok_get(offset, 0, &td);
		if(err != B1_RES_OK)
		{
			return err;
		}

		offset = td.offset;
		len = td.length;

		if(len != 1 || !(td.type & B1_TOKEN_TYPE_OPERATION))
		{
			return B1_RES_ESYNTAX;
		}

		c = b1_progline[offset];
		offset += len;
	}

	if(c != B1_T_C_EQ)
	{
		return B1_RES_ESYNTAX;
	}

	// build function expression RPN
	err = b1_rpn_build(offset, NULL, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// save function RPN
	i = 0;

	(*fn).def_line_cnt = def_line_cnt;
	(*fn).rpn_start_pos = b1_fn_udef_fn_rpn_off;

	while(1)
	{
		tflags = (*(b1_rpn + i)).flags;

		if(tflags == 0)
		{
			if(i == 0)
			{
				return B1_RES_ESYNTAX;
			}

			break;
		}

		if(b1_fn_udef_fn_rpn_off > B1_MAX_UDEF_FN_RPN_LEN - 1)
		{
			return B1_RES_EMANYDEF;
		}

		rpnrec = b1_fn_udef_fn_rpn + b1_fn_udef_fn_rpn_off;

		memcpy(rpnrec, b1_rpn + i, sizeof(B1_RPNREC));

		if(tflags == (B1_RPNREC_TYPE_FNVAR | (uint8_t)(0 << (B1_RPNREC_FN_ARG_INDEX_SHIFT))))
		{
			len = b1_int_find_hash(arg_hashes, argnum, (*rpnrec).data.id.hash);
			if(len != argnum)
			{
				(*rpnrec).flags = B1_RPNREC_TYPE_FN_ARG | (uint8_t)(len << (B1_RPNREC_FN_ARG_INDEX_SHIFT));
			}
		}

		i++;
		b1_fn_udef_fn_rpn_off++;
	}

	(*fn).fn.id.flags = B1_IDENT_FLAGS_SET_FN(argnum, 0);
	(*fn).rpn_len = i;

	return B1_RES_OK;
}
#endif

static B1_T_ERROR b1_int_st_set(B1_T_INDEX offset)
{
	B1_T_ERROR err;
	uint8_t value;

	err = b1_int_st_option_set(&offset, _MARGIN, B1_TOKEN_TYPE_NUMERIC, 0, &value);
	if(err == B1_RES_OK)
	{
		if(value < b1_int_print_zone_width)
		{
			return B1_RES_EINVARG;
		}

		b1_int_print_margin = value;
		b1_int_print_zone_num = b1_int_print_margin / b1_int_print_zone_width;

		return B1_RES_OK;
	}
	else
	if(err != B1_RES_EEOF)
	{
		return err;
	}

	err = b1_int_st_option_set(&offset, _ZONEWIDTH, B1_TOKEN_TYPE_NUMERIC, 0, &value);
	if(err == B1_RES_OK)
	{
		if(value == 0 || value > b1_int_print_margin)
		{
			return B1_RES_EINVARG;
		}

		b1_int_print_zone_width = value;
		b1_int_print_zone_num = b1_int_print_margin / b1_int_print_zone_width;

		return B1_RES_OK;
	}
	else
	if(err != B1_RES_EEOF)
	{
		return err;
	}

	err = b1_int_st_option_set(&offset, _INPUTECHO, B1_TOKEN_TYPE_IDNAME, 1, &value);
	if(err == B1_RES_OK)
	{
		if(value == 0)
		{
			b1_int_input_echo = 0;
		}
		else
		if(value == 1)
		{
			b1_int_input_echo = 1;
		}
		else
		{
			return B1_RES_EINVARG;
		}

		return B1_RES_OK;
	}
	else
	if(err == B1_RES_EEOF)
	{
		return B1_RES_EINVSTAT;
	}

	return B1_RES_OK;
}

#ifdef B1_FEATURE_STMT_WHILE_WEND
static B1_T_ERROR b1_int_st_while(B1_T_INDEX offset)
{
	B1_T_ERROR err;

	// build RPN for WHILE expression
	err = b1_rpn_build(offset, NULL, &offset);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(offset != 0)
	{
		return B1_RES_ESYNTAX;
	}

	// evaluate WHILE expression
	err = b1_eval(0, NULL);
	if(err != B1_RES_OK)
	{
		return err;
	}

	if(!B1_TYPE_TEST_BOOL(b1_rpn_eval[0].type))
	{
		return B1_RES_ETYPMISM;
	}

	return b1_rpn_eval[0].value.bval ? b1_int_save_stmt_state(B1_INT_STATE_WHILE) : b1_ex_prg_while_go_wend();
}
#endif

static B1_T_ERROR b1_int_interpret_stmt(uint8_t stmt)
{
	B1_T_ERROR err;
	B1_T_INDEX offset;
	uint8_t onpos;
	B1_VAR_REF var_ref;

	offset = b1_curr_prog_line_offset;
	b1_curr_prog_line_offset = 0;

	if(b1_next_line_num != B1_T_LINE_NUM_ABSENT)
	{
		// line number is present
		// process statements like "IF ... THEN 10", "ELSEIF ... THEN 20", "ELSE 30"
		if(b1_int_curr_stmt_state == (B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC))
		{
			return b1_int_restore_stmt_state();
		}
	}

	b1_next_line_num = B1_T_LINE_NUM_NEXT;

	if(stmt == B1_ID_STMT_ABSENT)
	{
		// empty string
		return B1_RES_OK;
	}

	// process REM statement
	if(stmt == B1_ID_STMT_REM)
	{
		// comments are not allowed as a part of IF/ELSEIF/ELSE satements
		if(b1_int_curr_stmt_state == (B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC))
		{
			return B1_RES_EINVSTAT;
		}

		return B1_RES_OK;
	}

	if(stmt == B1_ID_STMT_OPTION)
	{
		if(b1_options_allowed)
		{
			return b1_int_st_option(offset);
		}

		return B1_RES_EINVSTAT;
	}

	b1_options_allowed = 0;

	if(stmt == B1_ID_STMT_ELSEIF)
	{
		if(b1_int_curr_stmt_state == (B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC) || !(b1_int_curr_stmt_state & B1_INT_STATE_IF))
		{
			return B1_RES_EELSEWOIF;
		}

		if(b1_int_curr_stmt_state & B1_INT_STATE_IF_SKIP)
		{
			return B1_RES_OK;
		}

		err = b1_int_st_if(offset);
		if(err != B1_RES_OK)
		{
			return err;
		}

		b1_int_curr_stmt_state = b1_curr_prog_line_offset ? (B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC) : (B1_INT_STATE_IF | B1_INT_STATE_IF_NEXT);

		return B1_RES_OK;
	}

	if(stmt == B1_ID_STMT_ELSE)
	{
		if(b1_int_curr_stmt_state == (B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC) || !(b1_int_curr_stmt_state & B1_INT_STATE_IF))
		{
			return B1_RES_EELSEWOIF;
		}

		if(b1_int_curr_stmt_state & B1_INT_STATE_IF_SKIP)
		{
			return B1_RES_OK;
		}

		b1_int_curr_stmt_state = B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC;
		b1_curr_prog_line_offset = offset;

		return B1_RES_OK;
	}

	// any new statement should clear previous IF state
	if(b1_int_curr_stmt_state != (B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC) && (b1_int_curr_stmt_state & B1_INT_STATE_IF))
	{
		err = b1_int_restore_stmt_state();
		if(err != B1_RES_OK)
		{
			return err;
		}
	}

	if(stmt == B1_ID_STMT_IF)
	{
		// nested IF is forbidden
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			return B1_RES_ENESTEDIF;
		}

		err = b1_int_st_if(offset);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// save current statement state
		return b1_int_save_stmt_state(b1_curr_prog_line_offset ? (B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC) : (B1_INT_STATE_IF | B1_INT_STATE_IF_NEXT));
	}

	// any statement executing in IF should change state to SKIP
	if(b1_int_curr_stmt_state == (B1_INT_STATE_IF | B1_INT_STATE_IF_EXEC))
	{
		b1_int_curr_stmt_state = (B1_INT_STATE_IF | B1_INT_STATE_IF_SKIP);
	}

	// process "ON <exp> GOTO linen1,...linenN" and "ON <exp> GOSUB linen1,...linenN"
	onpos = 0;

	if(stmt == B1_ID_STMT_ON)
	{
		err = b1_int_st_on(offset, &onpos);
		if(err != B1_RES_OK)
		{
			return err;
		}

		return b1_int_save_stmt_state(B1_INT_STATE_ON_SET(onpos));
	}

	if(stmt == B1_ID_STMT_GOTO)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_ON)
		{
			onpos = B1_INT_STATE_ON_POS_GET(b1_int_curr_stmt_state);

			err = b1_int_restore_stmt_state();
			if(err != B1_RES_OK)
			{
				return err;
			}
		}

		// GOTO should terminate IF statement processing
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			err = b1_int_restore_stmt_state();
			if(err != B1_RES_OK)
			{
				return err;
			}
		}

		return b1_int_st_go(offset, onpos);
	}

	if(stmt == B1_ID_STMT_GOSUB)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_ON)
		{
			onpos = B1_INT_STATE_ON_POS_GET(b1_int_curr_stmt_state);

			err = b1_int_restore_stmt_state();
			if(err != B1_RES_OK)
			{
				return err;
			}
		}

		// save current statement state
		err = b1_int_save_stmt_state(B1_INT_STATE_GOSUB);
		if(err != B1_RES_OK)
		{
			return err;
		}

		return b1_int_st_go(offset, onpos);
	}

	if(stmt == B1_ID_STMT_RETURN)
	{
		// RETURN should unwind statements stack
		while(!(b1_int_curr_stmt_state & B1_INT_STATE_GOSUB))
		{
			err = b1_int_restore_stmt_state();
			if(err == B1_RES_ESTSTKUDF)
			{
				err = B1_RES_ENOGOSUB;
			}
			if(err != B1_RES_OK)
			{
				return err;
			}
		}

		return b1_int_restore_stmt_state();
	}

	if(stmt == B1_ID_STMT_FOR)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			return B1_RES_EINVSTAT;
		}

		// save current statement state
		err = b1_int_save_stmt_state(B1_INT_STATE_FOR);
		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_int_st_for_start(offset);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// b1_int_st_for_test restores previous statement state if needed
		return b1_int_st_for_test();
	}

	if(stmt == B1_ID_STMT_NEXT)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			return B1_RES_EINVSTAT;
		}

		if(!(b1_int_curr_stmt_state & B1_INT_STATE_FOR))
		{
			return B1_RES_ENXTWOFOR;
		}

		err = b1_int_st_for_next(offset);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// b1_int_st_for_test restores previous statement state if needed
		return b1_int_st_for_test();
	}

#ifdef B1_FEATURE_STMT_WHILE_WEND
	if(stmt == B1_ID_STMT_WHILE)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			return B1_RES_EINVSTAT;
		}

		// b1_int_st_while saves the current statement state if needed
		return b1_int_st_while(offset);
	}

	if(stmt == B1_ID_STMT_WEND)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			return B1_RES_EINVSTAT;
		}

		if(!(b1_int_curr_stmt_state & B1_INT_STATE_WHILE))
		{
			return B1_RES_EWNDWOWHILE;
		}

		// go to the beginning of the loop
		err = b1_int_restore_stmt_state();
		if(err != B1_RES_OK)
		{
			return err;
		}

		b1_curr_prog_line_cnt--;

		return B1_RES_OK;
	}
#endif

#ifdef B1_FEATURE_STMT_BREAK_CONTINUE
	if(stmt == B1_ID_STMT_BREAK || stmt == B1_ID_STMT_CONTINUE)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			err = b1_int_restore_stmt_state();
			if(err != B1_RES_OK)
			{
				return err;
			}
		}

#ifdef B1_FEATURE_STMT_WHILE_WEND
		if(!(b1_int_curr_stmt_state & (B1_INT_STATE_FOR | B1_INT_STATE_WHILE)))
#else
		if(!(b1_int_curr_stmt_state & B1_INT_STATE_FOR))
#endif
		{
			return B1_RES_ENOTINLOOP;
		}

		// go to FOR or WHILE satement
		b1_curr_prog_line_cnt = (*(b1_int_stmt_stack + b1_int_stmt_stack_ptr - 1)).ret_line_cnt;

#ifdef B1_FEATURE_STMT_WHILE_WEND
		err = B1_RES_OK;

		if(b1_int_curr_stmt_state & B1_INT_STATE_FOR)
		{
#endif
			// in case of FOR loop go to corresponding NEXT statement
			err = b1_ex_prg_for_go_next();
#ifdef B1_FEATURE_STMT_WHILE_WEND
		}
		else
		if(stmt == B1_ID_STMT_BREAK)
		{
			err = b1_ex_prg_while_go_wend();
		}
#endif
		if(err != B1_RES_OK)
		{
			return err;
		}

		if(stmt == B1_ID_STMT_CONTINUE)
		{
			b1_curr_prog_line_cnt--;
		}

		return B1_RES_OK;
	}
#endif

#ifdef B1_FEATURE_STMT_DATA_READ
	if(stmt == B1_ID_STMT_READ)
	{
		return b1_int_st_read(offset);
	}

	if(stmt == B1_ID_STMT_DATA)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			return B1_RES_EINVSTAT;
		}

		return B1_RES_OK;
	}

	if(stmt == B1_ID_STMT_RESTORE)
	{
		err = b1_int_st_restore(offset);
		// b1_int_st_restore function changes b1_next_line_num variable
		b1_next_line_num = B1_T_LINE_NUM_NEXT;
		return err;
	}
#endif

	if(stmt == B1_ID_STMT_PRINT)
	{
		return b1_int_st_print(offset);
	}

	if(stmt == B1_ID_STMT_INPUT)
	{
		return b1_int_st_input(offset);
	}

	if(stmt == B1_ID_STMT_DIM)
	{
		return b1_int_st_dim(offset);
	}

#ifdef B1_FEATURE_STMT_ERASE
	if(stmt == B1_ID_STMT_ERASE)
	{
		return b1_int_st_erase(offset);
	}
#endif

#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
	if(stmt == B1_ID_STMT_RANDOMIZE)
	{
		b1_ex_rnd_randomize(0);
		return B1_RES_OK;
	}
#endif
#endif

#ifdef B1_FEATURE_FUNCTIONS_USER
	// DEF <fn_name>[(<arg1_name[, arg2_name, ...argN_name]>)] = <expression>
	if(stmt == B1_ID_STMT_DEF)
	{
		if(b1_int_curr_stmt_state & B1_INT_STATE_IF)
		{
			return B1_RES_EINVSTAT;
		}

		// just skip the line
		return B1_RES_OK;
	}
#endif

	if(stmt == B1_ID_STMT_SET)
	{
		return b1_int_st_set(offset);
	}

	if(stmt == B1_ID_STMT_END)
	{
		return B1_RES_END;
	}

#ifdef B1_FEATURE_STMT_STOP
	if(stmt == B1_ID_STMT_STOP)
	{
		b1_int_exec_stop = 1;
		return B1_RES_OK;
	}
#endif

	// process LET statement
	return b1_int_st_let(offset, &var_ref, NULL, NULL);
}

B1_T_ERROR b1_int_prerun()
{
	B1_T_ERROR err;
	uint8_t stmt, for_nest;
	B1_T_LINE_NUM prev_line_n;
	B1_T_PROG_LINE_CNT line_cnt;

	prev_line_n = B1_T_LINE_NUM_ABSENT;
	for_nest = 0;

	while(1)
	{
		err = b1_ex_prg_get_prog_line(B1_T_LINE_NUM_NEXT);
		// do not treat B1_RES_EPROGUNEND as error in this case: just program end
		if(err == B1_RES_EPROGUNEND)
		{
			break;
		}

		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_tok_stmt_init(&stmt);
		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_ex_prg_cache_curr_line_num(b1_next_line_num, stmt);
		if(err != B1_RES_OK)
		{
			return err;
		}

		// check line number
		if(b1_next_line_num != B1_T_LINE_NUM_ABSENT)
		{
			if(prev_line_n != B1_T_LINE_NUM_ABSENT && prev_line_n >= b1_next_line_num)
			{
				return B1_RES_EINVLINEN;
			}

			prev_line_n = b1_next_line_num;
		}

		line_cnt = b1_curr_prog_line_cnt;

		if(stmt == B1_ID_STMT_FOR)
		{
			if(for_nest == B1_MAX_STMT_NEST_DEPTH)
			{
				return B1_RES_ESTSTKOVF;
			}

			b1_int_stmt_stack[for_nest].ret_line_cnt = line_cnt;

			for_nest++;
		}

		if(stmt == B1_ID_STMT_NEXT)
		{
			if(for_nest == 0)
			{
				return B1_RES_ENXTWOFOR;
			}

			for_nest--;
		}

#ifdef B1_FEATURE_FUNCTIONS_USER
		if(stmt == B1_ID_STMT_DEF)
		{
			err = b1_int_st_def(b1_curr_prog_line_offset, line_cnt);
			if(err != B1_RES_OK)
			{
				return err;
			}
		}
#endif

		b1_curr_prog_line_offset = 0;
	}

	if(for_nest != 0)
	{
		b1_curr_prog_line_cnt = b1_int_stmt_stack[for_nest].ret_line_cnt - 1;

		return B1_RES_EFORWONXT;
	}

	b1_curr_prog_line_cnt = 0;
	b1_next_line_num = B1_T_LINE_NUM_NEXT;

	return B1_RES_OK;
}

B1_T_ERROR b1_int_run()
{
	B1_T_ERROR err;
	uint8_t stmt;

	while(1)
	{
		if(b1_curr_prog_line_offset == 0)
		{
			err = b1_ex_prg_get_prog_line(b1_next_line_num);
			if(err != B1_RES_OK)
			{
				return err;
			}

#ifdef B1_FEATURE_DEBUG
			if(!b1_int_continue_after_break &&
				b1_dbg_check_breakpoint(b1_curr_prog_line_cnt) != B1_MAX_BREAKPOINT_NUM)
			{
				b1_curr_prog_line_cnt--;
				b1_int_continue_after_break++;
				return B1_RES_STOP;
			}

			b1_int_continue_after_break = 0;
#endif
		}

		err = b1_tok_stmt_init(&stmt);
		if(err != B1_RES_OK)
		{
			return err;
		}

		err = b1_int_interpret_stmt(stmt);
		if(err == B1_RES_END)
		{
			return B1_RES_OK;
		}
		if(err != B1_RES_OK)
		{
			return err;
		}

#ifdef B1_FEATURE_STMT_STOP
		if(b1_int_exec_stop)
		{
			b1_int_exec_stop = 0;
			return B1_RES_STOP;
		}
#endif
	}

	return err;
}
