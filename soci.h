//
// Copyright (C) 2004, 2005 Maciej Sobczak, Steve Hutton 
// 
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#ifndef SOCI_H_INCLUDED
#define SOCI_H_INCLUDED

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <ctime>
#include <oci.h>

#ifdef _MSC_VER
#pragma warning(disable:4512)
#endif

namespace SOCI
{

class Session;
class Statement;

void throwSOCIError(sword res, OCIError *errhp);

class SOCIError : public std::runtime_error
{
public:
    SOCIError(std::string const & msg, int errNum = 0);
    
    int errNum_;
};

// the enum type for indicator variables
enum eIndicator { eOK, eNoData, eNull, eTruncated };

// this is intended to be a base class for all classes that deal with
// defining output data
class IntoTypeBase
{
public:
    virtual ~IntoTypeBase() {}

    virtual void preDefine() = 0;
    virtual void define(Statement &st, int &position) = 0;
    virtual void preFetch() = 0;
    virtual void postFetch(bool gotData, bool calledFromFetch) = 0;
    virtual void cleanUp() = 0;
    virtual int size() = 0;
    virtual void resize(int sz) = 0;
};

// this is intended to be a base class for all classes that deal with
// binding input data (and OUT PL/SQL variables)
class UseTypeBase
{
public:
    virtual ~UseTypeBase() {}

    virtual void bind(Statement &st, int &position) = 0;
    virtual void preUse() = 0;
    virtual void postUse() = 0;
    virtual void cleanUp() = 0;

    virtual int size() = 0;
};

namespace details {

template <typename T>
class TypePtr
{
public:
    TypePtr(T *p) : p_(p) {}
    ~TypePtr() { delete p_; }   

    T * get() const { return p_; }
    void release() const { p_ = NULL; }

private:
    mutable T *p_;
};

typedef TypePtr<IntoTypeBase> IntoTypePtr;
typedef TypePtr<UseTypeBase> UseTypePtr;

} // namespace details


// the into function is a helper for defining output variables

// general case not implemented
template <typename T>
class IntoType;

template <typename T>
details::IntoTypePtr into(T &t)
{
    return details::IntoTypePtr(new IntoType<T>(t));
}

template <typename T, typename T1>
details::IntoTypePtr into(T &t, T1 p1)
{
    return details::IntoTypePtr(new IntoType<T>(t, p1));
}

template <typename T>
details::IntoTypePtr into(T &t, std::vector<eIndicator> &indicator)
{
    return details::IntoTypePtr(new IntoType<T>(t, indicator));
}

template <typename T>
details::IntoTypePtr into(T &t, eIndicator &indicator)
{
    return details::IntoTypePtr(new IntoType<T>(t, indicator));
}

template <typename T, typename T1, typename T2>
details::IntoTypePtr into(T &t, T1 p1, T2 p2)
{
    return details::IntoTypePtr(new IntoType<T>(t, p1, p2));
}

template <typename T, typename T1>
details::IntoTypePtr into(T &t, eIndicator &ind, T1 p1)
{
    return details::IntoTypePtr(new IntoType<T>(t, ind, p1));
}

template <typename T, typename T1, typename T2, typename T3>
details::IntoTypePtr into(T &t, T1 p1, T2 p2, T3 p3)
{
    return details::IntoTypePtr(new IntoType<T>(t, p1, p2, p3));
}

template <typename T, typename T1, typename T2>
details::IntoTypePtr into(T &t, eIndicator &ind, T1 p1, T2 p2)
{
    return details::IntoTypePtr(new IntoType<T>(t, ind, p1, p2));
}

template <typename T, typename T1, typename T2, typename T3, typename T4>
details::IntoTypePtr into(T &t, T1 p1, T2 p2, T3 p3, T4 p4)
{
    return details::IntoTypePtr(new IntoType<T>(t, p1, p2, p3, p4));
}

template <typename T, typename T1, typename T2, typename T3>
details::IntoTypePtr into(T &t, eIndicator &ind, T1 p1, T2 p2, T3 p3)
{
    return details::IntoTypePtr(new IntoType<T>(t, ind, p1, p2, p3));
}

template <typename T, typename T1, typename T2, typename T3,
          typename T4, typename T5>
details::IntoTypePtr into(T &t, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    return details::IntoTypePtr(new IntoType<T>(t, p1, p2, p3, p4, p5));
}

template <typename T, typename T1, typename T2, typename T3, typename T4>
details::IntoTypePtr into(T &t, eIndicator &ind, T1 p1, T2 p2, T3 p3, T4 p4)
{
    return details::IntoTypePtr(new IntoType<T>(t, ind, p1, p2, p3, p4));
}


// the use function is a helper for binding input variables
// (and output PL/SQL parameters)

// general case not implemented
template <typename T>
class UseType;

template <typename T>
details::UseTypePtr use(T &t)
{
    return details::UseTypePtr(new UseType<T>(t));
}

template <typename T, typename T1>
details::UseTypePtr use(T &t, T1 p1)
{
    return details::UseTypePtr(new UseType<T>(t, p1));
}

template <typename T>
details::UseTypePtr use(T &t, const std::vector<eIndicator> &indicator)
{
    return details::UseTypePtr(new UseType<T>(t, indicator));
}

template <typename T>
details::UseTypePtr use(T &t, eIndicator &indicator)
{
    return details::UseTypePtr(new UseType<T>(t, indicator));
}

template <typename T, typename T1, typename T2>
details::UseTypePtr use(T &t, T1 p1, T2 p2)
{
    return details::UseTypePtr(new UseType<T>(t, p1, p2));
}

template <typename T, typename T1>
details::UseTypePtr use(T &t, eIndicator &ind, T1 p1)
{
    return details::UseTypePtr(new UseType<T>(t, ind, p1));
}

template <typename T, typename T1>
details::UseTypePtr use(T &t, const std::vector<eIndicator> &ind, T1 p1)
{
    return details::UseTypePtr(new UseType<T>(t, ind, p1));
}

template <typename T, typename T1, typename T2, typename T3>
details::UseTypePtr use(T &t, T1 p1, T2 p2, T3 p3)
{
    return details::UseTypePtr(new UseType<T>(t, p1, p2, p3));
}

template <typename T, typename T1, typename T2>
details::UseTypePtr use(T &t, eIndicator &ind, T1 p1, T2 p2)
{
    return details::UseTypePtr(new UseType<T>(t, ind, p1, p2));
}

template <typename T, typename T1, typename T2, typename T3, typename T4>
details::UseTypePtr use(T &t, T1 p1, T2 p2, T3 p3, T4 p4)
{
    return details::UseTypePtr(new UseType<T>(t, p1, p2, p3, p4));
}

template <typename T, typename T1, typename T2, typename T3>
details::UseTypePtr use(T &t, eIndicator &ind, T1 p1, T2 p2, T3 p3)
{
    return details::UseTypePtr(new UseType<T>(t, ind, p1, p2, p3));
}

template <typename T, typename T1, typename T2, typename T3,
          typename T4, typename T5>
details::UseTypePtr use(T &t, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    return details::UseTypePtr(new UseType<T>(t, p1, p2, p3, p4, p5));
}

template <typename T, typename T1, typename T2, typename T3, typename T4>
details::UseTypePtr use(T &t, eIndicator &ind, T1 p1, T2 p2, T3 p3, T4 p4)
{
    return details::UseTypePtr(new UseType<T>(t, ind, p1, p2, p3, p4));
}

namespace details {

class PrepareTempType;

} // namespace details


// default traits class TypeConversion, acts as pass through for Row::get()
// when no actual conversion is needed.
template<typename T>
class TypeConversion
{
public:
    typedef T base_type;
    static T from(T& t) { return t; }
};

// TypeConversion specializations must use a stock type as the base_type.
// Each such specialization automatically creates a UseType and an IntoType.
template<> 
class TypeConversion<std::time_t> 
{
public:
    typedef std::tm base_type;
    static std::time_t from(std::tm& t) { return mktime(&t); }
    static std::tm to(std::time_t& t) { return *localtime(&t); }
};

// Base class Holder + derived class TypeHolder for storing type data
// instances in a container of Holder objects
template <typename T> class TypeHolder;

class Holder
{
public:
    Holder(){}
    virtual ~Holder(){};
    template<typename T> T get()
    {
        TypeHolder<T>* p = dynamic_cast<TypeHolder<T> *>(this);
        if (p)
        {
            return p->value<T>();
        }
        else
        {
            throw std::bad_cast();
        }
    }
private:
    template<typename T> T value();
};

template <typename T>
class TypeHolder : public Holder
{
public:
    TypeHolder(T* t) : t_(t) {}
    ~TypeHolder() { delete t_; }

