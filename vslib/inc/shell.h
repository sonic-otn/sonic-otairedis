#ifndef SHELL_H
#define SHELL_H

#include <parse.h>

/*
 * Typedef:     cmd_result_t
 * Purpose:    Type retured from all commands indicating success, fail, 
 *        or print usage.
 */
typedef enum cmd_result_e {
    CMD_OK   = 0,            /* Command completed successfully */
    CMD_FAIL = -1,            /* Command failed */
    CMD_USAGE= -2,            /* Command failed, print usage  */
    CMD_NFND = -3,            /* Command not found */
    CMD_EXIT = -4,            /* Exit current shell level */
    CMD_INTR = -5,            /* Command interrupted */
    CMD_NOTIMPL = -6            /* Command not implemented */
} cmd_result_t;


/*
 * Typedef:    cmd_func_t
 * Purpose:    Defines command function type
 */
typedef cmd_result_t (*cmd_func_t)(int, args_t *);

/*
 * Typedef:    cmd_t
 * Purpose:    Table command match structure.
 */
typedef struct cmd_s {
    parse_key_t    c_cmd;            /* Command string */
    cmd_func_t    c_f;            /* Function to call */
    const char     *c_usage;        /* Usage string */
    const char    *c_help;        /* Help string */
} cmd_t;

cmd_result_t sh_process(int u, const char *pfx, int eof_exit);
void add_history(char *p);

#endif /* SHELL_H */
