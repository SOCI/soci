//
// Copyright (C) 2023 Elyas El Idrissi
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_STD_PTR_H_INCLUDED
#define SOCI_STD_PTR_H_INCLUDED

#include "soci/type-conversion-traits.h"

#include <memory>

namespace soci
{
	// simple fall-back for std::unique_ptr
	template <typename T>
	struct type_conversion<std::unique_ptr<T>>
	{
		typedef typename type_conversion<T>::base_type base_type;

		static void from_base(base_type const & in, indicator ind, std::unique_ptr<T> & out)
		{
			if (ind == i_null)
			{
				out.reset();
			}
			else
			{
				T tmp = T();
				type_conversion<T>::from_base(in, ind, tmp);
				out = std::make_unique<T>(tmp);
			}
		}

		static void to_base(std::unique_ptr<T> const & in, base_type & out, indicator & ind)
		{
			if (in)
			{
				type_conversion<T>::to_base(*in, out, ind);
			}
			else
			{
				ind = i_null;
			}
		}
	};

	// simple fall-back for std::shared_ptr
	template <typename T>
	struct type_conversion<std::shared_ptr<T> >
	{
		typedef typename type_conversion<T>::base_type base_type;

		static void from_base(base_type const & in, indicator ind, std::shared_ptr<T> & out)
		{
			if (ind == i_null)
			{
				out.reset();
			}
			else
			{
				T tmp = T();
				type_conversion<T>::from_base(in, ind, tmp);
				out = std::make_shared<T>(tmp);
			}
		}

		static void to_base(std::shared_ptr<T> const & in, base_type & out, indicator & ind)
		{
			if (in)
			{
				type_conversion<T>::to_base(*in, out, ind);
			}
			else
			{
				ind = i_null;
			}
		}
	};
}

#endif // SOCI_STD_PTR_H_INCLUDED
