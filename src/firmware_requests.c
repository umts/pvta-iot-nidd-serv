/** @headerfile vzw_connect.h */
#include "firmware_requests.h"

#include <config.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "curl_callbacks.h"

#define REPO "umts/embedded-departure-board"
#define RELEASES_URL "https://github.com/" REPO "/releases/"

#define APP_UPDATE_DIR "/srv/firmware/"
#define APP_UPDATE_NAME_PARTIAL "_app_update.bin"

static void parse_tag(char *headers, char *tag) {
  char *p;
  p = strstr(headers, "location:");
  p += 9;
  p = strstr(p, "tag/");
  p += 4;

  int tag_size = (strstr(p, "\r\n") - p);
  memcpy(tag, p, tag_size);
  *(tag + tag_size) = '\0';
}

static CURLcode latest_firmware_tag(
    CURL *curl, RecvData *header_data, char *latest_tag
) {
  CURLcode res;

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl, CURLOPT_URL, RELEASES_URL "latest");

  PRINTDBG("Fetching latest firmware tag...");
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    PRINTERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    goto cleanup;
  }

  parse_tag(header_data->response, latest_tag);
  PRINTALRT("Latest firmware tag: %s", latest_tag);

cleanup:
  return res;
}

int download_firmware_github(FILE **fptr) {
  CURL *curl = curl_easy_init();
  CURLcode res;
  char *p;
  char latest_tag[12];
  char user_agent[19] = "licurl/";
  RecvData header_data = {NULL, 0};

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, heap_mem_write_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_data);
  curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(
      curl, CURLOPT_USERAGENT,
      strcat(user_agent, curl_version_info(CURLVERSION_NOW)->version)
  );
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  res = latest_firmware_tag(curl, &header_data, latest_tag);
  if (res != CURLE_OK) {
    goto cleanup;
  }

  char filename
      [sizeof(APP_UPDATE_DIR) + sizeof(latest_tag) +
       sizeof(APP_UPDATE_NAME_PARTIAL) - 2];

  p = stpcpy(filename, APP_UPDATE_DIR);
  p = stpcpy(p, latest_tag);
  (void)stpcpy(p, APP_UPDATE_NAME_PARTIAL);

  *fptr = fopen(filename, "wb+x");
  if (*fptr == NULL) {
    *fptr = fopen(filename, "rb");
    if (*fptr == NULL) {
      PRINTERR("Failed to create firmware file");
      goto cleanup;
    } else {
      PRINTDBG("Firmware file already exists");
      goto cleanup;
    }
  }

  char
      url[sizeof(RELEASES_URL) + sizeof("download/") + sizeof(latest_tag) +
          sizeof("/app_update.bin") - 3] = RELEASES_URL "download/";

  p = stpcpy((url + strlen(url)), &latest_tag[0]);
  (void)stpcpy(p, "/app_update.bin");

  header_data.size = 0;
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, heap_mem_write_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, *fptr);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
  curl_easy_setopt(curl, CURLOPT_URL, url);

  PRINTDBG("Downloading firmware file from %s to %s...", url, filename);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    PRINTERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    fclose(*fptr);
  }

  (void)rewind(*fptr);
  if (fptr == NULL) {
    PRINTERR("fptr null");
  }

  PRINTDBG("Done");

cleanup:
  free(header_data.response);
  curl_easy_cleanup(curl);
  return (int)res;
}
