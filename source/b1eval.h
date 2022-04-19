/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 b1eval.h: expressions evaluation
*/


#ifndef _B1_EVAL_
#define _B1_EVAL_

#include <stdint.h>

#include "b1var.h"


#define B1_EVAL_OPT_PRINT_FUNCTIONS ((uint8_t)0x1)


extern B1_T_ERROR b1_eval_get_numeric_value(B1_VAR *var);
extern B1_T_ERROR b1_eval_neg(B1_VAR *pvar, uint8_t optype, uint8_t abs);
extern B1_T_ERROR b1_eval_add(B1_VAR *pvar1, uint8_t optype);
extern B1_T_ERROR b1_eval_sub(B1_VAR *pvar1, uint8_t optype);
extern B1_T_ERROR b1_eval_cmp(B1_VAR *pvar1, B1_T_CHAR c, B1_T_CHAR c1, uint8_t optype);
extern B1_T_ERROR b1_eval(uint8_t options, B1_VAR_REF *var_ref);

#endif
