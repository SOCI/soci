//
// Copyright (C) 2008 Maciej Sobczak with contributions from Artyom Tonkikh
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/soci-platform.h"
#include "soci/backend-loader.h"
#include "soci/error.h"
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#ifndef _MSC_VER
#include <stdint.h>
#endif

#include "soci_backends_config.h"

using namespace soci;
using namespace soci::dynamic_backends;

#ifdef _WIN32

#include <windows.h>

typedef CRITICAL_SECTION soci_mutex_t;
typedef HMODULE soci_handler_t;

#define LOCK(x) EnterCriticalSection(x)
#define UNLOCK(x) LeaveCriticalSection(x)
#define MUTEX_INIT(x) InitializeCriticalSection(x)
#define MUTEX_DEST(x) DeleteCriticalSection(x)
#ifdef _UNICODE
#define DLOPEN(x) LoadLibraryA(x)
#else
#define DLOPEN(x) LoadLibrary(x)
#endif
#define DLCLOSE(x) FreeLibrary(x)
#define DLSYM(x, y) GetProcAddress(x, y)

#ifdef SOCI_ABI_VERSION
  #ifndef NDEBUG
    #define LIBNAME(x) (SOCI_LIB_PREFIX + x + "_" SOCI_ABI_VERSION SOCI_DEBUG_POSTFIX SOCI_LIB_SUFFIX)
  #else
    #define LIBNAME(x) (SOCI_LIB_PREFIX + x + "_" SOCI_ABI_VERSION SOCI_LIB_SUFFIX)
  #endif
#else
#define LIBNAME(x) (SOCI_LIB_PREFIX + x + SOCI_LIB_SUFFIX)
#endif // SOCI_ABI_VERSION

#else

#include <pthread.h>
#include <dlfcn.h>

typedef pthread_mutex_t soci_mutex_t;
typedef void * soci_handler_t;

#define LOCK(x) pthread_mutex_lock(x)
#define UNLOCK(x) pthread_mutex_unlock(x)
#define MUTEX_INIT(x) pthread_mutex_init(x, NULL)
#define MUTEX_DEST(x) pthread_mutex_destroy(x)
#define DLOPEN(x) dlopen(x, RTLD_LAZY)
#define DLCLOSE(x) dlclose(x)
#define DLSYM(x, y) dlsym(x, y)

#ifdef SOCI_ABI_VERSION

#ifdef __APPLE__
#define LIBNAME(x) (SOCI_LIB_PREFIX + x + "." SOCI_ABI_VERSION SOCI_LIB_SUFFIX)
#else
#define LIBNAME(x) (SOCI_LIB_PREFIX + x + SOCI_LIB_SUFFIX "." SOCI_ABI_VERSION)
#endif

#else
#define LIBNAME(x) (SOCI_LIB_PREFIX + x + SOCI_LIB_SUFFIX)
#endif // SOCI_ABI_VERSION

#endif // _WIN32


namespace // unnamed
{

struct info
{
    soci_handler_t handler_;
    backend_factory const * factory_;

    // The use count is the number of existing sessions using this backend (in
    // fact it's the count of connection_parameters objects, but as these
    // objects are part of the session, it's roughly the same thing).
    //
    // While use count is non-zero, the corresponding backend can't be unloaded
    // as this would leave the code using it with dangling (code) pointers. If
    // it reaches 0, the backend is _not_ unloaded automatically because we
    // don't want to unload/reload it all the time when recreating sessions,
    // but it can be unloaded manually, if necessary, by calling unload() or
    // unload_all() functions.
    int use_count_;

    // If unloading this backend is requested while its use count is non-zero,
    // this flag is set to true and the backend is actually unloaded when the
    // use count drops to 0.
    bool unload_requested_;

    info() : handler_(0), factory_(0), use_count_(0), unload_requested_(false) {}
};

typedef std::map<std::string, info> factory_map;
factory_map factories_;

std::vector<std::string> search_paths_;

soci_mutex_t mutex_;

std::vector<std::string> get_default_paths()
{
    std::vector<std::string> paths;

    char const* const penv = std::getenv("SOCI_BACKENDS_PATH");
    std::string const env(penv ? penv : "");
    if (env.empty())
    {
        paths.push_back(".");
#ifdef DEFAULT_BACKENDS_PATH
        paths.push_back(DEFAULT_BACKENDS_PATH);
#endif // DEFAULT_BACKENDS_PATH
        return paths;
    }

    std::string::size_type searchFrom = 0;
    while (searchFrom != env.size())
    {
        std::string::size_type const found = env.find(":", searchFrom);
        if (found == searchFrom)
        {
            ++searchFrom;
        }
        else if (std::string::npos != found)
        {
            std::string const path(env.substr(searchFrom, found - searchFrom));
            paths.push_back(path);

            searchFrom = found + 1;
        }
        else // found == npos
        {
            std::string const path = env.substr(searchFrom);
            paths.push_back(path);

            searchFrom = env.size();
        }
    }

    return paths;
}

// used to automatically initialize the global state
struct static_state_mgr
{
    static_state_mgr()
    {
        MUTEX_INIT(&mutex_);

        search_paths_ = get_default_paths();
    }

