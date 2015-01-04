/*
 * tch_nclib.h
 *
 *  Created on: 2014. 7. 27.
 *      Author: manics99
 */

#ifndef TCH_NCLIB_H_
#define TCH_NCLIB_H_


#include "tch.h"
#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <wctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C"{
#endif


#if !defined(MICRO_LIBSET) && !defined(GNUX_LIBSET) && !defined(ANSI_LIBSET) && !defined(POSIX_LIBSET) && !defined(C99_LIBSET)
#define MICRO_LIBSET
#endif

/*!
 * \brief c standard I/O library structure
 *
 *  This Structure contains c standard I/O library functions pointer.
 *  Each pointer is linked to std lib function built statically with platform binary
 *  By default, only minimal set is available.
 *  if more std I/O lib functions are required, platform binary should be rebuilt with
 *  additional macro definition.
 *
 * \see MICRO_LIBSET
 * \see GNUX_LIBSET
 * \see ANSI_LIBSET
 * \see POSIX_LIBSET
 */
typedef struct _tch_stdio_ix_t {
#ifdef ANSI_LIBSET
	int (*remove)(char* filename);
	void (*clearerr)(FILE* fp);
	int (*fflush)(FILE* fp);
	int (*fgetpos)(FILE *fp,fpos_t* pos);
	int (*fsetpos)(FILE* fp,const fpos_t* pos);
	long (*ftell)(FILE* fp);
	FILE* (*fopen)(const char* file,const char* mode);
	FILE* (*freopen)(const char* file,const char* mode,FILE* fp);
	int (*rename)(const char* old,const char* ne);
	int (*printf)(const char* format,...);
	int (*fprintf)(FILE* fd,const char* format,...);
	int (*sprintf)(char* str,const char* format,...);
	int (*snprintf)(char* str,size_t size,const char* format,...);
	int (*asprintf)(char** strp,const char* format,...);
	char* (*asnprintf)(char* str,size_t* size,const char* format,...);
	int (*scanf)(const char* format,...);
	int (*fscanf)(FILE* fd,const char* format,...);
	int (*sscanf)(const char* str,const char* format,...);
	FILE* (*tmpfile)(void);
	char* (*tmpnam)(char* str);
	int (*ungetc)(int c,FILE* stream);
	int (*vprintf)(const char* fmt,va_list list);
	int (*vfprintf)(FILE* fp,const char* fmt,va_list list);
	int (*vsprintf)(char* str, const char* fmt,va_list list);
	int (*vsnprintf)(char* str,size_t size, const char* fmt,va_list list);
	int (*sniprintf)(char* str,size_t size,const char* format,...);
	int (*asiprintf)(char** strp,const char* format,...);
	char* (*asniprintf)(char* str,size_t* size,const char* format,...);
	void (*rewind)(FILE* fp);
	void (*setbuf)(FILE* fp, char* buf);
	int (*setvbuf)(FILE* fp, char* buf,int mode,size_t size);
	int (*fiscanf)(FILE* fd,const char* format,...);
//	int (*siscanf)(const char* str, const char* format,...);
	int (*fiprintf)(FILE* fd,const char* format,...);
//	int (*siprintf)(char* str,const char* format,...);
#endif
#ifdef POSIX_LIBSET
	int (*fileno)(FILE* fp);
	FILE* (*fmemopen)(void* buf,size_t size,const char* mode);
	int (*mkstemp)(char* path);
	char* (*mkdtemp)(char* path);
	FILE* (*open_memstream)(char** buf,size_t* size);
	FILE* (*open_wmemstream)(wchar_t** buf,size_t* size);
#endif
#ifdef GNUX_LIBSET
	int (*diprintf)(int fd,const char* format,...);
	int (*vdiprintf)(int fd,const char* format,...);
	int (*dprintf)(int fd,const char* format,...);
	int (*vdprintf)(int fd,const char* format,...);
	int (*fcloseall)(void);
	int (*vscanf)(const char* fmt, va_list list);
	int (*vfscanf)(FILE* fp,const char* fmt,va_list list);
	int (*vsscanf)(const char* str,const char* fmt,va_list list);
	int (*vwscanf)(const wchar_t* fmt,va_list list);
	int (*vfwscanf)(FILE* fp,const wchar_t* fmt,va_list list);
	int (*vswscanf)(const wchar_t* str,const wchar_t *fmt,va_list list);

#endif
#ifdef C99_LIBSET
	int (*fgetwc)(FILE* fp);
	wchar_t* (*fgetws)(wchar_t *ws,int n,FILE* fp);
	wint_t (*fputwc)(wchar_t wc,FILE* fp);
	int (*fputws)(const wchar_t* ws,FILE* fp);
	wint_t (*getwchar)(void);
	wint_t (*putwchar)(wchar_t wc);
	int (*wprintf)(const wchar_t* format,...);
	int (*fwprintf)(FILE* fp,const wchar_t* format,...);
	int (*swprintf)(wchar_t* str,size_t size,const wchar_t* format,...);
	int (*wscanf)(const wchar_t* format,...);
	int (*fwscanf)(FILE* fp,const wchar_t* format,...);
	int (*swscanf)(const wchar_t* str,const wchar_t* format,...);
	int (*vwprintf)(const wchar_t* fmt,va_list list);
	int (*vfwprintf)(FILE* fp,const wchar_t* fmt,va_list list);
	int (*vswprintf)(wchar_t* str,size_t size,const wchar_t* fmt,va_list list);
#endif
#ifdef MFEATURE_FILESYSTEM
	int (*fclose)(FILE* fp);
	FILE* (*fdopen)(int fd,const char* mode);
	int (*feof)(FILE* fp);
	int (*ferror)(FILE* fp);
	int (*fgetc)(FILE* fp);
	char* (*fgets)(char*__restrict buf,int n,FILE*__restrict fp);
	FILE* (*fopencookie)(void* cookie,const char* mode,cookie_io_functions_t funcs);
	int (*fputc)(int ch,FILE* fp);
	int (*fputs)(const char* s,FILE* fp);
	size_t (*fread)(void* buf,size_t size,size_t count,FILE* fp);
	int (*fseek)(FILE* fp,long offset,int whence);
	size_t (*fwrite)(const void* buf,size_t size,size_t count,FILE* fp);
	int (*getc)(FILE* fp);
	int (*putc)(int ch,FILE* fp);
#endif
	int (*getchar)(void);
	void (*perror)(const char* prefix);
	int (*putchar)(int ch);
	int (*puts)(const char* s);
	char* (*gets)(char* buf);
	int (*siscanf)(const char* str, const char* format,...);
	int (*siprintf)(char* str,const char* format,...);
	int (*iprintf)(const char* format,...);
	int (*iscanf)(const char* format,...);
}tch_stdio_ix;

