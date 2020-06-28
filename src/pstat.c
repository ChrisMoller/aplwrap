#include "../config.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "pstat.h"
#include "menu.h"
#include "aplio.h"
#include "aplwrap.h"

static GtkWidget *pstat_grid = NULL;

typedef struct {
  const gchar *label;
  GtkWidget   *value;
} pstat_ety_s;

pstat_ety_s pstat_etys[] = {
  {"# sequence",	NULL},
  {"∆ elapsed time",	NULL},
  {"∆ user time",	NULL},
  {"∆ system time",	NULL},
  {"∆ virtual size",	NULL},
  {"∆ resident size",	NULL},
  {"∆ minor faults",	NULL},
  {"∆ major faults",	NULL},
  {"∆ block io wait",	NULL},
  {"∆ read chars",	NULL},
  {"∆ write chars",	NULL},
  {"∆ read bytes",	NULL},
  {"∆ write bytes",	NULL},
  {"∆ syscalls read",	NULL},
  {"∆ syscalls write",	NULL},
  {"∆ canceled bytes",	NULL},
};

void
pstat_destroy (GtkWidget *widget,
               gpointer   user_data)
{
  gtk_widget_destroy (GTK_WIDGET (widget));
  pstat_grid = NULL;
}

void
set_pstat_value (gint idx, const gchar *val)
{
  if (!pstat_grid) return;
  if (idx >= 0 && idx < PSTAT_MAX && pstat_etys[idx].value) 
    gtk_label_set_text (GTK_LABEL (pstat_etys[idx].value), val);
}

void
show_pstat (GtkWidget *widget,
            gpointer   user_data)
{
  GtkWidget *window;
  GtkWidget *vbox;
  static GtkCssProvider *provider = NULL;

  if (pstat_grid) return;

  window =  gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  gtk_window_set_title (GTK_WINDOW (window), _ ("Pstat"));
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  if (!provider) {
    provider = gtk_css_provider_new ();
#define CSS_STRING "* { background-color: white; color: black; }"
    gtk_css_provider_load_from_data (provider, CSS_STRING, -1, NULL);
  }
  gtk_style_context_add_provider (gtk_widget_get_style_context (window),
                                      GTK_STYLE_PROVIDER (provider),
                                      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  pstat_grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (pstat_grid), 12);

  for (int i = 0; i < sizeof(pstat_etys) / sizeof(pstat_ety_s); i++) {
    GtkWidget *lbl = gtk_label_new (pstat_etys[i].label);
    gtk_widget_set_halign (lbl, GTK_ALIGN_END);
    GtkWidget *val = gtk_label_new ("");
    gtk_widget_set_halign (val, GTK_ALIGN_START);
    pstat_etys[i].value = val;
    gtk_grid_attach (GTK_GRID (pstat_grid), lbl, 0, i, 1, 1);
    gtk_grid_attach (GTK_GRID (pstat_grid), val, 1, i, 1, 1);
  }
  update_pstat_strings ();
  
  g_signal_connect (window, "destroy",
		    G_CALLBACK (pstat_destroy), NULL);

  gtk_box_pack_start (GTK_BOX (vbox), pstat_grid, FALSE, FALSE, 2);
  gtk_widget_show_all (window);
}

void
ps_toggle_cb (GtkToggleButton *togglebutton,
	      gpointer         user_data)
{
  show_status = gtk_toggle_button_get_active (togglebutton);
  set_status_visibility (show_status);
}
  
/*
  Get user time, system time, virtual size, resident set size, minor
  faults count, major faults count and aggregate block I/O delay time
  from APL process. Children and thread of APL are included.

  This code is Linux-specific.
*/

static char*
commas(char *str) {
  /* Add comma separators to a number string */
  static char result[80];
  char *dot;
  size_t len, whole, tail;
  unsigned i, j, k;
  dot = strchr(str, '.');
  len = strlen(str);
  tail = dot ? str + len - dot : 0;
  whole = len - tail;
  for (i = 0, j = 0, k = 3 - whole % 3; i < whole;) {
    result[j++] = str[i];
    if (++i < whole && ++k%3 == 0) result[j++] = ',';
  }
  strcpy(&result[j], &str[len-tail]);
  return result;
}

/* System boot time from /proc/stat */
#define SCANF_BTIME "%*[^b]btime %lu"

/* Process stats from /proc/<pid>/stat

  pid comm state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt
  cmajflt utime stime cutime cstime priority nice num_threads itrealvalue
  starttime vsize rss rsslim startcode endcode startstack kstkesp kstkeip
  signal blocked sigignore sigcatch wchan nswap cnswap exit_signal processor
  rt_priority policy delayacct_blkio_ticks guest_time cguest_time
*/
#define SCANFMT_STAT                                            \
  "%*d (%*[^)]) %*c %*d %*d %*d %*d %*d %*u %lu %lu %lu "       \
  "%lu %lu %lu %ld %ld %*d %*d %*d %*d "                        \
  "%Lu %lu %ld %*u %*u %*u %*u %*u %*u "                        \
  "%*u %*u %*u %*u %*u %*u %*u %*d %*d"                         \
  "%*u %*u %Lu %*u %*u"

/* Process I/O stats from /proc/<pid>/io */
#define SCANFMT_IO                                              \
  "rchar: %lu wchar: %lu syscr: %lu syscw: %lu "                \
  "read_bytes: %lu write_bytes: %lu cancelled_write_bytes: %lu"

