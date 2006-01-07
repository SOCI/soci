//
// Copyright (C) 2004, 2005 Maciej Sobczak, Steve Hutton
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//


#include "soci-oracle.h"
#include "soci.h"
#include <sstream>
#include <limits>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


OracleSOCIError::OracleSOCIError(std::string const & msg, int errNum)
    : SOCIError(msg), errNum_(errNum)
{
}

// retrieves service name, user name and password from the
// uniform connect string
void chopConnectString(std::string const &connectString,
    std::string &serviceName, std::string &userName, std::string &password)
{
    std::string tmp;
    for (std::string::const_iterator i = connectString.begin(),
             end = connectString.end(); i != end; ++i)
    {
        if (*i == '=')
        {
            tmp += ' ';
        }
        else
        {
            tmp += *i;
        }
    }

    serviceName.clear();
    userName.clear();
    password.clear();

    std::istringstream iss(tmp);
    std::string key, value;
    while (iss >> key >> value)
    {
        if (key == "service")
        {
            serviceName = value;
        }
        else if (key == "user")
        {
            userName = value;
        }
        else if (key == "password")
        {
            password = value;
        }
    }
}

void getErrorDetails(sword res, OCIError *errhp,
    std::string &msg, int &errNum)
{
    text errbuf[512];
    sb4 errcode;
    errNum = 0;

    switch (res)
    {
    case OCI_NO_DATA:
        msg = "SOCI error: No data";
        break;
    case OCI_ERROR:
        OCIErrorGet(errhp, 1, 0, &errcode,
             errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
        msg = reinterpret_cast<char*>(errbuf);
        errNum = static_cast<int>(errcode);
        break;
    case OCI_INVALID_HANDLE:
        msg = "SOCI error: Invalid handle";
        break;
    default:
        msg = "SOCI error: Unknown error code";
    }
}

void throwOracleSOCIError(sword res, OCIError *errhp)
{
    std::string msg;
    int errNum;

    getErrorDetails(res, errhp, msg, errNum);
    throw OracleSOCIError(msg, errNum);
}

OracleSessionBackEnd::OracleSessionBackEnd(std::string const & serviceName,
    std::string const & userName, std::string const & password)
    : envhp_(NULL), srvhp_(NULL), errhp_(NULL), svchp_(NULL), usrhp_(NULL)
{
    sword res;

    // create the environment
    res = OCIEnvCreate(&envhp_, OCI_DEFAULT, 0, 0, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot create environment");
    }

    // create the server handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&srvhp_),
        OCI_HTYPE_SERVER, 0, 0);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot create server handle");
    }

    // create the error handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&errhp_),
        OCI_HTYPE_ERROR, 0, 0);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot create error handle");
    }

    // create the server context
    sb4 serviceNameLen = static_cast<sb4>(serviceName.size());
    res = OCIServerAttach(srvhp_, errhp_,
        reinterpret_cast<text*>(const_cast<char*>(serviceName.c_str())),
        serviceNameLen, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        getErrorDetails(res, errhp_, msg, errNum);
        cleanUp();
        throw OracleSOCIError(msg, errNum);
    }

    // create service context handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&svchp_),
        OCI_HTYPE_SVCCTX, 0, 0);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot create service context");
    }

    // set the server attribute in the context handle
    res = OCIAttrSet(svchp_, OCI_HTYPE_SVCCTX, srvhp_, 0,
        OCI_ATTR_SERVER, errhp_);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        getErrorDetails(res, errhp_, msg, errNum);
        cleanUp();
        throw OracleSOCIError(msg, errNum);
    }

    // allocate user session handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&usrhp_),
        OCI_HTYPE_SESSION, 0, 0);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot allocate user session handle");
    }

    // set username attribute in the user session handle
    sb4 userNameLen = static_cast<sb4>(userName.size());
    res = OCIAttrSet(usrhp_, OCI_HTYPE_SESSION,
        reinterpret_cast<dvoid*>(const_cast<char*>(userName.c_str())),
        userNameLen, OCI_ATTR_USERNAME, errhp_);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot set username");
    }

    // set password attribute
    sb4 passwordLen = static_cast<sb4>(password.size());
    res = OCIAttrSet(usrhp_, OCI_HTYPE_SESSION,
        reinterpret_cast<dvoid*>(const_cast<char*>(password.c_str())),
        passwordLen, OCI_ATTR_PASSWORD, errhp_);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot set password");
    }

    // begin the session
    res = OCISessionBegin(svchp_, errhp_, usrhp_,
        OCI_CRED_RDBMS, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        getErrorDetails(res, errhp_, msg, errNum);
        cleanUp();
        throw OracleSOCIError(msg, errNum);
    }

    // set the session in the context handle
    res = OCIAttrSet(svchp_, OCI_HTYPE_SVCCTX, usrhp_,
        0, OCI_ATTR_SESSION, errhp_);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        getErrorDetails(res, errhp_, msg, errNum);
        cleanUp();
        throw OracleSOCIError(msg, errNum);
    }
}

