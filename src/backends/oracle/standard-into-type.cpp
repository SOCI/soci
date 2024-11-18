//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci/oracle/soci-oracle.h"
#include "soci/blob.h"
#include "clob.h"
#include "error.h"
#include "soci/rowid.h"
#include "soci/statement.h"
#include "soci/soci-platform.h"
#include "soci-exchange-cast.h"
#include "soci-mktime.h"
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include <oci.h>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::oracle;

oracle_standard_into_type_backend *
oracle_statement_backend::make_into_type_backend()
{
    return new oracle_standard_into_type_backend(*this);
}

oracle_standard_use_type_backend *
oracle_statement_backend::make_use_type_backend()
{
    return new oracle_standard_use_type_backend(*this);
}

oracle_vector_into_type_backend *
oracle_statement_backend::make_vector_into_type_backend()
{
    return new oracle_vector_into_type_backend(*this);
}

oracle_vector_use_type_backend *
oracle_statement_backend::make_vector_use_type_backend()
{
    return new oracle_vector_use_type_backend(*this);
}

void oracle_standard_into_type_backend::define_by_pos(
    int &position, void *data, exchange_type type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType SOCI_DUMMY_INIT(0);
    sb4 size SOCI_DUMMY_INIT(0);

    switch (type)
    {
    // simple cases
    case x_char:
        oracleType = SQLT_AFC;
        size = sizeof(char);
        break;
    case x_int8:
        oracleType = SQLT_INT;
        size = sizeof(int8_t);
        break;
    case x_uint8:
        oracleType = SQLT_UIN;
        size = sizeof(uint8_t);
        break;
    case x_int16:
        oracleType = SQLT_INT;
        size = sizeof(int16_t);
        break;
    case x_uint16:
        oracleType = SQLT_UIN;
        size = sizeof(uint16_t);
        break;
    case x_int32:
        oracleType = SQLT_INT;
        size = sizeof(int32_t);
        break;
    case x_uint32:
        oracleType = SQLT_UIN;
        size = sizeof(uint32_t);
        break;
    case x_double:
        oracleType = statement_.session_.get_double_sql_type();
        size = sizeof(double);
        break;

    // cases that require adjustments and buffer management
    case x_int64:
    case x_uint64:
        oracleType = SQLT_STR;
        size = 100; // arbitrary buffer length
        buf_ = new char[size];
        data = buf_;
        break;
    case x_stdstring:
        oracleType = SQLT_STR;
        size = 32769;  // support selecting strings from LONG columns
        buf_ = new char[size];
        data = buf_;
        break;
    case x_stdtm:
        oracleType = SQLT_DAT;
        size = 7 * sizeof(ub1);
        buf_ = new char[size];
        data = buf_;
        break;

    // cases that require special handling
    case x_statement:
        {
            oracleType = SQLT_RSET;

            statement *st = static_cast<statement *>(data);
            st->alloc();

            oracle_statement_backend *stbe
                = static_cast<oracle_statement_backend *>(st->get_backend());
            size = sizeof(stbe->stmtp_);
            data = &stbe->stmtp_;
        }
        break;
    case x_rowid:
        {
            oracleType = SQLT_RDD;

            rowid *rid = static_cast<rowid *>(data);

            oracle_rowid_backend *rbe
                = static_cast<oracle_rowid_backend *>(rid->get_backend());

            size = sizeof(rbe->rowidp_);
            data = &rbe->rowidp_;
        }
        break;
    case x_blob:
        {
            oracleType = SQLT_BLOB;

            blob *b = static_cast<blob *>(data);

            oracle_blob_backend *bbe
                = static_cast<oracle_blob_backend *>(b->get_backend());

            // Reset the blob to ensure that even when the query fails,
            // the blob does not contain leftover data from whatever it
            // contained before.
            bbe->reset();

            // Allocate our very own LOB locator. We have to do this instead of
            // reusing the locator from the existing Blob object in case the
            // statement for which we are binding is going to be executed multiple
            // times. If we borrowed the locator from the Blob object, consecutive
            // queries would override a previous locator (which might belong
            // to an independent Blob object by now).
            sword res = OCIDescriptorAlloc(statement_.session_.envhp_,
                reinterpret_cast<dvoid**>(&ociData_), OCI_DTYPE_LOB, 0, 0);
            if (res != OCI_SUCCESS)
            {
                throw soci_error("Cannot allocate the LOB locator");
            }


            size = sizeof(ociData_);
            data = &ociData_;
        }
        break;

    case x_xmltype:
    case x_longstring:
        {
            oracleType = SQLT_CLOB;

            // lazy initialization of the temporary LOB object,
            // actual creation of this object is in pre_exec, which
            // is called right before statement's execute

            OCILobLocator * lobp = NULL;

            size = sizeof(lobp);
            data = &ociData_;
            ociData_ = lobp;
        }
        break;
    }

    sword res = OCIDefineByPos(statement_.stmtp_, &defnp_,
            statement_.session_.errhp_,
            position++, data, size, oracleType,
            &indOCIHolder_, 0, &rCode_, OCI_DEFAULT);

    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, statement_.session_.errhp_);
    }
}

void oracle_standard_into_type_backend::pre_exec(int /* num */)
{
    if (type_ == x_xmltype || type_ == x_longstring)
    {
        // lazy initialization of the temporary LOB object
        ociData_ = create_temp_lob(statement_.session_);
    }
}

void oracle_standard_into_type_backend::pre_fetch()
{
    // nothing to do except with Statement into objects

    if (type_ == x_statement)
    {
        statement *st = static_cast<statement *>(data_);
        st->undefine_and_bind();
    }
}

