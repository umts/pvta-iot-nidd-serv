/** @headerfile parse_stop_json.h */
#include "parse_stop_json.h"

#define JSMN_HEADER

#include <config.h>
#include <jsmn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stop.h>
#include <string.h>
#include <time.h>

#include "json_helpers.h"

/** The number of keys + values in for each object type in our JSON string */
#define DEPARTURE_TOK_COUNT 18
#define HEADWAY_TOK_COUNT 14
#define ROUTE_DIRECTION_TOK_COUNT \
  (12 + (MAX_DEPARTURES * (DEPARTURE_TOK_COUNT + HEADWAY_TOK_COUNT)))
#define STOP_TOK_COUNT (6 + (MAX_ROUTES * ROUTE_DIRECTION_TOK_COUNT))

/** With jsmn JSON objects count as a token, so we need to offset by an
 *  additional 1 for each level deeper we go.
 *  These offsets need to be temporary though, because objects don't have a
 *  value to account for. At least that's what I believe the reason to be...
 */
// #define STOP_TOK tokens[t + 1]
#define ROUTE_DIRECTION_TOK tokens[t + rdir + 2]
#define DEPARTURE_TOK tokens[t + rdir + dep + 3]

static int unique_disply_text(
    const char *const json_ptr, RouteDirection *route_direction,
    const jsmntok_t *tok, size_t valid_departure_count
) {
  for (size_t i = 0; i < valid_departure_count; i++) {
    Departure *departure = &route_direction->departures[i];
    if (jsoneq(json_ptr, tok, departure->display_text)) {
      return 0;
    }
  }
  return 1;
}

/** Iterates through the Departures array objects to find desired values. */
static int parse_departures(
    const char *const json_ptr, int t, jsmntok_t tokens[], int rdir,
    size_t departures_count, RouteDirection *route_direction,
    const long time_now
) {
  size_t valid_departure_count = 0;

  for (size_t dep_num = 0; dep_num < departures_count; dep_num++) {
    Departure *departure = &route_direction->departures[valid_departure_count];
    bool departure_uniq = false;
    bool edt_in_future = false;

    /** Increase t by 1 to step into the object */
    t++;

    /** The number of keys in the Departure object.
     *
     *  ROUTE_DIRECTION_TOK is a Departure object,
     *  so ROUTE_DIRECTION_TOK.size is the number of Departure keys.
     */
    size_t departure_size = ROUTE_DIRECTION_TOK.size;
    for (size_t dep = 0; dep < (departure_size * 2); dep++) {
      if (jsoneq(json_ptr, &DEPARTURE_TOK, "DisplayText")) {
        dep++;
        departure_uniq = unique_disply_text(
            json_ptr, route_direction, &DEPARTURE_TOK, valid_departure_count
        );

        if (departure_uniq) {
          memcpy(
              departure->display_text, json_ptr + DEPARTURE_TOK.start,
              DEPARTURE_TOK.end - DEPARTURE_TOK.start
          );
          departure->display_text[(DEPARTURE_TOK.end - DEPARTURE_TOK.start)] =
              '\0';
          if (edt_in_future) {
            goto check_validity;
          }
        } else {
          break;
        }
      } else if (jsoneq(json_ptr, &DEPARTURE_TOK, "EDT")) {
        dep++;
        /* EDT ex: /Date(1648627309163-0400)\, where 1648627309163 is ms since
         * the epoch. We increase the pointer by 6 to remove the leading
         * '/Date(' for atoi(). This also ignores the timezone which we don't
         * need.
         */
        const char *const edt_string = json_ptr + DEPARTURE_TOK.start + 7;
        long edt = atol(edt_string);
        PRINTDBG("* EDT: %ld", edt);
        if (edt > time_now) {
          departure->etd = edt;
          edt_in_future = true;
          if (departure_uniq) {
            goto check_validity;
          }
        } else if (departure_uniq) {
          departure->display_text[0] = '\0';
          break;
        } else {
          break;
        }
      } else if (jsoneq(json_ptr, &DEPARTURE_TOK, "ETA")) {
        dep++;
      } else if (jsoneq(json_ptr, &DEPARTURE_TOK, "GoogleTripId")) {
        dep++;
      } else if (jsoneq(json_ptr, &DEPARTURE_TOK, "LastUpdated")) {
        dep++;
      } else if (jsoneq(json_ptr, &DEPARTURE_TOK, "SDT")) {
        dep++;
      } else if (jsoneq(json_ptr, &DEPARTURE_TOK, "STA")) {
        dep++;
      } else if (jsoneq(json_ptr, &DEPARTURE_TOK, "VehicleId")) {
        dep++;
      } else {
        dep++;
        PRINTALRT(
            "Unexpected key in Departures: %.*s",
            DEPARTURE_TOK.end - DEPARTURE_TOK.start,
            json_ptr + DEPARTURE_TOK.start
        );
      }

    check_validity:
      if (departure_uniq && edt_in_future) {
        valid_departure_count++;
        break;
      }
    }
    t += (departure_size * 2);
  }
  route_direction->departures_size = valid_departure_count;
  return t;
}

