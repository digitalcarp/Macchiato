// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Copyright 2025 Daniel Gao

#pragma once

#include "macchiato/base.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <format>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#ifndef MACCHIATO_NAMESPACE
#error "MACCHIATO_NAMESPACE is not defined"
#endif

namespace MACCHIATO_NAMESPACE {

template <class T> concept FULL_WORD_OP = requires(T op, WORD word) {
    {
        op(word)
    } -> std::same_as<WORD>;
};

template <class T> concept PARTIAL_WORD_OP =
        requires(T op, WORD word, size_t start, size_t stop_incl) {
            {
                op(word, start, stop_incl)
            } -> std::same_as<WORD>;
        };

template <class T> concept BOOLEAN_OP = requires(T op, WORD lhs, WORD rhs) {
    {
        op(lhs, rhs)
    } -> std::same_as<WORD>;
};

enum class MixedWidthMode { SAME_SIZE_ONLY, UNSIGNED_PROMOTION };

// Terminology:
// - MSB := Most significant bit
// - LSB := Least significant bit
// - MSW := Most significant word (holding the N MSBs of the bitset)
// - LSW := Least significant word (holding the N LSBs of the bitset)
//
// Invariants:
// - The cube/bitset is stored as a contiguous array of words.
//
// - The words are in least significant to most significant order.
//   (i.e. Word 0 holds bits [0, 63], word 1 holds bits [64, 127], etc...)
//
// - If the number of bits is not aligned to a word, the unused MSBs of the MSW
//   are reset to 0.
template <MixedWidthMode WidthMode = MixedWidthMode::SAME_SIZE_ONLY,
          class Allocator = std::allocator<WORD>>
class Bitset {
public:
    using buffer_type = std::vector<WORD, Allocator>;
    using allocator_type = buffer_type::allocator_type;
    using size_type = buffer_type::size_type;

    static_assert(std::is_same_v<size_type, size_t>);

    Bitset() noexcept : m_num_bits(0) {}
    Bitset(const Allocator& alloc) noexcept : m_data(alloc), m_num_bits(0) {}

    explicit Bitset(size_type num_bits, WORD value = ZEROS,
                    const Allocator& alloc = Allocator());

    Bitset(const Bitset&) = default;
    Bitset& operator=(const Bitset&) = default;
    Bitset(Bitset&&) = default;
    Bitset& operator=(Bitset&&) = default;

    ~Bitset() = default;

    void swap(Bitset& other) noexcept
    {
        std::swap(m_data, other.m_data);
        std::swap(m_num_bits, other.m_num_bits);
    }

    allocator_type get_allocator() const noexcept { return m_data.get_allocator(); }

    size_type num_words() const { return m_data.size(); }
    size_type num_bits() const { return m_num_bits; }
    size_type word_capacity() const { return m_data.capacity(); }
    size_type bit_capacity() const { return word_capacity() * BITS_PER_WORD; }
    bool is_empty() const { return num_bits() == 0; }

    bool is_narrower_than(const Bitset& other) const;

    void reserve(size_type num_bits) { m_data.reserve(calc_num_words_needed(num_bits)); }
    void clear() noexcept;
    void shrink_to_fit();

    void resize(size_type num_bits, bool value = false);
    void push_msb(bool value);
    void pop_msb() noexcept;
    void extend_msb_with_word(WORD word);

    bool all() const;
    bool any() const;
    bool none() const;
    size_type count() const;

    bool test(size_type n) const;

    Bitset& set();
    Bitset& set(size_type n);
    Bitset& set(size_type n, size_type len);

    Bitset& reset();
    Bitset& reset(size_type n);
    Bitset& reset(size_type n, size_type len);

    Bitset& flip();
    Bitset& flip(size_type n);
    Bitset& flip(size_type n, size_type len);

    constexpr size_type word_index(size_type bit_pos) const;
    constexpr size_type bit_offset(size_type bit_pos) const;
    constexpr WORD one_hot_mask(size_type bit_pos) const;

    Bitset& operator&=(const Bitset& other);
    Bitset& operator|=(const Bitset& other);
    Bitset& operator^=(const Bitset& other);
    // Since the default behaviour of ~ is to create a copy, provide inplace
    // functions for common negated operators.
    Bitset& nand(const Bitset& other);
    Bitset& nor(const Bitset& other);
    Bitset& xnor(const Bitset& other);

    Bitset operator~() const;
    Bitset& inplace_not() noexcept;

