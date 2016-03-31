#ifndef EDIT_H
#define EDIT_H

#include <gtk/gtk.h>

#include "search.h"

#define NEW_FILE ((gchar*)-1)

/* One per edit buffer. A buffer may be displayed in multiple windows. */
typedef struct {
  GtkTextBuffer *buffer;
  gchar *name;
  gint ref_count;
  gint nc;
  GSList *windows; /* list of window_s* */
} buffer_s;

/* One per edit window. */
typedef struct {
  GtkWidget *window;
  GtkWidget *status_line;
  GtkWidget *view;
  buffer_s  *buffer;
  gchar     *path;
  gboolean   error;
  gboolean   cb_done;
  gboolean   closing;
#if ENABLE_SEARCH
  search_context_t *search;
#endif
} window_s;
#define window(w) (w)->window
#define status(w) (w)->status_line
#define view(w)   (w)->view
#define buffer(w) (w)->buffer
#define path(w)   (w)->path
#define error(w)  (w)->error
#define cb_done(w) (w)->cb_done
#define closing(w) (w)->closing
#if ENABLE_SEARCH
#define search(w) (w)->search
#endif

void edit_object (gchar* name, gint nc);

void edit_file (gchar *path);

gboolean dirty_edit_buffers ();

gint message_dialog (GtkWidget     *parent,
		     GtkMessageType type,
                     GtkButtonsType buttons,
                     gchar         *message,
                     gchar         *secondary);

void save_dirty_edit_buffers ();

#endif  /* EDIT_H */
