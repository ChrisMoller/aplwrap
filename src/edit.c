#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "apl.h"
#include "aplio.h"
#include "menu.h"
#include "aplwrap.h"
#include "txtbuf.h"
#include "options.h"
#include "edit.h"

static GHashTable *buffers = NULL;
static gint seq_nr = 1;

static void
set_status_line (window_s *tw, buffer_s *tb)
{
  GtkTextIter line_iter, end_iter;
  GtkTextMark *insert = gtk_text_buffer_get_insert (tb->buffer);
  gtk_text_buffer_get_iter_at_mark (tb->buffer, &line_iter, insert);
  gint line_nr = gtk_text_iter_get_line (&line_iter);
  gint offset  = gtk_text_iter_get_line_offset (&line_iter);
  gtk_text_buffer_get_end_iter (tb->buffer, &end_iter);
  gint line_ct = gtk_text_buffer_get_line_count (tb->buffer) -
    (gtk_text_iter_get_line_offset (&end_iter) == 0);
  gboolean modified = gtk_text_buffer_get_modified (tb->buffer);
  
  gchar *st = g_strdup_printf ("%s %d / %d, %d\n",
			       modified ? "**" : "  ",
			       line_nr, line_ct, offset);
  gtk_label_set_text (GTK_LABEL (status (tw)), st);
  g_free (st);
}

static void
edit_mark_set_event (GtkTextBuffer *buffer, GtkTextIter *location,
                     GtkTextMark *mark, gpointer data)
{
  window_s *tw = (window_s*)data;
  buffer_s *tb = tw->buffer;
  set_status_line (tw, tb);
}

static void
edit_close (GtkWidget *widget,
	    gpointer  data)
{
  window_s *tw = data;
  if (!gtk_widget_in_destruction (GTK_WIDGET (window (tw)))) {
    buffer_s *tb = buffer (tw);
    tb->ref_count--;
    if (0 == tb->ref_count) {
      g_hash_table_remove (buffers, tb->name);
      g_free (tb->name);
      g_free (tb);
    }
    gtk_widget_destroy(window (tw));
    g_free (tw);
  }
}

static void
message_dialog (GtkMessageType type,
                gchar         *message,
                gchar         *secondary)
{
  GtkWidget *dialog =
    gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, type, GTK_BUTTONS_CLOSE,
                            message);
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%s", secondary);
  gtk_dialog_run (GTK_DIALOG(dialog));
  gtk_widget_destroy (dialog);
}

static void
edit_save_object_cb (gchar *text, void *data)
{
  window_s *tw = data;
  set_send_cb (NULL, NULL);
  error (tw) = FALSE;
  gchar *error_text = NULL;
  gchar **lines = g_strsplit (text, "\n", 0);
  for (int i = 0; lines[i]; i++) {
    if (!*lines[i]) continue;
    if (!error (tw) && !g_ascii_strncasecmp (lines[i], "error", 5)) {
      error (tw) = TRUE;
      continue;
    }
    if (!g_strcmp0 (lines[i], END_TAG)) break;
    if (error (tw)) {
      if (!error_text)
        error_text = g_strdup (lines[i]);
      else {
        gchar *new_error_text =
          g_strconcat (error_text, "\n", lines[i], NULL);
        g_free (error_text);
        error_text = new_error_text;
      }
    }
    else {
      tagged_insert (lines[i], -1, TAG_EDM);
      tagged_insert (" ", -1, TAG_EDM);
    }
  }
  g_strfreev (lines);
  if (error_text) {
    message_dialog (GTK_MESSAGE_WARNING,
                    _ ("APL error while defining function"),
                    error_text);
    g_free (error_text);
  }
  else {
    tagged_insert ("\n      ", -1, TAG_PRM);
    mark_input ();
  }
  buffer_s *tb = buffer (tw);
  if (!error (tw)) {
    gtk_text_buffer_set_modified (tb->buffer, FALSE);
    set_status_line (tw, tb);
  }
  cb_done (tw) = TRUE;
}

static void
clone_object (GtkWidget *widget,
	      gpointer  data)
{
  window_s *tw = data;
  buffer_s *tb = buffer (tw);

  edit_object (tb->name, tb->nc);
}

