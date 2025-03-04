//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TEST_MYINT_H_INCLUDED
#define SOCI_TEST_MYINT_H_INCLUDED

#include "soci/soci.h"

// user-defined object used in some tests
class MyInt
{
public:
    MyInt() : i_() {}
    MyInt(int i) : i_(i) {}
    void set(int i) { i_ = i; }
    int get() const { return i_; }
private:
    int i_;
};

namespace soci
{

// basic type conversion for user-defined type with single base value
template<> struct type_conversion<MyInt>
{
    typedef int base_type;

    static void from_base(int i, indicator ind, MyInt &mi)
    {
        if (ind == i_ok)
        {
            mi.set(i);
        }
    }

    static void to_base(MyInt const &mi, int &i, indicator &ind)
    {
        i = mi.get();
        ind = i_ok;
    }
};

} // namespace soci

#endif // SOCI_TEST_MYINT_H_INCLUDED
