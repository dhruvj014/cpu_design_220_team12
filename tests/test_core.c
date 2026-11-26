/* test_core.c - Unit tests for util, memory (and later alu, cpu). Owner: Dhruv. */
#include <stdio.h>
#include "util.h"
#include "memory.h"
#include "alu.h"

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

/* Checks that parse_number accepts text and gives the expected value. */
static void expect_num(const char *text, long want)
{
    long got = 12345;
    int rc = parse_number(text, &got);
    expect(rc == 0 && got == want, text);
}

/* Checks that parse_number rejects text. */
static void expect_bad(const char *text)
{
    long got = 12345;
    expect(parse_number(text, &got) == -1, text);
}

/* Tests the six helpers in util.c. */
static void test_util(void)
{
    expect(str_eq("pow", "pow"), "str_eq same");
    expect(!str_eq("pow", "fact"), "str_eq different");
    expect(!str_eq("po", "pow"), "str_eq prefix");
    expect(str_eq("", ""), "str_eq empty");
    expect(str_len("") == 0, "str_len empty");
    expect(str_len("LDI R1") == 6, "str_len 6");
    expect(is_space(' ') && is_space('\t') && is_space('\r'), "is_space blanks");
    expect(!is_space('\n') && !is_space('a'), "is_space not newline");
    expect(is_digit('0') && is_digit('9') && !is_digit('a'), "is_digit");
    expect(is_alpha('a') && is_alpha('Z') && is_alpha('_') && !is_alpha('1'), "is_alpha");

    expect_num("0", 0);
    expect_num("511", 511);
    expect_num("0x3E", 62);
    expect_num("0x3e", 62);
    expect_num("-32", -32);
    expect_num("65535", 65535);
    expect_num("0xFFFF", 65535);
    expect_num("-0x10", -16);
    expect_bad("");
    expect_bad("-");
    expect_bad("0x");
    expect_bad("12a");
    expect_bad("70000");
    expect_bad("0xG1");
    expect_bad("65536");
    expect_bad("1 2");
}

/* Tests clear, big-endian read/write and the address checks in memory.c. */
static void test_memory(void)
{
    Memory m;
    u16 v = 0;
    int i, all_zero = 1;

    mem_clear(&m);
    for (i = 0; i < MEM_SIZE; i++)
        if (m.byte[i] != 0)
            all_zero = 0;
    expect(all_zero, "mem_clear zeroes all 256 bytes");

    expect(mem_write16(&m, 0x30, 0x1234) == 0, "write 0x30 ok");
    expect(m.byte[0x30] == 0x12 && m.byte[0x31] == 0x34, "big-endian: high byte first");
    expect(mem_read16(&m, 0x30, &v) == 0 && v == 0x1234, "read back 0x1234");

    expect(mem_write16(&m, 0xFE, 0xABCD) == 0, "write 0xFE ok");
    v = 0;
    expect(mem_read16(&m, 0xFE, &v) == 0 && v == 0xABCD, "read 0xFE ok");

    v = 0x5555;
    expect(mem_read16(&m, 0x31, &v) == -1, "read odd address fails");
    expect(mem_read16(&m, 0xFF, &v) == -1, "read 0xFF fails");
    expect(mem_read16(&m, 0x100, &v) == -1, "read 0x100 fails");
    expect(v == 0x5555, "failed read leaves output alone");

    expect(mem_write16(&m, 0x31, 0xFFFF) == -1, "write odd address fails");
    expect(mem_write16(&m, 0x100, 0xFFFF) == -1, "write 0x100 fails");
    expect(m.byte[0x31] == 0x34 && m.byte[0x00] == 0, "failed write changes nothing");
}

/* Checks one ALU result: value, Z and C. */
static void expect_alu(const char *name, Opcode op, u16 a, u16 b, u16 value, int z, int c)
{
    AluOut r = alu_exec(op, a, b);
    expect(r.value == value && r.z == z && r.c == c, name);
}

/* Tests every ALU operation, including flag edge cases. */
static void test_alu(void)
{
    expect_alu("ADD 0xFFFF,1",      OP_ADD, 0xFFFF, 1,      0x0000, 1, 1);
    expect_alu("ADD 2,3",           OP_ADD, 2, 3,           0x0005, 0, 0);
    expect_alu("ADD 0x8000,0x8000", OP_ADD, 0x8000, 0x8000, 0x0000, 1, 1);
    expect_alu("ADD 0,0",           OP_ADD, 0, 0,           0x0000, 1, 0);
    expect_alu("SUB 1,2",           OP_SUB, 1, 2,           0xFFFF, 0, 1);
    expect_alu("SUB 5,5",           OP_SUB, 5, 5,           0x0000, 1, 0);
    expect_alu("SUB 0,1",           OP_SUB, 0, 1,           0xFFFF, 0, 1);
    expect_alu("SUB 9,4",           OP_SUB, 9, 4,           0x0005, 0, 0);
    expect_alu("CMP 5,5",           OP_CMP, 5, 5,           0x0000, 1, 0);
    expect_alu("CMP 2,5",           OP_CMP, 2, 5,           0xFFFD, 0, 1);
    expect_alu("CMP 5,2",           OP_CMP, 5, 2,           0x0003, 0, 0);
    expect_alu("MUL 5040,8",        OP_MUL, 5040, 8,        40320,  0, 0);
    expect_alu("MUL 0xFFFF,0xFFFF", OP_MUL, 0xFFFF, 0xFFFF, 0x0001, 0, 1);
    expect_alu("MUL 0,7",           OP_MUL, 0, 7,           0x0000, 1, 0);
    expect_alu("MUL 256,256",       OP_MUL, 256, 256,       0x0000, 1, 1);
    expect_alu("AND F0F0,0F0F",     OP_AND, 0xF0F0, 0x0F0F, 0x0000, 1, 0);
    expect_alu("AND 00FF,0F0F",     OP_AND, 0x00FF, 0x0F0F, 0x000F, 0, 0);
    expect_alu("ADDI 8,-1",         OP_ADDI, 8, 0xFFFF,     0x0007, 0, 1);
    expect_alu("ADDI 0,-32",        OP_ADDI, 0, 0xFFE0,     0xFFE0, 0, 0);
    expect_alu("ADDI 5,31",         OP_ADDI, 5, 31,         0x0024, 0, 0);
    expect_alu("ADDI 1,-1",         OP_ADDI, 1, 0xFFFF,     0x0000, 1, 1);
}

/* Runs all tests and returns 1 if any failed. */
int main(void)
{
    test_util();
    test_memory();
    test_alu();
    printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
