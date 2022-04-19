/*
 BASIC1 interpreter
 Copyright (c) 2020-2021 Nikolay Pletnev
 MIT license

 b1tok.c: tokenizer
*/


#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "b1.h"
#include "b1tok.h"
#include "b1id.h"
#include "b1types.h"
#include "b1err.h"


B1_T_IDHASH b1_tok_id_hash;


// token character types
#define B1_CTYPE_UNKNOWN ((uint8_t)0x0)
#define B1_CTYPE_LETTER ((uint8_t)0x1)
#define B1_CTYPE_DIGIT ((uint8_t)0x2)
#define B1_CTYPE_OPER ((uint8_t)0x4)
#define B1_CTYPE_SPACE ((uint8_t)0x8)
#ifdef B1_FRACTIONAL_TYPE_EXISTS
#define B1_CTYPE_POINT ((uint8_t)0x10)
#endif
#define B1_CTYPE_TYPE_SPEC ((uint8_t)0x20)
#define B1_CTYPE_NULL ((uint8_t)0x40)
#define B1_CTYPE_COMMENT ((uint8_t)0x80)

// numeric value parsing stages
// integer part
#define B1_NUMERIC_PART_INT ((uint8_t)0x1)
#ifdef B1_FRACTIONAL_TYPE_EXISTS
// fractional part (after the point)
#define B1_NUMERIC_PART_FRAC ((uint8_t)0x2)
// exponential part start (E letter is found)
#define B1_NUMERIC_PART_EXP1 ((uint8_t)0x4)
// exponent sign is found
#define B1_NUMERIC_PART_EXP2 ((uint8_t)0x8)
// exponent part
#define B1_NUMERIC_PART_EXP3 ((uint8_t)0x10)
#endif
// type specificator
#define B1_NUMERIC_PART_TYPE_SPEC ((uint8_t)0x20)


