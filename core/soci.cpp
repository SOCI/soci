//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;

SOCIError::SOCIError(std::string const & msg)
    : std::runtime_error(msg)
{
}

Session::Session(BackEndFactory const &factory,
    std::string const & connectString)
    : once(this), prepare(this), logStream_(NULL)
{
    backEnd_ = factory.makeSession(connectString);
}

Session::~Session()
{
    delete backEnd_;
}

void Session::begin()
{
    backEnd_->begin();
}

void Session::commit()
{
    backEnd_->commit();
}

void Session::rollback()
{
    backEnd_->rollback();
}

void Session::setLogStream(std::ostream *s)
{
    logStream_ = s;
}

std::ostream * Session::getLogStream() const
{
    return logStream_;
}

void Session::logQuery(std::string const &query)
{
    if (logStream_ != NULL)
    {
        *logStream_ << query << '\n';
    }

    lastQuery_ = query;
}

std::string Session::getLastQuery() const
{
    return lastQuery_;
}

StatementBackEnd * Session::makeStatementBackEnd()
{
    return backEnd_->makeStatementBackEnd();
}

RowIDBackEnd * Session::makeRowIDBackEnd()
{
    return backEnd_->makeRowIDBackEnd();
}

BLOBBackEnd * Session::makeBLOBBackEnd()
{
    return backEnd_->makeBLOBBackEnd();
}

details::StatementImpl::StatementImpl(Session &s)
    : session_(s), refCount_(1), row_(0),
      fetchSize_(1), initialFetchSize_(1),
      alreadyDescribed_(false)
{
    backEnd_ = s.makeStatementBackEnd();
}

details::StatementImpl::StatementImpl(PrepareTempType const &prep)
    : session_(*prep.getPrepareInfo()->session_),
      refCount_(1), row_(0), fetchSize_(1), alreadyDescribed_(false)
{
    backEnd_ = session_.makeStatementBackEnd();

    RefCountedPrepareInfo *prepInfo = prep.getPrepareInfo();

    // take all bind/define info
    intos_.swap(prepInfo->intos_);
    uses_.swap(prepInfo->uses_);

    // allocate handle
    alloc();

    // prepare the statement
    query_ = prepInfo->getQuery();
    prepare(query_);

    defineAndBind();
}

details::StatementImpl::~StatementImpl()
{
    cleanUp();
}

void details::StatementImpl::alloc()
{
    backEnd_->alloc();
}

void details::StatementImpl::bind(Values& values)
{
    size_t cnt = 0;

    try
    {
        for(std::vector<details::StandardUseType*>::iterator it = 
            values.uses_.begin(); it != values.uses_.end(); ++it)
        {
            // only bind those variables which are actually
            // referenced in the statement
            const std::string name = ":" + (*it)->getName();

            size_t pos = query_.find(name);
            if (pos != std::string::npos)
            {
                const char nextChar = query_[pos + name.size()];
                if(nextChar == ' ' || nextChar == ',' ||
                   nextChar == '\0' || nextChar == ')')
                {
                    int position = static_cast<int>(uses_.size());
                    (*it)->bind(*this, position);
                    uses_.push_back(*it);
                    indicators_.push_back(values.indicators_[cnt]);
                }
                else
                {
                    values.addUnused(*it, values.indicators_[cnt]);
                }
            }
            else
            {
                values.addUnused(*it, values.indicators_[cnt]);
            }

            cnt++;
        }
    }
    catch(...)
    {
        for(size_t i = ++cnt; i != values.uses_.size(); ++i)
        {            
            values.addUnused(uses_[i], values.indicators_[i]);
        }
        throw; 
    }
}


void details::StatementImpl::exchange(IntoTypePtr const &i)
{
    intos_.push_back(i.get());
    i.release();
}

void details::StatementImpl::exchangeForRow(IntoTypePtr const &i)
{
    intosForRow_.push_back(i.get());
    i.release();
}

