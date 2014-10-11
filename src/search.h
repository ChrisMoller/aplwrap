#ifndef SEARCH_H
#define SEARCH_H

#include <gtk/gtk.h>

#define ENABLE_SEARCH GTK_CHECK_VERSION(3,10,0)

#if ENABLE_SEARCH
typedef struct {
  GtkWidget *search_bar;
  GtkTextIter search_start, match_start, match_end;
  const gchar *search_text;
  GtkTextBuffer *buffer;
  GtkWidget *view;
} search_context_t;

search_context_t* new_search_context (GtkWidget     *search_bar,
                                      GtkWidget     *view,
                                      GtkTextBuffer *buffer);

gboolean search_key_press_event (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer   user_data);

void search_changed_event (GtkSearchEntry *entry,
                           gpointer        user_data);
#endif

#endif
