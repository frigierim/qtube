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

HLP_RES helper_process_url(QTD_ARGS *arguments, std::vector<std::string> playlist_urls, std::string *res_response)
{
    char buf[BUFSIZE];
    struct mpd_connection *conn;
    unsigned int ret_length = 0;
    HLP_RES ret_val = HLP_SUCCESS;
    std::string complete_cmd = "youtube-dl --get-url -i -f bestaudio --socket-timeout 20 ";    
    std::string response;

    memset(buf, BUFSIZE, 0);
    response = "URL successfully submitted\n";

    if (playlist_urls.size() == 0)
    {
      response = "No URL requested!\n";
	    if (res_response)
        *res_response = response; 
      
      return HLP_FAILURE;
    }

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
              response = "Error extracting URL\n";
              ret_val = HLP_FAILURE;
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
              const char *mpd_server_err = mpd_connection_get_error_message(conn); 
              *arguments->qtd_arg_err_stream << "helper_process_url(): server refused to add stream - " << mpd_server_err << std::endl;
              response = "MPD server error: ";
              response += mpd_server_err;
              response += "\n";

	            ret_val = HLP_FAILURE;
            }
            else
            {
              *arguments->qtd_arg_err_stream << "helper_process_url(): stream successfully added to server" << std::endl;
            }
          }

          if (pclose(fp) != 0)
          {
            *arguments->qtd_arg_err_stream << "helper_process_url(): youtube-dl could not process URL" << std::endl;
            response = "Error processing URL\n";
            ret_val = HLP_FAILURE;
          }
      }
      else
      {
        *arguments->qtd_arg_err_stream << "helper_process_url(): MPD connection error"  << std::endl;
        response = "Error connecting to MPD server\n";
        ret_val = HLP_FAILURE;
      }
    }
    else
    {
      *arguments->qtd_arg_err_stream << "helper_process_url(): cannot connect to MPD server"  << std::endl;
      response = "Error connecting to MPD server\n";
      ret_val = HLP_FAILURE;
    }

end:

    if (conn)
      mpd_connection_free(conn);
    
    if (res_response)
      *res_response = response; 
    
    return ret_val;
}


static void helper_process_playlist_requests(QTD_ARGS *arguments, std::vector<std::string> playlist_urls)
{
	*arguments->qtd_arg_out_stream << "helper_process_playlist_request(): processing " << playlist_urls.size() << " elements in playlist" << std::endl;
	std::thread processor(helper_process_url, arguments, playlist_urls, nullptr);
	processor.detach();
}

HLP_RES helper_reset(QTD_ARGS *arguments, const std::string &password, std::string &response)
{
    const std::string cmd_stop = "sudo service mopidy stop";     
    const std::string cmd_start = "sudo service mopidy start";     
    if (arguments->qtd_arg_password != NULL && password == std::string(arguments->qtd_arg_password))
    {
	    FILE *fp;
	    if ((fp = popen(cmd_stop.c_str(), "r")) == NULL) 
	    {
	      *arguments->qtd_arg_err_stream << "helper_reset(): error opening pipe" << std::endl;
	      response = "Error stopping server\n";
	      return HLP_FAILURE; 
	    }
	    if (pclose(fp) != 0)
	    {
	      *arguments->qtd_arg_err_stream << "helper_reset(): error reported stopping the service" << std::endl;
	      response = "Error stopping server\n";
	      return HLP_FAILURE; 
	    }

	    
	    if ((fp = popen(cmd_start.c_str(), "r")) == NULL) 
	    {
	      *arguments->qtd_arg_err_stream << "helper_reset(): error opening pipe" << std::endl;
	      response = "Error starting server\n";
	      return HLP_FAILURE; 
	    }
	    if (pclose(fp) != 0)
	    {
	      *arguments->qtd_arg_err_stream << "helper_reset(): error reported starting the service" << std::endl;
	      response = "Error starting server\n";
	      return HLP_FAILURE; 
	    }
	    response = "Server restarted successfully";
	    return HLP_SUCCESS;
    }
    else
    {
      sleep(5);
      response = "helper_reset(): invalid password";
      return HLP_FAILURE;
    }
}

HLP_RES helper_process_playlist(QTD_ARGS *arguments, const std::string &url, std::string *res_response)
{
    char buf[BUFSIZE] = {0};
    std::vector<std::string> playlist_urls;
    HLP_RES ret_val = HLP_SUCCESS;
    std::string cmd = "youtube-dl --flat-playlist -j --socket-timeout=20 ";     
    std::string single_url = "http://youtu.be/";
    std::string response;
    const unsigned int ID_LENGTH = 11;

    *(arguments->qtd_arg_out_stream) << "helper_process_playlist(): requested URL " << url << std::endl;
    std::string complete_cmd = cmd + url;

    FILE *fp;
    if ((fp = popen(complete_cmd.c_str(), "r")) == NULL) 
    {
	    *arguments->qtd_arg_err_stream << "helper_process_playlist(): error opening pipe" << std::endl;
      response = "Error extracting URL\n";
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
      response = "Error processing URL\n";
      if (res_response)
        *res_response = response;
      return HLP_FAILURE;
    }
    else
    {
      response = "Extracted ";
      response += playlist_urls.size();
      response += " URLs, processing them in background...\n";
    }
    
    helper_process_playlist_requests(arguments, playlist_urls);
   
    if (res_response)
      *res_response = response;
    
    return ret_val;

}
