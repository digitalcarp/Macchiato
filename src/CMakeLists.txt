# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

add_library(macchiato INTERFACE)
# set(SOURCES
#     macchiato.cc
# )
#
# if(macchiato_BUILD_SHARED_LIBS)
#     add_library(macchiato SHARED ${SOURCES})
# else()
#     add_library(macchiato STATIC ${SOURCES})
# endif()

mct_target_cxx_standard(macchiato INTERFACE)
target_include_directories(macchiato INTERFACE include)
# target_link_libraries(macchiato PRIVATE fmt::fmt)

if(macchiato_BUILD_TESTING)
    set(TEST_SOURCES
        bitset_test.cc
    )

    add_executable(macchiato_test ${TEST_SOURCES})
    mct_target_cxx_standard(macchiato_test PRIVATE)

    target_link_libraries(macchiato_test PRIVATE
        macchiato
        Catch2::Catch2WithMain
        fmt::fmt
    )

    catch_discover_tests(macchiato_test)
endif()