OracleSessionBackEnd::~OracleSessionBackEnd()
{
    cleanUp();
}

void OracleSessionBackEnd::begin()
{
    sword res = OCITransStart(svchp_, errhp_, 0, OCI_TRANS_NEW);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, errhp_);
    }
}

void OracleSessionBackEnd::commit()
{
    sword res = OCITransCommit(svchp_, errhp_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, errhp_);
    }
}

void OracleSessionBackEnd::rollback()
{
    sword res = OCITransRollback(svchp_, errhp_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, errhp_);
    }
}

void OracleSessionBackEnd::cleanUp()
{
    if (svchp_ != NULL && errhp_ != NULL && usrhp_ != NULL)
    {
        OCISessionEnd(svchp_, errhp_, usrhp_, OCI_DEFAULT);
    }

    if (usrhp_) OCIHandleFree(usrhp_, OCI_HTYPE_SESSION);
    if (svchp_) OCIHandleFree(svchp_, OCI_HTYPE_SVCCTX);
    if (srvhp_)
    {
        OCIServerDetach(srvhp_, errhp_, OCI_DEFAULT);
        OCIHandleFree(srvhp_, OCI_HTYPE_SERVER);
    }
    if (errhp_) OCIHandleFree(errhp_, OCI_HTYPE_ERROR);
    if (envhp_) OCIHandleFree(envhp_, OCI_HTYPE_ENV);
}

OracleStatementBackEnd * OracleSessionBackEnd::makeStatementBackEnd()
{
    return new OracleStatementBackEnd(*this);
}

OracleRowIDBackEnd * OracleSessionBackEnd::makeRowIDBackEnd()
{
    return new OracleRowIDBackEnd(*this);
}

OracleBLOBBackEnd * OracleSessionBackEnd::makeBLOBBackEnd()
{
    return new OracleBLOBBackEnd(*this);
}

OracleStatementBackEnd::OracleStatementBackEnd(OracleSessionBackEnd &session)
    : session_(session), stmtp_(NULL)
{
}

void OracleStatementBackEnd::alloc()
{
    sword res = OCIHandleAlloc(session_.envhp_,
        reinterpret_cast<dvoid**>(&stmtp_),
        OCI_HTYPE_STMT, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot allocate statement handle");
    }
}

void OracleStatementBackEnd::cleanUp()
{
    // deallocate statement handle
    if (stmtp_ != NULL)
    {
        OCIHandleFree(stmtp_, OCI_HTYPE_STMT);
        stmtp_ = NULL;
    }
}

void OracleStatementBackEnd::prepare(std::string const &query)
{
    sb4 stmtLen = static_cast<sb4>(query.size());
    sword res = OCIStmtPrepare(stmtp_,
        session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(query.c_str())),
        stmtLen, OCI_V7_SYNTAX, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }
}

StatementBackEnd::execFetchResult OracleStatementBackEnd::execute(int number)
{
    sword res = OCIStmtExecute(session_.svchp_, stmtp_, session_.errhp_,
        static_cast<ub4>(number), 0, 0, 0, OCI_DEFAULT);

    if (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO)
    {
        return eSuccess;
    }
    else if (res == OCI_NO_DATA)
    {
        return eNoData;
    }
    else
    {
        throwOracleSOCIError(res, session_.errhp_);
        return eNoData; // unreachable dummy return to please the compiler
    }
}

StatementBackEnd::execFetchResult OracleStatementBackEnd::fetch(int number)
{
    sword res = OCIStmtFetch(stmtp_, session_.errhp_,
        static_cast<ub4>(number), OCI_FETCH_NEXT, OCI_DEFAULT);

    if (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO)
    {
        return eSuccess;
    }
    else if (res == OCI_NO_DATA)
    {
        return eNoData;
    }
    else
    {
        throwOracleSOCIError(res, session_.errhp_);
        return eNoData; // unreachable dummy return to please the compiler
    }
}

int OracleStatementBackEnd::getNumberOfRows()
{
    int rows;
    sword res = OCIAttrGet(static_cast<dvoid*>(stmtp_),
        static_cast<ub4>(OCI_HTYPE_STMT), static_cast<dvoid*>(&rows),
        0, static_cast<ub4>(OCI_ATTR_ROWS_FETCHED), session_.errhp_);

    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return rows;
}

std::string OracleStatementBackEnd::rewriteForProcedureCall(
    std::string const &query)
{
    std::string newQuery("begin ");
    newQuery += query;
    newQuery += "; end;";
    return newQuery;
}

