#include <gtk/gtk.h>
#include <string.h>

#include "aplwrap.h"
#include "txtbuf.h"

GtkTextBuffer *buffer = NULL;

GtkTextTag * err_tag;
GtkTextTag * out_tag;

static void
scroll_to_end ()
{
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				gtk_text_buffer_get_mark (buffer, "insert"),
				0.0,
				TRUE,
				0.2,
				1.0);
}

void
tagged_insert (char *text, ssize_t text_idx, GtkTextTag *tag)
{
  GtkTextIter insert_iter;
  GtkTextMark *mark = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);

  gtk_text_buffer_insert_with_tags (buffer,
                                    &insert_iter,
                                    text,
                                    text_idx,
                                    tag,
                                    NULL);

  scroll_to_end ();
}

void
handle_history_replacement (gchar *text)
{
  GtkTextIter start_iter, end_iter;

  // mark
  gtk_text_buffer_get_end_iter (buffer, &start_iter);
  gtk_text_iter_set_line_offset (&start_iter, 6);
  end_iter = start_iter;
  gtk_text_iter_forward_to_line_end (&end_iter);

  // delete
  gtk_text_buffer_delete (buffer, &start_iter, &end_iter);

  // insert
  gtk_text_buffer_get_end_iter (buffer, &end_iter);
  gtk_text_buffer_place_cursor (buffer, &end_iter);
  gtk_text_buffer_insert_at_cursor (buffer, text, strlen(text));

  // reveal
  scroll_to_end ();
}

void
handle_copy_down (gchar *text)
{
  // TBCL
  //
  // Plan:
  //  If selection is not empty and does not span newline
  //    copy selection to end of buffer
  //    If selection does not end with a space
  //      append a space to end of buffer
  //
  // NOTE: Do not scroll_to_end().
}

gchar *
get_input_text (gint *sz, int *from_selection)
{
  GtkTextMark *mark    = gtk_text_buffer_get_insert (buffer);
  GtkTextIter end_iter, start_iter;
  gchar *text;
 
  gtk_text_buffer_get_iter_at_mark (buffer, &start_iter, mark);

  gtk_text_iter_set_line_offset (&start_iter, 0);
  end_iter = start_iter;
  gtk_text_iter_forward_to_line_end (&end_iter);
  text = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
  *sz = gtk_text_iter_get_bytes_in_line (&start_iter);
  return text;
}

void
define_tags ()
{
  err_tag = gtk_text_buffer_create_tag (buffer, "err_tag",
                                        "foreground", "red",
                                        "editable", FALSE,
                                        NULL);
  out_tag = gtk_text_buffer_create_tag (buffer, "out_tag",
                                        "editable", FALSE,
                                        NULL);
}
