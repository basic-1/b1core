/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 exprg.cpp: external interpreter function samples for loading program
 from file, navigating through program lines, caching line numbers and
 expressions in form of RPN
*/


#include <cstdio>
#include <cstdlib>
#include <vector>
#include <stack>
#include <map>
#include <iterator>

extern "C"
{
#include "b1rpn.h"
#include "b1int.h"
#include "b1err.h"
}


#define B1_ENV_PROG_LINES_INC 10


#if B1_T_PROG_LINE_CNT_MAX_VALUE > UINT16_MAX || B1_T_INDEX_MAX_VALUE > UINT16_MAX
#error current RPN caching implementation does not work with B1_T_PROG_LINE_CNT and B1_T_INDEX types larger than 16 bit
#endif


// line number cache
static std::map<B1_T_LINE_NUM, B1_T_PROG_LINE_CNT> b1_ex_prg_line_num_cache;
// FOR/NEXT statement line counters cache
static std::stack<B1_T_PROG_LINE_CNT> b1_ex_prg_for_line_cnt_stack;
static std::map<B1_T_PROG_LINE_CNT, B1_T_PROG_LINE_CNT> b1_ex_prg_for_line_cnt_cache;
#ifdef B1_FEATURE_STMT_WHILE_WEND
// WHILE/WEND statement line counters cache
static std::stack<B1_T_PROG_LINE_CNT> b1_ex_prg_while_line_cnt_stack;
static std::map<B1_T_PROG_LINE_CNT, B1_T_PROG_LINE_CNT> b1_ex_prg_while_line_cnt_cache;
#endif
// DATA statement line counters cache
#ifdef B1_FEATURE_STMT_DATA_READ
static std::vector<std::pair<B1_T_PROG_LINE_CNT, B1_T_INDEX>> b1_ex_prg_data_line_cnt_cache;
#endif
#ifdef B1_FEATURE_RPN_CACHING
static std::map<uint32_t, std::pair<B1_T_INDEX, std::vector<B1_RPNREC>>> b1_ex_prg_rpn_expr_cache;
#endif
static std::vector<std::vector<B1_T_CHAR>> b1_ex_prg_lines;


#ifdef B1_FEATURE_UNICODE_UCS2
static B1_T_ERROR b1_ex_prg_read_line(FILE *fp, std::vector<B1_T_CHAR> &ostr)
{
	wint_t c;
	B1_T_ERROR err;

	err = B1_RES_OK;

	while(1)
	{
		c = fgetwc(fp);
		if(c == WEOF)
		{
			if(feof(fp))
			{
				err = B1_RES_EEOF;
			}
			else
			{
				err = B1_RES_EENVFAT;
			}

			break;
		}

		if(c == '\n')
		{
			break;
		}

		ostr.push_back((B1_T_CHAR)c);
	}

	ostr.push_back(0);

	return err;
}
#else
static B1_T_ERROR b1_ex_prg_read_line(FILE *fp, std::vector<B1_T_CHAR> &ostr)
{
	int c;
	B1_T_ERROR err;

	err = B1_RES_OK;

	while(1)
	{
		c = fgetc(fp);
		if(c == EOF)
		{
			if(feof(fp))
			{
				err = B1_RES_EEOF;
			}
			else
			{
				err = B1_RES_EENVFAT;
			}

			break;
		}

		if(c == '\n')
		{
			break;
		}

		ostr.push_back((B1_T_CHAR)c);
	}

	ostr.push_back(0);

	return err;
}
#endif

extern "C" B1_T_ERROR b1_ex_prg_set_prog_file(const char *prog_file)
{
	FILE *fp;
	std::vector<B1_T_CHAR> line;
	B1_T_ERROR err;

	if(!b1_ex_prg_lines.empty())
	{
		b1_ex_prg_line_num_cache.clear();
		while(!b1_ex_prg_for_line_cnt_stack.empty())
		{
			b1_ex_prg_for_line_cnt_stack.pop();
		}
		b1_ex_prg_for_line_cnt_cache.clear();
#ifdef B1_FEATURE_STMT_WHILE_WEND
		while(!b1_ex_prg_while_line_cnt_stack.empty())
		{
			b1_ex_prg_while_line_cnt_stack.pop();
		}
		b1_ex_prg_while_line_cnt_cache.clear();
#endif
#ifdef B1_FEATURE_STMT_DATA_READ
		b1_ex_prg_data_line_cnt_cache.clear();
#endif
#ifdef B1_FEATURE_RPN_CACHING
		b1_ex_prg_rpn_expr_cache.clear();
#endif
		b1_ex_prg_lines.clear();
	}

	if(prog_file != NULL)
	{
		fp = fopen(prog_file, "rt");
		if(fp == NULL)
		{
			return B1_RES_EENVFAT;
		}

		err = B1_RES_OK;

		while(err == B1_RES_OK)
		{
			err = b1_ex_prg_read_line(fp, line);
			if(err == B1_RES_OK || err == B1_RES_EEOF)
			{
				b1_ex_prg_lines.push_back(line);
				line.clear();
			}
			else
			{
				return err;
			}
		}
	}

	return B1_RES_OK;
}

