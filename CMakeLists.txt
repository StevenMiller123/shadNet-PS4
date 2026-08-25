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
    message(FATAL_ERROR "OO_PS4_TOOLCHAIN is not set")
endif()
if(GOLDHEN_SDK STREQUAL "")
    message(FATAL_ERROR "GOLDHEN_SDK is not set")
endif()

set(PROJ_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(EXTERNALS_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/externals")
set(COMMON_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/src/common")
set(BUILD_FOLDER   "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins")

set(TARGET_PRX_DIR "${BUILD_FOLDER}/prx${PLUGIN_BUILD_TYPE}")
set(TARGET_ELF_DIR "${BUILD_FOLDER}/elf${PLUGIN_BUILD_TYPE}")
set(TARGET_ELF     "${TARGET_ELF_DIR}/${PLUGIN_NAME}")
set(TARGET_PRX     "${TARGET_PRX_DIR}/${PLUGIN_NAME}.prx")

file(MAKE_DIRECTORY "${TARGET_PRX_DIR}")
file(MAKE_DIRECTORY "${TARGET_ELF_DIR}")

set(EXTERNAL_MODULE_NAMES "")
if(IS_DIRECTORY "${EXTERNALS_DIR}")
    file(GLOB EXTERNAL_CANDIDATES RELATIVE "${EXTERNALS_DIR}" "${EXTERNALS_DIR}/*")
    foreach(CANDIDATE ${EXTERNAL_CANDIDATES})
        if(IS_DIRECTORY "${EXTERNALS_DIR}/${CANDIDATE}")
            list(APPEND EXTERNAL_MODULE_NAMES "${CANDIDATE}")
        endif()
    endforeach()
endif()

set(PLUGIN_INCLUDE_FLAGS "")
set(EXTERNAL_INCLUDE_DIRS "")

foreach(EXT_NAME ${EXTERNAL_MODULE_NAMES})
    set(EXT_ROOT "${EXTERNALS_DIR}/${EXT_NAME}")
    if(IS_DIRECTORY "${EXT_ROOT}/include")
        set(EXT_INCLUDE "${EXT_ROOT}/include")
    else()
        set(EXT_INCLUDE "${EXT_ROOT}")
    endif()
    list(APPEND EXTERNAL_INCLUDE_DIRS "${EXT_INCLUDE}")
    list(APPEND PLUGIN_INCLUDE_FLAGS "-I${EXT_INCLUDE}")
endforeach()

list(APPEND PLUGIN_INCLUDE_FLAGS
    "-I${GOLDHEN_SDK}/include"
    "-I${INCLUDE_DIR}"
    "-I${COMMON_DIR}"
)

set(PLUGIN_SYSROOT_FLAGS
    "SHELL:-isysroot ${OO_PS4_TOOLCHAIN}"
    "SHELL:-isystem ${OO_PS4_TOOLCHAIN}/include"
    "SHELL:-isystem ${OO_PS4_TOOLCHAIN}/include/c++/v1"
)

set(EXTRA_FLAGS ${PLUGIN_DEBUG_FLAG} ${PLUGIN_LOG_TYPE} -fcolor-diagnostics -Wall)

set(PLUGIN_CROSS_COMPILE_FLAGS
    --target=x86_64-pc-freebsd12-elf
    -fPIC
    -funwind-tables
    ${EXTRA_FLAGS}
)

set(PLUGIN_LIBS
    -lSceLibcInternal
    -lSceVideoOut
    -lGoldHEN_Hook
    -lkernel
    -lc
    -lc++
    -lSceSysmodule
)

set(EXTERNAL_OBJECT_TARGETS "")

foreach(EXT_NAME ${EXTERNAL_MODULE_NAMES})
    set(EXT_ROOT "${EXTERNALS_DIR}/${EXT_NAME}")
    file(GLOB EXT_ROOT_C      "${EXT_ROOT}/*.c")
    file(GLOB EXT_ROOT_CC     "${EXT_ROOT}/*.cc")
    file(GLOB EXT_ROOT_CPP    "${EXT_ROOT}/*.cpp")
    file(GLOB EXT_SOURCE_CPPM "${EXT_ROOT}/*.cppm")
    file(GLOB EXT_SRC_C       "${EXT_ROOT}/src/*.c")
    file(GLOB EXT_SRC_CC      "${EXT_ROOT}/src/*.cc")
    file(GLOB EXT_SRC_CPP     "${EXT_ROOT}/src/*.cpp")
    file(GLOB EXT_SOURCE_CPPM "${EXT_ROOT}/src/*.cppm")
    file(GLOB EXT_SOURCE_C    "${EXT_ROOT}/source/*.c")
    file(GLOB EXT_SOURCE_CC   "${EXT_ROOT}/source/*.cc")
    file(GLOB EXT_SOURCE_CPP  "${EXT_ROOT}/source/*.cpp")
    file(GLOB EXT_SOURCE_CPPM "${EXT_ROOT}/source/*.cppm")

    set(EXT_ALL_SOURCES
        ${EXT_ROOT_C} ${EXT_ROOT_CC} ${EXT_ROOT_CPP} ${EXT_ROOT_CPPM}
        ${EXT_SRC_C} ${EXT_SRC_CC} ${EXT_SRC_CPP} ${EXT_SRC_CPPM}
        ${EXT_SOURCE_C} ${EXT_SOURCE_CC} ${EXT_SOURCE_CPP} ${EXT_SOURCE_CPPM}
    )

    if(EXT_ALL_SOURCES)
        set(EXT_TARGET "ext_${EXT_NAME}")
        add_library(${EXT_TARGET} OBJECT ${EXT_ALL_SOURCES})

        target_compile_options(${EXT_TARGET} PRIVATE
            ${PLUGIN_CROSS_COMPILE_FLAGS}
            ${PLUGIN_INCLUDE_FLAGS}
            ${PLUGIN_SYSROOT_FLAGS}
        )
        list(APPEND EXTERNAL_OBJECT_TARGETS ${EXT_TARGET})
        message(STATUS "adding ${EXT_NAME}")
    endif()
endforeach()

file(GLOB C_SOURCES     "${PROJ_DIR}/*.c")
file(GLOB CPP_SOURCES    "${PROJ_DIR}/*.cpp")
file(GLOB COMMON_SOURCES "${COMMON_DIR}/*.cpp")
set(ALL_SOURCES ${C_SOURCES} ${CPP_SOURCES} ${COMMON_SOURCES})

if(NOT ALL_SOURCES)
    message(FATAL_ERROR "No source files found under ${PROJ_DIR} or ${COMMON_DIR}")
endif()

add_library(plugin_objects OBJECT ${ALL_SOURCES})

target_compile_options(plugin_objects PRIVATE
    ${PLUGIN_CROSS_COMPILE_FLAGS}
    ${PLUGIN_INCLUDE_FLAGS}
    ${PLUGIN_SYSROOT_FLAGS}
)

if(EXTERNAL_OBJECT_TARGETS)
    add_dependencies(plugin_objects ${EXTERNAL_OBJECT_TARGETS})
endif()

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

set(ALL_LINK_OBJECTS "$<TARGET_OBJECTS:plugin_objects>")
foreach(EXT_TARGET ${EXTERNAL_OBJECT_TARGETS})
    list(APPEND ALL_LINK_OBJECTS "$<TARGET_OBJECTS:${EXT_TARGET}>")
endforeach()

add_custom_command(
    OUTPUT "${TARGET_PRX}"
    COMMAND ${PLUGIN_LD}
            -m elf_x86_64 -pie
            --script ${OO_PS4_TOOLCHAIN}/link.x
            -e _init --eh-frame-hdr
            -L${OO_PS4_TOOLCHAIN}/lib
            -L${GOLDHEN_SDK}
            ${GOLDHEN_SDK}/build/crtprx.o
            ${ALL_LINK_OBJECTS}
            -o ${TARGET_ELF}.elf
            ${PLUGIN_LIBS}
    COMMAND ${OO_PS4_TOOLCHAIN}/bin/${PLUGIN_CDIR}/create-fself
            -in=${TARGET_ELF}.elf
            -out=${TARGET_ELF}.oelf
            --lib=${TARGET_PRX}
            --paid 0x3800000000000011
    COMMAND chmod +x "${TARGET_PRX}"
    COMMAND_EXPAND_LISTS
    DEPENDS plugin_objects ${EXTERNAL_OBJECT_TARGETS} ${ALL_LINK_OBJECTS}
    COMMENT "Linking and signing ${PLUGIN_NAME}.prx"
    VERBATIM
)
add_custom_target(plugin_prx ALL DEPENDS "${TARGET_PRX}")