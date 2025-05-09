//
// Copyright (C) 2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_SIMPLE_H_INCLUDED
#define SOCI_SIMPLE_H_INCLUDED

#include "soci/soci-platform.h"

#include <cstdint>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// session

typedef void * session_handle;
SOCI_DECL session_handle soci_create_session(char const * connectionString);
SOCI_DECL void soci_destroy_session(session_handle s);

SOCI_DECL void soci_begin(session_handle s);
SOCI_DECL void soci_commit(session_handle s);
SOCI_DECL void soci_rollback(session_handle s);

SOCI_DECL int soci_session_state(session_handle s);
SOCI_DECL char const * soci_session_error_message(session_handle s);


// blob

typedef void *blob_handle;
SOCI_DECL blob_handle soci_create_blob(session_handle s);
SOCI_DECL void soci_destroy_blob(blob_handle b);

SOCI_DECL int soci_blob_get_len(blob_handle b);
[[deprecated("Use soci_blob_read_from_start instead")]]
SOCI_DECL int soci_blob_read(blob_handle b, int offset, char *buf, int toRead);
SOCI_DECL int soci_blob_read_from_start(blob_handle b, char *buf, int toRead, int offset = 0);
[[deprecated("Use soci_blob_write_from_start instead")]]
SOCI_DECL int soci_blob_write(blob_handle b, int offset, char const *buf, int toWrite);
SOCI_DECL int soci_blob_write_from_start(blob_handle b, char const *buf, int toWrite, int offset = 0);
SOCI_DECL int soci_blob_append(blob_handle b, char const *buf, int toWrite);
SOCI_DECL int soci_blob_trim(blob_handle b, int newLen);

SOCI_DECL int soci_blob_state(blob_handle b);
SOCI_DECL char const * soci_blob_error_message(blob_handle b);


// statement

typedef void * statement_handle;
SOCI_DECL statement_handle soci_create_statement(session_handle s);
SOCI_DECL void soci_destroy_statement(statement_handle st);

// positional bind of into elments (the functions return the position for convenience)
SOCI_DECL int soci_into_string   (statement_handle st);
SOCI_DECL int soci_into_int      (statement_handle st);
SOCI_DECL int soci_into_long_long(statement_handle st);
SOCI_DECL int soci_into_int8     (statement_handle st);
SOCI_DECL int soci_into_uint8    (statement_handle st);
SOCI_DECL int soci_into_int16    (statement_handle st);
SOCI_DECL int soci_into_uint16   (statement_handle st);
SOCI_DECL int soci_into_int32    (statement_handle st);
SOCI_DECL int soci_into_uint32   (statement_handle st);
SOCI_DECL int soci_into_int64    (statement_handle st);
SOCI_DECL int soci_into_uint64   (statement_handle st);
SOCI_DECL int soci_into_double   (statement_handle st);
SOCI_DECL int soci_into_date     (statement_handle st);
SOCI_DECL int soci_into_blob     (statement_handle st);

// vector versions
SOCI_DECL int soci_into_string_v   (statement_handle st);
SOCI_DECL int soci_into_int_v      (statement_handle st);
SOCI_DECL int soci_into_long_long_v(statement_handle st);
SOCI_DECL int soci_into_int8_v     (statement_handle st);
SOCI_DECL int soci_into_uint8_v    (statement_handle st);
SOCI_DECL int soci_into_int16_v    (statement_handle st);
SOCI_DECL int soci_into_uint16_v   (statement_handle st);
SOCI_DECL int soci_into_int32_v    (statement_handle st);
SOCI_DECL int soci_into_uint32_v   (statement_handle st);
SOCI_DECL int soci_into_int64_v    (statement_handle st);
SOCI_DECL int soci_into_uint64_v   (statement_handle st);
SOCI_DECL int soci_into_double_v   (statement_handle st);
SOCI_DECL int soci_into_date_v     (statement_handle st);

// positional read of into elements
SOCI_DECL int          soci_get_into_state    (statement_handle st, int position);
SOCI_DECL char const * soci_get_into_string   (statement_handle st, int position);
SOCI_DECL int          soci_get_into_int      (statement_handle st, int position);
SOCI_DECL long long    soci_get_into_long_long(statement_handle st, int position);
SOCI_DECL int8_t       soci_get_into_int8     (statement_handle st, int position);
SOCI_DECL uint8_t      soci_get_into_uint8    (statement_handle st, int position);
SOCI_DECL int16_t      soci_get_into_int16    (statement_handle st, int position);
SOCI_DECL uint16_t     soci_get_into_uint16   (statement_handle st, int position);
SOCI_DECL int32_t      soci_get_into_int32    (statement_handle st, int position);
SOCI_DECL uint32_t     soci_get_into_uint32   (statement_handle st, int position);
SOCI_DECL int64_t      soci_get_into_int64    (statement_handle st, int position);
SOCI_DECL uint64_t     soci_get_into_uint64   (statement_handle st, int position);
SOCI_DECL double       soci_get_into_double   (statement_handle st, int position);
SOCI_DECL char const * soci_get_into_date     (statement_handle st, int position);
SOCI_DECL blob_handle  soci_get_into_blob     (statement_handle st, int position);

