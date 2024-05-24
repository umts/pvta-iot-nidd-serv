/** @headerfile http_get_stop.h */
#include "http_get_stop.h"

#include "config.h"
#include "curl_callbacks.h"

/* Newlib C includes */
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

/** A macro that defines the HTTP request host name for the request headers. */
#define HTTP_REQUEST_HOSTNAME "bustracker.pvta.com"

/** A macro that defines the HTTP file path for the request headers. */
#define HTTP_REQUEST_PATH \
  "/InfoPoint/rest/SignageStopDepartures/GetSignageDeparturesByStopId?stopId="

#define STOP_URL "https://" HTTP_REQUEST_HOSTNAME HTTP_REQUEST_PATH

#define STOP_URL_LEN (sizeof(STOP_URL) / sizeof(char)) + 4

/** @def RECV_HEADER_BUF_SIZE
 *  @brief A macro that defines the max size for HTTP response headers receive
 * buffer.
 *
 *  Seems to be ~320 bytes, size ~doubled for safety.
 */
// #define RECV_HEADER_BUF_SIZE 1000

char *http_request_stop_json(void *path) {
  PRINTDBG("Stop ID: %s", (char *)(path + 6));

  CURL *curl = curl_easy_init();
  CURLcode res;

  char url[STOP_URL_LEN] = STOP_URL;

  strcat(url, (char *)(path + 6));

  /** HTTP response headers buffer with size defined by the RECV_HEADER_BUF_SIZE
   * macro. */
  char *header_buffer = NULL;
  char *body_buffer = NULL;

  RecvData recv_body = {body_buffer, 0};
  RecvData recv_headers = {header_buffer, 0};

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Accept: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, heap_mem_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&recv_body);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, heap_mem_write_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&recv_headers);

  /*curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");*/
  /*curl_easy_setopt(curl, CURLOPT_SSLCERT, "/srv/cert/");*/
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  /* Perform the request, res will get the return code */
  res = curl_easy_perform(curl);
  /* Check for errors */
  if (res != CURLE_OK) {
    PRINTERR("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return NULL;
  }

  (void)free(header_buffer);
  // (void)free(body_buffer);

  /* always cleanup */
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return recv_body.response;
}
