//
// Copyright (C) 2023 Elyas El Idrissi
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_STD_PATH_H_INCLUDED
#define SOCI_STD_PATH_H_INCLUDED

#include "soci/type-conversion-traits.h"

#include <filesystem>
#include <string>

namespace soci
{

// simple fall-back for std::filesystem::path
template <>
struct type_conversion<std::filesystem::path>
{
	typedef std::string base_type;

	static void from_base(base_type in, indicator ind, std::filesystem::path & out)
	{
		out = ind != i_null ? std::filesystem::path{in} : std::filesystem::path{};
	}

	static void to_base(std::filesystem::path in, base_type & out, indicator & ind)
	{
		out = in.string();
		ind = i_ok;
	}
};

} // namespace soci

#endif // SOCI_STD_PATH_H_INCLUDED
