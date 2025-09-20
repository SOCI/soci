//
// Copyright (C) 2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/connection-pool.h"
#include "soci/error.h"
#include "soci/session.h"
#include <vector>
#include <utility>

#include "soci-mutex.h"

using namespace soci;

// Common base class for both POSIX and Windows implementations.
struct soci_connection_pool_base_impl
{
    explicit soci_connection_pool_base_impl(std::size_t size)
    {
        if (size == 0)
        {
            throw soci_error("Invalid pool size");
        }

        sessions_.resize(size);
        for (std::size_t i = 0; i != size; ++i)
        {
            sessions_[i] = std::make_pair(true, std::make_unique<session>());
        }
    }

    ~soci_connection_pool_base_impl() = default;

    bool find_free(std::size_t & pos)
    {
        for (std::size_t i = 0; i != sessions_.size(); ++i)
        {
            if (sessions_[i].first)
            {
                pos = i;
                return true;
            }
        }

        return false;
    }


    // by convention, first == true means the entry is free (not used)
    std::vector<std::pair<bool, std::unique_ptr<session>> > sessions_;
};

#ifndef _WIN32
// POSIX implementation

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

struct connection_pool::connection_pool_impl : soci_connection_pool_base_impl
{
    explicit connection_pool_impl(std::size_t size)
        : soci_connection_pool_base_impl(size)
    {
        if (pthread_cond_init(&cond_, NULL) != 0)
        {
            throw soci_error("Synchronization error");
        }
    }

    connection_pool_impl(connection_pool_impl const&) = delete;
    connection_pool_impl& operator=(connection_pool_impl const&) = delete;

    ~connection_pool_impl()
    {
        pthread_cond_destroy(&cond_);
    }

    soci_mutex_t mtx_;
    pthread_cond_t cond_;
};

bool connection_pool::try_lease(std::size_t & pos, int timeout)
{
    struct timespec tm;
    if (timeout >= 0)
    {
        // timeout is relative in milliseconds

        struct timeval tmv;
        gettimeofday(&tmv, NULL);

        tm.tv_sec = tmv.tv_sec + timeout / 1000;
        tm.tv_nsec = tmv.tv_usec * 1000 + (timeout % 1000) * 1000 * 1000;

        if (tm.tv_nsec >= 1000 * 1000 * 1000)
        {
            ++tm.tv_sec;
            tm.tv_nsec -= 1000 * 1000 * 1000;
        }
    }

    soci_scoped_lock lock(&pimpl_->mtx_);

    int cc = 0;
    while (pimpl_->find_free(pos) == false)
    {
        if (timeout < 0)
        {
            // no timeout, allow unlimited blocking
            cc = pthread_cond_wait(
                &(pimpl_->cond_), pimpl_->mtx_.native_handle());
        }
        else
        {
            // wait with timeout
            cc = pthread_cond_timedwait(
                &(pimpl_->cond_), pimpl_->mtx_.native_handle(), &tm);
        }

        if (cc == ETIMEDOUT)
        {
            break;
        }

        // pthread_cond_timedwait() can apparently return these errors too,
        // even if POSIX doesn't document them for the scenario in which we
        // call it.
        if (cc == EINVAL || cc == EPERM)
        {
            // We should perhaps throw an exception here, but at the very least
            // exit the loop to avoid being stuck in it forever.
            break;
        }
    }

    if (cc == 0)
    {
        pimpl_->sessions_[pos].first = false;
    }

    if (cc != 0)
    {
        // we can only fail if timeout expired
        if (timeout < 0)
        {
            throw soci_error("Getting connection from the pool unexpectedly failed");
        }

        return false;
    }

    return true;
}

void connection_pool::give_back(std::size_t pos)
{
    if (pos >= pimpl_->sessions_.size())
    {
        throw soci_error("Invalid pool position");
    }

    soci_scoped_lock lock(&pimpl_->mtx_);

    if (pimpl_->sessions_[pos].first)
    {
        throw soci_error("Cannot release pool entry (already free)");
    }

    pimpl_->sessions_[pos].first = true;

    pthread_cond_signal(&(pimpl_->cond_));
}

#else
// Windows implementation

#include <windows.h>

struct connection_pool::connection_pool_impl : soci_connection_pool_base_impl
{
    explicit connection_pool_impl(std::size_t size)
        : soci_connection_pool_base_impl(size)
    {
        // initially all entries are available
        sem_ = CreateSemaphore(NULL,
            static_cast<LONG>(size), static_cast<LONG>(size), NULL);
        if (sem_ == NULL)
        {
            throw soci_error("Synchronization error");
        }
    }

    ~connection_pool_impl()
    {
        CloseHandle(sem_);
    }

    soci_mutex_t mtx_;
    HANDLE sem_;
};

bool connection_pool::try_lease(std::size_t & pos, int timeout)
{
    DWORD cc = WaitForSingleObject(pimpl_->sem_,
        timeout >= 0 ? static_cast<DWORD>(timeout) : INFINITE);
    if (cc == WAIT_OBJECT_0)
    {
        // semaphore acquired, there is (at least) one free entry

        soci_scoped_lock lock(&pimpl_->mtx_);

        if (!pimpl_->find_free(pos))
        {
            // this should be impossible
            throw soci_error("Getting connection from the pool unexpectedly failed");
        }

        pimpl_->sessions_[pos].first = false;

        return true;
    }
    else if (cc == WAIT_TIMEOUT)
    {
        return false;
    }
    else
    {
        throw soci_error("Synchronization error");
    }
}

void connection_pool::give_back(std::size_t pos)
{
    if (pos >= pimpl_->sessions_.size())
    {
        throw soci_error("Invalid pool position");
    }

    soci_scoped_lock lock(&pimpl_->mtx_);

    if (pimpl_->sessions_[pos].first)
    {
        throw soci_error("Cannot release pool entry (already free)");
    }

    pimpl_->sessions_[pos].first = true;

    ReleaseSemaphore(pimpl_->sem_, 1, NULL);
}

#endif // _WIN32

connection_pool::connection_pool(std::size_t size)
               : pimpl_(std::make_unique<connection_pool_impl>(size))
{
}

connection_pool::~connection_pool() = default;

session & connection_pool::at(std::size_t pos)
{
    if (pos >= pimpl_->sessions_.size())
    {
        throw soci_error("Invalid pool position");
    }

    return *(pimpl_->sessions_[pos].second);
}

std::size_t connection_pool::lease()
{
    std::size_t pos SOCI_DUMMY_INIT(0);

    // no timeout, so can't fail
    try_lease(pos, -1);

    return pos;
}


