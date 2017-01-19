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
AC_INIT([aushape], [1])
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
PKG_CHECK_MODULES([AUPARSE], [auparse])

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
        AC_DEFINE(HAVE_NEW_AUPARSE_SET_ESCAPE_MODE, [1],
                  [Define if auparse_set_escape_mode accepts two arguments])
    ],
    [AC_MSG_RESULT([one-argument])]
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