B1_T_ERROR b1_tok_get(B1_T_INDEX offset, uint8_t options, B1_TOKENDATA *tokendata)
{
	B1_T_CHAR c, c1;
	B1_T_INDEX b, len, out_index;
	uint8_t ctype;
	uint8_t toktype, extra;

	// token starting offset
	b = offset;
	offset--;
	// token type
	toktype = B1_TOKEN_TYPE_SKIP_SPACES;
	// the variable is used to mark some special cases (open/closed quote, dollar sign at the end of variable name, etc.)
	extra = 0;
	out_index = 1;
	c = 0;

	while(1)
	{
		offset++;

		c1 = c;
		c = *(b1_progline + offset);

		if(offset == B1_T_INDEX_MAX_VALUE && !B1_T_ISCSTRTERM(c))
		{
			return B1_RES_ELINLONG;
		}

		// token length
		len = offset - b;

		// determine character type
		if(B1_T_ISCSTRTERM(c))
			// string terminator
			ctype = B1_CTYPE_NULL;
		else
		if(c == B1_T_C_APOSTROPHE)
			ctype = B1_CTYPE_COMMENT;
		else
		if(B1_T_ISALPHA(c))
			// a letter
			ctype = B1_CTYPE_LETTER;
		else
		if(B1_T_ISDIGIT(c))
			// a digit
			ctype = B1_CTYPE_DIGIT;
		else
		if(B1_T_ISEXPRCHAR(c))
			// an operation or part of an operation
			ctype = B1_CTYPE_OPER;
		else
		if(B1_T_ISBLANK(c) || B1_T_ISCRLF(c))
			// a space character
			ctype = B1_CTYPE_SPACE;
		else
			ctype =
#ifdef B1_FRACTIONAL_TYPE_EXISTS
				// point
				c == B1_T_C_POINT	?	B1_CTYPE_POINT :
#endif
				// dollar sign
				c == B1_T_C_DOLLAR ||
#ifdef B1_FEATURE_TYPE_SINGLE
				c == B1_T_C_EXCLAMATION ||
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
				c == B1_T_C_NUMBER ||
#endif
				c == B1_T_C_PERCENT	?	B1_CTYPE_TYPE_SPEC :
					B1_CTYPE_UNKNOWN;

		// determine token type by its first character
		if(len == 0)
		{
			toktype =
				// variable/function name should start from a letter
				ctype == B1_CTYPE_LETTER						?	B1_TOKEN_TYPE_IDNAME | B1_TOKEN_TYPE_LETTERS :
				// numerics start from a digit or point
#ifdef B1_FRACTIONAL_TYPE_EXISTS
				ctype & (B1_CTYPE_DIGIT | B1_CTYPE_POINT)		?	B1_TOKEN_TYPE_NUMERIC | B1_TOKEN_TYPE_DIGITS :
#else
				ctype == B1_CTYPE_DIGIT							?	B1_TOKEN_TYPE_NUMERIC | B1_TOKEN_TYPE_DIGITS :
#endif
				// operation
				ctype == B1_CTYPE_OPER							?	B1_TOKEN_TYPE_OPERATION :
				// special token type, needed to skip leading spaces
				ctype & (B1_CTYPE_SPACE | B1_CTYPE_NULL | B1_CTYPE_COMMENT)	?	B1_TOKEN_TYPE_SKIP_SPACES :
				// string starts from quote sign
				c == B1_T_C_DBLQUOTE								?	B1_TOKEN_TYPE_QUOTEDSTR :
				// unknown type
																	B1_TOKEN_TYPE_UNKNOWN;

#ifdef B1_FRACTIONAL_TYPE_EXISTS
			if(toktype & B1_TOKEN_TYPE_NUMERIC)
			{
				extra = B1_NUMERIC_PART_INT;
			}
#endif
		}

		if(len == 1 && (options & B1_TOK_ALLOW_UNARY_OPER) && (toktype & B1_TOKEN_TYPE_OPERATION))
		{
#ifdef B1_FRACTIONAL_TYPE_EXISTS
			if((B1_T_ISMINUS(c1) || B1_T_ISPLUS(c1)) && (ctype & (B1_CTYPE_DIGIT | B1_CTYPE_POINT)))
			{
				toktype = B1_TOKEN_TYPE_NUMERIC | B1_TOKEN_TYPE_DIGITS;
				extra = B1_NUMERIC_PART_INT;
			}
#else
			if((B1_T_ISMINUS(c1) || B1_T_ISPLUS(c1)) && (ctype == B1_CTYPE_DIGIT))
			{
				toktype = B1_TOKEN_TYPE_NUMERIC | B1_TOKEN_TYPE_DIGITS;
			}
#endif
		}

		if(options & B1_TOK_COPY_VALUE)
		{
			*(b1_tmp_buf + out_index) = c;
		}

		// skip leading spaces
		if(toktype & B1_TOKEN_TYPE_SKIP_SPACES)
		{
			// in case of string termination return zero length token
			if(ctype & (B1_CTYPE_NULL | B1_CTYPE_COMMENT))
			{
				len = 0;
				break;
			}
			// a non-space character = token starting character, reinitiate variables responsible to token starting position
			if(ctype != B1_CTYPE_SPACE)
			{
				b = offset;
				offset--;
			}

			continue;
		}
		else
		// string token processing
		if(toktype & B1_TOKEN_TYPE_QUOTEDSTR)
		{
			// if the character is quote, just change open/close quote mark
			if(B1_T_ISDBLQUOTE(c))
			{
				if(extra)
				{
					out_index++;
				}
				extra = ~extra;
				continue;
			}
			else
			{
				if(extra)
				{
					out_index++;
					// if quote is opened any non-NULL character is allowed
					if(ctype != B1_CTYPE_NULL)
						continue;
					// in case of NULL just go at the end of the loop (to the "return B1_RES_EINVTOK;" statement)
				}
				else
				{
					// correct the closing quote
					out_index--;
					// if quote is closed the next NULL, space or operation character means the token end
					if(ctype & (B1_CTYPE_NULL | B1_CTYPE_SPACE | B1_CTYPE_OPER | B1_CTYPE_COMMENT))
						break;
					// another character means incorrect token
				}
			}
		}
		else
		// process opreation token
		if(toktype & B1_TOKEN_TYPE_OPERATION)
		{
			if(!(len == 0
				|| (c == B1_T_C_EQ && (c1 == B1_T_C_GT || c1 == B1_T_C_LT))
				|| (c == B1_T_C_GT && c1 == B1_T_C_LT)
				|| (c == c1 && (c1 == B1_T_C_GT || c1 == B1_T_C_LT))
				))
			{
				break;
			}

			out_index++;
			continue;
		}
		else
		// process variable or function name or a statement
		if(toktype & B1_TOKEN_TYPE_IDNAME)
		{
			if(ctype & (B1_CTYPE_DIGIT | B1_CTYPE_LETTER | B1_CTYPE_TYPE_SPEC))
			{
				if(ctype == B1_CTYPE_DIGIT)
				{
					toktype = B1_TOKEN_TYPE_IDNAME;
				}

				// extra flag stands for '$', '!' or '%' character presence (it can be the last character of basic function or variable name)
				if(!extra)
				{
					if(ctype == B1_CTYPE_TYPE_SPEC)
					{
						toktype = B1_TOKEN_TYPE_IDNAME;
						extra++;
					}

					out_index++;

					continue;
				}
			}
			else
			if(ctype & (B1_CTYPE_SPACE | B1_CTYPE_OPER | B1_CTYPE_NULL | B1_CTYPE_COMMENT))
			{
				break;
			}
		}
		else
		// numeric token
		if(toktype & B1_TOKEN_TYPE_NUMERIC)
		{
#ifdef B1_FRACTIONAL_TYPE_EXISTS
			// support both integer and fractional numeric representation forms
			if(extra == B1_NUMERIC_PART_TYPE_SPEC)
			{
				if(ctype & (B1_CTYPE_SPACE | B1_CTYPE_OPER | B1_CTYPE_NULL | B1_CTYPE_COMMENT))
				{
					break;
				}
			}
			else
			if(extra & (B1_NUMERIC_PART_INT | B1_NUMERIC_PART_FRAC | B1_NUMERIC_PART_EXP3))
			{
				if(c == B1_T_C_PERCENT
#ifdef B1_FEATURE_TYPE_SINGLE
					|| c == B1_T_C_EXCLAMATION
#endif
#ifdef B1_FEATURE_TYPE_DOUBLE
					|| c == B1_T_C_NUMBER
#endif
					)
				{
#ifdef B1_FEATURE_HEX_NUM
					if(c == B1_T_C_PERCENT || !(toktype & B1_TOKEN_TYPE_HEX))
					{
#endif
					extra = B1_NUMERIC_PART_TYPE_SPEC;
					toktype &= ~(B1_TOKEN_TYPE_DIGITS);
					out_index++;
					continue;
#ifdef B1_FEATURE_HEX_NUM
					}
#endif
				}
				else
				if(ctype & (B1_CTYPE_SPACE | B1_CTYPE_OPER | B1_CTYPE_NULL | B1_CTYPE_COMMENT))
				{
					if(extra != B1_NUMERIC_PART_INT)
					{
						toktype = B1_TOKEN_TYPE_NUMERIC;
					}

					break;
				}
				else
				if(extra & (B1_NUMERIC_PART_INT | B1_NUMERIC_PART_FRAC))
				{
#ifdef B1_FEATURE_HEX_NUM
					if((extra & B1_NUMERIC_PART_INT) && (c == (B1_T_CHAR)'X' || c == (B1_T_CHAR)'x') && c1 == B1_T_C_0)
					{
						toktype = B1_TOKEN_TYPE_NUMERIC | B1_TOKEN_TYPE_HEX;
						out_index++;
						continue;
					}

					if(!(toktype & B1_TOKEN_TYPE_HEX))
					{
#endif
					if((extra & B1_NUMERIC_PART_INT) && ctype == B1_CTYPE_POINT)
					{
						extra = B1_NUMERIC_PART_FRAC;
						out_index++;
						continue;
					}
					// 'E' or 'e' letter
					if(c == B1_T_C_UCE || c == B1_T_C_LCE)
					{
						if((extra & B1_NUMERIC_PART_INT) || len > 1)
						{
							extra = B1_NUMERIC_PART_EXP1;
							out_index++;
							continue;
						}
					}
#ifdef B1_FEATURE_HEX_NUM
					}
#endif
				}
			}
			else
			{
				if((extra & B1_NUMERIC_PART_EXP1) && (B1_T_ISMINUS(c) || B1_T_ISPLUS(c)))
				{
					extra = B1_NUMERIC_PART_EXP2;
					out_index++;
					continue;
				}
				if(ctype == B1_CTYPE_DIGIT)
				{
					extra = B1_NUMERIC_PART_EXP3;
				}
			}
#else
			// support integer form ony
			if(!extra && c == B1_T_C_PERCENT)
			{
				extra = B1_NUMERIC_PART_TYPE_SPEC;
				toktype &= ~(B1_TOKEN_TYPE_DIGITS);
				out_index++;
				continue;
			}

			if(ctype & (B1_CTYPE_SPACE | B1_CTYPE_OPER | B1_CTYPE_NULL | B1_CTYPE_COMMENT))
			{
				break;
			}
#endif
			if((ctype == B1_CTYPE_DIGIT
#ifdef B1_FEATURE_HEX_NUM
				|| ((toktype & B1_TOKEN_TYPE_HEX) && ((c >= (B1_T_CHAR)'A' && c <= (B1_T_CHAR)'F') || (c >= (B1_T_CHAR)'a' && c <= (B1_T_CHAR)'f')))
#ifndef B1_FRACTIONAL_TYPE_EXISTS
				|| ((c == (B1_T_CHAR)'X' || c == (B1_T_CHAR)'x') && c1 == B1_T_C_0)
#endif
#endif
				)
				&& extra != B1_NUMERIC_PART_TYPE_SPEC)
			{
#if defined(B1_FEATURE_HEX_NUM) && !defined(B1_FRACTIONAL_TYPE_EXISTS)
				if(ctype != B1_CTYPE_DIGIT)
				{
					toktype = B1_TOKEN_TYPE_NUMERIC | B1_TOKEN_TYPE_HEX;
				}
#endif
				out_index++;
				continue;
			}
		}

		return B1_RES_EINVTOK;
	}

	if(tokendata != NULL)
	{
		(*tokendata).type = toktype;
		(*tokendata).offset = b;
		(*tokendata).length = len;
	}

	if(options & B1_TOK_COPY_VALUE)
	{
		out_index--;
		*(b1_tmp_buf) = (B1_T_CHAR)out_index;
	}

	if(options & B1_TOK_CALC_HASH)
	{
		b1_tok_id_hash = b1_id_calc_hash(b1_progline + b, len * B1_T_CHAR_SIZE);
	}

	return B1_RES_OK;
}

