//
// Copyright (C) 2004, 2005 Maciej Sobczak, Steve Hutton
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#include "soci.h"
#include "soci-empty.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


EmptySessionBackEnd::EmptySessionBackEnd(
    std::string const & /* connectString */)
{
    // ...
}

EmptySessionBackEnd::~EmptySessionBackEnd()
{
    cleanUp();
}

void EmptySessionBackEnd::begin()
{
    // ...
}

void EmptySessionBackEnd::commit()
{
    // ...
}

void EmptySessionBackEnd::rollback()
{
    // ...
}

void EmptySessionBackEnd::cleanUp()
{
    // ...
}

EmptyStatementBackEnd * EmptySessionBackEnd::makeStatementBackEnd()
{
    return new EmptyStatementBackEnd(*this);
}

EmptyRowIDBackEnd * EmptySessionBackEnd::makeRowIDBackEnd()
{
    return new EmptyRowIDBackEnd(*this);
}

EmptyBLOBBackEnd * EmptySessionBackEnd::makeBLOBBackEnd()
{
    return new EmptyBLOBBackEnd(*this);
}

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

void EmptyStatementBackEnd::prepare(std::string const & /* query */)
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

void EmptyStandardIntoTypeBackEnd::defineByPos(
    int & /* position */, void * /* data */, eExchangeType /* type */)
{
    // ...
}

void EmptyStandardIntoTypeBackEnd::preFetch()
{
    // ...
}

void EmptyStandardIntoTypeBackEnd::postFetch(
    bool /* gotData */, bool /* calledFromFetch */, eIndicator * /* ind */)
{
    // ...
}

void EmptyStandardIntoTypeBackEnd::cleanUp()
{
    // ...
}

void EmptyVectorIntoTypeBackEnd::defineByPos(
    int & /* position */, void * /* data */, eExchangeType /* type */)
{
    // ...
}

void EmptyVectorIntoTypeBackEnd::preFetch()
{
    // ...
}

void EmptyVectorIntoTypeBackEnd::postFetch(
    bool /* gotData */, eIndicator * /* ind */)
{
    // ...
}

void EmptyVectorIntoTypeBackEnd::resize(std::size_t /* sz */)
{
    // ...
}

std::size_t EmptyVectorIntoTypeBackEnd::size()
{
    // ...
    return 1;
}

void EmptyVectorIntoTypeBackEnd::cleanUp()
{
    // ...
}

void EmptyStandardUseTypeBackEnd::bindByPos(
    int & /* position */, void * /* data */, eExchangeType /* type */)
{
    // ...
}

void EmptyStandardUseTypeBackEnd::bindByName(
    std::string const & /* name */, void * /* data */,
    eExchangeType /* type */)
{
    // ...
}

void EmptyStandardUseTypeBackEnd::preUse(eIndicator const * /* ind */)
{
    // ...
}

void EmptyStandardUseTypeBackEnd::postUse(
    bool /* gotData */, eIndicator * /* ind */)
{
    // ...
}

void EmptyStandardUseTypeBackEnd::cleanUp()
{
    // ...
}

void EmptyVectorUseTypeBackEnd::bindByPos(int & /* position */,
        void * /* data */, eExchangeType /* type */)
{
    // ...
}

void EmptyVectorUseTypeBackEnd::bindByName(
    std::string const & /* name */, void * /* data */,
    eExchangeType /* type */)
{
    // ...
}

void EmptyVectorUseTypeBackEnd::preUse(eIndicator const * /* ind */)
{
    // ...
}

std::size_t EmptyVectorUseTypeBackEnd::size()
{
    // ...
    return 1;
}

void EmptyVectorUseTypeBackEnd::cleanUp()
{
    // ...
}

EmptyRowIDBackEnd::EmptyRowIDBackEnd(EmptySessionBackEnd & /* session */)
{
    // ...
}

EmptyRowIDBackEnd::~EmptyRowIDBackEnd()
{
    // ...
}

EmptyBLOBBackEnd::EmptyBLOBBackEnd(EmptySessionBackEnd &session)
    : session_(session)
{
    // ...
}

EmptyBLOBBackEnd::~EmptyBLOBBackEnd()
{
    // ...
}

std::size_t EmptyBLOBBackEnd::getLen()
{
    // ...
    return 0;
}

std::size_t EmptyBLOBBackEnd::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    // ...
    return 0;
}

std::size_t EmptyBLOBBackEnd::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    // ...
    return 0;
}

std::size_t EmptyBLOBBackEnd::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    // ...
    return 0;
}

void EmptyBLOBBackEnd::trim(std::size_t /* newLen */)
{
    // ...
}


// concrete factory for Empty concrete strategies
struct EmptyBackEndFactory : BackEndFactory
{
    virtual EmptySessionBackEnd * makeSession(
        std::string const &connectString) const
    {
        return new EmptySessionBackEnd(connectString);
    }

} emptyBEF;

namespace
{

// global object for automatic factory registration
struct EmptyAutoRegister
{
    EmptyAutoRegister()
    {
        theBEFRegistry().registerMe("empty", &emptyBEF);
    }
} emptyAutoRegister;

} // namespace anonymous
