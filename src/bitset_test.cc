// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

#include "macchiato/bitset.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_range_equals.hpp"

using namespace macchiato;

using Catch::Matchers::RangeEquals;

TEST_CASE("Construction", "[bitset]")
{
    SECTION("Empty")
    {
        Bitset bitset;
        CHECK(bitset.num_bits() == 0);
        CHECK(bitset.num_words() == 0);
        CHECK(bitset.is_empty());
    }
    SECTION("Single word")
    {
        Bitset bitset(BITS_PER_WORD, ZEROS);
        CHECK(bitset.num_bits() == BITS_PER_WORD);
        CHECK(bitset.num_words() == 1);
        CHECK_FALSE(bitset.is_empty());
    }
    SECTION("Less than single word")
    {
        Bitset bitset(7, ONES);
        CHECK(bitset.num_bits() == 7);
        CHECK(bitset.num_words() == 1);
        CHECK_FALSE(bitset.is_empty());

        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector{0b111'1111}));
    }
    SECTION("Aligned multiple words")
    {
        size_t num_bits = BITS_PER_WORD * 3;
        Bitset bitset(num_bits, ONES);

        CHECK(bitset.num_bits() == num_bits);
        CHECK(bitset.num_words() == 3);
        CHECK_FALSE(bitset.is_empty());

        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0, 0}));
    }
    SECTION("Unaligned multiple words")
    {
        size_t num_bits = BITS_PER_WORD * 3 + 5;
        Bitset bitset(num_bits, ONES);

        CHECK(bitset.num_bits() == num_bits);
        CHECK(bitset.num_words() == 4);
        CHECK_FALSE(bitset.is_empty());

        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0, 0, 0}));
    }
}

TEST_CASE("Capacity", "[bitset]")
{
    Bitset bitset;
    CHECK(bitset.word_capacity() == 0);
    CHECK(bitset.bit_capacity() == 0);

    constexpr size_t num_bits = BITS_PER_WORD * 3 + 5;
    bitset.reserve(num_bits);
    CHECK(bitset.word_capacity() == 4);
    CHECK(bitset.bit_capacity() == 4 * BITS_PER_WORD);

    bitset.clear();
    bitset.shrink_to_fit();
    CHECK(bitset.word_capacity() == 0);
    CHECK(bitset.bit_capacity() == 0);
}

TEST_CASE("Size", "[bitset]")
{
    Bitset bitset;
    CHECK(bitset.num_words() == 0);
    CHECK(bitset.num_bits() == 0);

    bitset.push_msb(true);
    bitset.push_msb(false);
    bitset.push_msb(true);
    CHECK(bitset.num_words() == 1);
    CHECK(bitset.num_bits() == 3);
    CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{0b101}));

    bitset.extend_msb_with_word(ONES);
    CHECK(bitset.num_words() == 2);
    CHECK(bitset.num_bits() == BITS_PER_WORD + 3);
    CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES & ~0b010, 0b111}));

    bitset.pop_msb();
    CHECK(bitset.num_words() == 2);
    CHECK(bitset.num_bits() == BITS_PER_WORD + 2);
    CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES & ~0b010, 0b11}));

    bitset.clear();
    CHECK(bitset.num_words() == 0);
    CHECK(bitset.num_bits() == 0);
}

