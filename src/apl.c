#define _POSIX_SOURCE

#include "../config.h"

#include <gtk/gtk.h>
#include <glib-unix.h>
#include <string.h>
#include <strings.h>

#include "apl.h"
#include "aplwrap.h"
#include "aplio.h"
#include "options.h"

#include <sys/types.h>
#include <signal.h>

static GPid apl_pid = -1;

GPid
get_apl_pid ()
{
  return apl_pid;
}

void
aplwrap_quit (GtkWidget *widget,
	      gpointer  data)
{
  if (apl_pid != -1) {
    kill ((pid_t)apl_pid, SIGKILL);
    g_spawn_close_pid (apl_pid);
  }

  if (plot_pipe_fd != -1) close (plot_pipe_fd);
  if (plot_pipe_name) {
    unlink (plot_pipe_name);
    g_free (plot_pipe_name);
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
  aplwrap_quit (NULL, NULL);
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
  env = g_try_malloc((3 + envc) * sizeof(gchar*));
  env[envc+2] = NULL;
  env[envc+1] = g_strdup_printf ("APLPLOT=%s",plot_pipe_name);
  env[envc+0] = "APLWRAP=" VERSION;
  while (envc--) {
    if (!strncmp(environ[envc], "TERM=", 5))
      env[envc] = "TERM=dumb";
    else
      env[envc] = environ[envc];
  }
  return env;
}

static gchar **apl_argv      = NULL;
static int     apl_argv_next = 0;
static int     apl_argv_max  = 0;
#define APL_ARGV_INCR	16

static void
append_argv (gchar *av)
{
  if (apl_argv_max <= apl_argv_next) {
    apl_argv_max += APL_ARGV_INCR;
    apl_argv = g_realloc (apl_argv, apl_argv_max * sizeof(gchar *));
  }
  apl_argv[apl_argv_next++] = av;
}

int apl_spawn (int   argc,
               char *argv[])
{
  GError *error = NULL;
  gboolean rc;

  {
    append_argv ("apl");
    append_argv ("--noColor");
    append_argv ("--rawCIN");
    append_argv ("--emacs_arg");
    append_argv ("0");
    append_argv ("-w");
    append_argv ("500");
    append_argv ("--silent");
    if (opt_lx) {
      append_argv ("--LX");
      append_argv (opt_lx);
    }
    if (script) {
      append_argv ("-f");
      append_argv (script);
    }

    if (argc > 1) {
      int i;
      for (i = 1; i < argc; i++) {
	if (0 == g_strcmp0 (argv[i], "--")) break;
      }
      for (; i < argc; ++i) {
        append_argv (argv[i]);
      }
    }
    append_argv (NULL);
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
  if (new_fn)   g_free (new_fn);
  if (apl_argv) g_free (apl_argv);
  if (script)   g_free (script);
  
  if (!rc) {
    g_print ("error opening APL: %s\n", error->message);
    return 1;
  }

  g_child_watch_add (apl_pid, apl_exit, NULL);

  apl_expect_network =  TRUE;
  
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
    
    if (-1 != plot_pipe_fd) {
      g_unix_set_fd_nonblocking (plot_pipe_fd, TRUE, NULL);
      g_unix_fd_add (plot_pipe_fd,		// gint fd,
		     G_IO_IN | G_IO_PRI,	// GIOCondition condition,
		     apl_read_plot_pipe,	// GUnixFDSourceFunc function,
		     NULL);			// gpointer user_data
    }
  }
  else {
    g_print ("error opening APL file descriptors.");
    return 1;
  }

  return 0;
}

