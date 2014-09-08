#include <gtk/gtk.h>
#include <string.h>

#include "aplwrap.h"
#include "txtbuf.h"

GtkTextBuffer *buffer = NULL;

static GtkTextTag* tags[_tag_t_count] = { 0 };

static GtkTextTag*
get_tag (tag_t select)
{
  return (select >= _tag_t_count) ? NULL : tags[select];
}

void
tagged_insert (char   *text,
               ssize_t text_idx,
               tag_t   tag)
{
  gboolean rc;
  GtkTextIter insert_iter;
  GtkTextMark *mark = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);

  gboolean run = TRUE;
  while (run) {
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

  gtk_text_buffer_insert_with_tags (buffer,
				    &insert_iter,
				    text,
				    text_idx,
				    get_tag(tag),
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
    // TBCL
    //
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
        ztext[sz] = '\0';
        handle_history_replacement (ztext);
        g_free (ztext);
      }
      return 1;
    }
    else if (gtk_text_iter_has_tag (&insert_iter, get_tag(TAG_LCK)))
      return 1;
    return 0;
  }
}

gchar *
get_input_text (gint *sz)
{
  GtkTextMark *mark    = gtk_text_buffer_get_insert (buffer);
  GtkTextIter end_iter, start_iter;
  gchar *text;
  GtkTextTag* tag;
 
  gtk_text_buffer_get_iter_at_mark (buffer, &start_iter, mark);

  gtk_text_iter_set_line_offset (&start_iter, 0);
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
                                              "editable", FALSE,
                                              NULL);
}