TEST_CASE("Aggregate", "[bitset]")
{
    Bitset bitset;
    CHECK_FALSE(bitset.all());
    CHECK_FALSE(bitset.any());
    CHECK(bitset.none());
    CHECK(bitset.count() == 0);

    SECTION("Single word")
    {
        bitset.push_msb(true);
        CHECK(bitset.all());
        CHECK(bitset.any());
        CHECK_FALSE(bitset.none());
        CHECK(bitset.count() == 1);

        bitset.push_msb(false);
        CHECK_FALSE(bitset.all());
        CHECK(bitset.any());
        CHECK_FALSE(bitset.none());
        CHECK(bitset.count() == 1);

        bitset.push_msb(true);
        CHECK(bitset.count() == 2);
    }
    SECTION("Multi word all")
    {
        bitset.extend_msb_with_word(ONES);
        bitset.extend_msb_with_word(ONES);
        CHECK(bitset.all());
        CHECK(bitset.any());
        CHECK_FALSE(bitset.none());
        CHECK(bitset.count() == 2 * BITS_PER_WORD);

        bitset.push_msb(true);
        bitset.push_msb(true);
        bitset.push_msb(true);
        CHECK(bitset.all());
        CHECK(bitset.any());
        CHECK(bitset.count() == 2 * BITS_PER_WORD + 3);

        bitset.push_msb(false);
        CHECK_FALSE(bitset.all());
        CHECK(bitset.any());
        CHECK(bitset.count() == 2 * BITS_PER_WORD + 3);
    }
    SECTION("Multi word any")
    {
        bitset.extend_msb_with_word(ONES);
        CHECK(bitset.all());
        CHECK(bitset.any());
        CHECK_FALSE(bitset.none());
        CHECK(bitset.count() == BITS_PER_WORD);

        bitset.extend_msb_with_word(ONES & 0xdead);
        CHECK_FALSE(bitset.all());
        CHECK(bitset.any());
        CHECK(bitset.count() == BITS_PER_WORD + 11);

        bitset.extend_msb_with_word(ONES);
        CHECK(bitset.any());
        CHECK(bitset.count() == 2 * BITS_PER_WORD + 11);
    }
    SECTION("Multi word none")
    {
        bitset.extend_msb_with_word(ZEROS);
        bitset.extend_msb_with_word(ZEROS);
        CHECK_FALSE(bitset.all());
        CHECK_FALSE(bitset.any());
        CHECK(bitset.none());
        CHECK(bitset.count() == 0);
    }
}

TEST_CASE("Single bit", "[bitset]")
{
    const auto num_bits = BITS_PER_WORD + 4;
    Bitset bitset(num_bits, ONES);
    REQUIRE(bitset.num_bits() == num_bits);

    SECTION("OOB")
    {
        CHECK_THROWS(bitset.test(num_bits));
        CHECK_THROWS(bitset.set(num_bits));
        CHECK_THROWS(bitset.reset(num_bits));
        CHECK_THROWS(bitset.flip(num_bits));
    }
    SECTION("Operations")
    {
        REQUIRE_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0b0000}));

        bitset.reset(MSB_POS);
        CHECK_FALSE(bitset.test(MSB_POS));
        CHECK_THAT(bitset.raw_data(),
                   RangeEquals(std::vector<size_t>{ONES & ~(WORD{1} << MSB_POS), 0}));

        bitset.set(MSB_POS);
        CHECK(bitset.test(MSB_POS));
        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0}));

        bitset.flip(0);
        CHECK_FALSE(bitset.test(0));
        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES - 1, 0}));

        bitset.flip(0);
        CHECK(bitset.test(1));
        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0}));

        bitset.set(BITS_PER_WORD + 1);
        CHECK(bitset.test(BITS_PER_WORD + 1));
        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0b0010}));

        bitset.reset(BITS_PER_WORD + 1);
        CHECK_FALSE(bitset.test(BITS_PER_WORD + 1));
        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0b0000}));

        bitset.flip(BITS_PER_WORD + 2);
        CHECK(bitset.test(BITS_PER_WORD + 2));
        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0b0100}));

        bitset.flip(BITS_PER_WORD + 2);
        CHECK_FALSE(bitset.test(BITS_PER_WORD + 2));
        CHECK_THAT(bitset.raw_data(), RangeEquals(std::vector<size_t>{ONES, 0b0000}));
    }
}

