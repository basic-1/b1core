# Embedding BASIC1 interpreter core  
  
## Preamble  
  
BASIC1 interpreter core is a simple line-by-line BASIC programming language interpreter written in C language. It can be easily embedded in any software developed with C/C++ compiler. BASIC1 interpreter core does not use standard memory allocation, input/output or other environment-specific functions. The interpreter hosting application has to provide such functions to the core as well as some other functions allowing navigating through program lines, caching line numbers and expressions data, etc.  
  
## Typical embedding scenario  
  
1) add core source files from `./source` directory to hosting application project  
2) copy `./source/common/feat.h` file to the project direcory and change it if necessary  
3) implement all external functions necessary for the core to act (there are samples of the functions in `./source/ext` directory)  
4) implement BASIC program loading  
5) call the interpreter initialization and run functions  
  
## Choose interpreter's features  
Some interpreter's features can be turned on or off by editing application's `feat.h` file. Comment corresponding macro definition to disable a feature.  
  
`B1_FEATURE_STMT_ERASE`: enables `ERASE` statement.  
  
`B1_FEATURE_STMT_DATA_READ`: enables `DATA`, `READ` and `RESTORE` statements. If the feature is disabled `b1_ex_prg_data_go_next` function is not needed.  
  
`B1_FEATURE_FUNCTIONS_STANDARD`: enables `LEN`, `ASC`, `CHR$`, `STR$`, `VAL`, `IIF`, `IIF$` functions.  
  
`B1_FEATURE_FUNCTIONS_MATH_BASIC`: enables `ABS`, `INT`, `RND`, `SGN` functions and `RANDOMIZE` statement. No need to implement `b1_ex_rnd_randomize` and `b1_ex_rnd_get_next_seed` functions if the feature is disabled. Also disabling floating-pont data types turns off random generator feature.  
  
`B1_FEATURE_FUNCTIONS_MATH_EXTRA`: enables `ATN`, `COS`, `EXP`, `LOG`, `PI`, `SIN`, `SQR`, `TAN` functions. The feature is not allowed without `SINGLE` or `DOUBLE` type enabled.  
  
`B1_FEATURE_FUNCTIONS_STRING`: enables `MID$`, `INSTR`, `LTRIM$`, `RTRIM$`, `LEFT$`, `RIGHT$`, `LSET$`, `RSET$`, `UCASE$`, `LCASE$` functions.  
  
`B1_FEATURE_FUNCTIONS_USER`: enables `DEF` statement and user defined functions. `b1_ex_ufn_init` and `b1_ex_ufn_get` functions have to be implemented if the feature is enabled.  
  
`B1_FEATURE_TYPE_SINGLE`: enables `SINGLE` data type.  
  
`B1_FEATURE_TYPE_DOUBLE`: enables `DOUBLE` data type.  
  
`B1_FEATURE_RPN_CACHING`: enables expression postfix notation caching. `b1_ex_prg_rpn_cache` and `b1_ex_prg_rpn_get_cached` functions have to be implemented to do the caching if enabled.  
  
`B1_FEATURE_SUBSCRIPT_XXBIT`, where `XX` can be `8`, `12`, `16`, `24`: selects type and range of interpreter's array subscript. Enabling two or more macros of the group is not allowed. The macro determines signed integer data type for internal subscript value representation and subscript range (minimal and maximal values). Default subscript type (if no one macro is enabled) is 16-bit.  
  
`B1_FEATURE_MEMOFFSET_XXBIT`, where `XX` can be `16` or `32`: selects data type for internal memory offset representation. The type should be at least 4 bits larger than subscript data type. Default type is 32-bit.  
  
`B1_FEATURE_LOCALES`: makes the interpreter core use locale-specific functions for built-in `UCASE$`, `LCASE$` and `INSTR` functions and string comparison operators. The locale-specific functions are not a part of the interpreter core and have to be implemented when embedding the core. The functions are: `b1_t_toupper_l`, `b1_t_tolower_l` and `b1_t_strcmp_l`.  
  
`B1_FEATURE_UNICODE_UCS2`: defines `B1_T_CHAR` type as `uint16_t` allowing representing program lines and BASIC string values with 2-byte character encoding.  
  
