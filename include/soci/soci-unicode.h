#ifndef SOCI_UNICODE_H_INCLUDED
#define SOCI_UNICODE_H_INCLUDED

#include "soci/error.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <wchar.h>

// Define SOCI_WCHAR_T_IS_WIDE if wchar_t is wider than 16 bits (e.g., on Unix/Linux)
#if WCHAR_MAX > 0xFFFFu
#define SOCI_WCHAR_T_IS_WIDE
#endif

namespace soci
{
  namespace details
  {

    /**
     * Helper function to check if a UTF-8 sequence is valid.
     *
     * This function takes a byte sequence and its length as input and checks if the sequence is a valid UTF-8 encoded character.
     *
     * @param bytes Pointer to the byte sequence to be checked.
     * @param length Length of the byte sequence.
     * @return True if the sequence is a valid UTF-8 encoded character, false otherwise.
     */
    constexpr inline bool is_valid_utf8_sequence(const unsigned char *bytes, int length)
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

    // Check if a UTF-8 string is valid
    /**
     * This function checks if the given string is a valid UTF-8 encoded string.
     * It iterates over each byte in the string and checks if it is a valid start of a UTF-8 character.
     * If it is, it checks if the following bytes form a valid UTF-8 character sequence.
     * If the string is not a valid UTF-8 string, the function returns false.
     * If the string is a valid UTF-8 string, the function returns true.
     *
     * @param utf8 The string to check for valid UTF-8 encoding.
     * @return True if the string is a valid UTF-8 encoded string, false otherwise.
     */
    inline void is_valid_utf8(const std::string &utf8)
    {
      const auto *bytes = reinterpret_cast<const unsigned char *>(utf8.data()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const std::size_t length = utf8.length();

      for (std::size_t i = 0; i < length;)
      {
        if ((bytes[i] & 0x80U) == 0)
        {
          // ASCII character, one byte
          i += 1;
        }
        else if ((bytes[i] & 0xE0U) == 0xC0)
        {
          // Two-byte character, check if the next byte is a valid continuation byte
          if (i + 1 >= length || !is_valid_utf8_sequence(bytes + i, 2))
          {
            throw soci_error("Invalid UTF-8 sequence: Truncated or invalid two-byte sequence");
          }
          i += 2;
        }
        else if ((bytes[i] & 0xF0U) == 0xE0U)
        {
          // Three-byte character, check if the next two bytes are valid continuation bytes
          if (i + 2 >= length || !is_valid_utf8_sequence(bytes + i, 3))
          {
            throw soci_error("Invalid UTF-8 sequence: Truncated or invalid three-byte sequence");
          }
          i += 3;
        }
        else if ((bytes[i] & 0xF8U) == 0xF0U)
        {
          // Four-byte character, check if the next three bytes are valid continuation bytes
          if (i + 3 >= length || !is_valid_utf8_sequence(bytes + i, 4))
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

    /**
     * Check if a given UTF-16 string is valid.
     *
     * A UTF-16 string is considered valid if it follows the UTF-16 encoding rules.
     * This means that all code units in the range 0xD800 to 0xDBFF must be followed
     * by a code unit in the range 0xDC00 to 0xDFFF. Conversely, code units in the
     * range 0xDC00 to 0xDFFF must not appear without a preceding code unit in the
     * range 0xD800 to 0xDBFF.
     *
     * @param utf16 The UTF-16 string to check.
     * @return True if the string is valid, false otherwise.
     */
    inline void is_valid_utf16(const std::u16string &utf16)
    {
      const char16_t *chars = utf16.data();
      const std::size_t length = utf16.length();

      for (std::size_t i = 0; i < length; ++i)
      {
        const char16_t chr = chars[i];
        if (chr >= 0xD800 && chr <= 0xDBFF)
        { // High surrogate
          if (i + 1 >= length)
          {
            throw soci_error("Invalid UTF-16 sequence (truncated surrogate pair)");
          }
          const char16_t next = chars[i + 1];
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

    /**
     * @brief Check if a given UTF-32 string is valid.
     *
     * This function checks whether all code points in the input
     * UTF-32 string are within the Unicode range (0x0 to 0x10FFFF) and
     * do not fall into the surrogate pair range (0xD800 to 0xDFFF).
     *
     * @param utf32 The input UTF-32 string.
     * @return True if the input string is valid, false otherwise.
     */
    inline void is_valid_utf32(const std::u32string &utf32)
    {
      const char32_t *chars = utf32.data();
      const std::size_t length = utf32.length();

      for (std::size_t i = 0; i < length; ++i)
      {
        const char32_t chr = chars[i];

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

        // Check for non-characters
        if ((chr >= 0xFDD0 && chr <= 0xFDEF) || (chr & 0xFFFF) == 0xFFFE)
        {
          throw soci_error("Invalid UTF-32 sequence: Non-character found");
        }
      }
    }

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
     *
     * This function iterates through the input string and converts each UTF-8 sequence into
     * its corresponding UTF-16 code unit(s).
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    inline std::u16string utf8_to_utf16(const std::string &utf8)
    {
      // Ensure the input string is valid UTF-8
      is_valid_utf8(utf8);

      std::u16string utf16;
      const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
      size_t length = utf8.length();

      for (size_t i = 0; i < length;)
      {
        if ((bytes[i] & 0x80U) == 0)
        {
          // ASCII character, one byte
          utf16.push_back(static_cast<char16_t>(bytes[i]));
          i += 1;
        }
        else if ((bytes[i] & 0xE0U) == 0xC0U)
        {
          // Two-byte character
          utf16.push_back(static_cast<char16_t>(((bytes[i] & 0x1FU) << 6U) | (bytes[i + 1] & 0x3FU)));
          i += 2;
        }
        else if ((bytes[i] & 0xF0U) == 0xE0U)
        {
          // Three-byte character
          utf16.push_back(static_cast<char16_t>(((bytes[i] & 0x0FU) << 12U) | ((bytes[i + 1] & 0x3FU) << 6U) | (bytes[i + 2] & 0x3FU)));
          i += 3;
        }
        else if ((bytes[i] & 0xF8U) == 0xF0U)
        {
          // Four-byte character
          uint32_t codepoint = (static_cast<uint32_t>(bytes[i] & 0x07U) << 18U) |
                               (static_cast<uint32_t>(bytes[i + 1] & 0x3FU) << 12U) |
                               (static_cast<uint32_t>(bytes[i + 2] & 0x3FU) << 6U) |
                               (static_cast<uint32_t>(bytes[i + 3] & 0x3FU));

          if (codepoint <= 0xFFFFU)
          {
            utf16.push_back(static_cast<char16_t>(codepoint));
          }
          else
          {
            // Encode as a surrogate pair
            codepoint -= 0x10000;
            utf16.push_back(static_cast<char16_t>((codepoint >> 10U) + 0xD800U));
            utf16.push_back(static_cast<char16_t>((codepoint & 0x3FFU) + 0xDC00U));
          }
          i += 4;
        }
        else
        {
          // This should never happen if is_valid_utf8 did its job
          throw soci_error("Invalid UTF-8 sequence detected after validation");
        }
      }

      return utf16;
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
     *
     * This function iterates through the input string and converts each UTF-16 code unit into
     * its corresponding UTF-8 sequence(s).
     * If the input string contains invalid UTF-16 encoding, a soci_error exception is thrown.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    inline std::string utf16_to_utf8(const std::u16string &utf16)
    {
      // Ensure the input is valid UTF-16
      is_valid_utf16(utf16);

      std::string utf8;
      utf8.reserve(utf16.size() * 4); // Allocate enough space to avoid reallocations

      for (std::size_t i = 0; i < utf16.length(); ++i)
      {
        const char16_t chr = utf16[i];

        if (chr < 0x80)
        {
          // 1-byte sequence (ASCII)
          utf8.push_back(static_cast<char>(chr));
        }
        else if (chr < 0x800)
        {
          // 2-byte sequence
          utf8.push_back(static_cast<char>(0xC0U | ((static_cast<unsigned int>(chr) >> 6U) & 0x1FU)));
          utf8.push_back(static_cast<char>(0x80U | (static_cast<unsigned int>(chr) & 0x3FU)));
        }
        else if ((chr >= 0xD800U) && (chr <= 0xDBFFU))
        {
          // Handle UTF-16 surrogate pairs

          const char16_t chr2 = utf16[i + 1];
          const auto codepoint = static_cast<uint32_t>(((chr & 0x3FFU) << 10U) | (chr2 & 0x3FFU)) + 0x10000U;

          utf8.push_back(static_cast<char>(0xF0U | ((codepoint >> 18U) & 0x07U)));
          utf8.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
          utf8.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
          utf8.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
          ++i; // Skip the next character as it is part of the surrogate pair
        }
        else
        {
          // 3-byte sequence
          utf8.push_back(static_cast<char>(0xE0U | ((static_cast<unsigned int>(chr) >> 12U) & 0x0FU)));
          utf8.push_back(static_cast<char>(0x80U | ((static_cast<unsigned int>(chr) >> 6U) & 0x3FU)));
          utf8.push_back(static_cast<char>(0x80U | (static_cast<unsigned int>(chr) & 0x3FU)));
        }
      }

      return utf8;
    }

    /**
     * @brief Converts a UTF-16 encoded string to a UTF-32 encoded string.
     *
     * This function iterates through the input string and converts each UTF-16 code unit into
     * its corresponding UTF-32 code point(s).
     * If the input string contains invalid UTF-16 encoding, a soci_error exception is thrown.
     *
     * @param utf16 The UTF-16 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-16 encoding.
     */
    inline std::u32string utf16_to_utf32(const std::u16string &utf16)
    {
      // Ensure that the input string is valid UTF-16
      is_valid_utf16(utf16);

      std::u32string utf32;
      for (std::size_t i = 0; i < utf16.length(); ++i)
      {
        const char16_t chr = utf16[i];
        if (chr >= 0xD800U && chr <= 0xDBFFU)
        {
          // High surrogate, must be followed by a low surrogate
          const char16_t chr2 = utf16[++i]; // Directly increment i here

          const auto codepoint = static_cast<uint32_t>(((static_cast<unsigned int>(chr) & 0x3FFU) << 10U) | (static_cast<unsigned int>(chr2) & 0x3FFU)) + 0x10000U;
          utf32.push_back(codepoint);
        }
        else
        {
          // Valid BMP character or a low surrogate that is part of a valid pair (already checked by is_valid_utf16)
          utf32.push_back(static_cast<char32_t>(chr));
        }
      }
      return utf32;
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-16 encoded string.
     *
     * This function iterates through the input string and converts each UTF-32 code point into
     * its corresponding UTF-16 code unit(s).
     * If the input string contains invalid UTF-32 code points, a soci_error exception is thrown.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::u16string The UTF-16 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    inline std::u16string utf32_to_utf16(const std::u32string &utf32)
    {
      // Ensure that the input UTF-32 string is valid
      is_valid_utf32(utf32);

      std::u16string utf16;
      utf16.reserve(utf32.size()); // Reserve space to avoid reallocations

      for (char32_t codepoint : utf32)
      {
        if (codepoint <= 0xFFFFU)
        {
          // BMP character
          utf16.push_back(static_cast<char16_t>(codepoint));
        }
        else if (codepoint <= 0x10FFFFU)
        {
          // Encode as a surrogate pair
          codepoint -= 0x10000;
          utf16.push_back(static_cast<char16_t>((codepoint >> 10U) + 0xD800U));
          utf16.push_back(static_cast<char16_t>((codepoint & 0x3FFU) + 0xDC00U));
        }
        else
        {
          // Invalid Unicode range - This should never happen as is_valid_utf32 already checks this
          throw soci_error("Invalid UTF-32 code point: out of Unicode range");
        }
      }

      return utf16; // Return the constructed string
    }

    /**
     * @brief Converts a UTF-8 encoded string to a UTF-32 encoded string.
     *
     * This function iterates through the input string and converts each UTF-8 sequence into
     * its corresponding UTF-32 code point(s).
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
     *
     * @param utf8 The UTF-8 encoded string.
     * @return std::u32string The UTF-32 encoded string.
     * @throws soci_error if the input string contains invalid UTF-8 encoding.
     */
    inline std::u32string utf8_to_utf32(const std::string &utf8)
    {
      // Ensure the input string is valid UTF-8
      is_valid_utf8(utf8);

      std::u32string utf32;
      const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
      std::size_t length = utf8.length();

      for (std::size_t i = 0; i < length;)
      {
        unsigned char chr1 = bytes[i];

        // 1-byte sequence (ASCII)
        if ((chr1 & 0x80U) == 0)
        {
          utf32.push_back(static_cast<char32_t>(chr1));
          ++i;
        }
        // 2-byte sequence
        else if ((chr1 & 0xE0U) == 0xC0U)
        {
          utf32.push_back(static_cast<char32_t>(((chr1 & 0x1FU) << 6U) | (bytes[i + 1] & 0x3FU)));
          i += 2;
        }
        // 3-byte sequence
        else if ((chr1 & 0xF0U) == 0xE0U)
        {
          utf32.push_back(static_cast<char32_t>(((chr1 & 0x0FU) << 12U) | ((bytes[i + 1] & 0x3FU) << 6U) | (bytes[i + 2] & 0x3FU)));
          i += 3;
        }
        // 4-byte sequence
        else if ((chr1 & 0xF8U) == 0xF0U)
        {
          utf32.push_back(static_cast<char32_t>(((chr1 & 0x07U) << 18U) | ((bytes[i + 1] & 0x3FU) << 12U) | ((bytes[i + 2] & 0x3FU) << 6U) | (bytes[i + 3] & 0x3FU)));
          i += 4;
        }
      }

      return utf32;
    }

    /**
     * @brief Converts a UTF-32 encoded string to a UTF-8 encoded string.
     *
     * This function iterates through the input string and converts each UTF-32 code point into
     * its corresponding UTF-8 sequence(s).
     * If the input string contains invalid UTF-32 code points, a soci_error exception is thrown.
     *
     * @param utf32 The UTF-32 encoded string.
     * @return std::string The UTF-8 encoded string.
     * @throws soci_error if the input string contains invalid UTF-32 code points.
     */
    inline std::string utf32_to_utf8(const std::u32string &utf32)
    {
      // Ensure the input string is valid UTF-32
      is_valid_utf32(utf32); // Validate the input UTF-32 string

      std::string utf8;
      utf8.reserve(utf32.length() * 4); // Preallocate memory for potential worst-case scenario (all 4-byte sequences)

      for (char32_t codepoint : utf32)
      {
        if (codepoint < 0x80)
        {
          // 1-byte sequence (ASCII)
          utf8.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint < 0x800)
        {
          // 2-byte sequence
          utf8.push_back(static_cast<char>(0xC0U | ((codepoint >> 6U) & 0x1FU)));
          utf8.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
        else if (codepoint < 0x10000)
        {
          // 3-byte sequence
          utf8.push_back(static_cast<char>(0xE0U | ((codepoint >> 12U) & 0x0FU)));
          utf8.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
          utf8.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
        else if (codepoint <= 0x10FFFF)
        {
          // 4-byte sequence
          utf8.push_back(static_cast<char>(0xF0U | ((codepoint >> 18U) & 0x07U)));
          utf8.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
          utf8.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
          utf8.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
        else
        {
          // This should never happen as is_valid_utf32 already checks this
          throw soci_error("Invalid UTF-32 code point");
        }
      }

      return utf8;
    }

    /**
     * @brief Converts a UTF-8 encoded string to a wide string (wstring).
     *
     * This function uses the platform's native wide character encoding. On Windows, this is UTF-16,
     * while on Unix/Linux and other platforms, it is UTF-32 or UTF-8 depending on the system
     * configuration.
     * If the input string contains invalid UTF-8 encoding, a soci_error exception is thrown.
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
     * This function uses the platform's native wide character encoding. On Windows, this is UTF-16,
     * while on Unix/Linux and other platforms, it is UTF-32 or UTF-8 depending on the system
     * configuration.
     * If the input string contains invalid wide characters, a soci_error exception is thrown.
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
