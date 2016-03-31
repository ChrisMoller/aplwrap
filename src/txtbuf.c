#include <gtk/gtk.h>
#include <string.h>

#include "aplwrap.h"
#include "txtbuf.h"
#include "options.h"
#include "aplio.h"

GtkTextBuffer *buffer = NULL;

static GtkTextTag* tags[_tag_t_count] = { 0 };

GtkTextTag*
get_tag (tag_t select)
{
  return (select >= _tag_t_count) ? NULL : tags[select];
}

void
tagged_insert (char   *text,
               ssize_t text_idx,
               tag_t   tag)
{
  GtkTextIter insert_iter;
  GtkTextMark *mark = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);

  gboolean run = TRUE;
  while (run) {
    gboolean rc;
    gchar *end = NULL;
    gchar *ptr = text;
    ssize_t remaining  = text_idx;
    
    rc = g_utf8_validate (ptr, remaining, (const gchar **)(&end));

    if (!rc) {
      if (end) {
	*end = ' ';
	remaining -= (end - ptr) + 1;
	ptr = end + 1;
	end = NULL;
      }
      continue;
    }
    else run = FALSE;
  }

  {
    gchar *ptr = text;
    while(ptr < text + text_idx) {
      gunichar c = g_utf8_get_char_validated (ptr, -1);
      gchar *op = ptr;
      ptr = g_utf8_next_char (ptr);
      if (g_unichar_iscntrl (c) && *op != '\n' && *op != '\r') {
	gint cl = ptr - op;
	if (*op == '\a') {
          gdk_beep ();
          memmove (op, op+1, text_idx-(ptr-text)+1);
          --text_idx;
          --ptr;
        }
        else
          for (int i = 0; i < cl; i++) *op++ = '.';
      }
    }
  }

  if (strncmp (text+text_idx-7, "\r      ", 7))
      gtk_text_buffer_insert_with_tags (buffer,
                                        &insert_iter,
                                        text,
                                        text_idx,
                                        get_tag(tag),
                                        NULL);
  else {
      gtk_text_buffer_insert_with_tags (buffer,
                                        &insert_iter,
                                        text,
                                        text_idx-7,
                                        get_tag(tag),
                                        NULL);
      gtk_text_buffer_insert_with_tags (buffer,
                                        &insert_iter,
                                        text+text_idx-7,
                                        7,
                                        get_tag(TAG_PRM),
                                        NULL);
  }    
  scroll_to_cursor ();
}

void
image_insert (gchar     *text_before,
              GdkPixbuf *image_pixbuf,
              gchar     *text_after)
{
  GtkTextIter insert_iter, left_iter, right_iter;
  GtkTextMark *mark;
  static GtkTextMark *pixbuf_left = 0, *pixbuf_right = 0;

  if (pixbuf_left == NULL) {
    GtkTextIter start_iter;
    pixbuf_left = gtk_text_mark_new ("pixbuf-left", TRUE);
    gtk_text_buffer_get_start_iter (buffer, &start_iter);
    gtk_text_buffer_add_mark (buffer, pixbuf_left, &start_iter);
  }
  if (pixbuf_right == NULL) {
    GtkTextIter start_iter;
    pixbuf_right = gtk_text_mark_new ("pixbuf-right", FALSE);
    gtk_text_buffer_get_start_iter (buffer, &start_iter);
    gtk_text_buffer_add_mark (buffer, pixbuf_right, &start_iter);
  }

  tagged_insert (text_before, -1, TAG_OUT);
  mark = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);
  gtk_text_buffer_move_mark (buffer, pixbuf_left, &insert_iter);
  gtk_text_buffer_insert_pixbuf (buffer, &insert_iter, image_pixbuf);
  mark = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);
  gtk_text_buffer_move_mark (buffer, pixbuf_right, &insert_iter);
  gtk_text_buffer_get_iter_at_mark (buffer, &left_iter, pixbuf_left);
  gtk_text_buffer_get_iter_at_mark (buffer, &right_iter, pixbuf_right);
  gtk_text_buffer_apply_tag (buffer, get_tag (TAG_OUT),
                             &left_iter, &right_iter);
  tagged_insert (text_after, -1, TAG_OUT);
  scroll_to_cursor ();
}

void
handle_history_replacement (gchar *text)
{
  if (is_at_prompt ()) {
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
    scroll_to_cursor ();
  }
}

