#include "options.h"

#define XEQ_FALLBACK	"apl"

#define FT_SIZE_FALLBACK	12
gint ft_size = FT_SIZE_FALLBACK;

#define WIDTH_FALLBACK	680
gint width = WIDTH_FALLBACK;

#define HEIGHT_FALLBACK	440
gint height = HEIGHT_FALLBACK;

gboolean vwidth   = FALSE;
gboolean nocolour = FALSE;
gchar *new_fn = NULL;
gchar *opt_lx = NULL;

GOptionEntry entries[] = {
  { "ftsize", 's', 0, G_OPTION_ARG_INT,
    &ft_size,
    "Font size in points (integer).",
    NULL },
  { "width", 'w', 0, G_OPTION_ARG_INT,
    &width,
    "Width in pixels (integer).",
    NULL },
  { "height", 'h', 0, G_OPTION_ARG_INT,
    &height,
    "Height in pixels (integer).",
    NULL },
  { "vwidth", 'v', 0, G_OPTION_ARG_NONE,
    &vwidth,
    "Use variable width font (boolean switch).",
    NULL },
  { "nocolour", 'n', 0, G_OPTION_ARG_NONE,
    &nocolour,
    "Turn off coloured error text. (boolean switch).",
    NULL },
  { "xeq", 'x', 0, G_OPTION_ARG_FILENAME,
    &new_fn,
    "Set an absolute or on-path executable APL other than the default.",
    NULL },
  { "LX", 0, 0, G_OPTION_ARG_STRING,
    &opt_lx,
    "Invoke APL ⎕LX on startup. (string: APL command or expression)",
    NULL },
  { NULL }
};
