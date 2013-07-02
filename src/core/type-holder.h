//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_HOLDER_H_INCLUDED
#define SOCI_TYPE_HOLDER_H_INCLUDED
// std
#include <typeinfo>
#include <vector>

namespace soci
{

namespace details
{

//convertible is for pass test at [commontests.h, test12()]. Otherwise the test is an undefined runtime error
//default:: any type convert is allowed 
template <typename from, typename to> class convertible{public:enum {IS = 1};};

// Base class holder + derived class type_holder for storing type data
// instances in a container of holder objects
template <typename T>
class vector_type_holder;

class vector_holder
{
public:
    vector_holder() {}
    virtual ~vector_holder() {}

    template<typename T>
    const T& get(size_t pos)
    {
        //statement_impl::bind_into have those types::
        //std::string, double, int, long long, unsigned long long, std::tm

        //typeid() is faster than dynamic_cast

        const std::type_info& ti = this->type();

        if  (ti == typeid(int))
        {
            return (const T&)cast<int, T>(pos);          
        }
        else if (ti == typeid(double))
        {                    
            return (const T&)cast<double, T>(pos);       
        }
        else if (ti == typeid(std::string))
        {
            return (const T&)cast<std::string, T>(pos);
        }
        else if (ti == typeid(long long))
        {
            return (const T&)cast<long long, T>(pos);
        }
        else if (ti == typeid(unsigned long long))
        {
            return (const T&)cast<unsigned long long, T>(pos);
        }
        else if (ti == typeid(std::tm))
        {
            return (const T&)cast<std::tm, T>(pos);
        }
        else
        {
            throw soci_error("not support type");
        }
    }

    virtual const std::type_info & type() const = 0;

private:
    template<typename T, typename R>
    T& cast(size_t pos)
    {
        //T can convert to R. e.g.: T t; R r; t = r;
        if (convertible<T, R>::IS)
        {
            vector_type_holder<T>* p = static_cast<vector_type_holder<T> *>(this);
            return p->template value<T>(pos);
        }

        throw std::bad_cast();
    }

    template<typename T>
    T& value(size_t pos) const;
};

template <typename T>
class vector_type_holder : public vector_holder
{
public:
    vector_type_holder(std::vector<T> *vec) : vec_(vec) {}
    ~vector_type_holder() { delete vec_; }

    template<typename TypeValue>
    TypeValue& value(size_t pos) const { return (*vec_)[pos];}

    const std::type_info & type() const {return typeid(T);}

private:
    std::vector<T>* vec_;
};

#define CONVERTIBLE(T, R, yes_no) \
    template<> class convertible<T, R>{public:enum {IS = yes_no};}

//std::string
CONVERTIBLE(int, std::string, 0);
CONVERTIBLE(double, std::string, 0);
CONVERTIBLE(long long, std::string, 0);
CONVERTIBLE(unsigned long long, std::string, 0);
CONVERTIBLE(std::tm, std::string, 0);
CONVERTIBLE(std::string, int, 0);
CONVERTIBLE(std::string, double, 0);
CONVERTIBLE(std::string, long long, 0);
CONVERTIBLE(std::string, unsigned long long, 0);
CONVERTIBLE(std::string, std::tm, 0);
//std::tm
CONVERTIBLE(int, std::tm, 0);
CONVERTIBLE(double, std::tm, 0);
CONVERTIBLE(long long, std::tm, 0);
CONVERTIBLE(unsigned long long, std::tm, 0);
CONVERTIBLE(std::tm, int, 0);
CONVERTIBLE(std::tm, double, 0);
CONVERTIBLE(std::tm, long long, 0);
CONVERTIBLE(std::tm, unsigned long long, 0);



/*
//string
template<> class convertible<int,                string>{public:enum {IS = 0};};
template<> class convertible<double,             string>{public:enum {IS = 0};};
template<> class convertible<long long,          string>{public:enum {IS = 0};};
template<> class convertible<unsigned long long, string>{public:enum {IS = 0};};
template<> class convertible<std::tm,            string>{public:enum {IS = 0};};
template<> class convertible<string, int               >{public:enum {IS = 0};};
template<> class convertible<string, double            >{public:enum {IS = 0};};
template<> class convertible<string, long long         >{public:enum {IS = 0};};
template<> class convertible<string, unsigned long long>{public:enum {IS = 0};};
template<> class convertible<string, std::tm           >{public:enum {IS = 0};};

//std::tm
template<> class convertible<int,                std::tm>{public:enum {IS = 0};};
template<> class convertible<double,             std::tm>{public:enum {IS = 0};};
template<> class convertible<long long,          std::tm>{public:enum {IS = 0};};
template<> class convertible<unsigned long long, std::tm>{public:enum {IS = 0};};
template<> class convertible<string,             std::tm>{public:enum {IS = 0};};
template<> class convertible<std::tm, int               >{public:enum {IS = 0};};
template<> class convertible<std::tm, double            >{public:enum {IS = 0};};
template<> class convertible<std::tm, long long         >{public:enum {IS = 0};};
template<> class convertible<std::tm, unsigned long long>{public:enum {IS = 0};};
template<> class convertible<std::tm, string            >{public:enum {IS = 0};};
*/


} // namespace details

} // namespace soci

#endif // SOCI_TYPE_HOLDER_H_INCLUDED
