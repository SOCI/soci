//
// Copyright (C) 2004, 2005 Maciej Sobczak, Steve Hutton
// 
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#include "soci.h"
#include <limits>
#include <cmath>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;

SOCIError::SOCIError(std::string const & msg, int errNum)
    : std::runtime_error(msg), errNum_(errNum)
{
}

namespace SOCI {

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

void throwSOCIError(sword res, OCIError *errhp)
{
    std::string msg;
    int errNum;

    getErrorDetails(res, errhp, msg, errNum);
    throw SOCIError(msg, errNum);
}

} // namespace SOCI

Session::Session()
    : once(this), prepare(this),
      envhp_(NULL), srvhp_(NULL), errhp_(NULL), svchp_(NULL), usrhp_(NULL)
{
}

Session::Session(std::string const & serviceName,
    std::string const & userName, std::string const & password)
    : once(this), prepare(this),
      envhp_(NULL), srvhp_(NULL), errhp_(NULL), svchp_(NULL), usrhp_(NULL)
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
        throw SOCIError(msg, errNum);
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
        throw SOCIError(msg, errNum);
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
        throw SOCIError(msg, errNum);
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
        throw SOCIError(msg, errNum);
    }
}

Session::~Session()
{
    cleanUp();
}

void Session::commit()
{
    sword res = OCITransCommit(svchp_, errhp_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, errhp_);
    }
}

void Session::rollback()
{
    sword res = OCITransRollback(svchp_, errhp_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, errhp_);
    }
}

void Session::cleanUp()
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

Statement::Statement(Session &s)
    : stmtp_(0), session_(s), row_(0), fetchSize_(1)
{
}

Statement::Statement(PrepareTempType const &prep)
    : stmtp_(0), session_(*prep.getPrepareInfo()->session_), row_(0), fetchSize_(1)
{
    RefCountedPrepareInfo *prepInfo = prep.getPrepareInfo();

    // take all bind/define info
    intos_.swap(prepInfo->intos_);
    uses_.swap(prepInfo->uses_);

    // allocate handle
    alloc();

    // prepare the statement
    prepare(prepInfo->getQuery());

    defineAndBind();
}

Statement::~Statement()
{
    cleanUp();
}

void Statement::alloc()
{
    sword res = OCIHandleAlloc(session_.envhp_,
        reinterpret_cast<dvoid**>(&stmtp_),
        OCI_HTYPE_STMT, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot allocate statement handle");
    }
}

void Statement::exchange(IntoTypePtr const &i)
{
    intos_.push_back(i.get());
    i.release();
}

void Statement::exchange(UseTypePtr const &u)
{
    uses_.push_back(u.get());
    u.release();
}

void Statement::cleanUp()
{
    // deallocate all bind and define objects
    for (size_t i = intos_.size(); i > 0; --i)
    {
        intos_[i - 1]->cleanUp();
        delete intos_[i - 1];
        intos_.resize(i - 1);
    }

    for (size_t i = uses_.size(); i > 0; --i)
    {
        uses_[i - 1]->cleanUp();
        delete uses_[i - 1];
        uses_.resize(i - 1);
    }

    // deallocate statement handle
    if (stmtp_ != NULL)
    {
        OCIHandleFree(stmtp_, OCI_HTYPE_STMT);
        stmtp_ = NULL;
    }
}

void Statement::prepare(std::string const &query)
{
    sb4 stmtLen = static_cast<sb4>(query.size());
    sword res = OCIStmtPrepare(stmtp_,
        session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(query.c_str())),
        stmtLen, OCI_V7_SYNTAX, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, session_.errhp_);
    }
}

void Statement::defineAndBind()
{
    define();

    int bindPosition = 1;
    for (size_t i = 0; i != uses_.size(); ++i)
        uses_[i]->bind(*this, bindPosition);

    if (row_)
        define();
}

void Statement::unDefAndBind()
{
    for (size_t i = intos_.size(); i > 0; --i)
    {
        intos_[i - 1]->cleanUp();
    }

    for (size_t i = uses_.size(); i > 0; --i)
    {
        uses_[i - 1]->cleanUp();
    }
}

