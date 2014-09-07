#include <string.h>

#include "complete.h"
#include "aplio.h"
#include "txtbuf.h"
#include "aplwrap.h"

#define TRACE 0
#if TRACE
#include <stdio.h>
#endif

/* Evaluate this expression in APL to define the tab completion
  function `z←i ___ p` . */
static gchar* aplfx="⎕fx 'z←i ___ p;m'"
  " '⍝ aplwrap tab-completion helper'"
  " '→(0≤i)/F'"
  " '→(3≠⎕nc ''__'')/D'"
  " '_←__ p'"
  " '→0'"
  " 'D: _←⎕nl 2 3 4 5 6'"
  " '→0'"
  " 'F: m←((⎕io=,1↑[⎕io+1]p⍷_)⌿_)⍪'' '''"
  " 'z←((↑⍴m)|i+1) (('' ''≠z)/z←m[((⎕io-1)+↑⍴m)⌊⎕io⌈⎕io+i;])'";

/* Evaluate this expression in APL to determine the presence of the
   ___ function; 1 = yes, 0 = no. */
static gchar* aplck="3=⎕nc '___'";

static int
is_ident_char (gunichar uch)
{
  if (uch >= L'0' && uch <= L'9') return 1;
  if (uch >= L'A' && uch <= L'Z') return 1;
  if (uch >= L'a' && uch <= L'z') return 1;
  if (uch == L'_') return 1;
  if (uch == L'¯') return 1;
  if (uch == L'∆') return 1;
  if (uch == L'⍙') return 1;
  if (uch == L'⎕') return 1;

  return 0;
}

#define PSIZE 8

typedef struct _state {
  gchar *context;
  size_t context_length;
  gchar *prior_context;
  size_t prior_context_length;
  gchar *prefix;
  size_t prefix_length;
  gchar *prior_prefix;
  size_t prior_prefix_length;
  int    prefix_chars;
  gchar  position[PSIZE];
  int    refresh_context;
} state_t;

static state_t state = { 0 };

GtkTextMark *completion_begin = 0;
GtkTextMark *completion_end = 0;
GtkTextMark *completion_mark = 0;

static void
replace_completion (gchar *text, size_t length, state_t *state)
{
  /* Replace the text between completion bounds. The cursor, which is
     always contained within the bounds, is unmoved. */
  GtkTextIter start, end, cursor;
  GtkTextMark *insert = gtk_text_buffer_get_insert (buffer);
  int i = state->prefix_chars;
  gtk_text_buffer_get_iter_at_mark (buffer, &cursor, insert);
  gtk_text_buffer_get_iter_at_mark (buffer, &start, completion_begin);
  gtk_text_buffer_get_iter_at_mark (buffer, &end, completion_end);
  gtk_text_buffer_delete (buffer, &start, &end);
  gtk_text_buffer_get_iter_at_mark (buffer, &start, completion_begin);
  gtk_text_buffer_insert (buffer, &start, text, length);
  gtk_text_buffer_get_iter_at_mark (buffer, &cursor, completion_begin);
  while (i--)
    gtk_text_iter_forward_char (&cursor);
  gtk_text_buffer_place_cursor (buffer, &cursor);
  gtk_text_buffer_move_mark (buffer, completion_mark, &cursor);
}

/*
  The following is a chain of calls out to APL to get the tab
  completion. The chain changes depending upon current state; see
  aplck_callback().

  We receive each apl_eval() result via a callback. The callback
  *asynchronously* processes the result and, as appropriate, invokes
  the next eval in the sequence.

  The result buffer is allocated by the first apl_eval() call in the
  sequence and freed by the call to apl_eval_end() after the final
  callback in the sequence completes.
*/

static void
lookup_callback (gchar *result, size_t idx, void *state)
{
  /* Parse the next lookup index */
  size_t i = 0, b, e;
  while (i < idx && result[i] == ' ') ++i;
  b = i;
  if (!strncmp(&result[i], "¯", strlen("¯"))) i += strlen("¯");
  while (i < idx && result[i] >= '0' && result[i] <= '9') ++i;
  e = i;
  if (e == b) {
    strcpy(((state_t*)state)->position, "0");
  }
  else if (e-b < PSIZE) {
    memset(((state_t*)state)->position, 0, PSIZE);
    strncpy(((state_t*)state)->position, &result[b], e-b);
  }
  if (e > b) {
    /* Parse the completion text */
    while (i < idx && result[i] == ' ') ++i;
    b = i;
    while (i < idx && result[i] != ' ' && result[i] != '\n') ++i;
    e = i;
    /* Update the completion */
    if (e-b) {
#if TRACE
      printf("completion [%3s] (%2d): %.*s\n", ((state_t*)state)->position,
             (int)(e-b), (int)(e-b), result+b);
#endif
      replace_completion (result+b, e-b, state);
    }
    else
      beep (); /* no completion */
  }
  else
    beep (); /* no index */

  ((state_t*)state)->refresh_context = FALSE;
  apl_eval_end();
}

#define LOOKUPEXPR "%s ___ '%.*s'"

#define SIZE 200

static void
lookup (struct _state *state)
{
  gchar expr[SIZE];
  int len;
  len = snprintf(expr, SIZE, LOOKUPEXPR,
                 state->position, (int)state->prefix_length, state->prefix);
  if (len > 0 && len < SIZE)
    apl_eval (expr, len, lookup_callback, state);
}

static void
context_callback(gchar *result, size_t idx, void *state)
{
  strncpy(((state_t*)state)->position, "0", PSIZE);
  lookup (state);
}

