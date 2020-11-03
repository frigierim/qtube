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
    unsigned int ret_length = 0;
    HLP_RES ret_val = HLP_SUCCESS;
    const char *cmd = "youtube-dl --get-url -f bestaudio --socket-timeout 20 ";    
    unsigned int cmd_len = strlen(cmd) + url_len + 1;

    *(arguments->qtd_arg_out_stream) << "helper_process_url(): requested URL " << url << std::endl;
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
          
            ret_length = strlen(buf);

            // remove trailing newline
            buf[ret_length-1] = 0;
	    *(arguments->qtd_arg_out_stream) << "helper_process_url(): retrieved stream " << buf << std::endl;

            if(mpd_run_add(conn, buf) == false)
            {
              *arguments->qtd_arg_err_stream << "helper_process_url(): server refused to add stream" << std::endl;
	      ret_val = HLP_NOSERVER;
	    }
	    else
	    {
              *arguments->qtd_arg_err_stream << "helper_process_url(): stream successfully added to server" << std::endl;
	    }
          }

          if (pclose(fp) != 0)
            ret_val = HLP_NOPIPE;
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


HLP_RES helper_process_playlist(QTD_ARGS *arguments, const char *url, unsigned int url_len, char *buf, unsigned int buf_size) 
{
    struct mpd_connection *conn;
    unsigned int ret_length = 0, index = 0;
    HLP_RES ret_val = HLP_SUCCESS;
    const char *cmd = "youtube-dl --flat-playlist -j --socket-timeout=20 ";     
    char single_url[] = "http://youtu.be/XXXXXXXXXXX";
    const unsigned int ID_OFFSET = sizeof("http://youtu.be/") - 1;
    const unsigned int ID_LENGTH = 11;
    const unsigned int SINGLE_URL_LEN = 24;

    unsigned int cmd_len = strlen(cmd) + url_len + 1;

    *(arguments->qtd_arg_out_stream) << "helper_process_playlist(): requested URL " << url << std::endl;
    memset(buf, buf_size, 0);
    char * complete_cmd = (char *)malloc(cmd_len);
    if (complete_cmd == NULL)
	    return HLP_NOMEM;

    memcpy(complete_cmd, cmd, cmd_len);
    strcat(complete_cmd, url);
    
    FILE *fp;
    if ((fp = popen(complete_cmd, "r")) == NULL) 
    {
	*arguments->qtd_arg_err_stream << "helper_process_playlist(): error opening pipe" << std::endl;
      	ret_val = HLP_NOPIPE;
      	goto end;
    }

    // Read one line at a time
    while ((fgets(buf, buf_size, fp)) != NULL) 
    {
        buf[strlen(buf)-1] = 0;
	const char *p_url = strstr(buf, "\"url\": \"");
	if (p_url)
	{
		p_url += sizeof("\"url\": \"")-1;
		const char *p_url_end = p_url + 1;
		while(p_url_end < buf + buf_size && *p_url_end != '\"') ++p_url_end;

		if (*p_url_end == '\"' && p_url_end - p_url == ID_LENGTH)
		{
			memcpy(&single_url[ID_OFFSET] ,p_url, ID_LENGTH);
			*arguments->qtd_arg_err_stream << "helper_process_playlist(): processing " << single_url << std::endl;
			ret_val =  helper_process_url(arguments, single_url, SINGLE_URL_LEN, buf, buf_size);
			if (ret_val != HLP_SUCCESS)
		    		break;
		}
	}
    }

    fclose(fp);

end:

    free(complete_cmd);
    return ret_val;

}
