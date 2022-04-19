/*
 BASIC1 interpreter
 Copyright (c) 2021-2022 Nikolay Pletnev
 MIT license

 b1rpn.c: converting expressions to RPN form
*/


#include <stdlib.h>

#include "b1.h"
#include "b1ex.h"
#include "b1tok.h"
#include "b1rpn.h"
#include "b1types.h"
#include "b1dbg.h"
#include "b1err.h"


// buffer for an expression represented in RPN
B1_RPNREC b1_rpn_buf[B1_MAX_RPN_LEN];
// RPN buffer pointer
const B1_RPNREC *b1_rpn;
// needed for expression evaluation
B1_VAR b1_rpn_eval[B1_MAX_RPN_EVAL_BUFFER_LEN];


static const B1_T_CHAR *b1_rpn_op_names[] =
{
	_MOD,
	_AND,
	_OR,
	_XOR,
	_NOT
};

static const B1_T_CHAR b1_rpn_op_chars[] =
{
	B1_T_C_PERCENT,
	B1_T_C_AMPERSAND,
	B1_T_C_PIPE,
	B1_T_C_TILDE,
	B1_T_C_EXCLAMATION
};


#define QUEUE_PUT(QUEUE_INDEX, VALUE) do \
{ \
	*(b1_rpn_buf + (QUEUE_INDEX)) = (VALUE); \
	(QUEUE_INDEX)++; \
} while(0)
#define STACK_PUSH(STACK_TOP_INDEX, VALUE) do \
{ \
	(STACK_TOP_INDEX)--; \
	*(b1_rpn_buf + (STACK_TOP_INDEX)) = (VALUE); \
} while(0)
#define STACK_IS_EMPTY(STACK_TOP_INDEX, STACK_CAPACITY) ((STACK_TOP_INDEX) == (STACK_CAPACITY))
#define STACK_TSTTYPE_COPY(STACK_TOP_INDEX, TYPE, CPYTO) b1_rpn_test_type_copy(STACK_TOP_INDEX, TYPE, CPYTO)
#define STACK_REMOVE(STACK_TOP_INDEX) ((STACK_TOP_INDEX)++)


//
static B1_T_ERROR b1_rpn_test_type_copy(B1_T_INDEX stack_top_index, uint8_t types_mask, B1_T_INDEX copy_to_index)
{
	B1_RPNREC *stack_top = b1_rpn_buf + stack_top_index;
	uint8_t flags = (*stack_top).flags;

	if(B1_RPNREC_TEST_TYPES(flags, types_mask))
	{
		*(b1_rpn_buf + copy_to_index) = *stack_top;
	}
	else
	{
		flags = 0;
	}

	return flags;
}