/** Iterates through the RouteDirections array objects to find desired values.
 */
static int parse_route_directions(
    const char *const json_ptr, int t, jsmntok_t tokens[],
    size_t route_directions_count, Stop *stop, const long time_now
) {
  size_t valid_route_count = 0;

  for (size_t rd_num = 0; rd_num < route_directions_count; rd_num++) {
    RouteDirection *route_direction =
        &stop->route_directions[valid_route_count];
    /** The number of keys in the RouteDirection object.
     *
     *  tokens[t + 1] is a RouteDirection object,
     *  so tokens[t + 1].size is the number of RouteDirection keys.
     */
    size_t route_direction_size = tokens[t + 1].size;
    /* jsmntok::size is the number of keys but we're iterating over keys and
     * values, so we need to double the size to get the true count.
     */
    for (size_t rdir = 0; rdir < (route_direction_size * 2); rdir++) {
      if (jsoneq(json_ptr, &ROUTE_DIRECTION_TOK, "Direction")) {
        rdir++;
        route_direction->direction_code =
            *(json_ptr + ROUTE_DIRECTION_TOK.start);
        PRINTDBG("- Direction: %c", *(json_ptr + ROUTE_DIRECTION_TOK.start));
      } else if (jsoneq(json_ptr, &ROUTE_DIRECTION_TOK, "IsDone")) {
        rdir++;
      } else if (jsoneq(json_ptr, &ROUTE_DIRECTION_TOK, "IsHeadway")) {
        rdir++;
      } else if (jsoneq(json_ptr, &ROUTE_DIRECTION_TOK, "RouteId")) {
        rdir++;
        route_direction->id = atoi(json_ptr + ROUTE_DIRECTION_TOK.start);
        PRINTDBG("- RouteId: %d", atoi(json_ptr + ROUTE_DIRECTION_TOK.start));
      } else if (jsoneq(json_ptr, &ROUTE_DIRECTION_TOK, "Departures")) {
        rdir++;
        if ((ROUTE_DIRECTION_TOK.type == JSMN_ARRAY) &&
            (ROUTE_DIRECTION_TOK.size > 0)) {
          t = parse_departures(
              json_ptr, t, tokens, rdir, ROUTE_DIRECTION_TOK.size,
              route_direction, time_now
          );
          valid_route_count++;
        } else {
          break;
        }
      } else if (jsoneq(json_ptr, &ROUTE_DIRECTION_TOK, "HeadwayDepartures")) {
        rdir++;
        if ((ROUTE_DIRECTION_TOK.type == JSMN_ARRAY) &&
            (ROUTE_DIRECTION_TOK.size > 0)) {
          /* We don't care about this array but we do need to account for it's
           * size */
          t += (ROUTE_DIRECTION_TOK.size * 2) + 1;
        }
      } else {
        /* We don't care about other keys so skip their values */
        rdir++;
        PRINTALRT(
            "Unexpected key in RouteDirections: %.*s",
            ROUTE_DIRECTION_TOK.end - ROUTE_DIRECTION_TOK.start,
            json_ptr + ROUTE_DIRECTION_TOK.start
        );
      }
    }
    /* Increase t by an additional 1 to step into the next object */
    t += (route_direction_size * 2) + 1;
  }
  stop->routes_size = valid_route_count;
  return t;
}

