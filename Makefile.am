SUBDIRS = src
dist_doc_DATA = README
gapl2fontdir = $(prefix)/share/fonts
gapl2font_DATA=fonts/*.ttf
EXTRA_DIST = autogen.sh images fonts

install-data-hook:
	fc-cache