`B1_FEATURE_INIT_FREE_MEMORY`: if the macro is defined `b1_int_reset` function frees memory allocated during preceding program execution (in the most cases the feature must be enabled to allow resetting program state). `b1_ex_var_enum` function has to be implemented for the feature to work.  
  
`B1_FEATURE_DEBUG`: extends `B1_NAMED_VAR` structure to save variable name (not only name hash), enables debugging abilities (`b1_dbg_*` functions and variables).  
  
`B1_FEATURE_CHECK_KEYWORDS`: forbids creating variables with the same names as statements and existing functions.  
  
`B1_FEATURE_STMT_WHILE_WEND`: enables `WHILE` and `WEND` statements, if the feature is enabled `b1_ex_prg_while_go_wend` function has to be implemented.  
  
`B1_FEATURE_3_DIM_ARRAYS`: enables three-dimensional arrays support.  
  
`B1_FEATURE_MINIMAL_EVALUATION`: enables minimal (short-circuit) evaluation of arguments of `IIF` and `IIF$` functions. If the feature enabled the interpreter evaluates the first argument of the function and then evaluates only one argument of the next two depending on the first argument evaluation result.  
  
`B1_FEATURE_STMT_BREAK_CONTINUE`: enables `BREAK` and `CONTINUE` statements.  
  
`B1_FEATURE_STMT_STOP`: enables possibility to stop program execution with `STOP` statement, makes `b1_int_exec_stop` variable available. See `b1_int_run` function description for details.  
  
`B1_FEATURE_TYPE_SMALL`: enables three small integer data types (`INT16`, `WORD` and `BYTE`)  
  
`B1_FEATURE_HEX_NUM`: enables hexadecimal format support for integer constants (with `0x` prefix, e.g.: `0xFFFF`)  
  
## Interpreter's global variables and functions  
  
Hosting application can control the interpreter core by reading/writing special global variables and calling some functions. The most of them are described below.  
  
`extern const B1_T_CHAR *b1_progline;`  
The variable should point to zero terminated string representing the currently executing program line. The interpreter core calls `b1_ex_prg_get_prog_line` function for the application to set the pointer.  
  
`extern B1_T_PROG_LINE_CNT b1_curr_prog_line_cnt;`  
One-based counter of the current program line. `b1_ex_prg_get_prog_line` and `b1_ex_prg_for_go_next` functions should change the variable by the interpreter's request. Zero value is reserved for the program before execution state.  
  
`extern B1_T_INDEX b1_curr_prog_line_offset;`  
The variable is used along with the previous one to specify an offset withing the current program line. The variable can be read and stored by `b1_ex_prg_cache_curr_line_num` function to return it with `b1_ex_prg_data_go_next` function call in `b1_data_curr_line_offset` variable.  
  
`extern B1_T_PROG_LINE_CNT b1_data_curr_line_cnt;`  
One-based counter of the current `DATA` statement program line. `b1_ex_prg_data_go_next` function should set the variable for it to point to the proper program line containing `DATA` statement.  
  
`extern B1_T_INDEX b1_data_curr_line_offset;`  
Zero-based offset the next `DATA` statement value. The value can bne read with the next `READ` statement. The variable has to be set by `b1_ex_prg_data_go_next` function and the value can be previously stored by `b1_ex_prg_cache_curr_line_num` function.
  
`extern uint8_t b1_int_exec_stop;`  
The variable can be used to stop program execution. Set it to any non-zero value to interrupt execution: this causes `b1_int_run` function termination with `B1_RES_STOP` code. The program execution can be resumed by calling the function again. The variable is available only if `B1_FEATURE_STMT_STOP` feature is enabled.  
  
`extern const B1_RPNREC *b1_rpn;`  
The variable should be used by `b1_ex_prg_rpn_cache` and `b1_ex_prg_rpn_get_cached` functions to cache expressions' postfix notation.  
  
`extern B1_T_ERROR b1_int_reset();`  
The function reset the interpreter core to its initial state. Has to be called before `b1_int_prerun` function to initialize the interpreter or after `b1_int_run` function to free resources.  
  
`extern B1_T_ERROR b1_int_prerun();`  
The function performs the first idle program run to check line numbers, proper `FOR` and `NEXT` statements placement, etc. Also the function calls `b1_ex_prg_cache_curr_line_num` function for every program line.  
  
