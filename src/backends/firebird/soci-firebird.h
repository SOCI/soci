//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//

#ifndef SOCI_FIREBIRD_H_INCLUDED
#define SOCI_FIREBIRD_H_INCLUDED

#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_FIREBIRD_SOURCE
#   define SOCI_FIREBIRD_DECL __declspec(dllexport)
#  else
#   define SOCI_FIREBIRD_DECL __declspec(dllimport)
#  endif // SOCI_FIREBIRD_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_FIREBIRD_DECL isn't defined yet define it now
#ifndef SOCI_FIREBIRD_DECL
# define SOCI_FIREBIRD_DECL
#endif

#include <soci-backend.h>
#include <ibase.h> // FireBird
#include <vector>

namespace SOCI
{

    std::size_t const stat_size = 20;

// size of buffer for error messages. All examples use this value.
// Anyone knows, where it is stated that 512 bytes is enough ?
    std::size_t const SOCI_FIREBIRD_ERRMSG = 512;

    class SOCI_FIREBIRD_DECL FirebirdSOCIError : public SOCIError
    {
        public:
            FirebirdSOCIError(std::string const & msg,
                              ISC_STATUS const * status = 0);

            ~FirebirdSOCIError() throw()
            {};

            std::vector<ISC_STATUS> status_;
    };


    enum BuffersType
    {
        eStandard, eVector
    };

    struct FirebirdStatementBackEnd;
    struct FirebirdStandardIntoTypeBackEnd : details::StandardIntoTypeBackEnd
    {
        FirebirdStandardIntoTypeBackEnd(FirebirdStatementBackEnd &st)
                : statement_(st), buf_(NULL)
        {}

        virtual void defineByPos(int &position,
                                 void *data, details::eExchangeType type);

        virtual void preFetch();
        virtual void postFetch(bool gotData, bool calledFromFetch,
                               eIndicator *ind);

        virtual void cleanUp();

        FirebirdStatementBackEnd &statement_;
        virtual void exchangeData();

        void *data_;
        details::eExchangeType type_;
        int position_;

        char *buf_;
        short indISCHolder_;
    };

    struct FirebirdVectorIntoTypeBackEnd : details::VectorIntoTypeBackEnd
    {
        FirebirdVectorIntoTypeBackEnd(FirebirdStatementBackEnd &st)
                : statement_(st), buf_(NULL)
        {}

        virtual void defineByPos(int &position,
                                 void *data, details::eExchangeType type);

        virtual void preFetch();
        virtual void postFetch(bool gotData, eIndicator *ind);

        virtual void resize(std::size_t sz);
        virtual std::size_t size();

        virtual void cleanUp();

        FirebirdStatementBackEnd &statement_;
        virtual void exchangeData(std::size_t row);

        void *data_;
        details::eExchangeType type_;
        int position_;

        char *buf_;
        short indISCHolder_;
    };

    struct FirebirdStandardUseTypeBackEnd : details::StandardUseTypeBackEnd
    {
        FirebirdStandardUseTypeBackEnd(FirebirdStatementBackEnd &st)
                : statement_(st), buf_(NULL), indISCHolder_(0)
        {}

        virtual void bindByPos(int &position,
                               void *data, details::eExchangeType type);
        virtual void bindByName(std::string const &name,
                                void *data, details::eExchangeType type);

        virtual void preUse(eIndicator const *ind);
        virtual void postUse(bool gotData, eIndicator *ind);

        virtual void cleanUp();

        FirebirdStatementBackEnd &statement_;
        virtual void exchangeData();

        void *data_;
        details::eExchangeType type_;
        int position_;

        char *buf_;
        short indISCHolder_;
    };

    struct FirebirdVectorUseTypeBackEnd : details::VectorUseTypeBackEnd
    {
        FirebirdVectorUseTypeBackEnd(FirebirdStatementBackEnd &st)
                : statement_(st), inds_(NULL), buf_(NULL), indISCHolder_(0)
        {}

        virtual void bindByPos(int &position,
                               void *data, details::eExchangeType type);
        virtual void bindByName(std::string const &name,
                                void *data, details::eExchangeType type);

        virtual void preUse(eIndicator const *ind);

        virtual std::size_t size();

        virtual void cleanUp();

        FirebirdStatementBackEnd &statement_;
        virtual void exchangeData(std::size_t row);

        void *data_;
        details::eExchangeType type_;
        int position_;
        eIndicator const *inds_;

        char *buf_;
        short indISCHolder_;
    };

    struct FirebirdSessionBackEnd;
    struct FirebirdStatementBackEnd : details::StatementBackEnd
    {
        FirebirdStatementBackEnd(FirebirdSessionBackEnd &session);

