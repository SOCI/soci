#ifndef SOCI_BIND_VALUES_H_INCLUDED
#define SOCI_BIND_VALUES_H_INCLUDED

#include "soci/soci-platform.h"
#include "exchange-traits.h"
#include "into-type.h"
#include "into.h"
#include "soci-backend.h"
#include "use-type.h"
#include "use.h"


#ifdef SOCI_HAVE_BOOST
#       include <boost/fusion/algorithm/iteration/for_each.hpp>
#       include <boost/mpl/bool.hpp>
#endif // SOCI_HAVE_BOOST
#include <vector>

namespace soci
{
namespace details
{

class use_type_vector
{
public:
    typedef std::vector<use_type_base*> type;
    typedef typename type::iterator iterator;
    typedef typename type::const_iterator const_iterator;
    typedef typename type::reverse_iterator reverse_iterator;
    typedef typename type::const_reverse_iterator const_reverse_iterator;
public:
    virtual ~use_type_vector()
    {
    }

    // std mapping - begin
    type::value_type& operator[](const type::size_type pos)
    { return _data.operator[](pos); }

    const type::value_type& operator[](const type::size_type pos) const
    { return _data.operator[](pos); }

    type::value_type& at(const type::size_type pos)
    { return _data.at(pos); }

    const type::value_type& at(const type::size_type pos) const
    { return _data.at(pos); }

    void reserve(const type::size_type maxCount)
    { _data.reserve(maxCount); }

    void resize(const type::size_type newSize)
    { _data.resize(newSize); }

    type& dataVec()
    { return _data; }

    const type& dataVec() const
    { return _data; }

    iterator begin()
    { return _data.begin(); }

    const_iterator begin() const
    { return _data.begin(); }

    iterator end()
    { return _data.end(); }

    const_iterator end() const
    { return _data.end(); }

    reverse_iterator rbegin()
    { return _data.rbegin(); }

    const_reverse_iterator rbegin() const
    { return _data.rbegin(); }

    reverse_iterator rend()
    { return _data.rend(); }

    const_reverse_iterator rend() const
    { return _data.rend(); }

    const_iterator cbegin() const
    { return _data.cbegin(); }

    const_iterator cend() const
    { return _data.cend(); }

    const_reverse_iterator crbegin() const
    { return _data.crbegin(); }

    const_reverse_iterator crend() const
    { return _data.crend(); }

    void clear()
    { return _data.clear(); }

    void swap(use_type_vector& right)
    { _data.swap(right.dataVec()); }

    type::size_type size() const
    { return _data.size(); }

    bool empty() const
    { return _data.empty(); }

    void push_back(type::value_type u)
    { _data.push_back(u); }
    // std mapping - end

    void exchange(use_type_ptr const& u) { push_back(u.get()); u.release(); }

    template <typename T, typename Indicator>
    void exchange(use_container<T, Indicator> const &uc)
    {
#ifdef SOCI_HAVE_BOOST
        exchange_(uc, (typename boost::fusion::traits::is_sequence<T>::type *)NULL);
#else
        exchange_(uc, NULL);
#endif // SOCI_HAVE_BOOST
    }

private:
#ifdef SOCI_HAVE_BOOST
    template <typename T, typename Indicator>
    struct use_sequence
    {
        use_sequence(use_type_vector &_p, Indicator &_ind)
            :p(_p), ind(_ind) {}

        template <typename T2>
        void operator()(T2 &t2) const
        {
            p.exchange(use(t2, ind));
        }

        use_type_vector &p;
        Indicator &ind;
    private:
        SOCI_NOT_COPYABLE(use_sequence)
    };

    template <typename T>
    struct use_sequence<T, details::no_indicator>
    {
        use_sequence(use_type_vector &_p)
            :p(_p) {}

        template <typename T2>
        void operator()(T2 &t2) const
        {
            p.exchange(use(t2));
        }

        use_type_vector &p;
    private:
        SOCI_NOT_COPYABLE(use_sequence)
    };

    template <typename T, typename Indicator>
    void exchange_(use_container<T, Indicator> const &uc, boost::mpl::true_ * /* fusion sequence */)
    {
        boost::fusion::for_each(uc.t, use_sequence<T, Indicator>(_data, uc.ind));
    }

    template <typename T>
    void exchange_(use_container<T, details::no_indicator> const &uc, boost::mpl::true_ * /* fusion sequence */)
    {
        boost::fusion::for_each(uc.t, use_sequence<T, details::no_indicator>(_data));
    }

#endif // SOCI_HAVE_BOOST

    template <typename T, typename Indicator>
    void exchange_(use_container<T, Indicator> const &uc, ...)
    { exchange(do_use(uc.t, uc.ind, uc.name, typename details::exchange_traits<T>::type_family())); }

    template <typename T>
    void exchange_(use_container<T, details::no_indicator> const &uc, ...)
    { exchange(do_use(uc.t, uc.name, typename details::exchange_traits<T>::type_family())); }

