/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 b1itypes.h: internal types (for identifier hash, character, program
 line number, array subscript, etc.)
*/


#ifndef _B1_ITYPES_
#define _B1_ITYPES_

#include <stdint.h>

#include "b1feat.h"


// sizes
#define B1_T_CHAR_SIZE ((uint8_t)(sizeof(B1_T_CHAR)))
#define B1_T_INDEX_SIZE ((uint8_t)(sizeof(B1_T_INDEX)))
#define B1_T_MEMOFFSET_SIZE ((uint8_t)(sizeof(B1_T_MEMOFFSET)))
#define B1_T_SUBSCRIPT_SIZE ((uint8_t)(sizeof(B1_T_SUBSCRIPT)))
#define B1_T_PROG_LINE_CNT_SIZE ((uint8_t)(sizeof(B1_T_PROG_LINE_CNT)))
#define B1_T_LINE_NUM_SIZE ((uint8_t)(sizeof(B1_T_LINE_NUM)))
#define B1_T_IDHASH_SIZE ((uint8_t)(sizeof(B1_T_IDHASH)))

// min/max values for internal interpreter types
#ifdef B1_FEATURE_UNICODE_UCS2
#define B1_T_CHAR_MAX_VALUE (UINT16_MAX)
#define B1_T_INDEX_MAX_VALUE (UINT16_MAX)
#else
#define B1_T_CHAR_MAX_VALUE (UINT8_MAX)
#define B1_T_INDEX_MAX_VALUE (UINT8_MAX)
#endif

#if defined(B1_FEATURE_SUBSCRIPT_8BIT)
#define B1_T_SUBSCRIPT_MIN_VALUE (INT8_MIN)
#define B1_T_SUBSCRIPT_MAX_VALUE (INT8_MAX)
#elif defined(B1_FEATURE_SUBSCRIPT_12BIT)
#define B1_T_SUBSCRIPT_MIN_VALUE (INT16_MIN >> 4)
#define B1_T_SUBSCRIPT_MAX_VALUE (INT16_MAX >> 4)
#elif defined(B1_FEATURE_SUBSCRIPT_16BIT)
#define B1_T_SUBSCRIPT_MIN_VALUE (INT16_MIN)
#define B1_T_SUBSCRIPT_MAX_VALUE (INT16_MAX)
#elif defined(B1_FEATURE_SUBSCRIPT_24BIT)
#define B1_T_SUBSCRIPT_MIN_VALUE (INT32_MIN >> 8)
#define B1_T_SUBSCRIPT_MAX_VALUE (INT32_MAX >> 8)
#else
#define B1_T_SUBSCRIPT_MIN_VALUE (INT16_MIN)
#define B1_T_SUBSCRIPT_MAX_VALUE (INT16_MAX)
#endif

#if defined(B1_FEATURE_MEMOFFSET_16BIT)
#define B1_T_MEMOFFSET_MIN_VALUE (INT16_MIN)
#define B1_T_MEMOFFSET_MAX_VALUE (INT16_MAX)
#elif defined(B1_FEATURE_MEMOFFSET_32BIT)
#define B1_T_MEMOFFSET_MIN_VALUE (INT32_MIN)
#define B1_T_MEMOFFSET_MAX_VALUE (INT32_MAX)
#else
#define B1_T_MEMOFFSET_MIN_VALUE (INT32_MIN)
#define B1_T_MEMOFFSET_MAX_VALUE (INT32_MAX)
#endif

#define B1_T_PROG_LINE_CNT_MAX_VALUE (UINT16_MAX)
#define B1_T_LINE_NUM_MIN_VALUE ((B1_T_LINE_NUM)1)
// 5 reserved values
#define B1_T_LINE_NUM_MAX_VALUE ((B1_T_LINE_NUM)((UINT16_MAX) - 5))

#define B1_T_MEM_BLOCK_DESC_INVALID ((B1_T_MEM_BLOCK_DESC)NULL)

// reserved values for B1_T_LINE_NUM type
#define B1_T_LINE_NUM_ABSENT ((B1_T_LINE_NUM)(B1_T_LINE_NUM_MAX_VALUE + 3))
#define B1_T_LINE_NUM_FIRST ((B1_T_LINE_NUM)(B1_T_LINE_NUM_MAX_VALUE + 2))
#define B1_T_LINE_NUM_NEXT ((B1_T_LINE_NUM)(B1_T_LINE_NUM_MAX_VALUE + 1))

#define B1_T_RAND_SEED_MAX_VALUE (UINT16_MAX)


// type for error codes
typedef uint8_t B1_T_ERROR;
// B1_T_CHAR: type for program line and data string characters (unsigned type, from 0 to B1_T_CHAR_MAX_VALUE)
// B1_T_INDEX: type for program line and data string index variables (unsigned type, from 0 to B1_T_INDEX_MAX_VALUE)
#ifdef B1_FEATURE_UNICODE_UCS2
typedef uint16_t B1_T_CHAR;
typedef uint16_t B1_T_INDEX;
#else
typedef uint8_t B1_T_CHAR;
typedef uint8_t B1_T_INDEX;
#endif
// type for array subscripts (signed type, from B1_T_SUBSCRIPT_MIN_VALUE to B1_T_SUBSCRIPT_MAX_VALUE)
#if defined(B1_FEATURE_SUBSCRIPT_8BIT)
typedef int8_t B1_T_SUBSCRIPT;
#elif defined(B1_FEATURE_SUBSCRIPT_12BIT)
typedef int16_t B1_T_SUBSCRIPT;
#elif defined(B1_FEATURE_SUBSCRIPT_16BIT)
typedef int16_t B1_T_SUBSCRIPT;
#elif defined(B1_FEATURE_SUBSCRIPT_24BIT)
typedef int32_t B1_T_SUBSCRIPT;
#else
typedef int16_t B1_T_SUBSCRIPT;
#endif
// type for pointer offsets and memory block sizes (the type must be signed)
#if defined(B1_FEATURE_MEMOFFSET_16BIT)
typedef int16_t B1_T_MEMOFFSET;
#elif defined(B1_FEATURE_MEMOFFSET_32BIT)
typedef int32_t B1_T_MEMOFFSET;
#else
typedef int32_t B1_T_MEMOFFSET;
#endif
// type for memory block descriptor, just a pointer to void, NULL stands for invalid value
typedef const void *B1_T_MEM_BLOCK_DESC;
// type for program line counter (from 0 to B1_T_PROG_LINE_CNT_MAX_VALUE, 0 means default state)
typedef uint16_t B1_T_PROG_LINE_CNT;
// type for program line number
typedef uint16_t B1_T_LINE_NUM;
// type of idendifier hash
typedef uint16_t B1_T_IDHASH;
// type for random generator (an unsigned integer type, floating point value returned by RND function
// is the result of the next expression: ((float)(seed - ((seed == B1_T_RAND_SEED_MAX_VALUE) ? 1 : 0))) / (float)B1_T_RAND_SEED_MAX_VALUE
typedef uint16_t B1_T_RAND_SEED;

#endif
