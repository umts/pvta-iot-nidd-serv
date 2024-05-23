/** @headerfile vzw_connect.h */
#include "firmware_requests.h"

#include <config.h>
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

#include "curl_callbacks.h"

#define REC_HEADER_SIZE 5000

#define REPO "umts/embedded-departure-board"

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
    CURL *curl, char *header_buf, char *latest_tag
) {
  CURLcode res;

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(
      curl, CURLOPT_URL, "https://github.com/" REPO "/releases/latest"
  );

  PRINTDBG("Fetching latest firmware tag...");
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    PRINTERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    goto cleanup;
  }

  parse_tag(header_buf, latest_tag);
  PRINTDBG("Latest firmware tag: %s", latest_tag);

cleanup:
  return res;
}

int download_firmware_github(FILE **fptr) {
  CURL *curl = curl_easy_init();
  CURLcode res;
  char *p;
  char latest_tag[12];
  char header_buf[REC_HEADER_SIZE];
  char user_agent[19] = "licurl/";

  RecvData header_data = {header_buf, 0};
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, mem_write_callback);
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

  res = latest_firmware_tag(curl, header_buf, latest_tag);
  char filename[14 + strlen(latest_tag) + 16];
  if (res != CURLE_OK) {
    goto cleanup;
  }

  p = stpcpy(filename, "/srv/firmware/");
  p = stpcpy(p, latest_tag);
  (void)stpcpy(p, "_app_update.bin");

  *fptr = fopen(filename, "wb+x");
  PRINTDBG("fptr %p", *fptr);
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

  char url[80] = "https://github.com/" REPO "/releases/download/";

  p = stpcpy((url + strlen(url)), &latest_tag[0]);
  (void)stpcpy(p, "/app_update.bin");

  header_data.size = 0;
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, mem_write_callback);
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
  curl_easy_cleanup(curl);
  return (int)res;
}
