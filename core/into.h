#ifndef INTO_H_INCLUDED
#define INTO_H_INCLUDED

#include "into-type.h"
#include "type-conversion.h"

namespace soci
{

// the into function is a helper for defining output variables
// these helpers work with both basic and user-defined types thanks to
// the tag-dispatching, as defined in exchange_traits template

template <typename T>
details::into_type_ptr into(T &t)
{
    return details::do_into(t, typename details::exchange_traits<T>::type_family());
}

template <typename T>
details::into_type_ptr into(T &t, eIndicator &indicator)
{
    return details::do_into(t, indicator,
        typename details::exchange_traits<T>::type_family());
}

template <typename T>
details::into_type_ptr into(T &t, std::vector<eIndicator> &indicator)
{
    return details::do_into(t, indicator,
        typename details::exchange_traits<T>::type_family());
}

// for char buffer with run-time size information
template <typename T>
details::into_type_ptr into(T &t, std::size_t bufSize)
{
    return details::into_type_ptr(new details::into_type<T>(t, bufSize));
}

} // namespace soci

#endif // INTO_H_INCLUDED
