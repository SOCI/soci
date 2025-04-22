//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci/odbc/soci-odbc.h"

using namespace soci;

soci_error::error_category odbc_soci_error::get_error_category() const
{
    const char* const s = reinterpret_cast<const char*>(sqlstate_);

    if ((s[0] == '0' && s[1] == '8') ||
        strcmp(s, "HYT01") == 0)
        return connection_error;

    if (strcmp(s, "23000") == 0 ||
        strcmp(s, "40002") == 0 ||
        strcmp(s, "44000") == 0)
        return constraint_violation;

    if (strcmp(s, "HY014") == 0)
        return system_error;

    return unknown;
}

std::string
odbc_soci_error::interpret_odbc_error(SQLSMALLINT htype,
                                      SQLHANDLE hndl,
                                      std::string const& msg)
{
    const char* socierror = NULL;

    SQLSMALLINT length, i = 1;
    switch ( SQLGetDiagRecA(htype, hndl, i, sqlstate_, &sqlcode_,
                           message_, SQL_MAX_MESSAGE_LENGTH + 1,
                           &length) )
    {
      case SQL_SUCCESS:
        // The error message was successfully retrieved.
        break;

      case SQL_INVALID_HANDLE:
        socierror = "[SOCI]: Invalid handle.";
        break;

      case SQL_ERROR:
        socierror = "[SOCI]: SQLGetDiagRec() error.";
        break;

      case SQL_SUCCESS_WITH_INFO:
        socierror = "[SOCI]: Error message too long.";
        break;

      case SQL_NO_DATA:
        socierror = "[SOCI]: No error.";
        break;

      default:
        socierror = "[SOCI]: Unexpected SQLGetDiagRec() return value.";
        break;
    }

    if (socierror)
    {
        // Use our own error message if we failed to retrieve the ODBC one.
        strcpy(reinterpret_cast<char*>(message_), socierror);

        // Use "General warning" SQLSTATE code.
        strcpy(reinterpret_cast<char*>(sqlstate_), "01000");

        sqlcode_ = 0;
    }

    std::ostringstream ss;
    ss << "Error " << msg << ": " << message_ << " (SQL state " << sqlstate_ << ")";

    return ss.str();
}

odbc_soci_error::odbc_soci_error(SQLSMALLINT htype,
                                 SQLHANDLE hndl,
                                 std::string const & msg)
    : soci_error(interpret_odbc_error(htype, hndl, msg))
{
}
