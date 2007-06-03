//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#define SOCI_SOURCE
#include "session.h"
#include "soci-backend.h"

#ifdef _MSC_VER
#pragma warning(disable:C4355)
#endif

using namespace soci;
using namespace soci::details;

session::session(backend_factory const &factory,
    std::string const & connectString)
    : once(this), prepare(this), logStream_(NULL)
{
    backEnd_ = factory.make_session(connectString);
}

session::~session()
{
    delete backEnd_;
}

void session::begin()
{
    backEnd_->begin();
}

void session::commit()
{
    backEnd_->commit();
}

void session::rollback()
{
    backEnd_->rollback();
}

void session::set_log_stream(std::ostream *s)
{
    logStream_ = s;
}

std::ostream * session::get_log_stream() const
{
    return logStream_;
}

void session::log_query(std::string const &query)
{
    if (logStream_ != NULL)
    {
        *logStream_ << query << '\n';
    }

    lastQuery_ = query;
}

std::string session::get_last_query() const
{
    return lastQuery_;
}

statement_backend * session::make_statement_backend()
{
    return backEnd_->make_statement_backend();
}

rowid_backend * session::make_rowid_backend()
{
    return backEnd_->make_rowid_backend();
}

blob_backend * session::make_blob_backend()
{
    return backEnd_->make_blob_backend();
}

