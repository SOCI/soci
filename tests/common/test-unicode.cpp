//
// Copyright (C) 2024  Benjamin Oldenburg
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include <catch.hpp>

using namespace soci;
using namespace soci::details;

TEST_CASE("UTF-8 validation tests", "[unicode]")
{
    // Valid UTF-8 strings - Should not throw exceptions
    CHECK_NOTHROW(ensure_valid_utf8("Hello, world!"));      // valid ASCII
    CHECK_NOTHROW(ensure_valid_utf8(""));                   // Empty string
    CHECK_NOTHROW(ensure_valid_utf8("Здравствуй, мир!")); // valid UTF-8
    CHECK_NOTHROW(ensure_valid_utf8("こんにちは世界"));   // valid UTF-8
    CHECK_NOTHROW(ensure_valid_utf8("😀😁😂🤣😃😄😅😆")); // valid UTF-8 with emojis

    // Invalid UTF-8 strings - Should throw soci_error exceptions
    CHECK_THROWS_AS(ensure_valid_utf8("\x80"), soci_error);                 // Invalid single byte
    CHECK_THROWS_AS(ensure_valid_utf8("\xC3\x28"), soci_error);             // Invalid two-byte character
    CHECK_THROWS_AS(ensure_valid_utf8("\xE2\x82"), soci_error);             // Truncated three-byte character
    CHECK_THROWS_AS(ensure_valid_utf8("\xF0\x90\x28"), soci_error);         // Truncated four-byte character
    CHECK_THROWS_AS(ensure_valid_utf8("\xF0\x90\x8D\x80\x80"), soci_error); // Extra byte in four-byte character
}

TEST_CASE("UTF-16 validation tests", "[unicode]")
{
    // Valid UTF-16 strings
    CHECK_NOTHROW(ensure_valid_utf16(u"Hello, world!"));    // valid ASCII
    CHECK_NOTHROW(ensure_valid_utf16(u"Здравствуй, мир!")); // valid Cyrillic
    CHECK_NOTHROW(ensure_valid_utf16(u"こんにちは世界"));   // valid Japanese
    CHECK_NOTHROW(ensure_valid_utf16(u"😀😁😂🤣😃😄😅😆")); // valid emojis

    // Invalid UTF-16 strings - these should throw exceptions
    std::u16string invalid_utf16;

    invalid_utf16 = u"";
    invalid_utf16 += 0xD800; // lone high surrogate
    CHECK_THROWS_AS(ensure_valid_utf16(invalid_utf16), soci_error);

    invalid_utf16 = u"";
    invalid_utf16 += 0xDC00; // lone low surrogate
    CHECK_THROWS_AS(ensure_valid_utf16(invalid_utf16), soci_error);

    invalid_utf16 = u"";
    invalid_utf16 += 0xD800;
    invalid_utf16 += 0xD800; // two high surrogates in a row
    CHECK_THROWS_AS(ensure_valid_utf16(invalid_utf16), soci_error);

    invalid_utf16 = u"";
    invalid_utf16 += 0xDC00;
    invalid_utf16 += 0xDC00; // two low surrogates in a row
    CHECK_THROWS_AS(ensure_valid_utf16(invalid_utf16), soci_error);
}

TEST_CASE("UTF-32 validation tests", "[unicode]")
{
    // Valid UTF-32 strings
    REQUIRE_NOTHROW(ensure_valid_utf32(U"Hello, world!"));    // valid ASCII
    REQUIRE_NOTHROW(ensure_valid_utf32(U"Здравствуй, мир!")); // valid Cyrillic
    REQUIRE_NOTHROW(ensure_valid_utf32(U"こんにちは世界"));   // valid Japanese
    REQUIRE_NOTHROW(ensure_valid_utf32(U"😀😁😂🤣😃😄😅😆")); // valid emojis

    // Invalid UTF-32 strings
    REQUIRE_THROWS_AS(ensure_valid_utf32(U"\x110000"), soci_error);   // Invalid UTF-32 code point
    REQUIRE_THROWS_AS(ensure_valid_utf32(U"\x1FFFFF"), soci_error);   // Invalid range
    REQUIRE_THROWS_AS(ensure_valid_utf32(U"\xFFFFFFFF"), soci_error); // Invalid range
}

TEST_CASE("UTF-16 to UTF-32 conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(utf16_to_utf32(u"Hello, world!") == U"Hello, world!");
    REQUIRE(utf16_to_utf32(u"こんにちは世界") == U"こんにちは世界");
    REQUIRE(utf16_to_utf32(u"😀😁😂🤣😃😄😅😆") == U"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u16string utf16;
    utf16.push_back(char16_t(0xD83D));               // high surrogate
    utf16.push_back(char16_t(0xDE00));               // low surrogate
    REQUIRE(utf16_to_utf32(utf16) == U"\U0001F600"); // 😀

    // Invalid conversion (should throw an exception)
    std::u16string invalid_utf16;
    invalid_utf16.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf16_to_utf32(invalid_utf16), soci_error);
}

