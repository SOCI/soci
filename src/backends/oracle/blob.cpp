//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/oracle/soci-oracle.h"
#include "error.h"
#include "soci/statement.h"
#include <cstring>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cctype>

#include <iostream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::oracle;

oracle_blob_backend::oracle_blob_backend(oracle_session_backend &session)
    : session_(session), lobp_(NULL), initialized_(false)
{
    sword res = OCIDescriptorAlloc(session.envhp_,
        reinterpret_cast<dvoid**>(&lobp_), OCI_DTYPE_LOB, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw soci_error("Cannot allocate the LOB locator");
    }
	std::cout << "Created Oracle Blob obj\n";
}

oracle_blob_backend::~oracle_blob_backend()
{
    try {
        reset();
    } catch (const oracle_soci_error &) {
        // Ignore, as we shouldn't throw exceptions from the destructor
    }

    OCIDescriptorFree(lobp_, OCI_DTYPE_LOB);
	std::cout << "Destroyed Oracle Blob obj\n";
}

std::size_t oracle_blob_backend::get_len()
{
	std::cout << "Getting blob length\n";
    if (!initialized_) {
        return 0;
    }

    ub4 len;

    sword res = OCILobGetLength(session_.svchp_, session_.errhp_,
        lobp_, &len);

    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(len);
}

std::size_t oracle_blob_backend::read_from_start(char *buf, std::size_t toRead, std::size_t offset)
{
	std::cout << "Reading from blob (" << toRead << ", " << offset << ")\n";
    if (offset >= get_len())
    {
        if (!initialized_ && offset == 0)
        {
            // Read-attempts (from the beginning) on uninitialized BLOBs is defined to be a no-op
            return 0;
        }

        throw soci_error("Can't read past-the-end of BLOB data.");
    }

    ub4 amt = static_cast<ub4>(toRead);

    sword res = OCILobRead(session_.svchp_, session_.errhp_, lobp_, &amt,
        static_cast<ub4>(offset + 1), reinterpret_cast<dvoid*>(buf),
        amt, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

std::size_t oracle_blob_backend::write_from_start(char const *buf, std::size_t toWrite, std::size_t offset)
{
	std::cout << "Writing to blob (" << toWrite << ", " << offset << ")\n";
	if (offset > get_len())
	{
        // If offset == length, the operation is to be understood as appending (and is therefore allowed)
        throw soci_error("Can't start writing far past-the-end of BLOB data.");
	}

    ensure_initialized();

    ub4 amt = static_cast<ub4>(toWrite);

    sword res = OCILobWrite(session_.svchp_, session_.errhp_, lobp_, &amt,
        static_cast<ub4>(offset + 1),
        reinterpret_cast<dvoid*>(const_cast<char*>(buf)),
        amt, OCI_ONE_PIECE, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

std::size_t oracle_blob_backend::append(char const *buf, std::size_t toWrite)
{
	std::cout << "Appending to blob (" << toWrite << ")\n";
    ensure_initialized();

    ub4 amt = static_cast<ub4>(toWrite);

    sword res = OCILobWriteAppend(session_.svchp_, session_.errhp_, lobp_,
        &amt, reinterpret_cast<dvoid*>(const_cast<char*>(buf)),
        amt, OCI_ONE_PIECE, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

void oracle_blob_backend::trim(std::size_t newLen)
{
	std::cout << "Trimming blob (" << newLen << ")\n";
    sword res = OCILobTrim(session_.svchp_, session_.errhp_, lobp_,
        static_cast<ub4>(newLen));
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }
}

oracle_blob_backend::locator_t oracle_blob_backend::get_lob_locator() const
{
    return lobp_;
}

void oracle_blob_backend::set_lob_locator(oracle_blob_backend::locator_t locator, bool initialized)
{
	std::cout << "Setting locator\n";
    reset();

    lobp_ = locator;

    initialized_ = initialized;

    if (initialized)
    {
        sword res = OCILobOpen(session_.svchp_, session_.errhp_, lobp_, OCI_LOB_READWRITE);

        if (res != OCI_SUCCESS)
        {
            throw_oracle_soci_error(res, session_.errhp_);
        }
    }
}

void oracle_blob_backend::reset()
{
	std::cout << "Resetting blob\n";
    if (!initialized_)
    {
        return;
    }

    boolean is_temporary = FALSE;
    sword res = OCILobIsTemporary(session_.envhp_, session_.errhp_, lobp_, &is_temporary);

    if (res != OCI_SUCCESS)
    {
		std::cout << "Can't check if temporary LOB\n";
        throw_oracle_soci_error(res, session_.errhp_);
    }

    if (is_temporary) {
        res = OCILobFreeTemporary(session_.svchp_, session_.errhp_, lobp_);
    } else {
        res = OCILobClose(session_.svchp_, session_.errhp_, lobp_);
    }

    if (res != OCI_SUCCESS)
    {
		std::cout << "Can't free/close LOB (is temporary: " << is_temporary << ")\n";
        throw_oracle_soci_error(res, session_.errhp_);
    }

    initialized_ = false;

	std::cout << "Reset complete\n";
}

void oracle_blob_backend::ensure_initialized()
{
    if (!initialized_)
    {
		std::cout << "Initializing blob\n";
        // If asked to initialize explicitly, we can only create a temporary LOB
        sword res = OCILobCreateTemporary(session_.svchp_, session_.errhp_, lobp_,
                OCI_DEFAULT, SQLCS_IMPLICIT, OCI_TEMP_BLOB, FALSE, OCI_DURATION_SESSION);

        if (res != OCI_SUCCESS)
        {
            throw_oracle_soci_error(res, session_.errhp_);
        }

        res = OCILobOpen(session_.svchp_, session_.errhp_, lobp_, OCI_LOB_READWRITE);

        if (res != OCI_SUCCESS)
        {
            throw_oracle_soci_error(res, session_.errhp_);
        }

        initialized_ = true;
    }
}
