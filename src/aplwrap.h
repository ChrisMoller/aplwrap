#ifndef APLWRAP_H
#define APLWRAP_H

#include <gtk/gtk.h>

gchar *plot_pipe_name;
gint   plot_pipe_fd;

gchar *get_rows_assign ();

void beep ();

void update_status_line (gchar *text);

void scroll_to_end ();

gchar *handle_apl_characters (gsize *bw_p, GdkEventKey *key_event);

#endif
