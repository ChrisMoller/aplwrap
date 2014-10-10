SUBDIRS = src
dist_doc_DATA = README APL_STATUS_LINE.md COMPLETION.md KEY_BINDINGS.md \
	APLPLOT.md LICENSE
gapl2fontdir = $(prefix)/share/fonts
gapl2font_DATA=fonts/*.ttf
EXTRA_DIST = autogen.sh images fonts

install-data-hook:
	fc-cache