    ~static_state_mgr()
    {
        unload_all();

        MUTEX_DEST(&mutex_);
    }
} static_state_mgr_;

class scoped_lock
{
public:
    scoped_lock(soci_mutex_t * m) : mptr(m) { LOCK(m); };
    ~scoped_lock() { UNLOCK(mptr); };
private:
    soci_mutex_t * mptr;
};

// non-synchronized helpers for the other functions
factory_map::iterator do_unload(factory_map::iterator i)
{
    soci_handler_t h = i->second.handler_;
    if (h != NULL)
    {
        DLCLOSE(h);
    }

    // TODO-C++11: Use erase() return value.
    factory_map::iterator const to_erase = i;
    ++i;
    factories_.erase(to_erase);
    return i;
}

void do_unload_or_throw_if_in_use(std::string const & name)
{
    factory_map::iterator i = factories_.find(name);

    if (i != factories_.end())
    {
        if (i->second.use_count_)
        {
            throw soci_error("Backend " + name + " is used and can't be unloaded");
        }

        do_unload(i);
    }
}

// non-synchronized helper
void do_register_backend(std::string const & name, std::string const & shared_object)
{
    do_unload_or_throw_if_in_use(name);

    // The rules for backend search are as follows:
    // - if the shared_object is given,
    //   it names the library file and the search paths are not used
    // - otherwise (shared_object not provided or empty):
    //   - file named libsoci_NAME.so.SOVERSION is searched in the list of search paths

    soci_handler_t h = 0;
    if (shared_object.empty() == false)
    {
        h = DLOPEN(shared_object.c_str());
    }
    else
    {
        // try system paths
        h = DLOPEN(LIBNAME(name).c_str());
        if (0 == h)
        {
            // try all search paths
            for (std::size_t i = 0; i != search_paths_.size(); ++i)
            {
                std::string const fullFileName(search_paths_[i] + "/" + LIBNAME(name));
                h = DLOPEN(fullFileName.c_str());
                if (0 != h)
                {
                    // already found
                    break;
                }
             }
         }
    }

    if (0 == h)
    {
        throw soci_error("Failed to find shared library for backend " + name);
    }

    std::string symbol = "factory_" + name;

    typedef backend_factory const * bfc_ptr;
    typedef bfc_ptr (*get_t)(void);
    get_t entry;
    entry = reinterpret_cast<get_t>(
            reinterpret_cast<uintptr_t>(DLSYM(h, symbol.c_str())));

    if (0 == entry)
    {
        DLCLOSE(h);
        throw soci_error("Failed to resolve dynamic symbol: " + symbol);
    }

    backend_factory const* f = entry();

    info new_entry;
    new_entry.factory_ = f;
    new_entry.handler_ = h;

    factories_[name] = new_entry;
}

} // unnamed namespace

backend_factory const& dynamic_backends::get(std::string const& name)
{
    scoped_lock lock(&mutex_);

    factory_map::iterator i = factories_.find(name);

    if (i == factories_.end())
    {
      // no backend found with this name, try to register it first

      do_register_backend(name, std::string());

      // second attempt, must succeed (the backend is already loaded)

      i = factories_.find(name);
    }

    i->second.use_count_++;

    return *(i->second.factory_);
}

void dynamic_backends::unget(std::string const& name)
{
    scoped_lock lock(&mutex_);

    factory_map::iterator i = factories_.find(name);

    if (i == factories_.end())
    {
        // We don't throw here as this is often called from destructors, and so
        // this would result in a call to std::terminate(), even if this is
        // totally unexpected -- but, unfortunately, we have no way to report
        // it to the application without possibly killing it.
        return;
    }

    info& backend_info = i->second;

    --backend_info.use_count_;

    // Check if this backend should be unloaded if unloading it had been
    // previously requested.
    if (backend_info.use_count_ == 0 && backend_info.unload_requested_)
    {
        do_unload(i);
    }
}

SOCI_DECL std::vector<std::string>& dynamic_backends::search_paths()
{
    return search_paths_;
}

SOCI_DECL void dynamic_backends::register_backend(
    std::string const& name, std::string const& shared_object)
{
    scoped_lock lock(&mutex_);

    do_register_backend(name, shared_object);
}

SOCI_DECL void dynamic_backends::register_backend(
    std::string const& name, backend_factory const& factory)
{
    scoped_lock lock(&mutex_);

    do_unload_or_throw_if_in_use(name);

    info new_entry;
    new_entry.factory_ = &factory;

    factories_[name] = new_entry;
}

SOCI_DECL std::vector<std::string> dynamic_backends::list_all()
{
    scoped_lock lock(&mutex_);

    std::vector<std::string> ret;
    ret.reserve(factories_.size());

    for (factory_map::iterator i = factories_.begin(); i != factories_.end(); ++i)
    {
        std::string const& name = i->first;
        ret.push_back(name);
    }

    return ret;
}

SOCI_DECL void dynamic_backends::unload(std::string const& name)
{
    scoped_lock lock(&mutex_);

    factory_map::iterator i = factories_.find(name);

    if (i != factories_.end())
    {
        info& backend_info = i->second;
        if (backend_info.use_count_)
        {
            // We can't unload the backend while it's in use, so do it later
            // when it's not used any longer.
            backend_info.unload_requested_ = true;
            return;
        }

        do_unload(i);
    }
}

SOCI_DECL void dynamic_backends::unload_all()
{
    scoped_lock lock(&mutex_);

    for (factory_map::iterator i = factories_.begin(); i != factories_.end(); )
    {
        info& backend_info = i->second;

        // Same logic as in unload() above.
        if (backend_info.use_count_)
        {
            backend_info.unload_requested_ = true;
            ++i;
            continue;
        }

        i = do_unload(i);
    }
}