/* ----------------------------------------------------------------- 
 * callback function to write buffer to the string.     
 * works in bytes, not in characters                 
 * ----------------------------------------------------------------- */
sb4 callback_to_read_in_bytes
(
    dvoid                             *pResultString,  // pointer to std::string to accumulate result
    CONST dvoid                       *bufxp,          // pointer to buffer from read
    ub4                                lenp,           // length data in the buffer
    ub1                                /*piecep*/      // OCI_FIRST_PIECE, OCI_NEXT_PIECE or OCI_LAST_PIECE
)
{
    static_cast<std::string*> ( pResultString )->append ( static_cast<const char *> ( bufxp ), lenp );
    return OCI_CONTINUE;
}


void oracle::read_from_lob(oracle_session_backend& session,
    OCILobLocator * lobp, std::string & value)
{
    ub4 len;

    // result is not a bytes count!!! For character LOBs (CLOBs, NCLOBs) it is the number of characters
    sword res = OCILobGetLength ( session.svchp_, session.errhp_, lobp, &len );
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session.errhp_);
    }

    ub4 amtp = 0;
    ub4 offset = 1;
    char buf[10240] = {};
    std::string accumTo;
    res = OCILobRead
    (
        session.svchp_, session.errhp_, lobp,   // service context, error handle, LOB locator
        &amtp,                                  // amount (in characters) to read. 0 means until the end of the LOB in streaming mode
        offset,                                 // offset == 1 means from the start of the LOB    
        &buf,
        sizeof (buf),
        &accumTo,                               // ctx for the callback - pointer to the result string        
        callback_to_read_in_bytes,              // callback function works in bytes!!!
        0,
        0
    );
    if ( res != OCI_SUCCESS )
    {
        throw_oracle_soci_error ( res, session.errhp_ );
    }
    value = std::move ( accumTo );
}

void oracle_standard_into_type_backend::post_fetch(
    bool gotData, bool calledFromFetch, indicator *ind)
{
    // first, deal with data
    if (gotData)
    {
        // only std::string, std::tm and Statement need special handling
        if (type_ == x_stdstring)
        {
            if (indOCIHolder_ != -1)
            {
                exchange_type_cast<x_stdstring>(data_) = buf_;
            }
        }
        else if (type_ == x_int64)
        {
            if (indOCIHolder_ != -1)
            {
                exchange_type_cast<x_int64>(data_) = std::strtoll(buf_, NULL, 10);
            }
        }
        else if (type_ == x_uint64)
        {
            if (indOCIHolder_ != -1)
            {
                exchange_type_cast<x_uint64>(data_) = std::strtoull(buf_, NULL, 10);
            }
        }
        else if (type_ == x_stdtm)
        {
            if (indOCIHolder_ != -1)
            {
                std::tm& t = exchange_type_cast<x_stdtm>(data_);

                ub1 *pos = reinterpret_cast<ub1*>(buf_);
                int year = (*pos++ - 100) * 100;
                year += *pos++ - 100;
                int const month = *pos++;
                int const day = *pos++;
                int const hour = *pos++ - 1;
                int const minute = *pos++ - 1;
                int const second = *pos++ - 1;

                details::mktime_from_ymdhms(t, year, month, day, hour, minute, second);
            }
        }
        else if (type_ == x_statement)
        {
            statement *st = static_cast<statement *>(data_);
            st->define_and_bind();
        }
        else if (type_ == x_xmltype)
        {
            if (indOCIHolder_ != -1)
            {
                OCILobLocator * lobp = static_cast<OCILobLocator *>(ociData_);

                read_from_lob(statement_.session_,
                    lobp, exchange_type_cast<x_xmltype>(data_).value);
            }
        }
        else if (type_ == x_longstring)
        {
            if (indOCIHolder_ != -1)
            {
                OCILobLocator * lobp = static_cast<OCILobLocator *>(ociData_);

                read_from_lob(statement_.session_,
                    lobp, exchange_type_cast<x_longstring>(data_).value);
            }
        }
        else if (type_ == x_blob)
        {
            blob *b = static_cast<blob *>(data_);

            oracle_blob_backend *bbe
                = static_cast<oracle_blob_backend *>(b->get_backend());

            bbe->set_lob_locator(reinterpret_cast<oracle_blob_backend::locator_t>(ociData_));
        }
    }

    // then - deal with indicators
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to set anything (fetch() will return false)
        return;
    }
    if (ind != NULL)
    {
        if (gotData)
        {
            if (indOCIHolder_ == 0)
            {
                *ind = i_ok;
            }
            else if (indOCIHolder_ == -1)
            {
                *ind = i_null;
            }
            else
            {
                *ind = i_truncated;
            }
        }
    }
    else
    {
        if (indOCIHolder_ == -1)
        {
            // fetched null and no indicator - programming error!
            throw soci_error("Null value fetched and no indicator defined.");
        }
    }
}

void oracle_standard_into_type_backend::clean_up()
{
    if (ociData_)
    {
        switch (type_)
        {
            case x_xmltype:
            case x_longstring:
                free_temp_lob(statement_.session_, static_cast<OCILobLocator *>(ociData_));
                break;

            case x_blob:
                OCIDescriptorFree(ociData_, OCI_DTYPE_LOB);
                break;

            default:
                throw soci_error("Internal error: OCI data used for unexpected type");
        }

        ociData_ = NULL;
    }

    if (defnp_ != NULL)
    {
        OCIHandleFree(defnp_, OCI_HTYPE_DEFINE);
        defnp_ = NULL;
    }

    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
