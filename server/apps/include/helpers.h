#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <vector>
#include <string>
#include "queuetube_daemon.h"

typedef enum
{
  HLP_SUCCESS,
  HLP_FAILURE,
} HLP_RES;

HLP_RES helper_process_url(QTD_ARGS *arguments, std::vector<std::string> playlist_urls, std::string *response);
HLP_RES helper_process_playlist(QTD_ARGS *arguments, const std::string &url, std::string *response);
HLP_RES helper_reset(QTD_ARGS *arguments, const std::string &password, std::string &res_response);

#endif // #ifndef _HELPERS_H_
