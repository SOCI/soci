//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_EMPTY_SOURCE
#include "soci.h"
#include "soci-empty.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


EmptyStatementBackEnd::EmptyStatementBackEnd(EmptySessionBackEnd &session)
    : session_(session)
{
}

void EmptyStatementBackEnd::alloc()
{
    // ...
}

void EmptyStatementBackEnd::cleanUp()
{
    // ...
}

void EmptyStatementBackEnd::prepare(std::string const & /* query */,
    eStatementType /* eType */)
{
    // ...
}

StatementBackEnd::execFetchResult
EmptyStatementBackEnd::execute(int /* number */)
{
    // ...
    return eSuccess;
}

StatementBackEnd::execFetchResult
EmptyStatementBackEnd::fetch(int /* number */)
{
    // ...
    return eSuccess;
}

int EmptyStatementBackEnd::getNumberOfRows()
{
    // ...
    return 1;
}

std::string EmptyStatementBackEnd::rewriteForProcedureCall(
    std::string const &query)
{
    return query;
}

int EmptyStatementBackEnd::prepareForDescribe()
{
    // ...
    return 0;
}

void EmptyStatementBackEnd::describeColumn(int /* colNum */,
    eDataType & /* type */, std::string & /* columnName */)
{
    // ...
}

EmptyStandardIntoTypeBackEnd * EmptyStatementBackEnd::makeIntoTypeBackEnd()
{
    return new EmptyStandardIntoTypeBackEnd(*this);
}

EmptyStandardUseTypeBackEnd * EmptyStatementBackEnd::makeUseTypeBackEnd()
{
    return new EmptyStandardUseTypeBackEnd(*this);
}

EmptyVectorIntoTypeBackEnd *
EmptyStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    return new EmptyVectorIntoTypeBackEnd(*this);
}

EmptyVectorUseTypeBackEnd * EmptyStatementBackEnd::makeVectorUseTypeBackEnd()
{
    return new EmptyVectorUseTypeBackEnd(*this);
}