    template<typename TVAL> TVAL value() const { return *t_; }
private:
    T* t_;
};

enum eDataType { eString, eDate, eDouble, eInteger, 
               eUnsignedLong };

class ColumnProperties
{
    // use getters/setters in case we want to make some
    // of the getters lazy in the future
public:

    std::string getName() const { return name_; }
    eDataType getDataType() const { return dataType_; }
    int getSize() const { return size_; }
    int getScale() const { return scale_; }
    int getPrecision() const { return precision_; }
    bool getNullOK() const { return nullok_; }
    
    void setName(const std::string& name) { name_ = name; }
    void setDataType(eDataType dataType) { dataType_ = dataType; }
    void setSize(int size) { size_ = size; }
    void setScale(int scale) { scale_ = scale; }
    void setPrecision(int precision) { precision_ = precision; }
    void setNullOK(bool nullok) { nullok_ = nullok; }

private:
    std::string name_;
    eDataType dataType_;
    int size_;
    int scale_;
    int precision_;
    bool nullok_;
};

class Row
{
public:

    void addProperties(const ColumnProperties& cp)
    {
        columns_.push_back(cp);
        index_[cp.getName()] = columns_.size() - 1;
    }
    size_t size() const { return holders_.size(); }
    const eIndicator indicator(int pos) const { return *indicators_.at(pos); }

    template<typename T> 
    inline void addHolder(T* t, eIndicator* ind)
    {
        holders_.push_back(new TypeHolder<T>(t));
        indicators_.push_back(ind);
    }

    const ColumnProperties& getProperties (size_t pos) const
    { return columns_.at(pos); }
    const ColumnProperties& getProperties (const std::string& name) const
    {
        std::map<std::string, size_t>::const_iterator it = index_.find(name);
        if (it == index_.end())
        {
            std::ostringstream msg;
            msg << "Column not found: " << name;
            throw SOCIError(msg.str());
        }
        return getProperties(it->second);
    }

    template<typename T> 
    T get (size_t pos) const
    {
        typedef typename TypeConversion<T>::base_type BASE_TYPE;
        BASE_TYPE baseVal = holders_.at(pos)->get<BASE_TYPE>();
        return TypeConversion<T>::from(baseVal);
    }

    template<typename T>
    T get (const std::string& name) const
    {
        std::map<std::string, size_t>::const_iterator it = index_.find(name);
        if (it == index_.end())
        {
            std::ostringstream msg;
            msg << "Column not found: "<<name;
            throw SOCIError(msg.str());
        }
        return get<T>(it->second);
    }
    
    Row() {}
    ~Row()
    {
        for(size_t i=0; i<holders_.size(); ++i)
        {
            delete holders_[i];
            delete indicators_[i];
        }
    }
private:
    // copy not supported
    Row(const Row&);
    Row operator=(const Row&);

