//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//
//

#ifndef SOCI_FIREBIRD_H_INCLUDED
#define SOCI_FIREBIRD_H_INCLUDED

#include <soci/soci-platform.h>

#ifdef SOCI_FIREBIRD_SOURCE
# define SOCI_FIREBIRD_DECL SOCI_DECL_EXPORT
#else
# define SOCI_FIREBIRD_DECL SOCI_DECL_IMPORT
#endif

#ifdef _WIN32
#include <ciso646> // To understand and/or/not on MSVC9
#endif
#include <soci/soci-backend.h>
#include <ibase.h> // FireBird
#include <cstdlib>
#include <vector>
#include <string>
#include <cstdint>

namespace soci
{

class SOCI_FIREBIRD_DECL firebird_soci_error : public soci_error
{
public:
    firebird_soci_error(std::string const & msg,
        ISC_STATUS const * status = 0);

    error_category get_error_category() const override;

    std::string get_backend_name() const override { return "firebird"; }
    int get_backend_error_code() const override;

    std::vector<ISC_STATUS> status_;
};

enum BuffersType
{
    eStandard, eVector
};

struct firebird_blob_backend;
struct firebird_statement_backend;
struct firebird_standard_into_type_backend : details::standard_into_type_backend
{
    firebird_standard_into_type_backend(firebird_statement_backend &st)
        : statement_(st), data_(NULL), type_(), position_(0), buf_(NULL), indISCHolder_(0)
    {}

    void define_by_pos(int &position,
        void *data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, bool calledFromFetch,
        indicator *ind) override;

    void clean_up() override;

    firebird_statement_backend &statement_;
    virtual void exchangeData();

    void *data_;
    details::exchange_type type_;
    int position_;

    char *buf_;
    short indISCHolder_;
};

struct firebird_vector_into_type_backend : details::vector_into_type_backend
{
    firebird_vector_into_type_backend(firebird_statement_backend &st)
        : statement_(st), data_(NULL), type_(), position_(0), buf_(NULL), indISCHolder_(0)
    {}

    void define_by_pos(int &position,
        void *data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, indicator *ind) override;

    void resize(std::size_t sz) override;
    std::size_t size() const override;

    void clean_up() override;

    firebird_statement_backend &statement_;
    virtual void exchangeData(std::size_t row);

    void *data_;
    details::exchange_type type_;
    int position_;

    char *buf_;
    short indISCHolder_;
};

struct firebird_standard_use_type_backend : details::standard_use_type_backend
{
    firebird_standard_use_type_backend(firebird_statement_backend &st)
        : statement_(st), data_(NULL), type_(), position_(0), buf_(NULL), indISCHolder_(0),
          blob_(NULL)
    {}

    void bind_by_pos(int &position,
        void *data, details::exchange_type type, bool readOnly) override;
    void bind_by_name(std::string const &name,
        void *data, details::exchange_type type, bool readOnly) override;

    void pre_use(indicator const *ind) override;
    void post_use(bool gotData, indicator *ind) override;

    void clean_up() override;

    firebird_statement_backend &statement_;
    virtual void exchangeData();

    void *data_;
    details::exchange_type type_;
    int position_;

    char *buf_;
    short indISCHolder_;

private:
    // Allocate a temporary blob, fill it with the data from the provided
    // string and copy its ID into buf_.
    void copy_to_blob(const std::string& in);

    // This is used for types mapping to CLOB.
    firebird_blob_backend* blob_;
};

struct firebird_vector_use_type_backend : details::vector_use_type_backend
{
    firebird_vector_use_type_backend(firebird_statement_backend &st)
        : statement_(st), data_(NULL), type_(), position_(0), buf_(NULL), indISCHolder_(0),
          blob_(NULL)
    {}

    void bind_by_pos(int &position,
        void *data, details::exchange_type type) override;
    void bind_by_name(std::string const &name,
        void *data, details::exchange_type type) override;

    void pre_use(indicator const *ind) override;

    std::size_t size() const override;

    void clean_up() override;

    firebird_statement_backend &statement_;
    virtual void exchangeData(std::size_t row);

    void *data_;
    details::exchange_type type_;
    int position_;
    indicator const *inds_;

    char *buf_;
    short indISCHolder_;

private:
    // Allocate a temporary blob, fill it with the data from the provided
    // string and copy its ID into buf_.
    void copy_to_blob(const std::string &in);

    // This is used for types mapping to CLOB.
    firebird_blob_backend *blob_;
};

struct firebird_session_backend;
struct SOCI_FIREBIRD_DECL firebird_statement_backend : details::statement_backend
{
    firebird_statement_backend(firebird_session_backend &session);

    void alloc() override;
    void clean_up() override;
    void prepare(std::string const &query,
        details::statement_type eType) override;

    exec_fetch_result execute(int number) override;
    exec_fetch_result fetch(int number) override;