/** Parses a JSON string with jsmn, then iterates through the jsmn key tokens
 *  to find desired values.
 */
int parse_stop_json(const char *const json_ptr, Stop *stop) {
  jsmn_parser p;

  /** The number of maximum possible tokens we expect in our JSON string + 1 for
   * the \0 delimiter. */
  jsmntok_t tokens[STOP_TOK_COUNT + 1];

  /** The jsmn token counter. */
  int t;
  int ret;

  (void)jsmn_init(&p);
  struct timespec ts;
  ret = timespec_get(&ts, TIME_UTC);
  if (!ret) {
    PRINTERR("Failed to get current time");
    return 1;
  }

  /** The number of tokens *allocated* from tokens array to parse the JSON
   * string */
  ret = jsmn_parse(
      &p, json_ptr, strlen(json_ptr), tokens, sizeof(tokens) / sizeof(jsmntok_t)
  );

  if (eval_jsmn_return(ret)) {
    return 1;
  }
  PRINTDBG("Tokens allocated: %d/%d", ret, STOP_TOK_COUNT);

  /* Set the starting position for t */
  switch (tokens[0].type) {
    case JSMN_ARRAY:
      /* If the root token is an array, we assume the next token is an object;
       * t is set to 2 so we can start in the object.
       */
      t = 2;
      break;
    case JSMN_OBJECT:
      /* If the root token an object t is set to 1 so we can start in the
       * object. */
      t = 1;
      break;
    default:
      PRINTERR("Top level token isn't an array or object.");
      return EXIT_FAILURE;
  }

  const long time_now = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
  if (time_now < 0) {
    PRINTERR("Time now is in the past!");
    return 1;
  }

  /* We want to loop over all the keys of the root object.
   * We know the token after a key is a value so we skip that iteration.
   */
  while (t < ret) {
    if (jsoneq(json_ptr, &tokens[t], "LastUpdated")) {
      t++;
      const char *const last_updated_string = json_ptr + tokens[t].start + 7;
      PRINTDBG("LastUpdated: %lld", strtoll(last_updated_string, NULL, 10));
      unsigned long long new_last_updated =
          strtoll(last_updated_string, NULL, 10);
      if (stop->last_updated < new_last_updated) {
        stop->last_updated = new_last_updated;
      } else {
        PRINTALRT("StopDepartures not updated, skipping.");
        break;
      }
    } else if (jsoneq(json_ptr, &tokens[t], "RouteDirections")) {
      /* Increase t by an additional 1 to step into the array */
      t++;
      /* tokens[t] is the RouteDirections array in this case,
       * so tokens[t].size is the number of RouteDirections
       */
      if (tokens[t].type == JSMN_ARRAY && tokens[t].size > 0) {
        t = parse_route_directions(
            json_ptr, t, tokens, tokens[t].size, stop, time_now
        );
      } else {
        PRINTERR("No RouteDirections to parse.");
        break;
      }
    } else if (jsoneq(json_ptr, &tokens[t], "StopId")) {
      t++;
      /** TODO: Maybe verify the ID matches the one requested? Seems silly. */
      PRINTDBG("StopId: %d", atoi(json_ptr + tokens[t].start));
    } else {
      t++;
      PRINTALRT(
          "Unexpected key in Stop: %.*s", tokens[t].end - tokens[t].start,
          json_ptr + tokens[t].start
      );
    }
    t++;
  }
  return EXIT_SUCCESS;
}