bool Statement::execute(int num)
{
    if (num > 0)
    {
        preFetch();
        preUse();
    }

    fetchSize_ = intosSize();
    int bindSize = usesSize();

    if (bindSize > 1 && fetchSize_ > 1)
    {
        throw SOCIError("Bulk insert/update and bulk select not allowed in same query");
    }

    if (num > 0)
        num = std::max(num, std::max(fetchSize_, bindSize));

    bool gotData;
    sword res = OCIStmtExecute(session_.svchp_, stmtp_, session_.errhp_,
                               static_cast<ub4>(num), 0, 0, 0, OCI_DEFAULT);

    if (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO)
    {
        gotData = num > 0 ? true : false;
    }
    else // res != OCI_SUCCESS
    {
        if (num == 0 || res != OCI_NO_DATA)
        {
            throwSOCIError(res, session_.errhp_);
        }
        else if (res == OCI_NO_DATA)
        {
            gotData = fetchSize_ > 1 ? resizeIntos() : false;
        }
        else
        {
            gotData = false;
        }
    }

    if (num > 0)
    {
        postFetch(gotData, false);
        postUse();
    }

    return gotData;
}

bool Statement::fetch()
{
    if (!fetchSize_)
        return false;

    bool gotData = false;
    int num = fetchSize_;
    sword res = OCIStmtFetch(stmtp_, session_.errhp_, num,
                            OCI_FETCH_NEXT, OCI_DEFAULT);

    if (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO)
    {
        gotData = true;
    }
    else
    {
        if (res == OCI_NO_DATA)
        {
            if (fetchSize_ > 1)
            {
                resizeIntos();
                gotData = true; 
                fetchSize_ = 0;
            }
            else
            {
                gotData = false;
            }
        }
        else
        {
            throwSOCIError(res, session_.errhp_);
        }
    }
    postFetch(gotData, true);
    return gotData;
}

int Statement::intosSize()
{
    int intosSize = 0;
    for (size_t i = 0; i != intos_.size(); ++i)
    {
        if (i==0)
        {
            intosSize = intos_[i]->size();
        }
        else if (intosSize != intos_[i]->size())
        {
            std::ostringstream msg;
            msg << "Bind variable size mismatch (into["<<i<<"] has size "
                << intos_[i]->size() << ", intos_[0] has size "<< intosSize <<std::endl;
            throw SOCIError(msg.str());            
        } 
    }
    return intosSize;
}

int Statement::usesSize()
{
    int usesSize = 0;
    for (size_t i = 0; i != uses_.size(); ++i)
    {
        if (i==0)
        {
            usesSize = uses_[i]->size();
        }
        else if (usesSize != uses_[i]->size())
        {
            std::ostringstream msg;
            msg << "Bind variable size mismatch (use["<<i<<"] has size "
                << uses_[i]->size() << ", use[0] has size "<< usesSize <<std::endl;
            throw SOCIError(msg.str());
        }
    }
    return usesSize;
}

bool Statement::resizeIntos()
{
    int rows;

    // Get The Actual Number Of Rows Fetched
    ub4 sizep = sizeof(ub4);
    OCIAttrGet(static_cast<dvoid*>(stmtp_), static_cast<ub4>(OCI_HTYPE_STMT),
    static_cast<dvoid*>(&rows), static_cast<ub4*>(&sizep), 
    static_cast<ub4>(OCI_ATTR_ROWS_FETCHED), session_.errhp_);

    for (size_t i = 0; i != intos_.size(); ++i)
    {
        intos_[i]->resize(rows);
    }

    return rows > 0 ? true : false;
}

void Statement::preFetch()
{
    for (size_t i = 0; i != intos_.size(); ++i) // todo for each?
        intos_[i]->preFetch();
}

void Statement::preUse()
{
    for (size_t i = 0; i != uses_.size(); ++i)
        uses_[i]->preUse();
}

void Statement::postFetch(bool gotData, bool calledFromFetch)
{
    for (size_t i = 0; i != intos_.size(); ++i)
        intos_[i]->postFetch(gotData, calledFromFetch);
}

void Statement::postUse()
{
    for (size_t i = 0; i != uses_.size(); ++i)
        uses_[i]->postUse();
}


void Statement::define()
{
    int definePosition = 1;
    const size_t sz = intos_.size();
    for (size_t i = 0; i < sz; ++i)
        intos_[i]->define(*this, definePosition);
}

// Map eDataTypes to stock types for dynamic result set support
namespace SOCI
{

template<> 
void Statement::bindInto<eString>()
{
    intoRow<std::string>();
}

template<> 
void Statement::bindInto<eDouble>()
{
    intoRow<double>();
}

template<>
void Statement::bindInto<eInteger>()
{
    intoRow<int>();
}

template<>
void Statement::bindInto<eUnsignedLong>()
{
    intoRow<unsigned long>();
}

template<> 
void Statement::bindInto<eDate>()
{
    intoRow<std::tm>();
}

} //namespace SOCI

