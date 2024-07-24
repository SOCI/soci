//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/empty/soci-empty.h"

// Normally the tests would include common-tests.h here, but we can't run any
// of the tests registered there, so instead include CATCH header directly.
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace soci;

std::string connectString;
backend_factory const &backEnd = *soci::factory_empty();

// NOTE:
// This file is supposed to serve two purposes:
// 1. To be a starting point for implementing new tests (for new backends).
// 2. To exercise (at least some of) the syntax and try the SOCI library
//    against different compilers, even in those environments where there
//    is no database. SOCI uses advanced template techniques which are known
//    to cause problems on different versions of popular compilers, and this
//    test is handy to verify that the code is accepted by as many compilers
//    as possible.
//
// Both of these purposes mean that the actual code here is meaningless
// from the database-development point of view. For new tests, you may wish
// to remove this code and keep only the general structure of this file.

struct Person
{
    int id;
    std::string firstName;
    std::string lastName;
};

namespace soci
{
    template<> struct type_conversion<Person>
    {
        typedef values base_type;
        static void from_base(values & /* r */, indicator /* ind */,
            Person & /* p */)
        {
        }
    };
}

TEST_CASE("Dummy test", "[empty]")
{
    soci::session sql(backEnd, connectString);

    sql << "Do what I want.";
    sql << "Do what I want " << 123 << " times.";

    char const* const query = "some query";
    sql << query;

    {
        std::string squery = "some query";
        sql << squery;
    }

    int i = 7;
    sql << "insert", use(i);
    sql << "select", into(i);
    sql << query, use(i);
    sql << query, into(i);

#if defined (__LP64__) || ( __WORDSIZE == 64 )
    long int li = 9;
    sql << "insert", use(li);
    sql << "select", into(li);
#endif

    long long ll = 11;
    sql << "insert", use(ll);
    sql << "select", into(ll);

    indicator ind = i_ok;
    sql << "insert", use(i, ind);
    sql << "select", into(i, ind);
    sql << query, use(i, ind);
    sql << query, use(i, ind);

    std::vector<int> numbers(100);
    sql << "insert", use(numbers);
    sql << "select", into(numbers);

    std::vector<indicator> inds(100);
    sql << "insert", use(numbers, inds);
    sql << "select", into(numbers, inds);

    {
        statement st = (sql.prepare << "select", into(i));
        st.execute();
        st.fetch();
    }
    {
        statement st = (sql.prepare << query, into(i));
        st.execute();
        st.fetch();
    }
    {
        statement st = (sql.prepare << "select", into(i, ind));
        statement sq = (sql.prepare << query, into(i, ind));
    }
    {
        statement st = (sql.prepare << "select", into(numbers));
    }
    {
        statement st = (sql.prepare << "select", into(numbers, inds));
    }
    {
        statement st = (sql.prepare << "insert", use(i));
        statement sq = (sql.prepare << query, use(i));
    }
    {
        statement st = (sql.prepare << "insert", use(i, ind));
        statement sq = (sql.prepare << query, use(i, ind));
    }
    {
        statement st = (sql.prepare << "insert", use(numbers));
    }
    {
        statement st = (sql.prepare << "insert", use(numbers, inds));
    }
    {
        Person p;
        sql << "select person", into(p);
    }
}

TEST_CASE("UTF-8 validation tests", "[unicode]")
{
    using namespace soci::details;

    // Valid UTF-8 strings - Should not throw exceptions
    REQUIRE_NOTHROW(is_valid_utf8("Hello, world!"));      // valid ASCII
    REQUIRE_NOTHROW(is_valid_utf8(""));                   // Empty string
    REQUIRE_NOTHROW(is_valid_utf8(u8"Здравствуй, мир!")); // valid UTF-8
    REQUIRE_NOTHROW(is_valid_utf8(u8"こんにちは世界"));   // valid UTF-8
    REQUIRE_NOTHROW(is_valid_utf8(u8"😀😁😂🤣😃😄😅😆")); // valid UTF-8 with emojis

    // Invalid UTF-8 strings - Should throw soci_error exceptions
    CHECK_THROWS_AS(is_valid_utf8("\x80"), soci_error);                 // Invalid single byte
    CHECK_THROWS_AS(is_valid_utf8("\xC3\x28"), soci_error);             // Invalid two-byte character
    CHECK_THROWS_AS(is_valid_utf8("\xE2\x82"), soci_error);             // Truncated three-byte character
    CHECK_THROWS_AS(is_valid_utf8("\xF0\x90\x28"), soci_error);         // Truncated four-byte character
    CHECK_THROWS_AS(is_valid_utf8("\xF0\x90\x8D\x80\x80"), soci_error); // Extra byte in four-byte character
}

