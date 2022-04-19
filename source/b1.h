/*
 BASIC1 interpreter
 Copyright (c) 2021 Nikolay Pletnev
 MIT license

 b1.h: macros, variables and functions that are difficult to categorize
*/

#ifndef _B1_COMMON_
#define _B1_COMMON_

#include "b1feat.h"
#include "b1itypes.h"


#define B1_TMP_BUF_LEN ((B1_T_INDEX)(((B1_MAX_STRING_LEN) > (B1_MAX_IDENTIFIER_LEN) ? (B1_MAX_STRING_LEN) : (B1_MAX_IDENTIFIER_LEN)) + 1))


extern B1_T_CHAR b1_tmp_buf[B1_TMP_BUF_LEN];
extern B1_T_CHAR b1_tmp_buf1[B1_TMP_BUF_LEN];

extern const B1_T_CHAR *b1_progline;
extern B1_T_PROG_LINE_CNT b1_curr_prog_line_cnt;
extern B1_T_INDEX b1_curr_prog_line_offset;
extern B1_T_LINE_NUM b1_next_line_num;

#ifdef B1_FEATURE_STMT_DATA_READ
// DATA statement counters
extern B1_T_PROG_LINE_CNT b1_data_curr_line_cnt;
extern B1_T_INDEX b1_data_curr_line_offset;
#endif

extern uint8_t b1_options_allowed;
extern uint8_t b1_opt_base_val;
extern uint8_t b1_opt_explicit_val;


extern void b1_reset();

#endif