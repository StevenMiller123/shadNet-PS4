# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.20)
include(ExternalProject)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ps4-toolchain.cmake" CACHE FILEPATH "PS4 toolchain")
endif()

get_filename_component(PLUGIN_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${PLUGIN_NAME} LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

execute_process(COMMAND uname -s OUTPUT_VARIABLE HOST_UNAME_S OUTPUT_STRIP_TRAILING_WHITESPACE)
if(HOST_UNAME_S STREQUAL "Linux")
    set(PLUGIN_LD   "ld.lld")
    set(PLUGIN_CDIR "linux")
elseif(HOST_UNAME_S STREQUAL "Darwin")
    set(PLUGIN_LD   "/usr/local/opt/llvm/bin/ld.lld")
    set(PLUGIN_CDIR "macos")
else()
    message(FATAL_ERROR "Unsupported host platform: ${HOST_UNAME_S}")
endif()

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
set(EXTERNALS_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/externals")
set(BUILD_FOLDER   "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins")

set(TARGET_PRX_DIR "${BUILD_FOLDER}/prx${PLUGIN_BUILD_TYPE}")
set(TARGET_ELF_DIR "${BUILD_FOLDER}/elf${PLUGIN_BUILD_TYPE}")
set(TARGET_ELF     "${TARGET_ELF_DIR}/${PLUGIN_NAME}")
set(TARGET_PRX     "${TARGET_PRX_DIR}/${PLUGIN_NAME}.prx")

file(MAKE_DIRECTORY "${TARGET_PRX_DIR}")
file(MAKE_DIRECTORY "${TARGET_ELF_DIR}")

set(PLUGIN_LIBS
    -lSceLibcInternal
    -lGoldHEN_Hook
    -lkernel
    -lc
    -lc++
    -lSceSysmodule
    -lSceNet
    -lSceNetCtl
    -lSceSysUtil
)

set(SHADNET_PROTO_OUT "${CMAKE_CURRENT_BINARY_DIR}/shadnet_proto_gen")
file(MAKE_DIRECTORY "${SHADNET_PROTO_OUT}")

set(COMMON_SOURCES src/common/error.cpp
                   src/common/goldhen_hook16.cpp
                   src/common/plugin_common.cpp
                   src/common/thread.cpp)

set(CORE_SOURCES src/core/libraries/np/np_handler.cpp)

set(SHADNET_SOURCES src/shadnet/client.cpp
                    src/shadnet/server_probe.cpp
                    ${SHADNET_PROTO_OUT}/shadnet.pb.cc)

set(MAIN_SOURCES src/client_main.cpp
                 src/main.c)

set(ALL_SOURCES ${CORE_SOURCES}
                ${SHADNET_SOURCES}
                ${MAIN_SOURCES}
                ${COMMON_SOURCES})

if(NOT ALL_SOURCES)
    message(FATAL_ERROR "No source files found under ${PROJ_DIR}")
endif()

add_library(plugin_objects OBJECT ${ALL_SOURCES})

target_include_directories(plugin_objects PRIVATE
    "${GOLDHEN_SDK}/include"
    "${PROJ_DIR}"
    "${SHADNET_PROTO_OUT}"
)

target_compile_options(plugin_objects PRIVATE
    ${PLUGIN_DEBUG_FLAG} ${PLUGIN_LOG_TYPE} -fcolor-diagnostics -Wall
)

set(PROTOBUF_HOST_SOURCE_DIR "${EXTERNALS_DIR}/protobuf")
set(PROTOBUF_HOST_BUILD_DIR  "${CMAKE_CURRENT_BINARY_DIR}/protobuf-host")
set(PROTOC_EXECUTABLE "${PROTOBUF_HOST_BUILD_DIR}/protoc")

if(IS_DIRECTORY "${PROTOBUF_HOST_SOURCE_DIR}")
    ExternalProject_Add(protobuf_host
        SOURCE_DIR       "${PROTOBUF_HOST_SOURCE_DIR}"
        BINARY_DIR       "${PROTOBUF_HOST_BUILD_DIR}"
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=Release
            -Dprotobuf_BUILD_TESTS=OFF
            -Dprotobuf_BUILD_SHARED_LIBS=OFF
            -Dprotobuf_ABSL_PROVIDER=module
            -Dprotobuf_WITH_ZLIB=OFF
        BUILD_BYPRODUCTS "${PROTOC_EXECUTABLE}"
        INSTALL_COMMAND ""
    )

    add_executable(protobuf::protoc IMPORTED GLOBAL)
    set_target_properties(protobuf::protoc PROPERTIES IMPORTED_LOCATION "${PROTOC_EXECUTABLE}")
    add_dependencies(protobuf::protoc protobuf_host)

    add_custom_command(
        OUTPUT
            "${SHADNET_PROTO_OUT}/shadnet.pb.cc"
            "${SHADNET_PROTO_OUT}/shadnet.pb.h"
        COMMAND protobuf::protoc
            "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/src/shadnet"
            "--cpp_out=${SHADNET_PROTO_OUT}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/shadnet/shadnet.proto"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/shadnet/shadnet.proto"
            protobuf::protoc
            "${PROTOC_EXECUTABLE}"
        COMMENT "Generating shadnet protobuf sources"
        VERBATIM
    )
endif()

set(EXTERNAL_LINK_FILES "")
set(EXTERNAL_BUILD_DEPS "")

function(shadnet_collect_targets_recursive DIR OUT_VAR)
    set(RESULT "")
    get_property(DIR_TARGETS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND RESULT ${DIR_TARGETS})
 
    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR ${SUBDIRS})
        shadnet_collect_targets_recursive("${SUBDIR}" SUB_RESULT)
        list(APPEND RESULT ${SUB_RESULT})
    endforeach()
 
    set(${OUT_VAR} ${RESULT} PARENT_SCOPE)
endfunction()

function(shadnet_add_cmake_external EXT_NAME)
    set(EXT_ROOT "${EXTERNALS_DIR}/${EXT_NAME}")
    if(NOT EXISTS "${EXT_ROOT}/CMakeLists.txt")
        return()
    endif()

    add_subdirectory("${EXT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/externals/${EXT_NAME}" EXCLUDE_FROM_ALL)


    shadnet_collect_targets_recursive("${EXT_ROOT}" EXT_TARGETS)
    set(REAL_LIB_TARGETS "")
    set(LOCAL_LINK_FILES "")
    foreach(TGT ${EXT_TARGETS})
        if(NOT TARGET ${TGT})
            continue()
        endif()
        get_target_property(TGT_TYPE ${TGT} TYPE)
        if(TGT_TYPE STREQUAL "STATIC_LIBRARY" OR TGT_TYPE STREQUAL "SHARED_LIBRARY")
            list(APPEND LOCAL_LINK_FILES "$<TARGET_FILE:${TGT}>")
            list(APPEND REAL_LIB_TARGETS ${TGT})
        elseif(TGT_TYPE STREQUAL "OBJECT_LIBRARY")
            list(APPEND LOCAL_LINK_FILES "$<TARGET_OBJECTS:${TGT}>")
            list(APPEND REAL_LIB_TARGETS ${TGT})
        elseif(TGT_TYPE STREQUAL "INTERFACE_LIBRARY")
            list(APPEND REAL_LIB_TARGETS ${TGT})
        endif()
    endforeach()

    if(REAL_LIB_TARGETS)
        target_link_libraries(plugin_objects PRIVATE ${REAL_LIB_TARGETS})
    endif()

    set(EXTERNAL_LINK_FILES ${EXTERNAL_LINK_FILES} ${LOCAL_LINK_FILES} PARENT_SCOPE)
    set(EXTERNAL_BUILD_DEPS ${EXTERNAL_BUILD_DEPS} ${REAL_LIB_TARGETS} PARENT_SCOPE)
    message(STATUS "Added ${EXT_NAME}")
endfunction()

set(SHADNET_HANDLED_EXTERNALS "")

if(IS_DIRECTORY "${EXTERNALS_DIR}/abseil-cpp")
    shadnet_add_cmake_external(abseil-cpp)
    list(APPEND SHADNET_HANDLED_EXTERNALS abseil-cpp)
endif()

if(IS_DIRECTORY "${EXTERNALS_DIR}/protobuf")
    set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "" FORCE)
    set(protobuf_PROTOC_EXE "${PROTOC_EXECUTABLE}" CACHE FILEPATH "" FORCE)
    set(utf8_range_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    shadnet_add_cmake_external(protobuf)
    list(APPEND SHADNET_HANDLED_EXTERNALS protobuf)
endif()

if(IS_DIRECTORY "${EXTERNALS_DIR}")
    file(GLOB EXTERNAL_CANDIDATES RELATIVE "${EXTERNALS_DIR}" "${EXTERNALS_DIR}/*")
    foreach(CANDIDATE ${EXTERNAL_CANDIDATES})
        if(NOT IS_DIRECTORY "${EXTERNALS_DIR}/${CANDIDATE}")
            continue()
        endif()
        list(FIND SHADNET_HANDLED_EXTERNALS "${CANDIDATE}" ALREADY_HANDLED)
        if(NOT ALREADY_HANDLED EQUAL -1)
            continue()
        endif()

        if(EXISTS "${EXTERNALS_DIR}/${CANDIDATE}/CMakeLists.txt")
            shadnet_add_cmake_external(${CANDIDATE})
        else()
            set(EXT_ROOT "${EXTERNALS_DIR}/${CANDIDATE}")
            if(IS_DIRECTORY "${EXT_ROOT}/include")
                target_include_directories(plugin_objects PRIVATE "${EXT_ROOT}/include")
            elseif(IS_DIRECTORY "${EXT_ROOT}/src")
                target_include_directories(plugin_objects PRIVATE "${EXT_ROOT}/src")
            else()
                target_include_directories(plugin_objects PRIVATE "${EXT_ROOT}")
            endif()

            file(GLOB EXT_ROOT_C   "${EXT_ROOT}/*.c")
            file(GLOB EXT_ROOT_CC  "${EXT_ROOT}/*.cc")
            file(GLOB EXT_ROOT_CPP "${EXT_ROOT}/*.cpp")
            file(GLOB EXT_SRC_C    "${EXT_ROOT}/src/*.c")
            file(GLOB EXT_SRC_CC   "${EXT_ROOT}/src/*.cc")
            file(GLOB EXT_SRC_CPP  "${EXT_ROOT}/src/*.cpp")
            set(EXT_ALL_SOURCES ${EXT_ROOT_C} ${EXT_ROOT_CC} ${EXT_ROOT_CPP} ${EXT_SRC_C} ${EXT_SRC_CC} ${EXT_SRC_CPP})

            if(EXT_ALL_SOURCES)
                set(EXT_TARGET "ext_${CANDIDATE}")
                add_library(${EXT_TARGET} OBJECT ${EXT_ALL_SOURCES})
                list(APPEND EXTERNAL_LINK_FILES "$<TARGET_OBJECTS:${EXT_TARGET}>")
                list(APPEND EXTERNAL_BUILD_DEPS ${EXT_TARGET})
                message(STATUS "Added ${CANDIDATE}")
            endif()
        endif()
    endforeach()
endif()

find_package(Git QUIET)
add_custom_target(build_info ALL
    COMMAND ${CMAKE_COMMAND}
        -DPROJ_DIR=${PROJ_DIR}
        -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateGitVersion.cmake
    COMMENT "Generating git_ver.h"
    VERBATIM
)
add_dependencies(plugin_objects build_info)
if(EXTERNAL_BUILD_DEPS)
    add_dependencies(plugin_objects ${EXTERNAL_BUILD_DEPS})
endif()

set(ALL_LINK_OBJECTS "$<TARGET_OBJECTS:plugin_objects>" ${EXTERNAL_LINK_FILES})

add_custom_command(
    OUTPUT "${TARGET_PRX}"
    COMMAND ${PLUGIN_LD}
            -m elf_x86_64 -pie
            --script ${OO_PS4_TOOLCHAIN}/link.x
            -e _init --eh-frame-hdr
            -L${OO_PS4_TOOLCHAIN}/lib
            -L${GOLDHEN_SDK}
            ${GOLDHEN_SDK}/build/crtprx.o
            --start-group
            ${ALL_LINK_OBJECTS}
            ${PLUGIN_LIBS}
            --end-group
            -o ${TARGET_ELF}.elf
    COMMAND ${OO_PS4_TOOLCHAIN}/bin/${PLUGIN_CDIR}/create-fself
            -in=${TARGET_ELF}.elf
            -out=${TARGET_ELF}.oelf
            --lib=${TARGET_PRX}
            --paid 0x3800000000000011
    COMMAND chmod +x "${TARGET_PRX}"
    COMMAND_EXPAND_LISTS
    DEPENDS plugin_objects ${EXTERNAL_BUILD_DEPS} ${ALL_LINK_OBJECTS}
    COMMENT "Linking ${PLUGIN_NAME}.prx"
    VERBATIM
)
add_custom_target(plugin_prx ALL DEPENDS "${TARGET_PRX}")