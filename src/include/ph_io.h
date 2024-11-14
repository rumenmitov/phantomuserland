#ifndef INCLUDE_PH_IO
#define INCLUDE_PH_IO

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>

int ph_putchar(char c);

int ph_vsnrprintf(char *str, size_t size, int radix, const char *format, va_list ap);
int ph_vsnprintf(char *str, size_t size, const char *format, va_list ap);
int ph_snprintf(char *str, size_t size, const char *format, ...);
int ph_vprintf(const char *fmt, va_list ap);
int ph_printf(const char *fmt, ...);

int ph_sscanf(const char *ibuf, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
