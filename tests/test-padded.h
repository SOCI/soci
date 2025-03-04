//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton, Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TEST_PADDED_H_INCLUDED
#define SOCI_TEST_PADDED_H_INCLUDED

// Although SQL standard mandates right padding CHAR(N) values to their length
// with spaces, some backends don't confirm to it:
//
//  - Firebird does pad the string but to the byte-size (not character size) of
//  the column (i.e. CHAR(10) NONE is padded to 10 bytes but CHAR(10) UTF8 --
//  to 40).
//  - For MySql PAD_CHAR_TO_FULL_LENGTH option must be set, otherwise the value
//  is trimmed.
//  - SQLite never behaves correctly at all.
//
// This method will check result string from column defined as fixed char It
// will check only bytes up to the original string size. If padded string is
// bigger than expected string then all remaining chars must be spaces so if
// any non-space character is found it will fail.
inline void
checkEqualPadded(const std::string& padded_str, const std::string& expected_str)
{
    size_t const len = expected_str.length();
    std::string const start_str(padded_str, 0, len);

    if (start_str != expected_str)
    {
        throw soci::soci_error(
                "Expected string \"" + expected_str + "\" "
                "is different from the padded string \"" + padded_str + "\""
              );
    }

    if (padded_str.length() > len)
    {
        std::string const end_str(padded_str, len);
        if (end_str != std::string(padded_str.length() - len, ' '))
        {
            throw soci::soci_error(
                  "\"" + padded_str + "\" starts with \"" + padded_str +
                  "\" but non-space characater(s) are found aftewards"
                );
        }
    }
}

#define CHECK_EQUAL_PADDED(padded_str, expected_str) \
    CHECK_NOTHROW(checkEqualPadded(padded_str, expected_str));

#endif // SOCI_TEST_PADDED_H_INCLUDED
