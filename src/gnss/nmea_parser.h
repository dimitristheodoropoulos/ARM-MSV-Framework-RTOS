#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

/**
 * nmea_parser.h — NMEA 0183 Protocol Parser
 * ============================================================
 * Relevant for: u-blox (GNSS/NMEA), TechBiz (aerospace nav)
 *
 * Supports:
 *   $GPGGA — Fix data (lat, lon, altitude, satellites)
 *   $GPRMC — Recommended minimum (speed, course, date)
 *
 * Features:
 *   - Checksum verification (XOR)
 *   - Field extraction without dynamic memory
 *   - Coordinate conversion (DDMM.MMMM → decimal degrees)
 * ============================================================
 */

/* ── Fix quality indicators ─────────────────────────────────── */
#define NMEA_FIX_INVALID    0
#define NMEA_FIX_GPS        1
#define NMEA_FIX_DGPS       2

/* ── Parsed GGA sentence ────────────────────────────────────── */
typedef struct {
    int     valid;              /* 1 = parsed OK, 0 = invalid   */
    int     hour, min, sec;     /* UTC time                      */
    float   latitude;           /* decimal degrees, + = N        */
    float   longitude;          /* decimal degrees, + = E        */
    int     fix_quality;        /* 0=invalid, 1=GPS, 2=DGPS      */
    int     satellites;         /* number of satellites in use   */
    float   altitude_m;         /* altitude above MSL in metres  */
} NMEA_GGA;

/* ── Parsed RMC sentence ────────────────────────────────────── */
typedef struct {
    int     valid;              /* 1 = parsed OK, 0 = invalid   */
    int     hour, min, sec;
    char    status;             /* 'A' = active, 'V' = void      */
    float   latitude;
    float   longitude;
    float   speed_knots;
    float   course_deg;
    int     day, month, year;
} NMEA_RMC;

/* ── Public API ─────────────────────────────────────────────── */

/**
 * nmea_verify_checksum() — verify XOR checksum of sentence
 * Returns 1 if valid, 0 if invalid
 */
int nmea_verify_checksum(const char *sentence);

/**
 * nmea_parse_gga() — parse $GPGGA sentence into struct
 * Returns 1 on success, 0 on failure
 */
int nmea_parse_gga(const char *sentence, NMEA_GGA *out);

/**
 * nmea_parse_rmc() — parse $GPRMC sentence into struct
 * Returns 1 on success, 0 on failure
 */
int nmea_parse_rmc(const char *sentence, NMEA_RMC *out);

/**
 * nmea_process_line() — auto-detect sentence type and parse
 * Prints results via UART
 */
void nmea_process_line(const char *sentence);

/**
 * nmea_print_sample() — send sample sentences for demo/test
 */
void nmea_print_sample(void);

#endif /* NMEA_PARSER_H */