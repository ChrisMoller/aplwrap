#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "menu.h"
#include "aplwrap.h"
#include "options.h"

#define GROUP_APLWRAP  "AplwrapResources"

#define KEY_SHOW_PSTAT		"ShowPstat"
#define KEY_FONT_SIZE		"FontSize"
#define KEY_FOREGROUND_COLOUR	"FgColour"
#define KEY_BACKGROUND_COLOUR	"BgColour"
#define KEY_ENABLE_EDIF		"EnableEdif"
#define KEY_EDIF_NAME		"EdifName"

static gchar *resource_path = NULL;

static void
build_resource_path ()
{
  gchar *config_path = g_strdup_printf ("%s/.config", g_get_home_dir ());
  if (g_file_test (config_path, G_FILE_TEST_EXISTS)) {
    if (!g_file_test (config_path, G_FILE_TEST_IS_DIR)) return;  // funky
  }
  else mkdir (config_path, 0775);    // ~/.config doesn't exist, create it.
  resource_path =
    g_strdup_printf ("%s/%s", config_path, g_get_prgname ());
  g_free (config_path);
}

void
restore_resources ()
{
  GKeyFile *key_file;
  gboolean rc;
  GError *error = NULL;

  build_resource_path ();

  key_file = g_key_file_new ();
  rc = g_key_file_load_from_file (key_file,
                                  resource_path,
                                  G_KEY_FILE_KEEP_COMMENTS
                                  | G_KEY_FILE_KEEP_TRANSLATIONS,
                                  NULL);

  if (rc) {
    gboolean bv;
    gint iv;
    gchar *sv;

    bv = g_key_file_get_boolean (key_file,
				 GROUP_APLWRAP,
				 KEY_SHOW_PSTAT,
				 &error);
    if (error) g_clear_error (&error);
    else show_status = bv;

    iv = g_key_file_get_integer (key_file,
				 GROUP_APLWRAP,
				 KEY_FONT_SIZE,
				 &error);
    if (error) g_clear_error (&error);
    else ft_size = iv;

    sv = g_key_file_get_string (key_file,
				GROUP_APLWRAP,
				KEY_FOREGROUND_COLOUR,
				&error);
    if (error) g_clear_error (&error);
    else {
      if (fg_colour) g_free (fg_colour);
      fg_colour = sv;
    }

    sv = g_key_file_get_string (key_file,
				GROUP_APLWRAP,
				KEY_BACKGROUND_COLOUR,
				&error);
    if (error) g_clear_error (&error);
    else {
      if (bg_colour) g_free (bg_colour);
      bg_colour = sv;
    }

    sv = g_key_file_get_string (key_file,
				GROUP_APLWRAP,
				KEY_EDIF_NAME,
				&error);
    if (error) g_clear_error (&error);
    else {
      if (edif_name) g_free (edif_name);
      edif_name = sv;
    }

    bv = g_key_file_get_boolean (key_file,
				 GROUP_APLWRAP,
				 KEY_ENABLE_EDIF,
				 &error);
    if (error) g_clear_error (&error);
    else enable_edif = bv;
  }

  g_key_file_free (key_file); 
}

void
save_resources ()
{
  GKeyFile *key_file = NULL;

  key_file = g_key_file_new ();

  g_key_file_set_comment (key_file,
                          NULL,
                          NULL,
                          "aplwrap resources",
                          NULL);

  g_key_file_set_boolean (key_file,
                          GROUP_APLWRAP,
                          KEY_SHOW_PSTAT,
                          show_status);

  g_key_file_set_string (key_file,
			 GROUP_APLWRAP,
			 KEY_FOREGROUND_COLOUR,
			 fg_colour);

  g_key_file_set_string (key_file,
			 GROUP_APLWRAP,
			 KEY_BACKGROUND_COLOUR,
			 bg_colour);

  g_key_file_set_integer (key_file,
			  GROUP_APLWRAP,
			  KEY_FONT_SIZE,
			  ft_size);

  g_key_file_set_boolean (key_file,
                          GROUP_APLWRAP,
                          KEY_ENABLE_EDIF,
                          enable_edif);

  g_key_file_set_string (key_file,
			 GROUP_APLWRAP,
			 KEY_EDIF_NAME,
			 edif_name);

  {
    gchar *data = NULL;
    gsize data_length;
    
    data = g_key_file_to_data (key_file, &data_length, NULL);
    g_file_set_contents (resource_path,
                         data,
                         data_length,
                         NULL);

    if (data) g_free (data);
  }

  g_key_file_free (key_file); 
}
