AM_CFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DPACKAGE_DATADIR=\""$(pkgdatadir)"\" \
	-DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_builddir) \
	$(MAPMAP_CFLAGS) \
	$(MAPMAP_GTK_FLAGS) \
	$(MAPMAP_WFLAGS) \
	$(WARN_CFLAGS)

bin_PROGRAMS = mapmap

mapmap_SOURCES = \
	main.c \
	main-window.c \
	$(NULL)

noinst_HEADERS = \
	main-window.h \
	$(NULL)

mapmap_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(MAPMAP_CFLAGS) \
	-include $(top_builddir)/src/$(CONFIG_HEADER) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(NULL)

mapmap_CFLAGS = -export-dynamic

mapmap_LDADD = \
	$(top_srcdir)/mapmap/libmapmap-$(MAPMAP_API_VERSION).la \
	$(MAPMAP_LIBS) \
	$(NULL)

