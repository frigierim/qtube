#ifndef _QUEUETUBE_DAEMON_H_
#define _QUEUETUBE_DAEMON_H_

#include "ostream"

typedef struct 
{
  const char *    qtd_arg_mpc_server;
  std::ostream *  qtd_arg_out_stream;
  std::ostream *  qtd_arg_err_stream;
  unsigned short  qtd_arg_listening_port;
  unsigned short  qtd_arg_mpc_port;
  bool            qtd_arg_debug;
} QTD_ARGS;

void * qtd_initialize(QTD_ARGS *args);
void qtd_deinitialize(void *h);

#endif // _QUEUETUBE_DAEMON_H_