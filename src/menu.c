#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "menu.h"
#include "apl.h"
#include "aplwrap.h"
#include "aplio.h"
#include "txtbuf.h"
#include "edit.h"
#include "pstat.h"
#include "options.h"

#include "layout.h"

static gchar *filename = NULL;
gboolean show_status = TRUE;
static GtkWidget *pstat_grid = NULL;

typedef struct {
  const gchar *label;
  GtkWidget   *value;
} pstat_ety_s;

pstat_ety_s pstat_etys[] = {
  {"# sequence",	NULL},
  {"∆ elapsed time",	NULL},
  {"∆ user time",	NULL},
  {"∆ system time",	NULL},
  {"∆ virtual size",	NULL},
  {"∆ resident size",	NULL},
  {"∆ minor faults",	NULL},
  {"∆ major faults",	NULL},
  {"∆ block io wait",	NULL},
  {"∆ read chars",	NULL},
  {"∆ write chars",	NULL},
  {"∆ read bytes",	NULL},
  {"∆ write bytes",	NULL},
  {"∆ syscalls read",	NULL},
  {"∆ syscalls write",	NULL},
  {"∆ canceled bytes",	NULL},
};

void
ps_dialog_cb (GtkDialog *dialog,
	      gint       response_id,
	      gpointer   user_data)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
  pstat_grid = NULL;
}

void
set_pstat_value (gint idx, const gchar *val)
{
  if (idx >= 0 && idx < PSTAT_MAX && pstat_etys[idx].value) 
    gtk_label_set_text (GTK_LABEL (pstat_etys[idx].value), val);
}

static void
ps_button_cb (GtkToggleButton *togglebutton,
	      gpointer         user_data)
{
  GtkWidget *dialog;
  GtkWidget *content;
  GtkWidget *vbox;

  if (pstat_grid) return;

  dialog =  gtk_dialog_new_with_buttons (_ ("Pstat"),
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         _ ("_Close"), GTK_RESPONSE_ACCEPT,
                                         NULL);
  g_signal_connect_swapped (dialog, "response",
                          G_CALLBACK (ps_dialog_cb),
                          dialog);
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (content), vbox);
  pstat_grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (pstat_grid), 8);

  for (int i = 0; i < sizeof(pstat_etys) / sizeof(pstat_ety_s); i++) {
    GtkWidget *lbl = gtk_label_new (pstat_etys[i].label);
    gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.0);
    GtkWidget *val = gtk_label_new ("");
    gtk_misc_set_alignment (GTK_MISC (val), 1.0, 0.0);
    pstat_etys[i].value = val;
    gtk_grid_attach (GTK_GRID (pstat_grid), lbl, 0, i, 1, 1);
    gtk_grid_attach (GTK_GRID (pstat_grid), val, 1, i, 1, 1);
  }
  update_pstat_strings ();
  
  gtk_box_pack_start (GTK_BOX (vbox), pstat_grid, FALSE, FALSE, 2);
  gtk_widget_show_all (dialog);
}

static void
ps_toggle_cb (GtkToggleButton *togglebutton,
	      gpointer         user_data)
{
  show_status = gtk_toggle_button_get_active (togglebutton);
  set_status_visibility (show_status);
}
  
static void
settings_cb (GtkWidget *widget,
	     gpointer   data)
{
  GtkWidget *dialog;
  GtkWidget *content;
  GtkWidget *vbox;
  GtkWidget *ps_toggle;
  GtkWidget *ps_button;
  
  dialog =  gtk_dialog_new_with_buttons (_ ("Settings"),
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         _ ("_OK"), GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (content), vbox);

  ps_toggle = gtk_check_button_new_with_mnemonic (_ ("Enable pstat _line."));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ps_toggle),
				show_status);
  g_signal_connect (ps_toggle, "toggled",
		    G_CALLBACK (ps_toggle_cb), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), ps_toggle, FALSE, FALSE, 2);

  ps_button = gtk_button_new_with_mnemonic (_ ("Open pstat _window."));
  g_signal_connect (ps_button, "clicked",
		    G_CALLBACK (ps_button_cb), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), ps_button, FALSE, FALSE, 2);

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

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