TEST_CASE("UTF-16 validation tests", "[unicode]")
{
    using namespace soci::details;

    // Valid UTF-16 strings
    REQUIRE_NOTHROW(is_valid_utf16(u"Hello, world!"));    // valid ASCII
    REQUIRE_NOTHROW(is_valid_utf16(u"Здравствуй, мир!")); // valid Cyrillic
    REQUIRE_NOTHROW(is_valid_utf16(u"こんにちは世界"));   // valid Japanese
    REQUIRE_NOTHROW(is_valid_utf16(u"😀😁😂🤣😃😄😅😆")); // valid emojis

    // Invalid UTF-16 strings - these should throw exceptions
    std::u16string invalid_utf16;

    invalid_utf16 = u"";
    invalid_utf16 += 0xD800; // lone high surrogate
    REQUIRE_THROWS_AS(is_valid_utf16(invalid_utf16), soci_error);

    invalid_utf16 = u"";
    invalid_utf16 += 0xDC00; // lone low surrogate
    REQUIRE_THROWS_AS(is_valid_utf16(invalid_utf16), soci_error);

    invalid_utf16 = u"";
    invalid_utf16 += 0xD800;
    invalid_utf16 += 0xD800; // two high surrogates in a row
    REQUIRE_THROWS_AS(is_valid_utf16(invalid_utf16), soci_error);

    invalid_utf16 = u"";
    invalid_utf16 += 0xDC00;
    invalid_utf16 += 0xDC00; // two low surrogates in a row
    REQUIRE_THROWS_AS(is_valid_utf16(invalid_utf16), soci_error);
}

TEST_CASE("UTF-32 validation tests", "[unicode]")
{
    using namespace soci::details;

    // Valid UTF-32 strings
    REQUIRE_NOTHROW(is_valid_utf32(U"Hello, world!"));    // valid ASCII
    REQUIRE_NOTHROW(is_valid_utf32(U"Здравствуй, мир!")); // valid Cyrillic
    REQUIRE_NOTHROW(is_valid_utf32(U"こんにちは世界"));   // valid Japanese
    REQUIRE_NOTHROW(is_valid_utf32(U"😀😁😂🤣😃😄😅😆")); // valid emojis

    // Invalid UTF-32 strings
    REQUIRE_THROWS_AS(is_valid_utf32(U"\x110000"), soci_error);   // Invalid UTF-32 code point
    REQUIRE_THROWS_AS(is_valid_utf32(U"\x1FFFFF"), soci_error);   // Invalid range
    REQUIRE_THROWS_AS(is_valid_utf32(U"\xFFFFFFFF"), soci_error); // Invalid range
}

TEST_CASE("UTF-16 to UTF-32 conversion tests", "[unicode]")
{
    using namespace soci::details;

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
    REQUIRE_THROWS_AS(utf16_to_utf32(invalid_utf16), soci::soci_error);
}

TEST_CASE("UTF-32 to UTF-16 conversion tests", "[unicode]")
{
    using namespace soci::details;

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
    REQUIRE_THROWS_AS(utf32_to_utf16(invalid_utf32), soci::soci_error);
}

TEST_CASE("UTF-8 to UTF-16 conversion tests", "[unicode]")
{
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf8_to_utf16(u8"Hello, world!") == u"Hello, world!");
    REQUIRE(utf8_to_utf16(u8"こんにちは世界") == u"こんにちは世界");
    REQUIRE(utf8_to_utf16(u8"😀😁😂🤣😃😄😅😆") == u"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    std::u16string expected_utf16 = u"\xD83D\xDE00";
    REQUIRE(utf8_to_utf16(utf8) == expected_utf16);

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_utf16(invalid_utf8), soci::soci_error);
}

TEST_CASE("UTF-16 to UTF-8 conversion tests", "[unicode]")
{
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf16_to_utf8(u"Hello, world!") == u8"Hello, world!");
    REQUIRE(utf16_to_utf8(u"こんにちは世界") == u8"こんにちは世界");
    REQUIRE(utf16_to_utf8(u"😀😁😂🤣😃😄😅😆") == u8"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u16string utf16;
    utf16.push_back(0xD83D);                             // high surrogate
    utf16.push_back(0xDE00);                             // low surrogate
    REQUIRE(utf16_to_utf8(utf16) == "\xF0\x9F\x98\x80"); // 😀

    // Invalid conversion (should throw an exception)
    std::u16string invalid_utf16;
    invalid_utf16.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf16_to_utf8(invalid_utf16), soci::soci_error);
}

TEST_CASE("UTF-8 to UTF-32 conversion tests", "[unicode]")
{
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf8_to_utf32(u8"Hello, world!") == U"Hello, world!");
    REQUIRE(utf8_to_utf32(u8"こんにちは世界") == U"こんにちは世界");
    REQUIRE(utf8_to_utf32(u8"😀😁😂🤣😃😄😅😆") == U"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    REQUIRE(utf8_to_utf32(utf8) == U"\U0001F600");

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_utf32(invalid_utf8), soci::soci_error);
}

TEST_CASE("UTF-32 to UTF-8 conversion tests", "[unicode]")
{
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf32_to_utf8(U"Hello, world!") == u8"Hello, world!");
    REQUIRE(utf32_to_utf8(U"こんにちは世界") == u8"こんにちは世界");
    REQUIRE(utf32_to_utf8(U"😀😁😂🤣😃😄😅😆") == u8"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::u32string utf32 = U"\U0001F600"; // 😀
    REQUIRE(utf32_to_utf8(utf32) == "\xF0\x9F\x98\x80");

    // Invalid conversion (should throw an exception)
    std::u32string invalid_utf32 = U"\x110000"; // Invalid code point
    REQUIRE_THROWS_AS(utf32_to_utf8(invalid_utf32), soci::soci_error);

    // Invalid conversion (should throw an exception)
    std::u32string invalid_wide;
    invalid_wide.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(utf32_to_utf8(invalid_wide), soci::soci_error);
}

