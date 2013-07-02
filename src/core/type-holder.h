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

        const std::type_info& ti = this->type();

        if  (ti == typeid(int))
        {                            
            return (const T&)cast<int>(pos);          
        }
        else if (ti == typeid(double))
        {                    
            return (const T&)cast<double>(pos);       
        }
        else if (ti == typeid(std::string))
        {
            return (const T&)cast<std::string>(pos);
        }
        else if (ti == typeid(long long))
        {
            return (const T&)cast<long long>(pos);
        }
        else if (ti == typeid(unsigned long long))
        {
            return (const T&)cast<unsigned long long>(pos);
        }
        else if (ti == typeid(std::tm))
        {
            return (const T&)cast<std::tm>(pos);
        }
        else
        {
            throw soci_error("bad type");
        }
    }

    virtual const std::type_info & type() const = 0;

private:
    template<typename T>
    T& cast(size_t pos)
    {
        vector_type_holder<T>* p = static_cast<vector_type_holder<T> *>(this);
        return p->template value<T>(pos);
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

} // namespace details

} // namespace soci

#endif // SOCI_TYPE_HOLDER_H_INCLUDED