    std::vector<ColumnProperties> columns_;
    std::vector<Holder*> holders_;
    std::vector<eIndicator*> indicators_;
    std::map<std::string, size_t> index_;
};

namespace details
{
class RefCountedPrepareInfo;

// this needs to be lightweight and copyable
class PrepareTempType
{
public:
    PrepareTempType(Session &);
    PrepareTempType(PrepareTempType const &);
    PrepareTempType & operator=(PrepareTempType const &);

    ~PrepareTempType();

    template <typename T>
    PrepareTempType & operator<<(T const &t)
    {
        rcpi_->accumulate(t);
        return *this;
    }

    PrepareTempType & operator,(IntoTypePtr const &i);
    PrepareTempType & operator,(UseTypePtr const &u);

    RefCountedPrepareInfo * getPrepareInfo() const { return rcpi_; }

private:
    RefCountedPrepareInfo *rcpi_;
};
} // namespace details

class Statement
{
public:
    Statement(Session &s);
    Statement(details::PrepareTempType const &prep);
    ~Statement();

    void alloc();
    void exchange(details::IntoTypePtr const &i);
    void exchange(details::UseTypePtr const &u);
    void cleanUp();

    void prepare(std::string const &query);
    void preDefine();
    void defineAndBind();
    void unDefAndBind();
    bool execute(int num = 0);
    bool fetch();
    void define();
    void describe();
    void setRow(Row* r){row_ = r;}
    
    OCIStmt *stmtp_;
    Session &session_;
 
protected:
    std::vector<IntoTypeBase*> intos_;
    std::vector<UseTypeBase*> uses_;

private:
    Row* row_;
    int  fetchSize_;

    template<typename T>
    void intoRow()
    {
        T* t = new T();
        eIndicator* ind = new eIndicator(eOK);
        row_->addHolder(t, ind);
        exchange(into(*t, *ind));
    }

    template<eDataType> void bindInto();

    int intosSize();
    int usesSize();
    void preFetch();
    void preUse();
    void postFetch(bool gotData, bool calledFromFetch);
    void postUse();
    bool resizeIntos();
};

class Procedure : public Statement
{
public:
    Procedure(Session &s)
        : Statement(s){}
    Procedure(details::PrepareTempType const &prep);
};

namespace details {

// this class is a base for both "once" and "prepare" statements
class RefCountedStBase
{
public:
    RefCountedStBase() : refCount_(1) {}
    virtual ~RefCountedStBase() {}

    void incRef() { ++refCount_; }
    void decRef() { if (--refCount_ == 0) delete this; }

    template <typename T>
    void accumulate(T const &t) { query_ << t; }
    
protected:
    RefCountedStBase(RefCountedStBase const &);
    RefCountedStBase & operator=(RefCountedStBase const &);

    int refCount_;
    std::ostringstream query_;
};

// this class is supposed to be a vehicle for the "once" statements
// it executes the whole statement in its destructor
class RefCountedStatement : public RefCountedStBase
{
public:
    RefCountedStatement(Session &s) : st_(s) {}
    ~RefCountedStatement();

    void exchange(IntoTypePtr const &i) { st_.exchange(i); }
    void exchange(UseTypePtr const &u) { st_.exchange(u); }

private:
    Statement st_;
};

// this class conveys only the statement text and the bind/define info
// it exists only to be passed to Statement's constructor
class RefCountedPrepareInfo : public RefCountedStBase
{
public:
    RefCountedPrepareInfo(Session &s) : session_(&s) {}
    ~RefCountedPrepareInfo();

    void exchange(IntoTypePtr const &i);
    void exchange(UseTypePtr const &u);

private:
    friend class Statement;
    friend class Procedure;

    Session *session_;

    std::vector<IntoTypeBase*> intos_;
    std::vector<UseTypeBase*> uses_;

    std::string getQuery() const { return query_.str(); }
};

// this needs to be lightweight and copyable
class OnceTempType
{
public:

    OnceTempType(Session &s);
    OnceTempType(OnceTempType const &o);
    OnceTempType & operator=(OnceTempType const &o);

    ~OnceTempType();

    template <typename T>
    OnceTempType & operator<<(T const &t)
    {
        rcst_->accumulate(t);
        return *this;
    }

    OnceTempType & operator,(IntoTypePtr const &);
    OnceTempType & operator,(UseTypePtr const &);

private:
    RefCountedStatement *rcst_;
};

// this needs to be lightweight and copyable
class OnceType
{
public:
    OnceType(Session *s) : session_(s) {}

    template <typename T>
    OnceTempType operator<<(T const &t)
    {
        OnceTempType o(*session_);
        o << t;
        return o;
    }

private:
    Session *session_;
};



// this needs to be lightweight and copyable
class PrepareType
{
public:
    PrepareType(Session *s) : session_(s) {}

    template <typename T>
    PrepareTempType operator<<(T const &t)
    {
        PrepareTempType p(*session_);
        p << t;
        return p;
    }

private:
    Session *session_;
};

} // namespace details




class Session
{
public:
    Session();

    Session(std::string const & serviceName,
        std::string const & userName,
        std::string const & password);

    ~Session();

    void commit();
    void rollback();

    void cleanUp();

    // once and prepare are for syntax sugar only
    details::OnceType once;
    details::PrepareType prepare;

    // even more sugar
    template <typename T>
    details::OnceTempType operator<<(T const &t) { return once << t; }

    OCIEnv *envhp_;
    OCIServer *srvhp_;
    OCIError *errhp_;
    OCISvcCtx *svchp_;
    OCISession *usrhp_;

private:
    Session(const Session&);
    Session& operator=(const Session&);

};


// template specializations for bind and define operations

// into and use type base classes for vectors

class VectorIntoType : public IntoTypeBase
{
public:
    VectorIntoType(std::size_t sz)
        : st_(NULL), defnp_(NULL), ind_(NULL), indOCIHolder_(NULL), sz_(sz) {}

