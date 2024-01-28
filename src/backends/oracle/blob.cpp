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
}

oracle_blob_backend::~oracle_blob_backend()
{
    try
    {
        reset();
    }
    catch (const oracle_soci_error &)
    {
        // Ignore, as we shouldn't throw exceptions from the destructor
    }

    OCIDescriptorFree(lobp_, OCI_DTYPE_LOB);
}

std::size_t oracle_blob_backend::get_len()
{
    if (!initialized_)
    {
        return 0;
    }

    oraub8 len;

    sword res = OCILobGetLength2(session_.svchp_, session_.errhp_,
        lobp_, &len);

    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(len);
}

std::size_t oracle_blob_backend::read_from_start(void *buf, std::size_t toRead, std::size_t offset)
{
    if (offset >= get_len())
    {
        if (!initialized_ && offset == 0)
        {
            // Read-attempts (from the beginning) on uninitialized BLOBs is defined to be a no-op
            return 0;
        }

        throw soci_error("Can't read past-the-end of BLOB data.");
    }

    auto amt = static_cast<oraub8>(toRead);

    sword res = OCILobRead2(session_.svchp_, session_.errhp_, lobp_, &amt, nullptr,
        static_cast<oraub8>(offset + 1), buf, amt, OCI_ONE_PIECE, nullptr, nullptr, 0, SQLCS_IMPLICIT);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

std::size_t oracle_blob_backend::write_from_start(const void *buf, std::size_t toWrite, std::size_t offset)
{
    if (offset > get_len())
    {
        // If offset == length, the operation is to be understood as appending (and is therefore allowed)
        throw soci_error("Can't start writing far past-the-end of BLOB data.");
    }

    ensure_initialized();

    auto amt = static_cast<oraub8>(toWrite);

    sword res = OCILobWrite2(session_.svchp_, session_.errhp_, lobp_, &amt, nullptr,
        static_cast<oraub8>(offset + 1),
        const_cast<void*>(buf), amt, OCI_ONE_PIECE, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

std::size_t oracle_blob_backend::append(const void *buf, std::size_t toWrite)
{
    ensure_initialized();

    auto amt = static_cast<oraub8>(toWrite);

    sword res = OCILobWriteAppend2(session_.svchp_, session_.errhp_, lobp_,
        &amt, nullptr, const_cast<void*>(buf), amt, OCI_ONE_PIECE, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(amt);
}

void oracle_blob_backend::trim(std::size_t newLen)
{
    sword res = OCILobTrim2(session_.svchp_, session_.errhp_, lobp_,
        static_cast<oraub8>(newLen));
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
    // If we select a BLOB value into a BLOB object, then the post_fetch code in
    // the standard_into_type_backend will set this object's locator to the one it is
    // already holding.
    // In this case, the locator now already points to the desired BLOB object and thus we
    // must not reset it.
    if (lobp_ != locator)
    {
        reset();

        lobp_ = locator;
    }

    initialized_ = initialized;

    if (initialized)
    {
        boolean already_open = FALSE;
        sword res = OCILobIsOpen(session_.svchp_, session_.errhp_, lobp_, &already_open);

        if (res != OCI_SUCCESS)
        {
            throw_oracle_soci_error(res, session_.errhp_);
        }

        if (!already_open)
        {
            res = OCILobOpen(session_.svchp_, session_.errhp_, lobp_, OCI_LOB_READWRITE);

            if (res != OCI_SUCCESS)
            {
                throw_oracle_soci_error(res, session_.errhp_);
            }
        }
    }
}

void oracle_blob_backend::reset()
{
    if (!initialized_)
    {
        return;
    }

    boolean is_temporary = FALSE;
    sword res = OCILobIsTemporary(session_.envhp_, session_.errhp_, lobp_, &is_temporary);

    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    if (is_temporary)
    {
        res = OCILobFreeTemporary(session_.svchp_, session_.errhp_, lobp_);
    }
    else
    {
        res = OCILobClose(session_.svchp_, session_.errhp_, lobp_);
    }

    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    initialized_ = false;
}

void oracle_blob_backend::ensure_initialized()
{
    if (!initialized_)
    {
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
