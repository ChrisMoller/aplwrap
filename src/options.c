#include "options.h"

#define XEQ_FALLBACK	"apl"

#define FT_SIZE_FALLBACK	12
gint ft_size = FT_SIZE_FALLBACK;

#define WIDTH_FALLBACK	680
gint width = WIDTH_FALLBACK;

#define HEIGHT_FALLBACK	440
gint height = HEIGHT_FALLBACK;

gchar *bg_colour = NULL;
gchar *fg_colour = NULL;

gboolean vwidth       = FALSE;
gboolean nocolour     = FALSE;
gboolean printversion = FALSE;
gchar *new_fn = NULL;
gchar *opt_lx = NULL;
gchar *opt_load = NULL;
gchar *rows_v = NULL;
gchar *script = NULL;
gchar *shortcuts_file = NULL;

GOptionEntry entries[] = {
  { "ftsize", 's', 0, G_OPTION_ARG_INT,
    &ft_size,
    "Font size in points [integer]",
    NULL },
  { "width", 'w', 0, G_OPTION_ARG_INT,
    &width,
    "Width in pixels [integer]",
    NULL },
  { "height", 'h', 0, G_OPTION_ARG_INT,
    &height,
    "Height in pixels [integer]",
    NULL },
  { "vwidth", 'v', 0, G_OPTION_ARG_NONE,
    &vwidth,
    "Use variable width font",
    NULL },
  { "nocolour", 'n', 0, G_OPTION_ARG_NONE,
    &nocolour,
    "Turn off coloured error text",
    NULL },
  { "xeq", 'x', 0, G_OPTION_ARG_FILENAME,
    &new_fn,
    "Path to the GNU APL executable [string]",
    NULL },
  { "LX", 0, 0, G_OPTION_ARG_STRING,
    &opt_lx,
    "APL startup command or expression [string]",
    NULL },
  { "load", 'L', 0, G_OPTION_ARG_STRING,
    &opt_load,
    "Preload the specified workspace [string]",
    NULL },
  { "rows-var", 0, 0, G_OPTION_ARG_STRING,
    &rows_v,
    "APL variable name for display row count [string]",
    NULL },
  { "shortcuts", 0, 0, G_OPTION_ARG_STRING,
    &shortcuts_file,
    "file chooser directory shortcuts file [path]",
    NULL },
  { "file", 'f', 0, G_OPTION_ARG_STRING,
    &script,
    "APL script file [path]",
    NULL },
  { "version", 0, 0, G_OPTION_ARG_NONE,
    &printversion,
    "Print version and exit",
    NULL },
  { "background", 0, 0, G_OPTION_ARG_STRING,
    &bg_colour,
    "background colour",
    NULL },
  { "foreground", 0, 0, G_OPTION_ARG_STRING,
    &fg_colour,
    "foreground colour",
    NULL },
  { NULL }
};
