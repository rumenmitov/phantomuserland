#include <debug_ext.h>
#include <stdarg.h>
#include <stdio.h>

void lprintf(char const *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    ph_vprintf(stdout, ap);
    va_end(ap);
}
