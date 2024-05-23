/** @headerfile json_helpers.h */
#include "json_helpers.h"

/** Compares a string with a jsmn token value. */
extern inline int jsoneq(
    const char *const json_ptr, const jsmntok_t *tok, const char *const string
);

extern inline int eval_jsmn_return(int ret);

extern inline char *json_obj_id_str(
    char *p, const void *id, const size_t id_len
);

extern inline char *json_obj_value_str(
    char *p, const void *value, const size_t value_len
);

extern inline char *json_obj_str(
    char *p, const void *id, const size_t id_len, const void *value,
    const size_t value_len
);
