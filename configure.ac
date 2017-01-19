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
# We define AUDIT_LIB_VERSION so we can, at compile time work our the version of audit-libs available
# as we need to conditionally compile depending on the version
# This means our packaging build could run
#  ./configure ... AUDIT_LIB_VERSION=$(printf "%03d%03d%03d" $(echo `rpm -q --qf '%{VERSION}' audit-libs` | tr '.' ' '))
# or
#  ./configure ... AUDIT_LIB_VERSION=$(printf "%03d%03d%03d" $(echo `dpkg-query --showformat='${Version}' --show libauparse0` | cut -d: -f2 | cut -d- -f1 | tr '.' ' '))
ABS_SRCDIR=`cd ${srcdir}; pwd`
CPPFLAGS="-I${ABS_SRCDIR} -I${ABS_SRCDIR}/include -DNDEBUG -DAUDIT_LIB_VERSION=$AUDIT_LIB_VERSION $CPPFLAGS"

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

# Output
AC_CONFIG_FILES([Makefile
                 include/Makefile
                 include/aushape/Makefile
                 lib/Makefile
                 src/Makefile])
AC_OUTPUT
