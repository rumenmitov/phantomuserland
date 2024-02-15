# This cmake makes it possible to use setjmp() and longjmp() functions
# provided by libc_setjmp genode package (when building with goa)

set (LIBC_SETJMP_DIR "${CMAKE_SOURCE_DIR}/../var/depot/genodelabs/api/libc_setjmp/2018-05-28")

include_directories("${LIBC_SETJMP_DIR}/include/libc-genode")

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set (LIBC_SETJMP_PLATFORM "arm")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
	message(FATAL_ERROR "No RISC-V :(")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (LIBC_SETJMP_PLATFORM "amd64")
else ()
    # Build as X86_32 by default in 32-bit platform
    set (LIBC_SETJMP_PLATFORM "i386")
endif ()

include_directories("${LIBC_SETJMP_DIR}/include/libc-${LIBC_SETJMP_PLATFORM}")
set (LIBC_SETJMP_SOURCES
    "${LIBC_SETJMP_DIR}/src/lib/libc/lib/libc/${LIBC_SETJMP_PLATFORM}/gen/setjmp.S"
    "${LIBC_SETJMP_DIR}/src/lib/libc/lib/libc/${LIBC_SETJMP_PLATFORM}/gen/_setjmp.S"
	genode_env/sigprocmask_stub.c
)

add_definitions(-DUSE_LIBC_SETJMP=1)