`extern B1_T_ERROR b1_int_run();`  
Runs the program. If `B1_FEATURE_STMT_STOP` feature is enabled the function can return `B1_RES_STOP` value indicating that the program is stopped. Other possibilities to stop program execution are setting `b1_int_exec_stop` variable to a non-zero value and using breakpoints. The next `b1_int_run` function call resumes the program execution.  
  
`extern B1_T_ERROR b1_dbg_get_var_dump(const B1_NAMED_VAR *var, B1_T_CHAR *sbuf, B1_T_INDEX buflen);`  
Returns a string consisting of a variable name, type and value (or values if the variable is array). `var` argument specifies the variable to dump content of, `sbuf` should be a pointer to buffer to receive the resulting string, `buflen` is the buffer length (number of `B1_T_CHAR` characters). The result is zero character terminating string.  
  
`extern B1_T_ERROR b1_dbg_add_breakpoint(B1_T_PROG_LINE_CNT line_cnt);`  
Adds breakpoint on the program line identified with `line_cnt` program line counter.  
  
`extern B1_T_ERROR b1_dbg_remove_breakpoint(B1_T_PROG_LINE_CNT line_cnt);`  
Removes breakpoint located on line `line_cnt`.  
  
`extern B1_T_ERROR b1_dbg_remove_all_breakpoints();`  
Removes all breakpoints.  
  
`extern B1_T_ERROR b1_dbg_get_break_line_cnt(B1_T_PROG_LINE_CNT *line_cnt);`  
Returns counter of a program line the program stopped on. Can be called after `b1_int_run` function termination with `B1_RES_STOP` code.  
  
`extern B1_T_INDEX b1_dbg_check_breakpoint(B1_T_PROG_LINE_CNT line_cnt);`  
Checks if breakpoint was added on the program line specified with `line_cnt` program line counter. Returns either breakpoint index or `B1_MAX_BREAKPOINT_NUM` constant value if there's no breakpoint on the line.  
  
`b1_dbg_*` functions can be called only when the interpreter is stopped (with `STOP` statement, `b1_int_exec_stop` variable or breakpoint). Also breakpoints can be added before program execution start (before the first `b1_int_run` function call). `STOP` statement stops program before the next line execution and breakpoint - before execution of the line it is added on.  
  
`b1_progline`, `b1_curr_prog_line_cnt`, `b1_curr_prog_line_offset`, `b1_data_curr_line_cnt` and `b1_data_curr_line_offset` variables are declared in `./source/b1.h` file, `b1_rpn` variable is declared in `./source/b1rpn.h` file, `b1_int_*` variables and functions are declared in `./source/b1int.h` file, `b1_dbg_*` functions and variables are declared in `./source/b1dbg.h`.  
  
## External functions needed for the interpreter core  
  
The functions have `b1_ex_` prefix and must be implemented for external linkage with C language (use `extern` keyword if they are written in C or `extern "C"` if they are written in C++).  
  
### Memory management functions  
  
`extern B1_T_ERROR b1_ex_mem_init();`  
`b1_ex_mem_init` function is called on interpreter initialization or reset (`b1_int_reset` function does this).  
  
`extern B1_T_ERROR b1_ex_mem_alloc(B1_T_MEMOFFSET size, B1_T_MEM_BLOCK_DESC *mem_desc, void **data);`  
`b1_ex_mem_alloc` function has to allocate `size` bytes of memory and return the memory block descriptor in `mem_desc` parameter and pointer to already allocated memory in `data` parameter (if `data` pointer is not `NULL` value). A memory block should be accessible for reading and writing after `b1_ex_mem_alloc` call (at most first `B1_MAX_STRING_LEN + 1` bytes of the block). Memory address returned in `data` parameter can point to a temporary buffer and the final data write can occur during the next `b1_ex_mem_release` call. `b1_ex_mem_release` function call makes `data` pointer invalid: to get a new valid pointer to the same memory block the interpreter calls `b1_ex_mem_access` function. The interpreter does not work with two or more memory blocks at the same time.  
  
