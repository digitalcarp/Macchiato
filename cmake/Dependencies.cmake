# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

include(FetchContent)

macro(mct_setup_dependencies)
    # Catch2::Catch2 or Catch2::Catch2WithMain
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.8.1
    )

    # fmt::fmt
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG        11.1.4
    )

    # Don't use FetchContent_Declare after this
    FetchContent_MakeAvailable(Catch2 fmt)
endmacro()
