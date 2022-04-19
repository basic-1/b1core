/*
 BASIC1 interpreter
 Copyright (c) 2020-2022 Nikolay Pletnev
 MIT license

 b1tok.h: tokenizer
*/


#ifndef _B1_TOKENS_
#define _B1_TOKENS_

#include "b1feat.h"
#include "b1itypes.h"


// tokenizer options
// copies token to b1_tmp_buf global variable, in case of quoted strings parsed content is copied
#define B1_TOK_COPY_VALUE ((uint8_t)0x1)
// treat unary operator as a part of numeric
#define B1_TOK_ALLOW_UNARY_OPER ((uint8_t)0x2)
// calculate token hash
#define B1_TOK_CALC_HASH ((uint8_t)0x4)

// token types
#define B1_TOKEN_TYPE_UNKNOWN ((uint8_t)0x0)
#define B1_TOKEN_TYPE_QUOTEDSTR ((uint8_t)0x1)
#define B1_TOKEN_TYPE_OPERATION ((uint8_t)0x2)
// identifier name
#define B1_TOKEN_TYPE_IDNAME ((uint8_t)0x4)
// numeric value
#define B1_TOKEN_TYPE_NUMERIC ((uint8_t)0x8)
// can be combined with B1_TOKEN_TYPE_IDNAME
#define B1_TOKEN_TYPE_LETTERS ((uint8_t)0x20)
// can be combined with B1_TOKEN_TYPE_NUMERIC
#define B1_TOKEN_TYPE_DIGITS ((uint8_t)0x20)
#ifdef B1_FEATURE_HEX_NUM
// can be combined with B1_TOKEN_TYPE_NUMERIC
#define B1_TOKEN_TYPE_HEX ((uint8_t)0x40)
#endif
// special type, used internally
#define B1_TOKEN_TYPE_SKIP_SPACES ((uint8_t)0x80)


typedef struct
{
	uint8_t type;
	B1_T_INDEX offset;
	B1_T_INDEX length;
} B1_TOKENDATA;


extern B1_T_IDHASH b1_tok_id_hash;

extern B1_T_ERROR b1_tok_get(B1_T_INDEX offset, uint8_t options, B1_TOKENDATA *tokendata);
extern B1_T_ERROR b1_tok_stmt_init(uint8_t *stmt);
extern B1_T_ERROR b1_tok_get_line_num(B1_T_INDEX *offset);

#endif
