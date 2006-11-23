dnl $Id: ax_oracle_oci.m4,v 1.3 2006/11/23 03:09:28 mloskot Exp $
dnl
dnl @synopsis AX_LIB_ORACLE_OCI([MINIMUM-VERSION])
dnl
dnl This macro provides tests of availability of Oracle OCI API
dnl of particular version or newer.
dnl This macros checks for Oracle OCI headers and libraries 
dnl and defines compilation flags
dnl 
dnl Macro supports following options and their values:
dnl 1) Single-option usage:
dnl --with-oracle - path to ORACLE_HOME directory
dnl 2) Two-options usage (both options are required):
dnl --with-oracle-include - path to directory with OCI headers
dnl --with-oracle-lib - path to directory with OCI libraries 
dnl
dnl NOTE: These options described above does not take yes|no values.
dnl If 'yes' value is passed, then WARNING message will be displayed,
dnl 'no' value, as well as the --without-oracle* variations will cause
dnl the macro won't check enything.
dnl
dnl This macro calls:
dnl
dnl   AC_SUBST(ORACLE_OCI_CFLAGS)
dnl   AC_SUBST(ORACLE_OCI_LDFLAGS)
dnl   AC_SUBST(ORACLE_OCI_VERSION)
dnl
dnl And sets:
dnl
dnl   HAVE_ORACLE
dnl
dnl @category InstalledPackages
dnl @category Cxx
dnl @author Mateusz Loskot <mateusz@loskot.net>
dnl @version $Date: 2006/11/23 03:09:28 $
dnl @license AllPermissive
dnl
dnl $Id: ax_oracle_oci.m4,v 1.3 2006/11/23 03:09:28 mloskot Exp $
dnl
AC_DEFUN([AX_LIB_ORACLE_OCI],
[
    AC_ARG_WITH([oracle],
        AC_HELP_STRING([--with-oracle=@<:@DIR@:>@],
            [use Oracle OCI API from given path to Oracle home directory]
        ),
        [oracle_home_dir="$withval"],
        [oracle_home_dir=""]
    )

    AC_ARG_WITH([oracle-include],
        AC_HELP_STRING([--with-oracle-include=@<:@DIR@:>@],
            [use Oracle OCI API headers from given path]
        ),
        [oracle_home_include_dir="$withval"],
        [oracle_home_include_dir=""]
    )
    AC_ARG_WITH([oracle-lib],
        AC_HELP_STRING([--with-oracle-lib=@<:@DIR@:>@],
            [use Oracle OCI API libraries from given path]
        ),
        [oracle_home_lib_dir="$withval"],
        [oracle_home_lib_dir=""]
    )

    ORACLE_OCI_CFLAGS=""
    ORACLE_OCI_LDFLAGS=""
    ORACLE_OCI_VERSION=""

    dnl
    dnl Collect include/lib paths
    dnl 
    want_oracle_but_no_path="no"

    if test -n "$oracle_home_dir"; then

        if test "$oracle_home_dir" != "no" -a "$oracle_home_dir" != "yes"; then
            dnl ORACLE_HOME path provided
            oracle_include_dir="$oracle_home_dir/rdbms/public"
            oracle_lib_dir="$oracle_home_dir/lib"
        elif test "$oracle_home_dir" = "yes"; then
            want_oracle_but_no_path="yes"
        fi

    elif test -n "$oracle_home_include_dir" -o -n "$oracle_home_lib_dir"; then

        if test "$oracle_home_include_dir" != "no" -a "$oracle_home_include_dir" != "yes"; then
            oracle_include_dir="$oracle_home_include_dir"
        elif test "$oracle_home_include_dir" = "yes"; then
            want_oracle_but_no_path="yes"
        fi

        if test "$oracle_home_lib_dir" != "no" -a "$oracle_home_lib_dir" != "yes"; then
            oracle_lib_dir="$oracle_home_lib_dir"
        elif test "$oracle_home_lib_dir" = "yes"; then
            want_oracle_but_no_path="yes"
        fi
    fi

    if test "$want_oracle_but_no_path" = "yes"; then
        AC_MSG_WARN([Oracle support is requested but no Oracle paths have been provided. \
Please, locate Oracle directories using --with-oracle or \
--with-oracle-include and --with-oracle-lib options.])
    fi

    dnl
    dnl Check OCI files
    dnl
    if test -n "$oracle_include_dir" -a -n "$oracle_lib_dir"; then

        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS -I$oracle_include_dir"

        saved_LDFLAGS="$LDFLAGS"
        oci_ldflags="-L$oracle_lib_dir -lclntsh -lnnz10"
        LDFLAGS="$LDFLAGS $oci_ldflags"

        dnl
        dnl Check OCI headers
        dnl
        AC_MSG_CHECKING([for Oracle OCI headers in $oracle_include_dir])

        AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([[@%:@include <oci.h>]],
                [[
#if defined(OCI_MAJOR_VERSION)
#if OCI_MAJOR_VERSION == 10 && OCI_MINOR_VERSION == 2
// Oracle 10.2 detected
#endif
#elif defined(OCI_V7_SYNTAX)
// OK, older Oracle detected
// TODO - mloskot: find better macro to check for older versions; 
#else
#  error Oracle oci.h header not found
#endif
                ]]
            )],
            [
            ORACLE_OCI_CFLAGS="-I$oracle_include_dir"
            oci_header_found="yes"
            AC_MSG_RESULT([yes])
            ],
            [
            oci_header_found="no"
            AC_MSG_RESULT([not found])
            ]
        )
        AC_LANG_POP([C++])
        
        dnl
        dnl Check OCI libraries
        dnl
        if test "$oci_header_found" = "yes"; then

            AC_MSG_CHECKING([for Oracle OCI libraries in $oracle_lib_dir])

            AC_LANG_PUSH(C++)
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM([[@%:@include <oci.h>]],
                    [[
OCIEnv* envh = 0;
OCIEnvCreate(&envh, OCI_DEFAULT, 0, 0, 0, 0, 0, 0);
if (envh) OCIHandleFree(envh, OCI_HTYPE_ENV);
                    ]]
                )],
                [
                ORACLE_OCI_LDFLAGS="$oci_ldflags"
                oci_lib_found="yes"
                AC_MSG_RESULT([yes])
                ],
                [
                oci_lib_found="no"
                AC_MSG_RESULT([not found])
                ]
            )
            AC_LANG_POP([C++])
        fi

        CPPFLAGS="$saved_CPPFLAGS"
        LDFLAGS="$saved_LDFLAGS"
    fi

    dnl
    dnl Check required version of Oracle is available
    dnl
    oracle_version_major=`cat $oracle_include_dir/oci.h \
                         | grep '#define.*OCI_MAJOR_VERSION.*' \
                         | sed -e 's/#define OCI_MAJOR_VERSION  *//' \
                         | sed -e 's/  *\/\*.*\*\///'`

    oracle_version_minor=`cat $oracle_include_dir/oci.h \
                         | grep '#define.*OCI_MINOR_VERSION.*' \
                         | sed -e 's/#define OCI_MINOR_VERSION  *//' \
                         | sed -e 's/  *\/\*.*\*\///'`

    ORACLE_OCI_VERSION="$oracle_version_major.$oracle_version_minor"

    oracle_version_req=ifelse([$1], [], [], [$1])

    if test "$oci_header_found" = "yes" -a \
            "$oci_lib_found" = "yes" -a \
            -n "$oracle_version_req"; then

        AC_MSG_CHECKING([if Oracle OCI version is >= $oracle_version_req])

        dnl Decompose required version string of Oracle
        dnl and calculate its number representation
        oracle_version_req_major=`expr $oracle_version_req : '\([[0-9]]*\)'`
        oracle_version_req_minor=`expr $oracle_version_req : '[[0-9]]*\.\([[0-9]]*\)'`

        oracle_version_req_number=`expr $oracle_version_req_major \* 1000000 \
                                   \+ $oracle_version_req_minor \* 1000`

        dnl Calculate its number representation 
        oracle_version_number=`expr $oracle_version_major \* 1000000 \
                              \+ $oracle_version_minor \* 1000`

        oracle_version_check=`expr $oracle_version_number \>\= $oracle_version_req_number`
        if test "$oracle_version_check" = "1"; then
            AC_MSG_RESULT([yes])
        else
            AC_MSG_RESULT([no])
            AC_MSG_ERROR([Oracle $ORACLE_OCI_VERSION found, but required version is $oracle_version_req])
        fi

    fi

    AC_SUBST([ORACLE_OCI_VERSION])
    AC_SUBST([ORACLE_OCI_CFLAGS])
    AC_SUBST([ORACLE_OCI_LDFLAGS])
])