`extern B1_T_ERROR b1_ex_mem_access(const B1_T_MEM_BLOCK_DESC mem_desc, B1_T_MEMOFFSET offset, B1_T_INDEX size, uint8_t options, void **data);`  
`b1_ex_mem_access` function has to make the memory block identified with `mem_desc` parameter accessible for reading, writing or both operations (depending on `options` parameter) and return the pointer to the block in `data` parameter. `offset` and `size` parameters specify offset and size of data to access. Maximal value for `size` parameter is `B1_MAX_STRING_LEN + 1` (for interpreter to be able managing entire strings). Zero `size` parameter stands for entire memory block or `B1_MAX_STRING_LEN + 1` bytes if the block is larger. `options` parameter is a bit set responsible to desired memory access, possible values: `B1_EX_MEM_READ`, `B1_EX_MEM_WRITE` and `B1_EX_MEM_READ | B1_EX_MEM_WRITE`.  
  
`extern B1_T_ERROR b1_ex_mem_release(const B1_T_MEM_BLOCK_DESC mem_desc);`  
`b1_ex_mem_release` function is called by the interpreter when it finishes working with the memory block and memory manager can move the changes made by the interpreter to some another memory location if needed. The function must not free the memory block. The interpreter calls the function after writing some data to a memory region returned via `data` parameter of `b1_ex_mem_alloc` or `b1_ex_mem_access` function.  
  
`extern B1_T_ERROR b1_ex_mem_free(const B1_T_MEM_BLOCK_DESC mem_desc);`  
`b1_ex_mem_free` function must free the memory block identified with `mem_desc` parameter.  
  
Possible return codes for the functions: `B1_RES_OK` (success), `B1_RES_ENOMEM` (not enough memory).  
  
The simplest implementation of the functions can be found in `./source/ext/exmem.cpp` file (using standard C++ `new` and `delete` operators).  
  
### Input/output functions  
  
`extern B1_T_ERROR b1_ex_io_print_char(B1_T_CHAR c);`  
The function must send a single character to the application textual output device (display, file, memory, etc.).  
  
`extern B1_T_ERROR b1_ex_io_print_newline();`  
The function has to write a new line sequence to the output device (moving printing caret on new line).  
  
`extern B1_T_ERROR b1_ex_io_print_margin_newline();`  
The function is called when the interpreter reaches output device right margin so it should either send new line sequence to the device or do nothing (depending on the device type).  
  
`extern B1_T_ERROR b1_ex_io_input_char(B1_T_CHAR *c);`  
`b1_ex_io_input_char` function has to read the next character from the hosting application textual input device.  
  
Possible return codes for the functions: `B1_RES_OK` (success), `B1_RES_EEOF` (no more data, can be returned by `b1_ex_io_input_char`), `B1_RES_EENVFAT` (fatal I/O error).  
  
The simplest functions implementation: `./source/ext/exio.c` (sdandard C input/output).  
  
### Variables cache functions  
  
`extern B1_T_ERROR b1_ex_var_init();`  
`b1_ex_var_init` function should initialize variables store or reset it removing all existing variables.  
  
`extern B1_T_ERROR b1_ex_var_alloc(B1_T_IDHASH name_hash, B1_NAMED_VAR **var);`  
`b1_ex_var_alloc` function is used for new variable creation or searching for already existing variable in the cache. A variable is represented with `B1_NAMED_VAR` structure and is identified with special hash value (generated from the variable name). If the memory is successfully allocated the function should return `B1_RES_OK` value, if the variable already exists it should return `B1_RES_EIDINUSE` code. In any case the function must return pointer to the structure in `var` parameter.  
  
`extern B1_T_ERROR b1_ex_var_free(B1_T_IDHASH name_hash);`  
The function has to free the memory allocated for `B1_NAMED_VAR` structure of a variable identified with `name_hash` hash value.  
  
`extern B1_T_ERROR b1_ex_var_enum(B1_NAMED_VAR **var);`  
The function is used by the interpreter core only if `B1_FEATURE_INIT_FREE_MEMORY` feature is enabled. It should allow reading all cached variables one-by-one. Calling the function with `*var` equal to `NULL` should result in returning pointer to the first variable in tha cache in the same `var` argument variable. The next call should return pointer to the next variable, etc. The function should put `NULL` value into `*var` if there're no more variables.  
Typical usage sample:  
```
  B1_NAMED_VAR *var = NULL;
  while(1)
  {
    b1_ex_var_enum(&var);
    if(var == NULL)
    {
      break;
    }
    // do something with var
  }
```  
  
