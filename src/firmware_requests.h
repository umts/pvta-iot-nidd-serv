/** @file firmware_requests.h
 *  @brief Macro and function defines for iot firmware related requests.
 */

#ifndef FIRMWARE_REQUESTS_H
#define FIRMWARE_REQUESTS_H

#include <stdio.h>

/** @fn int download_firmware_github(FILE **fptr);
 *  @brief Caches and sends latest firmware from github releases.
 */
int download_firmware_github(FILE **fptr);
#endif
