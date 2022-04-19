/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 b1rpn.h: constants and types for RPN
*/


#ifndef _B1_RPN_
#define _B1_RPN_

#include <stdint.h>

#include "b1feat.h"
#include "b1id.h"
#include "b1tok.h"
#include "b1var.h"


#define B1_RPNREC_TYPE_IMM_VALUE ((uint8_t)0x1)
#define B1_RPNREC_TYPE_FNVAR ((uint8_t)0x2)
#define B1_RPNREC_TYPE_OPER ((uint8_t)0x4)
// special type used during RPN building only (b1_rpn_build function output never contains such records)
#define B1_RPNREC_TYPE_OPEN_BRAC ((uint8_t)0x8)
// special type used with RPN for user defined functions (b1_rpn_build function output never contains such records)
#define B1_RPNREC_TYPE_FN_ARG ((uint8_t)0x8)
#define B1_RPNREC_TYPE_MASK ((uint8_t)0xF)

#ifdef B1_FEATURE_MINIMAL_EVALUATION
// special type values
#define B1_RPNREC_TYPE_SPEC_ARG_1 ((uint8_t)0xFF)
#define B1_RPNREC_TYPE_SPEC_ARG_2 ((uint8_t)0x7F)
#define B1_RPNREC_TYPE_SPEC_ARG_3 ((uint8_t)0x3F)
#define B1_RPNREC_TEST_SPEC_ARG(FLAGS) ((((uint8_t)FLAGS) & ((uint8_t)0x3F)) == (uint8_t)0x3F)
#endif

#define B1_RPNREC_IMM_VALUE_NULL_ARG ((uint8_t)0x10)

#define B1_RPNREC_FNVAR_ARG_NUM_MASK ((uint8_t)0x70)
#define B1_RPNREC_FNVAR_ARG_NUM_SHIFT ((uint8_t)0x4)

#define B1_RPNREC_FN_ARG_INDEX_MASK ((uint8_t)0x70)
#define B1_RPNREC_FN_ARG_INDEX_SHIFT ((uint8_t)0x4)

#define B1_RPNREC_OPER_PRI_MASK ((uint8_t)0x70)
#define B1_RPNREC_OPER_PRI_SHIFT ((uint8_t)0x4)
#define B1_RPNREC_OPER_LEFT_ASSOC ((uint8_t)0x80)

#define B1_RPNREC_GET_TYPE(FLAGS) ((((uint8_t)(FLAGS)) & (B1_RPNREC_TYPE_MASK)))
#define B1_RPNREC_TEST_TYPES(FLAGS, TYPES) (B1_RPNREC_GET_TYPE(FLAGS) & ((uint8_t)(TYPES)))

#define B1_RPNREC_TEST_IMM_VALUE_NULL_ARG(FLAGS) (((uint8_t)(FLAGS)) & B1_RPNREC_IMM_VALUE_NULL_ARG)

#define B1_RPNREC_GET_FNVAR_ARG_NUM(FLAGS) ((((uint8_t)(FLAGS)) & (B1_RPNREC_FNVAR_ARG_NUM_MASK)) >> (B1_RPNREC_FNVAR_ARG_NUM_SHIFT))

#define B1_RPNREC_GET_FN_ARG_INDEX(FLAGS) ((((uint8_t)(FLAGS)) & (B1_RPNREC_FN_ARG_INDEX_MASK)) >> (B1_RPNREC_FN_ARG_INDEX_SHIFT))

#define B1_RPNREC_GET_OPER_PRI(FLAGS) ((((uint8_t)(FLAGS)) & (B1_RPNREC_OPER_PRI_MASK)) >> (B1_RPNREC_OPER_PRI_SHIFT))
#define B1_RPNREC_TEST_OPER_PRI(FLAGS, PRIORITY) ((((uint8_t)(FLAGS)) & (B1_RPNREC_OPER_PRI_MASK)) == (((uint8_t)(PRIORITY)) << (B1_RPNREC_OPER_PRI_SHIFT)))

#define B1_RPNREC_TEST_OPER_LEFT_ASSOC(FLAGS) (((uint8_t)(FLAGS)) & B1_RPNREC_OPER_LEFT_ASSOC)


typedef struct
{
	uint8_t flags;
	B1_T_IDHASH hash;
#ifdef B1_FEATURE_DEBUG
	B1_T_INDEX offset;
	B1_T_INDEX length;
#endif
} B1_T_RPN_ID;

typedef struct
{
	B1_T_CHAR c;
	B1_T_CHAR c1;
} B1_T_RPN_OPER;

typedef union
{
	B1_TOKENDATA token;
	B1_T_RPN_ID id;
	B1_T_RPN_OPER oper;
#ifdef B1_FEATURE_MINIMAL_EVALUATION
	uint8_t nestlevel;
#endif
} B1_RPN_DATA;

typedef struct
{
	uint8_t flags;
	B1_RPN_DATA data;
} B1_RPNREC;


extern const B1_RPNREC *b1_rpn;
extern B1_VAR b1_rpn_eval[B1_MAX_RPN_EVAL_BUFFER_LEN];

extern B1_T_ERROR b1_rpn_build(B1_T_INDEX offset, const B1_T_CHAR **stop_tokens, B1_T_INDEX *continue_offset);

#endif
