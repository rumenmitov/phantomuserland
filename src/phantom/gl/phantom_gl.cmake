set (PHANTOM_GL_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${PHANTOM_GL_DIR})

set (source_all 
    specbuf.c 
    phantom_loop.c
    list.c 
    error.c 
    texture.c 
    select.c 
    init.c 
    light.c 
    arrays.c 
    vertex.c 
    oscontext.c 
    image_util.c 
    clip.c 
    phantomdrvr.c 
    api.c 
    misc.c 
    memory.c 
    clear.c 
    zdither.c 
    msghandling.c 
    ztriangle.c 
    zline.c 
    zmath.c 
    zbuffer.c 
    matrix.c 
    texobj.c 
    get.c
)

# convert filenames to absolute paths
list(TRANSFORM source_all PREPEND ${PHANTOM_GL_DIR}/)

set (PHANTOM_GL_SOURCE ${source_all})
