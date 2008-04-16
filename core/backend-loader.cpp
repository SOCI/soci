//
// Copyright (C) 2008 Maciej Sobczak with contributions from Artyom Tonkikh
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "backend-loader.h"
#include "error.h"
#include <map>
#include <cassert>

using namespace std;
using namespace soci;
using namespace soci::dynamic_backends;

#ifndef SOCI_LIBRARY_SUFFIX
#define SOCI_LIBRARY_SUFFIX ""
#endif

#ifdef _WIN32

#include <windows.h>

typedef CRITICAL_SECTION soci_mutex_t;
typedef HMODULE soci_handler_t;

#define LOCK(x)  EnterCriticalSection(x)
#define UNLOCK(x) LeaveCriticalSection(x)
#define MUTEX_INIT(x) InitializeCriticalSection(x)
#define MUTEX_DEST(x) DeleteCriticalSection(x)
#define DLOPEN(x) LoadLibrary(x)
#define DLCLOSE(x) FreeLibrary(x)
#define DLSYM(x, y) GetProcAddress(x, y)
#define LIBNAME(x) ("libsoci_" + x + SOCI_LIBRARY_SUFFIX + ".dll")

#else

#include <pthread.h>
#include <dlfcn.h>

typedef pthread_mutex_t soci_mutex_t;
typedef void * soci_handler_t;

#define LOCK(x)  pthread_mutex_lock(x)
#define UNLOCK(x) pthread_mutex_unlock(x)
#define MUTEX_INIT(x) pthread_mutex_init(x, NULL)
#define MUTEX_DEST(x) pthread_mutex_destroy(x)
#define DLOPEN(x) dlopen(x, RTLD_LAZY)
#define DLCLOSE(x) dlclose(x)
#define DLSYM(x, y) dlsym(x, y)
#define LIBNAME(x) ("libsoci_" + x + SOCI_LIBRARY_SUFFIX + ".so")

#endif // _WIN32


namespace // unnamed
{

struct info
{
    soci_handler_t handler_;
    backend_factory const * factory_;
    info() : handler_(NULL), factory_(NULL) {}
};

typedef map<string, info> factory_map;
factory_map factories_;

soci_mutex_t mutex_;

// used to automatically initialize the mutex above
struct mutex_mgr
{
    mutex_mgr()
    {
        MUTEX_INIT(&mutex_);
    }

    ~mutex_mgr()
    {
        unload_all();

        MUTEX_DEST(&mutex_);
    }
} mutex_mgr_;

class scoped_lock
{
public:
    scoped_lock(soci_mutex_t * m) : mptr(m) { LOCK(m); };
    ~scoped_lock() { UNLOCK(mptr); };
private:
    soci_mutex_t * mptr;
};

// non-synchronized helper for the other functions
void do_unload(string const & name)
{
    factory_map::iterator i = factories_.find(name);

    if (i != factories_.end())
    {
        soci_handler_t h = i->second.handler_;
        if (h != NULL)
        {
            DLCLOSE(h);
        }

        factories_.erase(i);
    }
}

// non-synchronized helper
void do_register_backend(
    string const & name, string const & shared_object)
{
    std::string so = (shared_object.empty() ? LIBNAME(name) : shared_object);

    soci_handler_t h = DLOPEN(so.c_str());
    if (h == NULL)
    {
        throw soci_error("Failed to open: " + so);
    }

    std::string symbol = "factory_" + name;

    typedef backend_factory const * bfc_ptr;
    typedef bfc_ptr (*get_t)(void);
    get_t entry;
    entry = reinterpret_cast<get_t>(reinterpret_cast<long>(DLSYM(h, symbol.c_str())));

    if (entry == NULL)
    {
        DLCLOSE(h);
        throw soci_error("Failed to resolve dynamic symbol: " + symbol);
    }

    // unload the existing handler if it's already loaded

    do_unload(name);
    
    backend_factory const * f = entry();

    info new_entry;
    new_entry.factory_ = f;
    new_entry.handler_ = h;

    factories_[name] = new_entry;
}

} // unnamed namespace

backend_factory const & dynamic_backends::get(string const & name)
{
    scoped_lock lock(&mutex_);

    factory_map::iterator i = factories_.find(name);

    if (i != factories_.end())
    {
        return *(i->second.factory_);
    }

    // no backend found with this name, try to register it first

    do_register_backend(name, std::string());

    // second attempt, must succeed (the backend is already loaded)

    i = factories_.find(name);

    assert(i != factories_.end());

    return *(i->second.factory_);
}

void dynamic_backends::register_backend(
    string const & name, string const & shared_object)
{
    scoped_lock lock(&mutex_);

    do_register_backend(name, shared_object);
}

void dynamic_backends::register_backend(
    string const & name, backend_factory const & factory)
{
    scoped_lock lock(&mutex_);

    // unload the existing handler if it's already loaded

    do_unload(name);
    
    info new_entry;
    new_entry.factory_ = &factory;

    factories_[name] = new_entry;
}

std::vector<std::string> dynamic_backends::list_all()
{
    scoped_lock lock(&mutex_);

    std::vector<std::string> ret;
    ret.reserve(factories_.size());

    factory_map::iterator i;
    for (i = factories_.begin(); i != factories_.end(); ++i)
    {
        std::string const & name = i->first;
        ret.push_back(name);
    }

    return ret;
}

void dynamic_backends::unload(string const & name)
{
    scoped_lock lock(&mutex_);

    do_unload(name);
}

void dynamic_backends::unload_all()
{
    scoped_lock lock(&mutex_);

    factory_map::iterator i;
    for (i = factories_.begin(); i != factories_.end(); ++i)
    {
        soci_handler_t h = i->second.handler_;
        if (h != NULL)
        {
            DLCLOSE(h);
        }
    }

    factories_.clear();
}
