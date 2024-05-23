/** @headerfile curl_callbacks.h */
#include "curl_callbacks.h"

#include <string.h>

size_t mem_write_callback(
    const char *buffer, const size_t size, const size_t nitems,
    const void *userdata
) {
  size_t realsize = size * nitems;
  RecvData *mem = (RecvData *)userdata;

  (void)memcpy(&(mem->response[mem->size]), buffer, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
  return realsize;
}
