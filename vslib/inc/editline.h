#pragma once

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if	!defined(SIZE_T)
#define SIZE_T	unsigned int
#endif	/* !defined(SIZE_T) */

#define CRLF		"\r\n"
#define FORWARD		STATIC

typedef unsigned char	CHAR;

#ifndef STATIC
#if	defined(HIDE)
#define STATIC	static
#else
#define STATIC	/* NULL */
#endif	/* !defined(HIDE) */
#endif

#define MEM_INC		64
#define SCREEN_INC	256

#define DISPOSE(p)	free((char *)(p))
#define NEW(T, c)	\
	((T *)malloc((unsigned int)(sizeof (T) * (c))))
#define COPYFROMTO(new, p, len)	\
	(void)memcpy((char *)(new), (char *)(p), (int)(len))

#if	!defined(CONST)
#define CONST	const
#else
#endif	/* !defined(CONST) */


typedef struct rl_input_state_s {
    unsigned char *line;
    int point;
} rl_input_state_t;

#ifndef TRUE
#define TRUE            1
#endif

#ifndef FALSE
#define FALSE            0
#endif

#ifndef NULL
#define NULL            0
#endif

/* editline asynchronous callback interface */

/** Called when an end of line is reached. */
typedef void (*rl_vcpfunc_t)(char *line, void *ctx);
/** Called when an end of file is reached. */
typedef void (*rf_vcpfunc_t)(void *ctx);
extern void rl_callback_read_char(CONST char *prompt);

/** Called when asynchronous edit line support is no longer needed. */
extern void rl_callback_handler_remove(void **eolCtx, void **eofCtx);
                                                                               
/** Must be called initially to enable asynchronous edit line support. */
extern void rl_callback_handler_install(CONST char *prompt, 
                                       rl_vcpfunc_t eol_handler, void *eolCtx,
                                       rf_vcpfunc_t eof_handler, void *eofCtx);


extern void rl_input_state(rl_input_state_t *state);

extern char	*(*rl_complete)(char *, int *);
extern int	(*rl_list_possib)(char *, char ***);


extern char	*readline(CONST char *prompt);

