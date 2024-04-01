#ifndef SOCI_PRIVATE_SOCI_UNICODE_H_INCLUDED
#define SOCI_PRIVATE_SOCI_UNICODE_H_INCLUDED

//#include <string>
//#include <system_error>
//#include <locale>
//#include <cwchar>

#include <string>
#include <stdexcept>


namespace soci
{

    namespace details
    {





        // Interface functions
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)


        inline std::wstring utf8_to_utf16(const std::string& utf8) {
            std::wstring utf16;
            for (size_t i = 0; i < utf8.size();) {
                uint32_t cp = 0;
                if ((utf8[i] & 0x80) == 0) {
                    cp = utf8[i++];
                }
                else if ((utf8[i] & 0xE0) == 0xC0) {
                    cp = (utf8[i++] & 0x1F) << 6;
                    cp |= (utf8[i++] & 0x3F);
                }
                else if ((utf8[i] & 0xF0) == 0xE0) {
                    cp = (utf8[i++] & 0x0F) << 12;
                    cp |= (utf8[i++] & 0x3F) << 6;
                    cp |= (utf8[i++] & 0x3F);
                }
                else if ((utf8[i] & 0xF8) == 0xF0) {
                    cp = (utf8[i++] & 0x07) << 18;
                    cp |= (utf8[i++] & 0x3F) << 12;
                    cp |= (utf8[i++] & 0x3F) << 6;
                    cp |= (utf8[i++] & 0x3F);
                }
                else {
                    throw std::runtime_error("Invalid UTF-8 encoding");
            }

                if (cp <= 0xFFFF) { // BMP character
                    utf16.push_back(static_cast<wchar_t>(cp));
                }
                else { // Supplementary character
                    cp -= 0x10000;
                    utf16.push_back(static_cast<wchar_t>((cp >> 10) + 0xD800));
                    utf16.push_back(static_cast<wchar_t>((cp & 0x3FF) + 0xDC00));
                }
        }

            return utf16;
    }

        inline std::string utf16_to_utf8(const std::wstring& utf16) {
            std::string utf8;
            for (size_t i = 0; i < utf16.size();) {
                uint32_t cp = utf16[i++];
                if ((cp >= 0xD800) && (cp <= 0xDBFF)) { // High surrogate
                    if (i < utf16.size()) {
                        uint32_t low = utf16[i++];
                        if (low >= 0xDC00 && low <= 0xDFFF) { // Low surrogate
                            cp = ((cp - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
                        }
                        else {
                            throw std::runtime_error("Invalid UTF-16 encoding");
                        }
                    }
                }

                if (cp < 0x80) {
                    utf8.push_back(static_cast<char>(cp));
                }
                else if (cp < 0x800) {
                    utf8.push_back(0xC0 | ((cp >> 6) & 0x1F));
                    utf8.push_back(0x80 | (cp & 0x3F));
                }
                else if (cp < 0x10000) {
                    utf8.push_back(0xE0 | ((cp >> 12) & 0x0F));
                    utf8.push_back(0x80 | ((cp >> 6) & 0x3F));
                    utf8.push_back(0x80 | (cp & 0x3F));
                }
                else if (cp < 0x110000) {
                    utf8.push_back(0xF0 | ((cp >> 18) & 0x07));
                    utf8.push_back(0x80 | ((cp >> 12) & 0x3F));
                    utf8.push_back(0x80 | ((cp >> 6) & 0x3F));
                    utf8.push_back(0x80 | (cp & 0x3F));
                }
                else {
                    throw std::runtime_error("Invalid code point");
                }
            }

            return utf8;
        }

        inline std::wstring utf8_to_wide(const std::string& utf8) {
            return utf8_to_utf16(utf8);
        }

        inline std::string wide_to_utf8(const std::wstring& wide) {
            return utf16_to_utf8(wide);
        }

#else // Unix/Linux and others

        inline std::wstring utf8_to_utf32(const std::string& utf8) {
            std::wstring utf32;
            for (size_t i = 0; i < utf8.size();) {
                uint32_t cp = 0;
                if ((utf8[i] & 0x80) == 0) { // 1-byte sequence
                    cp = utf8[i++];
                }
                else if ((utf8[i] & 0xE0) == 0xC0) { // 2-byte sequence
                    cp = (utf8[i++] & 0x1F) << 6;
                    cp |= (utf8[i++] & 0x3F);
                }
                else if ((utf8[i] & 0xF0) == 0xE0) { // 3-byte sequence
                    cp = (utf8[i++] & 0x0F) << 12;
                    cp |= (utf8[i++] & 0x3F) << 6;
                    cp |= (utf8[i++] & 0x3F);
                }
                else if ((utf8[i] & 0xF8) == 0xF0) { // 4-byte sequence
                    cp = (utf8[i++] & 0x07) << 18;
                    cp |= (utf8[i++] & 0x3F) << 12;
                    cp |= (utf8[i++] & 0x3F) << 6;
                    cp |= (utf8[i++] & 0x3F);
                }
                else {
                    throw std::runtime_error("Invalid UTF-8 encoding");
                }

                utf32.push_back(cp);
            }

            return utf32;
        }

        inline std::string utf32_to_utf8(const std::wstring& utf32) {
            std::string utf8;
            for (uint32_t cp : utf32) {
                if (cp < 0x80) { // 1-byte sequence
                    utf8.push_back(static_cast<char>(cp));
                }
                else if (cp < 0x800) { // 2-byte sequence
                    utf8.push_back(0xC0 | ((cp >> 6) & 0x1F));
                    utf8.push_back(0x80 | (cp & 0x3F));
                }
                else if (cp < 0x10000) { // 3-byte sequence
                    utf8.push_back(0xE0 | ((cp >> 12) & 0x0F));
                    utf8.push_back(0x80 | ((cp >> 6) & 0x3F));
                    utf8.push_back(0x80 | (cp & 0x3F));
                }
                else if (cp < 0x110000) { // 4-byte sequence
                    utf8.push_back(0xF0 | ((cp >> 18) & 0x07));
                    utf8.push_back(0x80 | ((cp >> 12) & 0x3F));
                    utf8.push_back(0x80 | ((cp >> 6) & 0x3F));
                    utf8.push_back(0x80 | (cp & 0x3F));
                }
                else {
                    throw std::runtime_error("Invalid UTF-32 code point");
                }
            }

            return utf8;
        }

        inline std::wstring utf8_to_wide(const std::string& utf8) {
            return utf8_to_utf32(utf8);
        }

        inline std::string wide_to_utf8(const std::wstring& wide) {
            return utf32_to_utf8(wide);
        }

#endif 






    } // namespace details

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_UNICODE_H_INCLUDED