    VectorIntoType(std::vector<eIndicator> &ind)
        : st_(NULL), defnp_(NULL), ind_(&ind.at(0)), sz_(ind.size()) {} 
       
    virtual void preDefine() 
    {
        indOCIHolderVec_.resize(sz_);
        indOCIHolder_ = &indOCIHolderVec_.front(); 
    }

    virtual void preFetch() {}
    virtual void postFetch(bool gotData, bool calledFromFetch);
    virtual void cleanUp();

protected:
    Statement *st_;
    OCIDefine *defnp_;
    eIndicator* ind_;
    sb2* indOCIHolder_;

private:
    virtual void convertFrom() {}
    std::vector<sb2> indOCIHolderVec_;
    const std::size_t sz_;
};

class VectorUseType : public UseTypeBase
{
public:
    VectorUseType(std::string const &name = std::string())
        : st_(NULL), bindp_(NULL), ind_(NULL), name_(name),
        indOCIHolder_(NULL) {}
    
    VectorUseType(const std::vector<eIndicator> &ind,
        std::string const &name = std::string())
        : st_(NULL), bindp_(NULL), ind_(&ind.at(0)),
        indOCIHolderVec_(ind.size()), name_(name) 
    {
        indOCIHolder_ = &indOCIHolderVec_.front();
    }

    virtual void preUse();
    virtual void postUse() {convertTo();}
    virtual void cleanUp();

protected:
    Statement *st_;
    OCIBind *bindp_;

private:
    const eIndicator *ind_;
    std::vector<sb2> indOCIHolderVec_;

protected:
    std::string name_;
    sb2 *indOCIHolder_;

private:
    virtual void convertTo() {} 
};


// standard types

class StandardIntoType : public IntoTypeBase
{
public:
    StandardIntoType()
        : st_(NULL), defnp_(NULL), ind_(NULL), indOCIHolder_(0) {}
    StandardIntoType(eIndicator& ind)
        : st_(NULL), defnp_(NULL), ind_(&ind), indOCIHolder_(0) {}

    virtual void postFetch(bool gotData, bool calledFromFetch);
    virtual void cleanUp();

    virtual void preFetch() {}
    virtual void preDefine() {}
    virtual int size() const { return 1; }
    virtual void resize(int sz) {}

protected:
    Statement *st_;
    OCIDefine *defnp_;
    eIndicator *ind_;
    sb2 indOCIHolder_;
    ub2 rcode_;
private:
    virtual void convertFrom() {}
};

class StandardUseType : public UseTypeBase
{
public:
    StandardUseType(std::string const &name = std::string())
        : st_(NULL), bindp_(NULL), ind_(NULL), name_(name) {}
    StandardUseType(eIndicator &ind,
        std::string const &name = std::string())
        : st_(NULL), bindp_(NULL), ind_(&ind), name_(name) {}

    virtual void preUse();
    virtual void postUse() {convertTo();}
    virtual void cleanUp();
    virtual int size() const { return 1; }

protected:
    Statement *st_;
    OCIBind *bindp_;
    eIndicator *ind_;
    sb2 indOCIHolder_;
    ub2 rcode_;
    std::string name_;

private:
    virtual void convertTo() {} 
};

// into and use types for integral types: char, short and int

template <typename T>
class IntegerIntoType : public StandardIntoType
{
public:
    IntegerIntoType(T &t) : t_(t) {}
    IntegerIntoType(T &t, eIndicator &ind) : StandardIntoType(ind), t_(t) {}

    virtual void define(Statement &st, int &position)
    {
        st_ = &st;

        sword res;

        res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
            position++, &t_, static_cast<sb4>(sizeof(T)), SQLT_INT,
                             &indOCIHolder_, 0, &rcode_, OCI_DEFAULT);
        if (res != OCI_SUCCESS)
        {
            throwSOCIError(res, st.session_.errhp_);
        }
    }

private:
    T &t_;
};

template <typename T>
class IntegerUseType : public StandardUseType
{
public:
    IntegerUseType(T &t, std::string const &name = std::string())
        : StandardUseType(name), t_(t) {}
    IntegerUseType(T &t, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), t_(t) {}

    virtual void bind(Statement &st, int &position)
    {
        st_ = &st;

        sword res;
        
        if (name_.empty())
        {
            // no name provided, bind by position
            res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
                position++, &t_, static_cast<sb4>(sizeof(T)), SQLT_INT,
                &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
        }
        else
        {
            // bind by name
            res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
                reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
                static_cast<sb4>(name_.size()),
                &t_, static_cast<sb4>(sizeof(T)), SQLT_INT,
                &indOCIHolder_, 0, &rcode_, 0, 0, OCI_DEFAULT);
        }

        if (res != OCI_SUCCESS)
        {
            throwSOCIError(res, st.session_.errhp_);
        }
    }

private:
    T &t_;
};

// into and use types for std::vector<T> where T is an integer

template <typename T>
class IntegerVectorIntoType : public VectorIntoType
{
public:
    IntegerVectorIntoType(std::vector<T> &v, std::size_t sz)  
        : VectorIntoType(sz), v_(v) {}
    IntegerVectorIntoType(std::vector<T> &v, std::vector<eIndicator> &vInd) 
        : VectorIntoType(vInd), v_(v) {}

    virtual int size() const { return v_.size(); }
    virtual void resize(int sz) { v_.resize(sz); }

