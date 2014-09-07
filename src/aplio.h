#ifndef APLIO_H
#define APLIO_H

#include <gtk/gtk.h>

extern gint apl_in;
extern gint apl_out;
extern gint apl_err;
extern gint sockfd;

typedef void (*socket_fcn)(gchar *text, void *tw);
  
gboolean send_apl (const void *buf, size_t len);
void     set_send_cb (socket_fcn fcn, void *tw);

int      is_at_prompt ();

ssize_t  get_prompt_len ();

void     reset_prompt_len ();

gboolean apl_read_out (gint         fd,
                       GIOCondition condition,
                       gpointer     user_data);

gboolean apl_read_err (gint         fd,
		       GIOCondition condition,
		       gpointer     user_data);

void     apl_send_inp (gchar  *text,
		       ssize_t sz);

void     apl_eval     (gchar  *expr,
                       gint    len,
                       void  (*callback)(gchar *result,
                                         size_t idx,
                                         void  *state),
                       void   *state);

void     apl_eval_end ();

/* copied from <aplsrc>/src/NamedObject.hh */

#define END_TAG   "APL_NATIVE_END_TAG"
#define END_TAGNL "APL_NATIVE_END_TAG\n"

enum NameClass {
  NC_INVALID          = -1,   ///< invalid name class.
  NC_UNUSED_USER_NAME =  0,   ///< unused user name eg. pushed but not assigned
  NC_LABEL            =  1,   ///< Label.
  NC_VARIABLE         =  2,   ///< (assigned) variable.
  NC_FUNCTION         =  3,   ///< (user defined) function.
  NC_OPERATOR         =  4,   ///< (user defined) operator.
  NC_SHARED_VAR       =  5,   ///< shared variable.
};

#endif
