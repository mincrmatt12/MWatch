cmake_minimum_required(VERSION 3.12)

# Set our toolchain file first 
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain-clang.cmake)

# Convince the SDK to laod
include(${CMAKE_CURRENT_LIST_DIR}/../vendor/pico-sdk/pico_sdk_init.cmake)  # setup SDK + toolchain options

project(mwatch-os-main-sys C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

find_program(openocd_executable openocd)

set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/cfg/board)
set(PICO_BOARD mwatch)

# Initialize picolibc
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../vendor/picolibc)
# Initialize sdk
pico_sdk_init() 

# get the sources
file(GLOB_RECURSE os_srcs FOLLOW_SYMLINKS 
	${CMAKE_CURRENT_LIST_DIR}/src/*.cpp 
	${CMAKE_CURRENT_LIST_DIR}/src/*.c 
	${CMAKE_CURRENT_LIST_DIR}/src/*.s
)

add_executable(mwos)
target_sources(mwos PRIVATE ${os_srcs})

target_link_libraries(mwos PRIVATE
	hardware_pio
	hardware_dma
	hardware_gpio
	hardware_pwm
)

pico_enable_stdio_usb(mwos 0)
pico_enable_stdio_uart(mwos 1)

pico_add_extra_outputs(mwos)
pico_generate_pio_header(mwos ${CMAKE_CURRENT_LIST_DIR}/src/hw/screen.pio)

pico_define_boot_stage2(gd25lq ${CMAKE_CURRENT_LIST_DIR}/boot/stage2.S)
pico_set_boot_stage2(mwos gd25lq)

target_compile_options(mwos
	PUBLIC
	$<$<COMPILE_LANGUAGE:CXX>: -Wno-register -Wno-volatile>
)

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
