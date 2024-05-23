/** @file vzw_connect.h
 *  @brief Macro and function defines for the vzw thingspace credentials.
 */

#ifndef VZW_CONNECT_H
#define VZW_CONNECT_H
#include <stop.h>

/** @fn int stop_to_nidd(nxt_unit_request_info_t* req, Stop* stop)
 *  @brief Transforms the Stop departure info into a NIDD message.
 */
// char *stop_to_nidd(Stop *stop);

/** @fn int get_vzw_auth_token(const char *username, const char *password, const
 * char *vzw_auth_keys, char *vzw_auth_token)
 *  @brief Retrieves a Thingspace auth token through a POST request.
 */
int get_vzw_auth_token(const char *vzw_auth_keys, char *vzw_auth_token);

/** @fn int get_vzw_m2m_token(const char *username, const char *password, const
 * char *vzw_auth_token, char *vzw_m2m_token)
 *  @brief Retrieves a Thingspace M2M token through a POST request.
 */
int get_vzw_m2m_token(
    const char *username, const char *password, const char *vzw_auth_token,
    char *vzw_m2m_token
);
#endif
