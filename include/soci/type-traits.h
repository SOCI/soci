//
// Copyright (C) 2023 Robert Adam
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_TYPE_TRAITS_H_INCLUDED
#define SOCI_PRIVATE_SOCI_TYPE_TRAITS_H_INCLUDED

#include <type_traits>

namespace soci
{

namespace details
{

template<typename...>
using void_t = void;

using false_type = std::integral_constant<bool, false>;
using true_type = std::integral_constant<bool, true>;

// Implementation from https://blog.tartanllama.xyz/detection-idiom/

namespace detector_detail
{

    template <template <class...> class Trait, class Enabler, class... Args>
    struct is_detected : false_type {};

    template <template <class...> class Trait, class... Args>
    struct is_detected<Trait, void_t<Trait<Args...>>, Args...> : true_type {};

}


template <template <class...> class Trait, class... Args>
using is_detected = typename detector_detail::is_detected<Trait, void, Args...>::type;

}

}

#endif // SOCI_PRIVATE_SOCI_TYPE_TRAITS_H_INCLUDED
