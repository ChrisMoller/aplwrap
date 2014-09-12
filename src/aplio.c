#include <gtk/gtk.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <sys/socket.h>
#define _GNU_SOURCE 
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "aplio.h"
#include "apl.h"
#include "txtbuf.h"
#include "history.h"
#include "options.h"
#include "edit.h"
#include "aplwrap.h"
#include "pstat.h"

gint apl_in  = -1;		// to write to apl in
gint apl_out = -1;		// to read from apl out
gint apl_err = -1;		// to read from apl err
gint sockfd  = -1;
guint event_id;

static int at_prompt = FALSE;
static gchar *comm_mode = NULL;
static gint   comm_addr = -1;

int
is_at_prompt ()
{
  return at_prompt;
}

static int
valid_end(char *start, ssize_t size)
{
  char *look = start + size - 1;
  ssize_t tail = 1;
  if (size == 0) return TRUE;
  if ((*look&0xff) < 0x80) return TRUE;
  while (look >= start && tail <= 6) {
    if ((*look&0xc0) != 0x80) break;
    --look; ++tail;
  }
  switch (tail) {
  case 2: if ((*look&0xe0) == 0xc0) return TRUE; break;
  case 3: if ((*look&0xf0) == 0xe0) return TRUE; break;
  case 4: if ((*look&0xf8) == 0xf0) return TRUE; break;
  case 5: if ((*look&0xfc) == 0xf8) return TRUE; break;
  case 6: if ((*look&0xfe) == 0xfc) return TRUE; break;
  default: ;
  }
  return FALSE;
}

#define BUFFER_SIZE     1024

static socket_fcn socket_cb   = NULL;
static window_s *this_window = NULL;

void
set_send_cb (socket_fcn cb, void *tw)
{
  socket_cb   = cb;
  this_window = tw;
}

gboolean
send_apl (const void *buf, size_t len)
{
  ssize_t rv = send (sockfd, buf, len, 0);
  return (rv != -1);
}

gboolean
apl_read_plot_pipe (gint         fd,
		    GIOCondition condition,
		    gpointer     user_data)
{
  static gchar  *text     = NULL;
  static ssize_t text_idx = 0;

  gboolean run = TRUE;
  while(run) {
    text = g_try_realloc (text, (gsize)(text_idx + BUFFER_SIZE));
    *text = 0;
    if (text) {
      errno = 0;
      ssize_t sz = read (fd, &text[text_idx], BUFFER_SIZE);
      if (sz == -1 && (errno == EAGAIN || errno == EINTR)) continue;
      if (sz == -1) run = FALSE;
      text_idx += sz;
      if (sz < BUFFER_SIZE && valid_end(text, text_idx)) run = FALSE;
    }
    else run = FALSE;
  }
  
  if (text_idx > 0) {
    text = g_try_realloc (text, (gsize)(text_idx + 16));
    text[text_idx] = 0;
    gint w, h;
    gdk_pixbuf_get_file_info (text, &w, &h);
    GdkPixbuf *pb = gdk_pixbuf_new_from_file (text, NULL);
    if (pb) {
      GtkTextIter insert_iter;
      GtkTextMark *mark;
      
      mark = gtk_text_buffer_get_insert (buffer);
      gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);
      tagged_insert ("\n", 1, TAG_OUT);
      
      mark = gtk_text_buffer_get_insert (buffer);
      gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);
      gtk_text_buffer_insert_pixbuf (buffer, &insert_iter, pb);
      g_object_unref (pb);
      unlink (text);

#define PROMPT_LINE "\n\n      "
      mark = gtk_text_buffer_get_insert (buffer);
      gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, mark);
      tagged_insert (PROMPT_LINE, -1, TAG_OUT);
      scroll_to_end ();
    }
    g_free (text);
  }

  text = NULL;
  text_idx = 0;
  
  return TRUE;
}

