# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

if(NOT DEFINED PROJ_DIR)
    message(FATAL_ERROR "PROJ_DIR not provided")
endif()

set(GIT_COMMIT "unknown")
set(GIT_BRANCH "unknown")
set(GIT_COUNT 0)

if(GIT_EXECUTABLE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        OUTPUT_VARIABLE GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} branch --show-current
        OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
        OUTPUT_VARIABLE GIT_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(GIT_COMMIT STREQUAL "")
        set(GIT_COMMIT "unknown")
    endif()
    if(GIT_BRANCH STREQUAL "")
        set(GIT_BRANCH "unknown")
    endif()
    if(GIT_COUNT STREQUAL "")
        set(GIT_COUNT 0)
    endif()
endif()

string(TIMESTAMP BUILD_DATE "%b %d %Y @ %H:%M:%S")

file(WRITE "${PROJ_DIR}/common/git_ver.h"
"#define GIT_COMMIT \"${GIT_COMMIT}\"
#define GIT_VER \"${GIT_BRANCH}\"
#define GIT_NUM ${GIT_COUNT}
#define BUILD_DATE \"${BUILD_DATE}\"
")
