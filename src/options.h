#ifndef OPTIONS_H
#define OPTIONS_H

#include <gtk/gtk.h>

extern gint         ft_size;
extern gint         width;
extern gint         height;
extern gboolean     printversion;
extern gboolean     vwidth;
extern gboolean     nocolour;
extern gchar       *new_fn;
extern gchar       *opt_lx;
extern gchar       *script;
extern gchar       *rows_v;
extern GOptionEntry entries[];

#endif
