//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/oracle/soci-oracle.h"
#include "soci/statement.h"
#include "clob.h"
#include "error.h"
#include "soci/soci-platform.h"
#include "soci-mktime.h"
#include "soci-vector-helpers.h"
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::oracle;

void oracle_vector_into_type_backend::prepare_indicators(std::size_t size)
{
    if (size == 0)
    {
         throw soci_error("Vectors of size 0 are not allowed.");
    }

    indOCIHolderVec_.resize(size);

    sizes_.resize(size);
    rCodes_.resize(size);
}

void oracle_vector_into_type_backend::define_by_pos_bulk(
    int & position, void * data, exchange_type type,
    std::size_t begin, std::size_t * end)
{
    data_ = data; // for future reference
    type_ = type; // for future reference
    begin_ = begin;
    end_ = end;

    end_var_ = full_size();

    ub2 oracleType SOCI_DUMMY_INIT(0);
    sb4 elementSize SOCI_DUMMY_INIT(0);
    void * dataBuf = NULL;

    switch (type)
    {
    // simple cases
    case x_char:
        {
            oracleType = SQLT_AFC;
            elementSize = sizeof(char);
            std::vector<char> *vp = static_cast<std::vector<char> *>(data);
            std::vector<char> &v(*vp);
            prepare_indicators(size());
            dataBuf = &v[begin_];
        }
        break;
    case x_int8:
        {
            oracleType = SQLT_INT;
            elementSize = sizeof(int8_t);
            std::vector<int8_t> *vp = static_cast<std::vector<int8_t> *>(data);
            std::vector<int8_t> &v(*vp);
            prepare_indicators(size());
            dataBuf = &v[begin_];
        }
        break;
    case x_uint8:
        {
            oracleType = SQLT_UIN;
            elementSize = sizeof(uint8_t);
            std::vector<uint8_t> *vp = static_cast<std::vector<uint8_t> *>(data);
            std::vector<uint8_t> &v(*vp);
            prepare_indicators(size());
            dataBuf = &v[begin_];
        }
        break;
    case x_int16:
        {
            oracleType = SQLT_INT;
            elementSize = sizeof(int16_t);
            std::vector<int16_t> *vp = static_cast<std::vector<int16_t> *>(data);
            std::vector<int16_t> &v(*vp);
            prepare_indicators(size());
            dataBuf = &v[begin_];
        }
        break;
    case x_uint16:
        {
            oracleType = SQLT_UIN;
            elementSize = sizeof(uint16_t);
            std::vector<uint16_t> *vp = static_cast<std::vector<uint16_t> *>(data);
            std::vector<uint16_t> &v(*vp);
            prepare_indicators(size());
            dataBuf = &v[begin_];
        }
        break;
    case x_int32:
        {
            oracleType = SQLT_INT;
            elementSize = sizeof(int32_t);
            std::vector<int32_t> *vp = static_cast<std::vector<int32_t> *>(data);
            std::vector<int32_t> &v(*vp);
            prepare_indicators(size());
            dataBuf = &v[begin_];
        }
        break;
    case x_uint32:
        {
            oracleType = SQLT_UIN;
            elementSize = sizeof(uint32_t);
            std::vector<uint32_t> *vp = static_cast<std::vector<uint32_t> *>(data);
            std::vector<uint32_t> &v(*vp);
            prepare_indicators(size());
            dataBuf = &v[begin_];
        }
        break;
    case x_double:
        {
            oracleType = statement_.session_.get_double_sql_type();
            elementSize = sizeof(double);
            std::vector<double> *vp = static_cast<std::vector<double> *>(data);
            std::vector<double> &v(*vp);
            prepare_indicators(size());
            dataBuf = &v[begin_];
        }
        break;

    // cases that require adjustments and buffer management

    case x_int64:
        {
            oracleType = SQLT_STR;
            const std::size_t vecSize = size();
            colSize_ = 100; // arbitrary buffer size for each entry
            std::size_t const bufSize = colSize_ * vecSize;
            buf_ = new char[bufSize];

            prepare_indicators(vecSize);

            elementSize = static_cast<sb4>(colSize_);
            dataBuf = buf_;
        }
        break;
    case x_uint64:
        {
            oracleType = SQLT_STR;
            const std::size_t vecSize = size();
            colSize_ = 100; // arbitrary buffer size for each entry
            std::size_t const bufSize = colSize_ * vecSize;
            buf_ = new char[bufSize];

            prepare_indicators(vecSize);

            elementSize = static_cast<sb4>(colSize_);
            dataBuf = buf_;
        }
        break;
    case x_stdstring:
        {
            oracleType = SQLT_CHR;
            const std::size_t vecSize = size();
            colSize_ = statement_.column_size(position) + 1;
            std::size_t bufSize = colSize_ * vecSize;
            buf_ = new char[bufSize];

            prepare_indicators(vecSize);

            elementSize = static_cast<sb4>(colSize_);
            dataBuf = buf_;
        }
        break;
    case x_stdtm:
        {
            oracleType = SQLT_DAT;
            const std::size_t vecSize = size();

            prepare_indicators(vecSize);

            elementSize = 7; // 7 is the size of SQLT_DAT
            std::size_t bufSize = elementSize * vecSize;

            buf_ = new char[bufSize];
            dataBuf = buf_;
        }
        break;

    case x_xmltype:
    case x_longstring:
        {
            oracleType = SQLT_CLOB;
            std::size_t const vecSize = size();

            prepare_indicators(vecSize);

            elementSize = sizeof(OCILobLocator*);

            buf_ = new char[elementSize * vecSize];
            dataBuf = buf_;
        }
        break;

    case x_statement:
    case x_rowid:
    case x_blob:
    case x_stdwstring:
        throw soci_error("Unsupported type for vector into parameter");
    }

    sword res = OCIDefineByPos(statement_.stmtp_, &defnp_,
        statement_.session_.errhp_,
        position++, dataBuf, elementSize, oracleType,
        &indOCIHolderVec_[0], &sizes_[0], &rCodes_[0], OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, statement_.session_.errhp_);
    }
}

