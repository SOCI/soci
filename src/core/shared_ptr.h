#ifndef _SOCI_CORE_SHARED_PTR
#define _SOCI_CORE_SHARED_PTR

#ifdef SOCI_USE_BOOST
#include <boost/shared_ptr.hpp>

namespace soci
{

template <typename T>
struct shared_ptr: boost::shared_ptr<T>
{
    shared_ptr(T *t): boost::shared_ptr<T> { }
};

} // namespace soci

#else // SOCI_USE_BOOST

#include <assert.h>

namespace soci
{

namespace details
{
    template <typename T>
    struct shared_ptr_counter
    {
        shared_ptr_counter(T *t, const size_t c): ptr(t), counter(c) {}

        T       *ptr;
        size_t   counter;
    };
} // namespace details

template <typename T>
struct shared_ptr
{
    shared_ptr(): m_ptr(NULL) {}
    shared_ptr(T *t)
        : m_ptr(new details::shared_ptr_counter<T>(t, 1)) {}
    shared_ptr(const shared_ptr &sp): m_ptr(sp.m_ptr) { inc(); }

    ~shared_ptr() { dec(); }

    shared_ptr<T> &operator=(const shared_ptr &sp) { dec(); m_ptr = sp.m_ptr; inc(); return *this; }

    T &operator*() { assert(m_ptr); return *m_ptr->ptr; }
    T *operator->() const { assert(m_ptr); return m_ptr->ptr; }

    T *get() { assert(m_ptr); return m_ptr->ptr; }

    private:
        void inc() { if (m_ptr) m_ptr->counter++; }
        void dec() 
        { 
            if (!m_ptr) return;
            if (--m_ptr->counter == 0)
            {
                delete m_ptr->ptr;
                delete m_ptr;
            }
        }

    private:
        details::shared_ptr_counter<T> *m_ptr;
};

} // namespace soci

#endif

#endif