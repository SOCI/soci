//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_STATEMENT_H_INCLUDED
#define SOCI_STATEMENT_H_INCLUDED

#include "into.h"
#include "into-type.h"
#include "use-type.h"
#include "soci-backend.h"
#include "row.h"

#include <string>

namespace soci
{
class session;
class values;

namespace details
{
class into_type_base;
class use_type_base;
class prepare_temp_type;

class SOCI_DECL statement_impl
{
public:
    statement_impl(session &s);
    statement_impl(prepare_temp_type const &prep);
    ~statement_impl();

    void alloc();
    void bind(values& values);
    void exchange(into_type_ptr const &i);
    void exchange(use_type_ptr const &u);
    void clean_up();

    void prepare(std::string const &query,
                    eStatementType eType = eRepeatableQuery);
    void define_and_bind();
    void undefine_and_bind();
    bool execute(bool withDataExchange = false);
    bool fetch();
    void describe();
    void set_row(row* r);
    void exchange_for_rowset(into_type_ptr const &i);

    // for diagnostics and advanced users
    // (downcast it to expected back-end statement class)
    statement_backend * get_backend() { return backEnd_; }

    standard_into_type_backend * make_into_type_backend();
    standard_use_type_backend * make_use_type_backend();
    vector_into_type_backend * make_vector_into_type_backend();
    vector_use_type_backend * make_vector_use_type_backend();

    void inc_ref();
    void dec_ref();

    session &session_;

    std::string rewrite_for_procedure_call(std::string const &query);

protected:
    std::vector<details::into_type_base*> intos_;
    std::vector<details::use_type_base*> uses_;
    std::vector<eIndicator*> indicators_;

private:

    int refCount_;

    row* row_;
    std::size_t fetchSize_;
    std::size_t initialFetchSize_;
    std::string query_;
    std::map<std::string, use_type_base*> namedUses_;

    std::vector<into_type_base*> intosForRow_;
    int definePositionForRow_;

    void exchange_for_row(into_type_ptr const &i);
    void define_for_row();

    template<typename T>
    void into_row()
    {
        T* t = new T();
        eIndicator* ind = new eIndicator(eOK);
        row_->add_holder(t, ind);
        exchange_for_row(into(*t, *ind));
    }

    template<eDataType> void bind_into();

    bool alreadyDescribed_;

    std::size_t intos_size();
    std::size_t uses_size();
    void pre_fetch();
    void pre_use();
    void post_fetch(bool gotData, bool calledFromFetch);
    void post_use(bool gotData);
    bool resize_intos(std::size_t upperBound = 0);

    soci::details::statement_backend *backEnd_;
};

} // namespace details

// Statement is a handle class for statement_impl
// (this provides copyability to otherwise non-copyable type)
class SOCI_DECL statement
{
public:
    statement(session &s)
        : impl_(new details::statement_impl(s)) {}
    statement(details::prepare_temp_type const &prep)
        : impl_(new details::statement_impl(prep)) {}
    ~statement() { impl_->dec_ref(); }

    // copy is supported for this handle class
    statement(statement const &other)
        : impl_(other.impl_)
    {
        impl_->inc_ref();
    }

    void operator=(statement const &other)
    {
        other.impl_->inc_ref();
        impl_->dec_ref();
        impl_ = other.impl_;
    }

    void alloc()                                 { impl_->alloc();      }
    void bind(values& values)                    { impl_->bind(values); }
    void exchange(details::into_type_ptr const &i);
    void exchange(details::use_type_ptr const &u);
    void clean_up()                               { impl_->clean_up();    }

    void prepare(std::string const &query,
        details::eStatementType eType = details::eRepeatableQuery)
    {
        impl_->prepare(query, eType);
    }

    void define_and_bind() { impl_->define_and_bind(); }
    void undefine_and_bind()  { impl_->undefine_and_bind(); }
    bool execute(bool withDataExchange = false)
    {
        return impl_->execute(withDataExchange);
    }

    bool fetch()        { return impl_->fetch(); }
    void describe()     { impl_->describe();     }
    void set_row(row* r) { impl_->set_row(r);      }
    void exchange_for_rowset(details::into_type_ptr const &i)
    {
        impl_->exchange_for_rowset(i);
    }

    // for diagnostics and advanced users
    // (downcast it to expected back-end statement class)
    details::statement_backend * get_backend()
    {
        return impl_->get_backend();
    }

    details::standard_into_type_backend * make_into_type_backend()
    {
        return impl_->make_into_type_backend();
    }

    details::standard_use_type_backend * make_use_type_backend()
    {
        return impl_->make_use_type_backend();
    }

    details::vector_into_type_backend * make_vector_into_type_backend()
    {
        return impl_->make_vector_into_type_backend();
    }

    details::vector_use_type_backend * make_vector_use_type_backend()
    {
        return impl_->make_vector_use_type_backend();
    }

    std::string rewrite_for_procedure_call(std::string const &query)
    {
        return impl_->rewrite_for_procedure_call(query);
    }

private:
    details::statement_impl * impl_;
};

namespace details
{
// into and use types for Statement (for nested statements and cursors)

template <>
class into_type<statement> : public standard_into_type
{
public:
    into_type(statement &s) : standard_into_type(&s, eXStatement) {}
    into_type(statement &s, eIndicator &ind)
        : standard_into_type(&s, eXStatement, ind) {}
};

template <>
class use_type<statement> : public standard_use_type
{
public:
    use_type(statement &s, std::string const &name = std::string())
        : standard_use_type(&s, eXStatement, name) {}
    use_type(statement &s, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&s, eXStatement, ind, name) {}
};

} // namespace details

} // namespace SOCI

#endif
