#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "menu.h"
#include "apl.h"

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

guchar *keymap_buf = NULL;

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
  gchar *authors[] = {"C. H. L. Moller", "David B. Lamkins", NULL};
  gchar *comments = _("aplwrap is a GTK+-based front-end for GNU APL.");

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

void
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
