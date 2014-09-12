#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

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
  char path[20];
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
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);
  fp = fopen(path, "r");
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
  snprintf(path, sizeof(path), "/proc/%d/io", pid);
  fp = fopen(path, "r");
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

  tps = sysconf(_SC_CLK_TCK);
  pgsz = sysconf(_SC_PAGESIZE);
  /* sequence */
  snprintf(tmp, sizeof(tmp), "%u", stats->sequence);
  out += snprintf(buf+out, sizeof(buf)-out, "#%s ", commas(tmp));
  /* wall time */
  snprintf(tmp, sizeof(tmp), "%.2f", stats->wall_ticks/100.0);
  out += snprintf(buf+out, sizeof(buf)-out, "∆e: %s ", commas(tmp));
  /* stat */
  snprintf(tmp, sizeof(tmp), "%.2f", stats->utime/(float)tps);
  out += snprintf(buf+out, sizeof(buf)-out, "∆u: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%.2f", stats->stime/(float)tps);
  out += snprintf(buf+out, sizeof(buf)-out, "∆s: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%ld", stats->vsize);
  out += snprintf(buf+out, sizeof(buf)-out, "∆v: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%ld", stats->rssize*pgsz);
  out += snprintf(buf+out, sizeof(buf)-out, "∆r: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->minflt);
  out += snprintf(buf+out, sizeof(buf)-out, "∆f: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->majflt);
  out += snprintf(buf+out, sizeof(buf)-out, "∆F: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%.2f", stats->biowait/100.0);
  out += snprintf(buf+out, sizeof(buf)-out, "∆b: %s ", commas(tmp));
  /* io */
  snprintf(tmp, sizeof(tmp), "%lu", stats->rchar);
  out += snprintf(buf+out, sizeof(buf)-out, "∆rc: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->wchar);
  out += snprintf(buf+out, sizeof(buf)-out, "∆wc: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->read_bytes);
  out += snprintf(buf+out, sizeof(buf)-out, "∆rb: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->write_bytes);
  out += snprintf(buf+out, sizeof(buf)-out, "∆wb: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->syscr);
  out += snprintf(buf+out, sizeof(buf)-out, "∆ic: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->syscw);
  out += snprintf(buf+out, sizeof(buf)-out, "∆oc: %s ", commas(tmp));
  snprintf(tmp, sizeof(tmp), "%lu", stats->cancelled_write_bytes);
  out += snprintf(buf+out, sizeof(buf)-out, "∆cw: %s ", commas(tmp));
  return buf;
}
