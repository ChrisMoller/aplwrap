#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <sys/socket.h>

#include "menu.h"
#include "apl.h"
#include "aplio.h"
#include "txtbuf.h"
#include "edit.h"

#include "layout.h"

static gchar *filename = NULL;

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

static gboolean
set_filename ()
{
  gboolean rc = FALSE;
  GtkWidget *dialog;
  gchar *lname = NULL;
  gchar * dirname;

  dialog = gtk_file_chooser_dialog_new ("Save Log",
                                        NULL,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Save"),   GTK_RESPONSE_ACCEPT,
                                        NULL);
  if (filename) dirname = g_path_get_dirname (filename);
  else dirname = g_strdup (".");
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirname);
  g_free (dirname);
  gtk_file_chooser_set_create_folders (GTK_FILE_CHOOSER (dialog), TRUE);

  gboolean run = TRUE;
  while (run) {
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
      lname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (g_file_test (lname, G_FILE_TEST_EXISTS)) {
	GtkWidget *e_dialog;
	gint response;
	e_dialog = gtk_message_dialog_new (NULL,
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   GTK_MESSAGE_QUESTION,
					   GTK_BUTTONS_NONE,
					   _ ("File %s exists.  Overwrite it?"),
					   lname);
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
	  run = FALSE;			// fall through
	case GTK_RESPONSE_NO:
	  g_free (lname);
	  lname = NULL;
	  break;
	case GTK_RESPONSE_YES:
	  run = FALSE;
	  break;
	}
      }
      else run = FALSE;
    }
    else run = FALSE;
  }
  if (lname) {
    if (filename) g_free (filename);
    filename = lname;
    rc = TRUE;
  }
  gtk_widget_destroy (dialog);
  return rc;
}

static void
save_log_fer_real (GtkTextBuffer *buffer)
{
  gboolean written = FALSE;
  if (filename) {
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
    gchar *text =
      gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
    FILE *lfile = fopen (filename, "w");
    if (lfile) {
      // as brain-dead as it sounds, GTK has no way to get the length of
      // a buffer in bytes.
      fwrite (text, 1, strlen (text), lfile);
      fclose (lfile);
      written = TRUE;
    }
    g_free (text);
  }
  if (!written) {
    GtkWidget *e_dialog;
    e_dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_ERROR,
				       GTK_BUTTONS_OK,
				       _ ("Write failed"));
    gtk_window_set_keep_above (GTK_WINDOW (e_dialog), TRUE);
    gtk_window_set_position (GTK_WINDOW (e_dialog), GTK_WIN_POS_MOUSE);
    gtk_dialog_run (GTK_DIALOG (e_dialog));
    gtk_widget_destroy (e_dialog);
  }
}

void
save_log (GtkWidget *widget,
	  gpointer   data)
{
  GtkTextBuffer *lbuffer = data ? : buffer;
  gboolean doit = filename ? TRUE : set_filename ();
  if (doit) save_log_fer_real (lbuffer);
}

void
save_log_as (GtkWidget *widget,
	     gpointer   data)
{
  GtkTextBuffer *lbuffer = data ? : buffer;
  gboolean doit = set_filename ();
  if (doit) save_log_fer_real (lbuffer);
}

enum {
  OBJECT_NAME,
  OBJECT_RAW_NAME,
  OBJECT_NC,
  OBJECT_COUNT
};

typedef struct {
  gchar *name;
  gint nc;
} sym_def_s;

static void
names_cb (GtkTreeView *tree_view,
	  GtkTreePath *path,
	  GtkTreeViewColumn *column,
	  gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (!user_data) return;

  sym_def_s *sd = user_data;
  
  model = gtk_tree_view_get_model(tree_view);
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get(model, &iter,
                       OBJECT_RAW_NAME, &sd->name,
                       OBJECT_NC,       &sd->nc,
                       -1);
  }
}

static gboolean
file_button_press_cb (GtkWidget      *widget,
                     GdkEventButton *event,
                     gpointer        data)
{
  if (event-> type == GDK_2BUTTON_PRESS) {
    gtk_dialog_response (GTK_DIALOG (data), GTK_RESPONSE_ACCEPT);
    return TRUE;
  }
  else return FALSE;
}


