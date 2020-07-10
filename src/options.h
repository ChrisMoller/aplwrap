#ifndef OPTIONS_H
#define OPTIONS_H

#include <gtk/gtk.h>

#define BG_COLOUR_FALLBACK	"bisque"
#define FG_COLOUR_FALLBACK	"black"
#define XEQ_FALLBACK		"apl"
#define FT_SIZE_FALLBACK	12
#define WIDTH_FALLBACK		680
#define HEIGHT_FALLBACK		440
#define EDIF_FALLBACK		FALSE
#define EDIF_NAME_FALLBACK	"ed2"

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
extern gchar       *edif_name;
extern GOptionEntry entries[];

#endif
