#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <glib-unix.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include "Avec.h"
#include "keymap.h"

#include <signal.h>
int kill(pid_t pid, int sig);	// compiler issue

GtkTextBuffer *buffer = NULL;
GtkWidget *window;
GtkWidget *scroll;
GtkWidget *view;
PangoFontDescription *desc = NULL;

guchar *keymap_buf = NULL;

gint apl_in  = -1;		// to write to apl in
gint apl_out = -1;		// to read from apl out
gint apl_err = -1;		// to read from apl err
GPid apl_pid = -1;

#define XEQ_FALLBACK	"apl"

#define FT_SIZE_FALLBACK	12
static gint ft_size = FT_SIZE_FALLBACK;

#define WIDTH_FALLBACK	680
static gint width = WIDTH_FALLBACK;

#define HEIGHT_FALLBACK	440
static gint height = HEIGHT_FALLBACK;

static gboolean vwidth   = FALSE;
static gboolean nocolour = FALSE;

static GtkTextTag * err_tag;

void
gapl2_quit (GtkWidget *widget,
	     gpointer   data)
{
  if (apl_pid != -1) {
    kill ((pid_t)apl_pid, SIGKILL);
    g_spawn_close_pid (apl_pid);
  }

  if (keymap_buf) g_free (keymap_buf);
  
  gtk_main_quit ();
}

#include "layout.h"

guchar *
decompress_image_data ()
{
  size_t sz =
    GIMP_IMAGE_WIDTH * GIMP_IMAGE_HEIGHT * GIMP_IMAGE_BYTES_PER_PIXEL;
  
  guchar *buf = g_malloc (sz);

  GIMP_IMAGE_RUN_LENGTH_DECODE (buf, GIMP_IMAGE_RLE_PIXEL_DATA, 
			        GIMP_IMAGE_WIDTH * GIMP_IMAGE_HEIGHT,
			        GIMP_IMAGE_BYTES_PER_PIXEL);

  return buf;
}