int
handle_copy_down ()
{
  GtkTextIter start_iter, end_iter;
  if (gtk_text_buffer_get_selection_bounds (buffer, &start_iter, &end_iter)) {
    //  Case 1: selection is not empty
    //
    //  If selection does not span newline
    //    copy selection to end of buffer
    //    If selection does not end with a space
    //      append a space to end of buffer
    //    *Do not* scroll to end!
    gchar *text = gtk_text_buffer_get_text (buffer,
                                            &start_iter,
                                            &end_iter,
                                            FALSE);
    if (text == NULL || strchr (text, '\n')) return 0;

    gtk_text_buffer_get_end_iter (buffer, &end_iter);
    gtk_text_buffer_place_cursor (buffer, &end_iter);
    gtk_text_buffer_insert_at_cursor (buffer, text, -1);
    if (text[strlen(text)-1] != ' ')
      gtk_text_buffer_insert_at_cursor (buffer, " ", -1);

    g_free (text);
    return 1;
  }
  else {
    //  Case 2: selection is empty
    //
    //  If cursor is in previous input
    //    copy previous input to end of buffer
    //    scroll to end of buffer
    GtkTextIter insert_iter;
    GtkTextMark *mark = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);
    if (gtk_text_iter_has_tag (&insert_iter, get_tag(TAG_INP))) {
      gint sz;
      gchar *text = get_input_text (&sz);
      gchar *ztext = g_try_malloc (sz+1-6);
      if (ztext) {
        memcpy(ztext, text+6, sz-6);
#if 1	// bug reported by Christian Robert
	ztext[sz-6] = '\0';
#else
        ztext[sz] = '\0';
#endif
        handle_history_replacement (ztext);
        g_free (ztext);
      }
      return 1;
    }
    else {
      if (gtk_text_iter_has_tag (&insert_iter, get_tag(TAG_LCK))) 
	return 1;
    }
    return 0;
  }
}

static gint input_offset;

void
mark_input ()
{
  GtkTextIter input_iter;
  GtkTextMark *input_mark = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &input_iter, input_mark);
  gtk_text_iter_set_line_offset (&input_iter, 0);
  input_offset = gtk_text_iter_get_offset (&input_iter);
}

gchar *
get_input_text (gint *sz)
{
  GtkTextMark *mark    = gtk_text_buffer_get_insert (buffer);
  GtkTextIter end_iter, start_iter, input_iter;
  gchar *text;
  GtkTextTag* tag;
 
  gtk_text_buffer_get_iter_at_mark (buffer, &start_iter, mark);
  gtk_text_iter_set_line_offset (&start_iter, 0);
  gtk_text_buffer_get_iter_at_offset (buffer, &input_iter, input_offset);
  if (!gtk_text_iter_begins_tag (&start_iter, get_tag (TAG_PRM)) &&
      !gtk_text_iter_equal (&input_iter, &start_iter)) {
    gtk_text_buffer_get_end_iter (buffer, &end_iter);
    gtk_text_iter_set_line_offset (&input_iter, 6);
    gtk_text_buffer_delete (buffer, &input_iter, &end_iter);
    gtk_text_buffer_get_iter_at_mark (buffer, &start_iter, mark);
    beep ();
  }
  end_iter = start_iter;
  gtk_text_iter_forward_to_line_end (&end_iter);
  text = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
  *sz = gtk_text_iter_get_bytes_in_line (&start_iter);
  tag = get_tag (*sz >= 6 && !strncmp ("      ", text, 6) ? TAG_INP : TAG_LCK);
  gtk_text_buffer_apply_tag (buffer, tag, &start_iter, &end_iter);
  
  return text;
}

void
define_tags ()
{
  if (nocolour) {
   tags[TAG_INP] = gtk_text_buffer_create_tag (buffer, "inp_tag",
                                               "editable", FALSE,
                                               NULL);
   tags[TAG_ERR] = gtk_text_buffer_create_tag (buffer, "err_tag",
                                               "editable", FALSE,
                                               NULL);
   tags[TAG_OUT] = gtk_text_buffer_create_tag (buffer, "out_tag",
                                               "editable", FALSE,
                                               NULL);
   tags[TAG_LCK] = tags[TAG_OUT];

   tags[TAG_PRM] = gtk_text_buffer_create_tag (buffer, "prompt_tag",
                                               "editable", FALSE,
                                               NULL);

   tags[TAG_EDM] = gtk_text_buffer_create_tag (buffer, "editor_message_tag",
                                               "editable", FALSE,
                                               NULL);
  }
  else {
   tags[TAG_INP] = gtk_text_buffer_create_tag (buffer, "inp_tag",
                                               "foreground", "blue",
                                               "editable", FALSE,
                                               NULL);
   tags[TAG_ERR] = gtk_text_buffer_create_tag (buffer, "err_tag",
                                               "foreground", "red",
                                               "editable", FALSE,
                                               NULL);
   tags[TAG_OUT] = gtk_text_buffer_create_tag (buffer, "out_tag",
                                               "editable", FALSE,
                                               NULL);
   tags[TAG_LCK] = tags[TAG_OUT];

   tags[TAG_PRM] = gtk_text_buffer_create_tag (buffer, "prompt_tag",
                                               "foreground", "grey",
                                               "background", "azure",
                                               "editable", FALSE,
                                               NULL);

   tags[TAG_EDM] = gtk_text_buffer_create_tag (buffer, "editor_message_tag",
                                               "foreground", "green",
                                               "editable", FALSE,
                                               NULL);
  }
}
