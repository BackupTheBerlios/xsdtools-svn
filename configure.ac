AC_INIT([xsdtools], [0.1], [andrey.baboshin@gmail.com])
PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.4)
BOOST_REQUIRE([1.39.0])
AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 libxsd/Makefile
 xsd2sql/Makefile
 xsd2hbm/Makefile
])
AC_OUTPUT

