//
// Copyright (C) 2004, 2005 Maciej Sobczak, Steve Hutton
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#ifndef SOCI_COMMON_H_INCLUDED
#define SOCI_COMMON_H_INCLUDED

#include <string>
#include <map>
#include <stdexcept>
#include <cstddef>

namespace SOCI
{

// data types, as seen by the user
enum eDataType { eString, eChar, eDate, eDouble, eInteger,
                 eUnsignedLong };

// the enum type for indicator variables
enum eIndicator { eOK, eNoData, eNull, eTruncated };


class SOCIError : public std::runtime_error
{
public:
    SOCIError(std::string const & msg, int errNum = 0);

    int errNum_;
};


namespace details
{

// data types, as used to describe exchange format
enum eExchangeType { eXChar, eXCString, eXStdString, eXShort, eXInteger,
                     eXUnsignedLong, eXDouble, eXStdTm, eXStatement,
                     eXRowID, eXBLOB };

// polymorphic into type backend

class StandardIntoTypeBackEnd
{
public:
    virtual ~StandardIntoTypeBackEnd() {}

    virtual void defineByPos(int &position,
        void *data, eExchangeType type) = 0;

    virtual void preFetch() = 0;
    virtual void postFetch(bool gotData, bool calledFromFetch,
        eIndicator *ind) = 0;

    virtual void cleanUp() = 0;
};

class VectorIntoTypeBackEnd
{
public:
    virtual ~VectorIntoTypeBackEnd() {}

    virtual void defineByPos(int &position,
        void *data, eExchangeType type) = 0;

    virtual void preFetch() = 0;
    virtual void postFetch(bool gotData, eIndicator *ind) = 0;

    virtual void resize(std::size_t sz) = 0;
    virtual std::size_t size() = 0;

    virtual void cleanUp() = 0;
};

// polymorphic use type backend

class StandardUseTypeBackEnd
{
public:
    virtual ~StandardUseTypeBackEnd() {}

    virtual void bindByPos(int &position,
        void *data, eExchangeType type) = 0;
    virtual void bindByName(std::string const &name,
        void *data, eExchangeType type) = 0;

    virtual void preUse(eIndicator const *ind) = 0;
    virtual void postUse(bool gotData, eIndicator *ind) = 0;

    virtual void cleanUp() = 0;
};

class VectorUseTypeBackEnd
{
public:
    virtual ~VectorUseTypeBackEnd() {}

    virtual void bindByPos(int &position,
        void *data, eExchangeType type) = 0;
    virtual void bindByName(std::string const &name,
        void *data, eExchangeType type) = 0;

    virtual void preUse(eIndicator const *ind) = 0;

    virtual std::size_t size() = 0;

    virtual void cleanUp() = 0;
};

// polymorphic statement backend

class StatementBackEnd
{
public:
    virtual ~StatementBackEnd() {}

    virtual void alloc() = 0;
    virtual void cleanUp() = 0;
    virtual void prepare(std::string const &query) = 0;

    enum execFetchResult { eSuccess, eNoData };
    virtual execFetchResult execute(int number) = 0;
    virtual execFetchResult fetch(int number) = 0;

    virtual int getNumberOfRows() = 0;

    virtual std::string rewriteForProcedureCall(std::string const &query) = 0;

    virtual int prepareForDescribe() = 0;
    virtual void describeColumn(int colNum, eDataType &dtype,
        std::string &columnName, int &size, int &precision, int &scale,
        bool &nullOk) = 0;

    virtual StandardIntoTypeBackEnd * makeIntoTypeBackEnd() = 0;
    virtual StandardUseTypeBackEnd * makeUseTypeBackEnd() = 0;
    virtual VectorIntoTypeBackEnd * makeVectorIntoTypeBackEnd() = 0;
    virtual VectorUseTypeBackEnd * makeVectorUseTypeBackEnd() = 0;
};

// polymorphic RowID backend

class RowIDBackEnd
{
public:
    virtual ~RowIDBackEnd() {}
};

// polymorphic RowID backend

class BLOBBackEnd
{
public:
    virtual ~BLOBBackEnd() {}

    virtual std::size_t getLen() = 0;
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead) = 0;
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite) = 0;
    virtual std::size_t append(char const *buf, std::size_t toWrite) = 0;
    virtual void trim(std::size_t newLen) = 0;
};

// polymorphic session backend

class SessionBackEnd
{
public:
    virtual ~SessionBackEnd() {}

    virtual void begin() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;

    virtual StatementBackEnd * makeStatementBackEnd() = 0;
    virtual RowIDBackEnd * makeRowIDBackEnd() = 0;
    virtual BLOBBackEnd * makeBLOBBackEnd() = 0;
};


// helper class used to keep pointer and buffer size as a single object
struct CStringDescriptor
{
    CStringDescriptor(char *str, std::size_t bufSize)
        : str_(str), bufSize_(bufSize) {}

    char *str_;
    std::size_t bufSize_;
};

// simple registry for back-end factories

struct BackEndFactory
{
    virtual SessionBackEnd * makeSession(
        std::string const &connectString) const = 0;
};

class BackEndFactoryRegistry
{
public:
    void registerMe(std::string const &beName, BackEndFactory const *f);
    BackEndFactory const * find(std::string const &beName) const;

private:
    std::map<std::string, BackEndFactory const *> registry_;
};

// registry singleton
BackEndFactoryRegistry & theBEFRegistry();


} // namespace details

} // namespace SOCI

#endif // SOCI_COMMON_H_INCLUDED