static void
edit_save_object (GtkWidget *widget,
                  gpointer  data)
{
  window_s *tw = data;
  buffer_s *tb = buffer (tw);
  GtkTextIter start_iter, end_iter;
  gtk_text_buffer_get_bounds (tb->buffer, &start_iter, &end_iter);
  gchar *text =
    gtk_text_buffer_get_text (tb->buffer, &start_iter, &end_iter, FALSE);
  if (text[strlen (text) - 1] != '\n') {
    gtk_text_buffer_insert (tb->buffer, &end_iter, "\n", 1);
    gtk_text_buffer_get_bounds (tb->buffer, &start_iter, &end_iter);
    g_free (text);
    text =
      gtk_text_buffer_get_text (tb->buffer, &start_iter, &end_iter, FALSE);
  }

#define DEF_CMD "def\n"
  cb_done (tw) = FALSE;
  set_send_cb (edit_save_object_cb, tw);
  send_apl (DEF_CMD, strlen (DEF_CMD));

  gchar *ptr     = text;
  while (ptr && *ptr) {
    gchar *end_ptr = strchr (ptr, '\n');
    if (end_ptr++) {
      send_apl (ptr, end_ptr - ptr);
      ptr = end_ptr;
    }
  }
  g_free (text);
  send_apl (END_TAGNL, strlen(END_TAGNL));
}

static void
clone_file (GtkWidget *widget,
	      gpointer  data)
{
  window_s *tw = data;

  edit_file (path (tw));
}

static void
edit_save_file (GtkWidget *widget,
                gpointer  data)
{
  window_s *tw = data;
  buffer_s *tb = buffer (tw);
  if (path (tw) || set_filename("Save File", &(path (tw)))) {
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_start_iter (tb->buffer, &start_iter);
    gtk_text_buffer_get_end_iter (tb->buffer, &end_iter);
    gchar *text =
      gtk_text_buffer_get_text (tb->buffer, &start_iter, &end_iter, FALSE);
    g_file_set_contents (path (tw), text, -1, NULL);
    gtk_text_buffer_set_modified (tb->buffer, FALSE);
    set_status_line (tw, tb);
  }
}

static void
edit_save_file_as (GtkWidget *widget,
                   gpointer  data)
{
  window_s *tw = data;
  buffer_s *tb = buffer (tw);
  if (set_filename ("Save File", &(path (tw)))) {
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_start_iter (tb->buffer, &start_iter);
    gtk_text_buffer_get_end_iter (tb->buffer, &end_iter);
    gchar *text =
      gtk_text_buffer_get_text (tb->buffer, &start_iter, &end_iter, FALSE);
    g_file_set_contents (path (tw), text, -1, NULL);
    gtk_text_buffer_set_modified (tb->buffer, FALSE);
    set_status_line (tw, tb);
    gchar *lname = g_path_get_basename (path (tw));
    gtk_window_set_title (GTK_WINDOW (window (tw)), lname);
    g_hash_table_remove (buffers, tb->name);
    tb->name = lname;
    g_hash_table_insert (buffers, tb->name, tb);
  }
}

static gboolean
edit_delete_real (GtkWidget *widget,
		  gpointer   data)
{
  /* If you return FALSE in the "delete-event" signal handler,
   * GTK will emit the "destroy" signal. Returning TRUE means
   * you don't want the window to be destroyed.
   * This is useful for popping up 'are you sure you want to quit?'
   * type dialogs. */

  /* Change TRUE to FALSE and the main window will be destroyed with
   * a "delete-event". */
  
  window_s *tw = data;
  buffer_s *tb = buffer (tw);
  gboolean modified = gtk_text_buffer_get_modified (tb->buffer);
  if (modified) {
    GtkWidget *e_dialog;
    gint response;
    e_dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_NONE,
				       _ ("Buffer %s modified.  Save it?"),
				       tb->name);
    gtk_dialog_add_buttons (GTK_DIALOG (e_dialog),
			    _("Yes"),    GTK_RESPONSE_YES,
			    _("No"),     GTK_RESPONSE_NO,
			    _("Cancel"), GTK_RESPONSE_CANCEL,
			    NULL);
    gtk_window_set_keep_above (GTK_WINDOW (e_dialog), TRUE);
    gtk_window_set_position (GTK_WINDOW (e_dialog), GTK_WIN_POS_MOUSE);
    response = gtk_dialog_run (GTK_DIALOG (e_dialog));
    gtk_widget_destroy (e_dialog);
    switch(response) {
    case GTK_RESPONSE_CANCEL:
      return TRUE;			// abort close
      break;
    case GTK_RESPONSE_NO:
      return FALSE;			// close without saving
      break;
    case GTK_RESPONSE_YES:
      if (path (tw))
        edit_save_file (NULL, tw);
      else {
        cb_done (tw) = FALSE;
        edit_save_object (NULL, tw);
        while (!cb_done (tw)) {
          while (gtk_events_pending ()) gtk_main_iteration ();
          g_usleep(10000);
        }
        if (error (tw)) return TRUE;	// abort close if APL error
      }
      return FALSE;			// close
      break;
    }
  }
  return FALSE;			// close
}