int OracleStatementBackEnd::prepareForDescribe()
{
    sword res = OCIStmtExecute(session_.svchp_, stmtp_, session_.errhp_,
        1, 0, 0, 0, OCI_DESCRIBE_ONLY);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    int cols;
    res = OCIAttrGet(static_cast<dvoid*>(stmtp_),
        static_cast<ub4>(OCI_HTYPE_STMT), static_cast<dvoid*>(&cols),
        0, static_cast<ub4>(OCI_ATTR_PARAM_COUNT), session_.errhp_);

    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return cols;
}

void OracleStatementBackEnd::describeColumn(int colNum, eDataType &type,
    std::string &columnName, int &size, int &precision, int &scale,
    bool &nullOk)
{
    ub2 dbtype;
    text* dbname;
    ub4 nameLength;

    ub2 dbsize;
    sb2 dbprec;
    ub1 dbscale; //sb2 in some versions of Oracle?
    ub1 dbnullok;

    // Get the column handle
    OCIParam* colhd;
    sword res = OCIParamGet(reinterpret_cast<dvoid*>(stmtp_),
        static_cast<ub4>(OCI_HTYPE_STMT),
        reinterpret_cast<OCIError*>(session_.errhp_),
        reinterpret_cast<dvoid**>(&colhd),
        static_cast<ub4>(colNum));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
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
        throwOracleSOCIError(res, session_.errhp_);
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
        throwOracleSOCIError(res, session_.errhp_);
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
        throwOracleSOCIError(res, session_.errhp_);
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
        throwOracleSOCIError(res, session_.errhp_);
    }

    // get the scale
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbscale),
        0,
        static_cast<ub4>(OCI_ATTR_SCALE),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    // get the null allowed flag
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbnullok),
        0,
        static_cast<ub4>(OCI_ATTR_IS_NULL),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    columnName.assign(dbname, dbname + nameLength);
    size = static_cast<int>(dbsize);
    precision = static_cast<int>(dbprec);
    scale = static_cast<int>(dbscale);
    nullOk = (dbnullok != 0);

    switch (dbtype)
    {
    case SQLT_CHR:
    case SQLT_AFC:
        type = eString;
        break;
    case SQLT_NUM:
        if (scale > 0)
        {
            type = eDouble;
        }
        else if (precision < std::numeric_limits<int>::digits10)
        {
            type = eInteger;
        }
        else
        {
            type = eUnsignedLong;
        }
        break;
    case SQLT_DAT:
        type = eDate;
        break;
    }
}

std::size_t OracleStatementBackEnd::columnSize(int position)
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
        throwOracleSOCIError(res, session_.errhp_);
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
        throwOracleSOCIError(res, session_.errhp_);
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
        throwOracleSOCIError(res, session_.errhp_);
    }

    return static_cast<std::size_t>(colSize);
}

OracleStandardIntoTypeBackEnd * OracleStatementBackEnd::makeIntoTypeBackEnd()
{
    return new OracleStandardIntoTypeBackEnd(*this);
}

OracleStandardUseTypeBackEnd * OracleStatementBackEnd::makeUseTypeBackEnd()
{
    return new OracleStandardUseTypeBackEnd(*this);
}

OracleVectorIntoTypeBackEnd *
OracleStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    return new OracleVectorIntoTypeBackEnd(*this);
}

OracleVectorUseTypeBackEnd *
OracleStatementBackEnd::makeVectorUseTypeBackEnd()
{
    return new OracleVectorUseTypeBackEnd(*this);
}

void OracleStandardIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType = 0; // dummy initialization to please the compiler
    sb4 size = 0;       // also dummy

    switch (type)
    {
    // simple cases
    case eXChar:
        oracleType = SQLT_AFC;
        size = sizeof(char);
        break;
    case eXShort:
        oracleType = SQLT_INT;
        size = sizeof(short);
        break;
    case eXInteger:
        oracleType = SQLT_INT;
        size = sizeof(int);
        break;
    case eXUnsignedLong:
        oracleType = SQLT_UIN;
        size = sizeof(unsigned long);
        break;
    case eXDouble:
        oracleType = SQLT_FLT;
        size = sizeof(double);
        break;

    // cases that require adjustments and buffer management
    case eXCString:
        {
            details::CStringDescriptor *desc
                = static_cast<CStringDescriptor *>(data);
            oracleType = SQLT_STR;
            data = desc->str_;
            size = static_cast<sb4>(desc->bufSize_);
        }
        break;
    case eXStdString:
        oracleType = SQLT_STR;
        size = 4000;               // this is also Oracle limit
        buf_ = new char[size];
        data = buf_;
        break;
    case eXStdTm:
        oracleType = SQLT_DAT;
        size = 7 * sizeof(ub1);
        buf_ = new char[size];
        data = buf_;
        break;

    // cases that require special handling
    case eXStatement:
        {
            oracleType = SQLT_RSET;

            Statement *st = static_cast<Statement *>(data);
            st->alloc();

            OracleStatementBackEnd *stbe
                = static_cast<OracleStatementBackEnd *>(st->getBackEnd());
            size = 0;
            data = &stbe->stmtp_;
        }
        break;
    case eXRowID:
        {
            oracleType = SQLT_RDD;

            RowID *rid = static_cast<RowID *>(data);

            OracleRowIDBackEnd *rbe
                = static_cast<OracleRowIDBackEnd *>(rid->getBackEnd());

            size = 0;
            data = &rbe->rowidp_;
        }
        break;
    case eXBLOB:
        {
            oracleType = SQLT_BLOB;

            BLOB *b = static_cast<BLOB *>(data);

            OracleBLOBBackEnd *bbe
                = static_cast<OracleBLOBBackEnd *>(b->getBackEnd());

            size = 0;
            data = &bbe->lobp_;
        }
        break;
    }

    sword res = OCIDefineByPos(statement_.stmtp_, &defnp_,
            statement_.session_.errhp_,
            position++, data, size, oracleType,
            &indOCIHolder_, 0, &rCode_, OCI_DEFAULT);

    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleStandardIntoTypeBackEnd::preFetch()
{
    // nothing to do except with Statement into objects

    if (type_ == eXStatement)
    {
        Statement *st = static_cast<Statement *>(data_);
        st->unDefAndBind();
    }
}