TEST_CASE("Empty string tests", "[unicode]")
{
    using namespace soci::details;

    REQUIRE(utf16_to_utf8(u"") == u8"");
    REQUIRE(utf32_to_utf8(U"") == u8"");
    REQUIRE(utf8_to_utf16(u8"") == u"");
    REQUIRE(utf8_to_utf32(u8"") == U"");
}
TEST_CASE("Strings with Byte Order Marks (BOMs)", "[unicode]")
{
    using namespace soci::details;

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
    using namespace soci::details;

    REQUIRE_THROWS_AS(is_valid_utf16(u"\xD800\xD800"), soci_error);
    REQUIRE_THROWS_AS(is_valid_utf32(U"\xD800"), soci_error);
}

TEST_CASE("Strings with overlong encodings", "[unicode]")
{
    using namespace soci::details;

    REQUIRE_THROWS_AS(is_valid_utf8("\xC0\xAF"), soci_error);
}

TEST_CASE("Strings with non-characters", "[unicode]")
{
    using namespace soci::details;

    REQUIRE_THROWS_AS(is_valid_utf32(U"\xFFFE"), soci_error);
}

// TEST_CASE("Strings with combining characters", "[unicode]")
// {
//    using namespace soci::details;

//    REQUIRE_NOTHROW(is_valid_utf8(u8"a\u0300"));
//    REQUIRE(utf16_to_utf8(u"a\u0300") == u8"\xC3\xA0");
// }

TEST_CASE("Strings with right-to-left characters", "[unicode]")
{
    using namespace soci::details;

    REQUIRE_NOTHROW(is_valid_utf8(u8"مرحبا بالعالم"));
}

// TEST_CASE("Strings with different normalization forms", "[unicode]")
// {
//    using namespace soci::details;

//    REQUIRE(utf16_to_utf8(u"a\u0300") == u8"\xC3\xA0");
// }

TEST_CASE("UTF-8 to wide string conversion tests", "[unicode]")
{
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(utf8_to_wide(u8"Hello, world!") == L"Hello, world!");
    REQUIRE(utf8_to_wide(u8"こんにちは世界") == L"こんにちは世界");
    REQUIRE(utf8_to_wide(u8"😀😁😂🤣😃😄😅😆") == L"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::string utf8 = "\xF0\x9F\x98\x80"; // 😀
    std::wstring expected_wide = L"\U0001F600";
    REQUIRE(utf8_to_wide(utf8) == expected_wide);

    // Invalid conversion (should throw an exception)
    std::string invalid_utf8 = "\xF0\x28\x8C\xBC"; // Invalid UTF-8 sequence
    REQUIRE_THROWS_AS(utf8_to_wide(invalid_utf8), soci::soci_error);
}

TEST_CASE("Wide string to UTF-8 conversion tests", "[unicode]")
{
    using namespace soci::details;

    // Valid conversion tests
    REQUIRE(wide_to_utf8(L"Hello, world!") == u8"Hello, world!");
    REQUIRE(wide_to_utf8(L"こんにちは世界") == u8"こんにちは世界");
    REQUIRE(wide_to_utf8(L"😀😁😂🤣😃😄😅😆") == u8"😀😁😂🤣😃😄😅😆");

    // Edge cases
    std::wstring wide = L"\U0001F600"; // 😀
    REQUIRE(wide_to_utf8(wide) == "\xF0\x9F\x98\x80");

    // Invalid conversion (should throw an exception)
    std::wstring invalid_wide;
    invalid_wide.push_back(0xD800); // lone high surrogate
    REQUIRE_THROWS_AS(wide_to_utf8(invalid_wide), soci::soci_error);
}

TEST_CASE("UTF-16 to wide string conversion tests", "[unicode]")
{
    using namespace soci::details;

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
    REQUIRE_THROWS_AS(utf16_to_wide(invalid_utf16), soci::soci_error);
}

TEST_CASE("Wide string to UTF-16 conversion tests", "[unicode]")
{
    using namespace soci::details;

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
    REQUIRE_THROWS_AS(wide_to_utf16(invalid_wide), soci::soci_error);
}

int main(int argc, char** argv)
{

#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run assert()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

    if (argc >= 2)
    {
        connectString = argv[1];

        // Replace the connect string with the process name to ensure that
        // CATCH uses the correct name in its messages.
        argv[1] = argv[0];

        argc--;
        argv++;
    }
    else
    {
        std::cout << "usage: " << argv[0]
          << " connectstring [test-arguments...]\n"
            << "example: " << argv[0]
            << " \'connect_string_for_empty_backend\'\n";
        std::exit(1);
    }

    return Catch::Session().run(argc, argv);
}