gboolean
apl_read_sockid (gint         fd,
		 GIOCondition condition,
		 gpointer     user_data)
{
  static gchar  *text     = NULL;
  static ssize_t text_idx = 0;

  gboolean run = TRUE;
  while(run) {
    text = g_try_realloc (text, (gsize)(text_idx + BUFFER_SIZE));
    *text = 0;
    if (text) {
      errno = 0;
      ssize_t sz = read (fd, &text[text_idx], BUFFER_SIZE);
      if (sz == -1 && (errno == EAGAIN || errno == EINTR)) continue;
      if (sz == -1) run = FALSE;
      text_idx += sz;
      if (sz < BUFFER_SIZE && valid_end(text, text_idx)) run = FALSE;
    }
    else run = FALSE;
  }
  
  if (text_idx > 0) {
    if (socket_cb) (*socket_cb)(text, this_window);
    g_free (text);
  }

  text = NULL;
  text_idx = 0;
  return TRUE;
}

static gboolean eval = FALSE;
static gchar *eval_result = NULL;
static size_t eval_result_size;
static size_t eval_result_idx;
static void  *eval_state;
static void (*eval_callback)(gchar*, size_t, void*);

gboolean
apl_read_out (gint         fd,
	      GIOCondition condition,
	      gpointer     user_data)
{
  static gchar  *text     = NULL;
  static ssize_t text_idx = 0;

  gboolean run = TRUE;
  while(run) {
    text = g_try_realloc (text, (gsize)(text_idx + BUFFER_SIZE));
    if (text) {
      errno = 0;
      ssize_t sz = read (fd, &text[text_idx], BUFFER_SIZE);
      if (sz == -1 && (errno == EAGAIN || errno == EINTR)) continue;
      if (sz == -1) run = FALSE;
      text_idx += sz;
      if (sz < BUFFER_SIZE && valid_end(text, text_idx)) run = FALSE;
    }
    else run = FALSE;
  }
  
  if (text) {
    if (!eval) {
      gboolean eaten = FALSE;
      if (apl_expect_network) {
#define NW_PARSE "^.*mode:([[:alpha:]]*)[[:space:]]addr:([[:digit:]]*)*.*$"

        int rc;
        regex_t preg;
#define NR_PMATCH 4
        regmatch_t pmatch[NR_PMATCH];
        regcomp (&preg, NW_PARSE, REG_EXTENDED | REG_ICASE);
        rc = regexec (&preg, text, NR_PMATCH, pmatch, 0);
        if (rc == REG_NOERROR) {
          apl_expect_network = FALSE;
          eaten = TRUE;
          comm_mode = g_strndup (&text[pmatch[1].rm_so],
                                 pmatch[1].rm_eo - pmatch[1].rm_so);
          comm_addr = (gint)g_ascii_strtoll (&text[pmatch[2].rm_so], NULL, 0);

          if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            fprintf(stderr, "Error in socket() %d, %s",  // fixme
                    errno, strerror(errno)); 
          }
          else {
            struct sockaddr_in srv_addr;
            srv_addr.sin_family = AF_INET;
            srv_addr.sin_port = htons (comm_addr);
            srv_addr.sin_addr.s_addr = inet_addr ("127.0.0.1");

            if (connect(sockfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr))
                < 0) {
              perror("Error in connect()");
            }
            else {
              g_unix_set_fd_nonblocking (sockfd, TRUE, NULL);
              event_id =
                g_unix_fd_add (sockfd,		  // gint fd,
                               G_IO_IN | G_IO_PRI,  // GIOCondition condition,
                               apl_read_sockid,	  // GUnixFDSourceFunc function,
                               NULL);		  // gpointer user_data
            }
          }
        }
        regfree (&preg);
      }
      if (!eaten) {
        tagged_insert (text, text_idx, TAG_OUT);
        at_prompt = FALSE;
      }
    }
    else { /* eval */
      if (eval_result) {
        if (eval_result_size - eval_result_idx < text_idx)
          eval_result = g_try_realloc (eval_result,
                                       eval_result_size + text_idx);
        if (eval_result)
          memcpy (eval_result + eval_result_idx, text, text_idx);
        eval_result_idx += text_idx;
      }
    }
    g_free (text);
  }

  text = NULL;
  text_idx = 0;
  return TRUE;
}

