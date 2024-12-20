/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Z buffer implementation.
 *
 * Z buffer value 0 is lowest (most far) position. It is supposed to be used by background.
 *
**/

#define DEBUG_MSG_PREFIX "zbuf"
#include <debug_ext.h>
#define debug_level_flow 8
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/debug.h>

//#include <drv_video_screen.h>
#include <phantom_types.h>
#include <phantom_libc.h>
#include <phantom_assert.h>

//#include <video.h>
#include <video/rect.h>
#include <video/zbuf.h>
#include <video/screen.h>

#include <ph_malloc.h>
#include <ph_string.h>


/** Z buffer itself. */

zbuf_t *zbuf = 0;

#if USE_ZBUF_SHADOW
// z buffer shadow, gets applied just before paint
static zbuf_t *zbuf_shadow = 0;

static void scr_zbuf_reset_shadow(void);
#endif

static u_int32_t zbsize = 0;
static u_int32_t zbwidth = 0;

static rect_t zbuf_rect;

void scr_zbuf_init()
{
    if(zbuf) ph_free(zbuf);

    zbsize = scr_get_xsize() * scr_get_ysize() * sizeof(zbuf_t);
    zbwidth = scr_get_xsize();

    zbuf = ph_malloc( zbsize );

    zbuf_rect.x = 0;
    zbuf_rect.y = 0;
    zbuf_rect.xsize = scr_get_xsize();
    zbuf_rect.ysize = scr_get_ysize();

    scr_zbuf_reset();

#if USE_ZBUF_SHADOW
    zbuf_shadow = ph_malloc( zbsize );
    scr_zbuf_reset_shadow();
#endif
}



void scr_zbuf_reset()
{
    ph_memset( zbuf, 0, zbsize );
}

/* wrong! not byte but zbuf_t!
void scr_zbuf_reset_z(zbuf_t z)
{
    //LOG_FLOW( 1, "%d", z );
    ph_memset( zbuf, z, zbsize );
}
*/


/**
 *
 * Reset given square to 0 for window.
 *
**/

void scr_zbuf_reset_win( window_handle_t w )
{
#if 1
    // TODO XXX HACK alert - hardcoded decorations size
    const int bw = 3;
    scr_zbuf_reset_square( w->x - bw, w->y - bw, w->xsize + bw*2, w->ysize + bw*2 + 31 );
#else
    scr_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );
#endif
}


/**
 *
 * Set given square to win z for window.
 *
**/

void scr_zbuf_set_win_z( window_handle_t w )
{
    // TODO XXX HACK alert - hardcoded decorations size
    const int bw = 3;
    scr_zbuf_reset_square_z( w->x - bw, w->y - bw, w->xsize + bw*2, w->ysize + bw*2 + 21, w->z );
}


/**
 *
 * Reset given square to given z position. Usually to zero.
 *
**/

void scr_zbuf_reset_square(int x, int y, int xsize, int ysize )
{
    scr_zbuf_reset_square_z( x, y, xsize, ysize, 0 );
}

static int zb_upside = 0;
void scr_zbuf_turn_upside(int v) { zb_upside = v; }

#if !USE_ZBUF_SHADOW
void scr_zbuf_reset_square_z(int x, int y, int xsize, int ysize, zbuf_t zpos )
{
    //LOG_FLOW( 2, "@ %d/%d, sz %d x %d, z %d", x, y, xsize, ysize, zpos );
    //lprintf( "zb reset @ %d/%d, sz %d x %d, z %d\n", x, y, xsize, ysize, zpos );

    rect_t out;
    rect_t a;

    a.x = x;
    a.y = y;
    a.xsize = xsize;
    a.ysize = ysize;

    if(!rect_mul( &out, &a, &zbuf_rect ))
        return;

    int ys;
    for(ys = 0; ys < out.ysize; ys++)
    {
        int linpos;
        if( zb_upside )
            linpos = out.x + out.y * zbwidth;
        else
            linpos = out.x + ( (scr_get_ysize()-1) - out.y) * zbwidth;

        out.y++;

        void *p = zbuf+linpos;
        size_t len = out.xsize * sizeof(zbuf_t);

        assert( p >= (void*)zbuf );
        assert( p+len <= ((void*)zbuf)+zbsize );

        ph_memset( p, zpos, len );
    }
}
#else

