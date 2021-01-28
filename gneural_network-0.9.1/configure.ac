#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gneural], [0.0.1.2], [jeanmichel.sellier@gmail.com])
AC_CONFIG_SRCDIR([src/gneural_network.c])
#AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [exp], [echo "GNU libm found."], [echo "GNU libm is not found. Aborting"])

# Checks for header files.
AC_CHECK_HEADERS([memory.h stdint.h stdlib.h string.h inttypes.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
#AC_CHECK_FUNCS([pow sqrt])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT([src/Makefile])
