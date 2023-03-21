#pragma once

typedef struct sal_console_info_s {
    int cols;           /* characters per row */
    int rows;           /* number of rows */
} sal_console_info_t;


extern long sal_console_write(const void *buf, int count);
extern long sal_console_read(void *buf, int count);
extern int sal_console_info_get(sal_console_info_t *info);
