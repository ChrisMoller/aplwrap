#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


#include "Avec.h"
#include "keymap.h"

#include "txtbuf.h"
#include "history.h"
#include "aplio.h"
#include "options.h"
#include "apl.h"
#include "aplwrap.h"
#include "menu.h"
#include "complete.h"
#include "resources.h"
#include "search.h"

static GtkWidget *window;
static GtkWidget *scroll;
static GtkWidget *view;
static GtkCssProvider *provider;

gchar *plot_pipe_name = NULL;
gint   plot_pipe_fd   = -1;

char *flag_file = NULL;
char *flag_dir = NULL;

void
load_css_provider ()
{
#define CSS_STRING "* { font: %s; font-size: %dpx; background-color: %s; color: %s;}"
  gchar *css = g_strdup_printf (CSS_STRING,
				vwidth ? "UnifontMedium" : "FreeMono",
				ft_size, bg_colour, fg_colour);
  gtk_css_provider_load_from_data (provider, css, -1, NULL);
  g_free (css);
}

GtkWindow *
get_top_window ()
{
  return GTK_WINDOW (window);
}

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
aplwrap_count_rows (GdkWindow *event_window,
                    GdkEvent *event,
                    gpointer data)
{
  gint ypos, line_height, rows;
  GtkTextIter first;
  
  if (GTK_WIDGET (event_window) == window) {
    gtk_text_buffer_get_start_iter (buffer, &first);
    gtk_text_view_get_line_yrange (GTK_TEXT_VIEW (view),
                                   &first, &ypos, &line_height);
    line_height += gtk_text_view_get_pixels_above_lines (GTK_TEXT_VIEW (view));
    line_height += gtk_text_view_get_pixels_below_lines (GTK_TEXT_VIEW (view));
    rows = (gtk_widget_get_allocated_height (GTK_WIDGET (view))
            - gtk_container_get_border_width (GTK_CONTAINER (view)) * 2)
      / line_height;
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

void
scroll_to_cursor ()
{
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				gtk_text_buffer_get_mark (buffer, "insert"),
				0.1,
				FALSE,
				0.0,
				0.0);
}

static void
send_input_area_to_apl ()
{
  GtkTextIter end_iter;
  gchar *text;
  gint sz;

  gtk_text_buffer_get_end_iter (buffer, &end_iter);
  gtk_text_buffer_place_cursor (buffer, &end_iter);

  text = get_input_text(&sz);

  if (sz >= 6 && is_at_prompt ())
    history_insert(text+6, sz-6);
  apl_send_inp (text, sz);

  tagged_insert ("\n", 1, TAG_OUT);
  g_free (text);
}

static gboolean
maybe_copy_selected_text_without_tags ()
{
  GtkClipboard *clipboard = gtk_widget_get_clipboard ( GTK_WIDGET (view),
                                                       GDK_SELECTION_CLIPBOARD);
  GtkTextIter start, end;
  gchar *selection;
  if (gtk_text_buffer_get_selection_bounds (buffer, &start, &end)) {
    if (gtk_text_iter_can_insert (&start, TRUE)) return FALSE;
    selection = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
    gtk_clipboard_set_text (clipboard, selection, strlen(selection));
    g_free (selection);
  }
  return TRUE;
}

static void
home_to_end_of_apl_prompt (gboolean select)
{
  GtkTextIter line_iter, start_iter;
  GtkTextMark *insert = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &line_iter, insert);
  start_iter = line_iter;
  gtk_text_iter_set_line_offset (&line_iter, 0);
  while (gtk_text_iter_has_tag (&line_iter, get_tag (TAG_PRM)))
    gtk_text_iter_forward_char (&line_iter);
  gtk_text_buffer_place_cursor (buffer, &line_iter);
  if (select) gtk_text_buffer_select_range (buffer, &line_iter, &start_iter);
  scroll_to_cursor ();
}

static void
advance_by_apl_prompt (gboolean forward)
{
  GtkTextIter line_iter;
  GtkTextMark *insert = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &line_iter, insert);
  gtk_text_iter_set_line_offset (&line_iter, 0);
  gboolean (*move_fn)(GtkTextIter*) =
    forward ? gtk_text_iter_forward_line : gtk_text_iter_backward_line;
  while ((*move_fn) (&line_iter)
         && (gtk_text_iter_ends_line (&line_iter) /* i.e. empty line */
             || !gtk_text_iter_has_tag (&line_iter, get_tag (TAG_PRM))))
    ;
  if (gtk_text_iter_has_tag (&line_iter, get_tag (TAG_PRM))) {
    gtk_text_buffer_place_cursor (buffer, &line_iter);
    home_to_end_of_apl_prompt (FALSE);
  }
}

