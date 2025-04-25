//
// Copyright (C) 2004-2016 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ORACLE_H_INCLUDED
#define SOCI_ORACLE_H_INCLUDED

#include <soci/soci-platform.h>

#ifdef SOCI_ORACLE_SOURCE
# define SOCI_ORACLE_DECL SOCI_DECL_EXPORT
#else
# define SOCI_ORACLE_DECL SOCI_DECL_IMPORT
#endif

#include <soci/soci-backend.h>
#include <oci.h> // OCI
#include <sstream>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable:4512 4511)
#endif


namespace soci
{

class SOCI_ORACLE_DECL oracle_soci_error : public soci_error
{
public:
    oracle_soci_error(std::string const & msg, int errNum = 0);

    error_category get_error_category() const override;

    std::string get_backend_name() const override { return "oracle"; }
    int get_backend_error_code() const override { return err_num_; }

    // This member variable is only public for compatibility, don't use it
    // directly, call get_backend_error_code() instead.
    int err_num_;
};


struct oracle_statement_backend;
struct oracle_standard_into_type_backend : details::standard_into_type_backend
{
    oracle_standard_into_type_backend(oracle_statement_backend &st)
        : statement_(st), defnp_(NULL), indOCIHolder_(0),
          data_(NULL), buf_(NULL) {}

    void define_by_pos(int &position,
        void *data, details::exchange_type type) override;

    void pre_exec(int num) override;
    void pre_fetch() override;
    void post_fetch(bool gotData, bool calledFromFetch,
        indicator *ind) override;

    void clean_up() override;

    oracle_statement_backend &statement_;

    OCIDefine *defnp_;
    sb2 indOCIHolder_;
    void *data_;
    void *ociData_ = NULL;
    char *buf_;        // generic buffer
    details::exchange_type type_;

    ub2 rCode_;
};

struct oracle_vector_into_type_backend : details::vector_into_type_backend
{
    oracle_vector_into_type_backend(oracle_statement_backend &st)
        : statement_(st), defnp_(NULL),
        data_(NULL), buf_(NULL), user_ranges_(true) {}

    void define_by_pos(int &position,
        void *data, details::exchange_type type) override
    {
        user_ranges_ = false;
        define_by_pos_bulk(position, data, type, 0, &end_var_);
    }

    void define_by_pos_bulk(
        int & position, void * data, details::exchange_type type,
        std::size_t begin, std::size_t * end) override;

    void pre_exec(int num) override;
    void pre_fetch() override;
    void post_fetch(bool gotData, indicator *ind) override;

    void resize(std::size_t sz) override;
    std::size_t size() const override;
    std::size_t full_size() const;

    void clean_up() override;

    // helper function for preparing indicators and sizes_ vectors
    // (as part of the define_by_pos)
    void prepare_indicators(std::size_t size);

    oracle_statement_backend &statement_;

    OCIDefine *defnp_;
    std::vector<sb2> indOCIHolderVec_;
    void *data_;
    char *buf_;              // generic buffer
    details::exchange_type type_;
    std::size_t begin_;
    std::size_t * end_;
    std::size_t end_var_;
    bool user_ranges_;
    std::size_t colSize_;    // size of the string column (used for strings)
    std::vector<ub2> sizes_; // sizes of data fetched (used for strings)

    std::vector<ub2> rCodes_;
};

struct oracle_standard_use_type_backend : details::standard_use_type_backend
{
    oracle_standard_use_type_backend(oracle_statement_backend &st)
        : statement_(st), bindp_(NULL), indOCIHolder_(0),
          data_(NULL), buf_(NULL) {}

    void bind_by_pos(int &position,
        void *data, details::exchange_type type, bool readOnly) override;
    void bind_by_name(std::string const &name,
        void *data, details::exchange_type type, bool readOnly) override;

    // common part for bind_by_pos and bind_by_name
    void prepare_for_bind(void *&data, sb4 &size, ub2 &oracleType, bool readOnly);

    void pre_exec(int num) override;
    void pre_use(indicator const *ind) override;
    void post_use(bool gotData, indicator *ind) override;

    void clean_up() override;

    oracle_statement_backend &statement_;

    OCIBind *bindp_;
    sb2 indOCIHolder_;
    void *data_;
    void *ociData_ = NULL;
    bool readOnly_;
    char *buf_;        // generic buffer
    details::exchange_type type_;
};

struct oracle_vector_use_type_backend : details::vector_use_type_backend
{
    oracle_vector_use_type_backend(oracle_statement_backend &st)
        : statement_(st), bindp_(NULL),
          data_(NULL), buf_(NULL), bind_position_(0) {}

    void bind_by_pos(int & position,
        void * data, details::exchange_type type) override
    {
        bind_by_pos_bulk(position, data, type, 0, &end_var_);
    }

    void bind_by_pos_bulk(int & position,
        void * data, details::exchange_type type,
        std::size_t begin, std::size_t * end) override;

    void bind_by_name(const std::string & name,
        void * data, details::exchange_type type) override
    {
        bind_by_name_bulk(name, data, type, 0, &end_var_);
    }

    void bind_by_name_bulk(std::string const &name,
        void *data, details::exchange_type type,
        std::size_t begin, std::size_t * end) override;

    // pre_use() helper
    void prepare_for_bind(void *&data, sb4 &size, ub2 &oracleType);

    // helper function for preparing indicators and sizes_ vectors
    // (as part of the bind_by_pos and bind_by_name)
    void prepare_indicators(std::size_t size);

    void pre_use(indicator const *ind) override;

    std::size_t size() const override; // active size (might be lower than full vector size)
    std::size_t full_size() const;    // actual size of the user-provided vector

    void clean_up() override;

    oracle_statement_backend &statement_;

    OCIBind *bindp_;
    std::vector<sb2> indOCIHolderVec_;
    void *data_;
    char *buf_;        // generic buffer
    details::exchange_type type_;
    std::size_t begin_;
    std::size_t * end_;
    std::size_t end_var_;

    // used for strings only
    std::vector<ub2> sizes_;
    std::size_t maxSize_;

    // name is used if non-empty, otherwise position
    std::string bind_name_;
    int bind_position_;
};

struct oracle_session_backend;
struct SOCI_ORACLE_DECL oracle_statement_backend : details::statement_backend
{
    oracle_statement_backend(oracle_session_backend &session);

    void alloc() override;
    void clean_up() override;
    void prepare(std::string const &query,
        details::statement_type eType) override;

    exec_fetch_result execute(int number) override;
    exec_fetch_result fetch(int number) override;

    long long get_affected_rows() override;
    int get_number_of_rows() override;
    std::string get_parameter_name(int index) const override;
    int get_row_to_dump() const override { return error_row_; }

    std::string rewrite_for_procedure_call(std::string const &query) override;

    int prepare_for_describe() override;
    void describe_column(int colNum,
        db_type &dbtype,
        std::string &columnName) override;

    // helper for defining into vector<string>
    std::size_t column_size(int position);

    oracle_standard_into_type_backend * make_into_type_backend() override;
    oracle_standard_use_type_backend * make_use_type_backend() override;
    oracle_vector_into_type_backend * make_vector_into_type_backend() override;
    oracle_vector_use_type_backend * make_vector_use_type_backend() override;

    oracle_session_backend &session_;

    OCIStmt *stmtp_;

    bool boundByName_;
    bool boundByPos_;
    bool noData_;

private:
    // Wrapper for OCIAttrGet(), throws on error.
    template <typename T>
    T get_statement_attr(int attr) const;

    // First row with the error for bulk operations or -1.
    int error_row_ = -1;
};

struct SOCI_ORACLE_DECL oracle_rowid_backend : details::rowid_backend
{
    oracle_rowid_backend(oracle_session_backend &session);

    ~oracle_rowid_backend() override;

    OCIRowid *rowidp_;
};

struct SOCI_ORACLE_DECL oracle_blob_backend : details::blob_backend
{
    typedef OCILobLocator * locator_t;

    oracle_blob_backend(oracle_session_backend &session);

    ~oracle_blob_backend() override;

    std::size_t get_len() override;

    std::size_t read_from_start(void * buf, std::size_t toRead, std::size_t offset = 0) override;

    std::size_t write_from_start(const void * buf, std::size_t toWrite, std::size_t offset = 0) override;

    std::size_t append(const void *buf, std::size_t toWrite) override;

    void trim(std::size_t newLen) override;

    locator_t get_lob_locator() const;

    void set_lob_locator(const locator_t locator, bool initialized = true);

    void reset();

    void ensure_initialized();

    details::session_backend &get_session_backend() override;

private:
    std::size_t do_deprecated_read(std::size_t offset, void *buf, std::size_t toRead) override
    {
        // Offsets are 1-based in Oracle
        return read_from_start(buf, toRead, offset - 1);
    }

    std::size_t do_deprecated_write(std::size_t offset, const void *buf, std::size_t toWrite) override
    {
        // Offsets are 1-based in Oracle
        return write_from_start(buf, toWrite, offset - 1);
    }

    oracle_session_backend &session_;

    locator_t lobp_;

    // If this is true, then the locator lobp_ points to something useful
    // (instead of being the equivalent to a pointer with random value)
    bool initialized_;
};

struct SOCI_ORACLE_DECL oracle_session_backend : details::session_backend
{
    oracle_session_backend(std::string const & serviceName,
        std::string const & userName,
        std::string const & password,
        int mode,
        bool decimals_as_strings = false,
        int charset = 0,
        int ncharset = 0);

    ~oracle_session_backend() override;

    bool is_connected() override;

    void begin() override;
    void commit() override;
    void rollback() override;

    std::string get_table_names_query() const override
    {
        return "select table_name"
            " from user_tables";
    }

    std::string get_column_descriptions_query() const override
    {
        return "select column_name,"
            " data_type,"
            " char_length as character_maximum_length,"
            " data_precision as numeric_precision,"
            " data_scale as numeric_scale,"
            " decode(nullable, 'Y', 'YES', 'N', 'NO') as is_nullable"
            " from user_tab_columns"
            " where table_name = :t";
    }

    std::string create_column_type(db_type dt,
        int precision, int scale) override
    {
        //  Oracle-specific SQL syntax:

        std::string res;
        switch (dt)
        {
        case db_string:
            {
                std::ostringstream oss;

                if (precision == 0)
                {
                    oss << "clob";
                }
                else
                {
                    oss << "varchar(" << precision << ")";
                }

                res += oss.str();
            }
            break;

        case db_date:
            res += "timestamp";
            break;

        case db_double:
            {
                std::ostringstream oss;
                if (precision == 0)
                {
                    oss << "number";
                }
                else
                {
                    oss << "number(" << precision << ", " << scale << ")";
                }

                res += oss.str();
            }
            break;

        case db_int16:
            res += "smallint";
            break;

        case db_int32:
            res += "integer";
            break;

        case db_int64:
            res += "number";
            break;

        case db_uint64:
            res += "number";
            break;

        case db_blob:
            res += "blob";
            break;

        case db_xml:
            res += "xmltype";
            break;

        default:
            throw soci_error("this db_type is not supported in create_column");
        }

        return res;
    }
    std::string add_column(const std::string & tableName,
        const std::string & columnName, db_type dt,
        int precision, int scale) override
    {
        return "alter table " + tableName + " add " +
            columnName + " " + create_column_type(dt, precision, scale);
    }
    std::string alter_column(const std::string & tableName,
        const std::string & columnName, db_type dt,
        int precision, int scale) override
    {
        return "alter table " + tableName + " modify " +
            columnName + " " + create_column_type(dt, precision, scale);
    }
    std::string empty_blob() override
    {
        return "empty_blob()";
    }
    std::string nvl() override
    {
        return "nvl";
    }

    bool get_next_sequence_value(session &s,
         std::string const &sequence, long long &value) override;

    std::string get_dummy_from_table() const override { return "dual"; }

    std::string get_backend_name() const override { return "oracle"; }

    void clean_up();

    oracle_statement_backend * make_statement_backend() override;
    oracle_rowid_backend * make_rowid_backend() override;
    oracle_blob_backend * make_blob_backend() override;

    bool get_option_decimals_as_strings() { return decimals_as_strings_; }

    // Return either SQLT_FLT or SQLT_BDOUBLE as the type to use when binding
    // values of C type "double" (the latter is preferable but might not be
    // always available).
    ub2 get_double_sql_type() const;

    OCIEnv *envhp_;
    OCIServer *srvhp_;
    OCIError *errhp_;
    OCISvcCtx *svchp_;
    OCISession *usrhp_;
    bool decimals_as_strings_;
};

struct oracle_backend_factory : backend_factory
{
      oracle_backend_factory() {}
    oracle_session_backend * make_session(
        connection_parameters const & parameters) const override;
};

extern SOCI_ORACLE_DECL oracle_backend_factory const oracle;

extern "C"
{

// for dynamic backend loading
SOCI_ORACLE_DECL backend_factory const * factory_oracle();
SOCI_ORACLE_DECL void register_factory_oracle();

} // extern "C"

} // namespace soci

#endif
