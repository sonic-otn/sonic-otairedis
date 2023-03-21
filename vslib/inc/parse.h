#ifndef PARSE_H
#define PARSE_H

#define ARGS_CNT        1024            /* Max argv entries */
#define ARGS_BUFFER     8192           /* # bytes total for arguments */

/*
 * Typedef:     parse_key_t
 * Purpose:     Type used as FIRST entry in structures passed to
 *              parse_lookup.
 */
typedef char *parse_key_t;



/*
 * Typedef:     args_t
 * Purpose:     argc/argv (sorta) structure for parsing arguments.
 *
 * If the macro ARG_GET is used to consume parameters, unused parameters
 * may be passed to lower levels by simply passing the args_t struct.
 */
typedef struct args_s {
    parse_key_t a_cmd;                  /* Initial string */
    char        *a_argv[ARGS_CNT];      /* argv pointers */
    char        a_buffer[ARGS_BUFFER];  /* Split up buffer */
    int         a_argc;                 /* Parsed arg counter */
    int         a_arg;                  /* Pointer to NEXT arg */
} args_t;

#define ARG_CMD(_a)     (_a)->a_cmd

#define _ARG_CUR(_a)     \
    ((_a)->a_argv[(_a)->a_arg])

#define ARG_CUR(_a)     \
    (((_a)->a_arg >= (_a)->a_argc) ? NULL : _ARG_CUR(_a))

#define _ARG_GET(_a)     \
    ((_a)->a_argv[(_a)->a_arg++])

#define ARG_GET(_a)     \
    (((_a)->a_arg >= (_a)->a_argc) ? NULL : _ARG_GET(_a))

#define ARG_NEXT(_a)    (_a)->a_arg++
#define ARG_PREV(_a)    (_a)->a_arg--
#define ARG_CUR_INDEX(_a)   ((_a)->a_arg)

#define ARG_DISCARD(_a) \
    ((_a)->a_arg = (_a)->a_argc)

#define ARG_GET_WITH_INDEX(_a, id)     \
    ((_a)->a_argv[id])

/*
 * Macro:       ARG_CNT
 * Purpose:     Return the number of unconsumed arguments.
 */
#define ARG_CNT(_a)     ((_a)->a_argc - (_a)->a_arg)




#endif /* PARSE_H */

