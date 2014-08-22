#define _POSIX_SOURCE

#include "../config.h"

#include <gtk/gtk.h>
#include <glib-unix.h>
#include <string.h>
#include <strings.h>

#include "apl.h"
#include "aplio.h"
#include "options.h"

#include <sys/types.h>
#include <signal.h>

static GPid apl_pid = -1;

void
gapl2_quit (GtkWidget *widget,
	     gpointer  data)
{
  if (apl_pid != -1) {
    kill ((pid_t)apl_pid, SIGKILL);
    g_spawn_close_pid (apl_pid);
  }

  gtk_main_quit ();
}

static void
apl_exit (GPid     pid,
	  gint     status,
	  gpointer user_data)
{
  g_spawn_close_pid (pid);
  apl_pid = -1;
  gapl2_quit (NULL, NULL);
}

void apl_interrupt ()
{
  if (apl_pid != -1) kill ((pid_t)apl_pid, SIGINT);
}

static gchar**
make_env ()
{
  extern char **environ;
  ssize_t envc = 0;
  gchar **env = environ;

  while (*env++) ++envc;
  env = g_try_malloc(2 + envc * sizeof(gchar*));
  env[envc+1] = NULL;
  env[envc+0] = "APLWRAP=" VERSION;
  while (envc--) {
    if (!strncmp(environ[envc], "TERM=", 5))
      env[envc] = "TERM=dumb";
    else
      env[envc] = environ[envc];
  }
  return env;
}

int apl_spawn (int   argc,
               char *argv[])
{
  GError *error = NULL;
  gboolean rc;
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
				 make_env(),	// gchar **envp,
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

  return 0;
}
