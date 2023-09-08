include(${CMAKE_CURRENT_LIST_DIR}/toolchain-clang.cmake)

set(MWATCH_TOOLCHAIN_SYSROOT ${MWATCH_TOOLCHAIN_DIR}/../lib/clang-runtimes/arm-none-eabi/armv6m_soft_nofp)
set(MWATCH_TOOLCHAIN_COMMON_FLAGS "--target=armv6m-none-eabi -mfloat-abi=soft -march=armv6m -mcpu=cortex-m0plus --sysroot ${MWATCH_TOOLCHAIN_SYSROOT}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MWATCH_TOOLCHAIN_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MWATCH_TOOLCHAIN_COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS ${MWATCH_TOOLCHAIN_COMMON_FLAGS})
