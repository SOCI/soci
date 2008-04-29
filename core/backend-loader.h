//
// Copyright (C) 2008 Maciej Sobczak with contributions from Artyom Tonkikh
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BACKEND_LOADER_H_INCLUDED
#define SOCI_BACKEND_LOADER_H_INCLUDED

#include "soci-backend.h"
#include <string>
#include <vector>

namespace soci
{

namespace dynamic_backends
{

// used internally by session
backend_factory const & get(std::string const & name);

// provided for advanced user-level management
std::vector<std::string> & search_paths();
void register_backend(std::string const & name, std::string const & shared_object = std::string());
void register_backend(std::string const & name, backend_factory const & factory);
std::vector<std::string> list_all();
void unload(std::string const & name);
void unload_all();

} // namespace dynamic_backends

} // namespace soci

#endif // SOCI_BACKEND_LOADER_H_INCLUDED
