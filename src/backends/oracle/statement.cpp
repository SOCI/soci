//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci/oracle/soci-oracle.h"
#include "error.h"
#include "handle.h"
#include "soci/soci-backend.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <limits>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::oracle;

namespace
{

template <typename T, typename OCIHandle>
T get_oci_attr(OCIHandle* hp, int attr, OCIError* errhp)
{
    T value;
    sword res = OCIAttrGet(hp,
        oci_traits<OCIHandle>::type,
        &value,
        0,
        attr,
        errhp
    );

    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, errhp);
    }

    return value;
}

template <typename T, typename OCIHandle>
T get_oci_attr(handle<OCIHandle>& hp, int attr, OCIError* errhp)
{
    return get_oci_attr<T>(hp.get(), attr, errhp);
}

} // anonymous namespace


oracle_statement_backend::oracle_statement_backend(oracle_session_backend &session)
    : session_(session), stmtp_(NULL), boundByName_(false), boundByPos_(false),
      noData_(false)
{
}

void oracle_statement_backend::alloc()
{
    sword res = OCIHandleAlloc(session_.envhp_,
        reinterpret_cast<dvoid**>(&stmtp_),
        OCI_HTYPE_STMT, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw soci_error("Cannot allocate statement handle");
    }
}

void oracle_statement_backend::clean_up()
{
    // deallocate statement handle
    if (stmtp_ != NULL)
    {
        OCIHandleFree(stmtp_, OCI_HTYPE_STMT);
        stmtp_ = NULL;
    }

    boundByName_ = false;
    boundByPos_ = false;
}

void oracle_statement_backend::prepare(std::string const &query,
    statement_type /* eType */)
{
    sb4 stmtLen = static_cast<sb4>(query.size());
    sword res = OCIStmtPrepare(stmtp_,
        session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(query.c_str())),
        stmtLen, OCI_V7_SYNTAX, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }
}

statement_backend::exec_fetch_result oracle_statement_backend::execute(int number)
{
    ub4 mode = OCI_DEFAULT;

    // We want to use OCI_BATCH_ERRORS for bulk operations in order to get
    // information about the row(s) which resulted in errors.
    if (hasVectorUseElements_)
    {
        switch (get_statement_attr<ub2>(OCI_ATTR_STMT_TYPE))
        {
            case OCI_STMT_UPDATE:
            case OCI_STMT_DELETE:
            case OCI_STMT_INSERT:
                mode = OCI_BATCH_ERRORS;
                break;
        }
    }

    sword res = OCIStmtExecute(session_.svchp_, stmtp_, session_.errhp_,
        static_cast<ub4>(number), 0, 0, 0, mode);

    // For bulk operations, "success with info" is used even when some rows
    // resulted in errors, so check for this and return error in this case.
    if (hasVectorUseElements_ && res == OCI_SUCCESS_WITH_INFO)
    {
        // We don't handle multiple errors and only distinguish between not
        // having any errors at all and having at least one. This is, of
        // course, not ideal, but provides at least some information about the
        // (first) error.
        //
        // Note that we have to use a different error handle to this call to
        // avoid clobbering the error handle used for the statement itself.
        handle<OCIError> errhTmp(session_.envhp_);
        if (get_oci_attr<ub4>(stmtp_, OCI_ATTR_NUM_DML_ERRORS, errhTmp))
        {
            handle<OCIError> errhRow(session_.envhp_);
            sword res2 = OCIParamGet(session_.errhp_,
                OCI_HTYPE_ERROR,
                errhTmp,
                errhRow.ptr(),
                0
            );
            if (res2 != OCI_SUCCESS)
            {
                throw_oracle_soci_error(res2, errhTmp);
            }

            error_row_ = get_oci_attr<ub4>(errhRow, OCI_ATTR_DML_ROW_OFFSET, errhTmp);
            throw_oracle_soci_error(res, errhRow);
        }
        //else: No errors, handle as success below.
    }

    if (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO)
    {
        noData_ = false;
        return ef_success;
    }
    else if (res == OCI_NO_DATA)
    {
        noData_ = true;
        return ef_no_data;
    }
    else
    {
        throw_oracle_soci_error(res, session_.errhp_);
        return ef_no_data; // unreachable dummy return to please the compiler
    }
}

statement_backend::exec_fetch_result oracle_statement_backend::fetch(int number)
{
    if (noData_)
    {
        return ef_no_data;
    }

    sword res = OCIStmtFetch(stmtp_, session_.errhp_,
        static_cast<ub4>(number), OCI_FETCH_NEXT, OCI_DEFAULT);

    if (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO)
    {
        return ef_success;
    }
    else if (res == OCI_NO_DATA)
    {
        noData_ = true;
        return ef_no_data;
    }
    else
    {
        throw_oracle_soci_error(res, session_.errhp_);
        return ef_no_data; // unreachable dummy return to please the compiler
    }
}

template <typename T>
T oracle_statement_backend::get_statement_attr(int attr) const
{
    return get_oci_attr<T>(stmtp_, attr, session_.errhp_);
}

long long oracle_statement_backend::get_affected_rows()
{
    return get_statement_attr<ub4>(OCI_ATTR_ROW_COUNT);
}

int oracle_statement_backend::get_number_of_rows()
{
    return get_statement_attr<ub4>(OCI_ATTR_ROWS_FETCHED);
}

