/** @file nidd_client.h
 *  @brief Function definitions for user defined curl callback functions.
 */
#ifndef CURL_CALLBACKS_H
#define CURL_CALLBACKS_H
#include <curl/curl.h>

typedef struct RecvData {
  char *response;
  size_t size;
} RecvData;

/** @fn const size_t stack_mem_write_callback(const char *buffer, const size_t
 * size, size_t nitems, const void *userdata);
 *  @brief User defined curl callback for writing response data to stack mem.
 */
size_t stack_mem_write_callback(
    const char *buffer, const size_t size, const size_t nitems,
    const void *userdata
);

/** @fn const size_t heap_mem_write_callback(const char *buffer, const size_t
 * size, size_t nitems, const void *userdata);
 *  @brief User defined curl callback for writing response data to heap mem.
 */
size_t heap_mem_write_callback(
    const char *buffer, const size_t size, const size_t nitems,
    const void *userdata
);
#endif
