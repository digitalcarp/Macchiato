// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

// This is a C++ port of xoshiro256++ which is provided to the public domain.
// The original copyright is at the end of the file.

#pragma once

#include "xoshiro/splitmix64.h"

#include <array>
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace xoshiro {

/**
 * xoshiro256++ 1.0 is an all-purpose, rock-solid generator.
 *
 * It has excellent (sub-ns) speed and a state (256 bits) that is large enough
 * for any parallel application. It passes all tests the original creators were
 * aware of.
 */
class Xoshiro256PlusPlus {
public:
    using state_type = std::array<std::uint64_t, 4>;

    /**
     * Initialize the generator with the given state.
     *
     * The state must not be everywhere zero.
     */
    explicit Xoshiro256PlusPlus(const state_type& state) : s(state)
    {
        bool is_zero = true;
        for (std::uint64_t word : s) {
            if (word != 0) {
                is_zero = false;
                break;
            }
        }

        if (is_zero) {
            throw std::invalid_argument("State must not be zero");
        }
    }

    /**
     * Initialize the generator using a 64-bit seed.
     *
     * The initial seeded state is guaranteed to be not everywhere zero.
     */
    explicit Xoshiro256PlusPlus(std::uint64_t seed)
    {
        SplitMix64 seeder{seed};
        std::size_t i = 0;
        while (i < s.size()) {
            std::uint64_t rng = seeder.next();
            if (rng != 0) {
                s[i] = rng;
                i++;
            }
        }
    }

    std::uint64_t operator()() { return next(); }

    std::uint64_t next()
    {
        const std::uint64_t result = std::rotl(s[0] + s[3], 23) + s[0];
        const std::uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;

        s[3] = std::rotl(s[3], 45);

        return result;
    }

private:
    state_type s;
};

} // namespace xoshiro

/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */
