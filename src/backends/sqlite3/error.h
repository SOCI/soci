#ifndef SQLITE_SQLITE3_ERROR_H_INCLUDED
#define SQLITE_SQLITE3_ERROR_H_INCLUDED

#include <string>
#include <soci-backend.h>

namespace soci
{

class sqlite3_soci_error : public soci_error
{
public:
    sqlite3_soci_error(std::string const & msg, const int errNum = 0)
        : soci_error(msg), err_num_(errNum) { }

    int err_num() const { return err_num_; }

private:
    int err_num_;
};

} // namespace soci

#endif /* SQLITE_SQLITE3_ERROR_H_INCLUDED */
