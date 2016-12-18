/* glob.c -- file-name wildcard pattern matching for Bash.

 Copyright (C) 1985-2005 Free Software Foundation, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA.  */

/* To whomever it may concern: I have never seen the code which most
 Unix programs use to perform this function.  I wrote this from scratch
 based on specifications for the pattern matching.  --RMS.  */

//#include <config.h>
#if !defined (__GNUC__) && !defined (HAVE_ALLOCA_H) && defined (_AIX)
#pragma alloca
#endif /* _AIX && RISC6000 && !__GNUC__ */

//#include "bashtypes.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "config.h"
#include <stdlib.h>  //for  MB_CUR_MAX
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <alloca.h>
#include "ff_glob.h"
//#include <dirent.h> //for DIR

#if defined (HAVE_DIRENT_H)
#  include <dirent.h>
#  if defined (HAVE_STRUCT_DIRENT_D_NAMLEN)
#    define D_NAMLEN(d)	((d)->d_namlen)
#  else
#    define D_NAMLEN(d)   (strlen ((d)->d_name))
#  endif /* !HAVE_STRUCT_DIRENT_D_NAMLEN */
#else
#  if defined (HAVE_SYS_NDIR_H)
#    include <sys/ndir.h>
#  endif
#  if defined (HAVE_SYS_DIR_H)
#    include <sys/dir.h>
#  endif
#  if defined (HAVE_NDIR_H)
#    include <ndir.h>
#  endif
#  if !defined (dirent)
#    define dirent direct
#  endif /* !dirent */
#  define D_NAMLEN(d)   ((d)->d_namlen)
#endif /* !HAVE_DIRENT_H */

///* standard flags are like fnmatch(3). */
//#define	FNM_PATHNAME	(1 << 0) /* No wildcard can ever match `/'.  */
//#define	FNM_NOESCAPE	(1 << 1) /* Backslashes don't quote special chars.  */
//#define	FNM_PERIOD	(1 << 2) /* Leading `.' is matched only explicitly.  */
//
//#define	FNM_NOMATCH	1

//#include "bashansi.h"
//#include "posixdir.h"
//#include "posixstat.h"
//#include "shmbutil.h"
//#include "xmalloc.h"
//
//#include "filecntl.h"
#if !defined (F_OK)
#  define F_OK 0
#endif

//#include "stdc.h"
//#include "memalloc.h"
//#include "quit.h"
//
//#include "glob.h"
//#include "strmatch.h"

#include <ctype.h>

/* Jim Meyering writes:

 "... Some ctype macros are valid only for character codes that
 isascii says are ASCII (SGI's IRIX-4.0.5 is one such system --when
 using /bin/cc or gcc but without giving an ansi option).  So, all
 ctype uses should be through macros like ISPRINT...  If
 STDC_HEADERS is defined, then autoconf has verified that the ctype
 macros don't need to be guarded with references to isascii. ...
 Defining IN_CTYPE_DOMAIN to 1 should let any compiler worth its salt
 eliminate the && through constant folding."
 Solaris defines some of these symbols so we must undefine them first.  */

#if defined STDC_HEADERS || (!defined isascii && !defined HAVE_ISASCII)
#  define IN_CTYPE_DOMAIN(c) 1
#else
#  define IN_CTYPE_DOMAIN(c) isascii(c)
#endif

#if !defined (isspace) && !defined (HAVE_ISSPACE)
#  define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\f')
#endif

#if !defined (isprint) && !defined (HAVE_ISPRINT)
#  define isprint(c) (isalpha(c) || isdigit(c) || ispunct(c))
#endif

#if defined (isblank) || defined (HAVE_ISBLANK)
#  define ISBLANK(c) (IN_CTYPE_DOMAIN (c) && isblank (c))
#else
#  define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#endif

#if defined (isgraph) || defined (HAVE_ISGRAPH)
#  define ISGRAPH(c) (IN_CTYPE_DOMAIN (c) && isgraph (c))
#else
#  define ISGRAPH(c) (IN_CTYPE_DOMAIN (c) && isprint (c) && !isspace (c))
#endif

#if !defined (isxdigit) && !defined (HAVE_ISXDIGIT)
#  define isxdigit(c)	(((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#endif

#undef ISPRINT

#define ISPRINT(c) (IN_CTYPE_DOMAIN (c) && isprint (c))
#define ISDIGIT(c) (IN_CTYPE_DOMAIN (c) && isdigit (c))
#define ISALNUM(c) (IN_CTYPE_DOMAIN (c) && isalnum (c))
#define ISALPHA(c) (IN_CTYPE_DOMAIN (c) && isalpha (c))
#define ISCNTRL(c) (IN_CTYPE_DOMAIN (c) && iscntrl (c))
#define ISLOWER(c) (IN_CTYPE_DOMAIN (c) && islower (c))
#define ISPUNCT(c) (IN_CTYPE_DOMAIN (c) && ispunct (c))
#define ISSPACE(c) (IN_CTYPE_DOMAIN (c) && isspace (c))
#define ISUPPER(c) (IN_CTYPE_DOMAIN (c) && isupper (c))
#define ISXDIGIT(c) (IN_CTYPE_DOMAIN (c) && isxdigit (c))

#define ISLETTER(c)	(ISALPHA(c))

#define DIGIT(c)	((c) >= '0' && (c) <= '9')

#define ISWORD(c)	(ISLETTER(c) || DIGIT(c) || ((c) == '_'))

#define HEXVALUE(c) \
		(((c) >= 'a' && (c) <= 'f') \
				? (c)-'a'+10 \
						: (c) >= 'A' && (c) <= 'F' ? (c)-'A'+10 : (c)-'0')

#ifndef ISOCTAL
#  define ISOCTAL(c)	((c) >= '0' && (c) <= '7')
#endif
#define OCTVALUE(c)	((c) - '0')

#define TODIGIT(c)	((c) - '0')
#define TOCHAR(c)	((c) + '0')

#define TOLOWER(c)	(ISUPPER(c) ? tolower(c) : (c))
#define TOUPPER(c)	(ISLOWER(c) ? toupper(c) : (c))

#ifndef TOCTRL
/* letter to control char -- ASCII.  The TOUPPER is in there so \ce and
 \cE will map to the same character in $'...' expansions. */
#  define TOCTRL(x)	(TOUPPER(x) & 037)
#endif
#ifndef UNCTRL
/* control char to letter -- ASCII */
#  define UNCTRL(x)	(TOUPPER((x) | 0x40))
#endif

/* We #undef these before defining them because some losing systems
 (HP-UX A.08.07 for example) define these in <unistd.h>.  */
#undef  FNM_PATHNAME
#undef  FNM_NOESCAPE
#undef  FNM_PERIOD

/* Bits set in the FLAGS argument to `strmatch'.  */

/* standard flags are like fnmatch(3). */
#define	FNM_PATHNAME	(1 << 0) /* No wildcard can ever match `/'.  */
#define	FNM_NOESCAPE	(1 << 1) /* Backslashes don't quote special chars.  */
#define	FNM_PERIOD	(1 << 2) /* Leading `.' is matched only explicitly.  */

/* extended flags not available in most libc fnmatch versions, but we undef
 them to avoid any possible warnings. */
#undef FNM_LEADING_DIR
#undef FNM_CASEFOLD
#undef FNM_EXTMATCH

#define FNM_LEADING_DIR	(1 << 3) /* Ignore `/...' after a match. */
#define FNM_CASEFOLD	(1 << 4) /* Compare without regard to case. */
#define FNM_EXTMATCH	(1 << 5) /* Use ksh-like extended matching. */

/* Value returned by `strmatch' if STRING does not match PATTERN.  */
#undef FNM_NOMATCH

#define	FNM_NOMATCH	1

#define GX_MARKDIRS	0x01	/* mark directory names with trailing `/' */
#define GX_NOCASE	0x02	/* ignore case */
#define GX_MATCHDOT	0x04	/* match `.' literally */

#if !defined (HAVE_BCOPY) && !defined (bcopy)
#  define bcopy(s, d, n) ((void) memcpy ((d), (s), (n)))
#endif /* !HAVE_BCOPY && !bcopy */

#if !defined (NULL)
#  if defined (__STDC__)
#    define NULL ((void *) 0)
#  else
#    define NULL 0x0
#  endif /* __STDC__ */
#endif /* !NULL */

