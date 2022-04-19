/*
 BASIC1 interpreter
 Copyright (c) 2020 Nikolay Pletnev
 MIT license

 ext.cpp: external functions to support locales (upper case, lower case,
 string comparison)
*/


#include <cstdint>
#include <cstdlib>

extern "C"
{
#include "b1types.h"
#include "b1err.h"
}

#ifdef B1_FEATURE_LOCALES
#ifdef B1_FEATURE_UNICODE_UCS2
#include <cwchar>
#else
#include <cstring>
#endif
#endif


#ifdef B1_FEATURE_LOCALES
#ifdef B1_FEATURE_UNICODE_UCS2
extern "C" B1_T_CHAR b1_t_toupper_l(B1_T_CHAR c)
{
	return (B1_T_CHAR)towupper((wint_t)c);
}

extern "C" B1_T_CHAR b1_t_tolower_l(B1_T_CHAR c)
{
	return (B1_T_CHAR)towlower((wint_t)c);
}

extern "C" int8_t b1_t_strcmp_l(const B1_T_CHAR *s1, const B1_T_CHAR *s2data, B1_T_INDEX s2len)
{
	wchar_t *str1, *str2;
	B1_T_INDEX i, len;
	int8_t res;

	len = *s1;
	str1 = new wchar_t[len + 1];
	if(str1 == NULL)
	{
		return B1_RES_ENOMEM;
	}
	for(i = 0; i < len; i++)
	{
		s1++;
		*(str1 + i) = B1_T_TOLOWER_L(*s1);
	}
	*(str1 + i) = 0;

	len = s2len;
	str2 = new wchar_t[len + 1];
	if(str2 == NULL)
	{
		delete[] str1;
		return B1_RES_ENOMEM;
	}
	for(i = 0; i < len; i++)
	{
		*(str2 + i) = B1_T_TOLOWER_L(*s2data);
		s2data++;
	}
	*(str2 + i) = 0;

	res = (int8_t)wcscoll(str1, str2);

	delete[] str2;
	delete[] str1;

	return res;
}
#else
extern "C" B1_T_CHAR b1_t_toupper_l(B1_T_CHAR c)
{
	return (B1_T_CHAR)toupper((char)c);
}

extern "C" B1_T_CHAR b1_t_tolower_l(B1_T_CHAR c)
{
	return (B1_T_CHAR)tolower((char)c);
}

extern "C" int8_t b1_t_strcmp_l(const B1_T_CHAR *s1, const B1_T_CHAR *s2data, B1_T_INDEX s2len)
{
	char *str1, *str2;
	B1_T_INDEX i, len;
	int8_t res;

	len = *s1;
	str1 = new char[len + 1];
	if(str1 == NULL)
	{
		return B1_RES_ENOMEM;
	}
	for(i = 0; i < len; i++)
	{
		s1++;
		*(str1 + i) = B1_T_TOLOWER_L(*s1);
	}
	*(str1 + i) = 0;

	len = s2len;
	str2 = new char[len + 1];
	if(str2 == NULL)
	{
		delete[] str1;
		return B1_RES_ENOMEM;
	}
	for(i = 0; i < len; i++)
	{
		*(str2 + i) = B1_T_TOLOWER_L(*s2data);
		s2data++;
	}
	*(str2 + i) = 0;

	res = (int8_t)strcoll(str1, str2);

	delete[] str2;
	delete[] str1;

	return res;
}
#endif
#endif