#define CTXEXPR "¯1 ___ '%.*s'"

static void
set_context (struct _state *state)
{
  gchar expr[SIZE];
  int len;
  len = snprintf(expr, SIZE, CTXEXPR,
                 (int)state->context_length, state->context);
  if (len > 0 && len < SIZE)
    apl_eval (expr, len, context_callback, state);
}

static void
aplfx_callback (gchar *result, size_t idx, void *state)
{
  set_context (state);
}

static void
aplck_callback (gchar *result, size_t idx, void *state)
{
  if (result[0] == '0')
    apl_eval (aplfx, -1, aplfx_callback, state);
  else if (((state_t*)state)->refresh_context)
    set_context (state);
  else
    lookup (state);
}

static void
run_completer (struct _state *state)
{
  if (((state_t*)state)->prefix_length > 0)
    apl_eval(aplck, -1, aplck_callback, state);
}

int
cursor_in_input_area ()
{
  GtkTextMark *insert = gtk_text_buffer_get_insert (buffer);
  GtkTextIter cursor;
  gint buffer_lines = gtk_text_buffer_get_line_count (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &cursor, insert);
  gint cursor_line = gtk_text_iter_get_line (&cursor);
  return (buffer_lines - 1) == cursor_line;
}

void complete ()
{
  if (is_at_prompt () &&
      cursor_in_input_area () &&
      !gtk_text_buffer_get_has_selection (buffer)) {
    GtkTextIter cursor_iter, context_iter, prefix_iter, end_iter;
    GtkTextIter buff_iter, last_mark_iter;
    int prefix_chars = 0;
    
    /* Establish marks to be used for bounding the completion. */
    gtk_text_buffer_get_end_iter (buffer, &buff_iter);
    if (completion_begin == NULL)
      completion_begin =
        gtk_text_buffer_create_mark (buffer,
                                     NULL,
                                     &buff_iter,
                                     TRUE);
    if (completion_end == NULL)
      completion_end =
        gtk_text_buffer_create_mark (buffer,
                                     NULL,
                                     &buff_iter,
                                     FALSE);
    
    /* Establish a mark to be used to detect when the cursor position has
       changed between completions. */
    if (completion_mark == NULL)
      completion_mark =
        gtk_text_buffer_create_mark (buffer,
                                     NULL,
                                     &buff_iter,
                                     FALSE);

    GtkTextMark *insert = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &cursor_iter, insert);
    prefix_iter = end_iter = context_iter = cursor_iter;
    
    do {
      gtk_text_iter_backward_char (&prefix_iter);
      ++prefix_chars;
    } while (is_ident_char (gtk_text_iter_get_char (&prefix_iter)));

    if (gtk_text_iter_equal (&cursor_iter, &prefix_iter)) return;

    gtk_text_iter_forward_char(&prefix_iter);
    --prefix_chars;
    state.prefix_chars = prefix_chars;

    while (is_ident_char (gtk_text_iter_get_char (&end_iter)))
      gtk_text_iter_forward_char(&end_iter);

    /* Set marks around the identifier to be replaced by
       completion. */
    gtk_text_buffer_move_mark (buffer, completion_begin, &prefix_iter);
    gtk_text_buffer_move_mark (buffer, completion_end, &end_iter);

    while (gtk_text_iter_get_line_offset(&context_iter) > 6)
      gtk_text_iter_backward_char(&context_iter);

    /* Save the previous context and prefix for comparison. */
    if (state.prior_context) g_free (state.prior_context);
    state.prior_context = state.context;
    state.prior_context_length = state.context_length;
    if (state.prior_prefix) g_free (state.prior_prefix);
    state.prior_prefix = state.prefix;
    state.prior_prefix_length = state.prefix_length;

    /*
      context: context_iter .. prefix_iter
      prefix:  prefix_iter  .. cursor_iter
      begin:   prefix_iter
      end:     end_iter
    */
    state.context = gtk_text_buffer_get_text (buffer,
                                              &context_iter,
                                              &prefix_iter,
                                              FALSE);
    if (state.context == NULL) return;
    state.context_length = strlen (state.context);

    state.prefix = gtk_text_buffer_get_text (buffer,
                                             &prefix_iter,
                                             &cursor_iter,
                                             FALSE);
    if (state.prefix == NULL) { g_free (state.context); return; }
    state.prefix_length = strlen (state.prefix);

    /* If the cursor position has changed since the last completion,
       renew the context and restart the iteration. */
    gtk_text_buffer_get_iter_at_mark (buffer,
                                      &last_mark_iter,
                                      completion_mark);
    if (!gtk_text_iter_equal (&last_mark_iter, &cursor_iter) ||
        state.context_length != state.prior_context_length ||
        state.prefix_length  != state.prior_prefix_length ||
        strcmp(state.context, state.prior_context) ||
        strcmp(state.prefix,  state.prior_prefix)) {
#if TRACE
      puts("reset");
#endif
      state.refresh_context = TRUE;
    }

#if TRACE
    printf("context: %s\nprefix:  %s\n", state.context, state.prefix);
#endif

    /* Run the completion. */
    run_completer (&state);
  }
}

void
cursor_to_completion_end ()
{
  if (completion_end &&
      is_at_prompt () &&
      cursor_in_input_area () &&
      !gtk_text_buffer_get_has_selection (buffer)) {
    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark (buffer, &cursor, completion_end);
    gtk_text_buffer_place_cursor (buffer, &cursor);
  }
}