#if !defined (FREE)
#  define FREE(x)	if (x) free (x)
#endif

/* Don't try to alloca() more than this much memory for `struct globval'
 in glob_vector() */
#ifndef ALLOCA_MAX
#  define ALLOCA_MAX	100000
#endif

//extern void throw_to_top_level __P((void));
//extern int sh_eaccess __P((char *, int));

//extern int extended_glob;
//#include "xmbsrtowcs.c"

#ifndef MB_INVALIDCH
#define MB_INVALIDCH(x)		(0)
#define MB_NULLWCH(x)		(0)
#endif

#define STREQ(a, b) ((a)[0] == (b)[0] && strcmp(a, b) == 0)
#define STREQN(a, b, n) ((a)[0] == (b)[0] && strncmp(a, b, n) == 0)
#  define STREQ_W(s1, s2) ((wcscmp (s1, s2) == 0))
#  define STREQN_W(a, b, n) ((a)[0] == (b)[0] && wcsncmp(a, b, n) == 0)

#if HANDLE_MULTIBYTE
/* On some locales (ex. ja_JP.sjis), mbsrtowc doesn't convert 0x5c to U<0x5c>.
 So, this function is made for converting 0x5c to U<0x5c>. */

static mbstate_t local_state;
static int local_state_use = 0;

static size_t xmbsrtowcs(wchar_t *dest, const char **src, size_t len, mbstate_t *pstate) {
	mbstate_t *ps;
	size_t mblength, wclength, n;

	ps = pstate;
	if (pstate == NULL) {
		if (!local_state_use) {
			memset(&local_state, '\0', sizeof(mbstate_t));
			local_state_use = 1;
		}
		ps = &local_state;
	}

	n = strlen(*src);

	if (dest == NULL) {
		wchar_t *wsbuf;
		const char *mbs;
		mbstate_t psbuf;

		/* It doesn't matter if malloc fails here, since mbsrtowcs should do
		 the right thing with a NULL first argument. */
		wsbuf = (wchar_t *) malloc((n + 1) * sizeof(wchar_t));
		mbs = *src;
		psbuf = *ps;

		wclength = mbsrtowcs(wsbuf, &mbs, n, &psbuf);

		if (wsbuf)
			free(wsbuf);
		return wclength;
	}

	for (wclength = 0; wclength < len; wclength++, dest++) {
		if (mbsinit(ps)) {
			if (**src == '\0') {
				*dest = L'\0';
				*src = NULL;
				return (wclength);
			} else if (**src == '\\') {
				*dest = L'\\';
				mblength = 1;
			} else
				mblength = mbrtowc(dest, *src, n, ps);
		} else
			mblength = mbrtowc(dest, *src, n, ps);

		/* Cannot convert multibyte character to wide character. */
		if (mblength == (size_t) -1 || mblength == (size_t) -2)
			return (size_t) -1;

		*src += mblength;
		n -= mblength;

		/* The multibyte string  has  been  completely  converted,
		 including  the terminating '\0'. */
		if (*dest == L'\0') {
			*src = NULL;
			break;
		}
	}

	return (wclength);
}

/* Convert a multibyte string to a wide character string. Memory for the
 new wide character string is obtained with malloc.

 The return value is the length of the wide character string. Returns a
 pointer to the wide character string in DESTP. If INDICESP is not NULL,
 INDICESP stores the pointer to the pointer array. Each pointer is to
 the first byte of each multibyte character. Memory for the pointer array
 is obtained with malloc, too.
 If conversion is failed, the return value is (size_t)-1 and the values
 of DESTP and INDICESP are NULL. */

#define WSBUF_INC 32
//    wchar_t **destp;	/* Store the pointer to the wide character string */
//    char ***indicesp;	/* Store the pointer to the pointer array. */
//    const char *src;	/* Multibyte character string */

static size_t xdupmbstowcs(wchar_t **destp, char ***indicesp, const char *src) {
	const char *p; /* Conversion start position of src */
	wchar_t wc; /* Created wide character by conversion */
	wchar_t *wsbuf; /* Buffer for wide characters. */
	char **indices; /* Buffer for indices. */
	size_t wsbuf_size; /* Size of WSBUF */
	size_t wcnum; /* Number of wide characters in WSBUF */
	mbstate_t state; /* Conversion State */

	/* In case SRC or DESP is NULL, conversion doesn't take place. */
	if (src == NULL || destp == NULL) {
		if (destp)
			*destp = NULL;
		return (size_t) -1;
	}

	memset(&state, '\0', sizeof(mbstate_t));
	wsbuf_size = WSBUF_INC;

	wsbuf = (wchar_t *) malloc(wsbuf_size * sizeof(wchar_t));
	if (wsbuf == NULL) {
		*destp = NULL;
		return (size_t) -1;
	}

	indices = (char **) malloc(wsbuf_size * sizeof(char *));
	if (indices == NULL) {
		free(wsbuf);
		*destp = NULL;
		return (size_t) -1;
	}

	p = src;
	wcnum = 0;
	do {
		size_t mblength; /* Byte length of one multibyte character. */

		if (mbsinit(&state)) {
			if (*p == '\0') {
				wc = L'\0';
				mblength = 1;
			} else if (*p == '\\') {
				wc = L'\\';
				mblength = 1;
			} else
				mblength = mbrtowc(&wc, p, MB_LEN_MAX, &state);
		} else
			mblength = mbrtowc(&wc, p, MB_LEN_MAX, &state);

		/* Conversion failed. */
		if (MB_INVALIDCH(mblength)) {
			free(wsbuf);
			free(indices);
			*destp = NULL;
			return (size_t) -1;
		}

		++wcnum;

		/* Resize buffers when they are not large enough. */
		if (wsbuf_size < wcnum) {
			wchar_t *wstmp;
			char **idxtmp;

			wsbuf_size += WSBUF_INC;

			wstmp = (wchar_t *) realloc(wsbuf, wsbuf_size * sizeof(wchar_t));
			if (wstmp == NULL) {
				free(wsbuf);
				free(indices);
				*destp = NULL;
				return (size_t) -1;
			}
			wsbuf = wstmp;

			idxtmp = (char **) realloc(indices, wsbuf_size * sizeof(char **));
			if (idxtmp == NULL) {
				free(wsbuf);
				free(indices);
				*destp = NULL;
				return (size_t) -1;
			}
			indices = idxtmp;
		}

		wsbuf[wcnum - 1] = wc;
		indices[wcnum - 1] = (char *) p;
		p += mblength;
	} while (MB_NULLWCH (wc) == 0);

	/* Return the length of the wide character string, not including `\0'. */
	*destp = wsbuf;
	if (indicesp != NULL)
		*indicesp = indices;
	else
		free(indices);

	return (wcnum - 1);
}

#endif

/* Global variable which controls whether or not * matches .*.
 Non-zero means don't match .*.  */
int noglob_dot_filenames = 1;

/* Global variable which controls whether or not filename globbing
 is done without regard to case. */
int glob_ignore_case = 0;

/* Global variable to return to signify an error in globbing. */
char *glob_error_return;

/* Some forward declarations. */
//static int skipname __P((char *, char *));
//#if HANDLE_MULTIBYTE
//static int mbskipname __P((char *, char *));
//#endif
//static void dequote_pathname __P((char *));
//static int glob_testdir __P((char *));
//static char **glob_dir_to_array __P((char *, char **, int));
#define LL(CS)	CS
#define LL_W(CS)		L##CS

