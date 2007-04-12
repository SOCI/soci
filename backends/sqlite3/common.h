//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SQLITE3_COMMON_H_INCLUDED
#define SQLITE3_COMMON_H_INCLUDED

#include <vector>

namespace soci {

namespace details {

namespace sqlite3 {

// helper function for parsing datetime values
void parseStdTm(char const *buf, std::tm &t);

// helper for vector operations
template <typename T>
std::size_t getVectorSize(void *p)
{
    std::vector<T> *v = static_cast<std::vector<T> *>(p);
    return v->size();
}

template <typename T>
void resizeVector(void *p, std::size_t sz)
{
    std::vector<T> *v = static_cast<std::vector<T> *>(p);
    v->resize(sz);
}

} // namespace Sqlite3

} // namespace details

} // namespace SOCI

#endif // SQLITE3_COMMON_H_INCLUDED

