/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 b1fn.h: definitions and types for built-in and user defined functions
*/


#ifndef _B1_FN_
#define _B1_FN_

#include "b1feat.h"
#include "b1itypes.h"
#include "b1id.h"
#include "b1rpn.h"
#include "b1var.h"


#define B1_FN_TAB_FN_HASH ((B1_T_IDHASH)0xEE85)
#define B1_FN_SPC_FN_HASH ((B1_T_IDHASH)0xA8C6)
#ifdef B1_FEATURE_MINIMAL_EVALUATION
#define B1_FN_IIF_FN_HASH ((B1_T_IDHASH)0x621d)
#define B1_FN_STRIIF_FN_HASH ((B1_T_IDHASH)0x4866)
#endif

#ifdef B1_FEATURE_FUNCTIONS_STANDARD
#define B1_FN_BLTIN_COUNT_STANDARD 7
#else
#define B1_FN_BLTIN_COUNT_STANDARD 0
#endif

#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
#define B1_FN_BLTIN_COUNT_MATH_BASIC 4
#else
#define B1_FN_BLTIN_COUNT_MATH_BASIC 2
#endif
#else
#define B1_FN_BLTIN_COUNT_MATH_BASIC 0
#endif

#ifdef B1_FEATURE_FUNCTIONS_MATH_EXTRA
#define B1_FN_BLTIN_COUNT_MATH_EXTRA 8
#else
#define B1_FN_BLTIN_COUNT_MATH_EXTRA 0
#endif

#ifdef B1_FEATURE_FUNCTIONS_STRING
#define B1_FN_BLTIN_COUNT_STRING 10
#else
#define B1_FN_BLTIN_COUNT_STRING 0
#endif

#if defined(B1_FEATURE_FUNCTIONS_STANDARD) || defined(B1_FEATURE_FUNCTIONS_MATH_BASIC) || defined(B1_FEATURE_FUNCTIONS_MATH_EXTRA) || defined(B1_FEATURE_FUNCTIONS_STRING)
#define B1_FN_BLTIN_COUNT ((B1_T_INDEX)(B1_FN_BLTIN_COUNT_STANDARD + B1_FN_BLTIN_COUNT_MATH_BASIC + B1_FN_BLTIN_COUNT_MATH_EXTRA + B1_FN_BLTIN_COUNT_STRING))
#endif


#if defined(B1_FEATURE_FUNCTIONS_STANDARD) || defined(B1_FEATURE_FUNCTIONS_MATH_BASIC) || defined(B1_FEATURE_FUNCTIONS_MATH_EXTRA) || defined(B1_FEATURE_FUNCTIONS_STRING) || defined(B1_FEATURE_FUNCTIONS_USER)
typedef struct
{
	B1_ID id;
	uint8_t argtypes[B1_MAX_FN_ARGS_NUM];
	uint8_t ret_type;
} B1_FN;
#endif

#if defined(B1_FEATURE_FUNCTIONS_STANDARD) || defined(B1_FEATURE_FUNCTIONS_MATH_BASIC) || defined(B1_FEATURE_FUNCTIONS_MATH_EXTRA) || defined(B1_FEATURE_FUNCTIONS_STRING)
typedef uint8_t (*b1_fn_bltin_ptr)(B1_VAR *);

typedef struct
{
	B1_FN fn;
	b1_fn_bltin_ptr fn_ptr;
} B1_BLTIN_FN;
#endif

#ifdef B1_FEATURE_FUNCTIONS_USER
typedef struct
{
	B1_FN fn;
	B1_T_PROG_LINE_CNT def_line_cnt;
	B1_T_INDEX rpn_start_pos;
	B1_T_INDEX rpn_len;
} B1_UDEF_FN;

// structure to represent user defined function call stack
typedef struct
{
	// previous program line counter
	B1_T_PROG_LINE_CNT prev_line_cnt;
	// previous RPN pointer and index
	const B1_RPNREC *prev_rpn;
	B1_T_INDEX prev_rpn_end;
	B1_T_INDEX prev_rpn_index;
	// previous arguments base offset
	B1_T_INDEX prev_argsbase;
	// argument count and return type of the current function
	uint8_t curr_arg_num;
	uint8_t curr_ret_type;
} B1_UDEF_CALL;
#endif


#ifdef B1_FEATURE_FUNCTIONS_USER
extern B1_T_INDEX b1_fn_udef_fn_rpn_off;
extern B1_RPNREC b1_fn_udef_fn_rpn[B1_MAX_UDEF_FN_RPN_LEN];
// user defined function call stack
extern B1_UDEF_CALL b1_fn_udef_call_stack[B1_MAX_UDEF_CALL_NEST_DEPTH];
#endif

#ifdef B1_FEATURE_FUNCTIONS_USER
extern B1_T_ERROR b1_fn_get_params(B1_T_IDHASH name_hash, uint8_t alloc_new, B1_FN **fn_ptr);
#else
extern B1_T_ERROR b1_fn_get_params(B1_T_IDHASH name_hash, B1_FN **fn_ptr);
#endif

#endif