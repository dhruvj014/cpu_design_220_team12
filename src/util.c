/* util.c - Small string and number helpers; no library calls. Owner: Dhruv */
#include "util.h"

/* Returns 1 if the two strings are identical, else 0. */
int str_eq(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b)
            return 0;
        a++;
        b++;
    }
    return *a == *b;   /* equal only if both ended together */
}

/* Returns the number of characters before the terminating NUL. */
int str_len(const char *s)
{
    int n = 0;
    while (s[n] != '\0')
        n++;
    return n;
}

/* Returns 1 for space, tab or carriage return (not newline). */
int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

/* Returns 1 for '0' to '9'. */
int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

/* Returns 1 for a letter or underscore. */
int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* Returns the value 0-15 of a hex digit, or -1 if c is not one. */
static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* Parses decimal or 0x hex with optional leading '-'. 0 and *out set, or -1 if malformed or magnitude > 65535. */
int parse_number(const char *s, long *out)
{
    long value = 0;
    int negative = 0;
    int base = 10;
    int digits = 0;   /* how many digits were read */

    if (*s == '-') {
        negative = 1;
        s++;
    }
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    }
    while (*s != '\0') {
        int d = hex_value(*s);
        if (d < 0 || d >= base)
            return -1;                 /* not a digit of this base */
        value = value * base + d;
        if (value > 65535)
            return -1;                 /* too large (checked each step, so no overflow) */
        digits++;
        s++;
    }
    if (digits == 0)
        return -1;                     /* empty, "-" or "0x" */
    *out = negative ? -value : value;
    return 0;
}
