#ifndef _HELPERS_H_
#define _HELPERS_H_

#include "queuetube_daemon.h"

typedef enum
{
  HLP_SUCCESS,
  HLP_NOMEM,
  HLP_NOSERVER,
  HLP_NOPIPE
} HLP_RES;

HLP_RES helper_process_url(QTD_ARGS *arguments, const char *url, unsigned int url_len, char *buf, unsigned int buf_size);

#endif // #ifndef _HELPERS_H_