void details::StatementImpl::exchangeForRowset(IntoTypePtr const &i)
{
    if (!intos_.empty())
    {
        throw SOCIError("Explicit into elements not allowed with rowset.");
    }

    IntoTypeBase *p = i.get();
    intos_.push_back(p);
    i.release();

    int definePosition = 1;
    p->define(*this, definePosition);
    definePositionForRow_ = definePosition;
} 

void details::StatementImpl::exchange(UseTypePtr const &u)
{
    uses_.push_back(u.get());
    u.release();
}

void details::StatementImpl::cleanUp()
{
    // deallocate all bind and define objects
    std::size_t const isize = intos_.size();
    for (std::size_t i = isize; i != 0; --i)
    {
        intos_[i - 1]->cleanUp();
        delete intos_[i - 1];
        intos_.resize(i - 1);
    }

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = ifrsize; i != 0; --i)
    {
        intosForRow_[i - 1]->cleanUp();
        delete intosForRow_[i - 1];
        intosForRow_.resize(i - 1);
    }

    std::size_t const usize = uses_.size();
    for (std::size_t i = usize; i != 0; --i)
    {
        uses_[i - 1]->cleanUp();
        delete uses_[i - 1];
        uses_.resize(i - 1);
    }

    std::size_t const indsize = indicators_.size();
    for (std::size_t i = 0; i != indsize; ++i)
    {
        delete indicators_[i];
        indicators_[i] = NULL;
    }

    if (backEnd_ != NULL)
    {
        backEnd_->cleanUp();
        delete backEnd_;
        backEnd_ = NULL;
    }
}

void details::StatementImpl::prepare(std::string const &query,
    details::eStatementType eType)
{
    query_ = query;
    session_.logQuery(query);

    backEnd_->prepare(query, eType);
}

void details::StatementImpl::defineAndBind()
{
    int definePosition = 1;
    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intos_[i]->define(*this, definePosition);
    }

    // if there are some implicite into elements
    // injected by the row description process,
    // they should be defined in the later phase,
    // starting at the position where the above loop finished
    definePositionForRow_ = definePosition;

    int bindPosition = 1;
    std::size_t const usize = uses_.size();
    for (std::size_t i = 0; i != usize; ++i)
    {
        uses_[i]->bind(*this, bindPosition);
    }
}

void details::StatementImpl::defineForRow()
{
    std::size_t const isize = intosForRow_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intosForRow_[i]->define(*this, definePositionForRow_);
    }
}

void details::StatementImpl::unDefAndBind()
{
    std::size_t const isize = intos_.size();
    for (std::size_t i = isize; i != 0; --i)
    {
        intos_[i - 1]->cleanUp();
    }

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = ifrsize; i != 0; --i)
    {
        intosForRow_[i - 1]->cleanUp();
    }

    std::size_t const usize = uses_.size();
    for (std::size_t i = usize; i != 0; --i)
    {
        uses_[i - 1]->cleanUp();
    }
}

bool details::StatementImpl::execute(bool withDataExchange)
{
    initialFetchSize_ = intosSize();
    fetchSize_ = initialFetchSize_;

    std::size_t bindSize = usesSize();

    if (bindSize > 1 && fetchSize_ > 1)
    {
        throw SOCIError(
             "Bulk insert/update and bulk select not allowed in same query");
    }

    preUse();

    // looks like a hack and it is - row description should happen
    // *after* the use elements were completely prepared
    // and *before* the into elements are touched, so that the row
    // description process can inject more into elements for
    // implicit data exchange
    if (row_ != NULL && alreadyDescribed_ == false)
    {
        describe();
        defineForRow();
    }

    int num = 0;
    if (withDataExchange)
    {
        num = 1;

        preFetch();

        if (static_cast<int>(fetchSize_) > num)
        {
            num = static_cast<int>(fetchSize_);
        }
        if (static_cast<int>(bindSize) > num)
        {
            num = static_cast<int>(bindSize);
        }
    }

    StatementBackEnd::execFetchResult res = backEnd_->execute(num);

    bool gotData = false;

    if (res == StatementBackEnd::eSuccess)
    {
        // the "success" means that the statement executed correctly
        // and for select statement this also means that some rows were read

        if (num > 0)
        {
            gotData = true;

            // ensure into vectors have correct size
            resizeIntos(static_cast<std::size_t>(num));
        }
    }
    else // res == eNoData
    {
        // the "no data" means that the end-of-rowset condition was hit
        // but still some rows might have been read (the last bunch of rows)
        // it can also mean that the statement did not produce any results

        gotData = fetchSize_ > 1 ? resizeIntos() : false;
    }

    if (num > 0)
    {
        postFetch(gotData, false);
        postUse(gotData);
    }

    return gotData;
}

