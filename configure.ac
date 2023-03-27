#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Copyright (C) 2023 OSN Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

AC_PREREQ([2.71])
AC_INIT([dirutils], [0.4.0], [bug-dirutils@nongnu.org], [dirutils], [https://dirutils.nongnu.org/])
AM_SILENT_RULES([yes])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/dirstats.c])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([dirent.h getopt.h string.h unistd.h libgen.h signal.h sys/inotify.h sys/stat.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strdup strerror inotify_init])

AC_MSG_CHECKING([whether to enable colorized output])
AC_ARG_ENABLE([colors], [Enables colorized output on the terminal], [
    AC_SUBST([COLOR_CFLAGS], [-DUSE_COLORS])
    AC_MSG_RESULT([yes])
], [
    AC_MSG_RESULT([no])
])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
