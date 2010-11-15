//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BOOST_FUSION_H_INCLUDED
#define SOCI_BOOST_FUSION_H_INCLUDED

#include "values.h"
#include "type-conversion-traits.h"
// boost
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/utility/enable_if.hpp>

namespace soci
{
namespace detail
{
template <typename Seq, int size>
struct type_conversion;

template <typename Seq>
struct type_conversion<Seq, 1>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 2>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 3>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out)
            >> boost::fusion::at_c<2>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in)
            << boost::fusion::at_c<2>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 4>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out)
            >> boost::fusion::at_c<2>(out)
            >> boost::fusion::at_c<3>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in)
            << boost::fusion::at_c<2>(in)
            << boost::fusion::at_c<3>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 5>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out)
            >> boost::fusion::at_c<2>(out)
            >> boost::fusion::at_c<3>(out)
            >> boost::fusion::at_c<4>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in)
            << boost::fusion::at_c<2>(in)
            << boost::fusion::at_c<3>(in)
            << boost::fusion::at_c<4>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 6>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out)
            >> boost::fusion::at_c<2>(out)
            >> boost::fusion::at_c<3>(out)
            >> boost::fusion::at_c<4>(out)
            >> boost::fusion::at_c<5>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in)
            << boost::fusion::at_c<2>(in)
            << boost::fusion::at_c<3>(in)
            << boost::fusion::at_c<4>(in)
            << boost::fusion::at_c<5>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 7>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out)
            >> boost::fusion::at_c<2>(out)
            >> boost::fusion::at_c<3>(out)
            >> boost::fusion::at_c<4>(out)
            >> boost::fusion::at_c<5>(out)
            >> boost::fusion::at_c<6>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in)
            << boost::fusion::at_c<2>(in)
            << boost::fusion::at_c<3>(in)
            << boost::fusion::at_c<4>(in)
            << boost::fusion::at_c<5>(in)
            << boost::fusion::at_c<6>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 8>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out)
            >> boost::fusion::at_c<2>(out)
            >> boost::fusion::at_c<3>(out)
            >> boost::fusion::at_c<4>(out)
            >> boost::fusion::at_c<5>(out)
            >> boost::fusion::at_c<6>(out)
            >> boost::fusion::at_c<7>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in)
            << boost::fusion::at_c<2>(in)
            << boost::fusion::at_c<3>(in)
            << boost::fusion::at_c<4>(in)
            << boost::fusion::at_c<5>(in)
            << boost::fusion::at_c<6>(in)
            << boost::fusion::at_c<7>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 9>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out)
            >> boost::fusion::at_c<2>(out)
            >> boost::fusion::at_c<3>(out)
            >> boost::fusion::at_c<4>(out)
            >> boost::fusion::at_c<5>(out)
            >> boost::fusion::at_c<6>(out)
            >> boost::fusion::at_c<7>(out)
            >> boost::fusion::at_c<8>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in)
            << boost::fusion::at_c<2>(in)
            << boost::fusion::at_c<3>(in)
            << boost::fusion::at_c<4>(in)
            << boost::fusion::at_c<5>(in)
            << boost::fusion::at_c<6>(in)
            << boost::fusion::at_c<7>(in)
            << boost::fusion::at_c<8>(in);
    }
};

template <typename Seq>
struct type_conversion<Seq, 10>
{
    typedef values base_type;

    static void from_base(base_type const & in, indicator /*ind*/, Seq & out)
    {
        in
            >> boost::fusion::at_c<0>(out)
            >> boost::fusion::at_c<1>(out)
            >> boost::fusion::at_c<2>(out)
            >> boost::fusion::at_c<3>(out)
            >> boost::fusion::at_c<4>(out)
            >> boost::fusion::at_c<5>(out)
            >> boost::fusion::at_c<6>(out)
            >> boost::fusion::at_c<7>(out)
            >> boost::fusion::at_c<8>(out)
            >> boost::fusion::at_c<9>(out);
    }

    static void to_base(Seq & in, base_type & out, indicator & /*ind*/)
    {
        out
            << boost::fusion::at_c<0>(in)
            << boost::fusion::at_c<1>(in)
            << boost::fusion::at_c<2>(in)
            << boost::fusion::at_c<3>(in)
            << boost::fusion::at_c<4>(in)
            << boost::fusion::at_c<5>(in)
            << boost::fusion::at_c<6>(in)
            << boost::fusion::at_c<7>(in)
            << boost::fusion::at_c<8>(in)
            << boost::fusion::at_c<9>(in);
    }
};

} // namespace detail

template <typename T>
struct type_conversion<T, 
    typename boost::enable_if<
        boost::fusion::traits::is_sequence<T>
    >::type >
{
    typedef values base_type;

private:
    typedef typename boost::fusion::result_of::size<T>::type size;
    typedef detail::type_conversion<T, size::value> converter;

public:
    static void from_base(base_type const & in, indicator ind, T& out)
    {
        converter::from_base( in, ind, out );
    }

    static void to_base(T& in, base_type & out, indicator & ind)
    {
        converter::to_base( in, out, ind );
    }
};

} // namespace soci

#endif // SOCI_BOOST_FUSION_H_INCLUDED
