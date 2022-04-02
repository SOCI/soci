//
// Copyright (C) 2004-2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/transaction.h"
#include "soci/error.h"

using namespace soci;

transaction::transaction(session& sql)
    : handled_(false), sql_(sql), by_session_(false)
{
    if (sql_.current_transaction() == NULL ||  //Session already in transaction?
        sql_.allow_multiple_transaction())     //Session allow multiple transactions?
    {
        if (sql_.current_transaction() != NULL) //The session already with transaction?
        {
           if (sql_.current_transaction()->by_session()) //Yes already had. Is created by session object?
           {
               //Yes is created by session object.
               bool old_handled_state = sql_.transaction_->handled_;

               sql_.transaction_->handled_ = true; //Disable the transaction prevent call Transaction::Destructor -> Session::Rollback

               delete sql_.transaction_;  //Prevent memory leak

               if ( old_handled_state == false )
               {
                   sql_.rollback();           //Revert in session transaction
               }
           }
        }

        sql_.transaction_ = this; //Pass the reference back. This transaction is created outside of session object
        sql_.begin();
    }
    else {
        handled_ = true; //Yes. Session already in transaction and not allow mutiple transactions. Auto disable this transaction object
    }
}

//Private constructor use from session object in case not assigned transaction.
//Used in src/core/session.cpp:343
transaction::transaction(session& sql, bool by_session)
    : handled_(false), sql_(sql), by_session_(by_session)
{
    //This instance is created from inside of session object.
}

transaction::~transaction()
{
    if (handled_ == false)
    {
        try
        {
            rollback();
        }
        catch (...)
        {}
    }

    if (sql_.transaction_ == this)
    {
        sql_.transaction_ = NULL; //Clear my reference in the session object
    }
}

void transaction::commit()
{
    if (handled_)
    {
        throw soci_error("The transaction object cannot be handled twice.");
    }

    sql_.commit();
    handled_ = true;
}

void transaction::rollback()
{
    if (handled_)
    {
        throw soci_error("The transaction object cannot be handled twice.");
    }

    sql_.rollback();
    handled_ = true;
}

inline session* transaction::current_session()
{
    return &sql_;
}

bool transaction::is_active()
{
    return this->handled_ == false;
}

bool transaction::by_session() //This transaction is auto created inside of the object session? src/core/session.cpp:291
{
    return this->by_session_;
}
