#include "soci/odbc/common.h"

namespace soci
{
    namespace details
    {
        namespace odbc
        {

            char* soci_strncpy_safe(char* dest, const char* src, std::size_t n)
            {
                if (n == 0) return dest;
                std::size_t i = 0;
                for (; i < n - 1 && src[i] != '\0'; ++i)
                {
                    dest[i] = src[i];
                }
                dest[i] = '\0';
                return dest;
            }

        } // namespace odbc
    } // namespace details
} // namespace soci
