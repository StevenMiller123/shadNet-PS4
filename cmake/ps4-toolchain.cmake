# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(PS4_LLVM_HINTS "")
foreach(PS4_ENV_VAR LLVM_DIR LLVM_PATH)
    if(NOT "$ENV{${PS4_ENV_VAR}}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{${PS4_ENV_VAR}}" PS4_ENV_PATH)
        list(APPEND PS4_LLVM_HINTS "${PS4_ENV_PATH}/bin")
    endif()
endforeach()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    # clang is normally on PATH
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND PS4_LLVM_HINTS "/usr/local/opt/llvm/bin" "/opt/homebrew/opt/llvm/bin")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND PS4_LLVM_HINTS "C:/Program Files/LLVM/bin")
else()
    message(FATAL_ERROR "Unsupported host platform: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

find_program(PS4_CLANG   NAMES clang   HINTS ${PS4_LLVM_HINTS})
find_program(PS4_CLANGXX NAMES clang++ HINTS ${PS4_LLVM_HINTS})
if(NOT PS4_CLANG OR NOT PS4_CLANGXX)
    message(FATAL_ERROR "clang/clang++ not found")
endif()
set(CMAKE_C_COMPILER   "${PS4_CLANG}")
set(CMAKE_CXX_COMPILER "${PS4_CLANGXX}")

if(CMAKE_HOST_WIN32)
    find_program(PS4_LLVM_AR     NAMES llvm-ar     HINTS ${PS4_LLVM_HINTS})
    find_program(PS4_LLVM_RANLIB NAMES llvm-ranlib HINTS ${PS4_LLVM_HINTS})
    if(PS4_LLVM_AR)
        set(CMAKE_AR "${PS4_LLVM_AR}" CACHE FILEPATH "" FORCE)
    endif()
    if(PS4_LLVM_RANLIB)
        set(CMAKE_RANLIB "${PS4_LLVM_RANLIB}" CACHE FILEPATH "" FORCE)
    endif()
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

file(TO_CMAKE_PATH "${OO_PS4_TOOLCHAIN}" OO_PS4_TOOLCHAIN)
string(REGEX REPLACE "/+$" "" OO_PS4_TOOLCHAIN "${OO_PS4_TOOLCHAIN}")

set(CMAKE_SYSROOT "${OO_PS4_TOOLCHAIN}")

set(PS4_COMMON_FLAGS "--target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -isystem \"${OO_PS4_TOOLCHAIN}/include/c++/v1\" -isystem \"${OO_PS4_TOOLCHAIN}/include\"")
set(CMAKE_C_FLAGS_INIT   "${PS4_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${PS4_COMMON_FLAGS}")
