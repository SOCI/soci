//
// Copyright (C) 2004, 2005 Maciej Sobczak, Steve Hutton
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#ifndef SOCI_ORACLE_H_INCLUDED
#define SOCI_ORACLE_H_INCLUDED

#include <oci.h>
#include "soci-common.h"
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable:4512 4511)
#endif


namespace SOCI
{

struct OracleStatementBackEnd;
struct OracleStandardIntoTypeBackEnd : details::StandardIntoTypeBackEnd
{
    OracleStandardIntoTypeBackEnd(OracleStatementBackEnd &st)
        : statement_(st), defnp_(NULL), indOCIHolder_(0),
          data_(NULL), buf_(NULL) {}

    virtual void defineByPos(int &position,
        void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, bool calledFromFetch,
        eIndicator *ind);

    virtual void cleanUp();

    OracleStatementBackEnd &statement_;

    OCIDefine *defnp_;
    sb2 indOCIHolder_;
    void *data_;
    char *buf_;        // generic buffer
    details::eExchangeType type_;

    ub2 rCode_;
};

struct OracleVectorIntoTypeBackEnd : details::VectorIntoTypeBackEnd
{
    OracleVectorIntoTypeBackEnd(OracleStatementBackEnd &st)
        : statement_(st), defnp_(NULL), indOCIHolders_(NULL),
          data_(NULL), buf_(NULL) {}

    virtual void defineByPos(int &position,
        void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, eIndicator *ind);

    virtual void resize(std::size_t sz);
    virtual std::size_t size();

    virtual void cleanUp();

    // helper function for preparing indicators and sizes_ vectors
    // (as part of the defineByPos)
    void prepareIndicators(std::size_t size);

    OracleStatementBackEnd &statement_;

    OCIDefine *defnp_;
    sb2 *indOCIHolders_;
    std::vector<sb2> indOCIHolderVec_;
    void *data_;
    char *buf_;              // generic buffer
    details::eExchangeType type_;
    std::size_t colSize_;    // size of the string column (used for strings)
    std::vector<ub2> sizes_; // sizes of data fetched (used for strings)

    std::vector<ub2> rCodes_;
};

struct OracleStandardUseTypeBackEnd : details::StandardUseTypeBackEnd
{
    OracleStandardUseTypeBackEnd(OracleStatementBackEnd &st)
        : statement_(st), bindp_(NULL), indOCIHolder_(0),
          data_(NULL), buf_(NULL) {}

    virtual void bindByPos(int &position,
        void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
        void *data, details::eExchangeType type);

    // common part for bindByPos and bindByName
    void prepareForBind(void *&data, sb4 &size, ub2 &oracleType);

    virtual void preUse(eIndicator const *ind);
    virtual void postUse(bool gotData, eIndicator *ind);

    virtual void cleanUp();

    OracleStatementBackEnd &statement_;

    OCIBind *bindp_;
    sb2 indOCIHolder_;
    void *data_;
    char *buf_;        // generic buffer
    details::eExchangeType type_;
};

struct OracleVectorUseTypeBackEnd : details::VectorUseTypeBackEnd
{
    OracleVectorUseTypeBackEnd(OracleStatementBackEnd &st)
        : statement_(st), bindp_(NULL), indOCIHolders_(NULL),
          data_(NULL), buf_(NULL) {}

    virtual void bindByPos(int &position,
        void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
        void *data, details::eExchangeType type);

    // common part for bindByPos and bindByName
    void prepareForBind(void *&data, sb4 &size, ub2 &oracleType);

    // helper function for preparing indicators and sizes_ vectors
    // (as part of the bindByPos and bindByName)
    void prepareIndicators(std::size_t size);

    virtual void preUse(eIndicator const *ind);

    virtual std::size_t size();

    virtual void cleanUp();

    OracleStatementBackEnd &statement_;

    OCIBind *bindp_;
    std::vector<sb2> indOCIHolderVec_;
    sb2 *indOCIHolders_;
    void *data_;
    char *buf_;        // generic buffer
    details::eExchangeType type_;

    // used for strings only
    std::vector<ub2> sizes_;
    std::size_t maxSize_;
};

struct OracleSessionBackEnd;
struct OracleStatementBackEnd : details::StatementBackEnd
{
    OracleStatementBackEnd(OracleSessionBackEnd &session);

    virtual void alloc();
    virtual void cleanUp();
    virtual void prepare(std::string const &query);

    virtual execFetchResult execute(int number);
    virtual execFetchResult fetch(int number);

    virtual int getNumberOfRows();

    virtual int prepareForDescribe();
    virtual void describeColumn(int colNum, eDataType &dtype,
        std::string &columnName, int &size, int &precision, int &scale,
        bool &nullOk);

    // helper for defining into vector<string>
    std::size_t columnSize(int position);

    virtual OracleStandardIntoTypeBackEnd * makeIntoTypeBackEnd();
    virtual OracleStandardUseTypeBackEnd * makeUseTypeBackEnd();
    virtual OracleVectorIntoTypeBackEnd * makeVectorIntoTypeBackEnd();
    virtual OracleVectorUseTypeBackEnd * makeVectorUseTypeBackEnd();

    OracleSessionBackEnd &session_;

    OCIStmt *stmtp_;
};

struct OracleRowIDBackEnd : details::RowIDBackEnd
{
    OracleRowIDBackEnd(OracleSessionBackEnd &session);

    ~OracleRowIDBackEnd();

    OCIRowid *rowidp_;
};

struct OracleBLOBBackEnd : details::BLOBBackEnd
{
    OracleBLOBBackEnd(OracleSessionBackEnd &session);

    ~OracleBLOBBackEnd();

    virtual std::size_t getLen();
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead);
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite);
    virtual std::size_t append(char const *buf, std::size_t toWrite);
    virtual void trim(std::size_t newLen);

    OracleSessionBackEnd &session_;

    OCILobLocator *lobp_;
};

struct OracleSessionBackEnd : details::SessionBackEnd
{
    OracleSessionBackEnd(std::string const & serviceName,
        std::string const & userName,
        std::string const & password);

    ~OracleSessionBackEnd();

    virtual void commit();
    virtual void rollback();

    void cleanUp();

    virtual OracleStatementBackEnd * makeStatementBackEnd();
    virtual OracleRowIDBackEnd * makeRowIDBackEnd();
    virtual OracleBLOBBackEnd * makeBLOBBackEnd();

    OCIEnv *envhp_;
    OCIServer *srvhp_;
    OCIError *errhp_;
    OCISvcCtx *svchp_;
    OCISession *usrhp_;
};

} // namespace SOCI

#endif // SOCI_ORACLE_H_INCLUDED
