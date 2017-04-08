/*
 * string.c
 *
 *  Created on: 2015. 10. 3.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/string.h"
#include <stdarg.h>

#define FORMAT_DELIM			'%'
#define STR_TERM				'\0'


const char NUM_CHAR[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e','f'
};


void mset(void* dst,int v,size_t sz)
{
	if(!dst)
		return;
	while(sz--) ((uint8_t*) dst)[sz] = v;
}

void mcpy(void* dst,const void* src,size_t n)
{
	if(!dst || !src)
		return;

	while(n--)((uint8_t*) dst)[n] = ((uint8_t*) src)[n];
}

int mcmp(const void* s1,const void* s2,size_t n)
{
	if(!s1 || !s2)
		return -1;
	size_t i;
	uint8_t v;
	for(i = 0;i < n; i++){
		if(((v = ((uint8_t*) s1)[i] - ((uint8_t*) s2)[i])) != 0)
			return v < 0? -1 : 1;
	}
	return 0;
}

char* strchar(const char* s,int c)
{
	uint32_t idx = 0;
	while(s[idx] != '\0')
	{
		if(s[idx] == c)
			return (char*) &s[idx];
		idx++;
	}
	if(c == '\0')
	{
		return (char*) &s[idx];
	}
	return NULL;
}

size_t strcopy(char* dst,char* src){
	if(!dst || !src)
		return 0;
	char* init_dst = dst;
	while(*src != '\0'){
		*(dst++) = *(src++);
	}
	return (size_t) (dst - init_dst);
}

size_t ftostr(float val,char* str,int trunc){
	if(!str)
		return 0;
	float div = 10.f;
	char* init_str = str;
	str[0] = NUM_CHAR[(int) val];
	str[1] = '.';
	str = &str[2];
	while(trunc--){
		val -= (int) val;
		val *= div;
		*(str++) = NUM_CHAR[(int) val];
	}
	return (size_t) (str - init_str);
}

size_t itostr(int val,char* str,int radix)
{
	if(!str || (radix < 2))
		return 0;
	char* init_str = str;
	if(val < radix)
	{
		*str = NUM_CHAR[val];
		str++;
		return (size_t) (str - init_str);
	}
	int v = val;
	int div = radix;
	while(div != 1)
	{
		if((v / div) >= radix)
		{
			div *= radix;
		}
		else
		{
			*str = NUM_CHAR[v / div];
			str++;
			v %= div;
			div /= radix;
		}
	}
	*(str++) = NUM_CHAR[v];
	return (size_t) (str - init_str);
}

size_t vformat(char* dst,const char* fmt,va_list args){
	if(!dst)
		return 0;
	char *cp,*lcp;
	size_t sz = 0;
	uint32_t gap;
	const char* eos = strchar(fmt,STR_TERM);
	lcp = cp = (char*) fmt;
	while((cp < eos) && cp)
	{
		cp = (char*) strchar(lcp,FORMAT_DELIM);
		if(!cp)
		{
			sz += strcopy(dst,lcp);
			return sz;
		}
		gap = ((uint32_t) cp - (uint32_t) lcp);
		sz += gap;
		mcpy(dst,lcp , ((size_t) cp - (size_t) lcp) * sizeof(char));
		dst = &dst[gap];
		lcp = &cp[2];
		switch(cp[1]){
		case 'd':
			dst = &dst[(gap = itostr(va_arg(args,int),dst,10))];
			sz += gap;
			break;
		case 'f':
			if(cp[2] == '.' && isdigit(cp[3]))
			{
				dst = &dst[(gap = ftostr((float) va_arg(args,double),dst,cp[3] - 48))];
				lcp = &cp[4];
			}
			else
			{
				dst = &dst[(gap = ftostr((float) va_arg(args,double),dst,5))];
			}
			sz += gap;
			break;
		case 's':
			dst = &dst[(gap = strcopy(dst,va_arg(args,char*)))];
			sz += gap;
			break;
		case 'c':
			*(dst++) = (char) va_arg(args,int);
			sz++;
			break;
		}
	}
	return sz;
}

size_t format(char* dest,const char* fmt,...)
{
	if(!dest || !fmt)
		return 0;
	va_list args;
	va_start(args,fmt);
	size_t sz = vformat(dest,fmt,args);
	va_end(args);
	return sz;
}
