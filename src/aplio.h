#ifndef APLIO_H
#define APLIO_H

extern gint apl_in;
extern gint apl_out;
extern gint apl_err;

extern int at_prompt;
extern ssize_t prompt_len;

gboolean apl_read_out (gint fd,
                       GIOCondition condition,
                       gpointer user_data);

gboolean apl_read_err (gint fd,
                       GIOCondition condition,
                       gpointer user_data);

void apl_send_inp (gchar *text, ssize_t sz);

#endif