bool details::StatementImpl::fetch()
{
    if (fetchSize_ == 0)
    {
        return false;
    }

    bool gotData = false;

    // vectors might have been resized between fetches
    std::size_t newFetchSize = intosSize();
    if (newFetchSize > initialFetchSize_)
    {
        // this is not allowed, because most likely caused reallocation
        // of the vector - this would require complete re-bind

        throw SOCIError(
            "Increasing the size of the output vector is not supported.");
    }
    else if (newFetchSize == 0)
    {
        return false;
    }
    else
    {
        // the output vector was downsized or remains the same as before
        fetchSize_ = newFetchSize;
    }

    StatementBackEnd::execFetchResult res =
        backEnd_->fetch(static_cast<int>(fetchSize_));
    if (res == StatementBackEnd::eSuccess)
    {
        // the "success" means that some number of rows was read
        // and that it is not yet the end-of-rowset (there are more rows)

        gotData = true;

        // ensure into vectors have correct size
        resizeIntos(fetchSize_);
    }
    else // res == eNoData
    {
        // end-of-rowset condition

        if (fetchSize_ > 1)
        {
            // but still the last bunch of rows might have been read
            gotData = resizeIntos();
            fetchSize_ = 0;
        }
        else
        {
            gotData = false;
        }
    }

    postFetch(gotData, true);
    return gotData;
}

std::size_t details::StatementImpl::intosSize()
{
    // this function does not need to take into account intosForRow_ elements,
    // since their sizes are always 1 (which is the same and the primary
    // into(row) element, which has injected them)

    std::size_t intosSize = 0;
    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        if (i==0)
        {
            intosSize = intos_[i]->size();
            if (intosSize == 0)
            {
                 // this can happen only for vectors
                 throw SOCIError("Vectors of size 0 are not allowed.");
            }
        }
        else if (intosSize != intos_[i]->size())
        {
            std::ostringstream msg;
            msg << "Bind variable size mismatch (into["
                << static_cast<unsigned long>(i) << "] has size "
                << static_cast<unsigned long>(intos_[i]->size())
                << ", into[0] has size "
                << static_cast<unsigned long>(intosSize);
            throw SOCIError(msg.str());
        }
    }
    return intosSize;
}

std::size_t details::StatementImpl::usesSize()
{
    std::size_t usesSize = 0;
    std::size_t const usize = uses_.size();
    for (std::size_t i = 0; i != usize; ++i)
    {
        if (i==0)
        {
            usesSize = uses_[i]->size();
            if (usesSize == 0)
            {
                 // this can happen only for vectors
                 throw SOCIError("Vectors of size 0 are not allowed.");
            }
        }
        else if (usesSize != uses_[i]->size())
        {
            std::ostringstream msg;
            msg << "Bind variable size mismatch (use["
                << static_cast<unsigned long>(i) << "] has size "
                << static_cast<unsigned long>(uses_[i]->size())
                << ", use[0] has size "
                << static_cast<unsigned long>(usesSize);
            throw SOCIError(msg.str());
        }
    }
    return usesSize;
}

