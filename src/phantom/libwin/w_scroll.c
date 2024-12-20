/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Scrolling of window contents
 *
**/



//#include "drv_video_screen.h"
#include <phantom_assert.h>
#include <ph_string.h>
#include <errno.h>
#include <kernel/libkern.h>
//#include <video/font.h>
#include <video/window.h>



void w_scroll_up( window_handle_t win, int npix, rgba_t color)
{
    assert(npix >= 0);
    //assert(npix <= win->ysize);

    if(npix >= win->ysize)
    {
        w_fill( win, color );
        return;
    }

    struct rgba_t *dst = win->w_pixel + (win->xsize * npix);
    int len = win->xsize * (win->ysize - npix);
    ph_memmove( dst, win->w_pixel, len*sizeof(struct rgba_t) );
    int clrlen = win->xsize * npix;
    dst = win->w_pixel;
    while( clrlen-- )
    {
        *dst++ = color;
    }
}







// -----------------------------------------------------------------------
// Scroll
// -----------------------------------------------------------------------

/*
errno_t w_scroll_hor( window_handle_t w, int x, int y, int xs, int ys, int s )
{
    if( x < 0 || y < 0 || x+xs > w->xsize || y+ys > w->ysize )
        return EINVAL;

    rgba_scroll_hor( w->pixel+x+(y*w->xsize), xs, ys, w->xsize, s, w->bg );
    return 0;
}
*/


errno_t w_scroll_hor( window_handle_t h, int x, int y, int xs, int ys, int s )
{
    errno_t ret = 0;
#if NEW_WINDOWS
    window_t *w = pool_get_el(wp,h);
#else
    struct drv_video_window *w = h;
#endif


    if( x < 0 || y < 0 || x+xs > w->xsize || y+ys > w->ysize )
        ret = EINVAL;
    else
        rgba_scroll_hor( w->w_pixel+x+(y*w->xsize), xs, ys, w->xsize, s, w->bg );

#if NEW_WINDOWS
    pool_release_el( wp, h );
#endif
    return ret;
}

