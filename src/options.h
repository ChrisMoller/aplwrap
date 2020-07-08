#ifndef OPTIONS_H
#define OPTIONS_H

#include <gtk/gtk.h>

#define BG_COLOUR_FALLBACK	"bisque"
#define FG_COLOUR_FALLBACK	"black"

extern gint         ft_size;
extern gint         width;
extern gint         height;
extern gboolean     printversion;
extern gboolean     vwidth;
extern gboolean     nocolour;
extern gboolean     enable_edif;
extern gchar       *new_fn;
extern gchar       *opt_lx;
extern gchar       *opt_load;
extern gchar       *shortcuts_file;
extern gchar       *script;
extern gchar       *rows_v;
extern gchar       *bg_colour;
extern gchar       *fg_colour;
extern GOptionEntry entries[];

#endif
