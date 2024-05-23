/** @headerfile vzw_connect.h */
#include "vzw_connect.h"

#include <stdio.h>
#include <stdlib.h>

#define JSMN_HEADER

#include <base64.h>
#include <config.h>
#include <curl/curl.h>
#include <curl/header.h>
#include <jsmn.h>
#include <stddef.h>
#include <stop.h>
#include <string.h>

#include "../config/vzw_secrets.h"
#include "curl_callbacks.h"
#include "json_helpers.h"

#define REC_BUF_SIZE 1024
#define REC_HEADER_SIZE 1024

// char *stop_to_nidd_str(Stop *stop) {}

static int extract_token(const char *src, char *dst, const char *token) {
  jsmn_parser p;
  jsmntok_t tokens[10];
  int t = 0;

  (void)jsmn_init(&p);

  const int ret = jsmn_parse(
      &p, src, strlen(src), tokens, sizeof(tokens) / sizeof(jsmntok_t)
  );

  if (eval_jsmn_return(ret)) {
    return 1;
  }

  while (t < ret) {
    size_t token_len = (size_t)(tokens[t + 1].end - tokens[t + 1].start);
    if (jsoneq(src, &tokens[t], token)) {
      (void)memcpy((void *)dst, (void *)(src + tokens[t + 1].start), token_len);
    } else if (jsoneq(src, &tokens[t], "error") ||
        jsoneq(src, &tokens[t], "errorCode")) {
      PRINTERR(
          "Error \"%.*s\": %.*s", tokens[t + 1].end - tokens[t + 1].start,
          src + tokens[t + 1].start, tokens[t + 3].end - tokens[t + 3].start,
          src + tokens[t + 3].start
      );
      return 1;
    }
    t++;
  }
  return 0;
}

#define AUTH_TOKEN_FIELD "Authorization: Basic "

/** Base64 encoding is deterministic, so we know the length ahead of time + 1
 * for null terminator */
#define ENC_LEN BASE64LEN(strlen(vzw_auth_keys)) + 1

/** AUTH_TOKEN_FIELD length is 21 */
#define AUTH_TOKEN_FIELD_SIZE strlen(AUTH_TOKEN_FIELD) + ENC_LEN

int get_vzw_auth_token(const char *vzw_auth_keys, char *vzw_auth_token) {
  CURL *curl = curl_easy_init();
  CURLcode res;
  struct curl_slist *headers = NULL;

  char header_buf[REC_HEADER_SIZE];
  char recv_buf[REC_BUF_SIZE];

  RecvData header_data = {header_buf, 0};
  RecvData response_data = {recv_buf, 0};

  char encoded_token[ENC_LEN];
  char auth_token_field[AUTH_TOKEN_FIELD_SIZE];

  curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  curl_easy_setopt(
      curl, CURLOPT_URL, "https://thingspace.verizon.com/api/ts/v1/oauth2/token"
  );

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, mem_write_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_data);

  int base64StringLen =
      base64(vzw_auth_keys, strlen(vzw_auth_keys), encoded_token);

  (void)memcpy(auth_token_field, AUTH_TOKEN_FIELD, strlen(AUTH_TOKEN_FIELD));
  (void)memcpy(
      &auth_token_field[strlen(AUTH_TOKEN_FIELD)], encoded_token,
      base64StringLen
  );
  auth_token_field[AUTH_TOKEN_FIELD_SIZE - 1] = 0;

  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(
      headers, "Content-Type: application/x-www-form-urlencoded"
  );
  headers = curl_slist_append(headers, auth_token_field);

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "grant_type=client_credentials");

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    PRINTERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
    goto fail;
  }

  res = extract_token(recv_buf, vzw_auth_token, "access_token");
  if (res != CURLE_OK) {
    PRINTERR("Failed to get VZW Auth Token");
  }

fail:
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return (int)res;
}

#define USERNAME_FIELD "\"username\":\""
#define PASSWORD_FIELD "\",\"password\":\""
#define M2M_TOKEN_FIELD "Authorization: Bearer "

/** USERNAME_FIELD + PASSWORD_FIELD length + 4 for {"}0 */
#define LOGIN_FIELD_SIZE                                                   \
  strlen(USERNAME_FIELD) + strlen(PASSWORD_FIELD) + strlen(VZW_USERNAME) + \
      strlen(VZW_PASSWORD) + 4

int get_vzw_m2m_token(
    const char *username, const char *password, const char *vzw_auth_token,
    char *vzw_m2m_token
) {
  CURL *curl = curl_easy_init();
  CURLcode res;
  struct curl_slist *headers = NULL;

  char header_buf[REC_HEADER_SIZE];
  char recv_buf[REC_BUF_SIZE];

  RecvData header_data = {header_buf, 0};
  RecvData response_data = {recv_buf, 0};

  char post_field[LOGIN_FIELD_SIZE];
  size_t header_token_field_size =
      strlen(M2M_TOKEN_FIELD) + strlen(vzw_auth_token) + 1;
  char header_token_field[header_token_field_size];

  curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

  (void)memcpy(header_token_field, M2M_TOKEN_FIELD, strlen(M2M_TOKEN_FIELD));
  (void)memcpy(
      &header_token_field[strlen(M2M_TOKEN_FIELD)], vzw_auth_token,
      strlen(vzw_auth_token)
  );
  header_token_field[header_token_field_size - 1] = 0;

  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, header_token_field);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(
      curl, CURLOPT_URL,
      "https://thingspace.verizon.com/api/m2m/v1/session/login"
  );

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, mem_write_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

  post_field[0] = '{';
  (void)memcpy(&post_field[1], USERNAME_FIELD, strlen(USERNAME_FIELD));
  (void
  )memcpy(&post_field[strlen(USERNAME_FIELD) + 1], username, strlen(username));
  (void)memcpy(
      &post_field[strlen(USERNAME_FIELD) + 1 + strlen(username)],
      PASSWORD_FIELD, strlen(PASSWORD_FIELD)
  );
  (void)memcpy(
      &post_field
          [strlen(USERNAME_FIELD) + 1 + strlen(username) +
           strlen(PASSWORD_FIELD)],
      password, strlen(password)
  );
  post_field[LOGIN_FIELD_SIZE - 3] = '"';
  post_field[LOGIN_FIELD_SIZE - 2] = '}';
  post_field[LOGIN_FIELD_SIZE - 1] = 0;
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    PRINTERR("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    goto fail;
  }

  res = extract_token(recv_buf, vzw_m2m_token, "sessionToken");
  if (res != CURLE_OK) {
    PRINTERR("Failed to get VZW M2M Token");
  }

fail:
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return res;
}
