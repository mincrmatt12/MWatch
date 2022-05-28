cmake_minimum_required(VERSION 3.12)
include(${CMAKE_CURRENT_LIST_DIR}/../vendor/pico-sdk/pico_sdk_init.cmake)  # setup SDK + toolchain options

project(mwatch-os-main-sys C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()  # prepare sdk paths + create targets

# get the sources
file(GLOB_RECURSE os_srcs FOLLOW_SYMLINKS 
	${CMAKE_CURRENT_LIST_DIR}/src/*.cpp 
	${CMAKE_CURRENT_LIST_DIR}/src/*.c 
	${CMAKE_CURRENT_LIST_DIR}/src/*.s
)

add_executable(mwos)

target_sources(mwos PRIVATE ${os_srcs})

target_link_libraries(mwos PRIVATE
	pico_stdlib
	hardware_pio
	hardware_dma
	hardware_gpio
	hardware_pwm
)

pico_enable_stdio_usb(mwos 0)
pico_enable_stdio_uart(mwos 1)

pico_add_extra_outputs(mwos)
pico_generate_pio_header(mwos ${CMAKE_CURRENT_LIST_DIR}/src/hw/screen.pio)
#pico_enable_stdio_uart(mwos 0)

target_compile_options(mwos
	PUBLIC
	$<$<COMPILE_LANGUAGE:CXX>: -Wno-register -Wno-volatile>
)
