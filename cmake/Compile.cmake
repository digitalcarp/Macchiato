# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

function(mct_target_cxx_standard CXX_TARGET TYPE)
    target_compile_features(${CXX_TARGET} ${TYPE} cxx_std_20)
    set_target_properties(${CXX_TARGET} PROPERTIES CXX_STANDARD_REQUIRED ON)
    set_target_properties(${CXX_TARGET} PROPERTIES CXX_EXTENSIONS OFF)
endfunction()

function(mct_setup_target CXX_TARGET)
    set(COMPILE_OPTS "")

    if(WITH_SAN)
        list(APPEND COMPILE_OPTS "-fsanitize=${WITH_SAN}")
        list(APPEND LINK_OPTS    "-fsanitize=${WITH_SAN}")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND COMPILE_OPTS -Wall -Wextra)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND COMPILE_OPTS -Wall -Wextra)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        list(APPEND COMPILE_OPTS /W4)
    endif()

    target_compile_options(${CXX_TARGET} PRIVATE ${COMPILE_OPTS})

    if(macchiato_WARNINGS_AS_ERRORS)
        set_target_properties(${CXX_TARGET} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
    endif()
endfunction()
