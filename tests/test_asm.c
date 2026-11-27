/* test_asm.c - Unit tests for the assembler. Owner: Tanisha */
#include <stdio.h>
#include "assembler.h"
#include "cpu_layout.h"

static const char *POW_ASM =
#include "pow.inc"
;

/* The fact listing from SPEC section 9, written as the compiler's output format (SPEC section 8). */
static const char *FACT_LISTING =
    "        .org 0x00\n"
    "        LDI  R1, 2\n"
    "        ST   R1, [R0+0x34]\n"
    "loop:   LD   R1, [R0+0x34]\n"
    "        LD   R2, [R0+0x30]\n"
    "        CMP  R2, R1\n"
    "        JC   done\n"
    "        LD   R1, [R0+0x32]\n"
    "        LD   R2, [R0+0x34]\n"
    "        MUL  R1, R1, R2\n"
    "        ST   R1, [R0+0x32]\n"
    "        LD   R1, [R0+0x34]\n"
    "        ADDI R1, R1, 1\n"
    "        ST   R1, [R0+0x34]\n"
    "        JMP  loop\n"
    "done:   LD   R1, [R0+0x32]\n"
    "        ST   R1, [R0+0x36]\n"
    "        HLT\n"
    "        .org 0x30\n"
    "        .word 8\n"
    "        .word 1\n"
    "        .word 0\n"
    "        .word 0\n"
    "        .org 0x38\n"
    "        .ascii \"FACT 8! \"\n";

/* Golden words from SPEC section 9. */
static const u16 POW_WORDS[11] = {
    0x2230, 0x2432, 0x1601, 0x3634, 0xB080, 0xD014, 0x66C8, 0x3634, 0xA4BF, 0xC008, 0x0000
};
static const u16 FACT_WORDS[17] = {
    0x1202, 0x3234, 0x2234, 0x2430, 0xB088, 0xF01C, 0x2232, 0x2434, 0x6250,
    0x3232, 0x2234, 0xA241, 0x3234, 0xC004, 0x2232, 0x3236, 0x0000
};

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

/* Returns the big-endian word at addr. */
static u16 word_at(const Memory *m, int addr)
{
    return (u16)((m->byte[addr] << 8) | m->byte[addr + 1]);
}

/* Returns 1 if the words from address 0 equal want[0..count-1]. */
static int words_match(const Memory *m, const u16 *want, int count)
{
    int i;
    for (i = 0; i < count; i++)
        if (word_at(m, 2 * i) != want[i])
            return 0;
    return 1;
}

/* Returns 1 if bytes from addr equal the characters of text. */
static int bytes_match(const Memory *m, int addr, const char *text)
{
    int i;
    for (i = 0; text[i] != '\0'; i++)
        if (m->byte[addr + i] != (u8)text[i])
            return 0;
    return 1;
}

/* Returns 1 if every byte in [from, to) is 0. */
static int all_zero(const Memory *m, int from, int to)
{
    int i;
    for (i = from; i < to; i++)
        if (m->byte[i] != 0)
            return 0;
    return 1;
}

/* Checks that assembling src under layout l returns want (0 = success, else a line number). */
static void expect_line(const char *src, const Layout *l, int want, const char *name)
{
    Memory m;
    expect(assemble(src, &m, l) == want, name);
}

/* pow.inc must give the 11 golden words, the data and the tag, and nothing else. */
static void test_pow(void)
{
    Memory m;
    expect(assemble(POW_ASM, &m, &CPU_LAYOUT) == 0, "pow assembles");
    expect(words_match(&m, POW_WORDS, 11), "pow: 11 code words match");
    expect(all_zero(&m, 0x16, 0x30), "pow: rest of code is 0");
    expect(word_at(&m, 0x30) == 8 && word_at(&m, 0x32) == 2, "pow: base 8, exponent 2");
    expect(word_at(&m, 0x34) == 0 && word_at(&m, 0x36) == 0, "pow: result and spare word 0");
    expect(bytes_match(&m, 0x38, "POW 8^2 "), "pow: tag");
    expect(all_zero(&m, 0x40, MEM_SIZE), "pow: stack region is 0");
}

/* The fact listing must give the 17 golden words, the data and the tag. */
static void test_fact(void)
{
    Memory m;
    expect(assemble(FACT_LISTING, &m, &CPU_LAYOUT) == 0, "fact assembles");
    expect(words_match(&m, FACT_WORDS, 17), "fact: 17 code words match");
    expect(all_zero(&m, 0x22, 0x30), "fact: rest of code is 0");
    expect(word_at(&m, 0x30) == 8 && word_at(&m, 0x32) == 1, "fact: n 8, result 1");
    expect(word_at(&m, 0x34) == 0 && word_at(&m, 0x36) == 0, "fact: i and return slot 0");
    expect(bytes_match(&m, 0x38, "FACT 8! "), "fact: tag");
}

