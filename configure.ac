#
# Copyright (C) 2016 Red Hat
#
# This file is part of aushape.
#
# Aushape is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Aushape is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with aushape; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([aushape], [2])
AM_INIT_AUTOMAKE([1.9 -Wall foreign])
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_GNU_SOURCE
ABS_SRCDIR=`cd ${srcdir}; pwd`
CPPFLAGS="-I${ABS_SRCDIR} -I${ABS_SRCDIR}/include -DNDEBUG $CPPFLAGS"

# Check for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

# Check for features
AC_ARG_ENABLE(
    debug,
    AS_HELP_STRING([--enable-debug], [enable debugging features]),
    [], [enable_debug="no"])

# Output features to preprocessor and compiler
if test "$enable_debug" = "yes"; then
    CPPFLAGS="$CPPFLAGS -UNDEBUG"
    CFLAGS="$CFLAGS -Wall -Wextra -Werror -g -O0"
fi

# Check for libraries
PKG_CHECK_MODULES(
    [AUPARSE], [auparse], ,
    [
        AC_CHECK_LIB(
            [auparse], [auparse_init],
            [
                AC_SUBST(AUPARSE_CFLAGS, [])
                AC_SUBST(AUPARSE_LIBS, [-lauparse])
            ],
            [
                AC_MSG_ERROR([libauparse not found])
            ]
        )
    ]
)

have_audit=yes
PKG_CHECK_MODULES(
    [AUDIT], [audit], ,
    [
        AC_CHECK_LIB(
            [audit], [audit_open],
            [
                AC_SUBST(AUDIT_CFLAGS, [])
                AC_SUBST(AUDIT_LIBS, [-laudit])
            ],
            [
                have_audit=no
            ]
        )
    ]
)

# Check for functions
AC_MSG_CHECKING([for version of auparse_set_escape_mode])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[
        #include <auparse.h>
        void
        f(void)
        {
            auparse_set_escape_mode(NULL, AUPARSE_ESC_RAW);
        }
    ]])],
    [
        AC_MSG_RESULT([two-argument])
        AC_DEFINE(AUPARSE_SET_ESCAPE_MODE_VER, [2],
                  [Define to the version of auparse_set_escape_mode])
    ],
    [
        AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM([[
                #include <auparse.h>
                void
                f(void)
                {
                    auparse_set_escape_mode(AUPARSE_ESC_RAW);
                }
            ]])],
            [
                AC_MSG_RESULT([one-argument])
                AC_DEFINE(AUPARSE_SET_ESCAPE_MODE_VER, [1],
                          [Define to the version of auparse_set_escape_mode])
            ],
            [
                AC_MSG_RESULT([missing])
            ]
        )
    ]
)

AC_CHECK_DECL(
    [auparse_get_type_name],
    [AC_DEFINE(HAVE_AUPARSE_GET_TYPE_NAME, [1],
               [Define to 1 if you have auparse_get_type_name defined in auparse.h])],
    [
        AC_MSG_NOTICE([linking with libaudit to implement auparse_get_type_name])
        if test "x$have_audit" = xyes; then
            AUPARSE_CFLAGS="$AUPARSE_CFLAGS $AUDIT_CFLAGS"
            AUPARSE_LIBS="$AUPARSE_LIBS $AUDIT_LIBS"
        else
            AC_MSG_ERROR([libaudit not found])
        fi
    ],
    [[#include <auparse.h>]]
)

# Check for symbols
AC_CHECK_DECLS([AUPARSE_TYPE_ESCAPED_KEY],,,[[#include <auparse.h>]])

# Output
AC_CONFIG_FILES([Makefile
                 include/Makefile
                 include/aushape/Makefile
                 lib/Makefile
                 src/Makefile])
AC_OUTPUT
