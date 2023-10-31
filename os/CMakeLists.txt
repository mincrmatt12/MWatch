cmake_minimum_required(VERSION 3.12)

# Set our toolchain file first 
set(CMAKE_INTERPROCEDURAL_OPTIMZATION ON)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain-clang-rp.cmake)
set(PICO_SDK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../vendor/pico-sdk)
set(CMSIS_DIR ${CMAKE_CURRENT_LIST_DIR}/../vendor/CMSIS_5)

project(mwatch-os-main-sys C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include(min-pico-sdk-targets)
find_program(openocd_executable openocd)

# get the sources
file(GLOB_RECURSE os_srcs FOLLOW_SYMLINKS 
	${CMAKE_CURRENT_LIST_DIR}/src/*.cpp 
	${CMAKE_CURRENT_LIST_DIR}/src/*.c 
	${CMAKE_CURRENT_LIST_DIR}/src/*.S
)

add_executable(mwos)
target_sources(mwos PRIVATE ${os_srcs})
target_include_directories(mwos PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

add_subdirectory(boot)
add_subdirectory(pio)
add_subdirectory(res)

set(MWKERNEL_EMULATE_SYMTRANSFER TRUE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../kernel mwkernel)

target_link_libraries(mwos PRIVATE
	mwos_pios
	mwos_res
	mwkernel
	boot_rom_stage2
	picosdk
)
target_link_libraries(mwos PUBLIC c clang_rt.builtins)

target_compile_options(mwos PUBLIC -Rpass-missed=coro-elide -Rpass=coro-elide)
target_link_options(mwos PUBLIC    -Rpass-missed=coro-elide -Rpass=coro-elide)

target_compile_options(mwos
	PUBLIC
	-Wno-duplicate-decl-specifier
	$<$<COMPILE_LANGUAGE:CXX>: -Wno-register>
)

add_library(mwos_linkscript INTERFACE)
target_link_options(mwos_linkscript INTERFACE
	LINKER:--script LINKER:${CMAKE_CURRENT_LIST_DIR}/ld/kernel.ld
	LINKER:--Map=$<TARGET_FILE:mwos>.map
	LINKER:--gc-sections
	LINKER:--nmagic
)

add_library(cm0_cmsis INTERFACE)
target_include_directories(cm0_cmsis INTERFACE ${CMSIS_DIR}/CMSIS/Core/Include)

target_link_libraries(mwos PRIVATE cm0_cmsis)
target_link_libraries(mwos PUBLIC mwos_linkscript)

if (EXISTS ${openocd_executable})
	message(STATUS "Found OPENOCD " ${openocd_executable})

	# add upload target
	add_custom_target(upload
		COMMAND ${openocd_executable} -f ${CMAKE_CURRENT_LIST_DIR}/../util/openocd-rp.cfg -c "program $<TARGET_FILE:mwos> verify reset exit" || echo "program $<TARGET_FILE:mwos> verify reset" | telnet localhost 4444
	)

	add_dependencies(upload mwos)
else()
	message(STATUS "Disabling upload target due to no OpenOCD")
endif()
