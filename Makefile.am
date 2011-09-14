
bin_PROGRAMS = gnome-nds-thumbnailer

gnome_nds_thumbnailer_SOURCES = gnome-nds-thumbnailer.c
gnome_nds_thumbnailer_CPPFLAGS = $(THUMBNAILER_CFLAGS)
gnome_nds_thumbnailer_LDADD = $(THUMBNAILER_LIBS)

thumbnailerdir = $(datadir)/thumbnailers/
thumbnailer_DATA = gnome-nds-thumbnailer.thumbnailer
gnome-nds-thumbnailer.thumbnailer: gnome-nds-thumbnailer.thumbnailer.in Makefile
	$(AM_V_GEN) $(SED) -e "s|\@bindir\@|$(bindir)|" $< > $@

EXTRA_DIST = gnome-nds-thumbnailer.thumbnailer.in AUTHORS COPYING README

CLEANFILES = gnome-nds-thumbnailer.thumbnailer