#define GLOB_LOOP(CHAR,INT, L, INTERNAL_GLOB_PATTERN_P) \
		static int INTERNAL_GLOB_PATTERN_P (const CHAR *pattern) \
		{\
	register const CHAR *p;\
	register CHAR c;\
	int bopen;\
	\
	p = pattern;\
	bopen = 0;\
	\
	while ((c = *p++) != L##'\0')\
	switch (c)\
	{\
	case L##'?':\
	case L##'*':\
	return 1;\
	\
	case L##'[':      /* Only accept an open brace if there is a close */\
		bopen++;        /* brace to match it.  Bracket expressions must be */\
		continue;       /* complete, according to Posix.2 */\
		case L##']':\
		if (bopen)\
		return 1;\
		continue;\
		\
	case L##'+':         /* extended matching operators */\
		case L##'@':\
		case L##'!':\
		if (*p == L##'(')  /*) */\
		return 1;\
		continue;\
		\
		case L##'\\':\
		if (*p++ == L##'\0')\
		return 0;\
	}\
	\
	return 0;\
		}

/* Compile `glob_loop.c' for single-byte characters. */
//#define CHAR	unsigned char
//#define INT	int
//#define L(CS)	CS
//#define INTERNAL_GLOB_PATTERN_P internal_glob_pattern_p
//#include "glob_loop.c"
GLOB_LOOP(unsigned char, int,, internal_glob_pattern_p)

/* Compile `glob_loop.c' again for multibyte characters. */
#if HANDLE_MULTIBYTE

//#define CHAR	wchar_t
//#define INT	wint_t
//#define L(CS)	L##CS
//#define INTERNAL_GLOB_PATTERN_P internal_glob_wpattern_p
//#include "glob_loop.c"
GLOB_LOOP(wchar_t, wint_t, L, internal_glob_wpattern_p)

#endif /* HANDLE_MULTIBYTE */

#if defined (HAVE_STRCOLL)
/* Helper function for collating symbol equivalence. */
static int rangecmp (int c1, int c2)
{
	static char s1[2] = {' ', '\0'};
	static char s2[2] = {' ', '\0'};
	int ret;

	/* Eight bits only.  Period. */
	c1 &= 0xFF;
	c2 &= 0xFF;

	if (c1 == c2)
	return (0);

	s1[0] = c1;
	s2[0] = c2;

	if ((ret = strcoll (s1, s2)) != 0)
	return ret;
	return (c1 - c2);
}
#else /* !HAVE_STRCOLL */
#  define rangecmp(c1, c2)	((int)(c1) - (int)(c2))
#endif /* !HAVE_STRCOLL */

#if defined (HAVE_STRCOLL)
static int collequiv (int c1, int c2)
{
	return (rangecmp (c1, c2) == 0);
}
#else
#  define collequiv(c1, c2)	((c1) == (c2))
#endif

static int rangecmp_wc(wint_t c1, wint_t c2) {
	static wchar_t s1[2] = { L' ', L'\0' };
	static wchar_t s2[2] = { L' ', L'\0' };

	if (c1 == c2)
		return 0;

	s1[0] = c1;
	s2[0] = c2;

	return (wcscoll(s1, s2));
}

static int collequiv_wc(wint_t c, wint_t equiv) {
	return (!(c - equiv));
}

/* unibyte character classification */
#if !defined (isascii) && !defined (HAVE_ISASCII)
#  define isascii(c)	((unsigned int)(c) <= 0177)
#endif

enum char_class {
	CC_NO_CLASS = 0,
	CC_ASCII,
	CC_ALNUM,
	CC_ALPHA,
	CC_BLANK,
	CC_CNTRL,
	CC_DIGIT,
	CC_GRAPH,
	CC_LOWER,
	CC_PRINT,
	CC_PUNCT,
	CC_SPACE,
	CC_UPPER,
	CC_WORD,
	CC_XDIGIT
};

static char const * const cclass_name[] = { "", "ascii", "alnum", "alpha", "blank", "cntrl",
		"digit", "graph", "lower", "print", "punct", "space", "upper", "word", "xdigit" };

#define N_CHAR_CLASS (sizeof(cclass_name) / sizeof (cclass_name[0]))

static int is_cclass(int c, const char* name) {
	enum char_class char_class = CC_NO_CLASS;
	int i, result;

	for (i = 1; i < N_CHAR_CLASS; i++) {
		if (STREQ(name, cclass_name[i])) {
			char_class = (enum char_class) i;
			break;
		}
	}

	if (char_class == 0)
		return -1;

	switch (char_class) {
	case CC_ASCII:
		result = isascii(c);
		break;
	case CC_ALNUM:
		result = ISALNUM(c);
		break;
	case CC_ALPHA:
		result = ISALPHA(c);
		break;
	case CC_BLANK:
		result = ISBLANK(c);
		break;
	case CC_CNTRL:
		result = ISCNTRL(c);
		break;
	case CC_DIGIT:
		result = ISDIGIT(c);
		break;
	case CC_GRAPH:
		result = ISGRAPH(c);
		break;
	case CC_LOWER:
		result = ISLOWER(c);
		break;
	case CC_PRINT:
		result = ISPRINT(c);
		break;
	case CC_PUNCT:
		result = ISPUNCT(c);
		break;
	case CC_SPACE:
		result = ISSPACE(c);
		break;
	case CC_UPPER:
		result = ISUPPER(c);
		break;
	case CC_WORD:
		result = (ISALNUM (c) || c == '_');
		break;
	case CC_XDIGIT:
		result = ISXDIGIT(c);
		break;
	default:
		result = -1;
		break;
	}

	return result;
}

static int is_wcclass(wint_t wc, wchar_t *name) {
	char *mbs;
	mbstate_t state;
	size_t mbslength;
	wctype_t desc;
	int want_word;

	if ((wctype("ascii") == (wctype_t) 0) && (wcscmp(name, L"ascii") == 0)) {
		int c;

		if ((c = wctob(wc)) == EOF)
			return 0;
		else
			return (c <= 0x7F);
	}

	want_word = (wcscmp(name, L"word") == 0);
	if (want_word)
		name = L"alnum";

	memset(&state, '\0', sizeof(mbstate_t));
	mbs = (char *) malloc(wcslen(name) * MB_CUR_MAX + 1);
	mbslength = wcsrtombs(mbs, (const wchar_t **) &name, (wcslen(name) * MB_CUR_MAX + 1), &state);

	if (mbslength == (size_t) -1 || mbslength == (size_t) -2) {
		free(mbs);
		return -1;
	}
	desc = wctype(mbs);
	free(mbs);

	if (desc == (wctype_t) 0)
		return -1;

	if (want_word)
		return (iswctype(wc, desc) || wc == L'_');
	else
		return (iswctype(wc, desc));
}

#define COLLSYM_LOOP(CHAR,XCHAR,_COLLSYM,__COLLSYM,POSIXCOLL,L)\
		typedef struct _COLLSYM {\
	XCHAR *name;\
	CHAR code;\
} __COLLSYM;\
\
static __COLLSYM POSIXCOLL [] =\
{\
	{  L("NUL"),			L('\0') },\
	{  L("SOH"),			L('\001') },\
	{  L("STX"),			L('\002') },\
	{  L("ETX"),			L('\003') },\
	{  L("EOT"),			L('\004') },\
	{  L("ENQ"),			L('\005') },\
	{  L("ACK"),			L('\006') },\
	{  L("alert"),			L('\a') },\
	{  L("BS"),			L('\010') },\
	{  L("backspace"),		L('\b') },\
	{  L("HT"),			L('\011') },\
	{  L("tab"),			L('\t') },\
	{  L("LF"),			L('\012') },\
	{  L("newline"),		L('\n') },\
	{  L("VT"),			L('\013') },\
	{  L("vertical-tab"),		L('\v') },\
	{  L("FF"),			L('\014') },\
	{  L("form-feed"),		L('\f') },\
	{  L("CR"),			L('\015') },\
	{  L("carriage-return"),	L('\r') },\
	{  L("SO"),			L('\016') },\
	{  L("SI"),			L('\017') },\
	{  L("DLE"),			L('\020') },\
	{  L("DC1"),			L('\021') },\
	{  L("DC2"),			L('\022') },\
	{  L("DC3"),			L('\023') },\
	{  L("DC4"),			L('\024') },\
	{  L("NAK"),			L('\025') },\
	{  L("SYN"),			L('\026') },\
	{  L("ETB"),			L('\027') },\
	{  L("CAN"),			L('\030') },\
	{  L("EM"),			L('\031') },\
	{  L("SUB"),			L('\032') },\
	{  L("ESC"),			L('\033') },\
	{  L("IS4"),			L('\034') },\
	{  L("FS"),			L('\034') },\
	{  L("IS3"),			L('\035') },\
	{  L("GS"),			L('\035') },\
	{  L("IS2"),			L('\036') },\
	{  L("RS"),			L('\036') },\
	{  L("IS1"),			L('\037') },\
	{  L("US"),			L('\037') },\
	{  L("space"),			L(' ') },\
	{  L("exclamation-mark"),	L('!') },\
	{  L("quotation-mark"),		L('"') },\
	{  L("number-sign"),		L('#') },\
	{  L("dollar-sign"),		L('$') },\
	{  L("percent-sign"),		L('%') },\
	{  L("ampersand"),		L('&') },\
	{  L("apostrophe"),		L('\'') },\
	{  L("left-parenthesis"),	L('(') },\
	{  L("right-parenthesis"),	L(')') },\
	{  L("asterisk"),		L('*') },\
	{  L("plus-sign"),		L('+') },\
	{  L("comma"),			L(',') },\
	{  L("hyphen"),			L('-') },\
	{  L("hyphen-minus"),		L('-') },\
	{  L("minus"),			L('-') },	/* extension from POSIX.2 */\
	{  L("dash"),			L('-') },	/* extension from POSIX.2 */\
	{  L("period"),			L('.') },\
	{  L("full-stop"),		L('.') },\
	{  L("slash"),			L('/') },\
	{  L("solidus"),		L('/') },	/* extension from POSIX.2 */\
	{  L("zero"),			L('0') },\
	{  L("one"),			L('1') },\
	{  L("two"),			L('2') },\
	{  L("three"),			L('3') },\
	{  L("four"),			L('4') },\
	{  L("five"),			L('5') },\
	{  L("six"),			L('6') },\
	{  L("seven"),			L('7') },\
	{  L("eight"),			L('8') },\
	{  L("nine"),			L('9') },\
	{  L("colon"),			L(':') },\
	{  L("semicolon"),		L(';') },\
	{  L("less-than-sign"),		L('<') },\
	{  L("equals-sign"),		L('=') },\
	{  L("greater-than-sign"),	L('>') },\
	{  L("question-mark"),		L('?') },\
	{  L("commercial-at"),		L('@') },\
	/* upper-case letters omitted */\
	{  L("left-square-bracket"),	L('[') },\
	{  L("backslash"),		L('\\') },\
	{  L("reverse-solidus"),	L('\\') },\
	{  L("right-square-bracket"),	L(']') },\
	{  L("circumflex"),		L('^') },\
	{  L("circumflex-accent"),	L('^') },	/* extension from POSIX.2 */\
	{  L("underscore"),		L('_') },\
	{  L("grave-accent"),		L('`') },\
	/* lower-case letters omitted */\
	{  L("left-brace"),		L('{') },	/* extension from POSIX.2 */\
			{  L("left-curly-bracket"),	L('{') },\
					{  L("vertical-line"),		L('|') },\
					{  L("right-brace"),		L('}') },	/* extension from POSIX.2 */\
		{  L("right-curly-bracket"),	L('}') },\
		{  L("tilde"),			L('~') },\
		{  L("DEL"),			L('\177') },\
		{  0,	0 },\
};\

COLLSYM_LOOP(unsigned char, unsigned char, _collsym, __collsym, posix_collsyms, LL)

#if HANDLE_MULTIBYTE
COLLSYM_LOOP(wchar_t, wint_t, _collwcsym, __collwcsym, posix_collwcsyms, LL_W)
#endif

static int collsym(unsigned char* s, int len) {
	register struct _collsym *csp;
	char *x;

	x = (char *) s;
	for (csp = posix_collsyms; csp->name; csp++) {
		if (STREQN(csp->name, x, len) && csp->name[len] == '\0')
			return (csp->code);
	}
	if (len == 1)
		return s[0];
	return -1; //INVALID;
}

static wint_t collwcsym(wchar_t *s, int len) {
	register struct _collwcsym *csp;

	for (csp = posix_collwcsyms; csp->name; csp++) {
		if (STREQN_W(csp->name, s, len) && csp->name[len] == L'\0')
			return (csp->code);
	}
	if (len == 1)
		return s[0];
	return WEOF; //INVALID;
}

#define fold(c) ((flags & FNM_CASEFOLD) ? TOLOWER ((unsigned char)c) : ((unsigned char)c))
#define fold_w(c) ((flags & FNM_CASEFOLD) && iswupper (c) ? towlower (c) : (c))

//#include "strmatch.c"
#define SM_LOOP(CHAR,U_CHAR,XCHAR,INT,L,INVALID,FOLD, \
		FCT,GMATCH,COLLSYM,PARSE_COLLSYM,BRACKMATCH,PATSCAN,STRCOMPARE,EXTMATCH,\
		STRCHR,STRCOLL,STRLEN,STRCMP,RANGECMP,COLLEQUIV,IS_CCLASS)\
		\
/* Parse a bracket expression collating symbol ([.sym.]) starting at P, find\
   the value of the symbol, and move P past the collating symbol expression.\
   The value is returned in *VP, if VP is not null. */\
		static CHAR * PARSE_COLLSYM (CHAR* p, INT* vp)\
		{\
	register int pc;\
	INT val;\
	\
	p++;				/* move past the `.' */\
		\
		for (pc = 0; p[pc]; pc++)\
		if (p[pc] == L('.') && p[pc+1] == L(']'))\
		break;\
		val = COLLSYM (p, pc);\
		if (vp)\
		*vp = val;\
		return (p + pc + 2);\
		}\
		\
/* Use prototype definition here because of type promotion. */\
		static CHAR * BRACKMATCH (CHAR *p, U_CHAR test, int flags)\
		{\
			register CHAR cstart, cend, c;\
			register int not;    /* Nonzero if the sense of the character class is inverted.  */\
		int brcnt;\
		INT pc;\
		CHAR *savep;\
		\
		test = FOLD (test);\
		\
		savep = p;\
		\
		/* POSIX.2 3.13.1 says that an exclamation mark (`!') shall replace the\
     circumflex (`^') in its role in a `nonmatching list'.  A bracket\
     expression starting with an unquoted circumflex character produces\
     unspecified results.  This implementation treats the two identically. */\
		if (not = (*p == L('!') || *p == L('^')))\
		++p;\
		\
		c = *p++;\
		for (;;)\
		{\
			/* Initialize cstart and cend in case `-' is the last\
	 character of the pattern. */\
		cstart = cend = c;\
		\
		/* POSIX.2 equivalence class:  [=c=].  See POSIX.2 2.8.3.2.  Find\
	 the end of the equivalence class, move the pattern pointer past\
	 it, and check for equivalence.  XXX - this handles only\
	 single-character equivalence classes, which is wrong, or at\
	 least incomplete. */\
		if (c == L('[') && *p == L('=') && p[2] == L('=') && p[3] == L(']'))\
		{\
			pc = FOLD (p[1]);\
			p += 4;\
			if (COLLEQUIV (test, pc))\
			{\
/*[*/	      /* Move past the closing `]', since the first thing we do at\
		 the `matched:' label is back p up one. */\
		p++;\
		goto matched;\
			}\
			else\
			{\
				c = *p++;\
				if (c == L('\0'))\
				return ((test == L('[')) ? savep : (CHAR *)0); /*]*/\
		c = FOLD (c);\
		continue;\
			}\
		}\
		\
		/* POSIX.2 character class expression.  See POSIX.2 2.8.3.2. */\
		if (c == L('[') && *p == L(':'))\
		{\
			CHAR *close, *ccname;\
			\
			pc = 0;	/* make sure invalid char classes don't match. */\
		/* Find end of character class name */\
		for (close = p + 1; *close != '\0'; close++)\
		if (*close == L(':') && *(close+1) == L(']'))\
		break;\
		\
		if (*close != L('\0'))\
		{\
			ccname = (CHAR *)malloc ((close - p) * sizeof (CHAR));\
			if (ccname == 0)\
			pc = 0;\
			else\
			{\
				bcopy (p + 1, ccname, (close - p - 1) * sizeof (CHAR));\
				*(ccname + (close - p - 1)) = L('\0');\
				pc = IS_CCLASS (test, (XCHAR *)ccname);\
			}\
			if (pc == -1)\
			pc = 0;\
			else\
			p = close + 2;\
			\
			free (ccname);\
		}\
		\
		if (pc)\
		{\
/*[*/	      /* Move past the closing `]', since the first thing we do at\
		 the `matched:' label is back p up one. */\
		p++;\
		goto matched;\
		}\
		else\
		{\
			/* continue the loop here, since this expression can't be\
		 the first part of a range expression. */\
		c = *p++;\
		if (c == L('\0'))\
		return ((test == L('[')) ? savep : (CHAR *)0);\
		else if (c == L(']'))\
		break;\
		c = FOLD (c);\
		continue;\
		}\
		}\
		\
		/* POSIX.2 collating symbols.  See POSIX.2 2.8.3.2.  Find the end of\
	 the symbol name, make sure it is terminated by `.]', translate\
	 the name to a character using the external table, and do the\
	 comparison. */\
		if (c == L('[') && *p == L('.'))\
		{\
			p = PARSE_COLLSYM (p, &pc);\
			/* An invalid collating symbol cannot be the first point of a\
	     range.  If it is, we set cstart to one greater than `test',\
	     so any comparisons later will fail. */\
		cstart = (pc == INVALID) ? test + 1 : pc;\
		}\
		\
		if (!(flags & FNM_NOESCAPE) && c == L('\\'))\
		{\
			if (*p == '\0')\
			return (CHAR *)0;\
			cstart = cend = *p++;\
		}\
		\
		cstart = cend = FOLD (cstart);\
		\
		/* POSIX.2 2.8.3.1.2 says: `An expression containing a `[' that\
	 is not preceded by a backslash and is not part of a bracket\
	 expression produces undefined results.'  This implementation\
	 treats the `[' as just a character to be matched if there is\
	 not a closing `]'. */\
		if (c == L('\0'))\
		return ((test == L('[')) ? savep : (CHAR *)0);\
		\
		c = *p++;\
		c = FOLD (c);\
		\
		if ((flags & FNM_PATHNAME) && c == L('/'))\
		/* [/] can never match when matching a pathname.  */\
		return (CHAR *)0;\
		\
		/* This introduces a range, unless the `-' is the last\
	 character of the class.  Find the end of the range\
	 and move past it. */\
		if (c == L('-') && *p != L(']'))\
		{\
			cend = *p++;\
			if (!(flags & FNM_NOESCAPE) && cend == L('\\'))\
			cend = *p++;\
			if (cend == L('\0'))\
			return (CHAR *)0;\
			if (cend == L('[') && *p == L('.'))\
			{\
				p = PARSE_COLLSYM (p, &pc);\
				/* An invalid collating symbol cannot be the second part of a\
		 range expression.  If we get one, we set cend to one fewer\
		 than the test character to make sure the range test fails. */\
		cend = (pc == INVALID) ? test - 1 : pc;\
			}\
			cend = FOLD (cend);\
			\
			c = *p++;\
			\
			/* POSIX.2 2.8.3.2:  ``The ending range point shall collate\
	     equal to or higher than the starting range point; otherwise\
	     the expression shall be treated as invalid.''  Note that this\
	     applies to only the range expression; the rest of the bracket\
	     expression is still checked for matches. */\
		if (RANGECMP (cstart, cend) > 0)\
		{\
			if (c == L(']'))\
			break;\
			c = FOLD (c);\
			continue;\
		}\
		}\
		\
		if (RANGECMP (test, cstart) >= 0 && RANGECMP (test, cend) <= 0)\
		goto matched;\
		\
		if (c == L(']'))\
		break;\
		}\
		/* No match. */\
		return (!not ? (CHAR *)0 : p);\
		\
		matched:\
		/* Skip the rest of the [...] that already matched.  */\
		c = *--p;\
		brcnt = 1;\
		while (brcnt > 0)\
		{\
			/* A `[' without a matching `]' is just another character to match. */\
		if (c == L('\0'))\
		return ((test == L('[')) ? savep : (CHAR *)0);\
		\
		c = *p++;\
		if (c == L('[') && (*p == L('=') || *p == L(':') || *p == L('.')))\
		brcnt++;\
		else if (c == L(']'))\
		brcnt--;\
		else if (!(flags & FNM_NOESCAPE) && c == L('\\'))\
		{\
			if (*p == '\0')\
			return (CHAR *)0;\
			/* XXX 1003.2d11 is unclear if this is right. */\
		++p;\
		}\
		}\
		return (not ? (CHAR *)0 : p);\
		}\
		\
/* Match STRING against the filename pattern PATTERN, returning zero if it matches, FNM_NOMATCH if not.  */\
		static int GMATCH (CHAR* string, CHAR* se, CHAR* pattern, CHAR* pe, int flags)\
		{\
			CHAR *p, *n;		/* pattern, string */\
		INT c;		/* current pattern character - XXX U_CHAR? */\
		INT sc;		/* current string character - XXX U_CHAR? */\
		\
		p = pattern;\
		n = string;\
		\
		if (string == 0 || pattern == 0)\
		return FNM_NOMATCH;\
		\
		while (p < pe)\
		{\
			c = *p++;\
			c = FOLD (c);\
			\
			sc = n < se ? *n : '\0';\
			\
			switch (c)\
			{\
			case L('?'):		/* Match single character */\
		if (sc == '\0')\
		return FNM_NOMATCH;\
		else if ((flags & FNM_PATHNAME) && sc == L('/'))\
		/* If we are matching a pathname, `?' can never match a `/'. */\
		return FNM_NOMATCH;\
		else if ((flags & FNM_PERIOD) && sc == L('.') &&\
				(n == string || ((flags & FNM_PATHNAME) && n[-1] == L('/'))))\
				/* `?' cannot match a `.' if it is the first character of the\
	       string or if it is the first character following a slash and\
	       we are matching a pathname. */\
		return FNM_NOMATCH;\
		break;\
		\
			case L('\\'):		/* backslash escape removes special meaning */\
		if (p == pe)\
		return FNM_NOMATCH;\
		\
		if ((flags & FNM_NOESCAPE) == 0)\
		{\
			c = *p++;\
			/* A trailing `\' cannot match. */\
		if (p > pe)\
		return FNM_NOMATCH;\
		c = FOLD (c);\
		}\
		if (FOLD (sc) != (U_CHAR)c)\
		return FNM_NOMATCH;\
		break;\
		\
			case '*':		/* Match zero or more characters */\
		if (p == pe)\
		return 0;\
		\
		if ((flags & FNM_PERIOD) && sc == L('.') &&\
				(n == string || ((flags & FNM_PATHNAME) && n[-1] == L('/'))))\
				/* `*' cannot match a `.' if it is the first character of the\
	       string or if it is the first character following a slash and\
	       we are matching a pathname. */\
		return FNM_NOMATCH;\
		\
		/* Collapse multiple consecutive `*' and `?', but make sure that\
	     one character of the string is consumed for each `?'. */\
		for (c = *p++; (c == L('?') || c == L('*')); c = *p++)\
		{\
			if ((flags & FNM_PATHNAME) && sc == L('/'))\
			/* A slash does not match a wildcard under FNM_PATHNAME. */\
		return FNM_NOMATCH;\
		else if (c == L('?'))\
		{\
			if (sc == L('\0'))\
			return FNM_NOMATCH;\
			/* One character of the string is consumed in matching\
		     this ? wildcard, so *??? won't match if there are\
		     fewer than three characters. */\
		n++;\
		sc = n < se ? *n : '\0';\
		}\
		\
		if (p == pe)\
		break;\
		}\
		\
		/* If we've hit the end of the pattern and the last character of\
	     the pattern was handled by the loop above, we've succeeded.\
	     Otherwise, we need to match that last character. */\
		if (p == pe && (c == L('?') || c == L('*')))\
		return (0);\
		\
		/* General case, use recursion. */\
		{\
			U_CHAR c1;\
			\
			c1 = ((flags & FNM_NOESCAPE) == 0 && c == L('\\')) ? *p : c;\
			c1 = FOLD (c1);\
			for (--p; n < se; ++n)\
			{\
				/* Only call strmatch if the first character indicates a\
		   possible match.  We can check the first character if\
		   we're not doing an extended glob match. */\
		if ((flags & FNM_EXTMATCH) == 0 && c != L('[') && FOLD (*n) != c1) /*]*/\
		continue;\
		\
		/* If we're doing an extended glob match and the pattern is not\
		   one of the extended glob patterns, we can check the first\
		   character. */\
		if ((flags & FNM_EXTMATCH) && p[1] != L('(') && /*)*/\
		STRCHR (L("?*+@!"), *p) == 0 && c != L('[') && FOLD (*n) != c1) /*]*/\
		continue;\
		\
		/* Otherwise, we just recurse. */\
		if (GMATCH (n, se, p, pe, flags & ~FNM_PERIOD) == 0)\
		return (0);\
			}\
			return FNM_NOMATCH;\
		}\
		\
			case L('['):\
			{\
			if (sc == L('\0') || n == se)\
			return FNM_NOMATCH;\
			\
			/* A character class cannot match a `.' if it is the first\
	       character of the string or if it is the first character\
	       following a slash and we are matching a pathname. */\
		if ((flags & FNM_PERIOD) && sc == L('.') &&\
				(n == string || ((flags & FNM_PATHNAME) && n[-1] == L('/'))))\
				return (FNM_NOMATCH);\
				\
				p = BRACKMATCH (p, sc, flags);\
				if (p == 0)\
				return FNM_NOMATCH;\
			}\
			break;\
			\
			default:\
			if ((U_CHAR)c != FOLD (sc))\
			return (FNM_NOMATCH);\
			}\
			\
			++n;\
		}\
		\
		if (n == se)\
		return (0);\
		\
		if ((flags & FNM_LEADING_DIR) && *n == L('/'))\
		/* The FNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */\
		return 0;\
		\
		return (FNM_NOMATCH);\
		}\
		\
		static int FCT (CHAR* pattern,CHAR* string, int flags)\
		{\
			CHAR *se, *pe;\
			\
			if (string == 0 || pattern == 0)\
			return FNM_NOMATCH;\
			\
			se = string + STRLEN ((XCHAR *)string);\
			pe = pattern + STRLEN ((XCHAR *)pattern);\
			\
			return (GMATCH (string, se, pattern, pe, flags));\
		}

SM_LOOP(unsigned char, unsigned char, char, int, LL, -1, fold, internal_strmatch, gmatch, collsym,
		parse_collsym, brackmatch, patscan, strcompare, extmatch, strchr, strcoll, strlen, strcmp,
		rangecmp, collequiv, is_cclass)

#if HANDLE_MULTIBYTE
SM_LOOP(wchar_t, wint_t, wchar_t, wint_t, LL_W, WEOF, fold_w, internal_wstrmatch, gmatch_wc,
		collwcsym, parse_collwcsym, brackmatch_wc, patscan_wc, wscompare, extmatch_wc, wcschr,
		wcscoll, wcslen, wcscmp, rangecmp_wc, collequiv_wc, is_wcclass)
#endif

static int xstrmatch(char *pattern, char* string, int flags) {
#if HANDLE_MULTIBYTE
	int ret;
	size_t n;
	wchar_t *wpattern, *wstring;

	if (MB_CUR_MAX == 1)
		return (internal_strmatch((unsigned char *) pattern, (unsigned char *) string, flags));

	n = xdupmbstowcs(&wpattern, NULL, pattern);
	if (n == (size_t) -1 || n == (size_t) -2)
		return (internal_strmatch((unsigned char *) pattern, (unsigned char *) string, flags));

	n = xdupmbstowcs(&wstring, NULL, string);
	if (n == (size_t) -1 || n == (size_t) -2) {
		free(wpattern);
		return (internal_strmatch((unsigned char *) pattern, (unsigned char *) string, flags));
	}

	ret = internal_wstrmatch(wpattern, wstring, flags);

	free(wpattern);
	free(wstring);

	return ret;
#else
	return (internal_strmatch ((unsigned char *)pattern, (unsigned char *)string, flags));
#endif /* !HANDLE_MULTIBYTE */
}

static int strmatch(char* pattern, char* string, int flags) {
	if (string == 0 || pattern == 0)
		return FNM_NOMATCH;

	return (xstrmatch(pattern, string, flags));
}

#if defined (HANDLE_MULTIBYTE)
static int wcsmatch(wchar_t* wpattern, wchar_t *wstring, int flags) {
	if (wstring == 0 || wpattern == 0)
		return (FNM_NOMATCH);

	return (internal_wstrmatch(wpattern, wstring, flags));
}
#endif

/* And now a function that calls either the single-byte or multibyte version
 of internal_glob_pattern_p. */
static int glob_pattern_p(const char* pattern) {
#if HANDLE_MULTIBYTE
	size_t n;
	wchar_t *wpattern;
	int r;

	if (MB_CUR_MAX == 1)
		return (internal_glob_pattern_p((unsigned char *) pattern));

	/* Convert strings to wide chars, and call the multibyte version. */
	n = xdupmbstowcs(&wpattern, NULL, pattern);
	if (n == (size_t) -1)
		/* Oops.  Invalid multibyte sequence.  Try it as single-byte sequence. */
		return (internal_glob_pattern_p((unsigned char *) pattern));

	r = internal_glob_wpattern_p(wpattern);
	free(wpattern);

	return r;
#else
	return (internal_glob_pattern_p (pattern));
#endif
}

/* Return 1 if DNAME should be skipped according to PAT.  Mostly concerned
 with matching leading `.'. */

static int skipname(char* pat, char* dname) {
	/* If a leading dot need not be explicitly matched, and the pattern
	 doesn't start with a `.', don't match `.' or `..' */
	if (noglob_dot_filenames == 0 && pat[0] != '.' && (pat[0] != '\\' || pat[1] != '.')
			&& (dname[0] == '.' && (dname[1] == '\0' || (dname[1] == '.' && dname[2] == '\0'))))
		return 1;

	/* If a dot must be explicity matched, check to see if they do. */
	else if (noglob_dot_filenames && dname[0] == '.' && pat[0] != '.'
			&& (pat[0] != '\\' || pat[1] != '.'))
		return 1;

	return 0;
}

#if HANDLE_MULTIBYTE
/* Return 1 if DNAME should be skipped according to PAT.  Handles multibyte
 characters in PAT and DNAME.  Mostly concerned with matching leading `.'. */

static int mbskipname(char* pat, char* dname) {
	int ret;
	wchar_t *pat_wc, *dn_wc;
	size_t pat_n, dn_n;

	pat_n = xdupmbstowcs(&pat_wc, NULL, pat);
	dn_n = xdupmbstowcs(&dn_wc, NULL, dname);

	ret = 0;
	if (pat_n != (size_t) -1 && dn_n != (size_t) -1) {
		/* If a leading dot need not be explicitly matched, and the
		 pattern doesn't start with a `.', don't match `.' or `..' */
		if (noglob_dot_filenames == 0 && pat_wc[0] != L'.'
				&& (pat_wc[0] != L'\\' || pat_wc[1] != L'.')
				&& (dn_wc[0] == L'.'
						&& (dn_wc[1] == L'\0' || (dn_wc[1] == L'.' && dn_wc[2] == L'\0'))))
			ret = 1;

		/* If a leading dot must be explicity matched, check to see if the
		 pattern and dirname both have one. */
		else if (noglob_dot_filenames && dn_wc[0] == L'.' && pat_wc[0] != L'.'
				&& (pat_wc[0] != L'\\' || pat_wc[1] != L'.'))
			ret = 1;
	}

	FREE(pat_wc);
	FREE(dn_wc);

	return ret;
}
#endif /* HANDLE_MULTIBYTE */

/* Remove backslashes quoting characters in PATHNAME by modifying PATHNAME. */
static void udequote_pathname(char* pathname) {
	register int i, j;

	for (i = j = 0; pathname && pathname[i];) {
		if (pathname[i] == '\\')
			i++;

		pathname[j++] = pathname[i++];

		if (pathname[i - 1] == 0)
			break;
	}
	pathname[j] = '\0';
}

#if HANDLE_MULTIBYTE
/* Remove backslashes quoting characters in PATHNAME by modifying PATHNAME. */
static void wdequote_pathname(char* pathname) {
	mbstate_t ps;
	size_t len, n;
	wchar_t *wpathname;
	int i, j;
	wchar_t *orig_wpathname;

	len = strlen(pathname);
	/* Convert the strings into wide characters.  */
	n = xdupmbstowcs(&wpathname, NULL, pathname);
	if (n == (size_t) -1)
		/* Something wrong. */
		return;
	orig_wpathname = wpathname;

	for (i = j = 0; wpathname && wpathname[i];) {
		if (wpathname[i] == L'\\')
			i++;

		wpathname[j++] = wpathname[i++];

		if (wpathname[i - 1] == L'\0')
			break;
	}
	wpathname[j] = L'\0';

	/* Convert the wide character string into unibyte character set. */
	memset(&ps, '\0', sizeof(mbstate_t));
	n = wcsrtombs(pathname, (const wchar_t **) &wpathname, len, &ps);
	pathname[len] = '\0';

	/* Can't just free wpathname here; wcsrtombs changes it in many cases. */
	free(orig_wpathname);
}

static void dequote_pathname(char* pathname) {
	if (MB_CUR_MAX > 1)
		wdequote_pathname(pathname);
	else
		udequote_pathname(pathname);
}
#endif /* HANDLE_MULTIBYTE */

#if !HANDLE_MULTIBYTE
#  define dequote_pathname udequote_pathname
#endif

/* Test whether NAME exists. */

//#if defined (HAVE_LSTAT)
#  define GLOB_TESTNAME(name)  (lstat (name, &finfo))
//#else /* !HAVE_LSTAT */
//#  if !defined (AFS)
//#    define GLOB_TESTNAME(name)  (sh_eaccess (nextname, F_OK))
//#  else /* AFS */
//#    define GLOB_TESTNAME(name)  (access (nextname, F_OK))
//#  endif /* AFS */
//#endif /* !HAVE_LSTAT */

/* Return 0 if DIR is a directory, -1 otherwise. */
static int glob_testdir(char* dir) {
	struct stat finfo;

	if (stat(dir, &finfo) < 0)
		return (-1);

	if (S_ISDIR (finfo.st_mode) == 0)
		return (-1);

	return (0);
}

/* Return a vector of names of files in directory DIR
 whose names match glob pattern PAT.
 The names are not in any particular order.
 Wildcards at the beginning of PAT do not match an initial period.

 The vector is terminated by an element that is a null pointer.

 To free the space allocated, first free the vector's elements,
 then free the vector.

 Return 0 if cannot get enough memory to hold the pointer
 and the names.

 Return -1 if cannot access directory DIR.
 Look in errno for more information.  */

static char ** glob_vector(char* pat, char* dir, int flags) {
	struct globval {
		struct globval *next;
		char *name;
	};

	DIR *d;
	register struct dirent *dp;
	struct globval *lastlink;
	register struct globval *nextlink;
	register char *nextname, *npat;
	unsigned int count;
	int lose, skip;
	register char **name_vector;
	register unsigned int i;
	int mflags; /* Flags passed to strmatch (). */
	int nalloca;
	struct globval *firstmalloc, *tmplink;

	lastlink = 0;
	count = lose = skip = 0;

	firstmalloc = 0;
	nalloca = 0;

	/* If PAT is empty, skip the loop, but return one (empty) filename. */
	if (pat == 0 || *pat == '\0') {
		if (glob_testdir(dir) < 0)
			return ((char **) &glob_error_return);

		nextlink = (struct globval *) alloca(sizeof(struct globval));
		if (nextlink == NULL)
			return ((char **) NULL);

		nextlink->next = (struct globval *) 0;
		nextname = (char *) malloc(1);
		if (nextname == 0)
			lose = 1;
		else {
			lastlink = nextlink;
			nextlink->name = nextname;
			nextname[0] = '\0';
			count = 1;
		}

		skip = 1;
	}

	/* If the filename pattern (PAT) does not contain any globbing characters,
	 we can dispense with reading the directory, and just see if there is
	 a filename `DIR/PAT'.  If there is, and we can access it, just make the
	 vector to return and bail immediately. */
	if (skip == 0 && glob_pattern_p(pat) == 0) {
		int dirlen;
		struct stat finfo;

		if (glob_testdir(dir) < 0)
			return ((char **) &glob_error_return);

		dirlen = strlen(dir);
		nextname = (char *) malloc(dirlen + strlen(pat) + 2);
		npat = (char *) malloc(strlen(pat) + 1);
		if (nextname == 0 || npat == 0)
			lose = 1;
		else {
			strcpy(npat, pat);
			dequote_pathname(npat);

			strcpy(nextname, dir);
			nextname[dirlen++] = '/';
			strcpy(nextname + dirlen, npat);

			if (GLOB_TESTNAME (nextname) >= 0) {
				free(nextname);
				nextlink = (struct globval *) alloca(sizeof(struct globval));
				if (nextlink) {
					nextlink->next = (struct globval *) 0;
					lastlink = nextlink;
					nextlink->name = npat;
					count = 1;
				} else
					lose = 1;
			} else {
				free(nextname);
				free(npat);
			}
		}

		skip = 1;
	}

	if (skip == 0) {
		/* Open the directory, punting immediately if we cannot.  If opendir
		 is not robust (i.e., it opens non-directories successfully), test
		 that DIR is a directory and punt if it's not. */
#if defined (OPENDIR_NOT_ROBUST)
		if (glob_testdir (dir) < 0)
		return ((char **) &glob_error_return);
#endif

		d = opendir(dir);
		if (d == NULL)
			return ((char **) &glob_error_return);

		/* Compute the flags that will be passed to strmatch().  We don't
		 need to do this every time through the loop. */
		mflags = (noglob_dot_filenames ? FNM_PERIOD : 0) | FNM_PATHNAME;

#ifdef FNM_CASEFOLD
		if (glob_ignore_case)
			mflags |= FNM_CASEFOLD;
#endif

		//      if (extended_glob)
		//	mflags |= FNM_EXTMATCH;

		/* Scan the directory, finding all names that match.
		 For each name that matches, allocate a struct globval
		 on the stack and store the name in it.
		 Chain those structs together; lastlink is the front of the chain.  */
		while (1) {
			/* Make globbing interruptible in the shell. */
			//if (interrupt_state || terminating_signal)
			//  {
			//    lose = 1;
			//    break;
			//  }
			dp = readdir(d);
			if (dp == NULL)
				break;

			/* If this directory entry is not to be used, try again. */
			//	  if (REAL_DIR_ENTRY (dp) == 0)
			//	    continue;
#if 0
			if (dp->d_name == 0 || *dp->d_name == 0)
			continue;
#endif

#if HANDLE_MULTIBYTE
			if (MB_CUR_MAX > 1 && mbskipname(pat, dp->d_name))
				continue;
			else
#endif
			if (skipname(pat, dp->d_name))
				continue;

			if (strmatch(pat, dp->d_name, mflags) != FNM_NOMATCH) {
				if (nalloca < ALLOCA_MAX) {
					nextlink = (struct globval *) alloca(sizeof(struct globval));
					nalloca += sizeof(struct globval);
				} else {
					nextlink = (struct globval *) malloc(sizeof(struct globval));
					if (firstmalloc == 0)
						firstmalloc = nextlink;
				}
				nextname = (char *) malloc(D_NAMLEN (dp) + 1);
				if (nextlink == 0 || nextname == 0) {
					lose = 1;
					break;
				}
				nextlink->next = lastlink;
				lastlink = nextlink;
				nextlink->name = nextname;
				bcopy(dp->d_name, nextname, D_NAMLEN (dp) + 1);
				++count;
			}
		}

		(void) closedir(d);
	}

	if (lose == 0) {
		name_vector = (char **) malloc((count + 1) * sizeof(char *));
		lose |= name_vector == NULL;
	}

	/* Have we run out of memory?	 */
	if (lose) {
		tmplink = 0;

		/* Here free the strings we have got.  */
		while (lastlink) {
			/* Since we build the list in reverse order, the first N entries
			 will be allocated with malloc, if firstmalloc is set, from
			 lastlink to firstmalloc. */
			if (firstmalloc) {
				if (lastlink == firstmalloc)
					firstmalloc = 0;
				tmplink = lastlink;
			} else
				tmplink = 0;
			free(lastlink->name);
			lastlink = lastlink->next;
			FREE(tmplink);
		}

		//QUIT;

		return ((char **) NULL);
	}

	/* Copy the name pointers from the linked list into the vector.  */
	for (tmplink = lastlink, i = 0; i < count; ++i) {
		name_vector[i] = tmplink->name;
		tmplink = tmplink->next;
	}

	name_vector[count] = NULL;

	/* If we allocated some of the struct globvals, free them now. */
	if (firstmalloc) {
		tmplink = 0;
		while (lastlink) {
			tmplink = lastlink;
			if (lastlink == firstmalloc)
				lastlink = firstmalloc = 0;
			else
				lastlink = lastlink->next;
			free(tmplink);
		}
	}

	return (name_vector);
}

/* Return a new array which is the concatenation of each string in ARRAY
 to DIR.  This function expects you to pass in an allocated ARRAY, and
 it takes care of free()ing that array.  Thus, you might think of this
 function as side-effecting ARRAY.  This should handle GX_MARKDIRS. */
static char ** glob_dir_to_array(char* dir, char** array, int flags) {
	register unsigned int i, l;
	int add_slash;
	char **result, *new;
	struct stat sb;

	l = strlen(dir);
	if (l == 0) {
		if (flags & GX_MARKDIRS)
			for (i = 0; array[i]; i++) {
				if ((stat(array[i], &sb) == 0) && S_ISDIR(sb.st_mode)) {
					l = strlen(array[i]);
					new = (char *) realloc(array[i], l + 2);
					if (new == 0)
						return NULL;
					new[l] = '/';
					new[l + 1] = '\0';
					array[i] = new;
				}
			}
		return (array);
	}

	add_slash = dir[l - 1] != '/';

	i = 0;
	while (array[i] != NULL)
		++i;

	result = (char **) malloc((i + 1) * sizeof(char *));
	if (result == NULL)
		return (NULL);

	for (i = 0; array[i] != NULL; i++) {
		/* 3 == 1 for NUL, 1 for slash at end of DIR, 1 for GX_MARKDIRS */
		result[i] = (char *) malloc(l + strlen(array[i]) + 3);

		if (result[i] == NULL)
			return (NULL);

		strcpy(result[i], dir);
		if (add_slash)
			result[i][l] = '/';
		strcpy(result[i] + l + add_slash, array[i]);
		if (flags & GX_MARKDIRS) {
			if ((stat(result[i], &sb) == 0) && S_ISDIR(sb.st_mode)) {
				size_t rlen;
				rlen = strlen(result[i]);
				result[i][rlen] = '/';
				result[i][rlen + 1] = '\0';
			}
		}
	}
	result[i] = NULL;

	/* Free the input array.  */
	for (i = 0; array[i] != NULL; i++)
		free(array[i]);
	free((char *) array);

	return (result);
}

/* Do globbing on PATHNAME.  Return an array of pathnames that match,
 marking the end of the array with a null-pointer as an element.
 If no pathnames match, then the array is empty (first element is null).
 If there isn't enough memory, then return NULL.
 If a file system error occurs, return -1; `errno' has the error code.  */

char ** ff_glob_filename(char* pathname, int flags) {
	char **result;
	unsigned int result_size;
	char *directory_name, *filename;
	unsigned int directory_len;
	int free_dirname; /* flag */

	result = (char **) malloc(sizeof(char *));
	result_size = 1;
	if (result == NULL)
		return (NULL);

	result[0] = NULL;

	directory_name = NULL;

	/* Find the filename.  */
	filename = strrchr(pathname, '/');
	if (filename == NULL) {
		filename = pathname;
		directory_name = "";
		directory_len = 0;
		free_dirname = 0;
	} else {
		directory_len = (filename - pathname) + 1;
		directory_name = (char *) malloc(directory_len + 1);

		if (directory_name == 0) /* allocation failed? */
			return (NULL);

		bcopy(pathname, directory_name, directory_len);
		directory_name[directory_len] = '\0';
		++filename;
		free_dirname = 1;
	}

	/* If directory_name contains globbing characters, then we
	 have to expand the previous levels.  Just recurse. */
	if (glob_pattern_p(directory_name)) {
		char **directories;
		register unsigned int i;

		if (directory_name[directory_len - 1] == '/')
			directory_name[directory_len - 1] = '\0';

		directories = ff_glob_filename(directory_name, flags & ~GX_MARKDIRS);

		if (free_dirname) {
			free(directory_name);
			directory_name = NULL;
		}

		if (directories == NULL)
			goto memory_error;
		else if (directories == (char **) &glob_error_return) {
			free((char *) result);
			return ((char **) &glob_error_return);
		} else if (*directories == NULL) {
			free((char *) directories);
			free((char *) result);
			return ((char **) &glob_error_return);
		}

		/* We have successfully globbed the preceding directory name.
		 For each name in DIRECTORIES, call glob_vector on it and
		 FILENAME.  Concatenate the results together.  */
		for (i = 0; directories[i] != NULL; ++i) {
			char **temp_results;

			/* Scan directory even on a NULL pathname.  That way, `*h/'
			 returns only directories ending in `h', instead of all
			 files ending in `h' with a `/' appended. */
			temp_results = glob_vector(filename, directories[i], flags & ~GX_MARKDIRS);

			/* Handle error cases. */
			if (temp_results == NULL)
				goto memory_error;
			else if (temp_results == (char **) &glob_error_return)
				/* This filename is probably not a directory.  Ignore it.  */
				;
			else {
				char **array;
				register unsigned int l;

				array = glob_dir_to_array(directories[i], temp_results, flags);
				l = 0;
				while (array[l] != NULL)
					++l;

				result = (char **) realloc(result, (result_size + l) * sizeof(char *));

				if (result == NULL)
					goto memory_error;

				for (l = 0; array[l] != NULL; ++l)
					result[result_size++ - 1] = array[l];

				result[result_size - 1] = NULL;

				/* Note that the elements of ARRAY are not freed.  */
				free((char *) array);
			}
		}
		/* Free the directories.  */
		for (i = 0; directories[i]; i++)
			free(directories[i]);

		free((char *) directories);

		return (result);
	}

	/* If there is only a directory name, return it. */
	if (*filename == '\0') {
		result = (char **) realloc((char *) result, 2 * sizeof(char *));
		if (result == NULL)
			return (NULL);
		/* Handle GX_MARKDIRS here. */
		result[0] = (char *) malloc(directory_len + 1);
		if (result[0] == NULL)
			goto memory_error;
		bcopy(directory_name, result[0], directory_len + 1);
		if (free_dirname)
			free(directory_name);
		result[1] = NULL;
		return (result);
	} else {
		char **temp_results;

		/* There are no unquoted globbing characters in DIRECTORY_NAME.
		 Dequote it before we try to open the directory since there may
		 be quoted globbing characters which should be treated verbatim. */
		if (directory_len > 0)
			dequote_pathname(directory_name);

		/* We allocated a small array called RESULT, which we won't be using.
		 Free that memory now. */
		free(result);

		/* Just return what glob_vector () returns appended to the
		 directory name. */
		temp_results = glob_vector(filename, (directory_len == 0 ? "." : directory_name),
				flags & ~GX_MARKDIRS);

		if (temp_results == NULL || temp_results == (char **) &glob_error_return) {
			if (free_dirname)
				free(directory_name);
			return (temp_results);
		}

		result = glob_dir_to_array(directory_name, temp_results, flags);
		if (free_dirname)
			free(directory_name);
		return (result);
	}

	/* We get to memory_error if the program has run out of memory, or
	 if this is the shell, and we have been interrupted. */
	memory_error: if (result != NULL) {
		register unsigned int i;
		for (i = 0; result[i] != NULL; ++i)
			free(result[i]);
		free((char *) result);
	}

	if (free_dirname && directory_name)
		free(directory_name);

	//QUIT;

	return (NULL);
}
///* Free the contents of ARRAY, a NULL terminated array of char *. */
//void
//strvec_flush (array)
//     char **array;
//{
//  register int i;
//
//  if (array == 0)
//    return;
//
//  for (i = 0; array[i]; i++)
//    free (array[i]);
//}
//
//void
//strvec_dispose (array)
//     char **array;
//{
//  if (array == 0)
//    return;
//
//  strvec_flush (array);
//  free (array);
//}

#if defined (TEST)

int main (int argc,char* argv[])
{
	unsigned int i;

	for (i = 1; i < argc; ++i)
	{
		char **value = ff_glob_filename (argv[i], 0);
		if (value == NULL)
		puts ("Out of memory.");
		else if (value == &glob_error_return)
		perror (argv[i]);
		else
		for (i = 0; value[i] != NULL; i++)
		puts (value[i]);
	}

	exit (0);
}
#endif	/* TEST.  */
