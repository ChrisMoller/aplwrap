#ifndef APL_H
#define APL_H

gboolean apl_expect_network;

int apl_spawn (int   argc,
               char *argv[]);

GPid get_apl_pid ();

void aplwrap_quit (GtkWidget *widget,
                 gpointer   data);

void apl_interrupt ();

#endif
