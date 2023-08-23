include(${CMAKE_CURRENT_LIST_DIR}/toolchain-clang.cmake)

set(MWATCH_TOOLCHAIN_SYSROOT ${MWATCH_TOOLCHAIN_DIR}/../lib/clang-runtimes/arm-none-eabi/armv7em_hard_fpv4_sp_d16)
set(MWATCH_TOOLCHAIN_COMMON_FLAGS "--target=arm-none-eabif -mfloat-abi=hard -march=armv7em -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 --sysroot ${MWATCH_TOOLCHAIN_SYSROOT}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MWATCH_TOOLCHAIN_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MWATCH_TOOLCHAIN_COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS ${MWATCH_TOOLCHAIN_COMMON_FLAGS})
