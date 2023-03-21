#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([dirutils], [0.0.1], [bug-dirutils@nongnu.org])
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

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
