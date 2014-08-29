#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <sys/socket.h>

#include "apl.h"
#include "aplio.h"
#include "aplwrap.h"
#include "txtbuf.h"
#include "options.h"
#include "edit.h"

static GHashTable *windows = NULL;

static void
edit_close (GtkWidget *widget,
	    gpointer  data)
{
  //  window_s *tw = data;
  // fixme check for mods
  gtk_widget_destroy(widget);
#if 0
  g_print ("destroying %p\n", tw->window);
  if (tw->window) gtk_widget_destroy(tw->window);
  tw->window = NULL;
#endif
}

static void
edit_save_cb (gchar *text, void *tw)
{
  set_socket_cb (NULL, NULL);
  gchar **lines = g_strsplit (text, "\n", 0);
  for (int i = 0; lines[i]; i++) {
    if (!*lines[i]) continue;
    if (!g_strcmp0 (lines[i], END_TAG)) break;
    tagged_insert (lines[i], -1, TAG_OUT);
    tagged_insert (" ", -1, TAG_OUT);
  }
  g_strfreev (lines);
  tagged_insert ("\n      ", -1, TAG_OUT);
}

static void
edit_save (GtkWidget *widget,
	   gpointer  data)
{
  window_s *tw = data;
  GtkTextIter start_iter, end_iter;
  gtk_text_buffer_get_bounds (tw->buffer, &start_iter, &end_iter);
  gchar *text =
    gtk_text_buffer_get_text (tw->buffer, &start_iter, &end_iter, FALSE);
  if (text[strlen (text) - 1] != '\n') {
    gtk_text_buffer_insert (tw->buffer, &end_iter, "\n", 1);
    gtk_text_buffer_get_bounds (tw->buffer, &start_iter, &end_iter);
    text =
      gtk_text_buffer_get_text (tw->buffer, &start_iter, &end_iter, FALSE);
  }
  
  set_socket_cb (edit_save_cb, tw);
  send (sockfd, "def\n", strlen("def\n"), 0);
  gchar *ptr     = text;
  while (ptr && *ptr) {
    gchar *end_ptr = strchr (ptr, '\n');
    if (end_ptr++) {
      send (sockfd, ptr, end_ptr - ptr, 0);
      ptr = end_ptr;
    }
  }
  send (sockfd, END_TAGNL, strlen(END_TAGNL), 0);
}

static void
build_edit_menubar (GtkWidget *vbox, window_s *tw)
{
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *item;

  menubar = gtk_menu_bar_new();

  menu = gtk_menu_new();

  item = gtk_menu_item_new_with_label (_ ("File"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

  item = gtk_menu_item_new_with_label(_ ("New"));
  //  g_signal_connect (G_OBJECT (item), "activate",
  //                   G_CALLBACK (new_object), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Open"));
  //  g_signal_connect(G_OBJECT (item), "activate",
  //		   G_CALLBACK (open_object), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Clone"));
  //  g_signal_connect(G_OBJECT (item), "activate",
  //		   G_CALLBACK (open_object), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Save"));
  g_signal_connect(G_OBJECT (item), "activate",
		   G_CALLBACK (edit_save), tw);
  gtk_menu_shell_append(GTK_MENU_SHELL (menu), item);
  
  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  
  item = gtk_menu_item_new_with_label (_ ("Close"));
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (edit_close), tw);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (menubar), FALSE, FALSE, 2);
}

static gboolean
edit_key_press_event (GtkWidget *widget,
		      GdkEvent  *event,
		      gpointer   user_data)
{
  if (event->type != GDK_KEY_PRESS) return FALSE;

  GdkEventKey *key_event = (GdkEventKey *)event;

  if (!(key_event->state & GDK_MOD1_MASK)) return FALSE;

  gsize bw;
  window_s *tw = user_data;
  gchar *res = handle_apl_characters (&bw, key_event);
  if (res) {
    gtk_text_buffer_insert_at_cursor (tw->buffer, res, bw);
    g_free (res);
    return TRUE;
  }

  return FALSE;				// pass the event on
}

static void
edit_function_cb (gchar *text, void *data)
{
  set_socket_cb (NULL, NULL);
  window_s *tw = data;
  gchar **lines = g_strsplit (text, "\n", 0);
  for (int i = 1; lines[i]; i++) {
    if (!g_strcmp0 (lines[i], END_TAG)) break;
    gtk_text_buffer_insert_at_cursor (tw->buffer, lines[i], -1);
    gtk_text_buffer_insert_at_cursor (tw->buffer, "\n", -1);
  }
  g_strfreev (lines);
}

static void
edit_variable_cb (gchar *text, void *data)
{
  set_socket_cb (NULL, NULL);
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
  GtkWidget *window;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), name ? : "Unnamed");
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);

  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  
  desc =
    pango_font_description_from_string (vwidth ? "UnifontMedium" : "FreeMono");
  pango_font_description_set_size (desc, ft_size * PANGO_SCALE);

  scroll = gtk_scrolled_window_new (NULL, NULL);

  if (!windows) windows = g_hash_table_new (g_str_hash, g_str_equal);

  if (name && g_hash_table_contains (windows, name))
    this_window = g_hash_table_lookup (windows, name);
  
  if (!this_window) {
    this_window = g_malloc (sizeof(window_s));
    this_window->buffer = gtk_text_buffer_new (NULL);
    //    this_window->window = window;
    if (name) {
      g_hash_table_insert (windows, g_strdup (name), this_window);
      
      if (nc == NC_FUNCTION) {
	set_socket_cb (edit_function_cb, this_window);
	gchar *cmd = g_strdup_printf ("fn:%s\n", name);
	if (send(sockfd, cmd, strlen(cmd), 0) < 0) {
	  perror("Error in send()");	// fixme
	}
	g_free (cmd);
      }
      else if (nc == NC_VARIABLE) {
	set_socket_cb (edit_variable_cb, this_window);
	gchar *cmd = g_strdup_printf ("getvar:%s\n", name);
	if (send(sockfd, cmd, strlen(cmd), 0) < 0) {
	  perror("Error in send()");	// fixme
	}
	g_free (cmd);
      }
    }
  }

  build_edit_menubar (vbox, this_window);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (edit_close), this_window);
  
  view = gtk_text_view_new_with_buffer (this_window->buffer);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 8);

  g_signal_connect (view, "key-press-event",
		    G_CALLBACK (edit_key_press_event), this_window);
  if (desc) gtk_widget_override_font (view, desc);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scroll), TRUE, TRUE, 2);
  gtk_widget_show_all (window);
}
