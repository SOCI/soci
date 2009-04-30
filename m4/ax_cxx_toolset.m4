dnl $Id: ax_cxx_toolset.m4,v 1.2 2006/08/30 14:28:55 mloskot Exp $
dnl
dnl @synopsis AX_CXX_TOOLSET([])
dnl
dnl The AC_CXX_TOOLSET_INFO macro runs simple tests to detect C++ compiler.
dnl It sets four variables with compiler vendor name, abbreviation,
dnl toolset (based on toolset names used in Boost library)
dnl and compiler command.
dnl
dnl Currently, only GNU C/C++ and Microsoft Visual C++ are supported.
dnl
dnl This macro does not take any parameters.
dnl
dnl This macro sets following variables:
dnl
dnl CXX_TOOLSET_VENDOR
dnl CXX_TOOLSET_FULLNAME
dnl CXX_TOOLSET_NAME
dnl CXX_TOOLSET_COMMAND
dnl
dnl No AC_SUBST are called.
dnl
dnl The idea of this macro is based on AX_COMPILER_VENDOR macro
dnl developed and copyrighted by Ludovic Courtès <ludo@chbouib.org>
dnl Source: http://autoconf-archive.cryp.to/ac_cxx_compiler_vendor.html
dnl
dnl @category CXX
dnl @author Mateusz Loskot <mateusz@loskot.net>
dnl @version 2006-08-30
dnl @license AllPermissive

dnl
dnl Generic compiler detector
dnl
AC_DEFUN([AX_C_IFDEF],
[
    AC_COMPILE_IFELSE(
        [#ifndef $1
         #error "Compiler definition macro $1 is undefined!"
         #endif], [$2], [$3])
])

dnl
dnl Visual C++ compiler detector
dnl
dnl TODO - mloskot: It has not been tested with Autoconf + Visual C++.
dnl I even don't know if it's feasible and well-working to use
dnl Autoconf with Visual C++ toolset.
dnl If anyone knows how to test it, please give me a note. Thanks!
dnl
AC_DEFUN([AX_C_IFDEF_MSVC],
[
    AC_COMPILE_IFELSE(
        [
        #ifndef $1
        #error "Compiler definition macro $1 is undefined!"
        #endif
        ],
        [
        AC_COMPILE_IFELSE([#if _MSC_VER < 1300  // 1200 == VC++ 6.0, 1200-1202 == eVC++4
            #error "Macro test _MSC_VER < 1300 is false, it is not VC++ 6.0!"
            #endif
            ],
            [
            ac_cxx_msvc_toolset=vc60
            $2
            ],
            [
            AC_COMPILE_IFELSE([#if _MSC_VER <= 1300  // 1300 == VC++ 7.0
                #error "Macro test _MSC_VER <= 1300 is false, it is not VC++ 7.0!"
                #endif
                ],
                [
                dnl Visual C++ 6.0 detected
                ac_cxx_msvc_toolset=vc70
                $2
                ],
                [
                    AC_COMPILE_IFELSE([#if _MSC_VER <= 1300  // 1300 == VC++ 7.0
                        #error "Macro test _MSC_VER <= 1300 is false, it is not VC++ 7.0!"
                        #endif
                    ],
                    [
                    dnl Visual C++ 7.0 detected
                    ac_cxx_msvc_toolset=vc70
                    $2
                    ],
                    [
                        AC_COMPILE_IFELSE([#if _MSC_VER < 1310 // 1310 == VC++ 7.1
                            #error "Macro test _MSC_VER < 1310 is false, it is not VC++ 7.1!"
                            #endif
                        ],
                        [
                        dnl Visual C++ 7.1 detected
                        ac_cxx_msvc_toolset=vc71
                        $2
                        ],
                        [
                            AC_COMPILE_IFELSE([#if _MSC_VER <= 1400  // 1400 == VC++ 8.0
                                #error "Macro test _MSC_VER < 1400 is false, it is not VC++ 8.0!"
                                #endif
                            ],
                            [
                            dnl Visual C++ 8.0 detected
                            ac_cxx_msvc_toolset=vc80
                            $2
                            ],
                            [$3])
                        ])
                    ])
                ])
            ])
        ],
        [$3])
])

AC_DEFUN([AX_CXX_TOOLSET],
[
    AC_REQUIRE([AC_PROG_CXX])
    AC_REQUIRE([AC_PROG_CXXCPP])
    
    AC_CACHE_CHECK([the C++ compiler vendor],
        [ac_cxx_toolset_vendor],
        [
        AC_LANG_PUSH([C++])

        dnl GNU C/C++
        AX_C_IFDEF([__GNUG__],
            [
            ac_cxx_toolset_vendor="Free Software Foundation"
            ac_cxx_toolset_fullname="GNU C/C++ Compiler"
            ac_cxx_toolset_name="gcc"
            ac_cxx_toolset_cmd="g++"
            ],
            [
            dnl Microsoft Visual C++
            AX_C_IFDEF_MSVC([_MSC_VER],
                [
                ac_cxx_toolset_vendor="Microsoft"
                ac_cxx_toolset_fullname="Visual C++"
                ac_cxx_toolset_name=$ac_cxx_msvc_toolset
                ac_cxx_toolset_cmd="cl"
                ],
                [
                ac_cxx_toolset_vendor=unknown
                ac_cxx_toolset_fullname=unknown
                ac_cxx_toolset_name=unknown
                ac_cxx_toolset_cmd=unknown
                ])
            ])

        AC_LANG_POP()
        ])

    dnl
    dnl Output variables
    dnl
    CXX_TOOLSET_VENDOR=$ac_cxx_toolset_vendor
    CXX_TOOLSET_FULLNAME=$ac_cxx_toolset_fullname
    CXX_TOOLSET_NAME=$ac_cxx_toolset_name
    CXX_TOOLSET_COMMAND=$ac_cxx_toolset_cmd
    
])