static pstat stat_begin, stat_delta;

gboolean
apl_read_err (gint         fd,
	      GIOCondition condition,
	      gpointer     user_data)
{
  static gchar  *text     = NULL;
  static ssize_t text_idx = 0;

  gboolean run = TRUE;
  while(run) {
    text = g_try_realloc (text, (gsize)(text_idx + BUFFER_SIZE));
    if (text) {
      errno = 0;
      ssize_t sz = read (fd, &text[text_idx], BUFFER_SIZE);
      if (sz == -1 && (errno == EAGAIN || errno == EINTR)) continue;
      if (sz == -1) run = FALSE;
      text_idx += sz;
      if (sz < BUFFER_SIZE && valid_end(text, text_idx)) run = FALSE;
    }
    else run = FALSE;
  }

  if (text) {
    if (!eval) {
      /* GNU APL prefixes a prompt with a '\r' expecting a move to
         column 0 in the current line. GTK treats '\r' as a newline.
         Remove the '\r' and mimic a carriage return by moving the
         cursor and clearing the line. */
      gboolean prompt_text = text[0] == '\r';
      if (prompt_text) {
        memmove(text, text+1, --text_idx);

	GtkTextIter line_iter, end_iter;
        GtkTextMark *insert = gtk_text_buffer_get_insert (buffer);
        gtk_text_buffer_get_iter_at_mark (buffer, &line_iter, insert);
        end_iter = line_iter;
        gtk_text_iter_set_line_offset (&line_iter, 0);
        gtk_text_buffer_place_cursor (buffer, &line_iter);
        gtk_text_buffer_delete (buffer, &line_iter, &end_iter);
      }

      at_prompt = !strncmp("      ", text+text_idx-6, 6);
      if (at_prompt) {
        gchar *rows_assign = get_rows_assign ();
        if (rows_assign) apl_eval (rows_assign, -1, NULL, NULL);
        history_start();
        get_pstat (get_apl_pid (), &stat_delta);
        diff_pstat (&stat_begin, &stat_delta);
        update_status_line (format_pstat (&stat_delta));
      }
      tagged_insert(text, text_idx,
                    nocolour ? TAG_OUT : (prompt_text ? TAG_PRM : TAG_ERR));
    }
    else { /* eval */
      eval = !!strncmp("\r      ", text+text_idx-7, 7);
      if (!eval) {
        if (eval_callback)
          (*eval_callback)(eval_result, eval_result_idx, eval_state);
        else
          apl_eval_end ();
      }
    }
    g_free (text);
    text = NULL;
    text_idx = 0;
  }

  text = NULL;
  text_idx = 0;
  return TRUE;
}

void
apl_send_inp (gchar  *text,
              ssize_t sz)
{
  ssize_t __attribute__ ((unused)) wrc;
  char nl = '\n';

  if (!eval && at_prompt) {
    update_status_line ("Working…");
    get_pstat (get_apl_pid (), &stat_begin);
  }
  wrc = write (apl_in, text, sz);
  wrc = write (apl_in, &nl, 1);
}

void
apl_eval (gchar  *expr,
          gint    len,
          void  (*callback)(gchar *result, size_t idx, void *state),
          void   *state)
{
  if (eval_result == NULL) {
    eval_result_size = BUFFER_SIZE;
    eval_result = g_try_malloc (eval_result_size);
  }
  eval_result_idx = 0;
  if (eval_result) {
    if (len < 0) len = strlen(expr);
    eval_state = state;
    eval_callback = callback;
    eval = TRUE;
    apl_send_inp (expr, len);
  }
}

void
apl_eval_end ()
{
  g_free (eval_result);
  eval_result = 0;
}