// positional (re)size of vectors
SOCI_DECL int  soci_into_get_size_v(statement_handle st);
SOCI_DECL void soci_into_resize_v  (statement_handle st, int new_size);

// positional read of vectors
SOCI_DECL int          soci_get_into_state_v    (statement_handle st, int position, int index);
SOCI_DECL char const * soci_get_into_string_v   (statement_handle st, int position, int index);
SOCI_DECL int          soci_get_into_int_v      (statement_handle st, int position, int index);
SOCI_DECL long long    soci_get_into_long_long_v(statement_handle st, int position, int index);
SOCI_DECL int8_t       soci_get_into_int8_v     (statement_handle st, int position, int index);
SOCI_DECL uint8_t      soci_get_into_uint8_v    (statement_handle st, int position, int index);
SOCI_DECL int16_t      soci_get_into_int16_v    (statement_handle st, int position, int index);
SOCI_DECL uint16_t     soci_get_into_uint16_v   (statement_handle st, int position, int index);
SOCI_DECL int32_t      soci_get_into_int32_v    (statement_handle st, int position, int index);
SOCI_DECL uint32_t     soci_get_into_uint32_v   (statement_handle st, int position, int index);
SOCI_DECL int64_t      soci_get_into_int64_v    (statement_handle st, int position, int index);
SOCI_DECL uint64_t     soci_get_into_uint64_v   (statement_handle st, int position, int index);
SOCI_DECL double       soci_get_into_double_v   (statement_handle st, int position, int index);
SOCI_DECL char const * soci_get_into_date_v     (statement_handle st, int position, int index);


// named bind of use elements
SOCI_DECL void soci_use_string   (statement_handle st, char const * name);
SOCI_DECL void soci_use_int      (statement_handle st, char const * name);
SOCI_DECL void soci_use_long_long(statement_handle st, char const * name);
SOCI_DECL void soci_use_int8     (statement_handle st, char const * name);
SOCI_DECL void soci_use_uint8    (statement_handle st, char const * name);
SOCI_DECL void soci_use_int16    (statement_handle st, char const * name);
SOCI_DECL void soci_use_uint16   (statement_handle st, char const * name);
SOCI_DECL void soci_use_int32    (statement_handle st, char const * name);
SOCI_DECL void soci_use_uint32   (statement_handle st, char const * name);
SOCI_DECL void soci_use_int64    (statement_handle st, char const * name);
SOCI_DECL void soci_use_uint64   (statement_handle st, char const * name);
SOCI_DECL void soci_use_double   (statement_handle st, char const * name);
SOCI_DECL void soci_use_date     (statement_handle st, char const * name);
SOCI_DECL void soci_use_blob     (statement_handle st, char const * name);

// vector versions
SOCI_DECL void soci_use_string_v   (statement_handle st, char const * name);
SOCI_DECL void soci_use_int_v      (statement_handle st, char const * name);
SOCI_DECL void soci_use_long_long_v(statement_handle st, char const * name);
SOCI_DECL void soci_use_int8_v     (statement_handle st, char const * name);
SOCI_DECL void soci_use_uint8_v    (statement_handle st, char const * name);
SOCI_DECL void soci_use_int16_v    (statement_handle st, char const * name);
SOCI_DECL void soci_use_uint16_v   (statement_handle st, char const * name);
SOCI_DECL void soci_use_int32_v    (statement_handle st, char const * name);
SOCI_DECL void soci_use_uint32_v   (statement_handle st, char const * name);
SOCI_DECL void soci_use_int64_v    (statement_handle st, char const * name);
SOCI_DECL void soci_use_uint64_v   (statement_handle st, char const * name);
SOCI_DECL void soci_use_double_v   (statement_handle st, char const * name);
SOCI_DECL void soci_use_date_v     (statement_handle st, char const * name);