Possible return codes for the functions: `B1_RES_OK` (success), `B1_RES_ENOMEM` (not enough memory), `B1_RES_EIDINUSE` (variable already exists).  
  
The simplest functions implementation: `./source/ext/exvar.cpp` (with C++ standard library).  
  
### User-defined functions data cache functions  
  
The functions have to be implemented only if `B1_FEATURE_FUNCTIONS_USER` feature is enabled.  
  
`extern B1_T_ERROR b1_ex_ufn_init();`  
The function should initialize the cache or clear it.  
  
`extern B1_T_ERROR b1_ex_ufn_get(B1_T_IDHASH name_hash, uint8_t alloc_new, B1_UDEF_FN **fn);`  
The function is called by the interpreter to get cached user-defined function data or to add new record to the cache. `name_hash` parameter stands for user-defined function name hash value, `alloc_new` is a logical parameter responsible to the function behavior when record is not found and `fn` is a pointer to a variable to receive address of a found or just allocated cache record. The function should return address of cached `B1_UDEF_FN` structure identified with `name_hash` value: if the record is found the function should write its address to `*fn` and return `B1_RES_OK` value, if the record is not found it should just return `B1_RES_EUNKIDENT` error code. However, if `alloc_new` argument variable is non-zero the function should create new cache element, put its address into `*fn` and return the same `B1_RES_EUNKIDENT` value. If memory for new structure cannot me allocated the function should return `B1_RES_ENOMEM` or `B1_RES_EMANYDEF` error code.  
  
The simplest functions implementation: `./source/ext/exufn.cpp` (with C++ standard library).  
  
### Random values generator functions  
  
`extern void b1_ex_rnd_randomize(uint8_t init);`  
`b1_ex_rnd_randomize` function should either reset random values generator to its initial state (if `init` is not zero) or initialize it with some random value for `RND` function to start to return new random values sequence if (`init` is zero).  
  
`extern B1_T_RAND_SEED b1_ex_rnd_get_next_seed()`  
Should return new random value in range \[0 ... `B1_T_RAND_SEED_MAX_VALUE`\]. The interpreter calls the function when processing `RND` function call.  
  
See `./source/ext/exrnd.c` file for possible functions implementation.  
  
### Program navigation functions  
  
The functions are called by interpreter to navigate through program lines when executing a program.  
  
`extern B1_T_ERROR b1_ex_prg_cache_curr_line_num(B1_T_LINE_NUM curr_line_num, uint8_t stmt);`  
The function is called by the interpreter during the idle program run (see `b1_int_prerun` function description for details) allowing caching line numbers of every program line. The cached values can be used then with other navigation functions to make program line search faster. `curr_line_num` argument value is a program line number of the current program line (identified with a value of `b1_curr_prog_line_cnt` global variable). If a program line does not have line number the argument variable is set to `B1_T_LINE_NUM_ABSENT` value. `stmt` argument variable identifies the current program line statement and can be one of the `B1_ID_STMT_*` values defined in `./source/b1int.h` file.  
  
`extern B1_T_ERROR b1_ex_prg_get_prog_line(B1_T_LINE_NUM next_line_num);`  
`b1_ex_prg_get_prog_line` function is called by the interpreter to navigate to another program line depending on `next_line_num` argument variable value: `B1_T_LINE_NUM_FIRST` and `B1_T_LINE_NUM_NEXT` special values corresponds to the first line of the program and to the line coming after the current one. Other values are line numbers identifying program lines (e.g. the interpreter calls this function when executing `GOTO` statement). The function should return `B1_RES_ELINENNOTFND` code if the line number is not found and `B1_RES_EPROGUNEND` code if it reached the end of the program and the next program line does not exist. If the requested program line is found the function has to change `b1_progline` and `b1_curr_prog_line_cnt` variables properly.  
  
`extern B1_T_ERROR b1_ex_prg_for_go_next();`  
The function should find program line counter of a `NEXT` statement corresponding to the current `FOR` statement (identified with `b1_curr_prog_line_cnt` variable value). The resulting line counter should be written to the same `b1_curr_prog_line_cnt` variable. If the program line is not found the function should return `B1_RES_EFORWONXT` value.  
  
