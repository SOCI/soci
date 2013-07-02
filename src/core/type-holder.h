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


//this class is for pass test [commontests.h, test12()], 
//otherwise the test will failed with undefined runtime error.
//default:: any type convert is not allowed 
template <typename from, typename to> 
class convertible{public:enum {IS = 0};};

template<typename T>
class convertible<T, T>{public:enum {IS = 1};};


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
            return cast<int, T>(pos);          
        }
        else if (ti == typeid(double))
        {                    
            return cast<double, T>(pos);       
        }
        else if (ti == typeid(std::string))
        {
            return cast<std::string, T>(pos);
        }
        else if (ti == typeid(long long))
        {
            return cast<long long, T>(pos);
        }
        else if (ti == typeid(unsigned long long))
        {
            return cast<unsigned long long, T>(pos);
        }
        else if (ti == typeid(std::tm))
        {
            return cast<std::tm, T>(pos);
        }
        else
        {
            throw soci_error("not support type");
        }
    }

    virtual const std::type_info & type() const = 0;

private:
    template<typename Holder_T, typename T>
    const T& cast(size_t pos)
    {
#ifdef _MSC_VER
#pragma warning( push ) 
#pragma warning(disable:4127)
#endif // _MSC_VER

        //Holder_T can convert to T.  sample code: T t; Holder_T s; t=s;
        if (convertible<Holder_T, T>::IS)   //diable warning 4127
        {
            vector_type_holder<Holder_T>* p = 
                static_cast<vector_type_holder<Holder_T> *>(this);
            return (const T&)p->template value<Holder_T>(pos);
        }

#ifdef _MSC_VER
#pragma warning( pop ) 
#endif // _MSC_VER

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

//partial convertible class
#define CONVERTIBLE(T, R, yes_no) \
    template<> class convertible<T, R>{public:enum {IS = yes_no};}

#define INTEGRAL_CONVERTIBLE(T) \
    CONVERTIBLE(T, bool, 1);\
    CONVERTIBLE(T, char, 1);\
    CONVERTIBLE(T, unsigned char, 1);\
    CONVERTIBLE(T, short, 1);\
    CONVERTIBLE(T, unsigned short, 1);\
    CONVERTIBLE(T, int, 1);\
    CONVERTIBLE(T, unsigned int, 1);\
    CONVERTIBLE(T, long, 1);\
    CONVERTIBLE(T, unsigned long, 1);\
    CONVERTIBLE(T, long long, 1);\
    CONVERTIBLE(T, unsigned long long, 1);\
    CONVERTIBLE(T, float, 1);\
    CONVERTIBLE(T, double, 1);\
    CONVERTIBLE(T, long double, 1)

//int, double, long long, unsigned long long
INTEGRAL_CONVERTIBLE(int);
INTEGRAL_CONVERTIBLE(double);
INTEGRAL_CONVERTIBLE(long long);
INTEGRAL_CONVERTIBLE(unsigned long long);


} // namespace details

} // namespace soci

#endif // SOCI_TYPE_HOLDER_H_INCLUDED
