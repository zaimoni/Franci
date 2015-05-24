/* stdio_c.h */
#ifndef STDIO_C_H
#define STDIO_C_H 1

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

long get_filelength(FILE* src);

#ifdef __cplusplus
}	/* end extern "C" */
#endif

#endif