        virtual void alloc();
        virtual void cleanUp();
        virtual void prepare(std::string const &query,
            details::eStatementType eType);

        virtual execFetchResult execute(int number);
        virtual execFetchResult fetch(int number);

        virtual int getNumberOfRows();

        virtual std::string rewriteForProcedureCall(std::string const &query);

        virtual int prepareForDescribe();
        virtual void describeColumn(int colNum, eDataType &dtype,
                                    std::string &columnName);

        virtual FirebirdStandardIntoTypeBackEnd * makeIntoTypeBackEnd();
        virtual FirebirdStandardUseTypeBackEnd * makeUseTypeBackEnd();
        virtual FirebirdVectorIntoTypeBackEnd * makeVectorIntoTypeBackEnd();
        virtual FirebirdVectorUseTypeBackEnd * makeVectorUseTypeBackEnd();

        FirebirdSessionBackEnd &session_;

        isc_stmt_handle stmtp_;
        XSQLDA * sqldap_;
        XSQLDA * sqlda2p_;

        bool boundByName_;
        bool boundByPos_;

        friend struct FirebirdVectorIntoTypeBackEnd;
        friend struct FirebirdStandardIntoTypeBackEnd;
        friend struct FirebirdVectorUseTypeBackEnd;
        friend struct FirebirdStandardUseTypeBackEnd;

protected:
        int rowsFetched_;

        virtual void exchangeData(bool gotData, int row);
        virtual void prepareSQLDA(XSQLDA ** sqldap, int size = 10);
        virtual void rewriteQuery(std::string const & query,
                                  std::vector<char> & buffer);
        virtual void rewriteParameters(std::string const & src,
                                       std::vector<char> & dst);

        BuffersType intoType_;
        BuffersType useType_;

        std::vector<std::vector<eIndicator> > inds_;
        std::vector<void*> intos_;
        std::vector<void*> uses_;

        // named parameters
        std::map <std::string, int> names_;

        bool procedure_;
    };

    struct FirebirdRowIDBackEnd : details::RowIDBackEnd
    {
        FirebirdRowIDBackEnd(FirebirdSessionBackEnd &session);

        ~FirebirdRowIDBackEnd();
    };

    struct FirebirdBLOBBackEnd : details::BLOBBackEnd
    {
        FirebirdBLOBBackEnd(FirebirdSessionBackEnd &session);

        ~FirebirdBLOBBackEnd();

        virtual std::size_t getLen();
        virtual std::size_t read(std::size_t offset, char *buf,
                                 std::size_t toRead);
        virtual std::size_t write(std::size_t offset, char const *buf,
                                  std::size_t toWrite);
        virtual std::size_t append(char const *buf, std::size_t toWrite);
        virtual void trim(std::size_t newLen);

        FirebirdSessionBackEnd &session_;

        virtual void save();
        virtual void assign(ISC_QUAD const & bid)
        {
            cleanUp();

            bid_ = bid;
            from_db_ = true;
        }

        // BLOB id from in database
        ISC_QUAD bid_;

        // BLOB id was fetched from database (true)
        // or this is new BLOB
        bool from_db_;

        // BLOB handle
        isc_blob_handle bhp_;

protected:

        virtual void open();
        virtual long getBLOBInfo();
        virtual void load();
        virtual void writeBuffer(std::size_t offset, char const * buf,
                                 std::size_t toWrite);
        virtual void cleanUp();

        // buffer for BLOB data
        std::vector<char> data_;

        bool loaded_;
        long max_seg_size_;
    };

    struct FirebirdSessionBackEnd : details::SessionBackEnd
    {
        FirebirdSessionBackEnd(std::string const &connectString);

        ~FirebirdSessionBackEnd();

        virtual void begin();
        virtual void commit();
        virtual void rollback();

        void cleanUp();

        virtual FirebirdStatementBackEnd * makeStatementBackEnd();
        virtual FirebirdRowIDBackEnd * makeRowIDBackEnd();
        virtual FirebirdBLOBBackEnd * makeBLOBBackEnd();

        virtual void setDPBOption(int const option, std::string const & value);

        isc_db_handle dbhp_;
        isc_tr_handle trhp_;
        std::string dpb_;
    };

    struct FirebirdBackEndFactory : BackEndFactory
    {
        virtual FirebirdSessionBackEnd * makeSession(
            std::string const &connectString) const;
    };

    SOCI_FIREBIRD_DECL extern FirebirdBackEndFactory const firebird;

} // namespace SOCI

#endif // SOCI_FIREBIRD_H_INCLUDED