/* Every error must report its own line number (CPU_LAYOUT). */
static void test_errors(void)
{
    const Layout *l = &CPU_LAYOUT;
    expect_line("  LDI R1, 1\n  FOO R1, R2\n",        l, 2, "unknown mnemonic");
    expect_line("  HLT\n\n  ADD R8, R1, R2\n",        l, 3, "register R8");
    expect_line("  ADD R1, R2 R3\n",                  l, 1, "missing comma");
    expect_line("  LDI R1, 511\n  LDI R1, 512\n",     l, 2, "LDI R1, 512");
    expect_line("  ADDI R1, R1, 31\n  ADDI R1, R1, 32\n", l, 2, "ADDI R1, R1, 32");
    expect_line("  LD R1, [R0+63]\n  LD R1, [R0+64]\n", l, 2, "offset 64");
    expect_line("  JMP 0x03\n",                       l, 1, "odd jump target");
    expect_line("  JMP 0x2E\n  JMP 0x30\n",           l, 2, "jump to 0x30");
    expect_line("start: HLT\n  JMP nowhere\n",        l, 2, "unknown label");
    expect_line("a: HLT\nb: HLT\na: HLT\n",           l, 3, "duplicate label");
    expect_line("  .org 0x2E\n  HLT\n  HLT\n",        l, 3, "instruction at 0x30");
    expect_line("  .org 0xFC\n  .ascii \"WXYZ\"\n  .org 0xFC\n  .ascii \"WXYZA\"\n", l, 4, ".ascii past 0xFF");
    expect_line("  OR R1, R2, R3\n",                  l, 1, "OR R1, R2, R3");
    expect_line("  CALL 0x0C\n  CALL 0x0D\n",         l, 2, "CALL to an odd address");
    expect_line("  RET\n  RET R1\n",                  l, 2, "RET R1");
}

/* Extra syntax rules from SPEC section 7. */
static void test_syntax(void)
{
    Memory m;
    const Layout *l = &CPU_LAYOUT;

    expect(assemble("  JMP end\nmid:\nend: HLT", &m, l) == 0 && word_at(&m, 0) == 0xC002,
           "forward label, label alone, no final newline");
    expect(assemble("  .org 0x38\n  .ascii \"a;b\"   ; comment\n", &m, l) == 0 && bytes_match(&m, 0x38, "a;b")
           && m.byte[0x3B] == 0, "';' inside .ascii is text");
    expect(assemble("\tADDI\tR1 ,R1,  -32\n", &m, l) == 0 && word_at(&m, 0) == 0xA260, "tabs and spaces around tokens");
    expect(assemble("", &m, l) == 0 && all_zero(&m, 0, MEM_SIZE), "empty source gives all-zero memory");
    expect_line("  LDI R1, -1\n",                     l, 1, "minus only for ADDI");
    expect_line("  LD R1, [R0]\n",                    l, 1, "+imm is required");
    expect_line("  .word 1\n  .org 0x31\n  .word 2\n", l, 3, ".word at odd address");
    expect_line("  .org 256\n",                       l, 1, ".org 256");
    expect_line("abcdefghijklmnop: HLT\n",            l, 1, "label longer than 15");
    expect_line("  ldi R1, 1\n",                      l, 1, "mnemonics are uppercase");
}

/* A layout whose code range ends at 0x40 accepts a jump to 0x30 and an instruction at 0x30. */
static void test_wide_layout(void)
{
    Layout wide;
    wide.code_start = CPU_LAYOUT.code_start;    /* copy field by field (no struct copy) */
    wide.code_end = 0x40;
    wide.data_start = 0x40;
    wide.data_end = 0x40;
    wide.heap_start = CPU_LAYOUT.heap_start;
    wide.heap_end = CPU_LAYOUT.heap_end;
    wide.heap_ptr_addr = CPU_LAYOUT.heap_ptr_addr;
    wide.stack_limit = 0x40;
    wide.stack_top = CPU_LAYOUT.stack_top;
    wide.entry = CPU_LAYOUT.entry;
    expect_line("  JMP 0x30\n",           &wide, 0, "wide: jump to 0x30 accepted");
    expect_line("  .org 0x30\n  HLT\n",   &wide, 0, "wide: instruction at 0x30 accepted");
}

/* CALL and RET encodings. */
static void test_call_ret(void)
{
    Memory m;
    expect(assemble("  CALL 0x0C\n  RET\n", &m, &CPU_LAYOUT) == 0, "CALL/RET assemble");
    expect(word_at(&m, 0) == 0x800C, "CALL 0x0C is 800C");
    expect(word_at(&m, 2) == 0x9000, "RET is 9000");
}

/* Runs all tests and returns 1 if any failed. */
int main(void)
{
    test_pow();
    test_fact();
    test_errors();
    test_syntax();
    test_wide_layout();
    test_call_ret();
    printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