    long long get_affected_rows() override;
    int get_number_of_rows() override;
    std::string get_parameter_name(int index) const override;
    int get_row_to_dump() const override { return current_row_; }

    std::string rewrite_for_procedure_call(std::string const &query) override;

    int prepare_for_describe() override;
    void describe_column(int colNum,
        db_type &dbtype,
        std::string &columnName) override;

    firebird_standard_into_type_backend * make_into_type_backend() override;
    firebird_standard_use_type_backend * make_use_type_backend() override;
    firebird_vector_into_type_backend * make_vector_into_type_backend() override;
    firebird_vector_use_type_backend * make_vector_use_type_backend() override;

    firebird_session_backend &session_;

    isc_stmt_handle stmtp_;
    XSQLDA * sqldap_;
    XSQLDA * sqlda2p_;

    bool boundByName_;
    bool boundByPos_;

    friend struct firebird_vector_into_type_backend;
    friend struct firebird_standard_into_type_backend;
    friend struct firebird_vector_use_type_backend;
    friend struct firebird_standard_use_type_backend;

protected:
    int rowsFetched_;
    bool endOfRowSet_;

    long long rowsAffectedBulk_; // number of rows affected by the last bulk operation

    // Return the number of rows affected by last operation.
    long long get_last_row_count();

    virtual void exchangeData(bool gotData, int row);
    virtual void prepareSQLDA(XSQLDA ** sqldap, short size = 10);
    virtual void rewriteQuery(std::string const & query,
        std::vector<char> & buffer);
    virtual void rewriteParameters(std::string const & src,
        std::vector<char> & dst);

    BuffersType intoType_;
    BuffersType useType_;

    std::vector<std::vector<indicator> > inds_;
    std::vector<void*> intos_;
    std::vector<void*> uses_;

    // named parameters
    std::map <std::string, int> names_;

    bool procedure_;

private:
    // Used during bulk operations to keep track of the row which potentially
    // resulted in an error.
    int current_row_ = -1;
};

struct SOCI_FIREBIRD_DECL firebird_blob_backend : details::blob_backend
{
    firebird_blob_backend(firebird_session_backend &session);

    ~firebird_blob_backend() override;

    std::size_t get_len() override;

    std::size_t read_from_start(void * buf, std::size_t toRead, std::size_t offset = 0) override;

    std::size_t write_from_start(const void * buf, std::size_t toWrite, std::size_t offset = 0) override;

    std::size_t append(const void *buf, std::size_t toWrite) override;
    void trim(std::size_t newLen) override;

    details::session_backend &get_session_backend() override;

    // Writes the current data into the database by allocating a new BLOB
    // object for it.
    //
    // Returns The ID of the newly created BLOB object
    ISC_QUAD save_to_db();
    void assign(ISC_QUAD const & bid);

private:
    void open();
    long getBLOBInfo();
    void load();
    void writeBuffer(std::size_t offset, void const * buf,
        std::size_t toWrite);
    void closeBlob();

    firebird_session_backend &session_;
    ISC_QUAD blob_id_;
    // BLOB id was fetched from database (true)
    // or this is new BLOB
    bool from_db_;
    isc_blob_handle blob_handle_;
    // buffer for BLOB data
    std::vector<std::uint8_t> data_;
    bool loaded_;
    long max_seg_size_;
};

struct SOCI_FIREBIRD_DECL firebird_session_backend : details::session_backend
{
    firebird_session_backend(connection_parameters const & parameters);

    ~firebird_session_backend() override;

    bool is_connected() override;

    void begin() override;
    void commit() override;
    void rollback() override;

    bool get_next_sequence_value(session & s,
        std::string const & sequence, long long & value) override;

    std::string get_dummy_from_table() const override { return "rdb$database"; }

    std::string get_backend_name() const override { return "firebird"; }

    void cleanUp();

    firebird_statement_backend * make_statement_backend() override;
    details::rowid_backend* make_rowid_backend() override;
    firebird_blob_backend * make_blob_backend() override;

    bool get_option_decimals_as_strings() { return decimals_as_strings_; }

    // Returns the pointer to the current transaction handle, starting a new
    // transaction if necessary.
    //
    // The returned pointer should
    isc_tr_handle* current_transaction();

    isc_db_handle dbhp_;

private:
    isc_tr_handle trhp_;
    bool decimals_as_strings_;
};

struct firebird_backend_factory : backend_factory
{
    firebird_backend_factory() {}
    firebird_session_backend * make_session(
        connection_parameters const & parameters) const override;
};

extern SOCI_FIREBIRD_DECL firebird_backend_factory const firebird;

extern "C"
{

// for dynamic backend loading
SOCI_FIREBIRD_DECL backend_factory const * factory_firebird();
SOCI_FIREBIRD_DECL void register_factory_firebird();

} // extern "C"

} // namespace soci

#endif // SOCI_FIREBIRD_H_INCLUDED
