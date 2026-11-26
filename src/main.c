/* main.c - Entry point of the SJ16 simulator. Owner: Dhruv.
   Phase 1: only prints usage. The run loop and commands arrive in Phase 5. */
#include <stdio.h>
#include "cpu.h"
#include "trace.h"
#include "assembler.h"
#include "compiler.h"
#include "disasm.h"
#include "cpu_layout.h"

/* Prints the list of commands. */
static void print_usage(void)
{
    printf("SJ16 CPU simulator\n");
    printf("usage: ./cpu <command>\n");
    printf("  pow        run 8^2 written in SJ16 assembly\n");
    printf("  fact       compile the C factorial to SJ16 assembly and run it\n");
    printf("  fact-hand  run the hand-written SJ16 factorial\n");
    printf("  check      compare compiled fact with fact-hand\n");
}

/* Program entry: no commands yet in Phase 1, so always print usage. */
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    print_usage();
    return 1;
}