static void
show_keymap (GtkWidget *widget,
	     gpointer   data)
{
  GtkWidget *dialog;
  GtkWidget *content;
  GtkWidget *km;
  static GdkPixbuf *pb = NULL;

  if (!pb) {
    keymap_buf = decompress_image_data ();
    pb = gdk_pixbuf_new_from_data (keymap_buf,
				   GDK_COLORSPACE_RGB,
				   FALSE,
				   8,
				   GIMP_IMAGE_WIDTH,
				   GIMP_IMAGE_HEIGHT,
				   3 * GIMP_IMAGE_WIDTH,
				   NULL,
				   NULL);
  }

  dialog =  gtk_dialog_new_with_buttons (_ ("Keymap"),
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         _ ("_OK"), GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

  g_signal_connect_swapped (dialog,
			    "response",
			    G_CALLBACK (gtk_widget_destroy),
			    dialog);
  
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  km = gtk_image_new_from_pixbuf (pb);
  gtk_container_add (GTK_CONTAINER (content), km);
  gtk_widget_show_all (dialog);
}

static void
show_about (GtkWidget *widget,
            gpointer   data)
{
  gchar *authors[] = {"C. H. L. Moller", "David Lamkins", NULL};
  gchar *comments = _("aplwrap is at GTK+-based front-end for GNU APL.");

  gtk_show_about_dialog (NULL,
                         "program-name", "aplwrap",
                         "title", _("aplwrap"),
                         "version", "1.0",
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "copyright", "Copyright 2014",
                         "website", "http://moller@mollerware.com",
                         "website-label", "moller@mollerware.com",
                         "authors", authors,
                         "comments", comments,
                         NULL);

}


static void
build_menubar (GtkWidget *vbox)
{
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *item;
  
  menubar = gtk_menu_bar_new();

  /********* file menu ********/

  menu = gtk_menu_new();

  item = gtk_menu_item_new_with_label (_ ("File"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

#if 0
  item = gtk_menu_item_new_with_label(_ ("New"));
  //g_signal_connect (G_OBJECT (item), "activate",
  //                 G_CALLBACK (create_new_drawing), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Open"));
  g_signal_connect(G_OBJECT (item), "activate",
		   G_CALLBACK (open_file), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Save"));
  //  g_signal_connect (G_OBJECT (item), "activate",
  //               G_CALLBACK (save_mods), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Save as"));
  //  g_signal_connect(G_OBJECT(item), "activate",
  //               G_CALLBACK (save_mods), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Print setup"));
  //  g_signal_connect(G_OBJECT(item), "activate",
  //		   G_CALLBACK (print_setup), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
#endif

  item = gtk_menu_item_new_with_label (_ ("Quit"));
  g_signal_connect (G_OBJECT (item), "activate",
                   G_CALLBACK (gapl2_quit), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);


  /********* help menu ********/

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label (_ ("Help"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

  item = gtk_menu_item_new_with_label (_ ("Keymap"));
  g_signal_connect (G_OBJECT(item), "activate",
                    G_CALLBACK (show_keymap), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("About"));
  g_signal_connect (G_OBJECT(item), "activate",
                    G_CALLBACK (show_about), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (menubar), FALSE, FALSE, 2);
}

static gboolean
key_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  if (event->type != GDK_KEY_PRESS) return FALSE;

  ssize_t __attribute__ ((unused)) wrc;
  GdkEventKey *key_event = (GdkEventKey *)event;

#if 0
  // ctrl-break: state == 4, val = 0xff6b = GDK_KEY_Break
  //      pause: state == 0, val = 0xff13 = GDK_KEY_Pause
  g_print ("state = 0x%08x, val = 0x%08x\n",
	   key_event->state,
	   key_event->keyval);
#endif

  if (key_event->state == GDK_CONTROL_MASK &&
      key_event->keyval == GDK_KEY_Break) {
    if (apl_pid != -1) kill ((pid_t)apl_pid, SIGINT);
    return FALSE;
  }

  if ((key_event->state == 0 && key_event->keyval == GDK_KEY_Return) ||
      (key_event->state & GDK_MOD2_MASK
       && key_event->keyval == GDK_KEY_KP_Enter)) {
    GtkTextMark *mark    = gtk_text_buffer_get_insert (buffer);
    GtkTextIter end_iter;
    GtkTextIter start_iter;
    gchar *text;
    gint sz;
    char nl = '\n';
    gboolean at_end;
 
    gtk_text_buffer_get_iter_at_mark (buffer, &start_iter, mark);

    at_end = gtk_text_iter_is_end (&start_iter);
    
    gtk_text_iter_set_line_offset (&start_iter, 0);
    end_iter = start_iter;
    gtk_text_iter_forward_to_line_end (&end_iter);
    text = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
    sz = gtk_text_iter_get_bytes_in_line (&start_iter);

    gtk_text_buffer_get_end_iter (buffer, &end_iter);
    gtk_text_buffer_place_cursor (buffer, &end_iter);

    if (!at_end) {
      gtk_text_buffer_insert_at_cursor (buffer, "\n", 1);
      gtk_text_buffer_insert_at_cursor (buffer, text, sz-1);
    }

    wrc = write (apl_in, text, sz);
    wrc = write (apl_in, &nl, 1);
    g_free (text);
    return FALSE;
  }

  if (!(key_event->state & (GDK_MOD1_MASK | GDK_MOD2_MASK))) return FALSE;

  guint16 kc = key_event->hardware_keycode;
  if (kc < sizeof(keymap) / sizeof(keymap_s)) {
    CHT_Index ix = (key_event->state & GDK_SHIFT_MASK)
      ? key_shift_alt (kc) :  key_alt (kc);
    if (ix) {
      gshort uic = char_unicode (ix);
      gsize br, bw;
      gchar *res = g_convert ((const gchar *)(&uic),
			      sizeof(gshort),
			      "utf-8",
			      "unicode",
			      &br,
			      &bw,
			      NULL);
      gtk_text_buffer_insert_at_cursor (buffer, res, bw);
      g_free (res);
      return TRUE;
    }
  }

  return FALSE;				// pass the event on
}

int valid_end(char *start, ssize_t size) {
  char *look = start + size - 1;
  ssize_t tail = 1;
  if (size == 0) return TRUE;
  if ((*look&0xff) < 0x80) return TRUE;
  while (look >= start && tail <= 6) {
    if ((*look&0xc0) != 0x80) break;
    --look; ++tail;
  }
  switch (tail) {
  case 2: if ((*look&0xe0) == 0xc0) return TRUE; break;
  case 3: if ((*look&0xf0) == 0xe0) return TRUE; break;
  case 4: if ((*look&0xf8) == 0xf0) return TRUE; break;
  case 5: if ((*look&0xfc) == 0xf8) return TRUE; break;
  case 6: if ((*look&0xfe) == 0xfc) return TRUE; break;
  default: ;
  }
  return FALSE;
}

#define BUFFER_SIZE     1024

static gboolean
apl_read_out (gint fd,
	      GIOCondition condition,
	      gpointer user_data)
{
  static gchar  *text     = NULL;
  static ssize_t text_idx = 0;

  gboolean run = TRUE;
  while(run) {
    text = g_try_realloc (text, (gsize)(text_idx + BUFFER_SIZE));
    if (text) {
      errno = 0;
      ssize_t sz = read (fd, &text[text_idx], BUFFER_SIZE);
      if (sz == -1 && (errno == EAGAIN || errno == EINTR)) continue;
      if (sz == -1) run = FALSE;
      text_idx += sz;
      if (sz < BUFFER_SIZE && valid_end(text, text_idx)) run = FALSE;
    }
    else run = FALSE;
  }
  
  if (text) {
    gtk_text_buffer_insert_at_cursor (buffer, text, text_idx);
    g_free (text);
  }

  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				gtk_text_buffer_get_mark (buffer, "insert"),
				0.0,
				TRUE,
				0.2,
				1.0);
  text = NULL;
  text_idx = 0;
  return TRUE;
}

static gboolean
apl_read_err (gint fd,
	      GIOCondition condition,
	      gpointer user_data)
{
  static gchar  *text     = NULL;
  static ssize_t text_idx = 0;

  gboolean run = TRUE;
  while(run) {
    text = g_try_realloc (text, (gsize)(text_idx + BUFFER_SIZE));
    if (text) {
      errno = 0;
      ssize_t sz = read (fd, &text[text_idx], BUFFER_SIZE);
      if (sz == -1 && (errno == EAGAIN || errno == EINTR)) continue;
      if (sz == -1) run = FALSE;
      text_idx += sz;
      if (sz < BUFFER_SIZE && valid_end(text, text_idx)) run = FALSE;
    }
    else run = FALSE;
  }

  if (text) {
    if (nocolour)
      gtk_text_buffer_insert_at_cursor (buffer, text, text_idx);
    else {
      GtkTextIter insert_iter;
      GtkTextMark *mark = gtk_text_buffer_get_insert (buffer);
      gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);

      gtk_text_buffer_insert_with_tags (buffer,
					&insert_iter,
					text,
					text_idx,
					err_tag,
					NULL);
    }
    g_free (text);
    text = NULL;
    text_idx = 0;
  }
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				gtk_text_buffer_get_mark (buffer, "insert"),
				0.0,
				TRUE,
				0.2,
				1.0);
  text = NULL;
  text_idx = 0;
  return TRUE;
}

static void
apl_exit (GPid pid,
	  gint status,
	  gpointer user_data)
{
  g_spawn_close_pid (pid);
  apl_pid = -1;
  gapl2_quit (NULL, NULL);
}

int
main (int   argc,
      char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  GtkWidget *vbox;
  gboolean rc;
  gchar *new_fn = NULL;
  gchar *opt_lx = NULL;

  GOptionEntry entries[] = {
    { "ftsize", 's', 0, G_OPTION_ARG_INT,
      &ft_size,
      "Font size in points (integer).",
      NULL },
    { "width", 'w', 0, G_OPTION_ARG_INT,
      &width,
      "Width in pixels (integer).",
      NULL },
    { "height", 'h', 0, G_OPTION_ARG_INT,
      &height,
      "Height in pixels (integer).",
      NULL },
    { "vwidth", 'v', 0, G_OPTION_ARG_NONE,
      &vwidth,
      "Use variable width font (boolean switch).",
      NULL },
    { "nocolour", 'n', 0, G_OPTION_ARG_NONE,
      &nocolour,
      "Turn off coloured error text. (boolean switch).",
      NULL },
    { "xeq", 'x', 0, G_OPTION_ARG_FILENAME,
      &new_fn,
      "Set an absolute or on-path executable APL other than the default.",
      NULL },
    { "LX", 0, 0, G_OPTION_ARG_STRING,
      &opt_lx,
      "Invoke APL ⎕LX on startup. (string: APL command or expression)",
      NULL },
    { NULL }
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
    
  gtk_init (&argc, &argv);
  
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_warning ("option parsing failed: %s\n", error->message);
    g_clear_error (&error);
  }

  gchar **apl_argv = g_alloca ((9 + argc) * sizeof (gchar *));
  bzero (apl_argv, (9 + argc) * sizeof (gchar *));
  {
    gint ix = 0;
    apl_argv[ix++] = "apl";
    apl_argv[ix++] = "--noColor";
    apl_argv[ix++] = "--rawCIN";
    apl_argv[ix++] = "-w";
    apl_argv[ix++] = "500";
    apl_argv[ix++] = "--silent";
    if (opt_lx) {
      apl_argv[ix++] = "--LX";
      apl_argv[ix++] = opt_lx;
    }

    if (argc > 1) {
      int i;
      for (i = 1; i < argc; i++) {
	if (0 == g_strcmp0 (argv[i], "--")) break;
      }
      for (; i < argc; ++i) {
        apl_argv[ix++] = argv[i];
      }
    }
    apl_argv[ix++] = NULL;
  }

  if (new_fn) apl_argv[0] = new_fn;

  rc = g_spawn_async_with_pipes (NULL, 		// gchar *working_directory,
				 apl_argv,	// gchar **argv,
				 NULL,		// gchar **envp,
				 G_SPAWN_DO_NOT_REAP_CHILD |
				 G_SPAWN_SEARCH_PATH,	// GSpawnFlags flags,
				 NULL,	// GSpawnChildSetupFunc child_setup,
				 NULL,		// gpointer user_data,
				 &apl_pid,	// GPid *child_pid,
				 &apl_in,	// gint *standard_input,
				 &apl_out,	// gint *standard_output,
				 &apl_err,	// gint *standard_error,
				 &error);	// GError **error
  if (new_fn) g_free (new_fn);
  
  if (!rc) {
    g_print ("error opening APL: %s\n", error->message);
    return 1;
  }

  g_child_watch_add (apl_pid, apl_exit, NULL);

  if (apl_in != -1 && apl_out != -1 && apl_err != -1) {
    g_unix_set_fd_nonblocking (apl_out, TRUE, NULL);
    g_unix_fd_add (apl_out,		// gint fd,
		   G_IO_IN | G_IO_PRI,	// GIOCondition condition,
		   apl_read_out,	// GUnixFDSourceFunc function,
		   NULL);		// gpointer user_data

    g_unix_set_fd_nonblocking (apl_err, TRUE, NULL);
    g_unix_fd_add (apl_err,		// gint fd,
		   G_IO_IN | G_IO_PRI,	// GIOCondition condition,
		   apl_read_err,	// GUnixFDSourceFunc function,
		   NULL);		// gpointer user_data
  }
  else {
    g_print ("error opening APL file descriptors.");
    return 1;
  }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);
    
  g_signal_connect (window, "destroy",
		    G_CALLBACK (gapl2_quit), NULL);
    
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  build_menubar (vbox);

  desc =
    pango_font_description_from_string (vwidth ? "UnifontMedium" : "FreeMono");
  pango_font_description_set_size (desc, ft_size * PANGO_SCALE);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  view = gtk_text_view_new ();
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 8);
  g_signal_connect (view, "key-press-event",
		    G_CALLBACK (key_press_event), NULL);
  if (desc) gtk_widget_override_font (view, desc);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scroll), TRUE, TRUE, 2);
  
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  err_tag =
    gtk_text_buffer_create_tag (buffer, "err_tag",
				"foreground", "red",
				NULL);

  gtk_widget_show_all (window);
  gtk_main ();
    
  return 0;
}

