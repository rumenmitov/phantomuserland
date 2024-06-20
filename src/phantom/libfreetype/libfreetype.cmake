set (PHANTOM_LIBFREETYPE_DIR ${CMAKE_CURRENT_LIST_DIR})

set (source_all 
    sfnt.c 
    ftutil.c 
    ftoutln.c 
    ttsbit.c 
    ftbitmap.c 
    ttpost.c 
    ftstream.c 
    ftsystem.c 
    smooth.c 
    ttgxvar.c 
    ftinit.c 
    ftrfork.c 
    truetype.c 
    ftdebug.c 
    ftsmooth.c 
    ftobjs.c 
    ftgloadr.c 
    ftbase.c 
    ftdbgmem.c 
    sfobjs.c 
    autofit.c 
    fttrigon.c 
    ftnames.c 
    ttkern.c 
    ttload.c 
    ftcalc.c 
    ftgrays.c 
    ftglyph.c
)

# convert filenames to absolute paths
list(TRANSFORM source_all PREPEND ${PHANTOM_LIBFREETYPE_DIR}/)

set (PHANTOM_LIBFREETYPE_SOURCE ${source_all})
