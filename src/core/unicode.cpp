//
// Copyright (C) 2024 Benjamin Oldenburg
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/soci-unicode.h"
#include <cstdint>

namespace soci
{

namespace details
{

bool is_valid_utf8_sequence(unsigned char const* bytes, int length)
{
    if (length == 1)
    {
        return (bytes[0] & 0x80U) == 0;
    }
    if (length == 2)
    {
        if ((bytes[0] & 0xE0U) == 0xC0 && (bytes[1] & 0xC0U) == 0x80)
        {
            // Check for overlong encoding
            const uint32_t code_point = ((bytes[0] & 0x1FU) << 6U) | (bytes[1] & 0x3FU);
            return code_point >= 0x80;
        }
        return false;
    }
    if (length == 3)
    {
        if ((bytes[0] & 0xF0U) == 0xE0 && (bytes[1] & 0xC0U) == 0x80 && (bytes[2] & 0xC0U) == 0x80)
        {
            // Check for overlong encoding
            const uint32_t code_point = ((bytes[0] & 0x0FU) << 12U) | ((bytes[1] & 0x3FU) << 6U) | (bytes[2] & 0x3FU);
            return code_point >= 0x800 && code_point <= 0xFFFF;
        }
        return false;
    }
    if (length == 4)
    {
        if ((bytes[0] & 0xF8U) == 0xF0 && (bytes[1] & 0xC0U) == 0x80 && (bytes[2] & 0xC0U) == 0x80 && (bytes[3] & 0xC0U) == 0x80)
        {
            // Check for overlong encoding and valid Unicode code point
            const uint32_t code_point = ((bytes[0] & 0x07U) << 18U) | ((bytes[1] & 0x3FU) << 12U) | ((bytes[2] & 0x3FU) << 6U) | (bytes[3] & 0x3FU);
            return code_point >= 0x10000 && code_point <= 0x10FFFF;
        }
        return false;
    }
    return false;
}

void ensure_valid_utf8(char const* utf8, std::size_t len)
{
    auto const* const bytes = reinterpret_cast<unsigned char const*>(utf8);

    for (std::size_t i = 0; i < len;)
    {
        if ((bytes[i] & 0x80U) == 0)
        {
            // ASCII character, one byte
            i += 1;
        }
        else if ((bytes[i] & 0xE0U) == 0xC0)
        {
            // Two-byte character, check if the next byte is a valid continuation byte
            if (i + 1 >= len || !is_valid_utf8_sequence(bytes + i, 2))
            {
                throw soci_error("Invalid UTF-8 sequence: Truncated or invalid two-byte sequence");
            }
            i += 2;
        }
        else if ((bytes[i] & 0xF0U) == 0xE0U)
        {
            // Three-byte character, check if the next two bytes are valid continuation bytes
            if (i + 2 >= len || !is_valid_utf8_sequence(bytes + i, 3))
            {
                throw soci_error("Invalid UTF-8 sequence: Truncated or invalid three-byte sequence");
            }
            i += 3;
        }
        else if ((bytes[i] & 0xF8U) == 0xF0U)
        {
            // Four-byte character, check if the next three bytes are valid continuation bytes
            if (i + 3 >= len || !is_valid_utf8_sequence(bytes + i, 4))
            {
                throw soci_error("Invalid UTF-8 sequence: Truncated or invalid four-byte sequence");
            }
            i += 4;
        }
        else
        {
            // Invalid start byte
            throw soci_error("Invalid UTF-8 sequence: Invalid start byte");
        }
    }
}

void ensure_valid_utf16(char16_t const* s, std::size_t len)
{
    for (std::size_t i = 0; i < len; ++i)
    {
        const char16_t chr = s[i];
        if (chr >= 0xD800 && chr <= 0xDBFF)
        { // High surrogate
            if (i + 1 >= len)
            {
                throw soci_error("Invalid UTF-16 sequence (truncated surrogate pair)");
            }
            const char16_t next = s[i + 1];
            if (next < 0xDC00 || next > 0xDFFF)
            {
                throw soci_error("Invalid UTF-16 sequence (invalid surrogate pair)");
            }
            ++i; // Skip the next character as it's part of the pair
        }
        else if (chr >= 0xDC00 && chr <= 0xDFFF)
        { // Lone low surrogate
            throw soci_error("Invalid UTF-16 sequence (lone low surrogate)");
        }
    }
}

void ensure_valid_utf32(char32_t const* s, std::size_t len)
{
    for (std::size_t i = 0; i < len; ++i)
    {
        const char32_t chr = s[i];

        // Check if the code point is within the Unicode range
        if (chr > 0x10FFFF)
        {
            throw soci_error("Invalid UTF-32 sequence: Code point out of range");
        }

        // Surrogate pairs are not valid in UTF-32
        if (chr >= 0xD800 && chr <= 0xDFFF)
        {
            throw soci_error("Invalid UTF-32 sequence: Surrogate pair found");
        }

        // Check for non-characters U+FFFE and U+FFFF
        if (chr == 0xFFFE || chr == 0xFFFF)
        {
            throw soci_error("Invalid UTF-32 sequence: Non-character found");
        }
    }
}

std::size_t
utf8_to_utf16(char const* utf8, std::size_t len8,
              char16_t* out16, std::size_t len16)
{
    // Skip the check if we're just computing the length for efficiency, we'll
    // detect any errors when performing the actual conversion anyhow.
    if (out16)
        ensure_valid_utf8(utf8, len8);

    auto const* const bytes = reinterpret_cast<unsigned char const*>(utf8);

    std::size_t len = 0;

    // Check for UTF-8 BOM
    size_t start_index = 0;
    if (len8 >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
    {
        ++len;

        if (out16)
        {
            throw_if_too_small(len, len16);
            *out16++ = 0xFEFF; // Add UTF-16 BOM
        }

        start_index = 3;         // Start conversion after the BOM
    }

    for (size_t i = start_index; i < len8;)
    {
        uint32_t codepoint;
        if ((bytes[i] & 0x80) == 0)
        {
            // ASCII character
            codepoint = bytes[i++];
        }
        else if ((bytes[i] & 0xE0) == 0xC0)
        {
            // 2-byte sequence
            codepoint = ((bytes[i] & 0x1F) << 6) | (bytes[i + 1] & 0x3F);
            i += 2;
        }
        else if ((bytes[i] & 0xF0) == 0xE0)
        {
            // 3-byte sequence
            codepoint = ((bytes[i] & 0x0F) << 12) | ((bytes[i + 1] & 0x3F) << 6) | (bytes[i + 2] & 0x3F);
            i += 3;
        }
        else if ((bytes[i] & 0xF8) == 0xF0)
        {
            // 4-byte sequence
            codepoint = ((bytes[i] & 0x07) << 18) | ((bytes[i + 1] & 0x3F) << 12) | ((bytes[i + 2] & 0x3F) << 6) | (bytes[i + 3] & 0x3F);
            i += 4;
        }
        else
        {
            throw soci_error("Invalid UTF-8 sequence");
        }

        if (codepoint <= 0xFFFF)
        {
            ++len;

            if (out16)
            {
                throw_if_too_small(len, len16);
                *out16++ = static_cast<char16_t>(codepoint);
            }
        }
        else
        {
            // Encode as surrogate pair
            len += 2;

            if (out16)
            {
                throw_if_too_small(len, len16);
                codepoint -= 0x10000;
                *out16++ = static_cast<char16_t>((codepoint >> 10) + 0xD800);
                *out16++ = static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00);
            }
        }
    }

    return len;
}

std::size_t
utf16_to_utf8(char16_t const* utf16, std::size_t len16,
              char* out8, std::size_t len8)
{
    // Skip the check if we're just computing the length for efficiency, we'll
    // detect any errors when performing the actual conversion anyhow.
    if (out8)
        ensure_valid_utf16(utf16, len16);

    std::size_t len = 0;

    // Check for UTF-16 BOM
    size_t start_index = 0;
    if (len16 && utf16[0] == 0xFEFF)
    {
        len += 3;
        if (out8)
        {
            throw_if_too_small(len, len8);

            // Add UTF-8 BOM
            *out8++ = '\xEF';
            *out8++ = '\xBB';
            *out8++ = '\xBF';
        }

        start_index = 1;             // Start conversion after the BOM
    }

    for (std::size_t i = start_index; i < len16; ++i)
    {
        char16_t const chr = utf16[i];

        if (chr < 0x80)
        {
            // 1-byte sequence (ASCII)
            ++len;
            if (out8)
            {
                throw_if_too_small(len, len8);
                *out8++ = static_cast<char>(chr);
            }
        }
        else if (chr < 0x800)
        {
            // 2-byte sequence
            len += 2;
            if (out8)
            {
                throw_if_too_small(len, len8);
                *out8++ = static_cast<char>(0xC0U | ((chr >> 6) & 0x1FU));
                *out8++ = static_cast<char>(0x80U | (chr & 0x3FU));
            }
        }
        else if ((chr >= 0xD800U) && (chr <= 0xDBFFU))
        {
            // Handle UTF-16 surrogate pairs
            if (i + 1 >= len16)
            {
                throw soci_error("Invalid UTF-16 surrogate pair (truncated)");
            }
            char16_t const chr2 = utf16[i + 1];
            if (chr2 < 0xDC00U || chr2 > 0xDFFFU)
            {
                throw soci_error("Invalid UTF-16 surrogate pair");
            }
            auto const codepoint = static_cast<uint32_t>(((chr & 0x3FFU) << 10U) | (chr2 & 0x3FFU)) + 0x10000U;

            len += 4;
            if (out8)
            {
                throw_if_too_small(len, len8);
                *out8++ = static_cast<char>(0xF0U | ((codepoint >> 18U) & 0x07U));
                *out8++ = static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU));
                *out8++ = static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU));
                *out8++ = static_cast<char>(0x80U | (codepoint & 0x3FU));
            }

