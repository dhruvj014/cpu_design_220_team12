/* util.h - Small string and number helpers (no library calls). Owner: Dhruv */
#ifndef UTIL_H
#define UTIL_H
int str_eq(const char *a, const char *b);        /* 1 if identical, else 0 */
int str_len(const char *s);
int is_space(char c);                            /* space, tab or carriage return (not newline) */
int is_digit(char c);
int is_alpha(char c);                            /* letters and underscore */
/* Whole string must be decimal or 0x hex, with optional leading '-'.
   Returns 0 and sets *out, or -1 if malformed or magnitude > 65535. */
int parse_number(const char *s, long *out);
#endif
