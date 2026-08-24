# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.20)

execute_process(COMMAND uname -s OUTPUT_VARIABLE HOST_UNAME_S OUTPUT_STRIP_TRAILING_WHITESPACE)
if(HOST_UNAME_S STREQUAL "Linux")
    set(PLUGIN_CC  "clang")
    set(PLUGIN_CXX "clang++")
    set(PLUGIN_LD  "ld.lld")
    set(PLUGIN_CDIR "linux")
elseif(HOST_UNAME_S STREQUAL "Darwin")
    set(PLUGIN_CC  "/usr/local/opt/llvm/bin/clang")
    set(PLUGIN_CXX "/usr/local/opt/llvm/bin/clang++")
    set(PLUGIN_LD  "/usr/local/opt/llvm/bin/ld.lld")
    set(PLUGIN_CDIR "macos")
else()
    message(FATAL_ERROR "Unsupported host platform: ${HOST_UNAME_S}")
endif()

set(CMAKE_C_COMPILER   "${PLUGIN_CC}")
set(CMAKE_CXX_COMPILER "${PLUGIN_CXX}")

set(CMAKE_C_COMPILER_WORKS   TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

get_filename_component(PLUGIN_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${PLUGIN_NAME} LANGUAGES C CXX)

option(PLUGIN_DEBUG "Build debug configuration (Makefile DEBUG=1)" OFF)

if(PLUGIN_DEBUG)
    set(PLUGIN_DEBUG_FLAG "-D__FINAL__=0")
    set(PLUGIN_BUILD_TYPE "_debug")
else()
    set(PLUGIN_DEBUG_FLAG "-D__FINAL__=1")
    set(PLUGIN_BUILD_TYPE "_final")
endif()
set(PLUGIN_LOG_TYPE "-D__USE_PRINTF__")

if(NOT DEFINED OO_PS4_TOOLCHAIN)
    set(OO_PS4_TOOLCHAIN "$ENV{OO_PS4_TOOLCHAIN}")
endif()
if(NOT DEFINED GOLDHEN_SDK)
    set(GOLDHEN_SDK "$ENV{GOLDHEN_SDK}")
endif()
if(OO_PS4_TOOLCHAIN STREQUAL "")
    message(FATAL_ERROR "OO_PS4_TOOLCHAIN is not set (pass -DOO_PS4_TOOLCHAIN=... or export the env var)")
endif()
if(GOLDHEN_SDK STREQUAL "")
    message(FATAL_ERROR "GOLDHEN_SDK is not set (pass -DGOLDHEN_SDK=... or export the env var)")
endif()

set(PROJ_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(COMMON_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/src/common")
set(BUILD_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins")

set(TARGET_PRX_DIR "${BUILD_FOLDER}/prx${PLUGIN_BUILD_TYPE}")
set(TARGET_ELF_DIR "${BUILD_FOLDER}/elf${PLUGIN_BUILD_TYPE}")
set(TARGET_ELF     "${TARGET_ELF_DIR}/${PLUGIN_NAME}")
set(TARGET_PRX     "${TARGET_PRX_DIR}/${PLUGIN_NAME}.prx")
set(TARGET_STUB_SO "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.so")

file(MAKE_DIRECTORY "${TARGET_PRX_DIR}")
file(MAKE_DIRECTORY "${TARGET_ELF_DIR}")

file(GLOB C_SOURCES      "${PROJ_DIR}/*.c")
file(GLOB CPP_SOURCES     "${PROJ_DIR}/*.cpp")
set(ALL_SOURCES ${C_SOURCES} ${CPP_SOURCES} ${COMMON_SOURCES})

if(NOT ALL_SOURCES)
    message(FATAL_ERROR "No source files found under ${PROJ_DIR}")
endif()

set(PLUGIN_LIBS
    -lSceLibcInternal
    -lSceVideoOut
    -lGoldHEN_Hook
    -lkernel
    -lSceSysmodule
)

set(EXTRA_FLAGS ${PLUGIN_DEBUG_FLAG} ${PLUGIN_LOG_TYPE} -fcolor-diagnostics -Wall)

add_library(plugin_objects OBJECT ${ALL_SOURCES})

target_compile_options(plugin_objects PRIVATE
    --target=x86_64-pc-freebsd12-elf
    -fPIC
    -funwind-tables
    ${EXTRA_FLAGS}
    -isysroot ${OO_PS4_TOOLCHAIN}
    -isystem ${OO_PS4_TOOLCHAIN}/include
)
target_compile_options(plugin_objects PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-isystem${OO_PS4_TOOLCHAIN}/${INCLUDE_DIR}/c++/v1>
)
target_include_directories(plugin_objects PRIVATE
    ${GOLDHEN_SDK}/include
    ${INCLUDE_DIR}
    ${COMMON_DIR}
)

find_package(Git QUIET)
add_custom_target(build_info ALL
    COMMAND ${CMAKE_COMMAND}
        -DCOMMON_DIR=${COMMON_DIR}
        -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateGitVersion.cmake
    COMMENT "Generating git_ver.h"
    VERBATIM
)
add_dependencies(plugin_objects build_info)

add_custom_command(
    OUTPUT "${TARGET_PRX}"
    COMMAND ${PLUGIN_LD}
            -m elf_x86_64 -pie
            --script ${OO_PS4_TOOLCHAIN}/link.x
            -e _init --eh-frame-hdr
            -L${OO_PS4_TOOLCHAIN}/lib
            -L${GOLDHEN_SDK}
            ${GOLDHEN_SDK}/build/crtprx.o
            $<TARGET_OBJECTS:plugin_objects>
            -o ${TARGET_ELF}.elf
            ${PLUGIN_LIBS}
    COMMAND ${OO_PS4_TOOLCHAIN}/bin/${PLUGIN_CDIR}/create-fself
            -in=${TARGET_ELF}.elf
            -out=${TARGET_ELF}.oelf
            --lib=${TARGET_PRX}
            --paid 0x3800000000000011
    COMMAND chmod +x "${TARGET_PRX}"
    DEPENDS plugin_objects $<TARGET_OBJECTS:plugin_objects>
    COMMENT "Linking and signing ${PLUGIN_NAME}.prx"
    VERBATIM
)
add_custom_target(plugin_prx ALL DEPENDS "${TARGET_PRX}")