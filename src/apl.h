#ifndef APL_H
#define APL_H

extern GPid apl_pid;

int apl_spawn (int argc, char *argv[]);

void gapl2_quit (GtkWidget *widget,
                 gpointer   data);

#endif