`extern B1_T_ERROR b1_ex_prg_data_go_next(B1_T_LINE_NUM next_line_num);`  
The function is called by the interpreter when processing `READ` statement and all the data of the current `DATA` statement is already read or when processing `RESTORE` statement. `next_line_num` argument variable can be equal to either `B1_T_LINE_NUM_FIRST` and `B1_T_LINE_NUM_NEXT` values or to a program line number identifing a program line with `DATA` statement. `B1_T_LINE_NUM_FIRST` and `B1_T_LINE_NUM_NEXT` constants correspond to the first and the next program lines with `DATA` statements. The function should return `B1_RES_ELINENNOTFND` code if the line number is not found and `B1_RES_EDATAEND` if there's no more `DATA` statements in the program. If the program line is found the function has to change `b1_data_curr_line_cnt` and `b1_data_curr_line_offset` variables properly. Note that the function should work with `b1_int_data_curr_line_*` global variables not with `b1_int_curr_prog_line_*` ones.  
  
`extern B1_T_ERROR b1_ex_prg_while_go_wend();`  
The function should find program line counter of a `WEND` statement corresponding to the current `WHILE` statement (identified with `b1_curr_prog_line_cnt` variable value). The resulting line counter should be written to the same `b1_curr_prog_line_cnt` variable. If the program line is not found the function should return `B1_RES_EWHILEWOWND` value. The function has to be implemented if `B1_FEATURE_STMT_WHILE_WEND` feature is enabled.  
  
See `./source/ext/exprg.cpp` file for possible functions implementation.  
  
### Localized string functions  
  
The functions are called if `B1_FEATURE_LOCALES` feature is enabled to perform locale-specific textual data transformations, search and comparison.  
  
`extern B1_T_CHAR b1_t_toupper_l(B1_T_CHAR c);`  
The function is called to convert a character to upper case. Non-alphabetic characters should be returned as is.  
  
`extern B1_T_CHAR b1_t_tolower_l(B1_T_CHAR c);`  
The function is called to convert a character to lower case. Non-alphabetic characters should be returned as is.  
  
`extern int8_t b1_t_strcmp_l(const B1_T_CHAR *s1, const B1_T_CHAR *s2data, B1_T_INDEX s2len);`  
The function should perform lexicographical case-insensitive strings comparison. The first string is represented with `s1` argument and the second with `s2data`, `s2len` arguments. Note that `s1` pointer is not a null character terminated C text string but a string in internal BASIC-1 interpreter format: the first character of the data stands for the string characters number and the textual data itself comes right after it (so entire data block size is 1 + `<string_length>` characters). `s2data` argument is a pointer to character sequence of the second string and `s2len` is the second string length in characters. The function should return zero if both strings are equal, a negative value if the first string is less than the second and a positive value if the first string is greater than the second.  
  
See `./source/ext/ext.cpp` file for possible functions implementation.  
  
### Expressions postfix notation caching functions  
  
BASIC1 interpreter transforms every expression to its postfix notation or reverse Polish notation (RPN) before evaluation. Building an expression's RPN is not a fast operation so it's very desirable to cache expressions already converted to RPN. The caching can make executing programs faster if there are many repeating code blocks such as subroutines and loops. Enable `B1_FEATURE_RPN_CACHING` macro definition in `feat.h` file to turn the caching functions usage on.  
  
`extern B1_T_ERROR b1_ex_prg_rpn_cache(B1_T_INDEX offset, B1_T_INDEX continue_offset);`  
The function should copy expression from `b1_rpn` variable and `continue_offset` value to the cache. The cache record identifier should consist from values of `b1_curr_prog_line_cnt` and `offset` variables. `b1_rpn` is a pointer to array of `B1_RPNREC` structures, the last structure in the array has `flags` member set to zero value. The function should copy entire array including the termionating structure.  
  
`extern B1_T_ERROR b1_ex_prg_rpn_get_cached(B1_T_INDEX offset, B1_T_INDEX *continue_offset);`  
The function is called by interpreter before building expression's RPN. The expression is identified with values of `b1_curr_prog_line_cnt` and `offset` variables and the function should provide data previously stored with `b1_ex_prg_rpn_cache` function call: expression continue offset value should be written at the address `continue_offset` pointer points at and `b1_rpn` global variable should be changed to point to the expression data (`B1_RPNREC` structures array). If the expression is not found in the cache the pointers have to be left unmodified.  
  
See `./source/ext/exprg.cpp` file for possible functions implementation.  
  