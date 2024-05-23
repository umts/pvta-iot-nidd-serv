/** @file http_get_stop.h
 *  @brief Macros and function defines for the HTTP client.
 */
#ifndef HTTP_GET_STOP_H
#define HTTP_GET_STOP_H
#include <nxt_unit.h>
#include <stop.h>

/** @fn char* http_get_request(void)
 *  @brief Makes an HTTP GET request and returns a char pointer to the HTTP
 * response body buffer.
 */
int http_request_stop_json(
    nxt_unit_request_info_t *req, char *json_buffer, void *path
);
#endif