/***
    this is called as a result of a "destroy" signal to the window.
    a return FALSE will automatically result in a real closure
 ***/
static gboolean
edit_delete_event (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   data)
{
  gboolean rc = edit_delete_real (widget, data);
  if (!rc) edit_close (widget, data);
  return rc;
}

/***
    this is called as a result of pressing the close button in the
    file pulldown.
 ***/
static void
edit_delete (GtkWidget *widget,
	     gpointer   data)
{
  gboolean rc = edit_delete_real (widget, data);
  if (!rc) edit_close (widget, data);
}

static void
build_edit_menubar (GtkWidget *vbox, window_s *tw)
{
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *item;

  buffer_s *tb = buffer (tw);

  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window (tw)), accel_group);

  menubar = gtk_menu_bar_new();

  menu = gtk_menu_new();

  item = gtk_menu_item_new_with_label (_ ("File"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

  add_menu_item (_ ("_New"), GDK_KEY_n, accel_group,
                 G_CALLBACK (new_object), NULL, menu);

  add_menu_item (_ ("_Open Object"), GDK_KEY_o, accel_group,
                 G_CALLBACK (open_object), NULL, menu);

  add_menu_item (_ ("Open F_ile"), GDK_KEY_i, accel_group,
                 G_CALLBACK (import_file), NULL, menu);

  if (!path (tw)) {
    /* Menu items for object clone/save/export */
    add_menu_item (_ ("C_lone"), -1, NULL,
                   G_CALLBACK (clone_object), tw, menu);

    add_menu_item (_ ("_Save"), GDK_KEY_s, accel_group,
                   G_CALLBACK (edit_save_object), tw, menu);

    add_menu_item (_ ("_Export File"), -1, NULL,
                   G_CALLBACK (save_log), tb->buffer, menu);

    add_menu_item (_ ("Export File As"), -1, NULL,
                   G_CALLBACK (save_log_as), tb->buffer, menu);
  }
  else {
    /* Menu items for file clone/save */
    add_menu_item (_ ("C_lone"), -1, NULL,
                   G_CALLBACK (clone_file), tw, menu);

    add_menu_item (_ ("_Save"), GDK_KEY_s, accel_group,
                   G_CALLBACK (edit_save_file), tw, menu);

    add_menu_item (_ ("Save As"), -1, NULL,
                   G_CALLBACK (edit_save_file_as), tw, menu);
  }

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  
  add_menu_item (_ ("_Close"), -1, NULL,
                 G_CALLBACK (edit_delete), tw, menu);

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (menubar), FALSE, FALSE, 2);
}

static gboolean
edit_key_press_event (GtkWidget *widget,
		      GdkEvent  *event,
		      gpointer   user_data)
{
  gboolean rc = FALSE;
  if (event->type != GDK_KEY_PRESS) return FALSE;
  
  window_s *tw = user_data;
  buffer_s *tb = buffer (tw);

  GdkEventKey *key_event = (GdkEventKey *)event;
  
  gsize bw;
  gchar *res = NULL;
  
  if (key_event->state & GDK_MOD1_MASK)
    res = handle_apl_characters (&bw, key_event);

  // if (!(key_event->state & GDK_MOD1_MASK)) return FALSE;

  if (res) {
    gtk_text_buffer_insert_at_cursor (tb->buffer, res, bw);
    g_free (res);
    rc = TRUE;
  }

  set_status_line (tw, tb);

  return rc;
}

