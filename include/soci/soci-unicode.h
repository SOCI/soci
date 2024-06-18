#ifndef SOCI_UNICODE_H_INCLUDED
#define SOCI_UNICODE_H_INCLUDED

#include <stdexcept>
#include <wchar.h>
#include <string>
#include "soci/error.h"


#if WCHAR_MAX > 0xFFFFu
  #define SOCI_WCHAR_T_IS_WIDE
#endif


namespace soci
{
  namespace details
  {

    inline std::u16string utf8_to_utf16(const std::string &utf8)
    {
      std::u16string utf16;
      for (size_t i = 0; i < utf8.size();)
      {
        uint32_t cp = 0;
        if ((utf8[i] & 0x80) == 0)
        {
          cp = utf8[i++];
        }
        else if ((utf8[i] & 0xE0) == 0xC0)
        {
          cp = (utf8[i++] & 0x1F) << 6;
          cp |= (utf8[i++] & 0x3F);
        }
        else if ((utf8[i] & 0xF0) == 0xE0)
        {
          cp = (utf8[i++] & 0x0F) << 12;
          cp |= (utf8[i++] & 0x3F) << 6;
          cp |= (utf8[i++] & 0x3F);
        }
        else if ((utf8[i] & 0xF8) == 0xF0)
        {
          cp = (utf8[i++] & 0x07) << 18;
          cp |= (utf8[i++] & 0x3F) << 12;
          cp |= (utf8[i++] & 0x3F) << 6;
          cp |= (utf8[i++] & 0x3F);
        }
        else
        {
          throw soci_error("Invalid UTF-8 encoding");
        }

        if (cp <= 0xFFFF)
        { // BMP character
          utf16.push_back(static_cast<char16_t>(cp));
        }
        else
        { // Supplementary character
          cp -= 0x10000;
          utf16.push_back(static_cast<char16_t>((cp >> 10) + 0xD800));
          utf16.push_back(static_cast<char16_t>((cp & 0x3FF) + 0xDC00));
        }
      }

      return utf16;
    }

    inline std::string utf16_to_utf8(const std::u16string &utf16)
    {
      std::string utf8;
      for (size_t i = 0; i < utf16.size();)
      {
        uint32_t cp = utf16[i++];
        if ((cp >= 0xD800) && (cp <= 0xDBFF))
        { // High surrogate
          if (i < utf16.size())
          {
            uint32_t low = utf16[i++];
            if (low >= 0xDC00 && low <= 0xDFFF)
            { // Low surrogate
              cp = ((cp - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
            }
            else
            {
              throw soci_error("Invalid UTF-16 encoding");
            }
          }
        }

        if (cp < 0x80)
        {
          utf8.push_back(static_cast<char>(cp));
        }
        else if (cp < 0x800)
        {
          utf8.push_back(0xC0 | ((cp >> 6) & 0x1F));
          utf8.push_back(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
          utf8.push_back(0xE0 | ((cp >> 12) & 0x0F));
          utf8.push_back(0x80 | ((cp >> 6) & 0x3F));
          utf8.push_back(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x110000)
        {
          utf8.push_back(0xF0 | ((cp >> 18) & 0x07));
          utf8.push_back(0x80 | ((cp >> 12) & 0x3F));
          utf8.push_back(0x80 | ((cp >> 6) & 0x3F));
          utf8.push_back(0x80 | (cp & 0x3F));
        }
        else
        {
          throw soci_error("Invalid code point");
        }
      }

      return utf8;
    }

    inline std::u32string utf16_to_utf32(const std::u16string &utf16)
    {
      std::u32string utf32;
      for (size_t i = 0; i < utf16.size();)
      {
        uint32_t cp = utf16[i++];
        if ((cp >= 0xD800) && (cp <= 0xDBFF))
        {
          if (i < utf16.size())
          {
            uint32_t low = utf16[i++];
            if ((low >= 0xDC00) && (low <= 0xDFFF))
            {
              cp = ((cp - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
            }
            else
            {
              throw soci_error("Invalid UTF-16 encoding");
            }
          }
        }
        utf32.push_back(cp);
      }
      return utf32;
    }

    inline std::u16string utf32_to_utf16(const std::u32string &utf32)
    {
      std::u16string utf16;
      for (uint32_t cp : utf32)
      {
        if (cp <= 0xFFFF)
        {
          utf16.push_back(static_cast<char16_t>(cp));
        }
        else if (cp <= 0x10FFFF)
        {
          cp -= 0x10000;
          utf16.push_back(static_cast<char16_t>((cp >> 10) + 0xD800));
          utf16.push_back(static_cast<char16_t>((cp & 0x3FF) + 0xDC00));
        }
        else
        {
          throw soci_error("Invalid UTF-32 code point");
        }
      }
      return utf16;
    }

    inline std::u32string utf8_to_utf32(const std::string &utf8)
    {
      std::u32string utf32;
      for (size_t i = 0; i < utf8.size();)
      {
        uint32_t cp = 0;
        if ((utf8[i] & 0x80) == 0)
        { // 1-byte sequence
          cp = utf8[i++];
        }
        else if ((utf8[i] & 0xE0) == 0xC0)
        { // 2-byte sequence
          cp = (utf8[i++] & 0x1F) << 6;
          cp |= (utf8[i++] & 0x3F);
        }
        else if ((utf8[i] & 0xF0) == 0xE0)
        { // 3-byte sequence
          cp = (utf8[i++] & 0x0F) << 12;
          cp |= (utf8[i++] & 0x3F) << 6;
          cp |= (utf8[i++] & 0x3F);
        }
        else if ((utf8[i] & 0xF8) == 0xF0)
        { // 4-byte sequence
          cp = (utf8[i++] & 0x07) << 18;
          cp |= (utf8[i++] & 0x3F) << 12;
          cp |= (utf8[i++] & 0x3F) << 6;
          cp |= (utf8[i++] & 0x3F);
        }
        else
        {
          throw soci_error("Invalid UTF-8 encoding");
        }

        utf32.push_back(cp);
      }

      return utf32;
    }

    inline std::string utf32_to_utf8(const std::u32string &utf32)
    {
      std::string utf8;
      for (uint32_t cp : utf32)
      {
        if (cp < 0x80)
        { // 1-byte sequence
          utf8.push_back(static_cast<char>(cp));
        }
        else if (cp < 0x800)
        { // 2-byte sequence
          utf8.push_back(0xC0 | ((cp >> 6) & 0x1F));
          utf8.push_back(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        { // 3-byte sequence
          utf8.push_back(0xE0 | ((cp >> 12) & 0x0F));
          utf8.push_back(0x80 | ((cp >> 6) & 0x3F));
          utf8.push_back(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x110000)
        { // 4-byte sequence
          utf8.push_back(0xF0 | ((cp >> 18) & 0x07));
          utf8.push_back(0x80 | ((cp >> 12) & 0x3F));
          utf8.push_back(0x80 | ((cp >> 6) & 0x3F));
          utf8.push_back(0x80 | (cp & 0x3F));
        }
        else
        {
          throw soci_error("Invalid UTF-32 code point");
        }
      }

      return utf8;
    }

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