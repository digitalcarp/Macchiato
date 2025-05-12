# Populate .env with the following optional environment variables:
#
# DEV_PRESET    - dev config+build preset name
# STRICT_PRESET - dev-strict config+build preset name
# DEBUG_PRESET  - dev-debug config+build preset name
#
# TEST_PRESET - test preset name

set dotenv-load

dev_preset := env("DEV_PRESET", "dev")
strict_preset := env("STRICT_PRESET", "dev-strict")
debug_preset := env("DEBUG_PRESET", "dev-debug")
test_preset := env("TEST_PRESET", "test")

alias c := config
alias b := build
alias t := test
alias f := format

default:
    @just help

help:
    @just --list --unsorted

[group('dev')]
config:
    cmake --preset {{dev_preset}}

[group('dev')]
build:
    cmake --build --preset {{dev_preset}}

[group('strict')]
config-strict:
    cmake --preset {{strict_preset}}

[group('strict')]
build-strict:
    cmake --build --preset {{strict_preset}}

[group('debug')]
config-debug:
    cmake --preset {{debug_preset}}

[group('debug')]
build-debug:
    cmake --build --preset {{debug_preset}}

[group('test')]
test:
    ctest --preset {{test_preset}}

[group('lint')]
format:
    fd -e h -e cc . src -x clang-format --style=file -i {}

[group('lint')]
format-check:
    fd -e h -e cc . src -x clang-format --style=file --Werror -n {}

[group('espresso')]
build-espresso:
    git submodule init
    git submodule update
    cmake -S external/espresso -B build/espresso
    cmake --build build/espresso
    ctest --test-dir build/espresso

[group('espresso')]
man-espresso:
    man build/espresso/espresso.1