void Statement::describe()
{
    delete intos_[0];
    intos_.clear();

    sword res = OCIStmtExecute(session_.svchp_, stmtp_, session_.errhp_,
                               1, 0, 0, 0, OCI_DESCRIBE_ONLY);
    if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);

    int numcols;
    res = OCIAttrGet(reinterpret_cast<dvoid*>(stmtp_), 
                     static_cast<ub4>(OCI_HTYPE_STMT), 
                     reinterpret_cast<dvoid*>(&numcols), 
                     0, 
                     static_cast<ub4>(OCI_ATTR_PARAM_COUNT), 
                     reinterpret_cast<OCIError*>(session_.errhp_));

    if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);

    OCIParam* colhd;
    for (int i = 1; i <= numcols; i++)
    {
        ub2 dtype;
        text* name;
        ub4 name_length;

        ub2 dbsize;
        sb2 prec;
        ub1 scale; //sb2 in some versions of Oracle?
        ub1 nullok;

        // Get the column handle
        res = OCIParamGet(reinterpret_cast<dvoid*>(stmtp_), 
                          static_cast<ub4>(OCI_HTYPE_STMT), 
                          reinterpret_cast<OCIError*>(session_.errhp_), 
                          reinterpret_cast<dvoid**>(&colhd), 
                          static_cast<ub4>(i));
        if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);

        // Get the column name
        res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd), 
                         static_cast<ub4>(OCI_DTYPE_PARAM),
                         reinterpret_cast<dvoid**>(&name),
                         reinterpret_cast<ub4*>(&name_length), 
                         static_cast<ub4>(OCI_ATTR_NAME), 
                         reinterpret_cast<OCIError*>(session_.errhp_));
        if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);

        // Get the column type
        res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd), 
                         static_cast<ub4>(OCI_DTYPE_PARAM),
                         reinterpret_cast<dvoid*>(&dtype),
                         0, 
                         static_cast<ub4>(OCI_ATTR_DATA_TYPE), 
                         reinterpret_cast<OCIError*>(session_.errhp_));
        if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);

        // get the data size
        res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd), 
                         static_cast<ub4>(OCI_DTYPE_PARAM),
                         reinterpret_cast<dvoid*>(&dbsize),
                         0, 
                         static_cast<ub4>(OCI_ATTR_DATA_SIZE), 
                         reinterpret_cast<OCIError*>(session_.errhp_));
        if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);

        // get the precision
        res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd), 
                         static_cast<ub4>(OCI_DTYPE_PARAM),
                         reinterpret_cast<dvoid*>(&prec),
                         0, 
                         static_cast<ub4>(OCI_ATTR_PRECISION), 
                         reinterpret_cast<OCIError*>(session_.errhp_));
        if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);
            
        // get the scale
        res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd), 
                         static_cast<ub4>(OCI_DTYPE_PARAM),
                         reinterpret_cast<dvoid*>(&scale),
                         0, 
                         static_cast<ub4>(OCI_ATTR_SCALE), 
                         reinterpret_cast<OCIError*>(session_.errhp_));
        if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);

        // get the null allowed flag
        res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd), 
                         static_cast<ub4>(OCI_DTYPE_PARAM),
                         reinterpret_cast<dvoid*>(&nullok),
                         0, 
                         static_cast<ub4>(OCI_ATTR_IS_NULL), 
                         reinterpret_cast<OCIError*>(session_.errhp_));
        if (res != OCI_SUCCESS) throwSOCIError(res, session_.errhp_);
            
        const int max_column_length = 50;
        char col_name[max_column_length + 1];
        strncpy(col_name, (char*)name, name_length);
        col_name[name_length] = '\0';

        ColumnProperties props;
        props.setName(col_name);
        props.setSize(dbsize);
        props.setPrecision(prec);
        props.setScale(scale);
        props.setNullOK(nullok != 0);

        switch(dtype)
        {
            case SQLT_CHR:
                bindInto<eString>();
                props.setDataType(eString);
                break;
            case SQLT_NUM:
                if (props.getScale() > 0)
                {
                    bindInto<eDouble>();
                    props.setDataType(eDouble);
                }
                else if (pow(10, props.getPrecision())
                    < std::numeric_limits<int>::max())
                {
                    bindInto<eInteger>();
                    props.setDataType(eInteger);
                }
                else
                {
                    bindInto<eUnsignedLong>();
                    props.setDataType(eUnsignedLong);
                }
                break;
            case SQLT_DAT:
                bindInto<eDate>();
                props.setDataType(eDate);
                break;
            case SQLT_AFC:
                bindInto<eString>();
                props.setDataType(eString);
                break;
            default:
                std::ostringstream msg;
                msg << "db column type " << dtype 
                    <<" not supported for dynamic selects"<<std::endl;
                throw SOCIError(msg.str());
        }
        row_->addProperties(props);
    }
}



