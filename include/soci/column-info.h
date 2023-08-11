//
// Copyright (C) 2016 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_COLUMN_INFO_H_INCLUDED
#define SOCI_COLUMN_INFO_H_INCLUDED

#include "soci/soci-backend.h"
#include "soci/type-conversion.h"
#include "soci/values.h"

#include <cstdint>

namespace soci
{

struct SOCI_DECL column_info
{
    std::string name;
    // DEPRECATED. USE dataType INSTEAD.
    data_type type;
    db_type dataType;
    std::size_t length; // meaningful for text columns only
    std::size_t precision;
    std::size_t scale;
    bool nullable;
};

template <>
struct type_conversion<column_info>
{
    typedef values base_type;

    static std::size_t get_numeric_value(const values & v,
        const std::string & field_name)
    {
        db_type dt = v.get_properties(field_name).get_db_type();
        switch (dt)
        {
        case db_double:
            return static_cast<std::size_t>(
                v.get<double>(field_name, 0.0));
        case db_int8:
            return static_cast<std::size_t>(
                v.get<int8_t>(field_name, 0));
        case db_uint8:
            return static_cast<std::size_t>(
                v.get<uint8_t>(field_name, 0));
        case db_int16:
            return static_cast<std::size_t>(
                v.get<int16_t>(field_name, 0));
        case db_uint16:
            return static_cast<std::size_t>(
                v.get<uint16_t>(field_name, 0));
        case db_int32:
            return static_cast<std::size_t>(
                v.get<int32_t>(field_name, 0));
        case db_uint32:
            return static_cast<std::size_t>(
                v.get<uint32_t>(field_name, 0));
        case db_int64:
            return static_cast<std::size_t>(
                v.get<int64_t>(field_name, 0ll));
        case db_uint64:
            return static_cast<std::size_t>(
                v.get<uint64_t>(field_name, 0ull));
        default:
            return 0u;
        }
    }

    static void from_base(values const & v, indicator /* ind */, column_info & ci)
    {
        ci.name = v.get<std::string>("COLUMN_NAME");

        ci.length = get_numeric_value(v, "CHARACTER_MAXIMUM_LENGTH");
        ci.precision = get_numeric_value(v, "NUMERIC_PRECISION");
        ci.scale = get_numeric_value(v, "NUMERIC_SCALE");

        const std::string & type_name = v.get<std::string>("DATA_TYPE");
        if (type_name == "text" || type_name == "TEXT" ||
            type_name == "clob" || type_name == "CLOB" ||
            type_name.find("char") != std::string::npos ||
            type_name.find("CHAR") != std::string::npos)
        {
            ci.type = dt_string;
            ci.dataType = db_string;
        }
        else if (type_name == "integer" || type_name == "INTEGER")
        {
            ci.type = dt_integer;
            ci.dataType = db_int32;
        }
        else if (type_name.find("number") != std::string::npos ||
            type_name.find("NUMBER") != std::string::npos ||
            type_name.find("numeric") != std::string::npos ||
            type_name.find("NUMERIC") != std::string::npos)
        {
            if (ci.scale != 0)
            {
                ci.type = dt_double;
                ci.dataType = db_double;
            }
            else
            {
                ci.type = dt_integer;
                ci.dataType = db_int32;
            }
        }
        else if (type_name.find("time") != std::string::npos ||
            type_name.find("TIME") != std::string::npos ||
            type_name.find("date") != std::string::npos ||
            type_name.find("DATE") != std::string::npos)
        {
            ci.type = dt_date;
            ci.dataType = db_date;
        }
        else if (type_name.find("blob") != std::string::npos ||
            type_name.find("BLOB") != std::string::npos ||
            type_name.find("oid") != std::string::npos ||
            type_name.find("OID") != std::string::npos)
        {
            ci.type = dt_blob;
            ci.dataType = db_blob;
        }
        else if (type_name.find("xml") != std::string::npos ||
            type_name.find("XML") != std::string::npos)
        {
            ci.type = dt_xml;
            ci.dataType = db_xml;
        }
        else
        {
            // this seems to be a safe default
            ci.type = dt_string;
            ci.dataType = db_string;
        }

        const std::string & nullable_s = v.get<std::string>("IS_NULLABLE");
        ci.nullable = (nullable_s == "YES");
    }
};

} // namespace soci

#endif // SOCI_COLUMN_INFO_H_INCLUDED
