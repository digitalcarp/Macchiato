// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

#pragma once

#include <bit>
#include <cstdint>

#ifndef MACCHIATO_NAMESPACE
#define MACCHIATO_NAMESPACE macchiato
#endif

namespace MACCHIATO_NAMESPACE {

// Pick 32-bit or 64-bit based on architecture
using WORD = std::uint_fast32_t;
using std::size_t;

constexpr size_t BITS_PER_BYTE = 8;
constexpr size_t BYTES_PER_WORD = sizeof(WORD);
constexpr size_t BITS_PER_WORD = BYTES_PER_WORD * BITS_PER_BYTE;
constexpr size_t MSB_POS = BITS_PER_WORD - 1;

static_assert(BITS_PER_WORD >= BITS_PER_BYTE);
static_assert(MSB_POS < BITS_PER_WORD);

constexpr WORD ZEROS = 0;
constexpr WORD ONES = ~ZEROS;

static_assert(std::popcount(ZEROS) == 0);
static_assert(std::popcount(ONES) == BITS_PER_WORD);

consteval WORD generate_disjoint_word()
{
    WORD disjoint = ZEROS;

    WORD byte = 0x55;
    while (byte != ZEROS) {
        disjoint |= byte;
        byte <<= BITS_PER_BYTE;
    }

    return disjoint;
}

constexpr WORD DISJOINT = generate_disjoint_word();
static_assert((DISJOINT | (DISJOINT << 1)) == ONES);

constexpr size_t mask_from_trailing_zeros(size_t value) { return value - 1; }

template <size_t MOD>
constexpr size_t binary_mod(size_t value)
    requires(std::popcount(MOD) == 1)
{
    return value & mask_from_trailing_zeros(MOD);
}

constexpr size_t calc_num_words_needed(size_t num_bits)
{
    size_t n = num_bits / BITS_PER_WORD;
    if (binary_mod<BITS_PER_WORD>(num_bits) != 0) n++;
    return n;
}

} // namespace MACCHIATO_NAMESPACE
