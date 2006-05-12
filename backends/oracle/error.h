//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ORACLE_COMMON_H_INCLUDED
#define ORACLE_COMMON_H_INCLUDED

#include "soci-oracle.h"

namespace SOCI {

namespace details {

namespace Oracle {

void throwOracleSOCIError(sword res, OCIError *errhp);

void getErrorDetails(sword res, OCIError *errhp,
    std::string &msg, int &errNum);

} // namespace Oracle 

} // namespace details

} // namespace SOCI

#endif // ORACLE_COMMON_H_INCLUDED