TEST_CASE("Modify range", "[bitset]")
{
    Bitset bitset;
    bitset.extend_msb_with_word(ZEROS);
    bitset.extend_msb_with_word(ZEROS);
    bitset.push_msb(false);
    bitset.push_msb(false);
    REQUIRE(bitset.none());

    SECTION("All")
    {
        bitset.set();
        CHECK(bitset.all());
        bitset.reset();
        CHECK(bitset.none());
        bitset.flip();
        CHECK(bitset.all());
        bitset.flip();
        CHECK(bitset.none());
    }
    SECTION("OOB")
    {
        CHECK_THROWS(bitset.set(bitset.num_bits(), 1));
        CHECK_THROWS(bitset.reset(bitset.num_bits(), 1));
        CHECK_THROWS(bitset.flip(bitset.num_bits(), 1));
    }
    SECTION("Full")
    {
        bitset.set(0, bitset.num_bits());
        CHECK(bitset.all());
        bitset.reset(0, bitset.num_bits());
        CHECK(bitset.none());
        bitset.flip(0, bitset.num_bits());
        CHECK(bitset.all());
        bitset.flip(0, bitset.num_bits());
        CHECK(bitset.none());
    }
    SECTION("Safe bounds")
    {
        const size_t OOB = bitset.num_bits() + 1;
        bitset.set(0, OOB);
        CHECK(bitset.all());
        bitset.reset(0, OOB);
        CHECK(bitset.none());
        bitset.flip(0, OOB);
        CHECK(bitset.all());
        bitset.flip(0, OOB);
        CHECK(bitset.none());
    }
    SECTION("Length 0")
    {
        bitset.push_msb(true);
        bitset.push_msb(false);
        auto expected = std::vector<size_t>{0, 0, 0b0100};

        bitset.set(0, 0);
        CHECK_THAT(bitset.raw_data(), RangeEquals(expected));
        bitset.reset(0, 0);
        CHECK_THAT(bitset.raw_data(), RangeEquals(expected));
        bitset.flip(0, 0);
        CHECK_THAT(bitset.raw_data(), RangeEquals(expected));
    }
    SECTION("Length 1")
    {
        bitset.set(0, 1);
        CHECK(bitset.test(0));
        CHECK(bitset.count() == 1);
        bitset.reset(0, 1);
        CHECK_FALSE(bitset.test(0));
        CHECK(bitset.count() == 0);
        bitset.flip(0, 1);
        CHECK(bitset.test(0));
        CHECK(bitset.count() == 1);
        bitset.flip(0, 1);
        CHECK_FALSE(bitset.test(0));
        CHECK(bitset.count() == 0);

        constexpr size_t OFFSET = BITS_PER_WORD + (BITS_PER_WORD - 3);
        bitset.set(OFFSET, 1);
        CHECK(bitset.test(OFFSET));
        CHECK(bitset.count() == 1);
        bitset.reset(OFFSET, 1);
        CHECK_FALSE(bitset.test(OFFSET));
        CHECK(bitset.count() == 0);
        bitset.flip(OFFSET, 1);
        CHECK(bitset.test(OFFSET));
        CHECK(bitset.count() == 1);
        bitset.flip(OFFSET, 1);
        CHECK_FALSE(bitset.test(OFFSET));
        CHECK(bitset.count() == 0);
    }
    SECTION("Same word")
    {
        REQUIRE(BITS_PER_WORD >= 64);

        bitset.set(BITS_PER_WORD + 3, 51);
        CHECK(bitset.count() == 51);
        CHECK(bitset.word_at(1) == 0x003f'ffff'ffff'fff8ULL);

        bitset.reset(BITS_PER_WORD + 4, 16);
        CHECK(bitset.count() == 35);
        CHECK(bitset.word_at(1) == 0x003f'ffff'fff0'0008ULL);

        bitset.flip(BITS_PER_WORD + 8, 8);
        CHECK(bitset.count() == 43);
        CHECK(bitset.word_at(1) == 0x003f'ffff'fff0'ff08ULL);

        bitset.flip(BITS_PER_WORD + 40, 6);
        CHECK(bitset.count() == 37);
        CHECK(bitset.word_at(1) == 0x003f'c0ff'fff0'ff08ULL);
    }
    SECTION("Across words")
    {
        bitset.set(4, BITS_PER_WORD);
        CHECK_THAT(bitset.raw_data(),
                   RangeEquals(std::vector<size_t>{ONES << 4, 0xF, 0}));

        bitset.set();
        REQUIRE(bitset.all());
        bitset.reset(4, BITS_PER_WORD);
        CHECK_THAT(bitset.raw_data(),
                   RangeEquals(std::vector<size_t>{~(ONES << 4), ONES << 4, 0b11}));

        bitset.reset();
        REQUIRE(bitset.none());
        bitset.flip(8, BITS_PER_WORD + 3);
        CHECK_THAT(bitset.raw_data(),
                   RangeEquals(std::vector<size_t>{ONES << 8, ~(ONES << 11), 0}));

        bitset.set();
        REQUIRE(bitset.all());
        bitset.flip(1, 2 * BITS_PER_WORD);
        CHECK_THAT(bitset.raw_data(),
                   RangeEquals(std::vector<size_t>{~(ONES << 1), ZEROS, 0b10}));
    }
}

TEST_CASE("Basic mixed width bit operations", "[bitset]")
{
    using MixedBitset = Bitset<MixedWidthMode::UNSIGNED_PROMOTION>;

    SECTION("AND")
    {
        MixedBitset lhs;
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(DISJOINT);
        MixedBitset rhs;
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ONES);
        // Larger by one bit
        rhs.push_msb(true);

        MixedBitset result;
        std::vector<size_t> expected{ZEROS, ZEROS, ZEROS, ONES, DISJOINT, ZEROS};

        result = lhs;
        result &= rhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));

        result = rhs;
        result &= lhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));

        result = lhs & rhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));
        result = rhs & lhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));
    }
    SECTION("OR")
    {
        MixedBitset lhs;
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(DISJOINT);
        lhs.extend_msb_with_word(DISJOINT);
        MixedBitset rhs;
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(DISJOINT << 1);
        rhs.push_msb(false);
        rhs.push_msb(true);

        MixedBitset result;
        std::vector<size_t> expected{ZEROS, ONES, ONES, ONES, DISJOINT, ONES, 0b10};

        result = lhs;
        result |= rhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));

        result = rhs;
        result |= lhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));

        result = lhs | rhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));
        result = rhs | lhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));
    }
    SECTION("XOR")
    {
        MixedBitset lhs;
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(DISJOINT);
        lhs.extend_msb_with_word(DISJOINT);
        MixedBitset rhs;
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(DISJOINT);
        rhs.extend_msb_with_word(ZEROS);
        rhs.push_msb(false);
        rhs.push_msb(true);

        MixedBitset result;
        std::vector<size_t> expected{ZEROS, ONES, ONES, ZEROS, ZEROS, DISJOINT, 0b10};

        result = lhs;
        result ^= rhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));

        result = rhs;
        result ^= lhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));

        result = lhs ^ rhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));
        result = rhs ^ lhs;
        CHECK_THAT(result.raw_data(), RangeEquals(expected));
    }
    SECTION("NOT")
    {
        MixedBitset bitset;
        bitset.extend_msb_with_word(ZEROS);
        bitset.extend_msb_with_word(DISJOINT);
        bitset.extend_msb_with_word(ONES);

        std::vector<size_t> expected{ONES, ~DISJOINT, ZEROS};

        SECTION("alloc")
        {
            auto negated = ~bitset;
            CHECK_THAT(negated.raw_data(), RangeEquals(expected));
        }
        SECTION("inplace")
        {
            bitset.inplace_not();
            CHECK_THAT(bitset.raw_data(), RangeEquals(expected));
        }
    }
    SECTION("NAND")
    {
        MixedBitset lhs;
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(DISJOINT);
        MixedBitset rhs;
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ONES);
        // Larger by one bit
        rhs.push_msb(true);

        std::vector<size_t> expected{ONES, ONES, ONES, ZEROS, (DISJOINT << 1), 0b1};

        SECTION("lhs NAND rhs")
        {
            MixedBitset result = lhs;
            result.nand(rhs);
            CHECK_THAT(result.raw_data(), RangeEquals(expected));
        }
        SECTION("rhs NAND lhs")
        {
            MixedBitset result = rhs;
            result.nand(lhs);
            CHECK_THAT(result.raw_data(), RangeEquals(expected));
        }
    }
    SECTION("NOR")
    {
        MixedBitset lhs;
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(DISJOINT);
        lhs.extend_msb_with_word(DISJOINT);
        MixedBitset rhs;
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(DISJOINT << 1);
        rhs.push_msb(false);
        rhs.push_msb(true);

        std::vector<size_t> expected{ONES,  ZEROS, ZEROS, ZEROS, (DISJOINT << 1),
                                     ZEROS, 0b01};

        SECTION("lhs NOR rhs")
        {
            MixedBitset result = lhs;
            result.nor(rhs);
            CHECK_THAT(result.raw_data(), RangeEquals(expected));
        }
        SECTION("rhs NOR lhs")
        {
            MixedBitset result = rhs;
            result.nor(lhs);
            CHECK_THAT(result.raw_data(), RangeEquals(expected));
        }
    }
    SECTION("XNOR")
    {
        MixedBitset lhs;
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(ZEROS);
        lhs.extend_msb_with_word(ONES);
        lhs.extend_msb_with_word(DISJOINT);
        lhs.extend_msb_with_word(DISJOINT);
        MixedBitset rhs;
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ONES);
        rhs.extend_msb_with_word(ZEROS);
        rhs.extend_msb_with_word(DISJOINT << 1);
        rhs.push_msb(false);
        rhs.push_msb(true);

        std::vector<size_t> expected{ONES,  ZEROS, ZEROS, ONES, (DISJOINT << 1),
                                     ZEROS, 0b01};

        SECTION("lhs XNOR rhs")
        {
            MixedBitset result = lhs;
            result.xnor(rhs);
            CHECK_THAT(result.raw_data(), RangeEquals(expected));
        }
        SECTION("rhs XNOR lhs")
        {
            MixedBitset result = rhs;
            result.xnor(lhs);
            CHECK_THAT(result.raw_data(), RangeEquals(expected));
        }
    }
}

