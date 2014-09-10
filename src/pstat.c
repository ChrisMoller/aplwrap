#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "pstat.h"

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

/*
  From man 5 proc:

  pid comm state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt
  cmajflt utime stime cutime cstime priority nice num_threads itrealvalue
  starttime vsize rss rsslim startcode endcode startstack kstkesp kstkeip
  signal blocked sigignore sigcatch wchan nswap cnswap exit_signal processor
  rt_priority policy delayacct_blkio_ticks guest_time cguest_time
*/
#define SCANFMT                                                 \
  "%*d (%*[^)]) %*c %*d %*d %*d %*d %*d %*u %lu %lu %lu "       \
  "%lu %lu %lu %ld %ld %*d %*d %*d %*d "                        \
  "%*u %lu %ld %*u %*u %*u %*u %*u %*u "                        \
  "%*u %*u %*u %*u %*u %*u %*u %*d %*d"                         \
  "%*u %*u %Lu %*u %*u"

void
get_pstat (GPid   pid,
           pstat *stats)
{
  char path[20];
  FILE *fp;
  unsigned long minflt, cminflt, majflt, cmajflt, utime, stime, vsize;
  long cutime, cstime;
  unsigned long long biowait;
  long int rss;
  int conv;

  memset(stats, sizeof(struct _pstat), 0);
  if (pid < 0) return;
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);
  fp = fopen(path, "r");
  if (fp == NULL) return;
  conv = fscanf(fp, SCANFMT, &minflt, &cminflt, &majflt, &cmajflt,
                &utime, &stime, &cutime, &cstime, &vsize, &rss, &biowait);
  fclose(fp);
  if (conv != 11) return;
  stats->utime   = utime + cutime;
  stats->stime   = stime + cstime;
  stats->vsize   = vsize;
  stats->rssize  = rss;
  stats->minflt  = minflt + cminflt;
  stats->majflt  = majflt + cmajflt;
  stats->biowait = biowait;
}

void
diff_pstat (pstat *old,
            pstat *new)
{
  /* Subtract old from new, leaving result in new. */
  new->utime   -= old->utime;
  new->stime   -= old->stime;
  new->vsize   -= old->vsize;
  new->rssize  -= old->rssize;
  new->minflt  -= old->minflt;
  new->majflt  -= old->majflt;
  new->biowait -= old->biowait;
}

char*
format_pstat (pstat *stats)
{
  static char buf[100];
  char tmp[20];
  int out = 0;
  long tps, pgsz;

  tps = sysconf(_SC_CLK_TCK);
  pgsz = sysconf(_SC_PAGESIZE);
  snprintf(tmp, sizeof(tmp), "%.2f", stats->utime/(float)tps);
  out += snprintf(buf+out, sizeof(buf)-out, "u: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%.2f", stats->stime/(float)tps);
  out += snprintf(buf+out, sizeof(buf)-out, "s: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->vsize);
  out += snprintf(buf+out, sizeof(buf)-out, "v: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%ld", stats->rssize*pgsz);
  out += snprintf(buf+out, sizeof(buf)-out, "r: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->minflt);
  out += snprintf(buf+out, sizeof(buf)-out, "f: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->majflt);
  out += snprintf(buf+out, sizeof(buf)-out, "F: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%.2f", stats->biowait/100.0);
  out += snprintf(buf+out, sizeof(buf)-out, "b: %s ", commas(tmp));
  return buf;
}