static gint
trim_length (const gchar *text) {
  gint r = 0, i = 0;
  while (text[i]) {
    if (! g_ascii_isspace (text[i]))
      r = i;
    ++i;
  }
  return r + 1;
}

static void
edit_function_cb (gchar *text, void *data)
{
  set_send_cb (NULL, NULL);

  window_s *tw = data;
  buffer_s *tb = buffer (tw);
  gchar **lines = g_strsplit (text, "\n", 0);
  for (int i = 1; lines[i]; i++) {
    if (!g_strcmp0 (lines[i], END_TAG)) break;
    gtk_text_buffer_insert_at_cursor (tb->buffer,
                                      lines[i],
                                      trim_length (lines[i]));
    gtk_text_buffer_insert_at_cursor (tb->buffer, "\n", -1);
  }
  g_strfreev (lines);
  gtk_text_buffer_set_modified (tb->buffer, FALSE);
  GtkTextIter start_iter;
  gtk_text_buffer_get_start_iter (tb->buffer, &start_iter);
  gtk_text_buffer_place_cursor (tb->buffer, &start_iter);
}

static void
edit_variable_cb (gchar *text, void *data)
{
  set_send_cb (NULL, NULL);
  g_print ("vbl: %s\n", text);
#if 0
  window_s *tw = data;
  gchar **lines = g_strsplit (text, "\n", 0);
  for (int i = 1; lines[i]; i++) {
    if (!g_strcmp0 (lines[i], END_TAG)) break;
    gtk_text_buffer_insert_at_cursor (edit_buffer, lines[i], -1);
    gtk_text_buffer_insert_at_cursor (edit_buffer, "\n", -1);
  }
  g_strfreev (lines);
#endif
}

void
edit_object (gchar* name, gint nc)
{
  GtkWidget *vbox;
  GtkWidget *scroll;
  GtkWidget *view;
  PangoFontDescription *desc = NULL;
  window_s *this_window = NULL;
  buffer_s *this_buffer = NULL;
  GtkWidget *window;

  if (!buffers) buffers = g_hash_table_new (g_str_hash, g_str_equal);

  gchar *lname =
    name ? g_strdup (name) : g_strdup_printf ("Unnamed_%d", seq_nr++);

  this_window = g_malloc0 (sizeof(window_s));
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  window (this_window) = window;
  gtk_window_set_title (GTK_WINDOW (window), name ? : "Unnamed");
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);

  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  
  desc =
    pango_font_description_from_string (vwidth ? "UnifontMedium" : "FreeMono");
  pango_font_description_set_size (desc, ft_size * PANGO_SCALE);

  scroll = gtk_scrolled_window_new (NULL, NULL);

  if (g_hash_table_contains (buffers, lname)) {
    this_buffer = g_hash_table_lookup (buffers, lname);
    this_buffer->ref_count++;
  }
  
  if (!this_buffer) {
    this_buffer = g_malloc (sizeof(buffer_s));
    this_buffer->buffer = gtk_text_buffer_new (NULL);
    this_buffer->name = lname;
    this_buffer->ref_count = 1;
    this_buffer->nc = name ? nc : NC_FUNCTION; // fixme
    g_hash_table_insert (buffers, this_buffer->name, this_buffer);

    if (name) {
      if (nc == NC_FUNCTION) {
	set_send_cb (edit_function_cb, this_window);
	gchar *cmd = g_strdup_printf ("fn:%s\n", name);
	send_apl (cmd, strlen(cmd));
	g_free (cmd);
      }
      else if (nc == NC_VARIABLE) {
	set_send_cb (edit_variable_cb, this_window);
	gchar *cmd = g_strdup_printf ("getvar:%s\n", name);
	send_apl (cmd, strlen(cmd));
	g_free (cmd);
      }
    }
  }

  buffer (this_window) = this_buffer;

  build_edit_menubar (vbox, this_window);

  g_signal_connect (window, "delete-event",
		      G_CALLBACK (edit_delete_event), this_window);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (edit_close), this_window);
  
  view = gtk_text_view_new_with_buffer (this_buffer->buffer);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 8);

  g_signal_connect (view, "key-press-event",
		    G_CALLBACK (edit_key_press_event), this_window);

  g_signal_connect_after (this_buffer->buffer, "mark-set",
		    G_CALLBACK (edit_mark_set_event), this_window);

  if (desc) gtk_widget_override_font (view, desc);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scroll), TRUE, TRUE, 2);
  
  status (this_window) = gtk_label_new ("status");
  gtk_misc_set_alignment (GTK_MISC (status (this_window)), 0.0, 0.0);
  set_status_line (this_window, this_buffer);

  gtk_box_pack_start (GTK_BOX (vbox), status (this_window), FALSE, FALSE, 2);
  
  gtk_widget_show_all (window);
}