            ++i; // Skip the next character as it is part of the surrogate pair
        }
        else
        {
            // 3-byte sequence
            len += 3;
            if (out8)
            {
                throw_if_too_small(len, len8);
                *out8++ = static_cast<char>(0xE0U | ((chr >> 12) & 0x0FU));
                *out8++ = static_cast<char>(0x80U | ((chr >> 6) & 0x3FU));
                *out8++ = static_cast<char>(0x80U | (chr & 0x3FU));
            }
        }
    }

    return len;
}

std::size_t
utf16_to_utf32(char16_t const* utf16, std::size_t len16,
               char32_t* out32, std::size_t len32)
{
    // Skip the check if we're just computing the length for efficiency, we'll
    // detect any errors when performing the actual conversion anyhow.
    if (out32)
        ensure_valid_utf16(utf16, len16);

    std::size_t len = 0;
    for (std::size_t i = 0; i < len16; ++i)
    {
        char16_t const chr = *utf16++;

        ++len;
        if (out32)
            throw_if_too_small(len, len32);

        if (chr >= 0xD800U && chr <= 0xDBFFU)
        {
            // High surrogate, must be followed by a low surrogate
            char16_t const chr2 = *utf16++;
            ++i;

            if (out32)
            {
                const auto codepoint = static_cast<uint32_t>(((static_cast<unsigned int>(chr) & 0x3FFU) << 10U) | (static_cast<unsigned int>(chr2) & 0x3FFU)) + 0x10000U;
                *out32++ = codepoint;
            }
        }
        else
        {
            // Valid BMP character or a low surrogate that is part of a valid
            // pair (already checked by ensure_valid_utf16)
            if (out32)
                *out32++ = static_cast<char32_t>(chr);
        }
    }

    return len;
}

