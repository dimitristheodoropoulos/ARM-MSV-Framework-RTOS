/**
 * nmea_parser.c — NMEA 0183 Protocol Parser
 * ============================================================
 * Bare-metal implementation — no malloc, no stdio, no libc
 *
 * Relevant for: u-blox (GNSS receivers), TechBiz (aerospace)
 * ============================================================
 */

#include "nmea_parser.h"
#include "uart.h"

/* ── Internal helpers ──────────────────────────────────────── */

/**
 * str_to_int() — convert ASCII digits to integer
 * Reads up to 'n' characters.
 */
static int str_to_int(const char *s, int n)
{
    int result = 0;
    for (int i = 0; i < n && s[i] >= '0' && s[i] <= '9'; i++)
        result = result * 10 + (s[i] - '0');
    return result;
}

/**
 * str_to_float() — convert ASCII decimal to float
 * No libm dependency — handles "DDMM.MMMM" format directly.
 */
static float str_to_float(const char *s)
{
    float result = 0.0f;
    float frac   = 0.0f;
    float div    = 1.0f;
    int   in_frac = 0;

    for (int i = 0; s[i] != '\0' && s[i] != ',' && s[i] != '*'; i++) {
        if (s[i] == '.') {
            in_frac = 1;
            continue;
        }
        if (s[i] < '0' || s[i] > '9') break;

        if (!in_frac) {
            result = result * 10.0f + (s[i] - '0');
        } else {
            div  *= 10.0f;
            frac += (s[i] - '0') / div;
        }
    }
    return result + frac;
}

/**
 * ddmm_to_decimal() — convert NMEA DDMM.MMMM to decimal degrees
 *
 * NMEA encodes coordinates as DDDMM.MMMM (degrees + minutes).
 * Example: 4807.038 N → 48° 07.038' N → 48.1173° N
 */
static float ddmm_to_decimal(float ddmm)
{
    int   degrees = (int)(ddmm / 100);
    float minutes = ddmm - (degrees * 100.0f);
    return degrees + (minutes / 60.0f);
}

/**
 * get_field() — extract Nth comma-separated field from sentence
 * Writes result into 'buf' (max 'buflen' chars).
 * Returns 1 on success, 0 if field not found.
 *
 * Example: get_field("$GPGGA,123519,4807", buf, 16, 2)
 *          → buf = "4807"
 */
static int get_field(const char *sentence, char *buf,
                     int buflen, int field_num)
{
    int field = 0;
    int i     = 0;
    int j     = 0;

    while (sentence[i] != '\0') {
        if (sentence[i] == ',') {
            if (field == field_num) {
                buf[j] = '\0';
                return 1;
            }
            field++;
            j = 0;
            i++;
            continue;
        }
        if (sentence[i] == '*') {   /* checksum delimiter */
            if (field == field_num) {
                buf[j] = '\0';
                return 1;
            }
            break;
        }
        if (field == field_num && j < buflen - 1)
            buf[j++] = sentence[i];
        i++;
    }
    buf[j] = '\0';
    return (field == field_num) ? 1 : 0;
}

/* ── Checksum ───────────────────────────────────────────────── */

/**
 * nmea_verify_checksum() — XOR all bytes between '$' and '*'
 *
 * NMEA checksum = XOR of all characters between $ and *
 * Format: $...*HH where HH is two hex digits
 */
