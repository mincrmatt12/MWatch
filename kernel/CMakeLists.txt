cmake_minimum_required(VERSION 3.12)
project(mwatch-kernel CXX C)

set(CMAKE_CXX_STANDARD 20)

add_library(mwkernel STATIC)
file(GLOB_RECURSE mwkernel_srcs FOLLOW_SYMLINKS
	${CMAKE_CURRENT_LIST_DIR}/src/*.cpp 
	${CMAKE_CURRENT_LIST_DIR}/src/*.c)
target_sources(mwkernel PRIVATE ${mwkernel_srcs})
target_include_directories(mwkernel PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_compile_options(mwkernel PUBLIC -fno-exceptions)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wno-coroutine-missing-unhandled-exception COMPILER_SUPPORTS_CORO_NOEXCEPT)

if (COMPILER_SUPPORTS_CORO_NOEXCEPT)
	target_compile_options(mwkernel PUBLIC -Wno-coroutine-missing-unhandled-exception)
endif()

set(MWKERNEL_BUILD_STANDALONE_TESTS FALSE CACHE BOOL "enable for standalone compile")

if (NOT DEFINED MWKERNEL_EMULATE_SYMTRANSFER)
	set(MWKERNEL_EMULATE_SYMTRANSFER FALSE CACHE BOOL "enable to fake symmetric transfer for thumb1")
endif()

if (${MWKERNEL_EMULATE_SYMTRANSFER})
	target_compile_definitions(mwkernel PUBLIC -DMWKERNEL_EMULATE_SYMTRANSFER=1)
endif()

if (MWKERNEL_BUILD_STANDALONE_TESTS)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tests)
endif()
