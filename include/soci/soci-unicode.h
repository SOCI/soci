#ifndef SOCI_UNICODE_H_INCLUDED
#define SOCI_UNICODE_H_INCLUDED

#include <stdexcept>
#include <wchar.h>
#include <string>
#include "soci/error.h"

// Define SOCI_WCHAR_T_IS_WIDE if wchar_t is wider than 16 bits (e.g., on Windows)
#if WCHAR_MAX > 0xFFFFu
  #define SOCI_WCHAR_T_IS_WIDE
#endif

namespace soci
{
  namespace details
  {
    /**
     * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    inline std::u16string utf8_to_utf16(const std::string &utf8)
    {
      std::u16string utf16;
      utf16.reserve(utf8.size());

      for (std::size_t i = 0; i < utf8.size();)
      {
        unsigned char c1 = static_cast<unsigned char>(utf8[i++]);

        if (c1 < 0x80)
        {
          utf16.push_back(static_cast<char16_t>(c1));
        }
        else if ((c1 & 0xE0) == 0xC0)
        {
          if (i >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          if ((c2 & 0xC0) != 0x80)
            throw soci_error("Invalid UTF-8 sequence");

          utf16.push_back(static_cast<char16_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F)));
        }
        else if ((c1 & 0xF0) == 0xE0)
        {
          if (i + 1 >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c3 = static_cast<unsigned char>(utf8[i++]);
          if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80))
            throw soci_error("Invalid UTF-8 sequence");

          utf16.push_back(static_cast<char16_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)));
        }
        else if ((c1 & 0xF8) == 0xF0)
        {
          if (i + 2 >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c3 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c4 = static_cast<unsigned char>(utf8[i++]);
          if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80) || ((c4 & 0xC0) != 0x80))
            throw soci_error("Invalid UTF-8 sequence");

          uint32_t codepoint = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
          if (codepoint <= 0xFFFF)
          {
            utf16.push_back(static_cast<char16_t>(codepoint));
          }
          else
          {
            codepoint -= 0x10000;
            utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
            utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
          }
        }
        else
        {
          throw soci_error("Invalid UTF-8 sequence");
        }
      }

      return utf16;
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    inline std::string utf16_to_utf8(const std::u16string &utf16)
    {
      std::string utf8;
      utf8.reserve(utf16.size() * 3);

      for (std::size_t i = 0; i < utf16.size();)
      {
        char16_t c = utf16[i++];

        if (c < 0x80)
        {
          utf8.push_back(static_cast<char>(c));
        }
        else if (c < 0x800)
        {
          utf8.push_back(static_cast<char>(0xC0 | ((c >> 6) & 0x1F)));
          utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
        else if ((c >= 0xD800) && (c <= 0xDBFF))
        {
          if (i >= utf16.size())
            throw soci_error("Invalid UTF-16 sequence");

          char16_t c2 = utf16[i++];
          if ((c2 < 0xDC00) || (c2 > 0xDFFF))
            throw soci_error("Invalid UTF-16 sequence");

          uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
          utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else
        {
          utf8.push_back(static_cast<char>(0xE0 | ((c >> 12) & 0x0F)));
          utf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
      }

      return utf8;
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-32 encoded string.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    inline std::u32string utf16_to_utf32(const std::u16string &utf16)
    {
      std::u32string utf32;
      utf32.reserve(utf16.size());

      for (std::size_t i = 0; i < utf16.size();)
      {
        char16_t c = utf16[i++];

        if ((c >= 0xD800) && (c <= 0xDBFF))
        {
          if (i >= utf16.size())
            throw soci_error("Invalid UTF-16 sequence");

          char16_t c2 = utf16[i++];
          if ((c2 < 0xDC00) || (c2 > 0xDFFF))
            throw soci_error("Invalid UTF-16 sequence");

          uint32_t codepoint = (((c & 0x3FF) << 10) | (c2 & 0x3FF)) + 0x10000;
          utf32.push_back(codepoint);
        }
        else
        {
          utf32.push_back(static_cast<char32_t>(c));
        }
      }

      return utf32;
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-16 encoded string.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    inline std::u16string utf32_to_utf16(const std::u32string &utf32)
    {
      std::u16string utf16;
      utf16.reserve(utf32.size() * 2);

      for (char32_t codepoint : utf32)
      {
        if (codepoint <= 0xFFFF)
        {
          utf16.push_back(static_cast<char16_t>(codepoint));
        }
        else if (codepoint <= 0x10FFFF)
        {
          codepoint -= 0x10000;
          utf16.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800));
          utf16.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00));
        }
        else
        {
          throw soci_error("Invalid UTF-32 code point");
        }
      }

      return utf16;
    }

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-32 encoded string.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    inline std::u32string utf8_to_utf32(const std::string &utf8)
    {
      std::u32string utf32;
      utf32.reserve(utf8.size());

      for (std::size_t i = 0; i < utf8.size();)
      {
        unsigned char c1 = static_cast<unsigned char>(utf8[i++]);

        if (c1 < 0x80)
        {
          utf32.push_back(static_cast<char32_t>(c1));
        }
        else if ((c1 & 0xE0) == 0xC0)
        {
          if (i >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          if ((c2 & 0xC0) != 0x80)
            throw soci_error("Invalid UTF-8 sequence");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F)));
        }
        else if ((c1 & 0xF0) == 0xE0)
        {
          if (i + 1 >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c3 = static_cast<unsigned char>(utf8[i++]);
          if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80))
            throw soci_error("Invalid UTF-8 sequence");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)));
        }
        else if ((c1 & 0xF8) == 0xF0)
        {
          if (i + 2 >= utf8.size())
            throw soci_error("Invalid UTF-8 sequence");

          unsigned char c2 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c3 = static_cast<unsigned char>(utf8[i++]);
          unsigned char c4 = static_cast<unsigned char>(utf8[i++]);
          if (((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80) || ((c4 & 0xC0) != 0x80))
            throw soci_error("Invalid UTF-8 sequence");

          utf32.push_back(static_cast<char32_t>(((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)));
        }
        else
        {
          throw soci_error("Invalid UTF-8 sequence");
        }
      }

      return utf32;
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-8 encoded string.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    inline std::string utf32_to_utf8(const std::u32string &utf32)
    {
      std::string utf8;
      utf8.reserve(utf32.size() * 4);

      for (char32_t codepoint : utf32)
      {
        if (codepoint < 0x80)
        {
          utf8.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint < 0x800)
        {
          utf8.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint < 0x10000)
        {
          utf8.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint <= 0x10FFFF)
        {
          utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else
        {
          throw soci_error("Invalid UTF-32 code point");
        }
      }

      return utf8;
    }
    
    /**
     * @brief Converts a UTF-8 encoded string to a wide string (wstring).
     * 
     * @param utf8 The UTF-8 encoded string.
     * @return std::wstring The wide string.
     */
    inline std::wstring utf8_to_wide(const std::string &utf8)
    {
#if defined(SOCI_WCHAR_T_IS_WIDE) // Windows
      // Convert UTF-8 to UTF-32 first and then to wstring (UTF-32 on Unix/Linux)
      std::u32string utf32 = utf8_to_utf32(utf8);
      return std::wstring(utf32.begin(), utf32.end());
#else  // Unix/Linux and others
      std::u16string utf16 = utf8_to_utf16(utf8);
      return std::wstring(utf16.begin(), utf16.end());
#endif // SOCI_WCHAR_T_IS_WIDE
    }

    /**
     * @brief Converts a wide string (wstring) to a UTF-8 encoded string.
     * 
     * @param wide The wide string.
     * @return std::string The UTF-8 encoded string.
     */
    inline std::string wide_to_utf8(const std::wstring &wide)
    {
#if defined(SOCI_WCHAR_T_IS_WIDE) // Windows
      // Convert wstring (UTF-32) to utf8
      std::u32string utf32(wide.begin(), wide.end());
      return utf32_to_utf8(utf32);
#else  // Unix/Linux and others
      std::u16string utf16(wide.begin(), wide.end());
      return utf16_to_utf8(utf16);
#endif // SOCI_WCHAR_T_IS_WIDE
    }

  } // namespace details

} // namespace soci

#endif // SOCI_UNICODE_H_INCLUDED