static void
open_object_cb (gchar *text, void *tw)
{
  GtkWidget *dialog;
  GtkWidget *content;
  gboolean response;
  GtkListStore *names_store;
  GtkWidget *names_tree;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  set_socket_cb (NULL, NULL);

  dialog =  gtk_dialog_new_with_buttons (_ ("Open Object"),
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         _ ("_Cancel"), GTK_RESPONSE_CANCEL,
                                         _ ("_OK"), GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  names_store = gtk_list_store_new (OBJECT_COUNT,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_INT);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (names_store),
                                          OBJECT_NAME, GTK_SORT_ASCENDING);

  
  gchar **names = g_strsplit (text, "\n", 0);
  for (int i = 0; names[i]; i++) {
    GtkTreeIter   iter;
    if (!*names[i]) continue;
    if (!g_strcmp0 (names[i], END_TAG)) break;
    guint nc;
    gchar *endptr;
#ifdef EDIT_VARIABLES
    sscanf (names[i], "%ms %u", &endptr, &nc);
#else
    sscanf (names[i], "%ms", &endptr);
    nc = NC_FUNCTION;
#endif
    gtk_list_store_append (names_store, &iter);
    gchar *nn = g_strdup_printf ((nc == NC_FUNCTION) ? "<i>%s</i>" : "%s",
				 endptr);
    gtk_list_store_set (names_store, &iter,
			OBJECT_NAME,     nn,
			OBJECT_RAW_NAME, endptr,
			OBJECT_NC,       (int)nc,
			-1);
    g_free (nn);
    g_free (endptr);
  }

  g_strfreev (names);

  names_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (names_store));
  gtk_widget_set_events (names_tree, GDK_2BUTTON_PRESS);
  g_signal_connect (names_tree, "button-press-event",
		    G_CALLBACK (file_button_press_cb), dialog);
  selection =  gtk_tree_view_get_selection (GTK_TREE_VIEW (names_tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  GValue val = G_VALUE_INIT;
  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean (&val, TRUE);
  g_object_set_property (G_OBJECT (names_tree),
			 "activate-on-single-click", &val);
  g_value_unset (&val);
  sym_def_s sd = {NULL, -1};
  g_signal_connect(G_OBJECT(names_tree), "row-activated",
                   G_CALLBACK (names_cb), &sd);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Object",
						     renderer,
						     "markup",
						     OBJECT_NAME,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (names_tree), column); 

  GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, -1, 300);
  gtk_container_add (GTK_CONTAINER (scroll), names_tree);
  gtk_container_add (GTK_CONTAINER (content), scroll);
  gtk_widget_show_all (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  if (response == GTK_RESPONSE_ACCEPT) {
    if (sd.name) {
      edit_object (sd.name, sd.nc);
      g_free (sd.name);
    }
  }
}

void
open_object (GtkWidget *widget,
	     gpointer   data)
{
#ifdef EDIT_VARIABLES
#define COMMAND "variables:tagged\n"
#else
#define COMMAND "variables:function\n"
#endif
  set_socket_cb (open_object_cb, NULL);
  if (send(sockfd, COMMAND, strlen(COMMAND), 0) < 0) {
    perror("Error in send()");	// fixme
  }
}

void
new_object (GtkWidget *widget,
	    gpointer   data)
{
  edit_object (NULL, NC_INVALID);
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

  item = gtk_menu_item_new_with_label(_ ("New"));
  g_signal_connect (G_OBJECT (item), "activate",
                   G_CALLBACK (new_object), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Open"));
  g_signal_connect(G_OBJECT (item), "activate",
		   G_CALLBACK (open_object), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Save Log"));
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (save_log), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Save Log as"));
  g_signal_connect(G_OBJECT(item), "activate",
		   G_CALLBACK (save_log_as), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

#if 0
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
                   G_CALLBACK (aplwrap_quit), NULL);
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
