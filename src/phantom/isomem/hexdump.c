/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Hex dump. :)
 *
 *
**/


#include <phantom_libc.h>


void
hexdump(const void *ptr, int length, const char *hdr, int flags)
{
    int i, j, k;
    int cols;
    const unsigned char *cp;
    char delim;

    if ((flags & HD_DELIM_MASK) != 0)
        delim = (flags & HD_DELIM_MASK) >> 8;
    else
        delim = ' ';

    if ((flags & HD_COLUMN_MASK) != 0)
        cols = flags & HD_COLUMN_MASK;
    else
        cols = 16;

    cp = ptr;
    for (i = 0; i < length; i+= cols) {
        if (hdr != NULL)
            ph_printf("%s", hdr);

        if ((flags & HD_OMIT_COUNT) == 0)
            ph_printf("%04x  ", i);

        if ((flags & HD_OMIT_HEX) == 0) {
            for (j = 0; j < cols; j++) {
                k = i + j;
                if (k < length)
                    ph_printf("%c%02x", delim, cp[k]);
                else
                    ph_printf("   ");
            }
        }

        if ((flags & HD_OMIT_CHARS) == 0) {
            ph_printf("  |");
            for (j = 0; j < cols; j++) {
                k = i + j;
                if (k >= length)
                    ph_printf(" ");
                else if (cp[k] >= ' ' && cp[k] <= '~')
                    ph_printf("%c", cp[k]);
                else
                    ph_printf(".");
            }
            ph_printf("|");
        }
        ph_printf("\n");
    }
}

