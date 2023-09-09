# LLVM arm-none-eabi toolchain: path must be set as MWATCH_TOOLCHAIN_DIR, otherwise assumes /opt/llvm-arm/17/bin
set(MWATCH_TOOLCHAIN_DIR /opt/llvm-arm/18/bin CACHE PATH "Path to the LLVM arm toolchain bin directory.")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER ${MWATCH_TOOLCHAIN_DIR}/clang)
set(CMAKE_CXX_COMPILER ${MWATCH_TOOLCHAIN_DIR}/clang++)
set(CMAKE_ASM_COMPILER ${MWATCH_TOOLCHAIN_DIR}/clang)
set(CMAKE_LINKER ${MWATCH_TOOLCHAIN_DIR}/clang)
set(CMAKE_AR ${MWATCH_TOOLCHAIN_DIR}/llvm-ar)
set(CMAKE_RANLIB ${MWATCH_TOOLCHAIN_DIR}/llvm-ranlib)
set(CMAKE_OBJCOPY ${MWATCH_TOOLCHAIN_DIR}/llvm-objcopy)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# we always build with debug info because of the crash handler
set(CMAKE_C_FLAGS "-nostdlib -mthumb -ggdb3")
set(CMAKE_CXX_FLAGS "-nostdlib -mthumb -fno-exceptions -fno-rtti -Wno-register -ggdb3")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -ggdb3")
