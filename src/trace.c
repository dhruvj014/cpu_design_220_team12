/* trace.c - Prints one snapshot: the step, registers, flags and memory 0x00 to TRACE_END - 1
   in binary and ASCII (format: SPEC section 10). Reads state only; never changes it. Owner: Tanisha */
#include <stdio.h>
#include "trace.h"
#include "disasm.h"

#define RULE_HEAVY "=================================================================="
#define RULE_LIGHT "------------------------------------------------------------------"

/* Prints the low count bits of value as 0s and 1s, most significant bit first. */
static void print_bits(unsigned value, int count)
{
    int i;
    for (i = count - 1; i >= 0; i--)
        printf("%c", ((value >> i) & 1) ? '1' : '0');
}

/* Prints a byte as its character if printable (0x20 to 0x7E), else '.'. */
static void print_char(u8 b)
{
    printf("%c", (b >= 0x20 && b <= 0x7E) ? (char)b : '.');
}

/* Returns 1 if addr lies in the 4-byte row that starts at row. */
static int in_row(u16 addr, int row)
{
    return addr >= row && addr < row + 4;
}

/* Prints the header lines for one executed step. */
static void print_step_header(int step, const Cpu *c, const StepInfo *info)
{
    int i;
    printf(" STEP %03d   ADDR 0x%02X   IR ", step, (unsigned)info->addr);
    print_bits(info->ir, 16);
    printf("   ");
    disasm_print(info->ir);
    printf("\n");

    for (i = 0; i < NUM_REGS; i++)
        printf(" R%d=%04X", i, (unsigned)c->r[i]);
    printf("\n");

    printf(" NEXT PC=0x%02X   Z=%d  C=%d   CHANGED: ", (unsigned)c->pc, c->z, c->c);
    if (info->wrote)
        printf("[0x%02X] 0x%04X -> 0x%04X\n", (unsigned)info->waddr, (unsigned)info->before, (unsigned)info->after);
    else
        printf("none\n");
}

/* Prints the memory table, 4 bytes per row. '>' marks the row of the instruction just run,
   '*' the row a ST or CALL wrote (the '*' wins if both are the same row). */
static void print_memory(const Cpu *c, const Memory *m, const StepInfo *info)
{
    int row, i;
    printf("  ADDR   +0       +1       +2       +3        ASCII\n");
    for (row = 0; row < TRACE_END; row += 4) {
        char mark = ' ';
        if (info != 0 && in_row(info->addr, row))
            mark = '>';
        if (info != 0 && info->wrote && in_row(info->waddr, row))
            mark = '*';
        printf("%c 0x%02X  ", mark, (unsigned)row);
        for (i = 0; i < 4; i++) {
            printf(" ");
            print_bits(m->byte[row + i], 8);
        }
        printf("  ");
        for (i = 0; i < 4; i++)
            print_char(m->byte[row + i]);
        if (row >= TRACE_TAG)
            printf("   tag");
        else if (c->layout != 0 && row >= c->layout->data_start && row < c->layout->data_end)
            printf("   data");
        printf("\n");
    }
}

/* Prints one snapshot. info == 0 prints the initial snapshot (no registers, no markers). */
void trace_print(int step, const Cpu *c, const Memory *m, const StepInfo *info)
{
    printf("%s\n", RULE_HEAVY);
    if (info == 0)
        printf(" STEP %03d   initial memory\n", step);
    else
        print_step_header(step, c, info);
    printf("%s\n", RULE_LIGHT);
    print_memory(c, m, info);
    printf("%s\n", RULE_HEAVY);
}
