#ifndef HISTORY_H
#define HISTORY_H

#include <glib-unix.h>
#include <unistd.h>

void history_insert (gchar  *command,
                     ssize_t length);

gchar* history_prev ();

gchar* history_next ();

void history_start ();

#endif