static void do_scr_zbuf_reset_square_z(zbuf_t *target, int x, int y, int xsize, int ysize, zbuf_t zpos )
{
    //LOG_FLOW( 2, "@ %d/%d, sz %d x %d, z %d", x, y, xsize, ysize, zpos );
    //lprintf( "zb reset @ %d/%d, sz %d x %d, z %d\n", x, y, xsize, ysize, zpos );

    rect_t out;
    rect_t a;

    a.x = x;
    a.y = y;
    a.xsize = xsize;
    a.ysize = ysize;

    if(!rect_mul( &out, &a, &zbuf_rect ))
        return;

    int ys;
    for(ys = 0; ys < out.ysize; ys++)
    {
        int linpos;
        if( zb_upside )
            linpos = out.x + out.y * zbwidth;
        else
            linpos = out.x + ( (scr_get_ysize()-1) - out.y) * zbwidth;

        out.y++;

        void *p = target+linpos;
        size_t len = out.xsize * sizeof(zbuf_t);

        assert( p >= (void*)target );
        assert( p+len <= ((void*)target)+zbsize );

        ph_memset( p, zpos, len );
    }
}

void scr_zbuf_reset_square_z(int x, int y, int xsize, int ysize, zbuf_t zpos )
{
    do_scr_zbuf_reset_square_z( zbuf, x, y, xsize, ysize, zpos );
}


void scr_zbuf_request_reset_square_z(int x, int y, int xsize, int ysize, zbuf_t zpos )
{
    do_scr_zbuf_reset_square_z( zbuf_shadow, x, y, xsize, ysize, zpos );
}

void scr_zbuf_request_reset_square(int x, int y, int xsize, int ysize )
{
    scr_zbuf_request_reset_square_z( x, y, xsize, ysize, 0 );
}

static void scr_zbuf_reset_shadow(void)
{
    // in shadow -1 means not active (== ZBUF_TOP) - not copied to zbuf no application
    //scr_zbuf_request_reset_square_z(int x, int y, int xsize, int ysize, zbuf_t zpos )
    ph_memset( zbuf_shadow, 0xFF, zbsize );
}

void scr_zbuf_apply_shadow(void)
{
    int n = scr_get_xsize() * scr_get_ysize();

    zbuf_t *src = zbuf_shadow;
    zbuf_t *dst = zbuf;

    while( n-- )
    {
        if( *src != ZBUF_TOP )
            *dst = *src;

        *src = ZBUF_TOP;

        src++;
        dst++;
    }
}

#endif

/**
 *
 * Check if we shall draw this pixel.
 *
 * linpos is linear position in videobuffer (usually x+y*xsize).
 * zpos is current window's z coordinate.
 *
**/
int scr_zbuf_check( int linpos, zbuf_t zpos )
{
    if( zbuf[linpos] > zpos ) return 0;
    if( zbuf[linpos] == zpos ) return 1;

    // Check only if we going to modify zbuf
    if( linpos > zbsize ) return 1;

    zbuf[linpos] = zpos;
    return 1;
}


void scr_zbuf_dump()
{
    int y;
    int ysize = scr_get_ysize();
    int xsize = scr_get_xsize();

    ph_printf("zbuf dump %d*%d\n", xsize, ysize );
    for(y = 0; y < ysize; y++)
    {
        int x;
        //int xsize = xsize;

        for( x = 0; x < xsize; x++ )
        {
            int linpos = x + y*xsize;
            ph_printf("%02X", zbuf[linpos] );
        }
        ph_printf("\n");
    }
}


#include <video/screen.h>

void scr_zbuf_paint()
{
    int np = scr_get_xsize() * scr_get_ysize();

#if 1
    if(scr_get_bpp() == 24)
    {
        zbuf_t *zbp = zbuf;
        rgb_t *d = (void *)video_drv->screen;
        while( np-- )
        {
            d->r = d->g = d->b = 4 * *zbp++;
            d++;
        }
    }
    if(scr_get_bpp() == 32)
#endif
    {
        zbuf_t *zbp = zbuf;
        rgba_t *d = (void *)video_drv->screen;
        while( np-- )
        {
            d->r = d->g = d->b = 4 * *zbp++;
            d->a = 0xFF;
            d++;
        }
    }
}