std::string oracle_statement_backend::get_parameter_name(int index) const
{
    // We could query all parameters at once and cache the result, because we
    // know that typically we're going to need all of their names if we need
    // one of them, but for now keep it simple it and get them one by one, even
    // if it's probably a bit slower.
    sb4 signedCount = 0;
    OraText* name = NULL;
    ub1 len = 0;

    // We don't need the remaining outputs, but we still must specify them as
    // otherwise the function just fails with a non-existent ORA-24999.
    OraText* indName = NULL;
    ub1 indLen = 0;
    ub1 duplicate = 0;

    sword res = OCIStmtGetBindInfo(stmtp_,
        session_.errhp_,
        1,              // Number of parameters to query.
        index + 1,      // Position to start querying.
        &signedCount,   // Abs value (!) is the total number of parameters.
        &name,          // Name of the parameter.
        &len,           // Length of the name.
        &indName,       // Indicator name.
        &indLen,        // Length of the indicator name.
        &duplicate,     // Is the parameter a duplicate?
        NULL            // The bind handle -- not needed and can be omitted.
    );

    if ( res != OCI_SUCCESS )
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return std::string(reinterpret_cast<const char*>(name), len);
}

std::string oracle_statement_backend::rewrite_for_procedure_call(
    std::string const &query)
{
    std::string newQuery("begin ");
    newQuery += query;
    newQuery += "; end;";
    return newQuery;
}

int oracle_statement_backend::prepare_for_describe()
{
    const ub2 statementType = get_statement_attr<ub2>(OCI_ATTR_STMT_TYPE);

    if (statementType != OCI_STMT_SELECT)
        return 0;

    sword res = OCIStmtExecute(session_.svchp_, stmtp_, session_.errhp_,
        1, 0, 0, 0, OCI_DESCRIBE_ONLY);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return get_statement_attr<ub4>(OCI_ATTR_PARAM_COUNT);
}

void oracle_statement_backend::describe_column(int colNum,
    db_type &xdbtype,
    std::string &columnName)
{
    ub2 dbtype;
    text* dbname;
    ub4 nameLength;

    ub2 dbsize;
    sb2 dbprec;
    ub1 dbscale; //sb2 in some versions of Oracle?

    // Get the column handle
    OCIParam* colhd;
    sword res = OCIParamGet(reinterpret_cast<dvoid*>(stmtp_),
        static_cast<ub4>(OCI_HTYPE_STMT),
        reinterpret_cast<OCIError*>(session_.errhp_),
        reinterpret_cast<dvoid**>(&colhd),
        static_cast<ub4>(colNum));
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    // Get the column name
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid**>(&dbname),
        reinterpret_cast<ub4*>(&nameLength),
        static_cast<ub4>(OCI_ATTR_NAME),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    // Get the column type
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbtype),
        0,
        static_cast<ub4>(OCI_ATTR_DATA_TYPE),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    // get the data size
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbsize),
        0,
        static_cast<ub4>(OCI_ATTR_DATA_SIZE),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    // get the precision
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbprec),
        0,
        static_cast<ub4>(OCI_ATTR_PRECISION),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    // get the scale if necessary, i.e. if not using just NUMBER, for which
    // both precision and scale are 0 anyhow
    if (dbprec)
    {
        res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
            static_cast<ub4>(OCI_DTYPE_PARAM),
            reinterpret_cast<dvoid*>(&dbscale),
            0,
            static_cast<ub4>(OCI_ATTR_SCALE),
            reinterpret_cast<OCIError*>(session_.errhp_));
        if (res != OCI_SUCCESS)
        {
            throw_oracle_soci_error(res, session_.errhp_);
        }
    }
    else // precision is 0, meaning that this is the default number type
    {
        // don't bother retrieving the scale using OCI, we know its default
        // value, as NUMBER is the same as NUMBER(38,10), and we also don't
        // really care about it, all that matters is for it to be > 0 so that
        // the right type is selected below
        dbscale = 10;
    }

    columnName.assign(dbname, dbname + nameLength);

    switch (dbtype)
    {
    case SQLT_CHR:
    case SQLT_AFC:
        xdbtype = db_string;
        break;
    case SQLT_NUM:
        if (dbscale > 0)
        {
            if (session_.get_option_decimals_as_strings())
            {
                xdbtype = db_string;
            }
            else
            {
                xdbtype = db_double;
            }
        }
        else if (dbprec <= std::numeric_limits<int32_t>::digits10)
        {
            xdbtype = db_int32;
        }
        else
        {
            xdbtype = db_int64;
        }
        break;
    case OCI_TYPECODE_BDOUBLE:
        xdbtype = db_double;
        break;
    case SQLT_DAT:
        xdbtype = db_date;
        break;
    case SQLT_BLOB:
        xdbtype = db_blob;
        break;
    default:
        // Unknown oracle types will just be represented by a string
        xdbtype = db_string;
    }
}

std::size_t oracle_statement_backend::column_size(int position)
{
    // Note: we may want to optimize so that the OCI_DESCRIBE_ONLY call
    // happens only once per statement.
    // Possibly use existing statement::describe() / make column prop
    // access lazy at same time

    int colSize(0);

    sword res = OCIStmtExecute(session_.svchp_, stmtp_,
         session_.errhp_, 1, 0, 0, 0, OCI_DESCRIBE_ONLY);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    // Get The Column Handle
    OCIParam* colhd;
    res = OCIParamGet(reinterpret_cast<dvoid*>(stmtp_),
         static_cast<ub4>(OCI_HTYPE_STMT),
         reinterpret_cast<OCIError*>(session_.errhp_),
         reinterpret_cast<dvoid**>(&colhd),
         static_cast<ub4>(position));
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

     // Get The Data Size
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
         static_cast<ub4>(OCI_DTYPE_PARAM),
         reinterpret_cast<dvoid*>(&colSize),
         0,
         static_cast<ub4>(OCI_ATTR_DATA_SIZE),
         reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, session_.errhp_);
    }

    return static_cast<std::size_t>(colSize);
}

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
