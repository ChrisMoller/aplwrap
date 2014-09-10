#ifndef PSTAT_H
#define PSTAT_H

#include <gtk/gtk.h>

typedef struct _pstat {
  long utime;
  long stime;
  long vsize;
  long rssize;
  long minflt;
  long majflt;
  long biowait;
} pstat;

void  get_pstat (GPid   pid,
                 pstat *stats);

void  diff_pstat (pstat *old,
                  pstat *new);

char* format_pstat (pstat *stats);

#endif
