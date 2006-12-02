//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"
#include "error.h"

namespace SOCI
{

    FirebirdSOCIError::FirebirdSOCIError(std::string const & msg, ISC_STATUS const * status)
            : SOCIError(msg)
    {
        if (status != 0)
        {
            std::size_t i = 0;
            while (i < stat_size && status[i] != 0)
            {
                status_.push_back(status[i++]);
            }
        }
    }

    namespace details
    {

        namespace Firebird
        {

            void getISCErrorDetails(ISC_STATUS * status_vector, std::string &msg)
            {
                char msg_buffer[SOCI_FIREBIRD_ERRMSG];
                long *pvector = status_vector;

                try
                {
                    // fetching first error message
                    isc_interprete(msg_buffer, &pvector);
                    msg = msg_buffer;

                    // fetching next errors
                    while (isc_interprete(msg_buffer, &pvector))
                    {
                        msg += "\n";
                        msg += msg_buffer;
                    }
                }
                catch (...)
                {
                    throw SOCIError("Exception catched while fetching error information");
                }
            }

            bool checkISCError(ISC_STATUS const * status_vector, long errNum)
            {
                std::size_t i=0;
                while (status_vector[i] != 0)
                {
                    if (status_vector[i] == 1 && status_vector[i+1] == errNum)
                    {
                        return true;
                    }
                    ++i;
                }

                return false;
            }
            void throwISCError(ISC_STATUS * status_vector)
            {
                std::string msg;

                getISCErrorDetails(status_vector, msg);
                throw FirebirdSOCIError(msg, status_vector);
            }

        } // namespace Firebird

    } // namespace details

} // namespace SOCI