Procedure::Procedure(PrepareTempType const &prep)
    : Statement(*prep.getPrepareInfo()->session_)
{
    stmtp_ = 0;
    RefCountedPrepareInfo *prepInfo = prep.getPrepareInfo();

    // take all bind/define info
    intos_.swap(prepInfo->intos_);
    uses_.swap(prepInfo->uses_);

    // allocate handle
    alloc();

    // prepare the statement
    prepare("begin " + prepInfo->getQuery() + "; end;");

    defineAndBind();
}

RefCountedStatement::~RefCountedStatement()
{
    try
    {
        st_.alloc();
        st_.prepare(query_.str());
        st_.defineAndBind();
        st_.execute(1);
    }
    catch (...)
    {
        st_.cleanUp();
        throw;
    }

    st_.cleanUp();
}

void RefCountedPrepareInfo::exchange(IntoTypePtr const &i)
{
    intos_.push_back(i.get());
    i.release();
}

void RefCountedPrepareInfo::exchange(UseTypePtr const &u)
{
    uses_.push_back(u.get());
    u.release();
}

RefCountedPrepareInfo::~RefCountedPrepareInfo()
{
    // deallocate all bind and define objects
    for (size_t i = intos_.size(); i > 0; --i)
    {
        delete intos_[i - 1];
        intos_.resize(i - 1);
    }

    for (size_t i = uses_.size(); i > 0; --i)
    {
        delete uses_[i - 1];
        uses_.resize(i - 1);
    }
}

OnceTempType::OnceTempType(Session &s)
    : rcst_(new RefCountedStatement(s))
{
}

OnceTempType::OnceTempType(OnceTempType const &o)
    :rcst_(o.rcst_)
{
    rcst_->incRef();
}

OnceTempType & OnceTempType::operator=(OnceTempType const &o)
{
    o.rcst_->incRef();
    rcst_->decRef();
    rcst_ = o.rcst_;

    return *this;
}

OnceTempType::~OnceTempType()
{
    rcst_->decRef();
}

OnceTempType & OnceTempType::operator,(IntoTypePtr const &i)
{
    rcst_->exchange(i);
    return *this;
}

OnceTempType & OnceTempType::operator,(UseTypePtr const &u)
{
    rcst_->exchange(u);
    return *this;
}

PrepareTempType::PrepareTempType(Session &s)
    : rcpi_(new RefCountedPrepareInfo(s))
{
}

PrepareTempType::PrepareTempType(PrepareTempType const &o)
    :rcpi_(o.rcpi_)
{
    rcpi_->incRef();
}

PrepareTempType & PrepareTempType::operator=(PrepareTempType const &o)
{
    o.rcpi_->incRef();
    rcpi_->decRef();
    rcpi_ = o.rcpi_;

    return *this;
}

PrepareTempType::~PrepareTempType()
{
    rcpi_->decRef();
}

PrepareTempType & PrepareTempType::operator,(IntoTypePtr const &i)
{
    rcpi_->exchange(i);
    return *this;
}

PrepareTempType & PrepareTempType::operator,(UseTypePtr const &u)
{
    rcpi_->exchange(u);
    return *this;
}

// implementation of into and use types

// standard types

void StandardIntoType::postFetch(bool gotData, bool calledFromFetch)
{
    if(gotData)
    {
        convertFrom();
    }

    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to set anything (fetch() will return false)
        return;
    }
    if (ind_ != NULL)
    {
        if (gotData == false)
        {
            *ind_ = eNoData;
        }
        else
        {
            if (indOCIHolder_ == 0)
                *ind_ = eOK;
            else if (indOCIHolder_ == -1)
                *ind_ = eNull;
            else
                *ind_ = eTruncated;
        }
    }
    else
    {
        if (gotData == false)
        {
            // no data fetched and no indicator - programming error!
            throw SOCIError("No data fetched and no indicator defined.");
        }
    }
}

void StandardIntoType::cleanUp()
{
    if (defnp_)
    {
        OCIHandleFree(defnp_, OCI_HTYPE_DEFINE);
        defnp_ = NULL;
    }
}

void StandardUseType::preUse()
{
    convertTo();

    if (ind_ != NULL && *ind_ == eNull)
        indOCIHolder_ = -1; // null
    else
        indOCIHolder_ = 0;  // value is OK
}

void StandardUseType::cleanUp()
{
    if (bindp_)
    {
        OCIHandleFree(bindp_, OCI_HTYPE_BIND);
        bindp_ = NULL;
    }
}

// vector based types


