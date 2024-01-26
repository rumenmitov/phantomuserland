cmake_minimum_required(VERSION 3.10)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project(PhantomGenode)

message("CMAKE C FLAGS DEFAULT: ${CMAKE_C_FLAGS}")
message("CMAKE EXECUTABLE LINKER FLAGS DEFAULT: ${CMAKE_EXE_LINKER_FLAGS}")

include (phantom/gl/phantom_gl.cmake)
include (genode_env/genode_env.cmake)
include (phantom/isomem/isomem.cmake)
include (phantom/libwin/libwin.cmake)
include (phantom/libfreetype/libfreetype.cmake)
include (phantom/vm/vm.cmake)

set(SOURCES
	${PHANTOM_GENODE_ENV_SOURCE}
	${PHANTOM_GL_SOURCE}
	${PHANTOM_ISOMEM_SOURCE}
	${PHANTOM_LIBWIN_SOURCE}
	${PHANTOM_LIBFREETYPE_SOURCE}
	${PHANTOM_PVM_SOURCE}
)

set(ARCH genode-amd64)

include_directories(include/stub_libc)
include_directories(include)
include_directories(include/${ARCH})

add_definitions(-DKERNEL)
add_definitions(-DPHANTOM_GENODE)
add_definitions(-DNO_NETWORK)

get_filename_component(relConfigInclude "include/kernel/config.h"
                       REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
add_compile_options(-include ${relConfigInclude})

# remove -Werror automatically added by goa
string(REPLACE "-Werror" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_executable(isomem ${SOURCES})

target_link_libraries (isomem vmlib)

set (CMAKE_C_STANDARD 99)

# disable -rdynamic flag
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration")

# Checking if we want to run only tests

if (DEFINED PHANTOM_TESTS_ONLY)
  add_definitions(-DPHANTOM_TESTS_ONLY)
endif ()
