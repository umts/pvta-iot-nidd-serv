#ifndef STOP_H
#define STOP_H

/** @def MAX_ROUTES
 *  @brief A macro that defines the maximum number of routes expected for a
 * stop.
 *
 * Specify the maximum number of routes you expect your selected stop to have.
 */
#define MAX_ROUTES 6

/** @def MAX_DEPARTURES
 *  @brief A macro that defines the maximum number of departures expected for a
 * route.
 *
 *  Specify the maximum number of departures you expect each route to have.
 *  Applies to **ALL**  departures.
 */
#define MAX_DEPARTURES 4

/** @def STOP_JSON_BUF_SIZE
 *  @brief A macro that defines the max size for HTTP response body receive
 * buffer.
 *
 *  Actual size varies quite a bit depending on how many routes are currently
 * running.
 */
#define STOP_JSON_BUF_SIZE 10240

typedef struct Departure {
  char display_text[50];
  long etd;
} Departure;

typedef struct RouteDirection {
  char direction_code;
  int id;
  int departures_size;
  Departure departures[MAX_DEPARTURES];
} RouteDirection;

typedef struct Stop {
  unsigned long long last_updated;
  const char *id;
  int routes_size;
  RouteDirection route_directions[MAX_ROUTES];
} Stop;
#endif