TEST_CASE("Equals", "[bitset]")
{
    SECTION("Same width")
    {
        Bitset a(BITS_PER_WORD, ZEROS);
        a.push_msb(false);
        Bitset b(BITS_PER_WORD, ONES);
        b.push_msb(true);
        Bitset c(BITS_PER_WORD, DISJOINT);
        c.push_msb(true);
        Bitset d(BITS_PER_WORD, ZEROS);
        d.push_msb(true);

        Bitset a1(BITS_PER_WORD, ZEROS);
        a1.push_msb(false);
        Bitset b1(BITS_PER_WORD, ONES);
        b1.push_msb(true);
        Bitset c1(BITS_PER_WORD, DISJOINT);
        c1.push_msb(true);

        CHECK(a == a1);
        CHECK(b == b1);
        CHECK(c == c1);

        CHECK(a != b);
        CHECK(b != c);
        CHECK(c != a);
        CHECK(d != a);

        Bitset empty;
        Bitset empty1;
        CHECK(empty == empty1);
    }
    SECTION("Mixed width")
    {
        using MixedBitset = Bitset<MixedWidthMode::UNSIGNED_PROMOTION>;
        MixedBitset a(BITS_PER_WORD, ZEROS);
        a.push_msb(false);
        a.push_msb(false);
        MixedBitset b(BITS_PER_WORD, ONES);
        b.push_msb(true);
        b.push_msb(true);
        MixedBitset c(BITS_PER_WORD, DISJOINT);
        c.push_msb(true);
        c.push_msb(false);
        MixedBitset d(BITS_PER_WORD, ZEROS);
        d.push_msb(true);
        d.push_msb(true);
        MixedBitset e(BITS_PER_WORD, ZEROS);
        e.extend_msb_with_word(ZEROS);
        e.extend_msb_with_word(ZEROS);
        e.push_msb(true);

        MixedBitset a1(BITS_PER_WORD, ZEROS);
        a1.push_msb(false);
        a1.push_msb(false);
        MixedBitset b1(BITS_PER_WORD, ONES);
        b1.push_msb(true);
        b1.push_msb(true);
        MixedBitset c1(BITS_PER_WORD, DISJOINT);
        c1.push_msb(true);
        c1.push_msb(false);

        CHECK(a == a1);
        CHECK(b == b1);
        CHECK(c == c1);

        CHECK(a != b);
        CHECK(b != c);
        CHECK(c != a);
        CHECK(d != a);
        CHECK(e != a);

        MixedBitset empty;
        MixedBitset empty1;
        CHECK(empty == empty1);
        CHECK(empty != a);
        CHECK(empty != b);
        CHECK(empty != c);
        CHECK(empty != d);
        CHECK(empty != e);
    }
}

TEST_CASE("Mixed width operations with promotion disabled", "[bitset]")
{
    Bitset lhs(2, ZEROS);
    Bitset rhs(1, ZEROS);

    CHECK_THROWS(lhs == rhs);
    CHECK_THROWS(lhs != rhs);

    CHECK_THROWS(lhs &= rhs);
    CHECK_THROWS(lhs & rhs);
    CHECK_THROWS(lhs |= rhs);
    CHECK_THROWS(lhs | rhs);
    CHECK_THROWS(lhs ^= rhs);
    CHECK_THROWS(lhs ^ rhs);

    CHECK_THROWS(lhs.nand(rhs));
    CHECK_THROWS(lhs.nor(rhs));
    CHECK_THROWS(lhs.xnor(rhs));
}
