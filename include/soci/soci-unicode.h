#ifndef SOCI_UNICODE_H_INCLUDED
#define SOCI_UNICODE_H_INCLUDED

#include "soci/error.h"

#include <string>

#include <wchar.h>

// Define SOCI_WCHAR_T_IS_UTF32 if wchar_t is wider than 16 bits (e.g., on Unix/Linux)
#if WCHAR_MAX > 0xFFFFu
#define SOCI_WCHAR_T_IS_UTF32
#endif

namespace soci
{

namespace details
{

#if defined(SOCI_WCHAR_T_IS_UTF32)
static_assert(sizeof(wchar_t) == sizeof(char32_t), "wchar_t must be 32 bits");

inline char32_t const* wide_to_char_type(std::wstring const& ws)
{
    return reinterpret_cast<char32_t const*>(ws.data());
}
#else
static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t must be 16 bits");

inline char16_t const* wide_to_char_type(std::wstring const& ws)
{
    return reinterpret_cast<char16_t const*>(ws.data());
}
#endif

inline void throw_if_too_small(std::size_t required, std::size_t available)
{
    if (required > available)
        throw soci_error("Output buffer is too small");
}

/**
    Check if the given string is a valid UTF-8 encoded string.

    Throws soci_error if the string is not a valid UTF-8 string.

    @param utf8 The string of length @a len.
    @param len The length of the string.
 */
void SOCI_DECL ensure_valid_utf8(char const* utf8, std::size_t len);

/// @overload
inline void ensure_valid_utf8(std::string const& utf8)
{
    ensure_valid_utf8(utf8.data(), utf8.size());
}

/**
    Check if a given string is a valid UTF-16 encoded string.

    Throws soci_error if the string is not a valid UTF-16 string.

    @param s The UTF-16 string to check.
    @param len The length of the string in characters.
 */
void SOCI_DECL ensure_valid_utf16(char16_t const* s, std::size_t len);

/// @overload
inline void ensure_valid_utf16(std::u16string const& utf16)
{
    ensure_valid_utf16(utf16.data(), utf16.size());
}

/**
    Check if a given string is a valid UTF-32 encoded string.

    Throws soci_error if the string is not a valid UTF-32 string.

    @param utf32 The input UTF-32 string.
    @return True if the input string is valid, false otherwise.
 */
void SOCI_DECL ensure_valid_utf32(char32_t const* s, std::size_t len);

/// @overload
inline void ensure_valid_utf32(std::u32string const& utf32)
{
    ensure_valid_utf32(utf32.data(), utf32.size());
}

/**
    Convert a UTF-8 encoded string to a UTF-16 encoded string.

    The input string must be a valid UTF-8 encoded string of the given length
    (not necessarily NUL-terminated). The output buffer must either contain
    enough space to store @a len16 characters or be @c nullptr to just compute
    the length required for conversion (in which case @a len16 is ignored).

    @param utf8 The input UTF-8 encoded string.
    @param len8 The length of the input string.
    @param out16 The output buffer or nullptr to just compute the required
        length.
    @param len16 The length of the output buffer if it is non-null.
    @return The length of the UTF-16 output.
    @throws soci_error if the input string contains invalid UTF-8 encoding or
        if the required length is greater than @a len16 when @a out16 is not @c
        nullptr.
 */
std::size_t SOCI_DECL
utf8_to_utf16(char const* utf8, std::size_t len8,
              char16_t* out16, std::size_t len16);

/// @overload
inline std::u16string utf8_to_utf16(char const* s, std::size_t len)
{
    auto const len16 = utf8_to_utf16(s, len, nullptr, 0);
    std::u16string utf16(len16, u'\0');
    utf8_to_utf16(s, len, const_cast<char16_t*>(utf16.data()), len16);
    return utf16;
}

/// @overload
inline std::u16string utf8_to_utf16(std::string const& utf8)
{
    return utf8_to_utf16(utf8.data(), utf8.size());
}

/// @overload
inline std::u16string utf8_to_utf16(char const* s)
{
    return utf8_to_utf16(s, std::char_traits<char>::length(s));
}

/**
    Convert a UTF-16 encoded string to a UTF-8 encoded string.

    Semantics of this function are the same as for utf8_to_utf16(), see its
    documentation for more details.
 */
std::size_t SOCI_DECL
utf16_to_utf8(char16_t const* utf16, std::size_t len16,
              char* out8, std::size_t len8);

/// @overload
inline std::string utf16_to_utf8(char16_t const* s, std::size_t len)
{
    auto const len8 = utf16_to_utf8(s, len, nullptr, 0);
    std::string utf8(len8, '\0');
    utf16_to_utf8(s, len, const_cast<char*>(utf8.data()), len8);
    return utf8;
}

/// @overload
inline std::string utf16_to_utf8(std::u16string const& utf16)
{
    return utf16_to_utf8(utf16.data(), utf16.size());
}

/// @overload
inline std::string utf16_to_utf8(char16_t const* s)
{
    return utf16_to_utf8(s, std::char_traits<char16_t>::length(s));
}

/**
    Convert a UTF-16 encoded string to a UTF-32 encoded string.

    Semantics of this function are the same as for utf8_to_utf16(), see its
    documentation for more details.
 */
std::size_t SOCI_DECL
utf16_to_utf32(char16_t const* utf16, std::size_t len16,
               char32_t* out32, std::size_t len32);

/// @overload
inline std::u32string utf16_to_utf32(char16_t const* s, std::size_t len)
{
    auto const len32 = utf16_to_utf32(s, len, nullptr, 0);
    std::u32string utf32(len32, U'\0');
    utf16_to_utf32(s, len, const_cast<char32_t*>(utf32.data()), len32);
    return utf32;
}

/// @overload
inline std::u32string utf16_to_utf32(std::u16string const& utf16)
{
    return utf16_to_utf32(utf16.data(), utf16.size());
}

/// @overload
inline std::u32string utf16_to_utf32(char16_t const* s)
{
    return utf16_to_utf32(s, std::char_traits<char16_t>::length(s));
}


/**
    Convert a UTF-32 encoded string to a UTF-16 encoded string.

    Semantics of this function are the same as for utf8_to_utf16(), see its
    documentation for more details.
 */
std::size_t SOCI_DECL
utf32_to_utf16(char32_t const* utf32, std::size_t len32,
               char16_t* out16, std::size_t len16);

/// @overload
inline std::u16string utf32_to_utf16(char32_t const* utf32, std::size_t len)
{
    auto const len16 = utf32_to_utf16(utf32, len, nullptr, 0);
    std::u16string utf16(len16, u'\0');
    utf32_to_utf16(utf32, len, const_cast<char16_t*>(utf16.data()), len16);
    return utf16;
}

/// @overload
inline std::u16string utf32_to_utf16(std::u32string const& utf32)
{
    return utf32_to_utf16(utf32.data(), utf32.size());
}

/// @overload
inline std::u16string utf32_to_utf16(char32_t const* s)
{
    return utf32_to_utf16(s, std::char_traits<char32_t>::length(s));
}

/**
    Convert a UTF-8 encoded string to a UTF-32 encoded string.

    Semantics of this function are the same as for utf8_to_utf16(), see its
    documentation for more details.
 */
std::size_t SOCI_DECL
utf8_to_utf32(char const* utf8, std::size_t len8,
              char32_t* out32, std::size_t len32);

/// @overload
inline std::u32string utf8_to_utf32(char const* utf8, std::size_t len)
{
    auto const len32 = utf8_to_utf32(utf8, len, nullptr, 0);
    std::u32string utf32(len32, U'\0');
    utf8_to_utf32(utf8, len, const_cast<char32_t*>(utf32.data()), len32);
    return utf32;
}

/// @overload
inline std::u32string utf8_to_utf32(std::string const& utf8)
{
    return utf8_to_utf32(utf8.data(), utf8.size());
}

/// @overload
inline std::u32string utf8_to_utf32(char const* s)
{
    return utf8_to_utf32(s, std::char_traits<char>::length(s));
}

/**
    Convert a UTF-32 encoded string to a UTF-8 encoded string.

    Semantics of this function are the same as for utf8_to_utf16(), see its
    documentation for more details.
 */
std::size_t SOCI_DECL
utf32_to_utf8(char32_t const* utf32, std::size_t len32,
              char* out8, std::size_t len8);

/// @overload
inline std::string utf32_to_utf8(char32_t const* s, std::size_t len)
{
    auto const len8 = utf32_to_utf8(s, len, nullptr, 0);
    std::string utf8(len8, '\0');
    utf32_to_utf8(s, len, const_cast<char*>(utf8.data()), len8);
    return utf8;
}

/// @overload
inline std::string utf32_to_utf8(std::u32string const& utf32)
{
    return utf32_to_utf8(utf32.data(), utf32.size());
}

/// @overload
inline std::string utf32_to_utf8(char32_t const* s)
{
    return utf32_to_utf8(s, std::char_traits<char32_t>::length(s));
}

/**
    Convert a UTF-8 encoded string to a wide string (wstring).

    This is equivalent to either utf8_to_utf32() or utf8_to_utf16() depending
    on the platform.

    @param utf8 The input UTF-8 encoded string.
    @return The wide string.
 */
inline std::wstring utf8_to_wide(char const* s, std::size_t len)
{
#if defined(SOCI_WCHAR_T_IS_UTF32)
    auto const wlen = utf8_to_utf32(s, len, nullptr, 0);
    std::wstring ws(wlen, u'\0');
    utf8_to_utf32(s, len, const_cast<char32_t*>(wide_to_char_type(ws)), wlen);
    return ws;
#else // !SOCI_WCHAR_T_IS_UTF32
    auto const wlen = utf8_to_utf16(s, len, nullptr, 0);
    std::wstring ws(wlen, u'\0');
    utf8_to_utf16(s, len, const_cast<char16_t*>(wide_to_char_type(ws)), wlen);
    return ws;
#endif // SOCI_WCHAR_T_IS_UTF32
}

/// @overload
inline std::wstring utf8_to_wide(std::string const& utf8)
{
    return utf8_to_wide(utf8.data(), utf8.size());
}

/**
    Convert a wide string (wstring) to a UTF-8 encoded string.

    This is equivalent to either utf32_to_utf8() or utf16_to_utf8() depending
    on the platform.

    @param ws The wide string.
    @return std::string The UTF-8 encoded string.
 */
inline std::string wide_to_utf8(std::wstring const& ws)
{
#if defined(SOCI_WCHAR_T_IS_UTF32)
    return utf32_to_utf8(wide_to_char_type(ws), ws.size());
#else // !SOCI_WCHAR_T_IS_UTF32
    return utf16_to_utf8(wide_to_char_type(ws), ws.size());
#endif // SOCI_WCHAR_T_IS_UTF32
}

/**
    Convert a UTF-16 encoded string to a wide string (wstring).

    This is equivalent to either utf16_to_utf32() or direct copy depending on
    the platform.

    @param s The UTF-16 encoded string.
    @param len The length of the input string.
    @return The wide string.
    @throws soci_error if the input string contains invalid UTF-16 encoding.
 */
inline std::wstring utf16_to_wide(char16_t const* s, std::size_t len)
{
#if defined(SOCI_WCHAR_T_IS_UTF32)
    // Convert UTF-16 to UTF-32 which is used by wstring.
    auto const wlen = utf16_to_utf32(s, len, nullptr, 0);
    std::wstring ws(wlen, L'\0');
    utf16_to_utf32(s, len,
                   const_cast<char32_t*>(wide_to_char_type(ws)), wlen);
    return ws;
#else // !SOCI_WCHAR_T_IS_UTF32
    // Perform validation even though it's already UTF-16
    ensure_valid_utf16(s, len);
    wchar_t const* ws = reinterpret_cast<wchar_t const*>(s);
    return std::wstring(ws, ws + len);
#endif // SOCI_WCHAR_T_IS_UTF32
}

/// @overload
inline std::wstring utf16_to_wide(char16_t const* s)
{
    return utf16_to_wide(s, std::char_traits<char16_t>::length(s));
}

/// @overload
inline std::wstring utf16_to_wide(std::u16string const& utf16)
{
    return utf16_to_wide(utf16.data(), utf16.size());
}

/**
    Convert a wide string (wstring) to a UTF-16 encoded string.

    This is equivalent to either utf32_to_utf16() or direct copy depending on
    the platform.

    @param ws The wide string.
    @param out The output buffer or nullptr to just compute the required length.
    @param len The output buffer length in characters (ignored if @a out is @c
        nullptr).
    @return The length of the UTF-16 output.
    @throws soci_error if the input string contains invalid wide characters or
        if the output buffer is too small when @a out is not @c nullptr.
 */
inline
std::size_t wide_to_utf16(std::wstring const& ws, char16_t* out, std::size_t len)
{
#if defined(SOCI_WCHAR_T_IS_UTF32)
    // Convert UTF-32 string to UTF-16.
    return utf32_to_utf16(wide_to_char_type(ws), ws.length(), out, len);
#else // !SOCI_WCHAR_T_IS_UTF32
    // It's already in UTF-16, just copy, but check that it's valid and that we
    // have enough space -- or just return the length if not asked to copy.
    auto const wlen = ws.length();
    if (out)
    {
        throw_if_too_small(wlen, len);

        ensure_valid_utf16(wide_to_char_type(ws), wlen);
        std::memcpy(out, ws.data(), wlen * sizeof(wchar_t));
    }
    return wlen;
#endif // SOCI_WCHAR_T_IS_UTF32
}

/// @overload
inline std::u16string wide_to_utf16(std::wstring const& ws)
{
    auto const wlen = wide_to_utf16(ws, nullptr, 0);
    std::u16string utf16(wlen, u'\0');
    wide_to_utf16(ws, const_cast<char16_t*>(utf16.data()), wlen);
    return utf16;
}

} // namespace details

} // namespace soci

#endif // SOCI_UNICODE_H_INCLUDED