// named write of use elements
SOCI_DECL void soci_set_use_state    (statement_handle st, char const * name, int state);
SOCI_DECL void soci_set_use_string   (statement_handle st, char const * name, char const * val);
SOCI_DECL void soci_set_use_int      (statement_handle st, char const * name, int val);
SOCI_DECL void soci_set_use_long_long(statement_handle st, char const * name, long long val);
SOCI_DECL void soci_set_use_int8     (statement_handle st, char const * name, int8_t val);
SOCI_DECL void soci_set_use_uint8    (statement_handle st, char const * name, uint8_t val);
SOCI_DECL void soci_set_use_int18    (statement_handle st, char const * name, int16_t val);
SOCI_DECL void soci_set_use_uint18   (statement_handle st, char const * name, uint16_t val);
SOCI_DECL void soci_set_use_int32    (statement_handle st, char const * name, int32_t val);
SOCI_DECL void soci_set_use_uint32   (statement_handle st, char const * name, uint32_t val);
SOCI_DECL void soci_set_use_int64    (statement_handle st, char const * name, int64_t val);
SOCI_DECL void soci_set_use_uint64   (statement_handle st, char const * name, uint64_t val);
SOCI_DECL void soci_set_use_double   (statement_handle st, char const * name, double val);
SOCI_DECL void soci_set_use_date     (statement_handle st, char const * name, char const * val);
SOCI_DECL void soci_set_use_blob     (statement_handle st, char const * name, blob_handle blob);

// positional (re)size of vectors
SOCI_DECL int  soci_use_get_size_v(statement_handle st);
SOCI_DECL void soci_use_resize_v  (statement_handle st, int new_size);

// named write of use vectors
SOCI_DECL void soci_set_use_state_v(statement_handle st,
    char const * name, int index, int state);
SOCI_DECL void soci_set_use_string_v(statement_handle st,
    char const * name, int index, char const * val);
SOCI_DECL void soci_set_use_int_v(statement_handle st,
    char const * name, int index, int val);
SOCI_DECL void soci_set_use_long_long_v(statement_handle st,
    char const * name, int index, long long val);
SOCI_DECL void soci_set_use_int8_v(statement_handle st,
    char const * name, int index, int8_t val);
SOCI_DECL void soci_set_use_uint8_v(statement_handle st,
    char const * name, int index, uint8_t val);
SOCI_DECL void soci_set_use_int16_v(statement_handle st,
    char const * name, int index, int16_t val);
SOCI_DECL void soci_set_use_uint16_v(statement_handle st,
    char const * name, int index, uint16_t val);
SOCI_DECL void soci_set_use_int32_v(statement_handle st,
    char const * name, int index, int32_t val);
SOCI_DECL void soci_set_use_uint32_v(statement_handle st,
    char const * name, int index, uint32_t val);
SOCI_DECL void soci_set_use_int64_v(statement_handle st,
    char const * name, int index, int64_t val);
SOCI_DECL void soci_set_use_uint64_v(statement_handle st,
    char const * name, int index, uint64_t val);
SOCI_DECL void soci_set_use_double_v(statement_handle st,
    char const * name, int index, double val);
SOCI_DECL void soci_set_use_date_v(statement_handle st,
    char const * name, int index, char const * val);


// named read of use elements (for modifiable use values)
SOCI_DECL int          soci_get_use_state    (statement_handle st, char const * name);
SOCI_DECL char const * soci_get_use_string   (statement_handle st, char const * name);
SOCI_DECL int          soci_get_use_int      (statement_handle st, char const * name);
SOCI_DECL long long    soci_get_use_long_long(statement_handle st, char const * name);
SOCI_DECL int8_t       soci_get_use_int8     (statement_handle st, char const * name);
SOCI_DECL uint8_t      soci_get_use_uint8    (statement_handle st, char const * name);
SOCI_DECL int16_t      soci_get_use_int16    (statement_handle st, char const * name);
SOCI_DECL uint16_t     soci_get_use_uint16   (statement_handle st, char const * name);
SOCI_DECL int32_t      soci_get_use_int32    (statement_handle st, char const * name);
SOCI_DECL uint32_t     soci_get_use_uint32   (statement_handle st, char const * name);
SOCI_DECL int64_t      soci_get_use_int64    (statement_handle st, char const * name);
SOCI_DECL uint64_t     soci_get_use_uint64   (statement_handle st, char const * name);
SOCI_DECL double       soci_get_use_double   (statement_handle st, char const * name);
SOCI_DECL char const * soci_get_use_date     (statement_handle st, char const * name);
SOCI_DECL blob_handle  soci_get_use_blob     (statement_handle st, char const * name);


// statement preparation and execution
SOCI_DECL void      soci_prepare(statement_handle st, char const * query);
SOCI_DECL int       soci_execute(statement_handle st, int withDataExchange);
SOCI_DECL long long soci_get_affected_rows(statement_handle st);
SOCI_DECL int       soci_fetch(statement_handle st);
SOCI_DECL int       soci_got_data(statement_handle st);

SOCI_DECL int soci_statement_state(statement_handle s);
SOCI_DECL char const * soci_statement_error_message(statement_handle s);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // SOCI_SIMPLE_H_INCLUDED