void oracle_vector_into_type_backend::pre_exec(int /* num */)
{
    if (type_ == x_xmltype || type_ == x_longstring)
    {
        // lazy initialization of the temporary LOB objects
        OCILobLocator** const lobps = reinterpret_cast<OCILobLocator**>(buf_);

        std::size_t const vecSize = size();
        for (std::size_t i = 0; i != vecSize; ++i)
        {
            lobps[i] = create_temp_lob(statement_.session_);
        }
    }
}

void oracle_vector_into_type_backend::pre_fetch()
{
    // nothing to do for the supported types
}

void oracle_vector_into_type_backend::post_fetch(bool gotData, indicator * ind)
{
    if (gotData)
    {
        // first, deal with data

        // only std::string, std::tm, int64 and Statement need special handling
        if (type_ == x_stdstring)
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data_);

            std::vector<std::string> &v(*vp);

            char *pos = buf_;
            std::size_t const vecSize = size();
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                if (indOCIHolderVec_[i] != -1)
                {
                    v[begin_ + i].assign(pos, sizes_[i]);
                }
                pos += colSize_;
            }
        }
        else if (type_ == x_int64)
        {
            std::vector<int64_t> *vp
                = static_cast<std::vector<int64_t> *>(data_);

            std::vector<int64_t> &v(*vp);

            char *pos = buf_;
            std::size_t const vecSize = size();
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                if (indOCIHolderVec_[i] != -1)
                {
                    v[begin_ + i] = std::strtoll(pos, NULL, 10);
                }
                pos += colSize_;
            }
        }
        else if (type_ == x_uint64)
        {
            std::vector<uint64_t> *vp
                = static_cast<std::vector<uint64_t> *>(data_);

            std::vector<uint64_t> &v(*vp);

            char *pos = buf_;
            std::size_t const vecSize = size();
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                if (indOCIHolderVec_[i] != -1)
                {
                    v[begin_ + i] = std::strtoull(pos, NULL, 10);
                }
                pos += colSize_;
            }
        }
        else if (type_ == x_stdtm)
        {
            std::vector<std::tm> *vp
                = static_cast<std::vector<std::tm> *>(data_);

            std::vector<std::tm> &v(*vp);

            ub1 *pos = reinterpret_cast<ub1*>(buf_);
            std::size_t const vecSize = size();
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                if (indOCIHolderVec_[i] == -1)
                {
                     pos += 7; // size of SQLT_DAT
                }
                else
                {
                    int year = (*pos++ - 100) * 100;
                    year += *pos++ - 100;
                    int const month = *pos++;
                    int const day = *pos++;
                    int const hour = *pos++ - 1;
                    int const minute = *pos++ - 1;
                    int const second = *pos++ - 1;

                    details::mktime_from_ymdhms(v[begin_ + i],
                        year, month, day, hour, minute, second);
                }
            }
        }
        else if (type_ == x_xmltype || type_ == x_longstring)
        {
            OCILobLocator** const lobps = reinterpret_cast<OCILobLocator**>(buf_);

            std::size_t const vecSize = size();
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                if (indOCIHolderVec_[i] != -1)
                {
                    read_from_lob(statement_.session_,
                        lobps[i], vector_string_value(type_, data_, i));
                }
            }
        }
        else if (type_ == x_statement)
        {
            statement *st = static_cast<statement *>(data_);
            st->define_and_bind();
        }

        // then - deal with indicators
        if (ind != NULL)
        {
            std::size_t const indSize = statement_.get_number_of_rows();
            for (std::size_t i = 0; i != indSize; ++i)
            {
                if (indOCIHolderVec_[i] == 0)
                {
                    ind[begin_ + i] = i_ok;
                }
                else if (indOCIHolderVec_[i] == -1)
                {
                    ind[begin_ + i] = i_null;
                }
                else
                {
                    ind[begin_ + i] = i_truncated;
                }
            }
        }
        else
        {
            std::size_t const indSize = indOCIHolderVec_.size();
            for (std::size_t i = 0; i != indSize; ++i)
            {
                if (indOCIHolderVec_[i] == -1)
                {
                    // fetched null and no indicator - programming error!
                    throw soci_error(
                        "Null value fetched and no indicator defined.");
                }
            }
        }
    }
    else // gotData == false
    {
        // nothing to do here, vectors are truncated anyway
    }
}

