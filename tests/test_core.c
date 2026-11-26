/* test_core.c - Unit tests for the core and util. Owner: Dhruv.
   Phase 1: only the expect() helper; tests are added from Phase 2. */
#include <stdio.h>

static int failures = 0;   /* number of failed checks */
static int checks = 0;     /* number of checks run */

/* Records one check; prints a line only when it fails. */
static void expect(int condition, const char *name)
{
    checks++;
    if (!condition) {
        failures++;
        printf("FAIL: %s\n", name);
    }
}

/* Runs all tests and returns 1 if any failed. */
int main(void)
{
    expect(1, "test harness works");
    printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
