/*
 * tch_nclib.c
 *
 *  Created on: 2014. 7. 27.
 *      Author: manics99
 */

#include "tch_nclib.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>


const tch_stdio_ix STDIO_StaticObj =  {
#ifdef ANSI_LIBSET
	                                            remove,
                                                clearerr,
                                                fflush,
                                                fgetpos,
                                                fsetpos,
                                                ftell,
                                                fopen,
                                                freopen,
                                                rename,
                                                printf,
                                                fprintf,
                                                sprintf,
                                                snprintf,
                                                asprintf,
                                                asnprintf,
                                                scanf,
                                                fscanf,
                                                sscanf,
                                                tmpfile,
                                                tmpnam,
                                                ungetc,
                                                vprintf,
                                                vfprintf,
                                                vsprintf,
                                                vsnprintf,
                                                sniprintf,
                                                asiprintf,
                                                asniprintf,
	                                            rewind,
	                                            setbuf,
	                                            setvbuf,
	                                            fiscanf,
	                                            siscanf,
	                                            fiprintf,
	                                            siprintf,
#endif
#ifdef POSIX_LIBSET
	                                            fileno,
	                                            fmemopen,
	                                            mkstemp,
	                                            mkdtemp,
	                                            open_memstream,
	                                            open_wmemstream,
#endif
#ifdef GNUX_LIBSET
	                                            diprintf,
	                                            vdiprintf,
	                                            dprintf,
	                                            vdprintf,
	                                            fcloseall,
	                                            vscanf,
	                                            vfscanf,
	                                            vsscanf,
	                                            vwscanf,
	                                            vfwscanf,
	                                            vswscanf,
#endif
#ifdef C99_LIBSET
                                                fgetwc,
	                                            fgetws,
	                                            fputwc,
	                                            fputws,
	                                            getwchar,
	                                            putwchar,
	                                            wprintf,
	                                            fwprintf,
	                                            swprintf,
	                                            wscanf,
	                                            fwscanf,
	                                            swscanf,
	                                            vwprintf,
	                                            vfwprintf,
	                                            vswprintf,
#endif
#ifdef MFEATURE_FILESYSTEM
	                                            fclose,
	                                            fdopen,
	                                            feof,
	                                            ferror,
	                                            fgetc,
	                                            fgets,
	                                            fopencookie,
	                                            fputc,
	                                            fputs,
	                                            fread,
	                                            fseek,
	                                            fwrite,
	                                            getc,
	                                            putc,
#endif
	                                            getchar,
	                                            perror,
	                                            putchar,
	                                            puts,
	                                            gets,
	                                            iprintf,
	                                            iscanf
};

const tch_stdlib_ix STDLIB_StaticObj = {
#ifdef ANSI_LIBSET
		abort,
		atexit,
		atoll,
		div,
		ldiv,
		getenv,
		mblen,
		mbstowcs,
		mbtowc,
		strtoll,
		strtoul,
		strtoull,
		wcstod,
		wcstof,
		wcstol,
		wcstoll,
		wcstoul,
		wcstoull,
		system,
		wcstombs,
		wctomb,
		exit,
		strtod,
		strtof,
		strtol,
		bsearch,
		qsort,
#endif
#ifdef C99_LIBSET
		lldiv,
		llabs,
		wcsrtombs,
#endif
#ifdef POSIX_LIBSET
		mbsnrtowcs,
		wcsnrtombs,
#endif
		abs,
		atof,
		atoff,
		atoi,
		atol,
		calloc,
		free,
		malloc,
		realloc,
		labs,
		rand,
		srand
};


const tch_ctype_ix CTYPE_StaticObj = {
#ifdef ANSI_LIBSET
		isgraph,
		ispunct,
		isxdigit,
		toascii,
#endif
#ifdef POSIX_LIBSET
#endif
#ifdef C99_LIBSET
		iswalnum,
		iswalpha,
		iswcntrl,
		iswblank,
		iswdigit,
		iswxdigit,
		iswgraph,
		iswprint,
		iswspace,
		iswupper,
		towlower,
		towupper,
		iswctype,
		wctype,
		towctrans,
		wctrans,
#endif
		isalnum,
		isalpha,
		isascii,
		iscntrl,
		isdigit,
		islower,
		isprint,
		isspace,
		isupper,
		tolower,
		toupper
};


const tch_string_ix STRING_StaticObj = {
#ifdef GNUX_LIBSET
		memccpy,
		mempcpy,
		stpcpy,
		stpncpy,
		strnlen,
#endif
#ifdef ANSI_LIBSET
		strerror,
		strstr,
		strxfrm,
		strncpy,
		strrchr,
		strcspn,
		strcmp,
		strcpy,
#endif
#ifdef POSIX_LIBSET
		strsignal,
		wcscasecmp,
		wcsdup,
		wcsncasecmp,
#endif
		memchr,
		memcmp,
		memcpy,
		memmem,
		memmove,
		memset,
		strcat,
		strchr,
		strlen,
		strspn,
		strtok};


const tch_time_ix TIME_StaticObj = {
#ifdef ANSI_LIBSET
		asctime,
		clock,
		ctime,
		difftime,
		gmtime,
		localtime,
		mktime,
		strftime,
		time,
		tzset,
#endif
};

const tch_math_ix MATH_StaticObj = {

};

static tch_ulib_ix NLIB_Instance ;
const tch_ustdlib_ix* uStdLib = &NLIB_Instance;

tch_ustdlib_ix* tch_initCrt(void* arg){
	NLIB_Instance.stdio = &STDIO_StaticObj;
	NLIB_Instance.stdlib = &STDLIB_StaticObj;
	NLIB_Instance.string = &STRING_StaticObj;
	NLIB_Instance.ctype = &CTYPE_StaticObj;
	NLIB_Instance.time = &TIME_StaticObj;
	NLIB_Instance.math = &MATH_StaticObj;
	return &NLIB_Instance;
}


