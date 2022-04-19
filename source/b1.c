/*
 BASIC1 interpreter
 Copyright (c) 2021 Nikolay Pletnev
 MIT license

 b1.c: macros, variables and functions that are difficult to categorize
*/

#include "b1.h"


// temp. buffers for string operations
B1_T_CHAR b1_tmp_buf[B1_TMP_BUF_LEN];
B1_T_CHAR b1_tmp_buf1[B1_TMP_BUF_LEN];

// state variables
// pointer to a null terminated string, maximal string length is B1_MAX_PROGLINE_LEN
const B1_T_CHAR *b1_progline;
// program line counter, 0 value corresponds to before execution state
B1_T_PROG_LINE_CNT b1_curr_prog_line_cnt;
// offset of the currently processing item within program line
B1_T_INDEX b1_curr_prog_line_offset;
// b1_int_interpret_stmt function sets the variable for the next b1_ex_prg_get_prog_line
// function call to read proper program line (initially the variable is set to
// B1_T_LINE_NUM_NEXT value)
B1_T_LINE_NUM b1_next_line_num;

#ifdef B1_FEATURE_STMT_DATA_READ
// DATA statement counters
B1_T_PROG_LINE_CNT b1_data_curr_line_cnt;
B1_T_INDEX b1_data_curr_line_offset;
#endif

// options can appear in the program beginning only
uint8_t b1_options_allowed;
// OPTION BASE value
uint8_t b1_opt_base_val;
// OPTION EXPLICIT value
uint8_t b1_opt_explicit_val;


// initializes global variables
void b1_reset()
{
	b1_curr_prog_line_cnt = 0;
	b1_curr_prog_line_offset = 0;
	b1_next_line_num = B1_T_LINE_NUM_NEXT;

	b1_options_allowed = 1;
	// set OPTION BASE to 0
	b1_opt_base_val = 0;
	// set OPTION EXPLICIT turned off
	b1_opt_explicit_val = 0;

#ifdef B1_FEATURE_STMT_DATA_READ
	// reset DATA statement counters
	b1_data_curr_line_cnt = 0;
#endif
}