bool details::StatementImpl::resizeIntos(std::size_t upperBound)
{
    // this function does not need to take into account the intosForRow_
    // elements, since they are never used for bulk operations

    std::size_t rows = backEnd_->getNumberOfRows();
    if (upperBound != 0 && upperBound < rows)
    {
        rows = upperBound;
    }

    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intos_[i]->resize(rows);
    }

    return rows > 0 ? true : false;
}

void details::StatementImpl::preFetch()
{
    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intos_[i]->preFetch();
    }

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = 0; i != ifrsize; ++i)
    {
        intosForRow_[i]->preFetch();
    }
}

void details::StatementImpl::preUse()
{
    std::size_t const usize = uses_.size();
    for (std::size_t i = 0; i != usize; ++i)
    {
        uses_[i]->preUse();
    }
}

void details::StatementImpl::postFetch(bool gotData, bool calledFromFetch)
{
    // first iterate over intosForRow_ elements, since the Row element
    // (which is among the intos_ elements) might depend on the
    // values of those implicitly injected elements

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = 0; i != ifrsize; ++i)
    {
        intosForRow_[i]->postFetch(gotData, calledFromFetch);
    }

    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intos_[i]->postFetch(gotData, calledFromFetch);
    }
}

void details::StatementImpl::postUse(bool gotData)
{ 
    // iterate in reverse order here in case the first item
    // is an UseType<Values> (since it depends on the other UseTypes)
    for (std::size_t i = uses_.size(); i != 0; --i)
    {
        uses_[i-1]->postUse(gotData);
    }
}

details::StandardIntoTypeBackEnd *
details::StatementImpl::makeIntoTypeBackEnd()
{
    return backEnd_->makeIntoTypeBackEnd();
}

details::StandardUseTypeBackEnd *
details::StatementImpl::makeUseTypeBackEnd()
{
    return backEnd_->makeUseTypeBackEnd();
}

details::VectorIntoTypeBackEnd *
details::StatementImpl::makeVectorIntoTypeBackEnd()
{
    return backEnd_->makeVectorIntoTypeBackEnd();
}

details::VectorUseTypeBackEnd *
details::StatementImpl::makeVectorUseTypeBackEnd()
{
    return backEnd_->makeVectorUseTypeBackEnd();
}

void details::StatementImpl::incRef()
{
    ++refCount_;
}

void details::StatementImpl::decRef()
{
    if (--refCount_ == 0)
    {
        delete this;
    }
}

// Map eDataTypes to stock types for dynamic result set support
namespace SOCI
{
namespace details
{

template<>
void StatementImpl::bindInto<eString>()
{
    intoRow<std::string>();
}

template<>
void StatementImpl::bindInto<eDouble>()
{
    intoRow<double>();
}

template<>
void StatementImpl::bindInto<eInteger>()
{
    intoRow<int>();
}

template<>
void StatementImpl::bindInto<eUnsignedLong>()
{
    intoRow<unsigned long>();
}

template<>
void StatementImpl::bindInto<eDate>()
{
    intoRow<std::tm>();
}

} // namespace details

} //namespace SOCI

void details::StatementImpl::describe()
{
    int numcols = backEnd_->prepareForDescribe();

    for (int i = 1; i <= numcols; ++i)
    {
        eDataType dtype;
        std::string columnName;

        backEnd_->describeColumn(i, dtype, columnName);

        ColumnProperties props;
        props.setName(columnName);

        props.setDataType(dtype);
        switch(dtype)
        {
        case eString:
            bindInto<eString>();
            break;
        case eDouble:
            bindInto<eDouble>();
            break;
        case eInteger:
            bindInto<eInteger>();
            break;
        case eUnsignedLong:
            bindInto<eUnsignedLong>();
            break;
        case eDate:
            bindInto<eDate>();
            break;
        default:
            std::ostringstream msg;
            msg << "db column type " << dtype
                <<" not supported for dynamic selects"<<std::endl;
            throw SOCIError(msg.str());
        }
        row_->addProperties(props);
    }

    alreadyDescribed_ = true;
}

