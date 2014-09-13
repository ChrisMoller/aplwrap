#include "../config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "menu.h"
#include "aplwrap.h"

#define GROUP_APLWRAP  "AplwrapResources"
#define KEY_SHOW_PSTAT "ShowPstat"

void
restore_resources ()
{
  GKeyFile *key_file;
  gboolean rc;
  gchar *path;

  path = g_strdup_printf ("%s/.%s",
			  g_get_home_dir (),
			  g_get_prgname ());

  key_file = g_key_file_new ();
  rc = g_key_file_load_from_file (key_file,
                                  path,
                                  G_KEY_FILE_KEEP_COMMENTS
                                  | G_KEY_FILE_KEEP_TRANSLATIONS,
                                  NULL);

  if (rc) {
    gboolean bv;
    bv = g_key_file_get_boolean (key_file,
				 GROUP_APLWRAP,
				 KEY_SHOW_PSTAT,
				 NULL);

    show_status = bv;
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

  {
    gchar *path;
    gchar *data = NULL;
    gsize data_length;
    
    data = g_key_file_to_data (key_file, &data_length, NULL);
    path = g_strdup_printf ("%s/.%s",
                            g_get_home_dir (),
                            g_get_prgname ());
    g_file_set_contents (path,
                         data,
                         data_length,
                         NULL);
    g_free (path);
    
    if (data) g_free (data);
  }

  g_key_file_free (key_file); 
}
