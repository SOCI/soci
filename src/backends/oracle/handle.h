//
// Copyright (C) 2025 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ORACLE_HANDLE_H_INCLUDED
#define SOCI_ORACLE_HANDLE_H_INCLUDED

#include "soci/oracle/soci-oracle.h"

namespace soci
{

namespace details
{

namespace oracle
{

template <typename Handle>
struct oci_traits;

template <>
struct oci_traits<OCIStmt>
{
    static constexpr ub4 type = OCI_HTYPE_STMT;
};

template <>
struct oci_traits<OCIError>
{
    static constexpr ub4 type = OCI_HTYPE_ERROR;
};

template <typename OCIHandle>
class handle
{
public:
    static constexpr ub4 HandleType = oci_traits<OCIHandle>::type;

    explicit handle(OCIEnv *envhp)
        : handle_(NULL)
    {
        sword res = OCIHandleAlloc(envhp,
            reinterpret_cast<dvoid**>(&handle_),
            HandleType, 0, 0);
        if (res != OCI_SUCCESS)
        {
            throw oracle_soci_error("Failed to allocate handler", 0);
        }
    }

    ~handle()
    {
        OCIHandleFree(handle_, HandleType);
    }

    handle(handle const &) = delete;
    handle& operator=(handle const &) = delete;

    OCIHandle* get() { return handle_; }
    operator OCIHandle*() { return handle_; }

    // This is needed to allow passing handle to OCI functions.
    void** ptr() { return reinterpret_cast<void**>(&handle_); }

private:
    OCIHandle* handle_;
};

} // namespace oracle

} // namespace details

} // namespace soci

#endif // SOCI_ORACLE_HANDLE_H_INCLUDED
