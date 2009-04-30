dnl $Id: ax_odbc.m4,v 1.1 2008/05/12 13:16:58 mloskot Exp $
dnl
dnl @synopsis AX_LIB_ODBC([MINIMUM-VERSION])
dnl 
dnl Test for the ODBC library of a particular version (or newer)
dnl
dnl This macro takes only one optional argument, required version
dnl of ODBC implementation, for instance 0x0351.
dnl If required version is not passed, 0x0300 is used.
dnl
dnl If no intallation prefix to the installed ODBC library is given
dnl the macro searches under /usr, /usr/local, and /opt.
dnl
dnl This macro calls:
dnl
dnl   AC_SUBST(ODBC_CFLAGS)
dnl   AC_SUBST(ODBC_LDFLAGS)
dnl   AC_SUBST(ODBC_VERSION)
dnl
dnl And sets:
dnl
dnl   HAVE_ODBC
dnl
dnl @category InstalledPackages
dnl @category Cxx
dnl @author Mateusz Loskot <mateusz@loskot.net>
dnl @version $Date: 2008/05/12 13:16:58 $
dnl @license AllPermissive
dnl
AC_DEFUN([AX_LIB_ODBC],
[
    AC_ARG_WITH([odbc],
        AC_HELP_STRING(
            [--with-odbc=@<:@ARG@:>@],
            [use ODBC library @<:@default=yes@:>@, optionally specify ODBC installation prefix]
        ),
        [
        if test "$withval" = "no"; then
            WANT_ODBC="no"
        elif test "$withval" = "yes"; then
            WANT_ODBC="yes"
            ac_odbc_path=""
        else
            WANT_ODBC="yes"
            ac_odbc_path="$withval"
        fi
        ],
        [WANT_ODBC="yes"]
    )

    ODBC_CFLAGS=""
    ODBC_LDFLAGS=""
    ODBC_VERSION=""

    if test "x$WANT_ODBC" = "xyes"; then

        ac_odbc_header="sql.h"
        
        odbc_version_req=ifelse([$1], [], [0x0300], [$1])

        AC_MSG_CHECKING([for ODBC implementation >= $odbc_version_req])

        if test "$ac_odbc_path" != ""; then
            ac_odbc_ldflags="-L$ac_odbc_path/lib"
            ac_odbc_cppflags="-I$ac_odbc_path/include"
        else
            for ac_odbc_path_tmp in /usr /usr/local /opt ; do
                if test -f "$ac_odbc_path_tmp/include/$ac_odbc_header" \
                    && test -r "$ac_odbc_path_tmp/include/$ac_odbc_header"; then
                    ac_odbc_path=$ac_odbc_path_tmp
                    ac_odbc_ldflags="-I$ac_odbc_path_tmp/include"
                    ac_odbc_cppflags="-L$ac_odbc_path_tmp/lib"
                    break;
                fi
            done
        fi

        ac_odbc_ldflags="$ac_odbc_ldflags -lodbc"

        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS $ac_odbc_cppflags"

        AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE(
            [
            AC_LANG_PROGRAM([[@%:@include <sql.h>]],
                [[
#if (ODBCVER >= $odbc_version_req)
// Everything is okay
#else
#  error ODBC version is too old
#endif
                ]]
            )
            ],
            [
            AC_MSG_RESULT([yes])
            success="yes"
            ],
            [
            AC_MSG_RESULT([not found])
            succees="no"
            ]
        )
        AC_LANG_POP([C++])

        CPPFLAGS="$saved_CPPFLAGS"
        
        if test "$success" = "yes"; then
            
            ODBC_CFLAGS="$ac_odbc_cppflags"
            ODBC_LDFLAGS="$ac_odbc_ldflags"

            ac_odbc_header_path="$ac_odbc_path/include/$ac_odbc_header"

            dnl Retrieve ODBC release version
            if test "x$ac_odbc_header_path" != "x"; then
                ac_odbc_version=`cat $ac_odbc_header_path \
                    | grep '#define.*ODBCVER.*' | sed -e 's/.*#define ODBCVER.//'`
                if test $ac_odbc_version != ""; then
                    ODBC_VERSION=$ac_odbc_version
                else
                    AC_MSG_WARN([Can not find ODBCVER macro in sql.h header to retrieve ODBC version!])

                fi
            fi

            AC_SUBST([ODBC_CFLAGS])
            AC_SUBST([ODBC_LDFLAGS])
            AC_SUBST([ODBC_VERSION])
            AC_DEFINE([HAVE_ODBC], [1], [Define to 1 if ODBC library is available])
        fi
    fi
])