typedef struct _tch_string_ix_t {
#ifdef GNUX_LIBSET
	void* (*memccpy)(void* out,const void* in, int endchar,size_t n);
	void* (*mempcpy)(void* out,const void* in,size_t n);
	char* (*stpcpy)(char *dst, const char *src);
	char* (*stpncpy)(char* dst,const char* src,size_t len);
	size_t (*strnlen)(const char* str,size_t n);
#endif
#ifdef ANSI_LIBSET
	char* (*strerror)(int errnum);
	char* (*strstr)(const char* s1, const char* s2);
	size_t (*strxfrm)(char* s1, const char* s2,size_t n);
	char* (*strncpy)(char*__restrict dst,const char*__restrict src,size_t len);
	char* (*strrchr)(const char* string,int c);
	size_t (*strcspn)(const char* s1,const char* s2);
	int (*strcmp)(const char* str,const char* b);
	char* (*strcpy)(char* dst,const char* src);
#endif
#ifdef POSIX_LIBSET
	char* (*strsignal)(int signal);
	int (*wcscasecmp)(const wchar_t* a,const wchar_t* b);
	wchar_t* (*wcsdup)(const wchar_t* str);
	int (*wcsncasecmp)(const wchar_t* a,const wchar_t* b,size_t length);
#endif
	void* (*memchr)(const void* src,int c, size_t length);
	int (*memcmp)(const void* s1,const void* s2,size_t n);
	void* (*memcpy)(void* out,const void* in,size_t n);
	void* (*memmove)(void* dst,const void* src,size_t len);
	void* (*memset)(void* dst,int c,size_t length);
	char* (*strcat)(char* dst, const char* src);
	char* (*strchr)(const char* str,int c);
	size_t (*strlen)(const char* str);
	size_t (*strspn)(const char* s1,const char* s2);
	char* (*strtok)(char*__restrict src,const char*__restrict del);
}tch_string_ix;