void OracleStandardIntoTypeBackEnd::postFetch(
    bool gotData, bool calledFromFetch, eIndicator *ind)
{
    // first, deal with data
    if (gotData)
    {
        // only std::string, std::tm and Statement need special handling
        if (type_ == eXStdString)
        {
            std::string *s = static_cast<std::string *>(data_);
            *s = buf_;
        }
        else if (type_ == eXStdTm)
        {
            std::tm *t = static_cast<std::tm *>(data_);

            t->tm_isdst = -1;

            t->tm_year = (buf_[0] - 100) * 100 + buf_[1] - 2000;
            t->tm_mon = buf_[2] - 1;
            t->tm_mday = buf_[3];
            t->tm_hour = buf_[4] - 1;
            t->tm_min = buf_[5] - 1;
            t->tm_sec = buf_[6] - 1;

            // normalize and compute the remaining fields
            std::mktime(t);
        }
        else if (type_ == eXStatement)
        {
            Statement *st = static_cast<Statement *>(data_);
            st->defineAndBind();
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
        if (gotData == false)
        {
            *ind = eNoData;
        }
        else
        {
            if (indOCIHolder_ == 0)
            {
                *ind = eOK;
            }
            else if (indOCIHolder_ == -1)
            {
                *ind = eNull;
            }
            else
            {
                *ind = eTruncated;
            }
        }
    }
    else
    {
        if (indOCIHolder_ == -1)
        {
            // fetched null and no indicator - programming error!
            throw SOCIError("Null value fetched and no indicator defined.");
        }

        if (gotData == false)
        {
            // no data fetched and no indicator - programming error!
            throw SOCIError("No data fetched and no indicator defined.");
        }
    }
}

void OracleStandardIntoTypeBackEnd::cleanUp()
{
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

void OracleVectorIntoTypeBackEnd::prepareIndicators(std::size_t size)
{
    if (size == 0)
    {
         throw SOCIError("Vectors of size 0 are not allowed.");
    }

    indOCIHolderVec_.resize(size);
    indOCIHolders_ = &indOCIHolderVec_[0];

    sizes_.resize(size);
    rCodes_.resize(size);
}

void OracleVectorIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType = 0; // dummy initialization to please the compiler
    sb4 size = 0;       // also dummy

    switch (type)
    {
    // simple cases
    case eXChar:
        {
            oracleType = SQLT_AFC;
            size = sizeof(char);
            std::vector<char> *vp = static_cast<std::vector<char> *>(data);
            std::vector<char> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXShort:
        {
            oracleType = SQLT_INT;
            size = sizeof(short);
            std::vector<short> *vp = static_cast<std::vector<short> *>(data);
            std::vector<short> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXInteger:
        {
            oracleType = SQLT_INT;
            size = sizeof(int);
            std::vector<int> *vp = static_cast<std::vector<int> *>(data);
            std::vector<int> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXUnsignedLong:
        {
            oracleType = SQLT_UIN;
            size = sizeof(unsigned long);
            std::vector<unsigned long> *vp
                = static_cast<std::vector<unsigned long> *>(data);
            std::vector<unsigned long> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXDouble:
        {
            oracleType = SQLT_FLT;
            size = sizeof(double);
            std::vector<double> *vp = static_cast<std::vector<double> *>(data);
            std::vector<double> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;

    // cases that require adjustments and buffer management

    case eXStdString:
        {
            oracleType = SQLT_CHR;
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data);
            colSize_ = statement_.columnSize(position) + 1;
            std::size_t bufSize = colSize_ * v->size();
            buf_ = new char[bufSize];

            prepareIndicators(v->size());

            size = static_cast<sb4>(colSize_);
            data = buf_;
        }
        break;
    case eXStdTm:
        {
            oracleType = SQLT_DAT;
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data);

            prepareIndicators(v->size());

            size = 7; // 7 is the size of SQLT_DAT
            std::size_t bufSize = size * v->size();

            buf_ = new char[bufSize];
            data = buf_;
        }
        break;

    case eXCString:   break; // not supported
                             // (there is no specialization
                             // of IntoType<vector<char*> >)
    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    }

    sword res = OCIDefineByPos(statement_.stmtp_, &defnp_,
        statement_.session_.errhp_,
        position++, data, size, oracleType,
        indOCIHolders_, &sizes_[0], &rCodes_[0], OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleVectorIntoTypeBackEnd::preFetch()
{
    // nothing to do for the supported types
}

void OracleVectorIntoTypeBackEnd::postFetch(bool gotData, eIndicator *ind)
{
    if (gotData)
    {
        // first, deal with data

        // only std::string, std::tm and Statement need special handling
        if (type_ == eXStdString)
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data_);

            std::vector<std::string> &v(*vp);

            char *pos = buf_;
            std::size_t const vsize = v.size();
            for (std::size_t i = 0; i != vsize; ++i)
            {
                v[i].assign(pos, sizes_[i]);
                pos += colSize_;
            }
        }
        else if (type_ == eXStdTm)
        {
            std::vector<std::tm> *vp
                = static_cast<std::vector<std::tm> *>(data_);

            std::vector<std::tm> &v(*vp);

            ub1 *pos = reinterpret_cast<ub1*>(buf_);
            std::size_t const vsize = v.size();
            for (std::size_t i = 0; i != vsize; ++i)
            {
                std::tm t;
                t.tm_isdst = -1;

                t.tm_year = (*pos++ - 100) * 100;
                t.tm_year += *pos++ - 2000;
                t.tm_mon = *pos++ - 1;
                t.tm_mday = *pos++;
                t.tm_hour = *pos++ - 1;
                t.tm_min = *pos++ - 1;
                t.tm_sec = *pos++ - 1;

                // normalize and compute the remaining fields
                std::mktime(&t);
                v[i] = t;
            }
        }
        else if (type_ == eXStatement)
        {
            Statement *st = static_cast<Statement *>(data_);
            st->defineAndBind();
        }

        // then - deal with indicators
        if (ind != NULL)
        {
            std::size_t const indSize = indOCIHolderVec_.size();
            for (std::size_t i = 0; i != indSize; ++i)
            {
                if (indOCIHolderVec_[i] == 0)
                {
                    ind[i] = eOK;
                }
                else if (indOCIHolderVec_[i] == -1)
                {
                    ind[i] = eNull;
                }
                else
                {
                    ind[i] = eTruncated;
                }
            }
        }
        else
        {
            std::size_t const indSize = indOCIHolderVec_.size();
            for (std::size_t i = 0; i != indSize; ++i)
            {
                if (indOCIHolderVec_[i] == -1)
                {
                    // fetched null and no indicator - programming error!
                    throw SOCIError(
                        "Null value fetched and no indicator defined.");
                }
            }
        }
    }
    else // gotData == false
    {
        // nothing to do here, vectors are truncated anyway
    }
}

void OracleVectorIntoTypeBackEnd::resize(std::size_t sz)
{
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            std::vector<char> *v = static_cast<std::vector<char> *>(data_);
            v->resize(sz);
        }
        break;
    case eXShort:
        {
            std::vector<short> *v = static_cast<std::vector<short> *>(data_);
            v->resize(sz);
        }
        break;
    case eXInteger:
        {
            std::vector<int> *v = static_cast<std::vector<int> *>(data_);
            v->resize(sz);
        }
        break;
    case eXUnsignedLong:
        {
            std::vector<unsigned long> *v
                = static_cast<std::vector<unsigned long> *>(data_);
            v->resize(sz);
        }
        break;
    case eXDouble:
        {
            std::vector<double> *v
                = static_cast<std::vector<double> *>(data_);
            v->resize(sz);
        }
        break;
    case eXStdString:
        {
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data_);
            v->resize(sz);
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data_);
            v->resize(sz);
        }
        break;

    case eXCString:   break; // not supported
    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    }
}