std::size_t
utf32_to_utf16(char32_t const* utf32, std::size_t len32,
               char16_t* out16, std::size_t len16)
{
    // Skip the check if we're just computing the length for efficiency, we'll
    // detect any errors when performing the actual conversion anyhow.
    if (out16)
        ensure_valid_utf32(utf32, len32);

    std::size_t len = 0;
    for (std::size_t i = 0; i < len32; ++i)
    {
        char32_t codepoint = *utf32++;

        if (codepoint <= 0xFFFFU)
        {
            ++len;

            // BMP character
            if (out16)
            {
                throw_if_too_small(len, len16);
                *out16++ = static_cast<char16_t>(codepoint);
            }
        }
        else
        {
            len += 2;

            // Encode as a surrogate pair
            if (out16)
            {
                throw_if_too_small(len, len16);

                // Note that we know that the code point is valid here because
                // we called ensure_valid_utf32() above.
                codepoint -= 0x10000;
                *out16++ = static_cast<char16_t>((codepoint >> 10U) + 0xD800U);
                *out16++ = static_cast<char16_t>((codepoint & 0x3FFU) + 0xDC00U);
            }
        }
    }

    return len;
}

std::size_t
utf8_to_utf32(char const* utf8, std::size_t len8,
              char32_t* out32, std::size_t len32)
{
    // Skip the check if we're just computing the length for efficiency, we'll
    // detect any errors when performing the actual conversion anyhow.
    if (out32)
        ensure_valid_utf8(utf8, len8);

    auto const* const bytes = reinterpret_cast<unsigned char const*>(utf8);

    std::size_t len = 0;
    for (std::size_t i = 0; i < len8;)
    {
        unsigned char chr1 = bytes[i];

        ++len;
        if (out32)
            throw_if_too_small(len, len32);

        // 1-byte sequence (ASCII)
        if ((chr1 & 0x80U) == 0)
        {
            if (out32)
                *out32++ = static_cast<char32_t>(chr1);
            ++i;
        }
        // 2-byte sequence
        else if ((chr1 & 0xE0U) == 0xC0U)
        {
            if (out32)
                *out32++ = static_cast<char32_t>(((chr1 & 0x1FU) << 6U) | (bytes[i + 1] & 0x3FU));
            i += 2;
        }
        // 3-byte sequence
        else if ((chr1 & 0xF0U) == 0xE0U)
        {
            if (out32)
                *out32++ = static_cast<char32_t>(((chr1 & 0x0FU) << 12U) | ((bytes[i + 1] & 0x3FU) << 6U) | (bytes[i + 2] & 0x3FU));
            i += 3;
        }
        // 4-byte sequence
        else if ((chr1 & 0xF8U) == 0xF0U)
        {
            if (out32)
                *out32++ = static_cast<char32_t>(((chr1 & 0x07U) << 18U) | ((bytes[i + 1] & 0x3FU) << 12U) | ((bytes[i + 2] & 0x3FU) << 6U) | (bytes[i + 3] & 0x3FU));
            i += 4;
        }
    }

    return len;
}

