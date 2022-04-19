/*
 BASIC1 interpreter
 Copyright (c) 2021 Nikolay Pletnev
 MIT license

 b1dbg.c: functions for debugging
*/

#ifndef _B1_DEBUG_
#define _B1_DEBUG_

#include "b1feat.h"

#ifdef B1_FEATURE_DEBUG
#include "b1var.h"


extern B1_T_INDEX b1_dbg_breakpoints_num;

#ifdef B1_FEATURE_RPN_CACHING
extern uint8_t b1_dbg_rpn_caching_enabled;
#endif


extern B1_T_ERROR b1_dbg_get_var_dump(const B1_NAMED_VAR *var, B1_T_CHAR *sbuf, B1_T_INDEX buflen);
extern B1_T_INDEX b1_dbg_check_breakpoint(B1_T_PROG_LINE_CNT line_cnt);
extern B1_T_ERROR b1_dbg_add_breakpoint(B1_T_PROG_LINE_CNT line_cnt);
extern B1_T_ERROR b1_dbg_remove_breakpoint(B1_T_PROG_LINE_CNT line_cnt);
extern B1_T_ERROR b1_dbg_remove_all_breakpoints();
extern B1_T_ERROR b1_dbg_get_break_line_cnt(B1_T_PROG_LINE_CNT *line_cnt);
#endif

#endif
