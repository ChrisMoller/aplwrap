#ifndef TXTBUF_H
#define TXTBUF_H

extern GtkTextBuffer *buffer;

typedef enum {
  TAG_INP,
  TAG_ERR,
  TAG_OUT,
  TAG_LCK,
  TAG_PRM,
  TAG_EDM,
  _tag_t_count
} tag_t;

GtkTextTag* get_tag (tag_t select);

void handle_history_replacement (gchar *text);

int handle_copy_down ();

gchar *get_input_text (gint *sz);

void mark_input ();

void tagged_insert (char   *text,
                    ssize_t text_idx,
                    tag_t   tag);

void image_insert (gchar     *text_before,
                   GdkPixbuf *image,
                   gchar     *text_after);

void define_tags ();

#endif
