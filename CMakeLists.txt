# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

cmake_minimum_required(VERSION 3.25)

project(Macchiato
    VERSION 0.0.1
    DESCRIPTION "Two-level logic minimizer written in C++"
    LANGUAGES CXX C
)

include(cmake/StandardProjectSettings.cmake)
mct_force_out_of_source_builds()
mct_set_standard_settings()

option(macchiato_BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(macchiato_BUILD_TESTING "Build tests" OFF)
option(macchiato_WARNINGS_AS_ERRORS "Promote compiler warnings to errors" OFF)
option(macchiato_WITH_SAN "Build with runtime sanitizer" OFF)

include(cmake/Dependencies.cmake)
mct_setup_dependencies()

include(cmake/Compile.cmake)
if(macchiato_BUILD_TESTING)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    include(CTest)
endif()

add_subdirectory(src)

add_library(macchiato::macchiato ALIAS macchiato)
