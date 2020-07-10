#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "menu.h"
#include "help.h"
#include "apl.h"
#include "aplwrap.h"
#include "aplio.h"
#include "txtbuf.h"
#include "edit.h"
#include "pstat.h"
#include "options.h"

static gchar *filename = NULL;
gboolean show_status = TRUE;

#define START_EDIF	"'libedif2.so' ⎕FX '%s'"
#define CLOSE_EDIF	")erase %s"

#if 0
static void
aplfx_callback_2 (gchar *result, size_t idx, void *state)
{
  gchar * cmd = g_strdup_printf ("⍞←'%s'", result);
  if (state != (void *)(-1)) apl_send_inp (cmd, strlen (cmd));
  g_free (cmd);
}
#endif

static void
unset_edif ()
{
  if (edif_name) {
    gchar *cmd = g_strdup_printf (CLOSE_EDIF, edif_name);
    apl_eval (cmd, -1, NULL, NULL);
    g_free (cmd);
  }
}

void
set_edif (gboolean startup, gboolean state)
{
  if (edif_name && (!startup || state)) {
    char * ss = state ? START_EDIF : CLOSE_EDIF;
    gchar *cmd = g_strdup_printf (ss, edif_name);
#if 0
    apl_eval (cmd, -1, aplfx_callback_2, GINT_TO_POINTER (-1));
#else
    apl_eval (cmd, -1, NULL, NULL);
#endif
    g_free (cmd);
  }
}

static void
edif_toggle_cb (GtkToggleButton *togglebutton,
		gpointer         user_data)
{
  enable_edif = gtk_toggle_button_get_active (togglebutton);
  if (enable_edif) set_edif (FALSE, enable_edif);
}

static void
settings_cb (GtkWidget *widget,
	     gpointer   data)
{
  GtkWidget *dialog;
  GtkWidget *content;
  GtkWidget *vbox;
  GtkWidget *ps_toggle;
  
  dialog =  gtk_dialog_new_with_buttons (_ ("Settings"),
                                         get_top_window (),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
					 _ ("Cancel"), GTK_RESPONSE_CANCEL,
                                         _ ("_OK"), GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
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
  
  GtkWidget *edif_toggle = gtk_check_button_new_with_label (_("Enable Edif"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (edif_toggle),
				enable_edif);
  g_signal_connect (edif_toggle, "toggled",
		    G_CALLBACK (edif_toggle_cb), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), edif_toggle, FALSE, FALSE, 2);

  GdkRGBA fg_rgba;
  GdkRGBA bg_rgba;
  GtkWidget *fg_button;
  GtkWidget *bg_button;
  GtkWidget *ft_sz_spin;
  GtkWidget *ename_ety;
  
  {
    enum {
	  ROW_ENAME,
	  ROW_FG,
	  ROW_BG,
	  ROW_FS
    };
    
    GtkWidget *grid =  gtk_grid_new ();
    gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 8);

    GtkWidget *ename_lbl= gtk_label_new (_ ("Editor command"));
    ename_ety = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (ename_ety), edif_name);
    gtk_entry_set_input_purpose (GTK_ENTRY (ename_ety),
				 GTK_INPUT_PURPOSE_ALPHA);

    gdk_rgba_parse (&fg_rgba, fg_colour);
    gdk_rgba_parse (&bg_rgba, bg_colour);
    
    GtkWidget *fg_label = gtk_label_new (_("Foreground"));
    fg_button = gtk_color_button_new ();
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (fg_button), &fg_rgba);

    GtkWidget *bg_label = gtk_label_new (_("Background"));
    bg_button = gtk_color_button_new ();
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (bg_button), &bg_rgba);
    
    GtkWidget *ft_sz_label = gtk_label_new (_("Font size"));
    ft_sz_spin =
      gtk_spin_button_new_with_range (4.0, 56.0, 1.0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (ft_sz_spin), TRUE);
    gtk_spin_button_set_digits  (GTK_SPIN_BUTTON (ft_sz_spin), 0);
    gtk_spin_button_set_value   (GTK_SPIN_BUTTON (ft_sz_spin), (double)ft_size);

    gtk_grid_attach (GTK_GRID (grid), ename_lbl,	0, ROW_ENAME, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), ename_ety,	1, ROW_ENAME, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), fg_label,		0, ROW_FG, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), fg_button,	1, ROW_FG, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), bg_label,		0, ROW_BG, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), bg_button,	1, ROW_BG, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), ft_sz_label,	0, ROW_FS, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), ft_sz_spin,	1, ROW_FS, 1, 1);
  }


  gtk_widget_show_all (dialog);
  gint response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_ACCEPT) {
    GdkRGBA fg_rgba_new;
    GdkRGBA bg_rgba_new;
    gint    ft_size_new;
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (fg_button), &fg_rgba_new);
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (bg_button), &bg_rgba_new);

    const gchar *new_ename = gtk_entry_get_text (GTK_ENTRY (ename_ety));
    if (g_strcmp0 (new_ename, edif_name)) {
      unset_edif ();
      if (edif_name) g_free (edif_name);
      edif_name = g_strdup (new_ename);
      set_edif (FALSE, enable_edif);
    }

    ft_size_new =
      gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ft_sz_spin));
    gboolean do_reload = FALSE;

    if (ft_size != ft_size_new) {
      ft_size = ft_size_new;
      do_reload = TRUE;
    }
    if (!gdk_rgba_equal (&fg_rgba_new, &fg_rgba)) {
      if (fg_colour) g_free (fg_colour);
      fg_colour = gdk_rgba_to_string (&fg_rgba_new);
      do_reload = TRUE;
    }
    if (!gdk_rgba_equal (&bg_rgba_new, &bg_rgba)) {
      if (bg_colour) g_free (bg_colour);
      bg_colour = gdk_rgba_to_string (&bg_rgba_new);
      do_reload = TRUE;
    }
    if (do_reload) load_css_provider ();
  }

  gtk_widget_destroy (dialog);
}

