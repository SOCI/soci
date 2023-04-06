//
// Copyright (C) 2023 Elyas El Idrissi
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BOOL_H_INCLUDED
#define SOCI_BOOL_H_INCLUDED

#include "soci/type-conversion-traits.h"

namespace soci
{

// simple fall-back for bool
template <>
struct type_conversion<bool>
{
	typedef short base_type;

	static void from_base(base_type in, indicator ind, bool & out)
	{
		out = ind != i_null ? static_cast<bool>(in) : false;
	}

	static void to_base(bool in, base_type & out, indicator & ind)
	{
		out = static_cast<base_type>(in);
		ind = i_ok;
	}
};

} // namespace soci

#endif // SOCI_BOOL_H_INCLUDED
