//
// Copyright (C) 2025 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_MUTEX_H_INCLUDED
#define SOCI_PRIVATE_SOCI_MUTEX_H_INCLUDED

#ifdef _WIN32

#include <windows.h>

class soci_mutex_t
{
public:
    soci_mutex_t() { ::InitializeCriticalSection(&cs_); }
    ~soci_mutex_t() { ::DeleteCriticalSection(&cs_); }

    void lock() { ::EnterCriticalSection(&cs_); }
    void unlock() { ::LeaveCriticalSection(&cs_); }

    soci_mutex_t(soci_mutex_t const &) = delete;
    soci_mutex_t& operator=(soci_mutex_t const &) = delete;

private:
    CRITICAL_SECTION cs_;
};

#else // Assume POSIX

#include <pthread.h>

class soci_mutex_t
{
public:
    soci_mutex_t() { pthread_mutex_init(&m_, NULL); }
    soci_mutex_t(soci_mutex_t const &) = delete;
    soci_mutex_t& operator=(soci_mutex_t const &) = delete;

    ~soci_mutex_t() { pthread_mutex_destroy(&m_); }

    void lock() { pthread_mutex_lock(&m_); }
    void unlock() { pthread_mutex_unlock(&m_); }

    // This should be only used with pthread_cond_[timed]wait().
    pthread_mutex_t * native_handle() { return &m_; }

private:
    pthread_mutex_t m_;
};

#endif // _WIN32/POSIX

class soci_scoped_lock
{
public:
    explicit soci_scoped_lock(soci_mutex_t * m) : m_(m) { m_->lock(); };
    ~soci_scoped_lock() { m_->unlock(); };

    soci_scoped_lock(soci_scoped_lock const &) = delete;
    soci_scoped_lock& operator=(soci_scoped_lock const &) = delete;

private:
    soci_mutex_t * const m_;
};

#endif // SOCI_PRIVATE_SOCI_MUTEX_H_INCLUDED
