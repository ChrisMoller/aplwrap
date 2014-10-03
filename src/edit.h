#ifndef EDIT_H
#define EDIT_H

typedef struct {
  GtkTextBuffer *buffer;
  gchar *name;
  gint ref_count;
  gint nc;
} buffer_s;

typedef struct {
  GtkWidget *window;
  GtkWidget *status_line;
  buffer_s  *buffer;
} window_s;
#define window(w) (w)->window
#define buffer(w) (w)->buffer
#define status(w) (w)->status_line

void edit_object (gchar* name, gint nc);

#endif  /* EDIT_H */
