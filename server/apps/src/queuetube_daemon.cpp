#include <string>
#include <vector>
#include <string.h>
#include <microhttpd.h>
#include "queuetube_daemon.h"
#include "helpers.h"

static int queuetube_add_handler(QTD_ARGS *arguments, struct MHD_Connection * connection);
static int queuetube_service(void *cls, struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **ptr);

typedef int(* DISPATCH_FUNCTION)(QTD_ARGS *, struct MHD_Connection *);
#define MAKE_URL(x) (x), sizeof((x))-1

enum DISPATCH_METHOD
{
  METHOD_GET,
  METHOD_MAX
};

static const char *DISPATCH_METHOD_STRINGS[METHOD_MAX]=
{
  "GET"
};

static struct 
{
  DISPATCH_METHOD   method;
  const char *      url;
  size_t            url_len; 
  DISPATCH_FUNCTION dispatch_fun;
} dispatch_table[] =
{
  METHOD_GET, MAKE_URL("/add"), queuetube_add_handler
};


void * qtd_initialize(QTD_ARGS *args)
{
  struct MHD_Daemon * handle = NULL;
  handle = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
                            args->qtd_arg_listening_port,
                            NULL,
                            NULL,
                            &queuetube_service,
                            args,
                            MHD_OPTION_END);

  return handle;
}


void qtd_deinitialize(void *h)
{
  struct MHD_Daemon *handle = (struct MHD_Daemon *)h;
  MHD_stop_daemon(handle);
}

// PRIVATE API /////////////////////////////////////////////////////////////////

static int queuetube_positive_response(struct MHD_Connection * connection, const std::string &string)
{
		struct MHD_Response * response= MHD_create_response_from_buffer (string.length(),
                                                                    (void *)string.c_str(),
                                                                    MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection,
                                MHD_HTTP_OK,
                                response);
  		
		MHD_destroy_response(response);

    return ret;
}

static int queuetube_negative_response(struct MHD_Connection * connection, const std::string & string )
{
		struct MHD_Response * response= MHD_create_response_from_buffer (string.length(),
                                                                    (void *)string.c_str(),
                                                                    MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection,
                                MHD_HTTP_SERVICE_UNAVAILABLE,
                                response);
  		
		MHD_destroy_response(response);

    return ret;
}

static int queuetube_add_handler(QTD_ARGS *arguments, struct MHD_Connection * connection)
{
  bool is_playlist = false;
  std::string response;

  const char *value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "url");
  HLP_RES res;

  if (strstr(value, "playlist") != NULL)
  {
	  is_playlist = true;
  	res = helper_process_playlist(arguments, value, &response);
  }
  else
  {
	  std::vector<std::string> url;
	  url.push_back(value);
	  res = helper_process_url(arguments, url, &response);
  }  
  
  switch(res)
  {
    case HLP_SUCCESS:
      {
      	return queuetube_positive_response(connection, response);
      }
    
    case HLP_FAILURE:
      return queuetube_negative_response(connection, response);

    default:
      break;
  }
  return queuetube_negative_response(connection, "Invalid request!");
}

static int queuetube_dispatch(QTD_ARGS *arguments, const char *method, const char *url, struct MHD_Connection * connection)
{
  unsigned int i = 0;
  *(arguments->qtd_arg_out_stream) << "queuetube_dispatch: handling " << url << std::endl;

  for(;i<sizeof(dispatch_table)/sizeof(dispatch_table[0]);++i)
  {
    if (strcmp(method, DISPATCH_METHOD_STRINGS[dispatch_table[i].method]) == 0 && strcmp(url, dispatch_table[i].url) == 0)
    {
      return dispatch_table[i].dispatch_fun(arguments, connection);
    }
  }
  
  return queuetube_negative_response(connection, "Invalid request");
}

static int queuetube_service(void *                 cls,
                            struct MHD_Connection * connection,
                            const char *            url,
                            const char *            method,
                            const char *            version,
                            const char *            upload_data,
                            size_t *                upload_data_size,
                            void **                 ptr) 
{
  static int dummy;
  if (&dummy != *ptr)
  {
    /* The first time only the headers are valid,
      do not respond in the first round... */
    *ptr = &dummy;
    return MHD_YES;
  }
  *ptr = NULL; /* clear context pointer */
  
  return queuetube_dispatch((QTD_ARGS *)cls, method, url, connection);
}