std::size_t OracleVectorIntoTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            std::vector<char> *v = static_cast<std::vector<char> *>(data_);
            sz = v->size();
        }
        break;
    case eXShort:
        {
            std::vector<short> *v = static_cast<std::vector<short> *>(data_);
            sz = v->size();
        }
        break;
    case eXInteger:
        {
            std::vector<int> *v = static_cast<std::vector<int> *>(data_);
            sz = v->size();
        }
        break;
    case eXUnsignedLong:
        {
            std::vector<unsigned long> *v
                = static_cast<std::vector<unsigned long> *>(data_);
            sz = v->size();
        }
        break;
    case eXDouble:
        {
            std::vector<double> *v
                = static_cast<std::vector<double> *>(data_);
            sz = v->size();
        }
        break;
    case eXStdString:
        {
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data_);
            sz = v->size();
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data_);
            sz = v->size();
        }
        break;

    case eXCString:   break; // not supported
    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    }

    return sz;
}

void OracleVectorIntoTypeBackEnd::cleanUp()
{
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

void OracleStandardUseTypeBackEnd::prepareForBind(
    void *&data, sb4 &size, ub2 &oracleType)
{
    switch (type_)
    {
    // simple cases
    case eXChar:
        oracleType = SQLT_AFC;
        size = sizeof(char);
        break;
    case eXShort:
        oracleType = SQLT_INT;
        size = sizeof(short);
        break;
    case eXInteger:
        oracleType = SQLT_INT;
        size = sizeof(int);
        break;
    case eXUnsignedLong:
        oracleType = SQLT_UIN;
        size = sizeof(unsigned long);
        break;
    case eXDouble:
        oracleType = SQLT_FLT;
        size = sizeof(double);
        break;

    // cases that require adjustments and buffer management
    case eXCString:
        {
            details::CStringDescriptor *desc
                = static_cast<CStringDescriptor *>(data);
            oracleType = SQLT_STR;
            data = desc->str_;
            size = static_cast<sb4>(desc->bufSize_);
        }
        break;
    case eXStdString:
        oracleType = SQLT_STR;
        size = 4000;               // this is also Oracle limit
        buf_ = new char[size];
        data = buf_;
        break;
    case eXStdTm:
        oracleType = SQLT_DAT;
        size = 7 * sizeof(ub1);
        buf_ = new char[size];
        data = buf_;
        break;

    // cases that require special handling
    case eXStatement:
        {
            oracleType = SQLT_RSET;

            Statement *st = static_cast<Statement *>(data);
            st->alloc();

            OracleStatementBackEnd *stbe
                = static_cast<OracleStatementBackEnd *>(st->getBackEnd());
            size = 0;
            data = &stbe->stmtp_;
        }
        break;
    case eXRowID:
        {
            oracleType = SQLT_RDD;

            RowID *rid = static_cast<RowID *>(data);

            OracleRowIDBackEnd *rbe
                = static_cast<OracleRowIDBackEnd *>(rid->getBackEnd());

            size = 0;
            data = &rbe->rowidp_;
        }
        break;
    case eXBLOB:
        {
            oracleType = SQLT_BLOB;

            BLOB *b = static_cast<BLOB *>(data);

            OracleBLOBBackEnd *bbe
                = static_cast<OracleBLOBBackEnd *>(b->getBackEnd());

            size = 0;
            data = &bbe->lobp_;
        }
        break;
    }
}

void OracleStandardUseTypeBackEnd::bindByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepareForBind(data, size, oracleType);

    sword res = OCIBindByPos(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        position++, data, size, oracleType,
        &indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleStandardUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepareForBind(data, size, oracleType);

    sword res = OCIBindByName(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(name.c_str())),
        static_cast<sb4>(name.size()),
        data, size, oracleType,
        &indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleStandardUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // first deal with data
    if (type_ == eXStdString)
    {
        std::string *s = static_cast<std::string *>(data_);

        std::size_t const bufSize = 4000;
        std::size_t const sSize = s->size();
        std::size_t const toCopy =
            sSize < bufSize -1 ? sSize + 1 : bufSize - 1;
        strncpy(buf_, s->c_str(), toCopy);
        buf_[toCopy] = '\0';
    }
    else if (type_ == eXStdTm)
    {
        std::tm *t = static_cast<std::tm *>(data_);

        buf_[0] = static_cast<ub1>(100 + (1900 + t->tm_year) / 100);
        buf_[1] = static_cast<ub1>(100 + t->tm_year % 100);
        buf_[2] = static_cast<ub1>(t->tm_mon + 1);
        buf_[3] = static_cast<ub1>(t->tm_mday);
        buf_[4] = static_cast<ub1>(t->tm_hour + 1);
        buf_[5] = static_cast<ub1>(t->tm_min + 1);
        buf_[6] = static_cast<ub1>(t->tm_sec + 1);
    }
    else if (type_ == eXStatement)
    {
        Statement *s = static_cast<Statement *>(data_);

        s->unDefAndBind();
    }

    // then handle indicators
    if (ind != NULL && *ind == eNull)
    {
        indOCIHolder_ = -1; // null
    }
    else
    {
        indOCIHolder_ = 0;  // value is OK
    }
}

void OracleStandardUseTypeBackEnd::postUse(bool gotData, eIndicator *ind)
{
    // first, deal with data
    if (gotData)
    {
        if (type_ == eXStdString)
        {
            std::string *s = static_cast<std::string *>(data_);

            *s = buf_;
        }
        else if (type_ == eXStdTm)
        {
            std::tm *t = static_cast<std::tm *>(data_);

            t->tm_isdst = -1;
            t->tm_year = (buf_[0] - 100) * 100 + buf_[1] - 2000;
            t->tm_mon = buf_[2] - 1;
            t->tm_mday = buf_[3];
            t->tm_hour = buf_[4] - 1;
            t->tm_min = buf_[5] - 1;
            t->tm_sec = buf_[6] - 1;

            // normalize and compute the remaining fields
            std::mktime(t);
        }
        else if (type_ == eXStatement)
        {
            Statement *s = static_cast<Statement *>(data_);
            s->defineAndBind();
        }
    }

    if (ind != NULL)
    {
        if (gotData == false)
        {
            *ind = eNoData;
        }
        else
        {
            if (indOCIHolder_ == 0)
            {
                *ind = eOK;
            }
            else if (indOCIHolder_ == -1)
            {
                *ind = eNull;
            }
            else
            {
                *ind = eTruncated;
            }
        }
    }
    else
    {
        if (indOCIHolder_ == -1)
        {
            // fetched null and no indicator - programming error!
            throw SOCIError("Null value fetched and no indicator defined.");
        }

        if (gotData == false)
        {
            // no data fetched and no indicator - programming error!
            throw SOCIError("No data fetched and no indicator defined.");
        }
    }
}

void OracleStandardUseTypeBackEnd::cleanUp()
{
    if (bindp_ != NULL)
    {
        OCIHandleFree(bindp_, OCI_HTYPE_DEFINE);
        bindp_ = NULL;
    }

    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}

void OracleVectorUseTypeBackEnd::prepareIndicators(std::size_t size)
{
    if (size == 0)
    {
         throw SOCIError("Vectors of size 0 are not allowed.");
    }

    indOCIHolderVec_.resize(size);
    indOCIHolders_ = &indOCIHolderVec_[0];
}

void OracleVectorUseTypeBackEnd::prepareForBind(
    void *&data, sb4 &size, ub2 &oracleType)
{
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            oracleType = SQLT_AFC;
            size = sizeof(char);
            std::vector<char> *vp = static_cast<std::vector<char> *>(data);
            std::vector<char> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXShort:
        {
            oracleType = SQLT_INT;
            size = sizeof(short);
            std::vector<short> *vp = static_cast<std::vector<short> *>(data);
            std::vector<short> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXInteger:
        {
            oracleType = SQLT_INT;
            size = sizeof(int);
            std::vector<int> *vp = static_cast<std::vector<int> *>(data);
            std::vector<int> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXUnsignedLong:
        {
            oracleType = SQLT_UIN;
            size = sizeof(unsigned long);
            std::vector<unsigned long> *vp
                = static_cast<std::vector<unsigned long> *>(data);
            std::vector<unsigned long> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXDouble:
        {
            oracleType = SQLT_FLT;
            size = sizeof(double);
            std::vector<double> *vp = static_cast<std::vector<double> *>(data);
            std::vector<double> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;

    // cases that require adjustments and buffer management

    case eXStdString:
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data);
            std::vector<std::string> &v(*vp);

            std::size_t maxSize = 0;
            std::size_t const vecSize = v.size();
            prepareIndicators(vecSize);
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                std::size_t sz = v[i].length();
                sizes_.push_back(static_cast<ub2>(sz));
                maxSize = sz > maxSize ? sz : maxSize;
            }

            buf_ = new char[maxSize * vecSize];
            char *pos = buf_;
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                strncpy(pos, v[i].c_str(), v[i].length());
                pos += maxSize;
            }

            oracleType = SQLT_CHR;
            data = buf_;
            size = static_cast<sb4>(maxSize);
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *vp
                = static_cast<std::vector<std::tm> *>(data);

            prepareIndicators(vp->size());

            sb4 const dlen = 7; // size of SQLT_DAT
            buf_ = new char[dlen * vp->size()];

            oracleType = SQLT_DAT;
            data = buf_;
            size = dlen;
        }
        break;

    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    case eXCString:   break; // not supported
    }
}