    template <typename T, typename Indicator>
    void exchange_(use_container<const T, Indicator> const &uc, ...)
    { exchange(do_use(uc.t, uc.ind, uc.name, typename details::exchange_traits<T>::type_family())); }

    template <typename T>
    void exchange_(use_container<const T, details::no_indicator> const &uc, ...)
    { exchange(do_use(uc.t, uc.name, typename details::exchange_traits<T>::type_family())); }

private:
    type _data;
};

class into_type_vector
{
public:
    typedef std::vector<into_type_base*> type;
    typedef typename type::iterator iterator;
    typedef typename type::const_iterator const_iterator;
    typedef typename type::reverse_iterator reverse_iterator;
    typedef typename type::const_reverse_iterator const_reverse_iterator;
public:
    virtual ~into_type_vector()
    {
    }

    // std mapping - begin
    type::value_type& operator[](const type::size_type pos)
    { return _data.operator[](pos); }

    const type::value_type& operator[](const type::size_type pos) const
    { return _data.operator[](pos); }

    type::value_type& at(const type::size_type pos)
    { return _data.at(pos); }

    const type::value_type& at(const type::size_type pos) const
    { return _data.at(pos); }

    void reserve(const type::size_type maxCount)
    { _data.reserve(maxCount); }

    void resize(const type::size_type newSize)
    { _data.resize(newSize); }

    type& dataVec()
    { return _data; }

    const type& dataVec() const
    { return _data; }

    iterator begin()
    { return _data.begin(); }

    const_iterator begin() const
    { return _data.begin(); }

    iterator end()
    { return _data.end(); }

    const_iterator end() const
    { return _data.end(); }

    reverse_iterator rbegin()
    { return _data.rbegin(); }

    const_reverse_iterator rbegin() const
    { return _data.rbegin(); }

    reverse_iterator rend()
    { return _data.rend(); }

    const_reverse_iterator rend() const
    { return _data.rend(); }

    const_iterator cbegin() const
    { return _data.cbegin(); }

    const_iterator cend() const
    { return _data.cend(); }

    const_reverse_iterator crbegin() const
    { return _data.crbegin(); }

    const_reverse_iterator crend() const
    { return _data.crend(); }

    void clear()
    { return _data.clear(); }

    void swap(into_type_vector& right)
    { _data.swap(right.dataVec()); }

    type::size_type size() const
    { return _data.size(); }

    bool empty() const
    { return _data.empty(); }

    void push_back(type::value_type u)
    { _data.push_back(u); }
    // std mapping - end

    void exchange(into_type_ptr const& i) { push_back(i.get()); i.release(); }

    template <typename T, typename Indicator>
    void exchange(into_container<T, Indicator> const &ic)
    {
#ifdef SOCI_HAVE_BOOST
        exchange_(ic, (typename boost::fusion::traits::is_sequence<T>::type *)NULL);
#else
        exchange_(ic, NULL);
#endif // SOCI_HAVE_BOOST
    }

private:
#ifdef SOCI_HAVE_BOOST
    template <typename T, typename Indicator>
    struct into_sequence
    {
        into_sequence(into_type_vector &_p, Indicator &_ind)
            :p(_p), ind(_ind) {}

        template <typename T2>
        void operator()(T2 &t2) const
        {
            p.exchange(into(t2, ind));
        }

        into_type_vector &p;
        Indicator &ind;
    private:
        SOCI_NOT_COPYABLE(into_sequence)
    };

    template <typename T>
    struct into_sequence<T, details::no_indicator>
    {
        into_sequence(into_type_vector &_p)
            :p(_p) {}

        template <typename T2>
        void operator()(T2 &t2) const
        {
            p.exchange(into(t2));
        }

        into_type_vector &p;
    private:
        SOCI_NOT_COPYABLE(into_sequence)
    };

    template <typename T, typename Indicator>
    void exchange_(into_container<T, Indicator> const &ic, boost::mpl::true_ * /* fusion sequence */)
    {
        boost::fusion::for_each(ic.t, into_sequence<T, Indicator>(_data, ic.ind));
    }

    template <typename T>
    void exchange_(into_container<T, details::no_indicator> const &ic, boost::mpl::true_ * /* fusion sequence */)
    {
        boost::fusion::for_each(ic.t, into_sequence<T, details::no_indicator>(_data));
    }
#endif // SOCI_HAVE_BOOST

    template <typename T, typename Indicator>
    void exchange_(into_container<T, Indicator> const &ic, ...)
    { exchange(do_into(ic.t, ic.ind, typename details::exchange_traits<T>::type_family())); }

    template <typename T>
    void exchange_(into_container<T, details::no_indicator> const &ic, ...)
    { exchange(do_into(ic.t, typename details::exchange_traits<T>::type_family())); }

private:
    type _data;
};

} // namespace details
}// namespace soci
#endif // SOCI_BIND_VALUES_H_INCLUDED