/* FIX: edit_file() started life as a copy of edit_object(). Factor
   out the common code. */
void
edit_file (gchar *path)
{
  GtkWidget *vbox;
  GtkWidget *scroll;
  GtkWidget *view;
  PangoFontDescription *desc = NULL;
  window_s *this_window = NULL;
  buffer_s *this_buffer = NULL;
  GtkWidget *window;

  if (!buffers) buffers = g_hash_table_new (g_str_hash, g_str_equal);

  gchar *name = g_path_get_basename (path);
  gchar *lname =
    name ? g_strdup (name) : g_strdup_printf ("Unnamed_%d", seq_nr++);

  this_window = g_malloc0 (sizeof(window_s));
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  window (this_window) = window;
  path (this_window) = path;
  gtk_window_set_title (GTK_WINDOW (window), name ? : "Unnamed");
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);

  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  
  desc =
    pango_font_description_from_string (vwidth ? "UnifontMedium" : "FreeMono");
  pango_font_description_set_size (desc, ft_size * PANGO_SCALE);

  scroll = gtk_scrolled_window_new (NULL, NULL);

  if (g_hash_table_contains (buffers, lname)) {
    this_buffer = g_hash_table_lookup (buffers, lname);
    this_buffer->ref_count++;
  }
  
  if (!this_buffer) {
    this_buffer = g_malloc (sizeof(buffer_s));
    this_buffer->buffer = gtk_text_buffer_new (NULL);
    this_buffer->name = lname;
    this_buffer->ref_count = 1;
    g_hash_table_insert (buffers, this_buffer->name, this_buffer);

    gchar *text;
    GError *error = NULL;
    if (g_file_get_contents (path, &text, NULL, &error)) {
      gtk_text_buffer_set_text (this_buffer->buffer, text, -1);
      g_free (text);
      gtk_text_buffer_set_modified (this_buffer->buffer, FALSE);
      GtkTextIter start_iter;
      gtk_text_buffer_get_start_iter (this_buffer->buffer, &start_iter);
      gtk_text_buffer_place_cursor (this_buffer->buffer, &start_iter);
    }
    else {
      message_dialog (GTK_MESSAGE_WARNING, _ ("File error"), error->message);
      g_error_free (error);
      g_free (name);
      g_free (lname);
      g_free (this_window);
      gtk_widget_destroy (window);
      pango_font_description_free (desc);
      gtk_widget_destroy (scroll);
      return;
    }
  }

  buffer (this_window) = this_buffer;

  build_edit_menubar (vbox, this_window);

  g_signal_connect (window, "delete-event",
		      G_CALLBACK (edit_delete_event), this_window);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (edit_close), this_window);
  
  view = gtk_text_view_new_with_buffer (this_buffer->buffer);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 8);

  g_signal_connect (view, "key-press-event",
		    G_CALLBACK (edit_key_press_event), this_window);

  g_signal_connect_after (this_buffer->buffer, "mark-set",
		    G_CALLBACK (edit_mark_set_event), this_window);

  if (desc) gtk_widget_override_font (view, desc);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scroll), TRUE, TRUE, 2);
  
  status (this_window) = gtk_label_new ("status");
  gtk_misc_set_alignment (GTK_MISC (status (this_window)), 0.0, 0.0);
  set_status_line (this_window, this_buffer);

  gtk_box_pack_start (GTK_BOX (vbox), status (this_window), FALSE, FALSE, 2);
  
  gtk_widget_show_all (window);
}