void
get_pstat (GPid   pid,
           pstat *stats)
{
  struct timeval wall_time;
  char * path = NULL;
  FILE *fp;
  unsigned long btime;
  unsigned long minflt, cminflt, majflt, cmajflt, utime, stime, vsize;
  long cutime, cstime;
  unsigned long long start_time, biowait;
  long int rss;
  int conv;
  unsigned long rchar, wchar, syscr, syscw, read_bytes, write_bytes;
  unsigned long cancelled_write_bytes;

  memset(stats, 0, sizeof(struct _pstat));
  if (pid < 0) return;
  gettimeofday(&wall_time, NULL);
  stats->wall_ticks = wall_time.tv_sec * 100 + wall_time.tv_usec / 10000;
  fp = fopen("/proc/stat", "r");
  if (fp == NULL) return;
  conv = fscanf(fp, SCANF_BTIME, &btime);
  fclose(fp);
  if (conv != 1) return;
  asprintf(&path, "/proc/%d/stat", pid);
  fp = fopen(path, "r");
  free (path);
  if (fp == NULL) return;
  conv = fscanf(fp, SCANFMT_STAT, &minflt, &cminflt, &majflt, &cmajflt,
                &utime, &stime, &cutime, &cstime, &start_time,
                &vsize, &rss, &biowait);
  fclose(fp);
  if (conv != 12) return;
  stats->utime      = utime + cutime;
  stats->stime      = stime + cstime;
  stats->start_time = (start_time / sysconf(_SC_CLK_TCK) + btime) * 100;
  stats->vsize      = vsize;
  stats->rssize     = rss;
  stats->minflt     = minflt + cminflt;
  stats->majflt     = majflt + cmajflt;
  stats->biowait    = biowait;
#if 1
  asprintf(&path, "/proc/%d/stat", pid);
#else
  snprintf(path, sizeof(path), "/proc/%d/io", pid);
#endif
  fp = fopen(path, "r");
  free (path);
  if (fp == NULL) return;
  conv = fscanf(fp, SCANFMT_IO, &rchar, &wchar, &syscr, &syscw,
                &read_bytes, &write_bytes, &cancelled_write_bytes);
  fclose(fp);
  if (conv != 7) return;
  stats->rchar                 = rchar;
  stats->wchar                 = wchar;
  stats->syscr                 = syscr;
  stats->syscw                 = syscw;
  stats->read_bytes            = read_bytes;
  stats->write_bytes           = write_bytes;
  stats->cancelled_write_bytes = cancelled_write_bytes;
}

void
diff_pstat (pstat *old,
            pstat *new)
{
  /* Subtract old from new, leaving result in new. */
  if (old->wall_ticks == 0)
    new->wall_ticks          -= new->start_time;
  else
    new->wall_ticks          -= old->wall_ticks;
  new->utime                 -= old->utime;
  new->stime                 -= old->stime;
  new->vsize                 -= old->vsize;
  new->rssize                -= old->rssize;
  new->minflt                -= old->minflt;
  new->majflt                -= old->majflt;
  new->biowait               -= old->biowait;
  new->rchar                 -= old->rchar;
  new->wchar                 -= old->wchar;
  new->syscr                 -= old->syscr;
  new->syscw                 -= old->syscw;
  new->read_bytes            -= old->read_bytes;
  new->write_bytes           -= old->write_bytes;
  new->cancelled_write_bytes -= old->cancelled_write_bytes;
}

char*
format_pstat (pstat *stats)
{
  static char buf[240];
  char tmp[20];
  int out = 0;
  long tps, pgsz;
  char *cs;

  tps = sysconf(_SC_CLK_TCK);
  pgsz = sysconf(_SC_PAGESIZE);
  /* sequence */
  snprintf(tmp, sizeof(tmp), "%u", stats->sequence);
  set_pstat_value (PSTAT_SEQUENCE, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "#%s ", cs);
  /* wall time */
  snprintf(tmp, sizeof(tmp), "%.2f", stats->wall_ticks/100.0);
  set_pstat_value (PSTAT_ELAPSED, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆e: %s ", cs);
  /* stat */
  snprintf(tmp, sizeof(tmp), "%.2f", stats->utime/(float)tps);
  set_pstat_value (PSTAT_UTIME, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆u: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%.2f", stats->stime/(float)tps);
  set_pstat_value (PSTAT_STIME, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆s: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%ld", stats->vsize);
  set_pstat_value (PSTAT_VSIZE, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆v: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%ld", stats->rssize*pgsz);
  set_pstat_value (PSTAT_RSIZE, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆r: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%lu", stats->minflt);
  set_pstat_value (PSTAT_MINFLT, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆f: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%lu", stats->majflt);
  set_pstat_value (PSTAT_MAJFLT, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆F: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%.2f", stats->biowait/100.0);
  set_pstat_value (PSTAT_BIOWAIT, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆b: %s ", cs);
  /* io */
  snprintf(tmp, sizeof(tmp), "%lu", stats->rchar);
  set_pstat_value (PSTAT_RCHAR, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆rc: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%lu", stats->wchar);
  set_pstat_value (PSTAT_WCHAR, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆wc: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%lu", stats->read_bytes);
  set_pstat_value (PSTAT_READ_BYTES, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆rb: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%lu", stats->write_bytes);
  set_pstat_value (PSTAT_WRITE_BYTES, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆wb: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%lu", stats->syscr);
  set_pstat_value (PSTAT_SYSCR, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆ic: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%lu", stats->syscw);
  set_pstat_value (PSTAT_SYSCW, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆oc: %s ", cs);
  snprintf(tmp, sizeof(tmp), "%lu", stats->cancelled_write_bytes);
  set_pstat_value (PSTAT_CANCELED_BYTES, cs = commas(tmp));
  out += snprintf(buf+out, sizeof(buf)-out, "∆cw: %s ", cs);
  return buf;
}