void OracleVectorUseTypeBackEnd::bindByPos(int &position,
        void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepareForBind(data, size, oracleType);

    ub2 *sizesP = 0; // used only for std::string
    if (type == eXStdString)
    {
        sizesP = &sizes_[0];
    }

    sword res = OCIBindByPos(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        position++, data, size, oracleType,
        indOCIHolders_, sizesP, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleVectorUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepareForBind(data, size, oracleType);

    ub2 *sizesP = 0; // used only for std::string
    if (type == eXStdString)
    {
        sizesP = &sizes_[0];
    }

    sword res = OCIBindByName(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(name.c_str())),
        static_cast<sb4>(name.size()),
        data, size, oracleType,
        indOCIHolders_, sizesP, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleVectorUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // first deal with data
    if (type_ == eXStdString)
    {
        // nothing to do - it's already done during bind
        // (and it's probably impossible to separate them, because
        // changes in the string size could not be handled here)
    }
    else if (type_ == eXStdTm)
    {
        std::vector<std::tm> *vp
            = static_cast<std::vector<std::tm> *>(data_);
        std::vector<std::tm> &v(*vp);

        ub1* pos = reinterpret_cast<ub1*>(buf_);
        std::size_t const vsize = v.size();
        for (std::size_t i = 0; i != vsize; ++i)
        {
            *pos++ = static_cast<ub1>(100 + (1900 + v[i].tm_year) / 100);
            *pos++ = static_cast<ub1>(100 + v[i].tm_year % 100);
            *pos++ = static_cast<ub1>(v[i].tm_mon + 1);
            *pos++ = static_cast<ub1>(v[i].tm_mday);
            *pos++ = static_cast<ub1>(v[i].tm_hour + 1);
            *pos++ = static_cast<ub1>(v[i].tm_min + 1);
            *pos++ = static_cast<ub1>(v[i].tm_sec + 1);
        }
    }

    // then handle indicators
    if (ind != NULL)
    {
        std::size_t const vsize = size();
        for (std::size_t i = 0; i != vsize; ++i, ++ind)
        {
            if (*ind == eNull)
            {
                indOCIHolderVec_[i] = -1; // null
            }
            else
            {
                indOCIHolderVec_[i] = 0;  // value is OK
            }
        }
    }
    else
    {
        // no indicators - treat all fields as OK
        std::size_t const vsize = size();
        for (std::size_t i = 0; i != vsize; ++i, ++ind)
        {
            indOCIHolderVec_[i] = 0;  // value is OK
        }
    }
}

std::size_t OracleVectorUseTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            std::vector<char> *vp = static_cast<std::vector<char> *>(data_);
            sz = vp->size();
        }
        break;
    case eXShort:
        {
            std::vector<short> *vp = static_cast<std::vector<short> *>(data_);
            sz = vp->size();
        }
        break;
    case eXInteger:
        {
            std::vector<int> *vp = static_cast<std::vector<int> *>(data_);
            sz = vp->size();
        }
        break;
    case eXUnsignedLong:
        {
            std::vector<unsigned long> *vp
                = static_cast<std::vector<unsigned long> *>(data_);
            sz = vp->size();
        }
        break;
    case eXDouble:
        {
            std::vector<double> *vp
                = static_cast<std::vector<double> *>(data_);
            sz = vp->size();
        }
        break;
    case eXStdString:
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data_);
            sz = vp->size();
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *vp
                = static_cast<std::vector<std::tm> *>(data_);
            sz = vp->size();
        }
        break;

    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    case eXCString:   break; // not supported
    }

    return sz;
}

void OracleVectorUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }

    if (bindp_ != NULL)
    {
        OCIHandleFree(bindp_, OCI_HTYPE_DEFINE);
        bindp_ = NULL;
    }
}

OracleRowIDBackEnd::OracleRowIDBackEnd(OracleSessionBackEnd &session)
{
    sword res = OCIDescriptorAlloc(session.envhp_,
        reinterpret_cast<dvoid**>(&rowidp_), OCI_DTYPE_ROWID, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot allocate the ROWID descriptor");
    }
}

OracleRowIDBackEnd::~OracleRowIDBackEnd()
{
    OCIDescriptorFree(rowidp_, OCI_DTYPE_ROWID);
}

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


// concrete factory for Oracle concrete strategies
struct OracleBackEndFactory : BackEndFactory
{
    virtual OracleSessionBackEnd * makeSession(
        std::string const &connectString) const
    {
        std::string serviceName, userName, password;
        chopConnectString(connectString, serviceName, userName, password);

        return new OracleSessionBackEnd(serviceName, userName, password);
    }

} oracleBEF;

namespace
{

// global object for automatic factory registration
struct OracleAutoRegister
{
    OracleAutoRegister()
    {
        theBEFRegistry().registerMe("oracle", &oracleBEF);
    }
} oracleAutoRegister;

} // namespace anonymous
