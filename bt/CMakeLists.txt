cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain-clang-bt.cmake)

project(mwatch-os-bt-sys C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

file(GLOB_RECURSE os_srcs FOLLOW_SYMLINKS
	${CMAKE_CURRENT_LIST_DIR}/src/*.cpp 
	${CMAKE_CURRENT_LIST_DIR}/src/*.c 
	${CMAKE_CURRENT_LIST_DIR}/src/*.S
)

add_executable(mwbt)
target_sources(mwbt PRIVATE ${os_srcs})

set(NRFX_DIR ${CMAKE_CURRENT_LIST_DIR}/../vendor/nrfx)
set(CMSIS_DIR ${CMAKE_CURRENT_LIST_DIR}/../vendor/CMSIS_5)

file(GLOB_RECURSE nrfx_drivers_srcs ${NRFX_DIR}/drivers/src/*.c)

find_program(openocd_executable openocd)

add_library(nrfx STATIC
	${nrfx_drivers_srcs}
	${NRFX_DIR}/helpers/nrfx_flag32_allocator.c
	${NRFX_DIR}/soc/nrfx_atomic.c
)

target_include_directories(nrfx PUBLIC
	${NRFX_DIR}
	${NRFX_DIR}/drivers/include
	${CMAKE_CURRENT_LIST_DIR}/cfg
)

add_library(mdk_sys STATIC
	${NRFX_DIR}/mdk/system_nrf52833.c
)

target_include_directories(mdk_sys PUBLIC
	${NRFX_DIR}/mdk
	${CMSIS_DIR}/CMSIS/Core/Include
)

target_compile_definitions(mdk_sys PUBLIC
	-DCONFIG_GPIO_AS_PINRESET
	-DNRF52832_XXAA
)

# Setup kernel
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../kernel mwkernel)

target_compile_options(mdk_sys PUBLIC
	$<$<COMPILE_LANGUAGE:CXX>: -fno-threadsafe-statics -Wno-register>
)

add_library(mwbt_linkscript INTERFACE)
target_link_options(mwbt_linkscript INTERFACE
	LINKER:--script LINKER:${CMAKE_CURRENT_LIST_DIR}/ld/kernel.ld
	LINKER:--gc-sections
	LINKER:--nmagic
)

target_link_libraries(nrfx PUBLIC mdk_sys)

# Compiler builtins (either literally libc.a or the c target that gets produced by in-tree picolibc)
target_link_libraries(mwbt PUBLIC c clang_rt.builtins)
target_link_libraries(mwbt PUBLIC mwbt_linkscript)
target_link_libraries(mwbt PUBLIC nrfx)
target_link_libraries(mwbt PRIVATE mwkernel)

if (EXISTS ${openocd_executable})
	message(STATUS "Found OPENOCD " ${openocd_executable})

	# add upload target
	add_custom_target(upload
		COMMAND ${openocd_executable} -f ${CMAKE_CURRENT_LIST_DIR}/../util/openocd-bt.cfg -c "program $<TARGET_FILE:mwbt> verify reset exit" || echo "program $<TARGET_FILE:mwbt> verify reset" | telnet localhost 4444
	)

	add_dependencies(upload mwbt)
else()
	message(STATUS "Disabling upload target due to no OpenOCD")
endif()
