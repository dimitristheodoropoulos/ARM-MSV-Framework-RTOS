/**
 * string.c — Minimal String Library
 * ============================================================
 * Bare-metal replacement for <string.h>
 * No libc dependency — pure C, no dynamic allocation.
 *
 * Relevant for:
 *   ALL targets — shell parsing, NMEA parsing, AT commands
 *
 * Functions:
 *   strlen    — string length
 *   strcmp    — string compare
 *   strncmp   — string compare (max n chars)
 *   strcpy    — string copy
 *   strncpy   — string copy (max n chars)
 *   strcat    — string concatenate
 *   strchr    — find character in string
 *   memset    — fill memory block
 *   memcpy    — copy memory block
 *   memcmp    — compare memory blocks
 *   int_to_str— integer to ASCII string
 * ============================================================
 */

#include "string.h"

/* ── strlen ─────────────────────────────────────────────────── */

unsigned int strlen(const char *s)
{
    unsigned int len = 0;
    while (s[len]) len++;
    return len;
}

/* ── strcmp ─────────────────────────────────────────────────── */

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/* ── strncmp ────────────────────────────────────────────────── */

int strncmp(const char *s1, const char *s2, unsigned int n)
{
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/* ── strcpy ─────────────────────────────────────────────────── */

char *strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

/* ── strncpy ────────────────────────────────────────────────── */

char *strncpy(char *dst, const char *src, unsigned int n)
{
    char *ret = dst;
    while (n && (*dst++ = *src++)) n--;
    /* Pad remaining bytes with '\0' per C standard */
    while (n--) *dst++ = '\0';
    return ret;
}

/* ── strcat ─────────────────────────────────────────────────── */

char *strcat(char *dst, const char *src)
{
    char *ret = dst;
    while (*dst) dst++;         /* advance to end of dst */
    while ((*dst++ = *src++));  /* append src */
    return ret;
}

/* ── strchr ─────────────────────────────────────────────────── */

char *strchr(const char *s, int c)
{
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : 0;
}

/* ── memset ─────────────────────────────────────────────────── */

void *memset(void *ptr, int value, unsigned int n)
{
    unsigned char *p = (unsigned char *)ptr;
    while (n--) *p++ = (unsigned char)value;
    return ptr;
}

/* ── memcpy ─────────────────────────────────────────────────── */

void *memcpy(void *dst, const void *src, unsigned int n)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

/* ── memcmp ─────────────────────────────────────────────────── */

int memcmp(const void *a, const void *b, unsigned int n)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    while (n--) {
        if (*pa != *pb) return (int)*pa - (int)*pb;
        pa++; pb++;
    }
    return 0;
}

/* ── int_to_str ─────────────────────────────────────────────── */

/**
 * int_to_str() — convert unsigned integer to decimal ASCII
 * Writes result into buf (must be at least 11 bytes for UINT32_MAX).
 * Returns pointer to buf.
 *
 * Used by: shell status, power stats, NMEA print helpers
 */
char *int_to_str(unsigned int val, char *buf)
{
    char tmp[12];
    int  i = 0;

    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }

    /* Reverse */
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
    return buf;
}