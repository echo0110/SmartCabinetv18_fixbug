#ifndef _HTTP_REQUEST_H
#define _HTTP_REQUEST_H
#include "stm32f10x.h"
typedef struct http_request {
	char *uri;
	char *post_data;
	u8 is_post;
	char *params;
} HTTPRequest;
extern HTTPRequest post_info;//test  for  post data receive
#endif