    virtual void define(Statement &st, int &position)
    {
        st_ = &st;

        sword res;

        res = OCIDefineByPos(st.stmtp_, &defnp_, st.session_.errhp_,
            position++, &v_.at(0), static_cast<sb4>(sizeof(T)), SQLT_INT,
            indOCIHolder_, 0, 0, OCI_DEFAULT);
        if (res != OCI_SUCCESS)
        {
            throwSOCIError(res, st.session_.errhp_);
        }
    }

private:
    std::vector<T>& v_;
};

template <typename T>
class IntegerVectorUseType : public VectorUseType
{
public:
    IntegerVectorUseType(std::vector<T> &v,
        std::string const &name = std::string())
        : VectorUseType(name), v_(v) {}
    IntegerVectorUseType(std::vector<T> &v,
        const std::vector<eIndicator> &ind,
        std::string const &name = std::string())
        : VectorUseType(ind, name), v_(v) {}

    virtual int size() const { return v_.size(); }

    virtual void bind(Statement &st, int &position)
    {
        st_ = &st;

        sword res;

        if (name_.empty())
        {
            // no name provided, bind by position
            res = OCIBindByPos(st.stmtp_, &bindp_, st.session_.errhp_,
                position++, &v_.at(0), static_cast<sb4>(sizeof(T)), SQLT_INT,
                indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
        }
        else
        {
            // bind by name
            res = OCIBindByName(st.stmtp_, &bindp_, st.session_.errhp_,
                reinterpret_cast<text*>(const_cast<char*>(name_.c_str())),
                static_cast<sb4>(name_.size()),
                &v_.at(0), static_cast<sb4>(sizeof(T)), SQLT_INT,
                indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
        }

        if (res != OCI_SUCCESS)
        {
            throwSOCIError(res, st.session_.errhp_);
        }
    }

private:
    std::vector<T>& v_;
};


// into and use types for short

template <>
class IntoType<short> : public IntegerIntoType<short>
{
public:
    IntoType(short &s) : IntegerIntoType<short>(s) {}
    IntoType(short &s, eIndicator &ind) : IntegerIntoType<short>(s, ind) {}
};

template <>
class UseType<short> : public IntegerUseType<short>
{
public:
    UseType(short &s, std::string const &name = std::string())
        : IntegerUseType<short>(s, name) {}
    UseType(short &s, eIndicator &ind,
        std::string const &name = std::string())
        : IntegerUseType<short>(s, ind, name) {}
};

// into and use types for int

template <>
class IntoType<int> : public IntegerIntoType<int>
{
public:
    IntoType(int &i) : IntegerIntoType<int>(i) {}
    IntoType(int &i, eIndicator &ind) : IntegerIntoType<int>(i, ind) {}
};

template <>
class UseType<int> : public IntegerUseType<int>
{
public:
    UseType(int &i, std::string const &name = std::string())
        : IntegerUseType<int>(i, name) {}
    UseType(int &i, eIndicator &ind,
        std::string const &name = std::string())
        : IntegerUseType<int>(i, ind, name) {}
};

// into and use types for std::vector<int>

template <>
class IntoType<std::vector<int> > : public IntegerVectorIntoType<int>
{
public:
    IntoType(std::vector<int> &v) 
        : IntegerVectorIntoType<int>(v, v.size()) {}
    IntoType(std::vector<int> &v, std::vector<eIndicator> &ind) 
        : IntegerVectorIntoType<int>(v, ind) {}
};

template <>
class UseType<std::vector<int> > : public IntegerVectorUseType<int>
{
public:
    UseType(std::vector<int> &i, std::string const &name = std::string())
        : IntegerVectorUseType<int>(i, name) {}
    UseType(std::vector<int> &i, const std::vector<eIndicator> &ind,
        std::string const &name = std::string())
        : IntegerVectorUseType<int>(i, ind, name) {}
};

// into and use types for std::vector<short>

template <>
class IntoType<std::vector<short> > : public IntegerVectorIntoType<short>
{
public:
    IntoType(std::vector<short> &v) 
        : IntegerVectorIntoType<short>(v, v.size()) {}
    IntoType(std::vector<short> &v, std::vector<eIndicator> &ind) 
        : IntegerVectorIntoType<short>(v, ind) {}
};

template <>
class UseType<std::vector<short> > : public IntegerVectorUseType<short>
{
public:
    UseType(std::vector<short> &v, std::string const &name = std::string())
        : IntegerVectorUseType<short>(v, name) {}
    UseType(std::vector<short> &v, const std::vector<eIndicator> &ind,
        std::string const &name = std::string())
        : IntegerVectorUseType<short>(v, ind, name) {}
};


// into and use types for char

template <>
class IntoType<char> : public StandardIntoType
{
public:
    IntoType(char &c) : c_(c) {}
    IntoType(char &c, eIndicator &ind) 
      : StandardIntoType(ind), c_(c){}

    virtual void define(Statement &st, int &position);

private:
    char& c_;
};

template <>
class UseType<char> : public StandardUseType
{
public:
    UseType(char &c, std::string const &name = std::string())
        : StandardUseType(name), c_(c) {}
    UseType(char &c, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), c_(c) {}

    virtual void bind(Statement &st, int &position);

private:
    char& c_;
};

// into and use types for std::vector<char>

template <>
class IntoType<std::vector<char> >: public VectorIntoType
{
public:
    IntoType(std::vector<char> &v) 
        : VectorIntoType(v.size()), v_(v) {}
    IntoType(std::vector<char> &v, std::vector<eIndicator> &vind) 
        : VectorIntoType(vind), v_(v) {}

    void define(Statement &st, int &position);
    virtual int size() const { return v_.size(); }
    virtual void resize(int sz) { v_.resize(sz); }

private:
    std::vector<char> &v_;
};

template <>
class UseType<std::vector<char> >: public VectorUseType
{
public:
    UseType(std::vector<char> &v, std::string const &name = std::string())
        : VectorUseType(name), v_(v) {}
    UseType(std::vector<char> &v, const std::vector<eIndicator> &vind,
        std::string const &name = std::string())
        : VectorUseType(vind, name), v_(v) {}

    void bind(Statement &st, int &position);
    virtual int size() const { return v_.size(); }

private:
    std::vector<char> &v_;
};

// into and use types for unsigned long

template <>
class IntoType<unsigned long> : public StandardIntoType
{
public:
    IntoType(unsigned long &ul) : ul_(ul) {}
    IntoType(unsigned long &ul, eIndicator &ind)
        : StandardIntoType(ind), ul_(ul) {}

    virtual void define(Statement &st, int &position);

private:
    unsigned long &ul_;
};

template <>
class UseType<unsigned long> : public StandardUseType
{
public:
    UseType(unsigned long &ul, std::string const &name = std::string())
        : StandardUseType(name), ul_(ul) {}
    UseType(unsigned long &ul, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), ul_(ul) {}

    virtual void bind(Statement &st, int &position);

private:
    unsigned long &ul_;
};

template <>
class IntoType<std::vector<unsigned long> > : public VectorIntoType
{
public:
    IntoType(std::vector<unsigned long> &v) 
        : VectorIntoType(v.size()), v_(v) {}
    IntoType(std::vector<unsigned long> &v, std::vector<eIndicator> &vind) 
        : VectorIntoType(vind), v_(v) {}
    
    void define(Statement &st, int &position);
    virtual int size() const { return v_.size(); }
    virtual void resize(int sz) { v_.resize(sz); }

private:
    std::vector<unsigned long> &v_;    
};

template <>
class UseType<std::vector<unsigned long> > : public VectorUseType
{
public:
    UseType(std::vector<unsigned long> &v,
        std::string const &name = std::string())
        : VectorUseType(name), v_(v) {}
    UseType(std::vector<unsigned long> &v,
        const std::vector<eIndicator> &ind, 
        std::string const &name = std::string())
        : VectorUseType(ind, name), v_(v) {}

    void bind(Statement &st, int &position);
    virtual int size() const { return v_.size(); }

private:
    std::vector<unsigned long> &v_;
};

// into and use types for double

template <>
class IntoType<double> : public StandardIntoType
{
public:
    IntoType(double &d) : d_(d) {}
    IntoType(double &d, eIndicator &ind) : StandardIntoType(ind), d_(d) {}

    virtual void define(Statement &st, int &position);

private:
    double &d_;
};

class UseType<double> : public StandardUseType
{
public:
    UseType(double &d, std::string const &name = std::string())
        : StandardUseType(name), d_(d) {}
    UseType(double &d, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), d_(d) {}

    virtual void bind(Statement &st, int &position);

private:
    double &d_;
};

// into and use types for std::vector<double>

template <>
class IntoType<std::vector<double> > : public VectorIntoType
{
public:
    IntoType(std::vector<double> &v) 
        : VectorIntoType(v.size()), v_(v) {}
    IntoType(std::vector<double> &v, std::vector<eIndicator> &vind) 
        : VectorIntoType(vind), v_(v) {}
    
    void define(Statement &st, int &position);
    virtual int size() const { return v_.size(); }
    virtual void resize(int sz) { v_.resize(sz); }

private:
    std::vector<double> &v_;    
};

template <>
class UseType<std::vector<double> > : public VectorUseType
{
public:
    UseType(std::vector<double> &v, std::string const &name = std::string())
        : VectorUseType(name), v_(v) {}
    UseType(std::vector<double> &v, const std::vector<eIndicator> &ind,
        std::string const &name = std::string())
        : VectorUseType(ind, name), v_(v) {}

    void bind(Statement &st, int &position);
    virtual int size() const { return v_.size(); }

private:
    std::vector<double> &v_;
};

// into and use types for char*

template <>
class IntoType<char*> : public StandardIntoType
{
public:
    IntoType(char *str, size_t bufSize) : str_(str), bufSize_(bufSize) {}
    IntoType(char *str, eIndicator &ind, size_t bufSize)
        : StandardIntoType(ind), str_(str), bufSize_(bufSize) {}

    virtual void define(Statement &st, int &position);

private:
    char *str_;
    size_t bufSize_;
};

template <>
class UseType<char*> : public StandardUseType
{
public:
    UseType(char *str, size_t bufSize,
        std::string const &name = std::string())
        : StandardUseType(name), str_(str), bufSize_(bufSize) {}
    UseType(char *str, eIndicator &ind, size_t bufSize,
        std::string const &name = std::string())
        : StandardUseType(ind, name), str_(str), bufSize_(bufSize) {}

    virtual void bind(Statement &st, int &position);

private:
    char *str_;
    size_t bufSize_;
};

// into and use types for std::vector<std::string>

template <>
class IntoType<std::vector<std::string> > : public VectorIntoType
{
public:
    IntoType(std::vector<std::string>& v) 
        : VectorIntoType(v.size()), v_(v), buf_(NULL), strLen_(0),
        sizes_(v_.size()) {}
    IntoType(std::vector<std::string>& v, std::vector<eIndicator> &vind)
        : VectorIntoType(vind), v_(v), buf_(NULL),
        strLen_(0), sizes_(v_.size()) {}

    ~IntoType() { delete [] buf_; }
    virtual int size() const { return v_.size(); }
    virtual void resize(int sz) { v_.resize(sz); }

    virtual void cleanUp();
    virtual void define(Statement &st, int &position);
    virtual void postFetch(bool gotData, bool calledFromFetch);
  
private:
    std::vector<std::string>& v_;
    char* buf_;
    size_t strLen_;
    sb4 maxSize_;
    std::vector<ub2> sizes_;
};

template <>
class UseType<std::vector<std::string> > : public VectorUseType
{
public:
    UseType(std::vector<std::string>& v,
        const std::string &name = std::string()) 
        : VectorUseType(name), v_(v) {}
    UseType(std::vector<std::string>& v, const std::vector<eIndicator> &ind,
            std::string const &name = std::string())
        : VectorUseType(ind, name), v_(v) {}

    ~UseType() { delete [] buf_; }
    virtual int size() const { return v_.size(); }

    virtual void cleanUp();
    virtual void bind(Statement &st, int &position);

private:
    std::vector<ub2> sizes_;
    std::vector<std::string>& v_;
    char* buf_;
    sb4 maxSize_;
};


// into and use types for char arrays (with size known at compile-time)

template <size_t N>
class IntoType<char[N]> : public IntoType<char*>
{
public:
    IntoType(char str[]) : IntoType<char*>(str, N) {}
    IntoType(char str[], eIndicator &ind) : IntoType<char*>(str, ind, N) {}
};

template <size_t N>
class UseType<char[N]> : public UseType<char*>
{
public:
    UseType(char str[], std::string const &name = std::string())
        : UseType<char*>(str, N, name) {}
    UseType(char str[], eIndicator &ind,
        std::string const &name = std::string())
        : UseType<char*>(str, ind, N, name) {}
};

// into and use types for std::string

template <>
class IntoType<std::string> : public StandardIntoType
{
public:
  IntoType(std::string &s) : s_(s), buf_(NULL) {}
    IntoType(std::string &s, eIndicator &ind)
        : StandardIntoType(ind), s_(s), buf_(NULL) {}

    ~IntoType() { delete [] buf_; }

    virtual void define(Statement &st, int &position);
    virtual void postFetch(bool gotData, bool calledFromFetch);
    virtual void cleanUp();

private:
    std::string &s_;
    char *buf_;
};

template <>
class UseType<std::string> : public StandardUseType
{
public:
    UseType(std::string &s, std::string const &name = std::string())
        : StandardUseType(name), s_(s), buf_(NULL) {}
    UseType(std::string &s, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), s_(s), buf_(NULL) {}

    ~UseType() { delete [] buf_; }

    virtual void bind(Statement &st, int &position);
    virtual void preUse();
    virtual void postUse();
    virtual void cleanUp();

private:
    std::string &s_;
    char *buf_;
};

// into and use types for date and time (struct tm)

template <>
class IntoType<std::tm> : public StandardIntoType
{
public:
    IntoType(std::tm &t) : tm_(t) {}
    IntoType(std::tm &t, eIndicator &ind) : StandardIntoType(ind), tm_(t) {}

    virtual void define(Statement &st, int &position);
    virtual void postFetch(bool gotData, bool calledFromFetch);

private:
    std::tm &tm_;
    ub1 buf_[7];
};

template <>
class UseType<std::tm> : public StandardUseType
{
public:
    UseType(std::tm &t, std::string const &name = std::string())
        : StandardUseType(name), tm_(t) {}
    UseType(std::tm &t, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), tm_(t) {}

    virtual void bind(Statement &st, int &position);
    virtual void preUse();
    virtual void postUse();

private:
    std::tm &tm_;
    ub1 buf_[7];
};

// into and use types for std::vector<std::tm>

template <>
class IntoType<std::vector<std::tm> > : public VectorIntoType
{
public:
    IntoType(std::vector<std::tm>& v) 
        : VectorIntoType(v.size()), vec_(v), buf_(NULL) {}
    IntoType(std::vector<std::tm>& v, std::vector<eIndicator> &vind)
        : VectorIntoType(vind), vec_(v), buf_(NULL) {}

    ~IntoType() { delete []buf_; }
    void cleanUp();
    void define(Statement &st, int &position);
    void postFetch(bool gotData, bool calledFromFetch);

    virtual int size() const { return vec_.size(); }
    virtual void resize(int sz) { vec_.resize(sz); }

private:
    std::vector<std::tm>& vec_;
    ub1* buf_;
};


template <>
class UseType<std::vector<std::tm> > : public VectorUseType
{
public:
    UseType(std::vector<std::tm>& v, std::string const &name = std::string()) 
        : VectorUseType(name), v_(v), buf_(NULL) {}
    UseType(std::vector<std::tm>& v, const std::vector<eIndicator> &vind, 
        std::string const &name = std::string())
        : VectorUseType(vind, name), v_(v), buf_(NULL) {}

    ~UseType() { delete [] buf_; }
    int size() const { return v_.size(); }
    
    virtual void bind(Statement &st, int &position);
    virtual void cleanUp();
    virtual void preUse();
    virtual void postUse();

private:
    std::vector<std::tm>& v_;
    ub1* buf_;
};


// into and use types for Statement (for nested statements and cursors)

template <>
class IntoType<Statement> : public StandardIntoType
{
public:
    IntoType(Statement &s) : s_(s) {}
    IntoType(Statement &s, eIndicator &ind)
        : StandardIntoType(ind), s_(s) {}

    virtual void define(Statement &st, int &position);
    virtual void preFetch();
    virtual void postFetch(bool gotData, bool calledFromFetch);

private:
    Statement &s_;
};

template <>
class UseType<Statement> : public StandardUseType
{
public:
    UseType(Statement &s, std::string const &name = std::string())
        : StandardUseType(name), s_(s) {}
    UseType(Statement &s, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), s_(s) {}

    virtual void bind(Statement &st, int &position);
    virtual void preUse();
    virtual void postUse();

private:
    Statement &s_;
};

// Support selecting into a Row for dynamic queries

template <>
class IntoType<Row> : public StandardIntoType
{
public:
    IntoType(Row &r) : row_(r) {}
    IntoType(Row &r, eIndicator &ind)
        : StandardIntoType(ind), row_(r) {}

    virtual void define(Statement &st, int &position);

private:
    Row &row_;
};



// Automatically create an IntoType from a TypeConversion
template <typename T>
class IntoType : public IntoType<typename TypeConversion<T>::base_type>
{
public:
    typedef typename TypeConversion<T>::base_type BASE_TYPE;

    IntoType(T &value) : IntoType<BASE_TYPE>(base_value_), value_(value) {}
    IntoType(T &value, eIndicator &ind)
        : IntoType<BASE_TYPE>(base_value_, ind), value_(value) {}

private:
    void convertFrom() { value_ = TypeConversion<T>::from(base_value_); }
    
    T &value_;
    BASE_TYPE base_value_;
};

// Automatically create a UseType from a TypeConversion
template <typename T>
class UseType : public UseType<typename TypeConversion<T>::base_type>
{
public:
    typedef typename TypeConversion<T>::base_type BASE_TYPE;

    UseType(T &value) : UseType<BASE_TYPE>(base_value_), value_(value) {}
    UseType(T &value, eIndicator &ind)
        : UseType<BASE_TYPE>(base_value_, ind), value_(value) {}

private:
    void convertFrom() { value_ = TypeConversion<T>::from(base_value_); }
    void convertTo() { base_value_ = TypeConversion<T>::to(value_); }

    T &value_;
    BASE_TYPE base_value_;
};

// Automatically create a std::vector based IntoType from a TypeConversion
template <typename T>
class IntoType<std::vector<T> >
     : public IntoType<std::vector<typename TypeConversion<T>::base_type> >
{
public:
    typedef typename std::vector<typename TypeConversion<T>::base_type>
        BASE_TYPE;

    IntoType(std::vector<T> &value) 
        : IntoType<BASE_TYPE>(base_value_), value_(value),
        base_value_(value.size()) {}

    IntoType(std::vector<T> &value, std::vector<eIndicator> &ind)
        : IntoType<BASE_TYPE>(base_value_, ind), value_(value),
        base_value_(value.size()) {}

    virtual int size() const { return base_value_.size(); }
    virtual void resize(int sz) { value_.resize(sz); base_value_.resize(sz); }

private:
    void convertFrom() 
    { 
        size_t sz = base_value_.size();

        for (size_t i = 0; i != sz; ++i)
        {
            value_[i] = TypeConversion<T>::from(base_value_[i]);
        }
    }
    
    std::vector<T> &value_;
    BASE_TYPE base_value_;
};

// Automatically create a std::vector based UseType from a TypeConversion
template <typename T>
class UseType<std::vector<T> >
     : public UseType<std::vector<typename TypeConversion<T>::base_type> >
{
public:
    typedef typename std::vector<typename TypeConversion<T>::base_type>
        BASE_TYPE;

    UseType(std::vector<T> &value) 
        : UseType<BASE_TYPE>(base_value_), value_(value),
        base_value_(value_.size()) {}

    UseType(std::vector<T> &value, const std::vector<eIndicator> &ind,
        const std::string &name=std::string())
        : UseType<BASE_TYPE>(base_value_, ind, name), value_(value),
        base_value_(value_.size()) {}
 
private:
    void convertFrom() 
    {
        size_t sz = base_value_.size();
        for (size_t i = 0; i != sz; ++i)
        {
            value_[i] = TypeConversion<T>::from(base_value_[i]);
        }
    }
    void convertTo() 
    { 
        size_t sz = value_.size();
        for (size_t i = 0; i != sz; ++i)
        {
            base_value_[i] = TypeConversion<T>::to(value_[i]);
        }
    }

    std::vector<T> &value_;
    BASE_TYPE base_value_;
};

// basic BLOB operations

class BLOB
{
public:
    BLOB(Session &s);
    ~BLOB();

    size_t getLen();
    size_t read(size_t offset, char *buf, size_t toRead);
    size_t write(size_t offset, const char *buf, size_t toWrite);
    size_t append(const char *buf, size_t toWrite);
    void trim(size_t newLen);

    OCILobLocator *lobp_;

private:
    Session &session_;
};

template <>
class IntoType<BLOB> : public StandardIntoType
{
public:
    IntoType(BLOB &b) : b_(b) {}
    IntoType(BLOB &b, eIndicator &ind)
        : StandardIntoType(ind), b_(b) {}

    virtual void define(Statement &st, int &position);

private:
    BLOB &b_;
};

template <>
class UseType<BLOB> : public StandardUseType
{
public:
    UseType(BLOB &b, std::string const &name = std::string())
        : StandardUseType(name), b_(b) {}
    UseType(BLOB &b, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), b_(b) {}

    virtual void bind(Statement &st, int &position);

private:
    BLOB &b_;
};

// ROWID support

class RowID
{
public:
    RowID(Session &s);
    ~RowID();

    OCIRowid *rowidp_;

private:
    Session &session_;
};

template <>
class IntoType<RowID> : public StandardIntoType
{
public:
    IntoType(RowID &rid) : rid_(rid) {}
    IntoType(RowID &rid, eIndicator &ind)
        :StandardIntoType(ind), rid_(rid) {}

    virtual void define(Statement &st, int &position);

private:
    RowID &rid_;
};

template <>
class UseType<RowID> : public StandardUseType
{
public:
    UseType(RowID &rid, std::string const &name = std::string())
        : StandardUseType(name), rid_(rid) {}
    UseType(RowID &rid, eIndicator &ind,
        std::string const &name = std::string())
        : StandardUseType(ind, name), rid_(rid) {}

    virtual void bind(Statement &st, int &position);

private:
    RowID &rid_;
};



} // namespace SOCI

#endif // SOCI_H_INCLUDED