static B1_T_ERROR b1_ex_prg_get_next_prog_line()
{
	B1_T_PROG_LINE_CNT line_cnt;

	line_cnt = b1_curr_prog_line_cnt;

	if(line_cnt >= b1_ex_prg_lines.size())
	{
		return B1_RES_EPROGUNEND;
	}

	b1_progline = b1_ex_prg_lines[line_cnt].data();
	b1_curr_prog_line_cnt = line_cnt + 1;
	
	return B1_RES_OK;
}

static B1_T_ERROR b1_ex_prg_get_first_prog_line()
{
	b1_curr_prog_line_cnt = 0;
	return b1_ex_prg_get_next_prog_line();
}


// caches line numbers (allowing faster program navigation), the cached data can be used by b1_ex_prg_get_prog_line and other functions
extern "C" B1_T_ERROR b1_ex_prg_cache_curr_line_num(B1_T_LINE_NUM curr_line_num, uint8_t stmt)
{
#ifdef B1_FEATURE_STMT_DATA_READ
	if(stmt == B1_ID_STMT_DATA)
	{
		b1_ex_prg_data_line_cnt_cache.push_back(std::pair<B1_T_PROG_LINE_CNT, B1_T_INDEX>(b1_curr_prog_line_cnt, b1_curr_prog_line_offset));
	}
#endif

	if(curr_line_num != B1_T_LINE_NUM_ABSENT)
	{
		b1_ex_prg_line_num_cache[curr_line_num] = b1_curr_prog_line_cnt - 1;
	}

	// move FOR statement line counter to tmp. stack
	if(stmt == B1_ID_STMT_FOR)
	{
		b1_ex_prg_for_line_cnt_stack.push(b1_curr_prog_line_cnt);
	}

	if(stmt == B1_ID_STMT_NEXT && !b1_ex_prg_for_line_cnt_stack.empty())
	{
		b1_ex_prg_for_line_cnt_cache[b1_ex_prg_for_line_cnt_stack.top()] = b1_curr_prog_line_cnt;
		b1_ex_prg_for_line_cnt_stack.pop();
	}

#ifdef B1_FEATURE_STMT_WHILE_WEND
	// move WHILE statement line counter to tmp. stack
	if(stmt == B1_ID_STMT_WHILE)
	{
		b1_ex_prg_while_line_cnt_stack.push(b1_curr_prog_line_cnt);
	}

	if(stmt == B1_ID_STMT_WEND && !b1_ex_prg_while_line_cnt_stack.empty())
	{
		b1_ex_prg_while_line_cnt_cache[b1_ex_prg_while_line_cnt_stack.top()] = b1_curr_prog_line_cnt;
		b1_ex_prg_while_line_cnt_stack.pop();
	}
#endif

	return B1_RES_OK;
}

// the function should set b1_progline and b1_curr_prog_line_cnt global variables according to the program line
// number requested via next_line_num argument. the argument can be either BASIC line number or one of the next constants:
// B1_T_LINE_NUM_FIRST, B1_T_LINE_NUM_NEXT. can return the next values (error codes): B1_RES_OK, B1_RES_ELINENNOTFND,
// B1_RES_EPROGUNEND, B1_RES_EENVFAT
extern "C" B1_T_ERROR b1_ex_prg_get_prog_line(B1_T_LINE_NUM next_line_num)
{
	if(next_line_num == B1_T_LINE_NUM_FIRST)
	{
		return b1_ex_prg_get_first_prog_line();
	}

	if(next_line_num == B1_T_LINE_NUM_NEXT)
	{
		return b1_ex_prg_get_next_prog_line();
	}

	auto line_it = b1_ex_prg_line_num_cache.find(next_line_num);
	if(line_it == b1_ex_prg_line_num_cache.end())
	{	
		return B1_RES_ELINENNOTFND;
	}

	b1_progline = b1_ex_prg_lines[line_it->second].data();
	b1_curr_prog_line_cnt = line_it->second + 1;

	return B1_RES_OK;
}

