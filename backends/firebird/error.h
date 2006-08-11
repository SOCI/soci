//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef FIREBIRD_ERROR_H_INCLUDED
#define FIREBIRD_ERROR_H_INCLUDED

#include "soci-firebird.h"

namespace SOCI
{

    namespace details
    {

        namespace Firebird
        {

            void getISCErrorDetails(ISC_STATUS * status_vector, std::string &msg);

            bool checkISCError(ISC_STATUS const * status_vector, long errNum);

            void throwISCError(ISC_STATUS * status_vector);

        } // namespace Firebird

    } // namespace details

} // namespace SOCI

#endif // FIREBIRD_ERROR_H_INCLUDED
