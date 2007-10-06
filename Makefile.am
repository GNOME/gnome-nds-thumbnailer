
bin_PROGRAMS = gnome-nds-thumbnailer

gnome_nds_thumbnailer_SOURCES = gnome-nds-thumbnailer.c

gnome_nds_thumbnailer_CPPFLAGS = $(GDK_PIXBUF_CFLAGS)

gnome_nds_thumbnailer_LDADD = $(GDK_PIXBUF_LIBS)


schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = gnome-nds-thumbnailer.schemas.in
schemas_DATA = gnome-nds-thumbnailer.schemas

# Rule to make the service file with bindir expanded
$(schemas_DATA): $(schemas_in_files) Makefile
	@$(SED) -e "s|\@bindir\@|$(bindir)|" $< > $@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
          for p in $(schemas_DATA) ; do \
            GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p ; \
          done \
        fi
else
install-data-local:
endif

EXTRA_DIST = gnome-nds-thumbnailer.schemas.in AUTHORS COPYING README

CLEANFILES = gnome-nds-thumbnailer.schemas