void details::StatementImpl::setRow(Row *r)
{
    if (row_ != NULL)
    {
        throw SOCIError(
            "Only one Row element allowed in a single statement.");
    }

    row_ = r;
}

std::string details::StatementImpl::rewriteForProcedureCall(std::string const &query)
{
    return backEnd_->rewriteForProcedureCall(query);
}

details::ProcedureImpl::ProcedureImpl(PrepareTempType const &prep)
    : StatementImpl(*prep.getPrepareInfo()->session_)
{
    RefCountedPrepareInfo *prepInfo = prep.getPrepareInfo();

    // take all bind/define info
    intos_.swap(prepInfo->intos_);
    uses_.swap(prepInfo->uses_);

    // allocate handle
    alloc();

    // prepare the statement
    prepare(rewriteForProcedureCall(prepInfo->getQuery()));

    defineAndBind();
}

void RefCountedStatement::finalAction()
{
    try
    {
        st_.alloc();
        st_.prepare(query_.str(), details::eOneTimeQuery);
        st_.defineAndBind();
        st_.execute(true);
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

void RefCountedPrepareInfo::finalAction()
{
    // deallocate all bind and define objects
    for (std::size_t i = intos_.size(); i > 0; --i)
    {
        delete intos_[i - 1];
        intos_.resize(i - 1);
    }

    for (std::size_t i = uses_.size(); i > 0; --i)
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

void Row::addProperties(ColumnProperties const &cp)
{
    columns_.push_back(cp);
    index_[cp.getName()] = columns_.size() - 1;
}

std::size_t Row::size() const
{
    return holders_.size();
}

eIndicator Row::indicator(std::size_t pos) const
{
    assert(indicators_.size() >= static_cast<std::size_t>(pos + 1));
    return *indicators_[pos];
}

eIndicator Row::indicator(std::string const &name) const
{
    return indicator(findColumn(name));
}

ColumnProperties const & Row::getProperties(std::size_t pos) const
{
    assert(columns_.size() >= pos + 1);
    return columns_[pos];
}

ColumnProperties const & Row::getProperties(std::string const &name) const
{
    return getProperties(findColumn(name));
}

std::size_t Row::findColumn(std::string const &name) const
{
    std::map<std::string, std::size_t>::const_iterator it = index_.find(name);
    if (it == index_.end())
    {
        std::ostringstream msg;
        msg << "Column '" << name << "' not found";
        throw SOCIError(msg.str());
    }

    return it->second;
}

Row::~Row()
{
    std::size_t const hsize = holders_.size();
    for(std::size_t i = 0; i != hsize; ++i)
    {
        delete holders_[i];
        delete indicators_[i];
    }
}

eIndicator Values::indicator(std::size_t pos) const
{
    if (row_)
    {
        return row_->indicator(pos);
    }
    else
    {
        return *indicators_[pos];
    }
}

eIndicator Values::indicator(std::string const &name) const
{
    if (row_)
    {
        return row_->indicator(name);
    }
    else
    {
        std::map<std::string, std::size_t>::const_iterator it = index_.find(name);
        if (it == index_.end())
        {
            std::ostringstream msg;
            msg << "Column '" << name << "' not found";
            throw SOCIError(msg.str());
        }
        return *indicators_[it->second];
    }
}


// implementation of into and use types

// standard types

StandardIntoType::~StandardIntoType()
{
    delete backEnd_;
}

void StandardIntoType::define(StatementImpl &st, int &position)
{
    backEnd_ = st.makeIntoTypeBackEnd();
    backEnd_->defineByPos(position, data_, type_);
}

void StandardIntoType::preFetch()
{
    backEnd_->preFetch();
}

void StandardIntoType::postFetch(bool gotData, bool calledFromFetch)
{
    backEnd_->postFetch(gotData, calledFromFetch, ind_);

    if (gotData)
    {
        convertFrom();
    }
}

void StandardIntoType::cleanUp()
{
    // backEnd_ might be NULL if IntoType<Row> was used
    if (backEnd_ != NULL)
    {
        backEnd_->cleanUp();
    }
}

StandardUseType::~StandardUseType()
{
    delete backEnd_;
}

void StandardUseType::bind(StatementImpl &st, int &position)
{
    backEnd_ = st.makeUseTypeBackEnd();
    if (name_.empty())
    {
        backEnd_->bindByPos(position, data_, type_);
    }
    else
    {
        backEnd_->bindByName(name_, data_, type_);
    }
}

void StandardUseType::preUse()
{
    convertTo();
    backEnd_->preUse(ind_);
}

void StandardUseType::postUse(bool gotData)
{
    backEnd_->postUse(gotData, ind_);

    convertFrom();
}

void StandardUseType::cleanUp()
{
    if (backEnd_ != NULL)
    {
        backEnd_->cleanUp();
    }
}

// vector based types

VectorIntoType::~VectorIntoType()
{
    delete backEnd_;
}

void VectorIntoType::define(StatementImpl &st, int &position)
{
    backEnd_ = st.makeVectorIntoTypeBackEnd();
    backEnd_->defineByPos(position, data_, type_);
}

void VectorIntoType::preFetch()
{
    backEnd_->preFetch();
}

void VectorIntoType::postFetch(bool gotData, bool /* calledFromFetch */)
{
    if (indVec_ != NULL && indVec_->empty() == false)
    {
        assert(indVec_->empty() == false);
        backEnd_->postFetch(gotData, &(*indVec_)[0]);
    }
    else
    {
        backEnd_->postFetch(gotData, NULL);
    }

    if(gotData)
    {
        convertFrom();
    }
}

void VectorIntoType::resize(std::size_t sz)
{
    if (indVec_ != NULL)
    {
        indVec_->resize(sz);
    }

    backEnd_->resize(sz);
}

std::size_t VectorIntoType::size() const
{
    return backEnd_->size();
}

void VectorIntoType::cleanUp()
{
    if (backEnd_ != NULL)
    {
        backEnd_->cleanUp();
    }
}

VectorUseType::~VectorUseType()
{
    delete backEnd_;
}

void VectorUseType::bind(StatementImpl &st, int &position)
{
    backEnd_ = st.makeVectorUseTypeBackEnd();
    if (name_.empty())
    {
        backEnd_->bindByPos(position, data_, type_);
    }
    else
    {
        backEnd_->bindByName(name_, data_, type_);
    }
}

void VectorUseType::preUse()
{
    convertTo();

    backEnd_->preUse(ind_);
}

std::size_t VectorUseType::size() const
{
    return backEnd_->size();
}

void VectorUseType::cleanUp()
{
    if (backEnd_ != NULL)
    {
        backEnd_->cleanUp();
    }
}


// basic BLOB operations

BLOB::BLOB(Session &s)
{
    backEnd_ = s.makeBLOBBackEnd();
}

BLOB::~BLOB()
{
    delete backEnd_;
}

std::size_t BLOB::getLen()
{
    return backEnd_->getLen();
}

std::size_t BLOB::read(std::size_t offset, char *buf, std::size_t toRead)
{
    return backEnd_->read(offset, buf, toRead);
}

std::size_t BLOB::write(
    std::size_t offset, char const *buf, std::size_t toWrite)
{
    return backEnd_->write(offset, buf, toWrite);
}

std::size_t BLOB::append(char const *buf, std::size_t toWrite)
{
    return backEnd_->append(buf, toWrite);
}

void BLOB::trim(std::size_t newLen)
{
    backEnd_->trim(newLen);
}

// ROWID support

RowID::RowID(Session &s)
{
    backEnd_ = s.makeRowIDBackEnd();
}

RowID::~RowID()
{
    delete backEnd_;
}
