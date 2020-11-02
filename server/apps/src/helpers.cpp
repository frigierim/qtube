#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mpd/connection.h>
#include <mpd/queue.h>
#include "helpers.h"

HLP_RES helper_process_url(QTD_ARGS *arguments, const char *url, unsigned int url_len, char *buf, unsigned int buf_size) 
{
    struct mpd_connection *conn;
    unsigned int ret_length = 0, index = 0;
    HLP_RES ret_val = HLP_SUCCESS;
    const char *cmd = "youtube-dl --get-url --socket-timeout 10 ";    
    unsigned int cmd_len = strlen(cmd) + url_len + 1;

    memset(buf, buf_size, 0);
    char * complete_cmd = (char *)malloc(cmd_len);
    if (complete_cmd == NULL)
	    return HLP_NOMEM;

    memcpy(complete_cmd, cmd, cmd_len);
    strcat(complete_cmd, url);

    // connect to MPD server and append the buffer to the playlist 
    conn = mpd_connection_new(arguments->qtd_arg_mpc_server, arguments->qtd_arg_mpc_port, 0);
    if (conn != NULL)
    {
      if (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS)
      {
          FILE *fp;
          if ((fp = popen(complete_cmd, "r")) == NULL) 
          {
              *arguments->qtd_arg_err_stream << "helper_process_url(): error opening pipe" << std::endl;
              ret_val = HLP_NOPIPE;
              goto end;
          }

          while ((fgets(buf, buf_size, fp)) != NULL) 
          {
            ++index;
          
            if (index % 2 == 1)
              continue;

            ret_length = strlen(buf);

            // remove trailing newline
            buf[ret_length-1] = 0;

            mpd_run_add(conn, buf);
          }

          pclose(fp);
      }
      else
      {
        *arguments->qtd_arg_err_stream << "helper_process_url(): MPD connection error"  << std::endl;
        ret_val = HLP_NOSERVER;
      }
    }
    else
    {
      *arguments->qtd_arg_err_stream << "helper_process_url(): cannot connect to MPD server"  << std::endl;
      ret_val = HLP_NOSERVER;
    }

end:

    if (conn)
      mpd_connection_free(conn);
    
    free(complete_cmd);
    return ret_val;
}