static GSList *shortcuts = NULL;

static void
remember_shortcut (char *path)
{
  if (!g_slist_find (shortcuts, path))
    shortcuts = g_slist_prepend(shortcuts, path);
}

static void
add_shortcut (gpointer data, gpointer user_data)
{
  gchar *path = data;
  GtkFileChooser *chooser = user_data;
  gtk_file_chooser_add_shortcut_folder (chooser, path, NULL);
}

gboolean
import_file ()
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new ("Open File",
                                        NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Open"),   GTK_RESPONSE_ACCEPT,
                                        NULL);
  GtkFileFilter *filter_apl = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter_apl, "*.apl");
  gtk_file_filter_set_name (filter_apl, "APL files");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter_apl);
  GtkFileFilter *filter_text = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter_text, "text/*");
  gtk_file_filter_set_name (filter_text, "Text files");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter_text);
  g_slist_foreach (shortcuts, add_shortcut, dialog);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    gchar *lname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    remember_shortcut (g_path_get_dirname (lname));
    edit_file (lname);
  }
  gtk_widget_destroy (dialog);
  return TRUE;		/* handled */
}

gboolean
set_filename (const gchar *prompt, gchar **filename)
{
  gboolean rc = FALSE;
  GtkWidget *dialog;
  gchar *lname = NULL;
  gchar * dirname;

  dialog = gtk_file_chooser_dialog_new (prompt,
                                        NULL,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Save"),   GTK_RESPONSE_ACCEPT,
                                        NULL);
  if (*filename) dirname = g_path_get_dirname (*filename);
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
    if (*filename) g_free (*filename);
    *filename = lname;
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
      gtk_text_buffer_get_slice (buffer, &start_iter, &end_iter, TRUE);

    // gtk-text-iter-get-pixbuf.

      
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
  gboolean doit = filename ? TRUE : set_filename ("Save Log", &filename);
  if (doit) save_log_fer_real (lbuffer);
}

void
save_log_as (GtkWidget *widget,
	     gpointer   data)
{
  GtkTextBuffer *lbuffer = data ? : buffer;
  gboolean doit = set_filename ("Save Log", &filename);
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

  set_send_cb (NULL, NULL);

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
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
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
  set_send_cb (open_object_cb, NULL);
  send_apl (COMMAND, strlen(COMMAND));
}

void
new_object (GtkWidget *widget,
	    gpointer   data)
{
  edit_object (NULL, NC_INVALID);
}

void
add_menu_item (gchar         *name,
               gint           accel_key,
               GtkAccelGroup *accel_group,
               GCallback      callback,
               gpointer       data,
               GtkWidget     *menu)
{
  GtkWidget *item = gtk_menu_item_new_with_mnemonic (name);
  if (accel_group && accel_key >= 0) 
    gtk_widget_add_accelerator (item, "activate", accel_group,
                                accel_key, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  g_signal_connect (G_OBJECT (item), "activate", callback, data);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
}

void
build_menubar (GtkWidget *vbox)
{
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *item;

  GtkWidget *window = gtk_widget_get_parent (vbox);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  menubar = gtk_menu_bar_new();

  /********* file menu ********/

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

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  add_menu_item (_ ("Save _Log"), -1, NULL,
                 G_CALLBACK (save_log), NULL, menu);

  add_menu_item (_ ("Save Log as"), -1, NULL,
                 G_CALLBACK (save_log_as), NULL, menu);

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

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  add_menu_item (_ ("_Settings"), -1, NULL,
                 G_CALLBACK (settings_cb), NULL, menu);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  add_menu_item (_ ("_Quit"), -1, NULL,
                 G_CALLBACK (aplwrap_quit), NULL, menu);

  /********* help menu ********/

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label (_ ("Help"));
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

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (menubar), FALSE, FALSE, 2);
}
