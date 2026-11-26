/* main.c - Command line, run loop and program images of the SJ16 simulator. Owner: Dhruv

   ./cpu pow        assemble pow.inc and run it with a full trace
   ./cpu fact       print C source, compile it, print the assembly, assemble and run it
   ./cpu fact-hand  assemble the hand-written fact.inc and run it
   ./cpu check      compiled fact and hand-written fact must give identical memory */
#include <stdio.h>
#include "cpu.h"
#include "trace.h"
#include "assembler.h"
#include "compiler.h"
#include "disasm.h"
#include "util.h"
#include "cpu_layout.h"

/* ---------- program images ----------
   Each program is a string made of the lines of a .inc file. A pointer of 0 means "not ready yet".
   When an owner pushes the .inc file, they replace the "= 0;" line with the #include form shown in the comment. */

/* Tanisha: replace the next line with
       static const char *POW_ASM =
       #include "pow.inc"
       ;                                                                    */
static const char *POW_ASM = 0;

/* Mokshit: replace the next line with
       static const char *FACT_C =
       #include "fact_c.inc"
       ;                                                                    */
static const char *FACT_C = 0;

/* Mokshit: replace the next line with
       static const char *FACT_ASM =
       #include "fact.inc"
       ;                                                                    */
static const char *FACT_ASM = 0;

#define FACT_TAG "FACT 8! "      /* 8-character tag for the compiled program */

/* ---------- run loop ---------- */

/* Runs the program in m from the CPU_LAYOUT entry. Prints every snapshot, then RESULT or FAULT.
   result_addr is the data word printed as the result. Returns 0 after HLT, 1 on a fault or bad layout. */
static int run_program(Memory *m, u16 result_addr)
{
    Cpu cpu;
    StepInfo info;
    CpuStatus status = CPU_OK;
    int steps = 0;
    u16 result = 0;

    if (cpu_reset(&cpu, &CPU_LAYOUT) != 0) {
        printf("ERROR: invalid memory layout\n");
        return 1;
    }
    trace_print(0, &cpu, m, 0);                          /* step 0: initial memory */
    while (status == CPU_OK && steps < MAX_STEPS) {
        status = cpu_step(&cpu, m, &info);
        steps++;                                         /* HLT counts as a step */
        if (status != CPU_FAULT)
            trace_print(steps, &cpu, m, &info);
    }
    if (status == CPU_FAULT) {
        printf("FAULT at 0x%02X: %s\n", (unsigned)info.addr, info.fault);
        return 1;
    }
    if (status != CPU_HALT) {                            /* never reached HLT */
        printf("FAULT at 0x%02X: no HLT after %d steps\n", (unsigned)cpu.pc, MAX_STEPS);
        return 1;
    }
    mem_read16(m, result_addr, &result);
    printf("RESULT: %u   STEPS: %d\n", (unsigned)result, steps);
    return 0;
}

/* ---------- commands ---------- */

/* Assembles text into m under CPU_LAYOUT. Returns 0 on success, 1 (after printing the line number) on error. */
static int assemble_text(const char *text, Memory *m, const char *what)
{
    int line = assemble(text, m, &CPU_LAYOUT);
    if (line != 0) {
        printf("ERROR: %s: assembly failed on line %d\n", what, line);
        return 1;
    }
    return 0;
}

/* Prints a "not ready yet" message for a program whose .inc file has not been added. */
static int not_ready(const char *command)
{
    printf("%s: not ready yet\n", command);
    return 1;
}

/* ./cpu pow: 8^2 in SJ16 assembly; the result is at 0x34. */
static int cmd_pow(void)
{
    Memory m;
    if (POW_ASM == 0)
        return not_ready("pow");
    if (assemble_text(POW_ASM, &m, "pow") != 0)
        return 1;
    return run_program(&m, 0x34);
}

/* ./cpu fact-hand: hand-written factorial; the result is at 0x36. */
static int cmd_fact_hand(void)
{
    Memory m;
    if (FACT_ASM == 0)
        return not_ready("fact-hand");
    if (assemble_text(FACT_ASM, &m, "fact-hand") != 0)
        return 1;
    return run_program(&m, 0x36);
}

/* Compiles FACT_C into asm_out. Returns 0 on success, 1 (after printing the line number) on error. */
static int compile_fact(char *asm_out)
{
    int line = compile(FACT_C, FACT_TAG, asm_out, ASM_CAP);
    if (line != 0) {
        printf("ERROR: compile failed on line %d\n", line);
        return 1;
    }
    return 0;
}

/* ./cpu fact: show the C, compile it, show the assembly, assemble and run. */
static int cmd_fact(void)
{
    Memory m;
    char asm_text[ASM_CAP];
    if (FACT_C == 0)
        return not_ready("fact");
    printf("---- C source ----\n%s\n", FACT_C);
    if (compile_fact(asm_text) != 0)
        return 1;
    printf("---- generated SJ16 assembly ----\n%s\n", asm_text);
    if (assemble_text(asm_text, &m, "fact") != 0)
        return 1;
    return run_program(&m, 0x36);
}

/* ./cpu check: compiled image and hand-written image must match in all 256 bytes. */
static int cmd_check(void)
{
    Memory compiled, hand;
    char asm_text[ASM_CAP];
    int i;
    if (FACT_C == 0 || FACT_ASM == 0)
        return not_ready("check");
    if (compile_fact(asm_text) != 0 || assemble_text(asm_text, &compiled, "compiled fact") != 0)
        return 1;
    if (assemble_text(FACT_ASM, &hand, "fact-hand") != 0)
        return 1;
    for (i = 0; i < MEM_SIZE; i++) {
        if (compiled.byte[i] != hand.byte[i]) {
            printf("CHECK FAILED at 0x%02X: compiled=0x%02X hand=0x%02X\n",
                   (unsigned)i, (unsigned)compiled.byte[i], (unsigned)hand.byte[i]);
            return 1;
        }
    }
    printf("CHECK PASSED\n");
    return 0;
}

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

/* Program entry: runs the command named on the command line. Returns 0 on success, 1 on any failure. */
int main(int argc, char **argv)
{
    if (argc == 2) {
        if (str_eq(argv[1], "pow"))
            return cmd_pow();
        if (str_eq(argv[1], "fact"))
            return cmd_fact();
        if (str_eq(argv[1], "fact-hand"))
            return cmd_fact_hand();
        if (str_eq(argv[1], "check"))
            return cmd_check();
    }
    print_usage();
    return 1;
}
