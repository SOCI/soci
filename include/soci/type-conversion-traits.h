//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED
#define SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED

#include "soci/soci-backend.h"
#include "soci/is-detected.h"

#include <type_traits>

namespace soci
{

// default traits class type_conversion, acts as pass through for row::get() or row::move_as()
// when no actual conversion is needed.
template <typename T, typename Enable = void>
struct type_conversion
{
    typedef T base_type;

    struct from_base_check : std::integral_constant<bool, std::is_constructible<T, const base_type>::value> {};

    static void from_base(base_type const& in, indicator ind, T & out)
    {
        static_assert(from_base_check::value,
                "from_base can only be used if the target type can be constructed from an lvalue base reference");
        assert_non_null(ind);
        out = in;
    }

    struct move_from_base_check :
        std::integral_constant<bool,
            !std::is_const<base_type>::value
            && std::is_constructible<T, typename std::add_rvalue_reference<base_type>::type>::value
        > {};


    static void move_from_base(base_type & in, indicator ind, T & out)
    {
        static_assert(move_from_base_check::value,
                "move_to_base can only be used if the target type can be constructed from an rvalue base reference");
        assert_non_null(ind);
        out = std::move(in);
    }

    static void to_base(T const & in, base_type & out, indicator & ind)
    {
        out = in;
        ind = i_ok;
    }

    static void move_to_base(T & in, base_type & out, indicator & ind)
    {
        out = std::move(in);
        ind = i_ok;
    }

private:
    static void assert_non_null(indicator ind)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type");
        }
    }
};

namespace details
{

#if !defined(_MSC_VER) || (_MSC_VER >= 1910)
    // MSVC 2015 and earlier don't support SFINAE on return types
    #define SOCI_HAS_RET_TYPE_SFINAE
#endif

template<typename T>
using from_base_check_t = decltype(T::from_base_check::value);
template<typename T>
using move_from_base_check_t = decltype(T::move_from_base_check::value);

template<typename T>
using supports_from_base_check = is_detected<from_base_check_t, T>;
template<typename T>
using supports_move_from_base_check = is_detected<move_from_base_check_t, T>;

#ifdef SOCI_HAS_RET_TYPE_SFINAE

template<typename Trait>
constexpr auto can_use_from_base()
    -> typename std::enable_if<supports_from_base_check<Trait>::value, bool>::type
{
    return Trait::from_base_check::value;
}

template<typename Trait>
constexpr auto can_use_from_base()
    -> typename std::enable_if<!supports_from_base_check<Trait>::value, bool>::type
{
    return true;
}

template<typename Trait>
constexpr auto can_use_move_from_base()
    -> typename std::enable_if<supports_move_from_base_check<Trait>::value, bool>::type
{
    return Trait::from_base_check::value;
}

template<typename Trait>
constexpr auto can_use_move_from_base()
    -> typename std::enable_if<!supports_move_from_base_check<Trait>::value, bool>::type
{
    // Default to assuming that the special move_from_base function is not implemented
    // TODO: Find clever template magic to add a metaprogramming check for a suitable
    // move_from_base implementation
    return false;
}

#undef SOCI_HAS_RET_TYPE_SFINAE

#else

// Always return true - if it doesn't work the user will get a cryptic compiler error but at least
// it will compile properly for cases in which things do indeed work

template<typename Trait>
constexpr auto can_use_from_base()
{
    return true;
}

template<typename Trait>
constexpr auto can_use_move_from_base()
{
    return true;
}

#endif

}

} // namespace soci

#endif // SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED
