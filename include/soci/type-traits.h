#ifndef SOCI_TYPE_TRAITS_H_INCLUDED
#define SOCI_TYPE_TRAITS_H_INCLUDED

namespace soci {
    template<class T, class U>
    struct is_same {
        static const bool value = false;
    };

    template<class T>
    struct is_same<T, T> {
        static const bool value = true;
    };

    template<bool B, class T = void>
    struct enable_if {
    };

    template<class T>
    struct enable_if<true, T> {
        typedef T type;
    };
}

#endif  // SOCI_TYPE_TRAITS_H_INCLUDED