// reads line number from (b1_progline + *offset) into b1_next_line_num variable,
// updates *offset value with position of the next character after the line number
B1_T_ERROR b1_tok_get_line_num(B1_T_INDEX *offset)
{
	B1_T_ERROR err;
	B1_T_INDEX len;
	B1_TOKENDATA td;
	B1_T_CHAR buf[6];

	err = b1_tok_get(*offset, 0, &td);
	if(err != B1_RES_OK)
	{
		return err;
	}

	b1_next_line_num = B1_T_LINE_NUM_ABSENT;

	len = td.length;

	if(len != 0)
	{
		if((td.type == (B1_TOKEN_TYPE_NUMERIC | B1_TOKEN_TYPE_DIGITS)))
		{
			if(len <= B1_MAX_LINE_NUM_LEN)
			{
				memcpy(buf, b1_progline + td.offset, len * B1_T_CHAR_SIZE);
				buf[len] = 0;

				err = b1_t_strtoui16(buf, &b1_next_line_num);
				if(err != B1_RES_OK)
				{
					return err;
				}

				if(b1_next_line_num > B1_T_LINE_NUM_MAX_VALUE)
				{
					return B1_RES_EINVLINEN;
				}

				*offset = td.offset + len;
			}
		}
	}

	return B1_RES_OK;
}

B1_T_ERROR b1_tok_stmt_init(uint8_t *stmt)
{
	B1_T_ERROR err;
	B1_T_INDEX len;
	B1_TOKENDATA td;

	*stmt = B1_ID_STMT_ABSENT;

	// get line number
	err = b1_tok_get_line_num(&b1_curr_prog_line_offset);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// get statement
	err = b1_tok_get(b1_curr_prog_line_offset, B1_TOK_CALC_HASH, &td);
	if(err != B1_RES_OK)
	{
		return err;
	}

	// no statement
	len = td.length;
	if(!len)
	{
		return B1_RES_OK;
	}

	// check statement
	if(!(td.type & B1_TOKEN_TYPE_IDNAME))
	{
		return B1_RES_EINVSTAT;
	}

	*stmt = b1_id_get_stmt_by_hash(b1_tok_id_hash);
	if(*stmt != B1_ID_STMT_UNKNOWN)
	{
		b1_curr_prog_line_offset = td.offset + len;
	}

	return B1_RES_OK;
}
