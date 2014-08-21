#include "history.h"

typedef struct _history {
  gchar *command;
  struct _history *prev;
  struct _history *next;
} history;

typedef enum {
  prev, next
} direction;

static struct {
  history *current;
  direction last_direction;
} history_cursor = { NULL, prev };

static history *command_history         = NULL;

void
history_insert (gchar *command, ssize_t length)
{
  history *history = g_try_malloc(sizeof(history));
  if (history) {
    history->command = g_strndup(command, length);
    if (history->command) {
      if (command_history) {
        history->prev = command_history;
        history->next = NULL;
        command_history->next = history;
        command_history = history;
      }
      else {
        history->prev = NULL;
        history->next = NULL;
        command_history = history;
      }
    }
    else
      g_free(history);
  }
}

gchar*
history_prev ()
{
  if (history_cursor.current) {
    if (history_cursor.last_direction == next &&
        history_cursor.current->next &&
        history_cursor.current->prev)
      history_cursor.current = history_cursor.current->prev;
    history_cursor.last_direction = prev;
    gchar *cmd = history_cursor.current->command;
    if (history_cursor.current->prev)
      history_cursor.current = history_cursor.current->prev;
    return cmd;
  }
  else
    return "";
}

gchar*
history_next ()
{
  if (history_cursor.current) {
    if (history_cursor.last_direction == prev &&
        history_cursor.current->prev &&
        history_cursor.current->next)
      history_cursor.current = history_cursor.current->next;
    history_cursor.last_direction = next;
    if (history_cursor.current->next)
      history_cursor.current = history_cursor.current->next;
    else
      return "";
    return history_cursor.current->command;
  }
  else
    return "";
}

void
history_start ()
{
  history_cursor.current = command_history;
  history_cursor.last_direction = prev;
}