std::size_t
utf32_to_utf8(char32_t const* utf32, std::size_t len32,
              char* out8, std::size_t len8)
{
    // Skip the check if we're just computing the length for efficiency, we'll
    // detect any errors when performing the actual conversion anyhow.
    if (out8)
        ensure_valid_utf32(utf32, len32);

    std::size_t len = 0;

    for (std::size_t i = 0; i < len32; ++i)
    {
        auto const codepoint = utf32[i];

        if (codepoint < 0x80)
        {
            // 1-byte sequence (ASCII)
            ++len;
            if (out8)
            {
                throw_if_too_small(len, len8);
                *out8++ = static_cast<char>(codepoint);
            }
        }
        else if (codepoint < 0x800)
        {
            // 2-byte sequence
            len += 2;

            if (out8)
            {
                throw_if_too_small(len, len8);
                *out8++ = static_cast<char>(0xC0U | ((codepoint >> 6U) & 0x1FU));
                *out8++ = static_cast<char>(0x80U | (codepoint & 0x3FU));
            }
        }
        else if (codepoint < 0x10000)
        {
            // 3-byte sequence
            len += 3;

            if (out8)
            {
                throw_if_too_small(len, len8);
                *out8++ = static_cast<char>(0xE0U | ((codepoint >> 12U) & 0x0FU));
                *out8++ = static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU));
                *out8++ = static_cast<char>(0x80U | (codepoint & 0x3FU));
            }
        }
        else // This must be the only remaining case for valid UTF-32 string.
        {
            // 4-byte sequence
            len += 4;

            if (out8)
            {
                throw_if_too_small(len, len8);
                *out8++ = static_cast<char>(0xF0U | ((codepoint >> 18U) & 0x07U));
                *out8++ = static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU));
                *out8++ = static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU));
                *out8++ = static_cast<char>(0x80U | (codepoint & 0x3FU));
            }
        }
    }

    return len;
}

} // namespace details

} // namespace soci
