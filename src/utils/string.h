#ifndef STRING_H
#define STRING_H

/**
 * string.h — Minimal String Library
 * ============================================================
 * Bare-metal <string.h> replacement — no libc
 * ============================================================
 */

unsigned int  strlen (const char *s);
int           strcmp (const char *s1, const char *s2);
int           strncmp(const char *s1, const char *s2, unsigned int n);
char         *strcpy (char *dst, const char *src);
char         *strncpy(char *dst, const char *src, unsigned int n);
char         *strcat (char *dst, const char *src);
char         *strchr (const char *s, int c);
void         *memset (void *ptr, int value, unsigned int n);
void         *memcpy (void *dst, const void *src, unsigned int n);
int           memcmp (const void *a, const void *b, unsigned int n);
char         *int_to_str(unsigned int val, char *buf);

#endif /* STRING_H */