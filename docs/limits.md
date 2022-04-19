# BASIC1 interpreter limits and default values  

Maximum length of program line: 255 characters  
Maximum number of program lines: 65535  
Maximum length of identifier: 31 characters  
Maximum length of string value: 127 characters  
Maximum number of subscripted variable dimensions: 3 (2 if `B1_FEATURE_3_DIM_ARRAYS` feature is disabled)  
Maximum number of function arguments: 3  
Maximum expression brackets nesting level: 7  
Maximum call nesting depth of user-defined functions: 3  
Maximum statements nesting depth (`IF`, `ON`, `GOSUB`, `FOR` and `WHILE`): 10  
  
Line number range: \[1 ... 65530\]  
  
Default subscript lower boundary value: 0  
Default subscript upper boundary value: 10  

Integer `INT` data type range: \[−2147483648 ... 2147483647\]  
Integer `INT16` data type range: \[−32768 ... 32767\]  
Integer `WORD` data type range: \[0 ... 65535\]  
Integer `BYTE` data type range: \[0 ... 255\]  
Floating-point `SINGLE` data type range: \[±1.1754943e−38 ... ±3.4028234e38\]  
Floating-point `DOUBLE` data type range: \[±2.2250738585072014e−308 ... ±1.7976931348623157e308\]  
Character type range (ANSI version): \[0 ... 255\]  
Character type range (Unicode UCS-2 version): \[0 ... 65535\]  
Subscript range: \[-8388608 ... 8388607\]  
  