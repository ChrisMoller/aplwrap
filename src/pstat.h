#ifndef PSTAT_H
#define PSTAT_H

#include <gtk/gtk.h>

typedef struct _pstat {
  /* Application use */
  unsigned sequence;
  /* From gettimeofday() */
  long long wall_ticks;
  /* From /proc/<pid>/stat */
  long utime;
  long stime;
  long long start_time; /* seconds * 100 since epoch */
  long vsize;
  long rssize;
  long minflt;
  long majflt;
  long biowait;
  /* From /proc/<pid>/io */
  long rchar;
  long wchar;
  long syscr;
  long syscw;
  long read_bytes;
  long write_bytes;
  long cancelled_write_bytes;
} pstat;

void  get_pstat (GPid   pid,
                 pstat *stats);

void  diff_pstat (pstat *old,
                  pstat *new);

char* format_pstat (pstat *stats);

#endif
