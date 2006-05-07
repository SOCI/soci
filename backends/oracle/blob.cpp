//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-oracle.h"
#include "error.h"
#include <cstring>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cctype>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Oracle;

OracleBLOBBackEnd::OracleBLOBBackEnd(OracleSessionBackEnd &session)
    : session_(session)
{
    sword res = OCIDescriptorAlloc(session.envhp_,
        reinterpret_cast<dvoid**>(&lobp_), OCI_DTYPE_LOB, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot allocate the LOB locator");
    }
}

OracleBLOBBackEnd::~OracleBLOBBackEnd()
{
    OCIDescriptorFree(lobp_, OCI_DTYPE_LOB);
}

std::size_t OracleBLOBBackEnd::getLen()
{
    ub4 len;

    sword res = OCILobGetLength(session_.svchp_, session_.errhp_,
        lobp_, &len);

    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return static_cast<std::size_t>(len);
}

std::size_t OracleBLOBBackEnd::read(
    std::size_t offset, char *buf, std::size_t toRead)
{
    ub4 amt = static_cast<ub4>(toRead);

    sword res = OCILobRead(session_.svchp_, session_.errhp_, lobp_, &amt,
        static_cast<ub4>(offset), reinterpret_cast<dvoid*>(buf),
        amt, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

std::size_t OracleBLOBBackEnd::write(
    std::size_t offset, char const *buf, std::size_t toWrite)
{
    ub4 amt = static_cast<ub4>(toWrite);

    sword res = OCILobWrite(session_.svchp_, session_.errhp_, lobp_, &amt,
        static_cast<ub4>(offset),
        reinterpret_cast<dvoid*>(const_cast<char*>(buf)),
        amt, OCI_ONE_PIECE, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

std::size_t OracleBLOBBackEnd::append(char const *buf, std::size_t toWrite)
{
    ub4 amt = static_cast<ub4>(toWrite);

    sword res = OCILobWriteAppend(session_.svchp_, session_.errhp_, lobp_,
        &amt, reinterpret_cast<dvoid*>(const_cast<char*>(buf)),
        amt, OCI_ONE_PIECE, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

void OracleBLOBBackEnd::trim(std::size_t newLen)
{
    sword res = OCILobTrim(session_.svchp_, session_.errhp_, lobp_,
        static_cast<ub4>(newLen));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }
}

