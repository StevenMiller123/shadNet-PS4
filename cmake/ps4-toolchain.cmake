# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

execute_process(COMMAND uname -s OUTPUT_VARIABLE PS4_HOST_UNAME_S OUTPUT_STRIP_TRAILING_WHITESPACE)
if(PS4_HOST_UNAME_S STREQUAL "Linux")
    set(CMAKE_C_COMPILER   clang)
    set(CMAKE_CXX_COMPILER clang++)
    set(PS4_LINKER_BIN     ld.lld)
    set(PS4_CDIR           linux)
elseif(PS4_HOST_UNAME_S STREQUAL "Darwin")
    set(CMAKE_C_COMPILER   /usr/local/opt/llvm/bin/clang)
    set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
    set(PS4_LINKER_BIN     /usr/local/opt/llvm/bin/ld.lld)
    set(PS4_CDIR           macos)
else()
    message(FATAL_ERROR "Unsupported host platform: ${PS4_HOST_UNAME_S}")
endif()

set(CMAKE_C_COMPILER_WORKS   TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT DEFINED OO_PS4_TOOLCHAIN OR OO_PS4_TOOLCHAIN STREQUAL "")
    set(OO_PS4_TOOLCHAIN "$ENV{OO_PS4_TOOLCHAIN}")
endif()
if(OO_PS4_TOOLCHAIN STREQUAL "")
    message(FATAL_ERROR "OO_PS4_TOOLCHAIN is not set")
endif()

set(CMAKE_SYSROOT "${OO_PS4_TOOLCHAIN}")

set(PS4_COMMON_FLAGS "--target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -isystem ${OO_PS4_TOOLCHAIN}/include/c++/v1 -isystem ${OO_PS4_TOOLCHAIN}/include")
set(CMAKE_C_FLAGS_INIT   "${PS4_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${PS4_COMMON_FLAGS}")
