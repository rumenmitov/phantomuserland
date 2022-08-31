#include "util.h"
#include <base/log.h>
#include <util/string.h>
#include <base/log.h>
#include <stddef.h>

extern "C"
{
    int ph_putchar(char c)
    {
        Genode::log("%c", c);
    }

    void *ph_memcpy(void *dst0, const void *src0, size_t length)
    {
        return Genode::memcpy(dst0, src0, length);
    }

    void *ph_memmove(void *dst0, const void *src0, size_t length)
    {
        return Genode::memmove(dst0, src0, length);
    }
    
    int ph_memcmp(const void *s1v, const void *s2v, size_t size)
    {
        return Genode::memcmp(s1v, s2v, size);
    }
    
    void *ph_memset(void *dst0, int c0, size_t length)
    {
        return Genode::memset(dst0, c0, length);
    }

    long ph_strtol(const char *nptr, char **endptr, int base)
    {
        if (base != 10)
        {
            Genode::warning("ph_strtol: conversion from base ", base, " are not supported");
            return -1;
        }

        if (endptr != 0)
        {
            Genode::warning("ph_strtol: endptr != 0. Ignoring it");
        }

        long res = 0;

        Genode::ascii_to(nptr, res);

        return res;
    }

    long long ph_strtoq(const char *nptr, char **endptr, int base)
    {
        if (base != 10)
        {
            Genode::warning("ph_strtol: conversion from base ", base, " are not supported");
            return -1;
        }

        if (endptr != 0)
        {
            Genode::warning("ph_strtol: endptr != 0. Ignoring it");
        }

        // TODO : come up with the wrokaround to use long long
        long res = 0;

        Genode::ascii_to(nptr, res);

        return res;
    }

    unsigned long long ph_strtouq(const char *nptr, char **endptr, int base)
    {
        if (base != 10)
        {
            Genode::warning("ph_strtol: conversion from base ", base, " are not supported");
            return -1;
        }

        if (endptr != 0)
        {
            Genode::warning("ph_strtol: endptr != 0. Ignoring it");
        }

        unsigned long long res = 0;

        Genode::ascii_to(nptr, res);

        return res;
    }
}