void VectorIntoType::postFetch(bool gotData, bool calledFromFetch)
{
    if(gotData)
    {
        convertFrom();
    }

    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to set anything (fetch() will return false)
        return;
    }
    if (ind_ != NULL)
    {
        for(size_t i=0; i<indOCIHolderVec_.size(); ++i, ++ind_)
        {
            if (gotData == false)
            {
                *ind_ = eNoData;
            }
            else
            {
                if (indOCIHolderVec_[i] == 0)
                    *ind_ = eOK;
                else if (indOCIHolderVec_[i] == -1)
                    *ind_ = eNull;
                else
                    *ind_ = eTruncated;
            }
        }
    }
    else
    {
        if (gotData == false)
        {
            // no data fetched and no indicator - programming error!
            throw SOCIError("No data fetched and no indicator defined.");
        }
    }
}

void VectorIntoType::cleanUp()
{
    if (defnp_)
    {
        OCIHandleFree(defnp_, OCI_HTYPE_DEFINE);
        defnp_ = NULL;
    }
}

void VectorUseType::preUse()
{
    convertTo();

    if (ind_ != NULL)
    {
        for (size_t i=0; i<indOCIHolderVec_.size(); ++i, ++ind_)
        { 
            if (*ind_ == eNull)
                indOCIHolderVec_[i] = -1; // null
            else
                indOCIHolderVec_[i] = 0;  // value is OK
        }
    }
}

void VectorUseType::cleanUp()
{
    if (bindp_)
    {
        OCIHandleFree(bindp_, OCI_HTYPE_BIND);
        bindp_ = NULL;
    }
}

// into and use types for char

void IntoType<char>::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &c_, sizeof(char), SQLT_AFC,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<char>::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &c_, sizeof(char), SQLT_AFC,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()),
            &c_, sizeof(char), SQLT_AFC,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

// into and use types for std::vector<char>
void IntoType<std::vector<char> >::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &v_.at(0), sizeof(char), SQLT_AFC,
        indOCIHolder_, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<std::vector<char> >::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &v_.at(0), sizeof(char), SQLT_AFC,
            indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()),
            &v_.at(0), sizeof(char), SQLT_AFC,
            indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}



// into and use types for unsigned long

void IntoType<unsigned long>::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &ul_, sizeof(unsigned long), SQLT_UIN,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<unsigned long>::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &ul_, sizeof(unsigned long), SQLT_UIN,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()),
            &ul_, sizeof(unsigned long), SQLT_UIN,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}
// into and use types for std::vector<unsigned long>

void IntoType<std::vector<unsigned long> >::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &v_.at(0), sizeof(unsigned long), SQLT_UIN,
        indOCIHolder_, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<std::vector<unsigned long> >::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;
    
    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &v_.at(0), sizeof(unsigned long), SQLT_UIN,
            indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()),
            &v_.at(0), sizeof(unsigned long), SQLT_UIN,
            indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

// into and use types for double

void IntoType<double>::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &d_, sizeof(double), SQLT_FLT,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<double>::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &d_, sizeof(double), SQLT_FLT,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), &d_, sizeof(double), SQLT_FLT,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

// into and use types for std::vector<double>

void IntoType<std::vector<double> >::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &v_.at(0), sizeof(double), SQLT_FLT,
        indOCIHolder_, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<std::vector<double> >::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &v_.at(0), sizeof(double), SQLT_FLT,
            indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), &v_.at(0), sizeof(double), SQLT_FLT,
            indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }

}

// into and use types for char*

void IntoType<char*>::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, str_, static_cast<sb4>(bufSize_), SQLT_STR,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<char*>::bind(Statement &st, int &position)
{
    st_ = &st;

    ub4 bufLen = static_cast<sb4>(strlen(str_) + 1);
    if (bufLen < 2)
        bufLen = 2;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, str_, bufLen, SQLT_STR,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), str_, bufLen, SQLT_STR,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

// into and use types for std::string

