#ifndef APLWRAP_H
#define APLWRAP_H

#include <gtk/gtk.h>

void beep ();

void scroll_to_end ();

gchar *handle_apl_characters (gsize *bw_p, GdkEventKey *key_event);

#endif
