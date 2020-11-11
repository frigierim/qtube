#include "signal.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "watchdog/watchdog.h"
#include "queuetube_daemon.h"

static bool terminate = false;

static void term_signal_handler(int ignored)
{
  terminate = true;
}

// This function will be respawned by the watchdog, in case of unexpected kill
// Alternatively, returning SB::Watchdog::KO will force the watchdog to respawn.
// By returning any other value, this process and the watchdog will be terminated.
int loop_func(void *args, std::ostream *p_out, std::ostream *p_err)
{
  unsigned int i = 0;

  // Add code to gracefully handle shutdown by handling SIGTERM.
  // Watchdog will politely ask to terminate, before sending a SIGKILL. 
  if(signal(SIGTERM, term_signal_handler) == SIG_ERR)
      return -1;
  
  QTD_ARGS *arguments = (QTD_ARGS *)args;

  arguments->qtd_arg_out_stream = p_out;
  arguments->qtd_arg_err_stream = p_err;

  // Set up daemon
  void * qtd_handle = qtd_initialize(arguments);  

  // Cannot start daemon, retry
  if (qtd_handle == NULL) 
    return SB::Watchdog::KO;

  while(!terminate)
	  sleep(5000);

  // Tear down daemon
  qtd_deinitialize(qtd_handle);

  // clean shutdown
  return  0;
}


bool read_params(int argc, char ** argv, QTD_ARGS *p_args)
{
  struct sockaddr_in sa;

  int opt;
  while ((opt = getopt(argc, argv, "dhl:s:p:w:")) != -1) 
  {
      switch (opt) 
      {
          case 'd':
              p_args->qtd_arg_debug = true;
              break;

          case 'l':
              p_args->qtd_arg_listening_port = atoi(optarg);
              break;
          
          case 's':
              if (inet_pton(AF_INET, optarg, &(sa.sin_addr)) == 1 ||
                  inet_pton(AF_INET6, optarg, &(sa.sin_addr)) == 1)
                p_args->qtd_arg_mpc_server = optarg;
	      else
                p_args->qtd_arg_mpc_server = NULL;
              break;
          
          case 'p':
              p_args->qtd_arg_mpc_port = atoi(optarg);
              break;
        
	  case 'w':
              p_args->qtd_arg_password = optarg;
              break;

	  case 'h': 
          default: /* '?' */
              std::cerr << "Usage: " << argv[0] << " [-l service_port] [-s mpd_address] [-p mpd_port] [-w reset_password] [-d]" << std::endl;
              return false;
      }
  }

  if (p_args->qtd_arg_listening_port == 0 || p_args->qtd_arg_mpc_port == 0 || p_args->qtd_arg_mpc_server == NULL)
  {
      std::cerr << "Invalid parameter(s) provided!" << std::endl;
      return false; 
  }
  
  return true;
}

int main(int argc, char **argv)
{
  QTD_ARGS arguments = {};
  SB::WatchdogHelper wd;

  // Initialize default values 
  arguments.qtd_arg_listening_port = 9090;
  arguments.qtd_arg_mpc_server = "127.0.0.1";
  arguments.qtd_arg_mpc_port = 6600;
  arguments.qtd_arg_password = NULL;
  arguments.qtd_arg_debug = false;

  if (read_params(argc, argv, &arguments) == false)
    return -1;

  if (arguments.qtd_arg_debug)
    return loop_func(&arguments, &std::cout, &std::cerr);

  std::filebuf fb;
  if(fb.open("queueservice.log",std::ios::out) != NULL)
  {
  	std::ostream os(&fb);
  	return wd.start(loop_func, &arguments, &os, &os);
  }
  return wd.start(loop_func, &arguments, &std::cout, &std::cerr);
}
