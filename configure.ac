#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(mountmonitor, 1.0)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1, have_dbus_glib=yes, have_dbus_glib=no)
if test x$have_dbus_glib = xno ; then
    AC_MSG_ERROR([DBus-GLib development libraries not found])
fi

AM_CONDITIONAL(HAVE_DBUS_GLIB, test x$have_dbus_glib = xyes)

AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

PKG_CHECK_MODULES(UDISKS2, udisks2, have_udisks2=yes, have_udisks2=no)
if test x$have_udisks2 = xno ; then
    AC_MSG_ERROR([have_udisks2 development libraries not found])
fi

AM_CONDITIONAL(HAVE_UDISKS2, test x$have_udisks2 = xyes)

AC_SUBST(UDISKS2_CFLAGS)
AC_SUBST(UDISKS2_LIBS)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT([Makefile
           src/Makefile])