    // NE does not always require checking all bits and can return early.
    bool operator!=(const Bitset& other) const;
    bool operator==(const Bitset& other) const { return !(*this != other); }

    Bitset& inplace_set_difference(const Bitset& other) noexcept;

    // WARNING:
    // This group of functions if for internal testing only.
    // If you use these functions, there is no guarantee that the result
    // remains consistent between versions.
    WORD word_at(size_type index) const { return m_data.at(index); }
    const std::vector<WORD>& raw_data() const { return m_data; }

private:
    void init(size_type num_bits, WORD value);

    size_type msw_bit_alignment() const;
    WORD generate_used_bits_in_msw_mask() const;
    void zero_unused_bits_in_msw();

    void modify_range(size_type bit_pos, size_type len, FULL_WORD_OP auto&& full,
                      PARTIAL_WORD_OP auto&& partial);

    void apply_boolean_op(const Bitset& other, BOOLEAN_OP auto&& op);

    buffer_type m_data;
    size_type m_num_bits;
};

template <MixedWidthMode WidthMode, class Allocator>
void swap(Bitset<WidthMode, Allocator>& lhs, Bitset<WidthMode, Allocator>& rhs) noexcept
{
    lhs.swap(rhs);
}

template <MixedWidthMode WidthMode, class Allocator>
Bitset<WidthMode, Allocator> operator&(const Bitset<WidthMode, Allocator>& lhs,
                                       const Bitset<WidthMode, Allocator>& rhs)
{
    const auto& wider = (lhs.num_words() > rhs.num_words()) ? lhs : rhs;
    const auto& narrower = (lhs.num_words() > rhs.num_words()) ? rhs : lhs;

    Bitset result(wider);
    result &= narrower;
    return result;
}

template <MixedWidthMode WidthMode, class Allocator>
Bitset<WidthMode, Allocator> operator|(const Bitset<WidthMode, Allocator>& lhs,
                                       const Bitset<WidthMode, Allocator>& rhs)
{
    const auto& wider = (lhs.num_words() > rhs.num_words()) ? lhs : rhs;
    const auto& narrower = (lhs.num_words() > rhs.num_words()) ? rhs : lhs;

    Bitset result(wider);
    result |= narrower;
    return result;
}

template <MixedWidthMode WidthMode, class Allocator>
Bitset<WidthMode, Allocator> operator^(const Bitset<WidthMode, Allocator>& lhs,
                                       const Bitset<WidthMode, Allocator>& rhs)
{
    const auto& wider = (lhs.num_words() > rhs.num_words()) ? lhs : rhs;
    const auto& narrower = (lhs.num_words() > rhs.num_words()) ? rhs : lhs;

    Bitset result(wider);
    result ^= narrower;
    return result;
}

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

template <MixedWidthMode WidthMode, class Allocator>
inline Bitset<WidthMode, Allocator>::Bitset(size_type num_bits, WORD value,
                                            const Allocator& alloc)
        : m_data(alloc), m_num_bits(num_bits)
{
    init(num_bits, value);
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::init(size_type num_bits, WORD value)
{
    assert(m_data.size() == 0);

    m_num_bits = num_bits;
    size_type num_words = calc_num_words_needed(num_bits);
    if (num_words == 0) return;

    m_data.resize(num_words, ZEROS);
    m_data[0] = value;
    zero_unused_bits_in_msw();
}

template <MixedWidthMode WidthMode, class Allocator>
inline bool Bitset<WidthMode, Allocator>::is_narrower_than(const Bitset& other) const
{
    return m_num_bits < other.m_num_bits;
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::clear() noexcept
{
    m_data.clear();
    m_num_bits = 0;
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::shrink_to_fit()
{
    if (m_data.size() < m_data.capacity()) {
        // Copy-and-swap idiom
        buffer_type(m_data).swap(m_data);
    }
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::resize(size_type num_bits, bool value)
{
    size_type old_num_words = num_words();
    size_type new_num_words = calc_num_words_needed(num_bits);

    if (new_num_words != old_num_words) {
        WORD fill_word = value ? ONES : ZEROS;
        m_data.resize(new_num_words, fill_word);
    }

    // If expanded, the new words have the correct value (except for masking
    // the MSW). However, if the unmodified MSB wasn't aligned to the MSB of a
    // word, we need to update the old MSW's unused bits.
    //
    // Since unused MSBs are always 0, this operation only needs to be done if
    // we are filling with 1's.
    //
    // If shrunk, only need to mask most significant word.
    if (value && num_bits >= m_num_bits) {
        size_type align = msw_bit_alignment();
        if (align > 0) {
            m_data.at(old_num_words - 1) |= (ONES << align);
        }
    }

    m_num_bits = num_bits;
    zero_unused_bits_in_msw();
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::push_msb(bool value)
{
    resize(m_num_bits + 1, value);
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::pop_msb() noexcept
{
    assert(m_num_bits > 0);

    size_type old_num_words = num_words();

    --m_num_bits;
    size_type new_num_words = calc_num_words_needed(m_num_bits);

    if (new_num_words < old_num_words) {
        m_data.pop_back();
    } else {
        zero_unused_bits_in_msw();
    }
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::extend_msb_with_word(WORD word)
{
    size_type align = msw_bit_alignment();
    if (align == 0) {
        m_data.push_back(word);
    } else {
        assert(m_data.size() > 0);
        m_data.back() |= (word << align);
        m_data.push_back(word >> (BITS_PER_WORD - align));
    }

    m_num_bits += BITS_PER_WORD;
}

template <MixedWidthMode WidthMode, class Allocator>
inline bool Bitset<WidthMode, Allocator>::all() const
{
    if (is_empty()) return false;

    auto it = m_data.crbegin();
    if (*it != generate_used_bits_in_msw_mask()) return false;
    ++it;

    while (it != m_data.crend()) {
        if (*it != ONES) return false;
        ++it;
    }
    return true;
}

template <MixedWidthMode WidthMode, class Allocator>
inline bool Bitset<WidthMode, Allocator>::any() const
{
    return std::ranges::any_of(m_data, [](WORD word) { return word != ZEROS; });
}

template <MixedWidthMode WidthMode, class Allocator>
inline bool Bitset<WidthMode, Allocator>::none() const
{
    return std::ranges::none_of(m_data, [](WORD word) { return word != ZEROS; });
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::count() const -> size_type
{
    size_type num = 0;
    for (const auto& word : m_data) {
        num += std::popcount(word);
    }
    return num;
}

template <MixedWidthMode WidthMode, class Allocator>
inline constexpr auto Bitset<WidthMode, Allocator>::word_index(size_type bit_pos) const
        -> size_type
{
    return bit_pos / BITS_PER_WORD;
}

template <MixedWidthMode WidthMode, class Allocator>
inline constexpr auto Bitset<WidthMode, Allocator>::bit_offset(size_type bit_pos) const
        -> size_type
{
    return binary_mod<BITS_PER_WORD>(bit_pos);
}

template <MixedWidthMode WidthMode, class Allocator>
inline constexpr WORD Bitset<WidthMode, Allocator>::one_hot_mask(size_type bit_pos) const
{
    return WORD{1} << bit_offset(bit_pos);
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::set() -> Bitset&
{
    std::ranges::fill(m_data, ONES);
    zero_unused_bits_in_msw();
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::reset() -> Bitset&
{
    std::ranges::fill(m_data, ZEROS);
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::flip() -> Bitset&
{
    inplace_not();
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline bool Bitset<WidthMode, Allocator>::test(size_type n) const
{
    if (n >= m_num_bits) [[unlikely]] {
        throw std::out_of_range(std::format("Out of bounds access at {} in range [0, {})",
                                            n, m_num_bits));
    }

    WORD mask = one_hot_mask(bit_offset(n));
    return (m_data[word_index(n)] & mask) != ZEROS;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::set(size_type n) -> Bitset&
{
    if (n >= m_num_bits) [[unlikely]] {
        throw std::out_of_range(std::format("Out of bounds access at {} in range [0, {})",
                                            n, m_num_bits));
    }

    WORD mask = one_hot_mask(bit_offset(n));
    m_data[word_index(n)] |= mask;

    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::set(size_type n, size_type len) -> Bitset&
{
    auto full_op = [](WORD /*word*/) { return ONES; };
    auto partial_op = [](WORD word, size_type start, size_type stop_incl)
    {
        // Zero out LSBs
        WORD mask = (ONES >> start) << start;
        // Zero out MSBs
        mask = (mask << (MSB_POS - stop_incl)) >> (MSB_POS - stop_incl);
        return word | mask;
    };

    modify_range(n, len, full_op, partial_op);
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::reset(size_type n) -> Bitset&
{
    if (n >= m_num_bits) [[unlikely]] {
        throw std::out_of_range(std::format("Out of bounds access at {} in range [0, {})",
                                            n, m_num_bits));
    }

    WORD mask = one_hot_mask(bit_offset(n));
    mask = ~mask;
    m_data[word_index(n)] &= mask;

    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::reset(size_type n, size_type len) -> Bitset&
{
    auto full_op = [](WORD /*word*/) { return ZEROS; };
    auto partial_op = [](WORD word, size_type start, size_type stop_incl)
    {
        // Zero out LSBs
        WORD mask = (ONES >> start) << start;
        // Zero out MSBs
        mask = (mask << (MSB_POS - stop_incl)) >> (MSB_POS - stop_incl);
        return word & ~mask; // Negate the mask
    };

    modify_range(n, len, full_op, partial_op);
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::flip(size_type n) -> Bitset&
{
    if (n >= m_num_bits) {
        throw std::out_of_range(std::format("Out of bounds access at {} in range [0, {})",
                                            n, m_num_bits));
    }

    WORD mask = one_hot_mask(bit_offset(n));
    m_data[word_index(n)] ^= mask;

    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::flip(size_type n, size_type len) -> Bitset&
{
    auto full_op = [](WORD word) { return word ^ ONES; };
    auto partial_op = [](WORD word, size_type start, size_type stop_incl)
    {
        // Zero out LSBs
        WORD mask = (ONES >> start) << start;
        // Zero out MSBs
        mask = (mask << (MSB_POS - stop_incl)) >> (MSB_POS - stop_incl);
        return word ^ mask;
    };

    modify_range(n, len, full_op, partial_op);
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::operator&=(const Bitset& other) -> Bitset&
{
    apply_boolean_op(other, [](WORD lhs, WORD rhs) { return lhs & rhs; });
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::operator|=(const Bitset& other) -> Bitset&
{
    apply_boolean_op(other, [](WORD lhs, WORD rhs) { return lhs | rhs; });
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::operator^=(const Bitset& other) -> Bitset&
{
    apply_boolean_op(other, [](WORD lhs, WORD rhs) { return lhs ^ rhs; });
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::nand(const Bitset& other) -> Bitset&
{
    apply_boolean_op(other, [](WORD lhs, WORD rhs) { return ~(lhs & rhs); });
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::nor(const Bitset& other) -> Bitset&
{
    apply_boolean_op(other, [](WORD lhs, WORD rhs) { return ~(lhs | rhs); });
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::xnor(const Bitset& other) -> Bitset&
{
    apply_boolean_op(other, [](WORD lhs, WORD rhs) { return ~(lhs ^ rhs); });
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::operator~() const -> Bitset
{
    Bitset negated(get_allocator());
    negated.reserve(num_words());

    for (const auto& word : m_data) {
        negated.extend_msb_with_word(~word);
    }
    negated.zero_unused_bits_in_msw();

    return negated;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::inplace_not() noexcept -> Bitset&
{
    for (auto& word : m_data) {
        word = ~word;
    }
    zero_unused_bits_in_msw();
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline bool Bitset<WidthMode, Allocator>::operator!=(const Bitset& other) const
{
    if constexpr (WidthMode == MixedWidthMode::UNSIGNED_PROMOTION) {
        if (is_empty() != other.is_empty()) return true;

        const auto& narrow = is_narrower_than(other) ? *this : other;
        const auto& wide = is_narrower_than(other) ? other : *this;

        size_type i = 0;
        while (i < narrow.num_words()) {
            if (narrow.m_data[i] != wide.m_data[i]) return true;
            i++;
        }

        while (i < wide.num_words()) {
            if (wide.m_data[i] != ZEROS) return true;
            i++;
        }

        return false;
    } else {
        if (num_bits() != other.num_bits()) {
            throw std::invalid_argument(
                    std::format("Attempted to compare operands of width {} and {}",
                                num_bits(), other.num_bits()));
        }

        for (size_type i = 0; i < num_words(); i++) {
            if (m_data[i] != other.m_data[i]) return true;
        }
        return false;
    }
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto
Bitset<WidthMode, Allocator>::inplace_set_difference(const Bitset& other) noexcept
        -> Bitset&
{
    size_type end = std::min(num_words(), other.num_words());
    for (size_type i = 0; i < end; i++) {
        m_data[i] &= ~other.m_data[i];
    }
    return *this;
}

template <MixedWidthMode WidthMode, class Allocator>
inline auto Bitset<WidthMode, Allocator>::msw_bit_alignment() const -> size_type
{
    return binary_mod<BITS_PER_WORD>(m_num_bits);
}

template <MixedWidthMode WidthMode, class Allocator>
inline WORD Bitset<WidthMode, Allocator>::generate_used_bits_in_msw_mask() const
{
    size_type align = msw_bit_alignment();
    // Compiler should be able to make this branchless.
    if (align == 0) {
        // align == 0 means the MSW is fully used.
        return ONES;
    } else {
        return ~(ONES << align);
    }
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::zero_unused_bits_in_msw()
{
    if (is_empty()) [[unlikely]]
        return;
    m_data.back() &= generate_used_bits_in_msw_mask();
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::modify_range(size_type bit_pos, size_type len,
                                                       FULL_WORD_OP auto&& full,
                                                       PARTIAL_WORD_OP auto&& partial)
{
    if (bit_pos >= m_num_bits) [[unlikely]] {
        throw std::out_of_range(std::format("Out of bounds access at {} in range [0, {})",
                                            bit_pos, m_num_bits));
    }
    if (len == 0) return;

    const size_type start_word_index = word_index(bit_pos);
    const size_type start_bit_offset = bit_offset(bit_pos);

    // Indices guaranteed not out of bounds
    const size_type stop_pos = std::min(m_num_bits, bit_pos + len) - 1;
    const size_type stop_word_index = word_index(stop_pos);
    const size_type stop_bit_offset = bit_offset(stop_pos);

    assert(bit_pos <= stop_pos);
    assert(stop_pos < m_num_bits);
    assert(stop_word_index < m_data.size());
    assert(stop_bit_offset < BITS_PER_WORD);

    if (start_word_index == stop_word_index) {
        // Modifications applied only to one word
        assert(start_bit_offset <= stop_bit_offset);

        auto& word = m_data[start_word_index];
        word = partial(word, start_bit_offset, stop_bit_offset);
    } else {
        // Modifications applied to multiple words.
        // The LSW will have MSBs modified. The MSW will have LSBs modified.
        // In between blocks will be fully modified.

        // Start word is fully modified if start bit is the LSB.
        const size_type start_full_word_offset = (start_bit_offset == 0) ? 0 : 1;
        // Stop word is fully modified if stop bit is the MSB.
        // Since stop_pos < m_num_bits, there's no need to worry about the
        // unused bits of the MSW if it is not word aligned.
        const size_type stop_full_word_offset = (stop_bit_offset == MSB_POS) ? 0 : 1;

        if (start_full_word_offset != 0) {
            // Modify start bit to MSB
            auto& word = m_data[start_word_index];
            word = partial(word, start_bit_offset, MSB_POS);
        }
        if (stop_full_word_offset != 0) {
            // Modify LSB to stop bit
            auto& word = m_data[stop_word_index];
            word = partial(word, 0, stop_bit_offset);
        }

        const size_type start_full_word_index = start_word_index + start_full_word_offset;
        const size_type stop_full_word_index = stop_word_index - stop_full_word_offset;
        assert(start_full_word_index <= stop_full_word_index);

        for (size_type i = start_full_word_index; i <= stop_full_word_index; i++) {
            m_data[i] = full(m_data[i]);
        }
    }
}

template <MixedWidthMode WidthMode, class Allocator>
inline void Bitset<WidthMode, Allocator>::apply_boolean_op(const Bitset& other,
                                                           BOOLEAN_OP auto&& op)
{
    if constexpr (WidthMode == MixedWidthMode::UNSIGNED_PROMOTION) {
        const size_type min_size = std::min(num_words(), other.num_words());

        size_type i = 0;
        while (i < min_size) {
            m_data[i] = op(m_data[i], other.m_data[i]);
            i++;
        }

        if (num_words() > other.num_words()) {
            while (i < num_words()) {
                m_data[i] = op(m_data[i], ZEROS);
                i++;
            }
        } else if (num_words() < other.num_words()) {
            while (i < other.num_words()) {
                m_data.push_back(op(other.m_data[i], ZEROS));
                i++;
            }
        }

        m_num_bits = std::max(m_num_bits, other.m_num_bits);
        zero_unused_bits_in_msw();
    } else {
        if (num_bits() != other.num_bits()) {
            throw std::invalid_argument(std::format(
                    "Attempted to apply boolean op on operands of width {} and {}",
                    num_bits(), other.num_bits()));
        }

        for (size_type i = 0; i < num_words(); i++) {
            m_data[i] = op(m_data[i], other.m_data[i]);
        }

        zero_unused_bits_in_msw();
    }
}

} // namespace MACCHIATO_NAMESPACE
