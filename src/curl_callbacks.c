/** @headerfile curl_callbacks.h */
#include "curl_callbacks.h"

#include <stdlib.h>
#include <string.h>

size_t stack_mem_write_callback(
    const char *buffer, const size_t size, const size_t nitems,
    const void *userdata
) {
  size_t realsize = size * nitems;
  RecvData *mem = (RecvData *)userdata;

  memcpy(&(mem->response[mem->size]), buffer, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
  return realsize;
}

size_t heap_mem_write_callback(
    const char *buffer, const size_t size, const size_t nitems,
    const void *userdata
) {
  size_t realsize = size * nitems;
  RecvData *mem = (RecvData *)userdata;

  char *res = realloc(mem->response, mem->size + realsize + 1);
  if (res == NULL) {
    return 0;
  }

  mem->response = res;

  (void)memcpy(&(mem->response[mem->size]), buffer, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
  return realsize;
}