static GSList *shortcuts = NULL;

static void
remember_shortcut (char *path)
{
  if (!g_slist_find (shortcuts, path))
    shortcuts = g_slist_prepend(shortcuts, path);
}

void
init_shortcuts ()
{
  gchar *contents;
  if (shortcuts_file && g_file_get_contents (shortcuts_file, &contents,
                                             NULL, NULL)) {
    gchar **lines = g_strsplit (contents, "\n", -1);
    for (int i = g_strv_length (lines)-1; i >= 0; --i) {
      gchar *path = g_strstrip (lines[i]);
      if (path[0] != '#' && path[0] != ';' && path[0] != '\0' &&
          g_file_test (path, G_FILE_TEST_IS_DIR))
        remember_shortcut (g_strdup (path));
    }
    g_strfreev(lines);
  }
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
					get_top_window (),
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
					get_top_window (),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Save"),   GTK_RESPONSE_ACCEPT,
                                        NULL);
  dirname = *filename && *filename != NEW_FILE
    ? g_path_get_dirname (*filename) : g_strdup (".");
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
	e_dialog = gtk_message_dialog_new (get_top_window (),
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
    if (*filename && *filename != NEW_FILE) g_free (*filename);
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
    e_dialog = gtk_message_dialog_new (get_top_window (),
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
  if (path && gtk_tree_model_get_iter (model, &iter, path)) {
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

static gboolean
names_search (GtkTreeModel *model,
              gint          column,
              const gchar  *key,
              GtkTreeIter  *iter,
              gpointer      search_data)
{
  gchar *name;
  gtk_tree_model_get(model, iter,
                     OBJECT_RAW_NAME, &name,
                     -1);
  gboolean rv = !strstr(name, key);
  g_free (name);
  return rv;
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
                                         get_top_window (),
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
  gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW(names_tree),
                                       names_search, NULL, NULL);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW(names_tree), 1);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW(names_tree), FALSE);
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
  GtkTreePath *path;
  gtk_tree_view_get_cursor (GTK_TREE_VIEW (names_tree), &path, &column);
  gtk_tree_view_row_activated (GTK_TREE_VIEW (names_tree), path, column);
  gtk_tree_path_free (path);
  gboolean have_selection =
    gtk_tree_selection_get_selected (selection, NULL, NULL);
  gtk_widget_destroy (dialog);
  if (have_selection && response == GTK_RESPONSE_ACCEPT) {
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
new_file (GtkWidget *widget,
          gpointer   data)
{
  edit_file (NULL);
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

  add_menu_item (_ ("_New Object"), GDK_KEY_n, accel_group,
                 G_CALLBACK (new_object), NULL, menu);

  add_menu_item (_ ("New Fil_e"), GDK_KEY_e, accel_group,
                 G_CALLBACK (new_file), NULL, menu);

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

  help_menu (vbox, accel_group, menubar);

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (menubar), FALSE, FALSE, 2);
}