typedef struct _tch_stdlib_ix_t{
#ifdef ANSI_LIBSET
	void (*abort)(void);
	int (*atexit)(void (*func)(void));
	long long (*atoll)(const char* str);
	div_t (*div)(int n,int d);
	ldiv_t (*ldiv)(long n,long d);
	char* (*getenv)(const char* name);
	int (*mblen)(const char* s, size_t n);
	int (*mbstowcs)(wchar_t* pwc,const char* s,size_t n);
	int (*mbtowc)(wchar_t* pwc,const char* s, size_t n);
	long long (*strtoll)(const char* s,char** ptr,int base);
	unsigned long (*strtoul)(const char* str,char** ptr,int base);
	unsigned long long (*strtoull)(const char* str, char** ptr,int base);
	double (*wcstod)(const wchar_t* str,wchar_t** tail);
	float (*wcstof)(const wchar_t* str,wchar_t** tail);
	long (*wcstol)(const wchar_t* str,wchar_t** ptr,int base);
	long long (*wcstoll)(const wchar_t* str,wchar_t** ptr,int base);
	unsigned long (*wcstoul)(const wchar_t *s, wchar_t **ptr,int base);
	unsigned long long (*wcstoull)(const wchar_t *s, wchar_t **ptr,int base);
	int (*system)(char* s);
	size_t (*wcstombs)(char* s,const wchar_t* pwc,size_t n);
	int (*wctomb)(char* s, wchar_t wchar);
	void (*exit)(int code);
	double (*strtod)(const char* str, char** tail);
	float (*strtof)(const char* str,char** tail);
	long (*strtol)(const char* str,char** ptr,int base);
	void* (*bsearch)(const void* key,const void* base,size_t nitems,size_t size,int (*compare)(const void*, const void*));
	void (*qsort)(void* base,size_t nitems,size_t size,int (*compare)(const void*,const void*));
#endif
#ifdef C99_LIBSET
	lldiv_t (*lldiv)(long long n, long long d);
	long long (*llabs)(long long j);
	size_t (*wcsrtombs)(char *dst,const wchar_t** src,size_t len,mbstate_t* ps);
#endif
#ifdef POSIX_LIBSET
	size_t (*mbsnrtowcs)(wchar_t* dst,const char** src,size_t nms,size_t len,mbstate_t* ps);
	size_t (*wcsnrtombs)(char* dst,const wchar_t** src,size_t nwc,size_t len,mbstate_t* ps);
#endif
	int (*abs)(int x);
	double (*atof)(const char* str);
	float (*atoff)(const char* str);
	int (*atoi)(const char* str);
	long int (*atol)(const char* str);
	void* (*calloc)(size_t nitems,size_t size);
	void (*free)(void* ptr);
	void* (*malloc)(size_t size);
	void* (*realloc)(void* ptr,size_t size);
	long int (*labs)(long int x);
	int (*rand)(void);
	void (*srand)(unsigned seed);
}tch_stdlib_ix;

typedef struct _tch_math_ix_t {
	void* dummy;
}tch_math_ix;

typedef struct _tch_ctype_ix_t {
#ifdef ANSI_LIBSET
	int (*isgraph)(int c);
	int (*ispunct)(int c);
	int (*isxdigit)(int c);
	int (*toascii)(int c);
#endif
#ifdef POSIX_LIBSET
#endif
#ifdef C99_LIBSET
	int (*iswalnum)(wint_t c);
	int (*iswalpha)(wint_t c);
	int (*iswcntrl)(wint_t c);
	int (*iswblank)(wint_t c);
	int (*iswdigit)(wint_t c);
	int (*iswxdigit)(wint_t c);
	int (*iswgraph)(wint_t c);
	int (*iswprint)(wint_t c);
	int (*iswspace)(wint_t c);
	int (*iswupper)(wint_t c);
	wint_t (*towlower)(wint_t c);
	wint_t (*towupper)(wint_t c);
	int (*iswctype)(wint_t c,wctype_t desc);
	wctype_t (*wctype)(const char* c);
	wint_t (*towctrans)(wint_t c,wctrans_t w);
	wctrans_t (*wctrans)(const char* c);
#endif
	int (*isalnum)(int c);
	int (*isalpha)(int c);
	int (*isascii)(int c);
	int (*isctrl)(int c);
	int (*isdigit)(int c);
	int (*islower)(int c);
	int (*isprint)(int c);
	int (*isspace)(int c);
	int (*isupper)(int c);
	int (*tolower)(int c);
	int (*toupper)(int c);
}tch_ctype_ix;

typedef struct _tch_time_ix_t{
#ifdef ANSI_LIBSET
	char* (*asctime)(const struct tm* clock);
	clock_t (*clock)(void);
	char* (*ctime)(const time_t* clock);
	double (*difftime)(time_t tim1,time_t tim2);
	struct tm* (*gmtime)(const time_t *clock);
	struct tm* (*localtime)(time_t* clock);
	time_t (*mktime)(struct tm* timp);
	size_t (*strftime)(char* s,size_t maxsize,const char* fmt,const struct tm* timp);
	time_t (*time)(time_t* t);
	void (*tzset)(void);
#endif
}tch_time_ix;

typedef struct _tch_ustdl_ix_t {
	const tch_stdio_ix* stdio;
	const tch_stdlib_ix* stdlib;
	const tch_string_ix* string;
	const tch_math_ix* math;
	const tch_ctype_ix* ctype;
	const tch_time_ix* time;
}tch_ulib_ix;


extern tch_ustdlib_ix* tch_initStdLib(void);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_NCLIB_H_ */
