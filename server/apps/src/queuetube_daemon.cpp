#include <microhttpd.h>
#include <string.h>
#include "queuetube_daemon.h"
#include "helpers.h"

#define BUFSIZE 2048

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

static int queuetube_positive_response(struct MHD_Connection * connection)
{
    const char * page = "<html><head><title>QueueTube</title></head><body>OK</body></html>";
		struct MHD_Response * response= MHD_create_response_from_buffer (strlen(page),
                                                                    (void*) page,
                                                                    MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection,
                                MHD_HTTP_OK,
                                response);
  		
		MHD_destroy_response(response);

    return ret;
}

static int queuetube_negative_response(struct MHD_Connection * connection, int response_code)
{
    const char * page = "<html><head><title>QueueTube</title></head><body>ERR</body></html>";
		struct MHD_Response * response= MHD_create_response_from_buffer (strlen(page),
                                                                    (void*) page,
                                                                    MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection,
                                response_code,
                                response);
  		
		MHD_destroy_response(response);

    return ret;
}

static int queuetube_add_handler(QTD_ARGS *arguments, struct MHD_Connection * connection)
{
  char buf[BUFSIZE];
  const char *value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "url");
	
  HLP_RES res = helper_process_url(arguments, value, strlen(value), buf, BUFSIZE);
  switch(res)
  {
    case HLP_SUCCESS:
      return queuetube_positive_response(connection);

    case HLP_NOMEM:
    case HLP_NOPIPE:
      return queuetube_negative_response(connection, MHD_HTTP_PRECONDITION_FAILED );

    case HLP_NOSERVER:
      return queuetube_negative_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE);

    default:
      break;
  }
  return queuetube_negative_response(connection, MHD_HTTP_NOT_FOUND);
}

static int queuetube_dispatch(QTD_ARGS *arguments, const char *method, const char *url, struct MHD_Connection * connection)
{
  unsigned int i = 0;
  for(;i<sizeof(dispatch_table)/sizeof(dispatch_table[0]);++i)
  {
    if (strcmp(method, DISPATCH_METHOD_STRINGS[dispatch_table[i].method]) == 0 && strcmp(url, dispatch_table[i].url) == 0)
    {
      return dispatch_table[i].dispatch_fun(arguments, connection);
    }
  }
  
  return queuetube_negative_response(connection, MHD_HTTP_NOT_ACCEPTABLE);
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