void oracle_vector_into_type_backend::resize(std::size_t sz)
{
    if (user_ranges_)
    {
        // resize only in terms of user-provided ranges (below)
    }
    else
    {
        resize_vector(type_, data_, sz);

        end_var_ = sz;
    }

    // resize ranges, either user-provided or internally managed
    *end_ = begin_ + sz;
}

std::size_t oracle_vector_into_type_backend::size() const
{
    // as a special error-detection measure, check if the actual vector size
    // was changed since the original bind (when it was stored in end_var_):
    const std::size_t actual_size = full_size();
    if (actual_size != end_var_)
    {
        // ... and in that case return the actual size
        return actual_size;
    }

    if (end_ != NULL && *end_ != 0)
    {
        return *end_ - begin_;
    }
    else
    {
        return end_var_;
    }
}

std::size_t oracle_vector_into_type_backend::full_size() const
{
    return get_vector_size(type_, data_);
}

void oracle_vector_into_type_backend::clean_up()
{
    if (type_ == x_longstring || type_ == x_xmltype)
    {
        OCILobLocator** lobps = reinterpret_cast<OCILobLocator**>(buf_);

        std::size_t const vecSize = size();
        for (std::size_t i = 0; i != vecSize; ++i)
        {
            free_temp_lob(statement_.session_, lobps[i]);
        }
    }

    if (defnp_ != NULL)
    {
        OCIHandleFree(defnp_, OCI_HTYPE_DEFINE);
        defnp_ = NULL;
    }

    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
