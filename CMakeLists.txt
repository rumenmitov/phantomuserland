cmake_minimum_required(VERSION 3.10)
project(PhantomGenode)

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

string(REPLACE "-Werror" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_executable(isomem ${SOURCES})

set_property(TARGET isomem PROPERTY C_STANDARD 99)


target_compile_options(isomem PRIVATE
	-Werror=implicit-function-declaration
)

# Checking if we want to run only tests

if (DEFINED PHANTOM_TESTS_ONLY)
  add_definitions(-DPHANTOM_TESTS_ONLY)
endif ()
