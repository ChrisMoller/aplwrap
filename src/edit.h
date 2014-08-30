#ifndef EDIT_H
#define EDIT_H

typedef struct {
  GtkTextBuffer *buffer;
  gboolean modified;
  gchar *name;
  gint ref_count;
} buffer_s;

typedef struct {
  GtkWidget *window;
  buffer_s  *buffer;
} window_s;
#define window(w) (w)->window
#define buffer(w) (w)->buffer

void edit_object (gchar* name, gint nc);

#endif  /* EDIT_H */
