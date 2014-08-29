#ifndef EDIT_H
#define EDIT_H

typedef struct {
  GtkTextBuffer *buffer;
  //  GtkWidget *window;
} window_s;

void edit_object (gchar* name, gint nc);

#endif  /* EDIT_H */
