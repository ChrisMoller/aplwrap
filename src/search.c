#include "../config.h"

#include <gtk/gtk.h>

#include "search.h"

typedef enum {
  SEARCH_FORWARD,
  SEARCH_BACKWARD
} search_direction_t;

search_context_t*
new_search_context (GtkWidget     *search_bar,
                    GtkWidget     *view,
                    GtkTextBuffer *buffer)
{
  search_context_t *cxt = g_malloc0 (sizeof(search_context_t));
  cxt->search_bar = search_bar;
  cxt->view = view;
  cxt->buffer = buffer;
  return cxt;
}

static void
do_search (search_direction_t direction, search_context_t *cxt)
{
  gboolean found;
  switch (direction) {
  case SEARCH_FORWARD:
    found = gtk_text_iter_forward_search (&cxt->search_start, cxt->search_text,
                                          GTK_TEXT_SEARCH_CASE_INSENSITIVE,
                                          &cxt->match_start, &cxt->match_end,
                                          NULL);
    break;
  case SEARCH_BACKWARD:
    found = gtk_text_iter_backward_search (&cxt->search_start, cxt->search_text,
                                           GTK_TEXT_SEARCH_CASE_INSENSITIVE,
                                           &cxt->match_start, &cxt->match_end,
                                           NULL);
    break;
  }
  if (found) {
    gtk_text_buffer_select_range (cxt->buffer,
                                  &cxt->match_start, &cxt->match_end);
    gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (cxt->view), &cxt->match_start,
                                  0.0, FALSE, 0.0, 0.0);
  }
}

gboolean
search_key_press_event (GtkWidget *widget,
                        GdkEvent  *event,
                        gpointer   user_data)
{
  GdkEventKey *key_event = (GdkEventKey *)event;
  GdkModifierType mod_mask = gtk_accelerator_get_default_mod_mask ();
  search_context_t *cxt = user_data;

  /* If the search bar is hidden but still has focus (as can happen if
     the search bar is hidden using the Escape key or the close box),
     force the focus back to the transcript. Note that the key event
     is consumed. */
  if (!gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (cxt->search_bar))) {
    gtk_widget_grab_focus (cxt->view);
    return FALSE;
  }

  /* Control-F disables search mode */
  if (key_event->keyval == GDK_KEY_f &&
      (key_event->state & mod_mask) == GDK_CONTROL_MASK) {
    gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (cxt->search_bar), FALSE);
    gtk_widget_grab_focus (cxt->view);
    return TRUE;
  }

  /* Control-G repeats search forward */
  if (key_event->keyval == GDK_KEY_g &&
      (key_event->state & mod_mask) == GDK_CONTROL_MASK) {
    cxt->search_start = cxt->match_end;
    do_search (SEARCH_FORWARD, cxt);
    return TRUE;
  }

  /* Shift-Control-G repeats search backward */
  if (key_event->keyval == GDK_KEY_G &&
      (key_event->state & mod_mask) == (GDK_SHIFT_MASK|GDK_CONTROL_MASK)) {
    cxt->search_start = cxt->match_start;
    do_search (SEARCH_BACKWARD, cxt);
    return TRUE;
  }

  return FALSE;
}

void
search_changed_event (GtkSearchEntry *entry,
                      gpointer        user_data)
{
  search_context_t *cxt = user_data;
  cxt->search_text = gtk_entry_get_text (GTK_ENTRY (entry));
  if (cxt->search_text[0] == '\0') return;
  gtk_text_buffer_get_start_iter (cxt->buffer, &cxt->search_start);
  do_search(SEARCH_FORWARD, cxt);
}