int nmea_verify_checksum(const char *sentence)
{
    unsigned char calc = 0;
    int i = 1;  /* skip '$' */

    while (sentence[i] != '*' && sentence[i] != '\0')
        calc ^= (unsigned char)sentence[i++];

    if (sentence[i] != '*') return 0;   /* no checksum found */

    /* Parse two hex digits after '*' */
    const char *hex_str = sentence + i + 1;
    unsigned char recv = 0;

    for (int d = 0; d < 2; d++) {
        char c = hex_str[d];
        recv <<= 4;
        if      (c >= '0' && c <= '9') recv |= (c - '0');
        else if (c >= 'A' && c <= 'F') recv |= (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') recv |= (c - 'a' + 10);
        else return 0;  /* invalid hex digit */
    }

    return (calc == recv) ? 1 : 0;
}

/* ── GGA Parser ─────────────────────────────────────────────── */

/**
 * nmea_parse_gga() — parse $GPGGA sentence
 *
 * Field map:
 *   0  $GPGGA
 *   1  HHMMSS.ss  UTC time
 *   2  DDMM.MMMM  Latitude
 *   3  N/S
 *   4  DDDMM.MMMM Longitude
 *   5  E/W
 *   6  Fix quality
 *   7  Satellites
 *   8  HDOP
 *   9  Altitude
 *   10 M (metres)
 */
int nmea_parse_gga(const char *sentence, NMEA_GGA *out)
{
    char buf[16];

    out->valid = 0;

    if (!nmea_verify_checksum(sentence)) return 0;

    /* Time — field 1: HHMMSS */
    if (!get_field(sentence, buf, sizeof(buf), 1)) return 0;
    out->hour = str_to_int(buf,     2);
    out->min  = str_to_int(buf + 2, 2);
    out->sec  = str_to_int(buf + 4, 2);

    /* Latitude — field 2 + hemisphere field 3 */
    if (!get_field(sentence, buf, sizeof(buf), 2)) return 0;
    out->latitude = ddmm_to_decimal(str_to_float(buf));
    if (!get_field(sentence, buf, sizeof(buf), 3)) return 0;
    if (buf[0] == 'S') out->latitude = -out->latitude;

    /* Longitude — field 4 + hemisphere field 5 */
    if (!get_field(sentence, buf, sizeof(buf), 4)) return 0;
    out->longitude = ddmm_to_decimal(str_to_float(buf));
    if (!get_field(sentence, buf, sizeof(buf), 5)) return 0;
    if (buf[0] == 'W') out->longitude = -out->longitude;

    /* Fix quality — field 6 */
    if (!get_field(sentence, buf, sizeof(buf), 6)) return 0;
    out->fix_quality = str_to_int(buf, 1);

    /* Satellites — field 7 */
    if (!get_field(sentence, buf, sizeof(buf), 7)) return 0;
    out->satellites = str_to_int(buf, 2);

    /* Altitude — field 9 */
    if (!get_field(sentence, buf, sizeof(buf), 9)) return 0;
    out->altitude_m = str_to_float(buf);

    out->valid = 1;
    return 1;
}

/* ── RMC Parser ─────────────────────────────────────────────── */

/**
 * nmea_parse_rmc() — parse $GPRMC sentence
 *
 * Field map:
 *   0  $GPRMC
 *   1  HHMMSS     UTC time
 *   2  A/V        Status (A=active, V=void)
 *   3  DDMM.MMMM  Latitude
 *   4  N/S
 *   5  DDDMM.MMMM Longitude
 *   6  E/W
 *   7  Speed (knots)
 *   8  Course (degrees)
 *   9  DDMMYY     Date
 */
int nmea_parse_rmc(const char *sentence, NMEA_RMC *out)
{
    char buf[16];

    out->valid = 0;

    if (!nmea_verify_checksum(sentence)) return 0;

    /* Time */
    if (!get_field(sentence, buf, sizeof(buf), 1)) return 0;
    out->hour = str_to_int(buf,     2);
    out->min  = str_to_int(buf + 2, 2);
    out->sec  = str_to_int(buf + 4, 2);

    /* Status */
    if (!get_field(sentence, buf, sizeof(buf), 2)) return 0;
    out->status = buf[0];
    if (out->status != 'A') return 0;   /* void fix — no data */

    /* Latitude */
    if (!get_field(sentence, buf, sizeof(buf), 3)) return 0;
    out->latitude = ddmm_to_decimal(str_to_float(buf));
    if (!get_field(sentence, buf, sizeof(buf), 4)) return 0;
    if (buf[0] == 'S') out->latitude = -out->latitude;

    /* Longitude */
    if (!get_field(sentence, buf, sizeof(buf), 5)) return 0;
    out->longitude = ddmm_to_decimal(str_to_float(buf));
    if (!get_field(sentence, buf, sizeof(buf), 6)) return 0;
    if (buf[0] == 'W') out->longitude = -out->longitude;

    /* Speed */
    if (!get_field(sentence, buf, sizeof(buf), 7)) return 0;
    out->speed_knots = str_to_float(buf);

    /* Course */
    if (!get_field(sentence, buf, sizeof(buf), 8)) return 0;
    out->course_deg = str_to_float(buf);

    /* Date — DDMMYY */
    if (!get_field(sentence, buf, sizeof(buf), 9)) return 0;
    out->day   = str_to_int(buf,     2);
    out->month = str_to_int(buf + 2, 2);
    out->year  = str_to_int(buf + 4, 2) + 2000;

    out->valid = 1;
    return 1;
}

/* ── UART print helpers ─────────────────────────────────────── */

static void print_int(int val)
{
    if (val < 0) { uart_putc('-'); val = -val; }
    if (val >= 10) print_int(val / 10);
    uart_putc('0' + (val % 10));
}

static void print_float(float val, int decimals)
{
    if (val < 0.0f) { uart_putc('-'); val = -val; }
    int i = (int)val;
    print_int(i);
    uart_putc('.');
    for (int d = 0; d < decimals; d++) {
        val = (val - (int)val) * 10.0f;
        uart_putc('0' + (int)val);
    }
}

/* ── Public API ─────────────────────────────────────────────── */

void nmea_process_line(const char *sentence)
{
    /* Identify sentence type from characters 3-5 */
    if (sentence[3] == 'G' && sentence[4] == 'G' && sentence[5] == 'A') {
        NMEA_GGA gga;
        if (nmea_parse_gga(sentence, &gga)) {
            uart_puts("[GGA] Time=");
            print_int(gga.hour); uart_putc(':');
            print_int(gga.min);  uart_putc(':');
            print_int(gga.sec);
            uart_puts("  Lat=");  print_float(gga.latitude,  5);
            uart_puts("  Lon=");  print_float(gga.longitude, 5);
            uart_puts("  Alt=");  print_float(gga.altitude_m, 1);
            uart_puts("m  Sats=");print_int(gga.satellites);
            uart_puts("\r\n");
        } else {
            uart_puts("[GGA] Checksum error or invalid fix\r\n");
        }

    } else if (sentence[3] == 'R' && sentence[4] == 'M' && sentence[5] == 'C') {
        NMEA_RMC rmc;
        if (nmea_parse_rmc(sentence, &rmc)) {
            uart_puts("[RMC] Date=");
            print_int(rmc.day);   uart_putc('/');
            print_int(rmc.month); uart_putc('/');
            print_int(rmc.year);
            uart_puts("  Speed="); print_float(rmc.speed_knots, 1);
            uart_puts("kn  Course="); print_float(rmc.course_deg, 1);
            uart_puts("deg\r\n");
        } else {
            uart_puts("[RMC] Void fix or checksum error\r\n");
        }

    } else {
        uart_puts("[NMEA] Unknown sentence type\r\n");
    }
}

void nmea_print_sample(void)
{
    const char *gga =
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
    const char *rmc =
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";

    uart_puts("\r\n[NMEA] Sample sentences:\r\n");
    uart_puts(gga); uart_puts("\r\n");
    uart_puts(rmc); uart_puts("\r\n");

    uart_puts("[NMEA] Parsed output:\r\n");
    nmea_process_line(gga);
    nmea_process_line(rmc);
}