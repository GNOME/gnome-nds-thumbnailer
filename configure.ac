AC_PREREQ(2.59)

AC_INIT([gnome-nds-thumbnailer], [1.2.1], [hadess@hadess.net])
AM_INIT_AUTOMAKE(1.9 dist-bzip2 no-dist-gzip check-news)

AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_SED
AM_PROG_CC_C_O

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0, [], [AC_MSG_FAILURE([You need gdk-pixbuf library >= 2.0])])
AC_SUBST(GDK_PIXBUF_CFLAGS)
AC_SUBST(GDK_PIXBUF_LIBS)

PKG_CHECK_MODULES(GIO, gio-2.0, [], [AC_MSG_FAILURE([You need glib 2.16 or newer installed])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