static void
delete_uncommitted_input ()
{
  if (is_at_prompt () &&
      cursor_in_input_area ()) {
    GtkTextIter start_iter, end_iter;
    GtkTextMark *insert = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &start_iter, insert);
    gtk_text_iter_set_line_offset (&start_iter, 0);
    while (gtk_text_iter_has_tag (&start_iter, get_tag (TAG_PRM)))
      gtk_text_iter_forward_char (&start_iter);
    gtk_text_buffer_place_cursor (buffer, &start_iter);
    gtk_text_buffer_get_end_iter (buffer, &end_iter);
    gtk_text_buffer_delete (buffer, &start_iter, &end_iter);
  }
}


static gboolean
key_press_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
  if (event->type != GDK_KEY_PRESS) return FALSE;

  GdkEventKey *key_event = (GdkEventKey *)event;
  GdkModifierType mod_mask = gtk_accelerator_get_default_mod_mask ();
#if ENABLE_SEARCH
  search_context_t *cxt = user_data;

  /* Control-F enables search mode */
  if (key_event->keyval == GDK_KEY_f &&
      (key_event->state & mod_mask) == GDK_CONTROL_MASK) {
    gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (cxt->search_bar), TRUE);
    return TRUE;
  }
#endif

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
  if ((key_event->state & mod_mask) == GDK_CONTROL_MASK &&
      (key_event->keyval == GDK_KEY_Super_L ||
       key_event->keyval == GDK_KEY_Break)) {
    apl_interrupt ();
    return TRUE;
  }

  /* Handle Return and Enter keys */
  if (key_event->keyval == GDK_KEY_Return ||
      key_event->keyval == GDK_KEY_KP_Enter) {
    if (handle_copy_down ()) return TRUE;
    send_input_area_to_apl ();
    return TRUE;
  }

  /* Special handling of Home key in input area */
  if (cursor_in_input_area ()) {
    if (key_event->keyval == GDK_KEY_Home &&
        !(key_event->state & (GDK_CONTROL_MASK|GDK_MOD1_MASK))) {
      home_to_end_of_apl_prompt (!!key_event->state & GDK_SHIFT_MASK);
      return TRUE;
    }
  }

  /* Cause Home and End keys to always scroll the view, even when the
     cursor is already at the limit. */
  if (key_event->keyval == GDK_KEY_End || key_event->keyval == GDK_KEY_Home) {
    scroll_to_cursor ();
    return FALSE; /* GTK+ moves the cursor. */
  }

  /* Override copy and cut behaviors to not copy tags from transcript. */
  {
    guint key = gdk_keyval_to_lower (key_event->keyval);
    if ((key == GDK_KEY_c || key == GDK_KEY_x) &&
        (key_event->state & mod_mask) == GDK_CONTROL_MASK)
      return maybe_copy_selected_text_without_tags ();
  }

  /* Command history */
  if (key_event->keyval == GDK_KEY_Up) {
    handle_history_replacement(history_prev());
    return TRUE;
  }

  if (key_event->keyval == GDK_KEY_Down) {
    handle_history_replacement(history_next());
    return TRUE;
  }

  /* All remaining processing is for keys having Alt modifier */
  if (!(key_event->state & GDK_MOD1_MASK)) return FALSE; 

  /* Advance by prompt through transcript */
  if (key_event->keyval == GDK_KEY_Page_Up) {
    advance_by_apl_prompt (FALSE);
    return TRUE;
  }

  if (key_event->keyval == GDK_KEY_Page_Down) {
    advance_by_apl_prompt (TRUE);
    return TRUE;
  }

  /* Delete uncommitted input */
  if (key_event->keyval == GDK_KEY_Delete) {
    delete_uncommitted_input ();
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

static gboolean
button_press_event (GtkWidget *widget,
                    GdkEvent  *event,
                    gpointer   user_data)
{
  GdkEventButton *button_event = (GdkEventButton *)event;

  if (button_event->type == GDK_BUTTON_PRESS && button_event->button == 2) {
    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
    if (clipboard) {
      gchar *text = gtk_clipboard_wait_for_text (clipboard);
      if (text) {
        gint x, y, trailing;
        GtkTextIter insert;
        gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (view),
                                               GTK_TEXT_WINDOW_WIDGET,
                                               (gint)button_event->x,
                                               (gint)button_event->y,
                                               &x,
                                               &y);
        x += gtk_container_get_border_width (GTK_CONTAINER (view));
        gtk_text_view_get_iter_at_position (GTK_TEXT_VIEW (view),
                                            &insert,
                                            &trailing,
                                            x,
                                            y);
        gtk_text_iter_forward_chars (&insert, trailing);
        gtk_text_buffer_place_cursor (buffer, &insert);
        if (gtk_text_iter_can_insert (&insert, TRUE))
          gtk_text_buffer_insert_at_cursor (buffer, text, -1);
        g_free (text);
      }
      return TRUE;
    }
  }

  return FALSE;				// pass the event on
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

static GtkWidget *status;

void
set_status_visibility (gboolean show)
{
  if (show) gtk_widget_show (status);
  else gtk_widget_hide (status);
}

void
update_status_line (gchar *text)
{
  gtk_label_set_text (GTK_LABEL (status), text);
  while (gtk_events_pending ()) gtk_main_iteration ();
}

int asprintf(char **strp, const char *fmt, ...);

int
main (int   argc,
      char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  GtkWidget *vbox;
  int override_name;

  asprintf (&flag_dir, "/var/run/user/%d/%d",
	    (int)getuid (), (int)getpid ());
  mkdir (flag_dir, 0777);
  asprintf (&flag_file, "%s/aplwrap", flag_dir);
  creat (flag_file, 0666);

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
    
  override_name = !have_window_name (argv);
  gtk_init (&argc, &argv);

  restore_resources ();
  
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_warning ("option parsing failed: %s\n", error->message);
    g_clear_error (&error);
  }

  if (printversion) {
    g_print ("aplwrap version %s\n", VERSION);
    exit (0);
  }

  plot_pipe_name = g_strdup_printf ("/tmp/aplwrap-%d", (int)getpid ());
  plot_pipe_fd = (0 == mkfifo (plot_pipe_name, 0600)) ?
    // O_CLOEXEC seems to be a GNU extension
    open (plot_pipe_name, O_RDONLY /* | O_CLOEXEC */ | O_NONBLOCK) : -1;
  

  apl_expect_network = FALSE;
  if (apl_spawn(argc, argv)) return 1;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (override_name) gtk_window_set_title (GTK_WINDOW (window), _ (PGM_TITLE));
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);
    
  g_signal_connect (window, "destroy",
		    G_CALLBACK (aplwrap_quit), NULL);
    
  g_signal_connect (window, "size-allocate",
                    G_CALLBACK (aplwrap_count_rows), NULL);
    
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  build_menubar (vbox);
  
  scroll = gtk_scrolled_window_new (NULL, NULL);
  view = gtk_text_view_new ();
  gtk_container_set_border_width (GTK_CONTAINER (view), 4);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  provider = gtk_css_provider_new ();
  if (!bg_colour) bg_colour = g_strdup (BG_COLOUR_FALLBACK);
  if (!fg_colour) fg_colour = g_strdup (FG_COLOUR_FALLBACK);

  load_css_provider ();
  gtk_style_context_add_provider (gtk_widget_get_style_context (view),
				  GTK_STYLE_PROVIDER (provider),
				  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
  g_signal_connect (view, "button-press-event",
		    G_CALLBACK (button_press_event), NULL);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  
#if ENABLE_SEARCH
  GtkWidget *search_bar = gtk_search_bar_new ();
  gtk_search_bar_set_show_close_button (GTK_SEARCH_BAR (search_bar), TRUE);
  search_context_t *search_cxt = new_search_context (search_bar, view, buffer);
  GtkWidget *search_entry = gtk_search_entry_new ();
  g_signal_connect (search_entry, "search-changed",
                    G_CALLBACK (search_changed_event), search_cxt);
  gtk_container_add (GTK_CONTAINER (search_bar), search_entry );
  gtk_box_pack_start (GTK_BOX (vbox), search_bar, FALSE, FALSE, 0);

  g_signal_connect (search_entry, "key-press-event",
                    G_CALLBACK (search_key_press_event), search_cxt);
  g_signal_connect (view, "key-press-event",
		    G_CALLBACK (key_press_event), search_cxt);
#else
  g_signal_connect (view, "key-press-event",
		    G_CALLBACK (key_press_event), NULL);
#endif

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scroll), TRUE, TRUE, 2);

  status = gtk_label_new ("status");
  gtk_label_set_ellipsize (GTK_LABEL(status), PANGO_ELLIPSIZE_END);
  gtk_label_set_selectable (GTK_LABEL(status), TRUE);
  gtk_widget_set_halign (status, GTK_ALIGN_START);
  update_status_line ("Starting…");
  gtk_box_pack_start (GTK_BOX (vbox), status, FALSE, FALSE, 2);

  define_tags ();
  init_shortcuts ();

  gtk_widget_show_all (window);
  set_status_visibility (show_status);
  
  gtk_main ();
    
  return 0;
}
