set (WAMR_BUILD_PLATFORM "genode")
set (WAMR_BUILD_TARGET "X86_64")
set (WAMR_BUILD_LIBC_BUILTIN 0)
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_AOT 0)
set (WAMR_BUILD_FAST_INTERP 0)
set (WAMR_BUILD_BULK_MEMORY 1)
set (WAMR_BUILD_LIBC_WASI 0) # ???
set (WAMR_BUILD_SIMD 0)
set (WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/wasm-micro-runtime)

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
