#ifndef APL_H
#define APL_H

gboolean apl_expect_network;

int apl_spawn (int   argc,
               char *argv[]);

void aplwrap_quit (GtkWidget *widget,
                 gpointer   data);

void apl_interrupt ();

#endif
