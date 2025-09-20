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

using namespace soci;

namespace
{

// Common base class for both POSIX and Windows implementations.
struct connection_pool_base_impl
{
    explicit connection_pool_base_impl(std::size_t size)
    {
        if (size == 0)
        {
            throw soci_error("Invalid pool size");
        }

        sessions_.resize(size);
        for (std::size_t i = 0; i != size; ++i)
        {
            sessions_[i] = std::make_pair(true, new session());
        }
    }

    ~connection_pool_base_impl()
    {
        for (std::size_t i = 0; i != sessions_.size(); ++i)
        {
            delete sessions_[i].second;
        }
    }

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
    std::vector<std::pair<bool, session *> > sessions_;
};

} // anonymous namespace

#ifndef _WIN32
// POSIX implementation

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

struct connection_pool::connection_pool_impl : connection_pool_base_impl
{
    explicit connection_pool_impl(std::size_t size)
        : connection_pool_base_impl(size)
    {
        if (pthread_mutex_init(&mtx_, NULL) != 0)
        {
            throw soci_error("Synchronization error");
        }

        if (pthread_cond_init(&cond_, NULL) != 0)
        {
            throw soci_error("Synchronization error");
        }
    }

    connection_pool_impl(connection_pool_impl const&) = delete;
    connection_pool_impl& operator=(connection_pool_impl const&) = delete;

    ~connection_pool_impl()
    {
        pthread_mutex_destroy(&mtx_);
        pthread_cond_destroy(&cond_);
    }

    pthread_mutex_t mtx_;
    pthread_cond_t cond_;
};

connection_pool::connection_pool(std::size_t size)
{
    pimpl_ = new connection_pool_impl(size);
}

connection_pool::~connection_pool()
{
    delete pimpl_;
}

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

    int cc = pthread_mutex_lock(&(pimpl_->mtx_));
    if (cc != 0)
    {
        throw soci_error("Synchronization error");
    }

    while (pimpl_->find_free(pos) == false)
    {
        if (timeout < 0)
        {
            // no timeout, allow unlimited blocking
            cc = pthread_cond_wait(&(pimpl_->cond_), &(pimpl_->mtx_));
        }
        else
        {
            // wait with timeout
            cc = pthread_cond_timedwait(
                &(pimpl_->cond_), &(pimpl_->mtx_), &tm);
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

    pthread_mutex_unlock(&(pimpl_->mtx_));

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

    int cc = pthread_mutex_lock(&(pimpl_->mtx_));
    if (cc != 0)
    {
        throw soci_error("Synchronization error");
    }

    if (pimpl_->sessions_[pos].first)
    {
        pthread_mutex_unlock(&(pimpl_->mtx_));
        throw soci_error("Cannot release pool entry (already free)");
    }

    pimpl_->sessions_[pos].first = true;

    pthread_mutex_unlock(&(pimpl_->mtx_));

    pthread_cond_signal(&(pimpl_->cond_));
}

#else
// Windows implementation

#include <windows.h>

struct connection_pool::connection_pool_impl : connection_pool_base_impl
{
    explicit connection_pool_impl(std::size_t size)
        : connection_pool_base_impl(size)
    {
        InitializeCriticalSection(&mtx_);

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
        DeleteCriticalSection(&mtx_);
        CloseHandle(sem_);
    }

    CRITICAL_SECTION mtx_;
    HANDLE sem_;
};

connection_pool::connection_pool(std::size_t size)
{
    pimpl_ = new connection_pool_impl(size);
}

connection_pool::~connection_pool()
{
    delete pimpl_;
}

bool connection_pool::try_lease(std::size_t & pos, int timeout)
{
    DWORD cc = WaitForSingleObject(pimpl_->sem_,
        timeout >= 0 ? static_cast<DWORD>(timeout) : INFINITE);
    if (cc == WAIT_OBJECT_0)
    {
        // semaphore acquired, there is (at least) one free entry

        EnterCriticalSection(&(pimpl_->mtx_));

        if (!pimpl_->find_free(pos))
        {
            // this should be impossible
            throw soci_error("Getting connection from the pool unexpectedly failed");
        }

        pimpl_->sessions_[pos].first = false;

        LeaveCriticalSection(&(pimpl_->mtx_));

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

    EnterCriticalSection(&(pimpl_->mtx_));

    if (pimpl_->sessions_[pos].first)
    {
        LeaveCriticalSection(&(pimpl_->mtx_));
        throw soci_error("Cannot release pool entry (already free)");
    }

    pimpl_->sessions_[pos].first = true;

    LeaveCriticalSection(&(pimpl_->mtx_));

    ReleaseSemaphore(pimpl_->sem_, 1, NULL);
}

#endif // _WIN32

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


