#ifndef TXTBUF_H
#define TXTBUF_H

extern GtkTextBuffer *buffer;

extern GtkTextTag * err_tag;
extern GtkTextTag * out_tag;

void handle_history_replacement (gchar *text);

void handle_copy_down (gchar *text);

gchar *get_input_text (gint *sz, int *from_selection);

void tagged_insert (char *text, ssize_t text_idx, GtkTextTag *tag);

void define_tags ();

#endif
