## SUBDIRS=test
AM_CFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DPACKAGE_DATADIR=\""$(pkgdatadir)"\" \
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_builddir) \
	$(MAPMAP_CFLAGS) \
	$(MAPMAP_WFLAGS) \
	$(WARN_CFLAGS) \
	$(NULL)

lib_LTLIBRARIES = libmapmap-@MAPMAP_API_VERSION@.la

libmapmap_@MAPMAP_API_VERSION@_la_LIBADD = \
	$(MAPMAPLOOP_LIBS) \
	$(NULL)

# use lib_LTLIBRARIES to build a shared lib:

## Instruct libtool to include ABI version information in the generated shared
## library file (.so).  The library ABI version is defined in configure.ac, so
## that all version information is kept in one place.
libmapmap_@MAPMAP_API_VERSION@_la_LDFLAGS = -export-dynamic -version-info $(MAPMAP_LT_VERSION) $(AM_LIBS)
libmapmap_@MAPMAP_API_VERSION@_la_LIBS = $(AM_LIBS)
libmapmap_@MAPMAP_API_VERSION@_la_CXXFLAGS = $(AM_CXXFLAGS)
libmapmap_@MAPMAP_API_VERSION@_la_includedir = $(includedir)/mapmap-$(MAPMAP_API_VERSION)/mapmap

## Define the source file list for the "libmapmap-@MAPMAP_API_VERSION@.la"
## target.  Note that @MAPMAP_API_VERSION@ is not interpreted by Automake and
## will therefore be treated as if it were literally part of the target name,
## and the variable name derived from that.
## PLEASE KEEP THEM IN ALPHABETICAL ORDER
libmapmap_@MAPMAP_API_VERSION@_la_SOURCES = \
	version.c \
	$(NULL)

## PLEASE KEEP THEM IN ALPHABETICAL ORDER
libmapmap_@MAPMAP_API_VERSION@_la_include_HEADERS = \
	version.h \
	$(NULL)

