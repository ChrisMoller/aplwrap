bin_PROGRAMS = aplwrap
aplwrap_SOURCES = aplwrap.c Avec.h keymap.h layout.h \
                  menu.c menu.h options.c options.h \
                  apl.c apl.h aplio.c aplio.h \
                  history.c history.h txtbuf.c txtbuf.h \
                  edit.c edit.h complete.c complete.h \
                  pstat.c pstat.h resources.c resources.h \
		  help.c help.h search.c search.h

aplwrap_CFLAGS = -std=c99 -Wall -Werror $(GTK_CFLAGS) -I$(APL_SOURCES) 
aplwrap_CPPFLAGS=-DMANUALS_PATH=\"$(docdir)\"

aplwrap_LDADD = $(GTK_LIBS)

BUILT_SOURCES = build.h

build.h: build.h-new
	diff $@ $^ 2>/dev/null || mv $^ $@
	rm -f $^

build.h-new:
	./buildinfo > $@

clean-local:
	rm -f build.h
