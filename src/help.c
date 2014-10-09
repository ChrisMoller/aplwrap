#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "menu.h"
#include "help.h"
#include "options.h"
#include "aplwrap.h"
#include "build.h"

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

static guchar *keymap_buf = NULL;

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
  gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
  
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
show_manuals (GtkWidget *widget,
              gpointer   data)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), _ ("Manuals"));
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 400);
  GtkWidget *notebook = gtk_notebook_new ();
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook));
  gtk_container_add (GTK_CONTAINER (window), notebook);
  PangoFontDescription *desc =
    pango_font_description_from_string ("FreeMono");
  pango_font_description_set_size (desc, ft_size * PANGO_SCALE);

  GDir *dir = g_dir_open (MANUALS_PATH, 0, NULL);
  const gchar *fname;
  while ((fname = g_dir_read_name (dir))) {
    gchar *path = g_build_filename (MANUALS_PATH, fname, NULL);
    gchar *text;
    if (g_file_get_contents (path, &text, NULL, NULL)) {
      GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
      gtk_text_buffer_set_text (buffer, text, -1);
      g_free (text);
      GtkWidget *view = gtk_text_view_new_with_buffer (buffer);
      gtk_container_set_border_width (GTK_CONTAINER (view), 4);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
      gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
      gtk_widget_override_font (view, desc);
      GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
      GtkWidget *label = gtk_label_new (g_path_get_basename (path));
      gtk_container_add (GTK_CONTAINER (scroll), view);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scroll, label);
      gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (notebook), scroll, TRUE);
    }
  }
  g_dir_close (dir);

  gtk_widget_show_all (window);
}

static void
show_about (GtkWidget *widget,
            gpointer   data)
{
  gchar *authors[] = {"C. H. L. Moller", "David B. Lamkins", NULL};
  gchar *comments = _(PGM_TITLE " is a GTK+-based front-end for GNU APL.");

  gtk_show_about_dialog (NULL,
                         "program-name", _ (PGM_TITLE),
                         "title", _ ("About " PGM_TITLE),
                         "version", VERSION
                         #ifdef BUILD
                         " (build: " BUILD
                         #ifdef DIRTY
                         " [dirty]"
                         #endif
                         ")"
                         #endif
                         ,
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "copyright", "Copyright 2014",
                         "website", "mailto:moller@mollerware.com",
                         "website-label", "moller@mollerware.com",
                         "authors", authors,
                         "comments", comments,
                         NULL);

}

void
help_menu (GtkWidget     *vbox,
           GtkAccelGroup *accel_group,
           GtkWidget      *menubar)
{
  /********* help menu ********/

  GtkWidget *menu = gtk_menu_new();
  GtkWidget *item = gtk_menu_item_new_with_label (_ ("Help"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

  add_menu_item (_ ("_Keymap"), GDK_KEY_k, accel_group,
                 G_CALLBACK (show_keymap), NULL, menu);

  add_menu_item (_ ("_Manuals"), GDK_KEY_m, accel_group,
                 G_CALLBACK (show_manuals), NULL, menu);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  add_menu_item (_ ("_About"), -1, NULL,
                 G_CALLBACK (show_about), NULL, menu);
}
