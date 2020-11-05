#include <thread>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mpd/connection.h>
#include <mpd/queue.h>
#include "helpers.h"

#define BUFSIZE 2048

HLP_RES helper_process_url(QTD_ARGS *arguments, std::vector<std::string> playlist_urls)
{
    char buf[BUFSIZE];
    struct mpd_connection *conn;
    unsigned int ret_length = 0;
    HLP_RES ret_val = HLP_SUCCESS;
    const char *cmd = "youtube-dl --get-url -i -f bestaudio --socket-timeout 20 ";    

    memset(buf, BUFSIZE, 0);
    std::string complete_cmd = cmd;
    
    if (playlist_urls.size() == 0)
	    return HLP_NOMEM;

    std::vector<std::string>::iterator url_iterator = playlist_urls.begin();
    for(;url_iterator != playlist_urls.end(); ++url_iterator)
    {
        complete_cmd += *url_iterator + " ";
    }

    // connect to MPD server and append the buffer to the playlist 
    conn = mpd_connection_new(arguments->qtd_arg_mpc_server, arguments->qtd_arg_mpc_port, 0);
    if (conn != NULL)
    {
      if (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS)
      {
          FILE *fp;
          if ((fp = popen(complete_cmd.c_str(), "r")) == NULL) 
          {
              *arguments->qtd_arg_err_stream << "helper_process_url(): error opening pipe" << std::endl;
              ret_val = HLP_NOPIPE;
              goto end;
          }

          while ((fgets(buf, BUFSIZE, fp)) != NULL) 
          {
          
            ret_length = strlen(buf);

            // remove trailing newline
            buf[ret_length-1] = 0;
	    *(arguments->qtd_arg_out_stream) << "helper_process_url(): retrieved stream " << buf << std::endl;

            if(mpd_run_add(conn, buf) == false)
            {
              *arguments->qtd_arg_err_stream << "helper_process_url(): server refused to add stream" << mpd_connection_get_error_message(conn) << std::endl;
	      ret_val = HLP_NOSERVER;
	    }
	    else
	    {
              *arguments->qtd_arg_err_stream << "helper_process_url(): stream successfully added to server" << std::endl;
	    }
          }

          if (pclose(fp) != 0)
	  {
	  	*arguments->qtd_arg_err_stream << "helper_process_url(): youtube-dl could not process URL" << std::endl;
            	ret_val = HLP_NOPIPE;
	  }
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
    
    return ret_val;
}


static void helper_process_playlist_requests(QTD_ARGS *arguments, std::vector<std::string> playlist_urls)
{
	*arguments->qtd_arg_out_stream << "helper_process_playlist_request(): processing elements in playlist - " << playlist_urls.size() << std::endl;
	std::thread processor(helper_process_url, arguments, playlist_urls);
	processor.detach();
}


HLP_RES helper_process_playlist(QTD_ARGS *arguments, const char *url, unsigned int url_len)
{
    char buf[BUFSIZE] = {0};
    std::vector<std::string> playlist_urls;
    HLP_RES ret_val = HLP_SUCCESS;
    std::string cmd = "youtube-dl --flat-playlist -j --socket-timeout=20 ";     
    std::string single_url = "http://youtu.be/";
    const unsigned int ID_LENGTH = 11;

    *(arguments->qtd_arg_out_stream) << "helper_process_playlist(): requested URL " << url << std::endl;
    std::string complete_cmd = cmd + url;

    FILE *fp;
    if ((fp = popen(complete_cmd.c_str(), "r")) == NULL) 
    {
	*arguments->qtd_arg_err_stream << "helper_process_playlist(): error opening pipe" << std::endl;
      	return HLP_NOPIPE;
    }

    // Read one line at a time
    while ((fgets(buf, BUFSIZE, fp)) != NULL) 
    {
        buf[strlen(buf)-1] = 0;
	const char *p_url = strstr(buf, "\"url\": \"");
	if (p_url)
	{
		p_url += sizeof("\"url\": \"")-1;
		const char *p_url_end = p_url + 1;
		while(p_url_end < buf + BUFSIZE && *p_url_end != '\"') ++p_url_end;

		if (*p_url_end == '\"' && p_url_end - p_url == ID_LENGTH)
		{
			char current_url[ID_LENGTH+1];
			current_url[ID_LENGTH] = 0;	
			memcpy(current_url ,p_url, ID_LENGTH);
			playlist_urls.push_back(single_url + current_url);	
		}
	}
    }

    if (fclose(fp) != 0)
    {
	*arguments->qtd_arg_err_stream << "helper_process_playlist(): youtube-dl could not process URL" << std::endl;
	return HLP_NOPIPE;
    }

    helper_process_playlist_requests(arguments, playlist_urls);
    return ret_val;

}
