/*
 BASIC1 interpreter
 Copyright (c) 2020-2022 Nikolay Pletnev
 MIT license

 b1ex.h: declarations of functions that are external to the interpreter core
 (memory, variables cache, program lines navigation, input/output, etc.)
*/


#ifndef _B1_EX_
#define _B1_EX_

#include "b1var.h"
#include "b1fn.h"
#include "b1extra.h"


#define B1_EX_MEM_READ ((uint8_t)0x1)
#define B1_EX_MEM_WRITE ((uint8_t)0x2)


extern B1_T_ERROR b1_ex_mem_init();
extern B1_T_ERROR b1_ex_mem_alloc(B1_T_MEMOFFSET size, B1_T_MEM_BLOCK_DESC *mem_desc, void **data);
extern B1_T_ERROR b1_ex_mem_access(const B1_T_MEM_BLOCK_DESC mem_desc, B1_T_MEMOFFSET offset, B1_T_INDEX size, uint8_t options, void **data);
extern B1_T_ERROR b1_ex_mem_release(const B1_T_MEM_BLOCK_DESC mem_desc);
extern B1_T_ERROR b1_ex_mem_free(const B1_T_MEM_BLOCK_DESC mem_desc);

extern B1_T_ERROR b1_ex_var_init();
extern B1_T_ERROR b1_ex_var_alloc(B1_T_IDHASH name_hash, B1_NAMED_VAR **var);
extern B1_T_ERROR b1_ex_var_free(B1_T_IDHASH name_hash);
#ifdef B1_FEATURE_INIT_FREE_MEMORY
extern B1_T_ERROR b1_ex_var_enum(B1_NAMED_VAR **var);
#endif

#ifdef B1_FEATURE_FUNCTIONS_USER
extern B1_T_ERROR b1_ex_ufn_init();
extern B1_T_ERROR b1_ex_ufn_get(B1_T_IDHASH name_hash, uint8_t alloc_new, B1_UDEF_FN **fn);
#endif

// localized string functions
#ifdef B1_FEATURE_LOCALES
extern B1_T_CHAR b1_t_toupper_l(B1_T_CHAR c);
extern B1_T_CHAR b1_t_tolower_l(B1_T_CHAR c);
extern int8_t b1_t_strcmp_l(const B1_T_CHAR *s1, const B1_T_CHAR *s2data, B1_T_INDEX s2len);
#endif

// prints one character
extern B1_T_ERROR b1_ex_io_print_char(B1_T_CHAR c);
// prints new line sequence
extern B1_T_ERROR b1_ex_io_print_newline();
// prints display margin new line sequence
extern B1_T_ERROR b1_ex_io_print_margin_newline();
// copies next input value into the specified buffer
extern B1_T_ERROR b1_ex_io_input_char(B1_T_CHAR *c);

// caches line numbers (allowing faster program navigation), the cached data can be used by b1_ex_prg_get_prog_line ant other functions
extern B1_T_ERROR b1_ex_prg_cache_curr_line_num(B1_T_LINE_NUM curr_line_num, uint8_t stmt);
// the function should set b1_progline and b1_curr_prog_line_cnt global variables according to the program line
// number requested via next_line_num argument. the argument can be either BASIC line number or one of the next constants:
// B1_T_LINE_NUM_FIRST, B1_T_LINE_NUM_NEXT. can return the next values (error codes): B1_RES_OK, B1_RES_ELINENNOTFND,
// B1_RES_EPROGUNEND, B1_RES_EENVFAT
extern B1_T_ERROR b1_ex_prg_get_prog_line(B1_T_LINE_NUM next_line_num);
// sets the NEXT statement line counter for the current FOR statement
extern B1_T_ERROR b1_ex_prg_for_go_next();
#ifdef B1_FEATURE_STMT_WHILE_WEND
// sets the WEND statement line counter for the current WHILE statement
extern B1_T_ERROR b1_ex_prg_while_go_wend();
#endif
#ifdef B1_FEATURE_STMT_DATA_READ
// sets the next DATA stamtement line counter (b1_data_curr_line_cnt and b1_data_curr_line_offset),
// next_line_num can be either valid line number or B1_T_LINE_NUM_FIRST, B1_T_LINE_NUM_NEXT constants.
// possible return codes: B1_RES_OK, B1_RES_EDATAEND, B1_RES_ELINENNOTFND, B1_RES_EENVFAT, etc.
extern B1_T_ERROR b1_ex_prg_data_go_next(B1_T_LINE_NUM next_line_num);
#endif
#ifdef B1_FEATURE_RPN_CACHING
extern B1_T_ERROR b1_ex_prg_rpn_cache(B1_T_INDEX offset, B1_T_INDEX continue_offset);
extern B1_T_ERROR b1_ex_prg_rpn_get_cached(B1_T_INDEX offset, B1_T_INDEX *continue_offset);
#endif

#ifdef B1_FEATURE_FUNCTIONS_MATH_BASIC
#ifdef B1_FRACTIONAL_TYPE_EXISTS
extern void b1_ex_rnd_randomize(uint8_t init);
extern B1_T_RAND_SEED b1_ex_rnd_get_next_seed();
#endif
#endif

#endif
