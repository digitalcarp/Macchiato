# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

# set(SOURCES
#     macchiato.cc
# )

add_library(macchiato INTERFACE ${SOURCES})
# if(macchiato_BUILD_SHARED_LIBS)
#     add_library(macchiato SHARED ${SOURCES})
# else()
#     add_library(macchiato STATIC ${SOURCES})
# endif()

mct_target_cxx_standard(macchiato)
target_include_directories(macchiato PUBLIC ${PROJECT_SOURCE_DIR}/src/include)

# target_link_libraries(macchiato PRIVATE fmt::fmt)

if(macchiato_BUILD_TESTING)
    # set(TEST_SOURCES
    # )
    #
    # add_executable(macchiato_test ${TEST_SOURCES})
    # mct_target_cxx_standard(macchiato_test)
    #
    # target_link_libraries(macchiato_test PRIVATE
    #     macchiato
    #     Catch2::Catch2WithMain
    #     fmt::fmt
    # )
    #
    # catch_discover_tests(macchiato_test)
endif()
