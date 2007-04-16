#ifndef USE_H_INCLUDED
#define USE_H_INCLUDED

#include "use-type.h"
#include "type-conversion.h"

namespace soci
{

// the use function is a helper for defining input variables
// these helpers work with both basic and user-defined types thanks to
// the tag-dispatching, as defined in exchange_traits template

template <typename T>
details::use_type_ptr use(T &t, std::string const &name = std::string())
{
    return details::do_use(t, name,
        typename details::exchange_traits<T>::type_family());
}

template <typename T>
details::use_type_ptr use(T &t, eIndicator &indicator,
    std::string const &name = std::string())
{
    return details::do_use(t, indicator, name,
        typename details::exchange_traits<T>::type_family());
}

template <typename T>
details::use_type_ptr use(T &t, std::vector<eIndicator> &indicator,
    std::string const &name = std::string())
{
    return details::do_use(t, indicator, name,
        typename details::exchange_traits<T>::type_family());
}

// for char buffer with run-time size information
template <typename T>
details::use_type_ptr use(T &t, std::size_t bufSize,
    std::string const &name = std::string())
{
    return details::use_type_ptr(new details::use_type<T>(t, bufSize));
}

} // namespace soci

#endif // USE_H_INCLUDED
