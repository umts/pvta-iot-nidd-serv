/** @file json_helpers.h
 *  @brief Macro and function defines for the NIDD client.
 */

#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

#define JSMN_HEADER

#include <config.h>
#include <jsmn.h>
#include <nxt_unit.h>
#include <string.h>

#define json_obj_value_num(_p_, _v_) _p_ + sprintf(_p_, "%d", _v_)

/** Compares a string with a jsmn token value. */
inline int jsoneq(
    const char *const json_ptr, const jsmntok_t *tok, const char *const string
) {
  if (tok->type == JSMN_STRING &&
      (int)strlen(string) == tok->end - tok->start &&
      strncmp(json_ptr + tok->start, string, tok->end - tok->start) == 0) {
    return 1;
  }
  return 0;
}

inline int eval_jsmn_return(int ret) {
  switch (ret) {
    case 0:
      PRINTERR("Parsed Empty JSON string.\n");
      return 1;
    case JSMN_ERROR_NOMEM:
      PRINTERR("Failed to parse JSON; Not enough tokens were provided.");
      return 1;
    case JSMN_ERROR_INVAL:
      PRINTERR("Failed to parse JSON; Invalid character inside JSON string.");
      return 1;
    case JSMN_ERROR_PART:
      PRINTERR(
          "Failed to parse JSON; The string is not a full JSON packet, more "
          "bytes expected."
      );
      return 1;
    default:
      // PRINTSUCCES("Tokens allocated: %d/%d\n", ret, 8);
      return 0;
  }
}

inline char *json_obj_id_str(char *p, const void *id, const size_t id_len) {
  *p++ = '\"';
  memcpy(p, id, id_len);
  p += id_len;
  *p++ = '\"';
  *p++ = ':';
  return p;
}

inline char *json_obj_value_str(
    char *p, const void *value, const size_t value_len
) {
  *p++ = '\"';
  memcpy(p, value, value_len);
  p += value_len;
  *p++ = '\"';
  return p;
}

inline char *json_obj_str(
    char *p, const void *id, const size_t id_len, const void *value,
    const size_t value_len
) {
  *p++ = '\"';
  memcpy(p, id, id_len);
  p += id_len;
  *p++ = '\"';
  *p++ = ':';
  *p++ = '\"';
  memcpy(p, value, value_len);
  p += value_len;
  *p++ = '\"';
  return p;
}
#endif
