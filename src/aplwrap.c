#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include "Avec.h"
#include "keymap.h"

#include "txtbuf.h"
#include "history.h"
#include "aplio.h"
#include "options.h"
#include "apl.h"
#include "menu.h"
#include "complete.h"

static GtkWidget *window;
static GtkWidget *scroll;
static GtkWidget *view;
static PangoFontDescription *desc = NULL;

static gint rows_old = 0, rows_new;

gchar *
get_rows_assign ()
{
  static gchar expr[80];
  if (rows_new && rows_v) {
    snprintf(expr, sizeof(expr), "%.60s←%.2d", rows_v, rows_new);
    rows_new = 0;
    return expr;
  }
  else
    return NULL;
}

static gboolean
aplwrap_configure_window (GdkWindow *event_window,
                          GdkEvent *event,
                          gpointer data)
{
  gint ypos, line_height, rows;
  GtkTextIter first;
  GdkRectangle view_rect;
  
  if (GTK_WIDGET (event_window) == window) {
    gtk_text_buffer_get_start_iter (buffer, &first);
    gtk_text_view_get_line_yrange (GTK_TEXT_VIEW (view),
                                   &first, &ypos, &line_height);
    line_height += gtk_text_view_get_pixels_above_lines (GTK_TEXT_VIEW (view));
    line_height += gtk_text_view_get_pixels_below_lines (GTK_TEXT_VIEW (view));
    gtk_text_view_get_visible_rect (GTK_TEXT_VIEW (view), &view_rect);
    rows = view_rect.height / line_height;
    if ( rows != rows_old )
      rows_old = rows_new = rows;
    if (is_at_prompt ()) {
      gchar *rows_assign = get_rows_assign ();
      if (rows_assign)
        apl_eval (rows_assign, -1, NULL, NULL);
    }
  }

  return FALSE;
}

void
beep ()
{
  gdk_window_beep (gtk_widget_get_window (window));
}

gchar *
handle_apl_characters (gsize *bw_p, GdkEventKey *key_event)
{
  guint16 kc = key_event->hardware_keycode;
  gchar *res = NULL;
  if (kc < keymap_count) {
    CHT_Index ix = (key_event->state & GDK_SHIFT_MASK)
      ? key_shift_alt (kc) :  key_alt (kc);
    if (ix) {
      gshort uic = char_unicode (ix);
      gsize br, bw = 0;
      res = g_convert ((const gchar *)(&uic),
		       sizeof(gshort),
		       "utf-8",
		       "unicode",
		       &br,
		       &bw,
		       NULL);
      if (res && bw_p) *bw_p = bw;
    }
  }
  return res;
}

static gboolean
key_press_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
  if (event->type != GDK_KEY_PRESS) return FALSE;

  GdkEventKey *key_event = (GdkEventKey *)event;

  /* Tab key runs completion */
  if (key_event->keyval == GDK_KEY_Tab) {
    complete();
    return TRUE;
  }

  /* Esc key moves cursor to end of completion */
  if (key_event->keyval == GDK_KEY_Escape) {
    cursor_to_completion_end ();
    return TRUE;
  }

  /* Handle APL interrupt keys */
  if (key_event->state == GDK_CONTROL_MASK &&
      (key_event->keyval == GDK_KEY_Super_L ||
       key_event->keyval == GDK_KEY_Break)) {
    apl_interrupt ();
    return TRUE;
  }

  /* Handle Return and Enter keys */
  if (key_event->keyval == GDK_KEY_Return ||
      key_event->keyval == GDK_KEY_KP_Enter) {
    GtkTextIter end_iter;
    gchar *text;
    gint sz;

    if (handle_copy_down ()) return TRUE;
 
    gtk_text_buffer_get_end_iter (buffer, &end_iter);
    gtk_text_buffer_place_cursor (buffer, &end_iter);

    text = get_input_text(&sz);

    if (is_at_prompt ())
      history_insert(text+6, sz-6);
    apl_send_inp (text, sz);

    gtk_text_buffer_insert_at_cursor (buffer, "\n", 1);
    g_free (text);
    return TRUE;
  }

  /* Special handling of Home key in input area */
  if (key_event->keyval == GDK_KEY_Home &&
      !(key_event->state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK|GDK_MOD1_MASK))) {
    if (cursor_in_input_area ()) {
      GtkTextIter line_iter;
      GtkTextMark *insert = gtk_text_buffer_get_insert (buffer);
      gtk_text_buffer_get_iter_at_mark (buffer, &line_iter, insert);
      gtk_text_iter_set_line_offset (&line_iter, 6);
      gtk_text_buffer_place_cursor (buffer, &line_iter);
      return TRUE;
    }
    else
      return FALSE;
  }

  /* All remaining processing is for keys having Alt modifier */
  if (!(key_event->state & GDK_MOD1_MASK)) return FALSE; 

  /* Command history */
  if (key_event->keyval == GDK_KEY_Up) {
    handle_history_replacement(history_prev());
    return TRUE;
  }

  if (key_event->keyval == GDK_KEY_Down) {
    handle_history_replacement(history_next());
    return TRUE;
  }

  /* APL characters */
  gsize bw;
  gchar *res = handle_apl_characters (&bw, key_event);
  if (res) {
    gtk_text_buffer_insert_at_cursor (buffer, res, bw);
    g_free (res);
    return TRUE;
  }

  return FALSE;				// pass the event on
}

void
scroll_to_end ()
{
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				gtk_text_buffer_get_mark (buffer, "insert"),
				0.1,
				FALSE,
				0.5,
				1.0);
}

static int
have_window_name (char *argv[])
{
  while (*argv) {
    if (!(strcmp ("--name", *argv))) return 1;
    ++argv;
  }
  return 0;
}

int
main (int   argc,
      char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  GtkWidget *vbox;
  int override_name;

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
    
  override_name = !have_window_name (argv);
  gtk_init (&argc, &argv);
  
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_warning ("option parsing failed: %s\n", error->message);
    g_clear_error (&error);
  }

  apl_expect_network = FALSE;
  if (apl_spawn(argc, argv)) return 1;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (override_name) gtk_window_set_title (GTK_WINDOW (window), "APLwrap");
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);
    
  g_signal_connect (window, "destroy",
		    G_CALLBACK (aplwrap_quit), NULL);
    
  g_signal_connect (window, "configure-event",
		    G_CALLBACK (aplwrap_configure_window), NULL);
    
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  build_menubar (vbox);

  desc =
    pango_font_description_from_string (vwidth ? "UnifontMedium" : "FreeMono");
  pango_font_description_set_size (desc, ft_size * PANGO_SCALE);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  view = gtk_text_view_new ();
  gtk_container_set_border_width (GTK_CONTAINER (view), 4);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  g_signal_connect (view, "key-press-event",
		    G_CALLBACK (key_press_event), NULL);
  if (desc) gtk_widget_override_font (view, desc);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scroll), TRUE, TRUE, 2);
  
  define_tags ();

  gtk_widget_show_all (window);
  gtk_main ();
    
  return 0;
}