// build RPN for expressions without assignment operation
B1_T_ERROR b1_rpn_build(B1_T_INDEX offset, const B1_T_CHAR **stop_tokens, B1_T_INDEX *continue_offset)
{
	B1_T_INDEX i, top, len;
	B1_RPNREC rr;
	B1_T_ERROR err;
	uint8_t ttype, tmp, tmp1, lassoc, unop, argnum, argtop, argstk[B1_MAX_RPN_BRACK_NEST_DEPTH];
	B1_T_CHAR c, c1, cprev;
#ifdef B1_FEATURE_RPN_CACHING
	B1_T_INDEX init_offset;
#endif
#ifdef B1_FEATURE_MINIMAL_EVALUATION
	uint8_t iif;
#endif

	// use the same array for both output queue and operator stack
	// output queue index variable
	i = 0;
	// operator stack top index variable (when top == B1_ENV_MAX_RPN_LEN the stack is empty)
	top = B1_MAX_RPN_LEN;
	// token type
	ttype = 0;
	// function or subscripted variable argument number
	argnum = 0;
	// argument count stack top pointer
	argtop = 0;
	// first character of the current token
	c = 0;
	// unary operation
	unop = 1;
	// token length
	len = 0;

#ifdef B1_FEATURE_RPN_CACHING
#ifdef B1_FEATURE_DEBUG
	if(b1_dbg_rpn_caching_enabled)
	{
#endif
		b1_rpn = b1_rpn_buf;
		b1_rpn_buf[0].flags = 0;

		init_offset = offset;

		err = b1_ex_prg_rpn_get_cached(offset, continue_offset);
		if (err != B1_RES_OK)
		{
			return err;
		}

		if (b1_rpn[0].flags != 0)
		{
			return B1_RES_OK;
		}
#ifdef B1_FEATURE_DEBUG
	}
	else
	{
		b1_rpn = b1_rpn_buf;
	}
#endif
#else
	b1_rpn = b1_rpn_buf;
#endif

#ifdef B1_FEATURE_MINIMAL_EVALUATION
	iif = 0;
#endif

	while(1)
	{
		// ttype here is either 0 or type of the previous token
		tmp = ttype;
		// test for unary operators
		unop = unop || ((tmp & B1_TOKEN_TYPE_OPERATION) && c != B1_T_C_CLBRACK);
		cprev = c;

		// get next token
		err = b1_tok_get(offset + len, B1_TOK_CALC_HASH, &rr.data.token);
		if(err != B1_RES_OK)
		{
			return err;
		}

		ttype = rr.data.token.type;
		offset = rr.data.token.offset;
		len = rr.data.token.length;

		if(len == 0)
		{
			c = 0;
		}
		else
		{
			// get the first token character(-s)
			c = *(b1_progline + offset);
			c1 = (len == 1) ? (B1_T_CHAR)0 : *(b1_progline + offset + 1);

			for(tmp1 = 0; tmp1 < sizeof(b1_rpn_op_names) / sizeof(b1_rpn_op_names[0]); tmp1++)
			{
				if(!b1_t_strcmpi(b1_rpn_op_names[tmp1], b1_progline + offset, len))
				{
					c = b1_rpn_op_chars[tmp1];
					c1 = (B1_T_CHAR)0;
					ttype = B1_TOKEN_TYPE_OPERATION;
					break;
				}
			}
		}

		// process variables with subscripts and functions (tmp here is type of the previous token)
		if((tmp & B1_TOKEN_TYPE_IDNAME) && c != B1_T_C_OPBRACK)
		{	
			if(!STACK_IS_EMPTY(top, B1_MAX_RPN_LEN) && STACK_TSTTYPE_COPY(top, B1_RPNREC_TYPE_FNVAR, i) != 0)
			{
				i++;
				top++;
			}
		}

		if(c)
		{
			// look for stop token (argtop == 0 stands for balanced brackets state)
			if(stop_tokens && !argtop)
			{
				for(tmp = 0; ; tmp++)
				{
					if(*(stop_tokens + tmp) == NULL) break;

					if(!b1_t_strcmpi(*(stop_tokens + tmp), b1_progline + offset, len))
					{
						c = 0;
						
						// the first token is stop token: put NULL value into output queue
						if(!i)
						{
							rr.flags = B1_RPNREC_TYPE_IMM_VALUE | B1_RPNREC_IMM_VALUE_NULL_ARG;
							QUEUE_PUT(i, rr);
						}

						break;
					}
				}
			}
		}

		if(!c) break;

		// unop variable should be true only in case of unary minus or bitwise NOT
		if(unop)
		{
			switch(c)
			{
				case B1_T_C_PLUS:
					continue;
				case B1_T_C_MINUS:
				case B1_T_C_EXCLAMATION:
					break;
				default:
					unop = (uint8_t)0;
			}
		}

		if(B1_T_ISCOMMA(c) || c == B1_T_C_CLBRACK)
		{
			while(1)
			{
				if(STACK_IS_EMPTY(top, B1_MAX_RPN_LEN))
				{
					return B1_RES_EMISSBRACK;
				}
				if(STACK_TSTTYPE_COPY(top, (uint8_t)~(B1_RPNREC_TYPE_OPEN_BRAC), i) == 0) break;
				i++;
				top++;
			}

			if(B1_T_ISCOMMA(cprev) || cprev == B1_T_C_OPBRACK)
			{
				if(i == top)
				{
					return B1_RES_EEXPLONG;
				}
				rr.flags = B1_RPNREC_TYPE_IMM_VALUE | B1_RPNREC_IMM_VALUE_NULL_ARG;
				QUEUE_PUT(i, rr);
			}

			argnum++;
#ifdef B1_FEATURE_MINIMAL_EVALUATION
			// iif here stands for processing arguments of IIF or IIF$ function
			iif = argnum & 0x80;
			// tmp is the current argument number (one-based)
			tmp = argnum & 0x7F;

			if(tmp > B1_MAX_FN_ARGS_NUM)
#else
			if(argnum > B1_MAX_FN_ARGS_NUM)
#endif
			{
				return B1_RES_EWRARGCNT;
			}

#ifdef B1_FEATURE_MINIMAL_EVALUATION
			if(iif)
			{
				iif = 0;

				if(i == top)
				{
					return B1_RES_EEXPLONG;
				}

				rr.flags =	tmp == 1 ? B1_RPNREC_TYPE_SPEC_ARG_1 :
							tmp == 2 ? B1_RPNREC_TYPE_SPEC_ARG_2 : B1_RPNREC_TYPE_SPEC_ARG_3;
				rr.data.nestlevel = argtop;
				QUEUE_PUT(i, rr);
			}
#endif

			if(c == B1_T_C_CLBRACK)
			{
				STACK_REMOVE(top);

				if(!STACK_IS_EMPTY(top, B1_MAX_RPN_LEN) && STACK_TSTTYPE_COPY(top, B1_RPNREC_TYPE_FNVAR, i) != 0)
				{
#ifdef B1_FEATURE_MINIMAL_EVALUATION
					b1_rpn_buf[i].flags = B1_RPNREC_TYPE_FNVAR | (uint8_t)(tmp << (B1_RPNREC_FNVAR_ARG_NUM_SHIFT));
#else
					b1_rpn_buf[i].flags = B1_RPNREC_TYPE_FNVAR | (uint8_t)(argnum << (B1_RPNREC_FNVAR_ARG_NUM_SHIFT));
#endif
					i++;
					top++;
				}

				if(argtop == 0)
				{
					return B1_RES_EUNBRACK;
				}

				argtop--;
				argnum = argstk[argtop];
			}

			continue;
		}

		if(i == top)
		{
			return B1_RES_EEXPLONG;
		}

		if(ttype & (B1_TOKEN_TYPE_NUMERIC | B1_TOKEN_TYPE_QUOTEDSTR))
		{
			rr.flags = B1_RPNREC_TYPE_IMM_VALUE;
			// stack and queue should not overwrite each other
			QUEUE_PUT(i, rr);

			continue;
		}

		if(((ttype & (B1_TOKEN_TYPE_IDNAME)) || c == B1_T_C_OPBRACK))
		{
			if(c == B1_T_C_OPBRACK)
			{
				if(argtop == B1_MAX_RPN_BRACK_NEST_DEPTH)
				{
					return B1_RES_EMANYBRAC;
				}

				argstk[argtop] = argnum;
#ifdef B1_FEATURE_MINIMAL_EVALUATION
				// the first bit of argnum variable stands for processing arguments of IIF or IIF$ function
				argnum = iif ? (uint8_t)0x80 : (uint8_t)0;
				iif = 0;
#else
				argnum = 0;
#endif
				argtop++;
			}

			rr.flags = (c == B1_T_C_OPBRACK) ? (B1_RPNREC_TYPE_OPEN_BRAC) : (B1_RPNREC_TYPE_FNVAR);

			if(rr.flags == B1_RPNREC_TYPE_FNVAR)
			{
#ifdef B1_FEATURE_DEBUG
				rr.data.id.offset = offset;
				rr.data.id.length = len;
#endif
				rr.data.id.hash = b1_tok_id_hash;
#ifdef B1_FEATURE_MINIMAL_EVALUATION
				// set iif flag if hash corresponds to IIF or IIF$ name
				if(b1_tok_id_hash == B1_FN_IIF_FN_HASH || b1_tok_id_hash == B1_FN_STRIIF_FN_HASH)
				{
					iif++;
				}
#endif
				b1_t_get_type_by_type_spec(*(b1_progline + offset + len - 1), B1_TYPE_NULL, &rr.data.id.flags);
			}

			STACK_PUSH(top, rr);

			continue;
		}

		if(ttype & (B1_TOKEN_TYPE_OPERATION))
		{
			// semicolon is allowed as operation token but cannot be used in this context
			if(B1_T_ISSEMICOLON(c))
			{
				return B1_RES_ESYNTAX;
			}

			tmp = unop																?	(uint8_t)(0 << (B1_RPNREC_OPER_PRI_SHIFT)) | (B1_RPNREC_TYPE_OPER) :
				(c == B1_T_C_CARET)													?	(uint8_t)(1 << (B1_RPNREC_OPER_PRI_SHIFT)) | (B1_RPNREC_TYPE_OPER) :
				(c == B1_T_C_ASTERISK || c == B1_T_C_SLASH || c == B1_T_C_PERCENT)	?	(uint8_t)(2 << (B1_RPNREC_OPER_PRI_SHIFT)) | (B1_RPNREC_TYPE_OPER) | (B1_RPNREC_OPER_LEFT_ASSOC) :
				(B1_T_ISPLUS(c) || B1_T_ISMINUS(c))									?	(uint8_t)(3 << (B1_RPNREC_OPER_PRI_SHIFT)) | (B1_RPNREC_TYPE_OPER) | (B1_RPNREC_OPER_LEFT_ASSOC) :
				(c == c1 && (c == B1_T_C_LT || c == B1_T_C_GT))						?	(uint8_t)(4 << (B1_RPNREC_OPER_PRI_SHIFT)) | (B1_RPNREC_TYPE_OPER) | (B1_RPNREC_OPER_LEFT_ASSOC) :
				(c == B1_T_C_AMPERSAND)												?	(uint8_t)(5 << (B1_RPNREC_OPER_PRI_SHIFT)) | (B1_RPNREC_TYPE_OPER) | (B1_RPNREC_OPER_LEFT_ASSOC) :
				(c == B1_T_C_PIPE || c == B1_T_C_TILDE)								?	(uint8_t)(6 << (B1_RPNREC_OPER_PRI_SHIFT)) | (B1_RPNREC_TYPE_OPER) | (B1_RPNREC_OPER_LEFT_ASSOC) :
																						(uint8_t)(7 << (B1_RPNREC_OPER_PRI_SHIFT)) | (B1_RPNREC_TYPE_OPER) | (B1_RPNREC_OPER_LEFT_ASSOC);
			rr.flags = tmp;
			rr.data.oper.c = c;
			rr.data.oper.c1 = c1;

			unop = 0;

			lassoc = B1_RPNREC_TEST_OPER_LEFT_ASSOC(tmp);
			tmp = B1_RPNREC_GET_OPER_PRI(tmp);

			while(1)
			{
				if(STACK_IS_EMPTY(top, B1_MAX_RPN_LEN)) break;

				tmp1 = STACK_TSTTYPE_COPY(top, B1_RPNREC_TYPE_OPER, i);
				if(tmp1 == 0) break;

				tmp1 = B1_RPNREC_GET_OPER_PRI(tmp1);

				if(!(lassoc ? (tmp1 <= tmp) : (tmp1 < tmp)))
				{
					break;
				}

				i++;
				top++;
			}

			STACK_PUSH(top, rr);
		}
	}

	while(!STACK_IS_EMPTY(top, B1_MAX_RPN_LEN))
	{
		if(STACK_TSTTYPE_COPY(top, B1_RPNREC_TYPE_OPER, i) == 0) break;
		i++;
		top++;
	}

	if(!STACK_IS_EMPTY(top, B1_MAX_RPN_LEN))
	{
		return B1_RES_EUNBRACK;
	}

	if(i == top)
	{
		return B1_RES_EEXPLONG;
	}

	b1_rpn_buf[i].flags = 0;

	if(len == 0)
	{
		offset = 0;
	}

	if(continue_offset != NULL)
	{
		*continue_offset = offset;
	}

#ifdef B1_FEATURE_RPN_CACHING
	err = b1_ex_prg_rpn_cache(init_offset, offset);
	if(err != B1_RES_OK)
	{
		return err;
	}
#endif

	return B1_RES_OK;
}