void IntoType<std::string>::define(Statement &st, int &position)
{
    st_ = &st;

    const size_t bufSize = 4000;
    buf_ = new char[bufSize];

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, buf_, bufSize, SQLT_STR,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void IntoType<std::string>::postFetch(bool gotData, bool calledFromFetch)
{
    if (gotData)
        s_ = buf_;

    StandardIntoType::postFetch(gotData, calledFromFetch);
}

void IntoType<std::string>::cleanUp()
{
    delete [] buf_;
    buf_ = NULL;

    StandardIntoType::cleanUp();
}

void UseType<std::string>::bind(Statement &st, int &position)
{
    st_ = &st;

    const size_t bufSize = 4000;
    buf_ = new char[bufSize];

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
                           position++, buf_, bufSize, SQLT_STR,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), buf_, bufSize, SQLT_STR,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<std::string>::preUse()
{
    StandardUseType::preUse();

    const size_t bufSize = 4000;
    const size_t sSize = s_.size();
    size_t toCopy = sSize < bufSize - 1 ? sSize + 1 : bufSize - 1;
    strncpy(buf_, s_.c_str(), toCopy);
    buf_[toCopy] = '\0';
}

void UseType<std::string>::postUse()
{
    s_ = buf_;

    StandardUseType::postUse();
}

void UseType<std::string>::cleanUp()
{
    delete [] buf_;
    buf_ = NULL;
}

// into and use types for std::vector<std::string>
size_t columnSize(Statement &st, int position)
{
    // TODO move this to the Statement class?
    //Do A Describe To Get The Size For This Column
    //Note: we may want to optimize so that the OCI_DESCRIBE_ONLY call happens only once per statement
    //Possibly use existing statement::describe() / make column prop access lazy at same time

    size_t colSize(0);

    sword res = OCIStmtExecute(st.session_.svchp_, st.stmtp_, st.session_.errhp_,
                               1, 0, 0, 0, OCI_DESCRIBE_ONLY);
    if (res != OCI_SUCCESS) throwSOCIError(res, st.session_.errhp_);
 
    // Get The Column Handle
    OCIParam* colhd;
    res = OCIParamGet( reinterpret_cast<dvoid*>(st.stmtp_), 
                       static_cast<ub4>(OCI_HTYPE_STMT), 
                       reinterpret_cast<OCIError*>(st.session_.errhp_), 
                       reinterpret_cast<dvoid**>(&colhd), 
                       static_cast<ub4>(position));
    if (res != OCI_SUCCESS) throwSOCIError(res, st.session_.errhp_);

     // Get The Data Size
    res = OCIAttrGet( reinterpret_cast<dvoid*>(colhd), 
                      static_cast<ub4>(OCI_DTYPE_PARAM),
                      reinterpret_cast<dvoid*>(&colSize),
                      0, 
                      static_cast<ub4>(OCI_ATTR_DATA_SIZE), 
                      reinterpret_cast<OCIError*>(st.session_.errhp_));
    if (res != OCI_SUCCESS) throwSOCIError(res, st.session_.errhp_);

    return colSize;
}


void IntoType<std::vector<std::string> >::define(Statement &st, int &position)
{
    st_ = &st;

    strLen_ = columnSize(st, position) + 1;

    const size_t bufSize = strLen_ * v_.size();
    buf_ = new char[bufSize];
    
    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
                         position++, buf_, strLen_, SQLT_CHR,
                         indOCIHolder_, &sizes_[0], 0, OCI_DEFAULT);

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void IntoType<std::vector<std::string> >::postFetch(bool gotData, bool calledFromFetch)
{
    char* pos = buf_;
    if (gotData)
    {
        for (size_t i=0; i<v_.size(); ++i)
        {
            v_[i] = std::string(pos, sizes_[i]);
            pos += strLen_;
        }
    }
    VectorIntoType::postFetch(gotData, calledFromFetch);
}


void IntoType<std::vector<std::string> >::cleanUp()
{
    delete [] buf_;
    buf_ = NULL;
    VectorIntoType::cleanUp();
}

void UseType<std::vector<std::string> >::bind(Statement &st, int &position)
{
    st_ = &st;
    sword res;
    
    size_t maxSize = 0;
    for (size_t i=0; i<v_.size(); ++i)
    {
        size_t sz = v_[i].length();
        sizes_.push_back(sz); 
        maxSize = sz > maxSize ? sz : maxSize;
    }


    buf_ = new char[maxSize * v_.size()]; 
    char* pos = buf_;
    for (size_t i=0; i<v_.size(); ++i)
    {
        strncpy(pos, v_[i].c_str(), v_[i].length());
        pos += maxSize;
    }

    if(name_.empty())
    {
        // No Name Provided, Bind By Position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, (dvoid*)buf_, (sb4)maxSize, SQLT_CHR,
            indOCIHolder_, (ub2*)&sizes_.at(0), 0, 0, 0, OCI_DEFAULT);//todo static cast?
    }
    else
    {
        // Bind By Name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), buf_, (sb4)maxSize, SQLT_CHR,//todo static cast?
            indOCIHolder_, (ub2*)&sizes_.at(0), 0, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<std::vector<std::string> >::cleanUp()
{
    delete []buf_;
    buf_ = NULL;

    VectorUseType::cleanUp();
}


// into and use types for date and time (struct tm)

void IntoType<std::tm>::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, buf_, sizeof(buf_), SQLT_DAT,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void IntoType<std::tm>::postFetch(bool gotData, bool calledFromFetch)
{
  if (gotData)
    {
        tm_.tm_isdst = -1;

        tm_.tm_year = (buf_[0] - 100) * 100 + buf_[1] - 2000;
        tm_.tm_mon = buf_[2] - 1;
        tm_.tm_mday = buf_[3];
        tm_.tm_hour = buf_[4] - 1;
        tm_.tm_min = buf_[5] - 1;
        tm_.tm_sec = buf_[6] - 1;

        // normalize and compute the remaining fields
        std::mktime(&tm_);
    }
    StandardIntoType::postFetch(gotData, calledFromFetch);

}

void UseType<std::tm>::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, buf_, sizeof(buf_), SQLT_DAT,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), buf_, sizeof(buf_), SQLT_DAT,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<std::tm>::preUse()
{
    StandardUseType::preUse();

    buf_[0] = static_cast<ub1>(100 + (1900 + tm_.tm_year) / 100);
    buf_[1] = static_cast<ub1>(100 + tm_.tm_year % 100);
    buf_[2] = static_cast<ub1>(tm_.tm_mon + 1);
    buf_[3] = static_cast<ub1>(tm_.tm_mday);
    buf_[4] = static_cast<ub1>(tm_.tm_hour + 1);
    buf_[5] = static_cast<ub1>(tm_.tm_min + 1);
    buf_[6] = static_cast<ub1>(tm_.tm_sec + 1);
}

void UseType<std::tm>::postUse()
{
    tm_.tm_isdst = -1;
    tm_.tm_year = (buf_[0] - 100) * 100 + buf_[1] - 2000;
    tm_.tm_mon = buf_[2] - 1;
    tm_.tm_mday = buf_[3];
    tm_.tm_hour = buf_[4] - 1;
    tm_.tm_min = buf_[5] - 1;
    tm_.tm_sec = buf_[6] - 1;

    // normalize and compute the remaining fields
    std::mktime(&tm_);

    StandardUseType::postUse();
}

// into and use types for std::vector<std::tm>
void IntoType<std::vector<std::tm> >::define(Statement &st, int &position)
{
    st_ = &st;

    const sb4 dlen = 7;
    buf_ = new char[vec_.size() * dlen];

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, buf_, dlen, SQLT_DAT,
        indOCIHolder_, 0, 0, OCI_DEFAULT);

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void IntoType<std::vector<std::tm> >::cleanUp()
{
    delete []buf_; 
    buf_ = NULL;
    VectorIntoType::cleanUp();
}

void IntoType<std::vector<std::tm> >::postFetch(bool gotData, bool calledFromFetch)
{
    if (gotData)
    { 
        char* pos = buf_;
        for (size_t i=0; i < vec_.size(); ++i)
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
            vec_[i] = t;
        }
    }
    VectorIntoType::postFetch(gotData, calledFromFetch);
}

