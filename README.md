# BASIC1 interpreter core  
  
*just one more BASIC interpreter*  
  
# Brief  
  
- supports classic BASIC dialect with various extensions  
- simple line-by-line interpreter  
- configurable language features/extensions  
- interpreter core written in C  
- embeddable: memory functions, input/output and caching detached from core  
- [command-line interpreter](https://github.com/basic-1/basic-1) built for Windows x86, Windows x64, Linux i386, Linux amd64, Linux armhf  
- [interpreter with graphical IDE and debugger](https://github.com/basic-1/basic-1) built for various platforms (uses wxWidgets GUI library)  
- optional Unicode support  
- licensed under MIT license  
  
# Data types  
  
- STRING  
- SINGLE (32-bit floating-point)  
- DOUBLE (64-bit floating-point)  
- INT (32-bit integer)  
- INT16 (16-bit integer)  
- WORD (16-bit unsigned integer)  
- BYTE (8-bit unsigned integer)  
  
# Statements  
  
- BREAK  
- CONTINUE  
- DATA  
- DEF  
- DIM  
- ELSE  
- ELSEIF ... THEN  
- ERASE  
- FOR ... TO ... \[STEP\]  
- GOTO  
- GOSUB  
- IF ... THEN  
- INPUT  
- LET  
- NEXT  
- ON ... GOTO  
- ON ... GOSUB  
- OPTION BASE | EXPLICIT  
- PRINT  
- RANDOMIZE  
- READ  
- REM  
- RESTORE  
- RETURN  
- SET MARGIN | ZONEWIDTH | INPUTECHO  
- STOP  
- WHILE ... WEND  
  
# More features  
  
- optional line numbers  
- optional LET statement  
- case-insensitive statement, variable and function names  
- one-, two- and three-dimensional arrays  
- relational operators can be used with strings  
- automatic numeric to string conversion  
- functions: ABS, INT, RND, SGN, LEN, ASC, CHR$, STR$, VAL  
- more functions: IIF, IIF$, ATN, COS, EXP, LOG, PI, SIN, SQR, TAN  
- more functions: MID$, INSTR, LTRIM$, RTRIM$, LEFT$, RIGHT$, LSET$, RSET$, UCASE$, LCASE$  
  
# Project directories structure  
  
- `./docs` - documentation directory  
- `./source` - source directory  
- `./source/common` - common source files  
- `./source/ext` - a separate directory for environment-specific source files  
- `./LICENSE` - MIT license text file  
- `./README.md` - this file  
  
# More documents  
  
[BASIC1 language reference](https://github.com/basic-1/b1core/blob/master/docs/reference.md)  
[BASIC1 interpreter limitations](https://github.com/basic-1/b1core/blob/master/docs/limits.md)  
[BASIC1 interpreter core embedding](https://github.com/basic-1/b1core/blob/master/docs/embedding.md)  
[Change log](https://github.com/basic-1/b1core/blob/master/docs/changelog)  
  