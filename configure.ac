# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.64])
AC_INIT([MapMap],
        [0.0.1],
        [mapmap@toonloop.com],
        [mapmap],
        [http://mapmap.toonloop.com])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall -Wno-portability dist-xz no-define no-dist-gzip nostdinc subdir-objects])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_PROG_PKG_CONFIG([0.24])

AC_SUBST([TOONLOOP_WFLAGS], ["-Wall -DGSEAL_ENABLE"])

GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS([maximum])

#*******************************************************************************
# Libraries
#*******************************************************************************
# Before making a release, the LIB***_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
AC_SUBST([MAPMAP_LT_VERSION], [3:0:0])

# The version of the API. Should be major.minor and not change until not backward-compatible
AC_SUBST([MAPMAP_API_VERSION], [3.0])

#*******************************************************************************
# Internationalization
#*******************************************************************************
# AC_SUBST([GETTEXT_PACKAGE], [$PACKAGE_TARNAME])
# AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [GETTEXT package name])
# IT_PROG_INTLTOOL([0.50.0])

#*******************************************************************************
# Check required libraries
#*******************************************************************************

GLIB_REQUIRED="glib-2.0 >= 2.28.0"
GOBJECT_REQUIRED="gobject-2.0 >= 2.28.0"
GIO_REQUIRED="gio-2.0 >= 2.32.0"
GTK_REQUIRED="gtk+-3.0 >= 3.4.4"
GDK_REQUIRED="gdk-3.0 >= 3.3.6"
GDK_PIXBUF_REQUIRED="gdk-pixbuf-2.0"
GSTREAMER_VERSION=1.0.0
GSTREAMER_REQUIRED="gstreamer-plugins-base-1.0 >= $GSTREAMER_VERSION gstreamer-1.0 >= $GSTREAMER_VERSION gstreamer-pbutils-1.0 >= $GSTREAMER_VERSION"
GSTREAMER_PLUGINS_BAD_VERSION=1.0.0
GSTREAMER_PLUGINS_BAD_REQUIRED="gstreamer-plugins-bad-1.0 >= $GSTREAMER_PLUGINS_BAD_VERSION"
CAIRO_REQUIRED="cairo >= 1.10.0"
PANGOCAIRO_REQUIRED="pangocairo >= 1.28.0"
LIBRSVG_REQUIRED="librsvg-2.0 >= 2.32.0"
CLUTTER_REQUIRED="clutter-1.0 >= 1.13.2"
CLUTTERGTK_REQUIRED="clutter-gtk-1.0 >= 0.91.8"
CLUTTERGST_REQUIRED="clutter-gst-2.0 >= 1.9.0"
LIBCANBERRA_REQUIRED="libcanberra-gtk3 >= 0.26"

# Libtool requires this, but it is only expanded once anyway.
AC_CANONICAL_HOST

PKG_CHECK_MODULES([MAPMAP],
  [$GLIB_REQUIRED
  $GIO_REQUIRED
  $GOBJECT_REQUIRED
  $GDK_PIXBUF_REQUIRED
  $GSTREAMER_REQUIRED
  $GSTREAMER_PLUGINS_BAD_REQUIRED
  $LIBCANBERRA_REQUIRED
  $CAIRO_REQUIRED
  $LIBRSVG_REQUIRED
  $PANGOCAIRO_REQUIRED
  $CLUTTER_REQUIRED
  $GTK_REQUIRED
  $GDK_REQUIRED
  $CLUTTERGTK_REQUIRED
  $CLUTTERGST_REQUIRED])

GLIB_GSETTINGS
#*******************************************************************************

AC_CONFIG_FILES([
  Makefile
  mapmap/Makefile
  src/Makefile
  data/Makefile
  po/Makefile
  data/mapmap.desktop.in
	data/mapmap-${MAPMAP_API_VERSION}.pc:data/mapmap.pc.in
  ])
AC_OUTPUT

#**************************************************************************