void UseType<std::vector<std::tm> >::bind(Statement &st, int &position)
{
    st_ = &st;
    sword res;

    const sb4 dlen = 7;
    buf_ = new ub1[dlen * v_.size()];

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, buf_, dlen, SQLT_DAT,
            indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), buf_, dlen, SQLT_DAT,
            indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
    
}

void UseType<std::vector<std::tm> >::cleanUp()
{
    delete []buf_;
    buf_ = NULL;
    VectorUseType::cleanUp();
}

void UseType<std::vector<std::tm> >::preUse()
{
    VectorUseType::preUse();

    ub1* pos = buf_;
    for (size_t i=0; i < v_.size(); ++i)
    {
        *pos++ = static_cast<ub1>(100 + (1900 + v_[i].tm_year) / 100);
        *pos++ = static_cast<ub1>(100 + v_[i].tm_year % 100);
        *pos++ = static_cast<ub1>(v_[i].tm_mon + 1);
        *pos++ = static_cast<ub1>(v_[i].tm_mday);
        *pos++ = static_cast<ub1>(v_[i].tm_hour + 1);
        *pos++ = static_cast<ub1>(v_[i].tm_min + 1);
        *pos++ = static_cast<ub1>(v_[i].tm_sec + 1);
    }
}

void UseType<std::vector<std::tm> >::postUse()
{
    ub1* pos = buf_;
    for (size_t i=0; i<v_.size(); ++i)
    {
        v_[i].tm_isdst = -1;
        v_[i].tm_year = (*pos++ - 100) * 100;
        v_[i].tm_year += *pos++ - 2000;
        v_[i].tm_mon = *pos++ - 1;
        v_[i].tm_mday = *pos++;
        v_[i].tm_hour = *pos++ - 1;
        v_[i].tm_min = *pos++ - 1;
        v_[i].tm_sec = *pos++ - 1;

        // normalize and compute the remaining fields
        std::mktime(&v_[i]);
    }
    VectorUseType::postUse();
}


// into and use types for Statement (for nested statements and cursors)

