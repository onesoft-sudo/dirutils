#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>

#ifdef USE_COLORS
#define COLOR(c, s) "\033[" c "m" s "\033[0m"
#else 
#define COLOR(c, s) s
#endif

#define STREQ(s1, s2) strcmp(s1, s2) == 0

extern char *PROGRAM_NAME;

void set_program_name(char *name);
void print_error(bool str_error, const char *fmt, ...);
char *strcat_malloc(char *s1, char *s2, ...);

#endif
