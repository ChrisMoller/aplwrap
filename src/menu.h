#ifndef MENU_H
#define MENU_H

#include <gtk/gtk.h>

extern gboolean show_status;

#define PSTAT_SEQUENCE			 0
#define PSTAT_WALL_TICKS		 1
#define PSTAT_UTIME			 2
#define PSTAT_STIME			 3
#define PSTAT_VSIZE			 4
#define PSTAT_RSIZE			 5
#define PSTAT_MINFLT			 6
#define PSTAT_MAJFLT			 7
#define PSTAT_BIOWAIT			 8
#define PSTAT_RCHAR			 9
#define PSTAT_WCHAR			10
#define PSTAT_READ_BYTES		11
#define PSTAT_WRITE_BYTES		12
#define PSTAT_SYSCR			13
#define PSTAT_SYSCW			14
#define PSTAT_CANCELED_BYTES		15
#define PSTAT_MAX			16

void set_pstat_value (gint idx, const gchar *val);
void build_menubar (GtkWidget *vbox);
void open_object (GtkWidget *widget, gpointer   data);
void new_object (GtkWidget *widget, gpointer   data);
void save_log (GtkWidget *widget, gpointer   data);
void save_log_as (GtkWidget *widget, gpointer   data);

#endif
