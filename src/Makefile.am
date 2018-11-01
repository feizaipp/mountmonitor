AM_CPPFLAGS = \
	$(DBUS_GLIB_CFLAGS) \
	$(UDISKS2_CFLAGS)

LIBS = \
	$(DBUS_GLIB_LIBS) \
	$(UDISKS2_LIBS)

noinst_PROGRAMS = mountmonitor
mountmonitor_SOURCES = main.c mountmonitor.c mountmonitor.h mountinfo.c mountinfo.h

BUILT_SOURCES = mountmonitor-glue.h

$(BUILT_SOURCES) : mountmonitor.xml
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=mountmonitor --mode=glib-server \
                        --output=mountmonitor-glue.h $(srcdir)/mountmonitor.xml

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = mountmonitor.xml

