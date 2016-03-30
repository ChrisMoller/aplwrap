#ifndef APLWRAP_H
#define APLWRAP_H

#include <gtk/gtk.h>

#define PGM_TITLE "APLwrap"

gchar *plot_pipe_name;
gint   plot_pipe_fd;

gchar *get_rows_assign ();

void beep ();

void update_status_line (gchar *text);

void scroll_to_cursor ();

gchar *handle_apl_characters (gsize *bw_p, GdkEventKey *key_event);

void set_status_visibility (gboolean show);

void set_font (GtkTextBuffer *buffer);

GtkWindow *get_top_window (void);

#endif
