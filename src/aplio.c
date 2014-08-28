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

gint apl_in  = -1;		// to write to apl in
gint apl_out = -1;		// to read from apl out
gint apl_err = -1;		// to read from apl err
gint sockfd  = -1;
guint event_id;

static int at_prompt = FALSE;
static ssize_t prompt_len = 0;
static gchar *comm_mode = NULL;
static gint   comm_addr = -1;

int
is_at_prompt ()
{
  return at_prompt;
}

ssize_t
get_prompt_len ()
{
  return prompt_len;
}

void
reset_prompt_len ()
{
  prompt_len = 0;
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

static gchar *last_out = NULL;  // explained in apl_read_err()

static void (*socket_cb)() = NULL;

void
set_socket_cb (void (*cb)(gchar *text))
{
  socket_cb = cb;
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
    last_out = g_try_malloc(text_idx+1);
    if (last_out) {
      memcpy(last_out, text, text_idx);
      last_out[text_idx] = '\0';
    }

    if (socket_cb) (*socket_cb)(text);
    g_free (text);
  }

  text = NULL;
  text_idx = 0;
  return TRUE;
}

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
    at_prompt = FALSE;
    last_out = g_try_malloc(text_idx+1);
    if (last_out) {
      memcpy(last_out, text, text_idx);
      last_out[text_idx] = '\0';
    }

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
#if  0
	 {
	    int nbyte;
#define MESSAGE "proto\n"
	    if ((nbyte = send(sockfd, MESSAGE, strlen(MESSAGE), 0)) < 0) {
	      perror("Error in send()");
	    }
	    fprintf (stderr, "sent %d bytes\n", nbyte);
	    //	    shutdown(sockfd, SHUT_RDWR);
	    //	    close(sockfd);
	  }
#endif
	}
      }
      regfree (&preg);
    }
    if (!eaten) tagged_insert (text, text_idx, TAG_OUT);
    g_free (text);
  }

  text = NULL;
  text_idx = 0;
  return TRUE;
}

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
    int suppress = FALSE;

    /* GNU APL's stderr is prefixed with a '\r'; GTK treats that as a
       newline. Assume that we're already at the beginning of the line
       and remove the '\r'. */
    if (text[0] == '\r')
      memmove(text, text+1, --text_idx);

    /* GNU APL pushes quote-quad's prompt onto stdin just before
       reading from quote-quad. Then aplwrap sees that prompt and
       echoes it to stdout. After that, GNU APL writes the intended
       prompt to stderr, duplicating the prompt that aplwrap alread
       echoed to stdout. We work around that odd interaction by
       suppressing stderr's output in the case that its text matches
       the last text written to stdout. */
    if (last_out) {
      ssize_t lolen = strlen(last_out);
      suppress = (lolen >= text_idx) ?
        !strncmp(last_out+lolen-text_idx, text, text_idx) :
        !strncmp(last_out, text+text_idx-lolen, lolen);
      /* We also need to finesse the data returned to APL in response
         to a quote-quad input; we must send only the input following
         the prompt. See key_press_event() for the other half of this
         interaction. */
      prompt_len = suppress ? text_idx : 0;
      g_free(last_out);
      last_out = NULL;
    }

    if (!suppress) {
      at_prompt = !strncmp("      ", text+text_idx-6, 6);
      if (at_prompt)
        history_start();
      tagged_insert(text, text_idx, nocolour ? TAG_OUT : TAG_ERR);
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

  wrc = write (apl_in, text, sz);
  wrc = write (apl_in, &nl, 1);
}
