/*
 BASIC1 interpreter
 Copyright (c) 2021 Nikolay Pletnev
 MIT license

 b1err.h: interpreter error codes definition
*/


#ifndef _B1_ERR_
#define _B1_ERR_

#include "b1itypes.h"


// return codes
#define B1_RES_FIRSTERRCODE ((B1_T_ERROR)100)
#define B1_RES_LASTERRCODE ((B1_T_ERROR)146)
// OK, no error
#define B1_RES_OK ((B1_T_ERROR)0)
// OK, execution reached END statement
#define B1_RES_END ((B1_T_ERROR)1)
// OK, execution stopped
#define B1_RES_STOP ((B1_T_ERROR)2)
// EINVTOK error: invalid token
#define B1_RES_EINVTOK ((B1_T_ERROR)100)
// ELINLONG error: program line too long
#define B1_RES_ELINLONG ((B1_T_ERROR)101)
// EINVLINEN error: invalid line number
#define B1_RES_EINVLINEN ((B1_T_ERROR)102)
// EINVSTAT error: invalid statement
#define B1_RES_EINVSTAT ((B1_T_ERROR)103)
// EINVSTAT error: invalid argument
#define B1_RES_EINVARG ((B1_T_ERROR)104)
// EEXPLONG error: expression too long
#define B1_RES_EEXPLONG ((B1_T_ERROR)105)
// EMISSBRACK error: missing comma or bracket
#define B1_RES_EMISSBRACK ((B1_T_ERROR)106)
// EUNBRACK error: unbalanced brackets
#define B1_RES_EUNBRACK ((B1_T_ERROR)107)
// EWRARGNUM error: wrong argument count
#define B1_RES_EWRARGCNT ((B1_T_ERROR)108)
// ETMPSTKOVF error: expression evaluation temp stack overflow
#define B1_RES_ETMPSTKOVF ((B1_T_ERROR)109)
// ESYNTAX error: unknown syntax error
#define B1_RES_ESYNTAX ((B1_T_ERROR)110)
// EWARGTYPE error: wrong argument type
#define B1_RES_EWARGTYPE ((B1_T_ERROR)111)
// ENOMEM error: not enough memory
#define B1_RES_ENOMEM ((B1_T_ERROR)112)
// EINVMEMBLK error: invalid memory block descriptor
#define B1_RES_EINVMEMBLK ((B1_T_ERROR)113)
// EBUFSMALL error: buffer too small
#define B1_RES_EBUFSMALL ((B1_T_ERROR)114)
// ESTRLONG error: string too long
#define B1_ESTRLONG ((B1_T_ERROR)115)
// EMANYBRAC error: too many open brackets
#define B1_RES_EMANYBRAC ((B1_T_ERROR)116)
// EUNKIDENT error: unknown identifier
#define B1_RES_EUNKIDENT ((B1_T_ERROR)117)
// EWSUBSCNT error: wrong subscript count
#define B1_RES_EWSUBSCNT ((B1_T_ERROR)118)
// ETYPMISM error: type mismatch
#define B1_RES_ETYPMISM ((B1_T_ERROR)119)
// ESUBSRANGE error: subscript out of range
#define B1_RES_ESUBSRANGE ((B1_T_ERROR)120)
// EIDINUSE error: identifier already in use
#define B1_RES_EIDINUSE ((B1_T_ERROR)121)
// EIDIVZERO error: integer divide by zero
#define B1_RES_EIDIVZERO ((B1_T_ERROR)122)
// ENESTEDIF error: nested IF statement not allowed
#define B1_RES_ENESTEDIF ((B1_T_ERROR)123)
// EELSEWOIF error: ELSE without IF
#define B1_RES_EELSEWOIF ((B1_T_ERROR)124)
// ELINENNOTFND error: line number not found
#define B1_RES_ELINENNOTFND ((B1_T_ERROR)125)
// ESTSTKOVF error: statement stack overflow
#define B1_RES_ESTSTKOVF ((B1_T_ERROR)126)
// ESTSTKUDF error: statement stack underflow
#define B1_RES_ESTSTKUDF ((B1_T_ERROR)127)
// ERESWORD error: can't use the reserved word in this context
#define B1_RES_ERESWORD ((B1_T_ERROR)128)
// ENOTVAR error: not a variable
#define B1_RES_ENOTVAR ((B1_T_ERROR)129)
// EENVFAT error: environment fatal error (can't read program file, IO error, etc.)
#define B1_RES_EENVFAT ((B1_T_ERROR)130)
// ENOGOSUB error: unexpected RETURN statement
#define B1_RES_ENOGOSUB ((B1_T_ERROR)131)
// EPROGUNEND error: unexpected end of program (possibly no END statement)
#define B1_RES_EPROGUNEND ((B1_T_ERROR)132)
// EDATAEND error: the end of DATA block reached
#define B1_RES_EDATAEND ((B1_T_ERROR)133)
// EWNDWOWHILE error: WEND without WHILE
#define B1_RES_EWNDWOWHILE ((B1_T_ERROR)134)
// ENXTWOFOR error: NEXT without FOR
#define B1_RES_ENXTWOFOR ((B1_T_ERROR)135)
// EFORWONXT error: FOR without NEXT
#define B1_RES_EFORWONXT ((B1_T_ERROR)136)
// EFORSUBSVAR error: can't use subscripted variable as FOR loop control variable
#define B1_RES_EFORSUBSVAR ((B1_T_ERROR)137)
// EINVNUM error: invalid number
#define B1_RES_EINVNUM ((B1_T_ERROR)138)
// ENUMOVF error: numeric overflow
#define B1_RES_ENUMOVF ((B1_T_ERROR)139)
// EMANYDEF error: too many DEF statements
#define B1_RES_EMANYDEF ((B1_T_ERROR)140)
// EUDEFOVF error: user functions stack overflow
#define B1_RES_EUDEFOVF ((B1_T_ERROR)141)
// EEOF error: end of file
#define B1_RES_EEOF ((B1_T_ERROR)142)
// ERESKWRD error: use of a reserved keyword as identifier forbidden
#define B1_RES_ERESKWRD ((B1_T_ERROR)143)
// EWHILEWOWND error: WHILE without WEND
#define B1_RES_EWHILEWOWND ((B1_T_ERROR)144)
// ENOTINLOOP error: BREAK or CONTINUE statement not within a loop
#define B1_RES_ENOTINLOOP ((B1_T_ERROR)145)
// EMANYBRKPNT error: too many breakpoints
#define B1_RES_EMANYBRKPNT ((B1_T_ERROR)146)

#endif