// sets the NEXT statement line counter for the current FOR statement
extern "C" B1_T_ERROR b1_ex_prg_for_go_next()
{
	auto line_cnt_it = b1_ex_prg_for_line_cnt_cache.find(b1_curr_prog_line_cnt);

	if(line_cnt_it == b1_ex_prg_for_line_cnt_cache.end())
	{
		return B1_RES_EFORWONXT;
	}
	
	b1_curr_prog_line_cnt = line_cnt_it->second;

	return B1_RES_OK;
}

#ifdef B1_FEATURE_STMT_WHILE_WEND
// sets the WEND statement line counter for the current WHILE statement
extern "C" B1_T_ERROR b1_ex_prg_while_go_wend()
{
	auto line_cnt_it = b1_ex_prg_while_line_cnt_cache.find(b1_curr_prog_line_cnt);

	if(line_cnt_it == b1_ex_prg_while_line_cnt_cache.end())
	{
		return B1_RES_EWHILEWOWND;
	}
	
	b1_curr_prog_line_cnt = line_cnt_it->second;

	return B1_RES_OK;
}
#endif

#ifdef B1_FEATURE_STMT_DATA_READ
// sets the next DATA stamtement line counter (b1_data_curr_line_cnt and b1_data_curr_line_offset),
// next_line_num can be either valid line number or B1_T_LINE_NUM_FIRST, B1_T_LINE_NUM_NEXT constants.
// possible return codes: B1_RES_OK, B1_RES_EDATAEND, B1_RES_ELINENNOTFND, B1_RES_EENVFAT, etc.
extern "C" B1_T_ERROR b1_ex_prg_data_go_next(B1_T_LINE_NUM next_line_num)
{
	if(b1_ex_prg_data_line_cnt_cache.empty())
	{
		return B1_RES_EDATAEND;
	}

	if(next_line_num == B1_T_LINE_NUM_FIRST)
	{
		b1_data_curr_line_cnt = b1_ex_prg_data_line_cnt_cache[0].first;
	}
	else
	if(next_line_num != B1_T_LINE_NUM_NEXT)
	{
		auto line_it = b1_ex_prg_line_num_cache.find(next_line_num);
		if(line_it == b1_ex_prg_line_num_cache.end())
		{
			return B1_RES_ELINENNOTFND;
		}

		b1_data_curr_line_cnt = line_it->second + 1;
	}

	for(auto i = b1_ex_prg_data_line_cnt_cache.begin(); ; i++)
	{
		if(i == b1_ex_prg_data_line_cnt_cache.end() || i->first > b1_data_curr_line_cnt)
		{
			return B1_RES_EDATAEND;
		}

		if(i->first == b1_data_curr_line_cnt)
		{
			if(next_line_num == B1_T_LINE_NUM_NEXT)
			{
				i++;
				if(i == b1_ex_prg_data_line_cnt_cache.end())
				{
					return B1_RES_EDATAEND;
				}
			}

			b1_data_curr_line_cnt = i->first;
			b1_data_curr_line_offset = i->second;

			return B1_RES_OK;
		}
	}

	return B1_RES_OK;
}
#endif

#ifdef B1_FEATURE_RPN_CACHING
extern "C" B1_T_ERROR b1_ex_prg_rpn_cache(B1_T_INDEX offset, B1_T_INDEX continue_offset)
{
	std::vector<B1_RPNREC> rpn;

	// Do not cache empty or small RPN
	if(b1_rpn[0].flags == 0 || b1_rpn[1].flags == 0)
	{
		return B1_RES_OK;
	}

	for(B1_T_INDEX i = 0; ; i++)
	{
		rpn.push_back(b1_rpn[i]);
		if(b1_rpn[i].flags == 0)
		{
			break;
		}
	}

	b1_ex_prg_rpn_expr_cache[(((uint32_t)b1_curr_prog_line_cnt) << 16) + offset] = std::pair<B1_T_INDEX, std::vector<B1_RPNREC>>(continue_offset, rpn);
	
	return B1_RES_OK;
}

extern "C" B1_T_ERROR b1_ex_prg_rpn_get_cached(B1_T_INDEX offset, B1_T_INDEX *continue_offset)
{
	auto expr_rpn_it = b1_ex_prg_rpn_expr_cache.find((((uint32_t)b1_curr_prog_line_cnt) << 16) + offset);

	if(expr_rpn_it != b1_ex_prg_rpn_expr_cache.end())
	{
		if(continue_offset != NULL)
		{
			*continue_offset = expr_rpn_it->second.first;
		}

		b1_rpn = expr_rpn_it->second.second.data();
	}

	return B1_RES_OK;
}
#endif