void IntoType<Statement>::define(Statement &st, int &position)
{
    st_ = &st;

    s_.alloc();

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &s_.stmtp_, 0, SQLT_RSET,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void IntoType<Statement>::preFetch()
{
    StandardIntoType::preFetch();

    s_.unDefAndBind();
}

void IntoType<Statement>::postFetch(bool gotData, bool calledFromFetch)
{
    if (gotData)
        s_.defineAndBind();

    StandardIntoType::postFetch(gotData, calledFromFetch);
}

void UseType<Statement>::bind(Statement &st, int &position)
{
    st_ = &st;

    s_.alloc();

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &s_.stmtp_, 0, SQLT_RSET,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), &s_.stmtp_, 0, SQLT_RSET,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<Statement>::preUse()
{
    StandardUseType::preUse();

    s_.unDefAndBind();
}

void UseType<Statement>::postUse()
{
    s_.defineAndBind();

    StandardUseType::postUse();
}

// basic BLOB operations

BLOB::BLOB(Session &s)
    : session_(s)
{
    sword res = OCIDescriptorAlloc(session_.envhp_,
        reinterpret_cast<dvoid**>(&lobp_), OCI_DTYPE_LOB, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot allocate the LOB locator");
    }
}

BLOB::~BLOB()
{
    OCIDescriptorFree(lobp_, OCI_DTYPE_LOB);
}

size_t BLOB::getLen()
{
    ub4 len;
    sword res = OCILobGetLength(session_.svchp_, session_.errhp_,
        lobp_, &len);

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, session_.errhp_);
    }

    return static_cast<size_t>(len);
}

size_t BLOB::read(size_t offset, char *buf, size_t toRead)
{
    ub4 amt = static_cast<ub4>(toRead);
    sword res = OCILobRead(session_.svchp_, session_.errhp_, lobp_, &amt,
        static_cast<ub4>(offset), reinterpret_cast<dvoid*>(buf),
        amt, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, session_.errhp_);
    }

    return static_cast<size_t>(amt);
}

size_t BLOB::write(size_t offset, const char *buf, size_t toWrite)
{
    ub4 amt = static_cast<ub4>(toWrite);
    sword res = OCILobWrite(session_.svchp_, session_.errhp_, lobp_, &amt,
        static_cast<ub4>(offset),
        reinterpret_cast<dvoid*>(const_cast<char*>(buf)),
        amt, OCI_ONE_PIECE, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, session_.errhp_);
    }

    return static_cast<size_t>(amt);
}

size_t BLOB::append(const char *buf, size_t toWrite)
{
    ub4 amt = static_cast<ub4>(toWrite);
    sword res = OCILobWriteAppend(session_.svchp_, session_.errhp_, lobp_,
        &amt, reinterpret_cast<dvoid*>(const_cast<char*>(buf)),
        amt, OCI_ONE_PIECE, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, session_.errhp_);
    }

    return static_cast<size_t>(amt);
}

void BLOB::trim(size_t newLen)
{
    sword res = OCILobTrim(session_.svchp_, session_.errhp_, lobp_,
        static_cast<ub4>(newLen));
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, session_.errhp_);
    }
}

void IntoType<BLOB>::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &b_.lobp_, 0, SQLT_BLOB,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<BLOB>::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &b_.lobp_, 0, SQLT_BLOB,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), &b_.lobp_, 0, SQLT_BLOB,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

// ROWID support

RowID::RowID(Session &s)
    : session_(s)
{
    sword res = OCIDescriptorAlloc(session_.envhp_,
        reinterpret_cast<dvoid**>(&rowidp_), OCI_DTYPE_ROWID, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot allocate the ROWID descriptor");
    }
}

RowID::~RowID()
{
    OCIDescriptorFree(rowidp_, OCI_DTYPE_ROWID);
}

void IntoType<RowID>::define(Statement &st, int &position)
{
    st_ = &st;

    sword res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
        position++, &rid_.rowidp_, 0, SQLT_RDD,
        &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

void UseType<RowID>::bind(Statement &st, int &position)
{
    st_ = &st;

    sword res;

    if (name_.empty())
    {
        // no name provided, bind by position
        res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
            position++, &rid_.rowidp_, 0, SQLT_RDD,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }
    else
    {
        // bind by name
        res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
            reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
            static_cast<sb4>(name_.size()), &rid_.rowidp_, 0, SQLT_RDD,
            &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
    }

    if (res != OCI_SUCCESS)
    {
        throwSOCIError(res, st.session_.errhp_);
    }
}

// Support dynamic selecting into a Row object

void IntoType<Row>::define(Statement &st, int &)
{
    st.setRow(&row_);
    st.describe();
}