TEST_CASE("UTF-32 to UTF-16 conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(utf32_to_utf16(U"Hello, world!") == u"Hello, world!");
    REQUIRE(utf32_to_utf16(U"こんにちは世界") == u"こんにちは世界");
    REQUIRE(utf32_to_utf16(U"😀😁😂🤣😃😄😅😆") == u"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u32string utf32 = U"\U0001F600"; // 😀
    std::u16string expected_utf16;
    expected_utf16.push_back(0xD83D); // high surrogate
    expected_utf16.push_back(0xDE00); // low surrogate
    REQUIRE(utf32_to_utf16(utf32) == expected_utf16);

    // Invalid conversion (should throw an exception)
    std::u32string invalid_utf32 = U"\x110000"; // Invalid code point
    REQUIRE_THROWS_AS(utf32_to_utf16(invalid_utf32), soci_error);
}

TEST_CASE("UTF-8 to UTF-16 conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(utf8_to_utf16("Hello, world!") == u"Hello, world!");
    REQUIRE(utf8_to_utf16("こんにちは世界") == u"こんにちは世界");
    REQUIRE(utf8_to_utf16("😀😁😂🤣😃😄😅😆") == u"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    std::u16string expected_utf16 = u"\xD83D\xDE00";
    REQUIRE(utf8_to_utf16(utf8) == expected_utf16);

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_utf16(invalid_utf8), soci_error);
}

TEST_CASE("UTF-16 to UTF-8 conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(utf16_to_utf8(u"Hello, world!") == "Hello, world!");
    REQUIRE(utf16_to_utf8(u"こんにちは世界") == "こんにちは世界");
    REQUIRE(utf16_to_utf8(u"😀😁😂🤣😃😄😅😆") == "😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u16string utf16;
    utf16.push_back(0xD83D);                             // high surrogate
    utf16.push_back(0xDE00);                             // low surrogate
    REQUIRE(utf16_to_utf8(utf16) == "\xF0\x9F\x98\x80"); // 😀

    // Invalid conversion (should throw an exception)
    std::u16string invalid_utf16;
    invalid_utf16.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf16_to_utf8(invalid_utf16), soci_error);
}

TEST_CASE("UTF-8 to UTF-32 conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(utf8_to_utf32("Hello, world!") == U"Hello, world!");
    REQUIRE(utf8_to_utf32("こんにちは世界") == U"こんにちは世界");
    REQUIRE(utf8_to_utf32("😀😁😂🤣😃😄😅😆") == U"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    REQUIRE(utf8_to_utf32(utf8) == U"\U0001F600");

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_utf32(invalid_utf8), soci_error);
}

TEST_CASE("UTF-32 to UTF-8 conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(utf32_to_utf8(U"Hello, world!") == "Hello, world!");
    REQUIRE(utf32_to_utf8(U"こんにちは世界") == "こんにちは世界");
    REQUIRE(utf32_to_utf8(U"😀😁😂🤣😃😄😅😆") == "😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u32string utf32 = U"\U0001F600"; // 😀
    REQUIRE(utf32_to_utf8(utf32) == "\xF0\x9F\x98\x80");

    // Invalid conversion (should throw an exception)
    std::u32string invalid_utf32 = U"\x110000"; // Invalid code point
    REQUIRE_THROWS_AS(utf32_to_utf8(invalid_utf32), soci_error);

    // Invalid conversion (should throw an exception)
    std::u32string invalid_wide;
    invalid_wide.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf32_to_utf8(invalid_wide), soci_error);
}

TEST_CASE("Empty string tests", "[unicode]")
{
    REQUIRE(utf16_to_utf8(u"") == "");
    REQUIRE(utf32_to_utf8(U"") == "");
    REQUIRE(utf8_to_utf16("") == u"");
    REQUIRE(utf8_to_utf32("") == U"");
}

TEST_CASE("Strings with Byte Order Marks (BOMs)", "[unicode]")
{
    // UTF-8 BOM
    const std::string utf8_bom = "\xEF\xBB\xBF";
    // UTF-16 BOM (Little Endian)
    const std::u16string utf16_bom = u"\xFEFF";
    // UTF-32 BOM (Little Endian)
    const std::u32string utf32_bom = U"\x0000FEFF";

    const std::string content = "Hello, world!";
    const std::u16string content16 = u"Hello, world!";
    const std::u32string content32 = U"Hello, world!";

    SECTION("UTF-8 to UTF-16")
    {
        std::u16string result = utf8_to_utf16(utf8_bom + content);
        REQUIRE(result == utf16_bom + content16);
    }

    SECTION("UTF-8 to UTF-32")
    {
        std::u32string result = utf8_to_utf32(utf8_bom + content);
        REQUIRE(result == utf32_bom + content32);
    }

    SECTION("UTF-16 to UTF-8")
    {
        std::string result = utf16_to_utf8(utf16_bom + content16);
        REQUIRE(result == utf8_bom + content);
    }

    SECTION("UTF-16 to UTF-32")
    {
        std::u32string result = utf16_to_utf32(utf16_bom + content16);
        REQUIRE(result == utf32_bom + content32);
    }

    SECTION("UTF-32 to UTF-8")
    {
        std::string result = utf32_to_utf8(utf32_bom + content32);
        REQUIRE(result == utf8_bom + content);
    }

    SECTION("UTF-32 to UTF-16")
    {
        std::u16string result = utf32_to_utf16(utf32_bom + content32);
        REQUIRE(result == utf16_bom + content16);
    }

    SECTION("Roundtrip conversions")
    {
        // UTF-8 -> UTF-16 -> UTF-8
        REQUIRE(utf16_to_utf8(utf8_to_utf16(utf8_bom + content)) == utf8_bom + content);

        // UTF-8 -> UTF-32 -> UTF-8
        REQUIRE(utf32_to_utf8(utf8_to_utf32(utf8_bom + content)) == utf8_bom + content);

        // UTF-16 -> UTF-8 -> UTF-16
        REQUIRE(utf8_to_utf16(utf16_to_utf8(utf16_bom + content16)) == utf16_bom + content16);

        // UTF-16 -> UTF-32 -> UTF-16
        REQUIRE(utf32_to_utf16(utf16_to_utf32(utf16_bom + content16)) == utf16_bom + content16);

        // UTF-32 -> UTF-8 -> UTF-32
        REQUIRE(utf8_to_utf32(utf32_to_utf8(utf32_bom + content32)) == utf32_bom + content32);

        // UTF-32 -> UTF-16 -> UTF-32
        REQUIRE(utf16_to_utf32(utf32_to_utf16(utf32_bom + content32)) == utf32_bom + content32);
    }
}

TEST_CASE("Strings with invalid code unit sequences", "[unicode]")
{
    REQUIRE_THROWS_AS(ensure_valid_utf16(u"\xD800\xD800"), soci_error);
    REQUIRE_THROWS_AS(ensure_valid_utf32(U"\xD800"), soci_error);
}

TEST_CASE("Strings with overlong encodings", "[unicode]")
{
    REQUIRE_THROWS_AS(ensure_valid_utf8("\xC0\xAF"), soci_error);
}

TEST_CASE("Strings with non-characters", "[unicode]")
{
    REQUIRE_THROWS_AS(ensure_valid_utf32(U"\xFFFE"), soci_error);
}

TEST_CASE("Strings with right-to-left characters", "[unicode]")
{
    REQUIRE_NOTHROW(ensure_valid_utf8("مرحبا بالعالم"));
}

TEST_CASE("UTF-8 to wide string conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(utf8_to_wide("Hello, world!") == L"Hello, world!");
    REQUIRE(utf8_to_wide("こんにちは世界") == L"こんにちは世界");
    REQUIRE(utf8_to_wide("😀😁😂🤣😃😄😅😆") == L"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    std::wstring expected_wide = L"\U0001F600";
    REQUIRE(utf8_to_wide(utf8) == expected_wide);

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_wide(invalid_utf8), soci_error);
}

TEST_CASE("Wide string to UTF-8 conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(wide_to_utf8(L"Hello, world!") == "Hello, world!");
    REQUIRE(wide_to_utf8(L"こんにちは世界") == "こんにちは世界");
    REQUIRE(wide_to_utf8(L"😀😁😂🤣😃😄😅😆") == "😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::wstring wide = L"\U0001F600"; // 😀
    REQUIRE(wide_to_utf8(wide) == "\xF0\x9F\x98\x80");

    // Invalid conversion (should throw an exception)
    std::wstring invalid_wide;
    invalid_wide.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(wide_to_utf8(invalid_wide), soci_error);
}

TEST_CASE("UTF-16 to wide string conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(utf16_to_wide(u"Hello, world!") == L"Hello, world!");
    REQUIRE(utf16_to_wide(u"こんにちは世界") == L"こんにちは世界");
    REQUIRE(utf16_to_wide(u"😀😁😂🤣😃😄😅😆") == L"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u16string utf16 = u"\xD83D\xDE00"; // 😀
    std::wstring expected_wide = L"\U0001F600";
    REQUIRE(utf16_to_wide(utf16) == expected_wide);

    // Invalid conversion (should throw an exception)
    std::u16string invalid_utf16;
    invalid_utf16.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf16_to_wide(invalid_utf16), soci_error);
}

TEST_CASE("Wide string to UTF-16 conversion tests", "[unicode]")
{
    // Valid conversion tests
    REQUIRE(wide_to_utf16(L"Hello, world!") == u"Hello, world!");
    REQUIRE(wide_to_utf16(L"こんにちは世界") == u"こんにちは世界");
    REQUIRE(wide_to_utf16(L"😀😁😂🤣😃😄😅😆") == u"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::wstring wide = L"\U0001F600"; // 😀
    REQUIRE(wide_to_utf16(wide) == u"\xD83D\xDE00");

    // Invalid conversion (should throw an exception)
    std::wstring invalid_wide;
    invalid_wide.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(wide_to_utf16(invalid_wide), soci_error);
}

