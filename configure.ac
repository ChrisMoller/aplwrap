AC_INIT([aplwrap], [1.0], [moller@mollerware.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])



# apl source directory
AC_ARG_WITH(apl-source-dir,
	AC_HELP_STRING(
		[--with-apl-source-dir],
		[APL source directory]),
	[APL_SOURCES=$withval],
	[echo "";
echo "apl-source-dir must be specified and point to the GNU APL source tree.";
echo ""; exit])
AC_SUBST(APL_SOURCES)



AC_PROG_CC
AM_PROG_CC_C_O

GETTEXT_PACKAGE=gtk30
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])

AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE([enable])

PKG_CHECK_MODULES([GTK],   [gtk+-3.0 >= 3.0.12])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT
