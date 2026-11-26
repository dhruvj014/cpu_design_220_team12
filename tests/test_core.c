/* test_core.c - Unit tests for util, memory (and later alu, cpu). Owner: Dhruv. */
#include <stdio.h>
#include "util.h"
#include "memory.h"
#include "alu.h"
#include "cpu.h"
#include "cpu_layout.h"

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

/* ---------- CPU test helpers ---------- */

/* Encoders: build one 16-bit instruction from its fields (so tests are readable). */
static u16 enc_r(unsigned op, unsigned rd, unsigned a, unsigned b) { return (u16)((op << 12) | (rd << 9) | (a << 6) | (b << 3)); }
static u16 enc_i(unsigned op, unsigned rd, unsigned a, unsigned imm6) { return (u16)((op << 12) | (rd << 9) | (a << 6) | (imm6 & 0x3F)); }
static u16 enc_l(unsigned op, unsigned rd, unsigned imm9) { return (u16)((op << 12) | (rd << 9) | (imm9 & 0x1FF)); }
static u16 enc_j(unsigned op, unsigned addr) { return (u16)((op << 12) | (addr & 0x1FF)); }

/* Clears memory and stores n words starting at address 0. */
static void load_words(Memory *m, const u16 *w, int n)
{
    int i;
    mem_clear(m);
    for (i = 0; i < n; i++)
        mem_write16(m, (u16)(2 * i), w[i]);
}

/* Complete copy of CPU and memory, used to prove a faulting step changes nothing. */
typedef struct { Cpu cpu; u8 bytes[MEM_SIZE]; } Snapshot;

/* Saves cpu and memory into s. */
static void snap_take(Snapshot *s, const Cpu *c, const Memory *m)
{
    int i;
    s->cpu = *c;
    for (i = 0; i < MEM_SIZE; i++)
        s->bytes[i] = m->byte[i];
}

/* Returns 1 if cpu and memory still equal the snapshot (registers, PC, IR, flags, all bytes). */
static int snap_same(const Snapshot *s, const Cpu *c, const Memory *m)
{
    int i;
    for (i = 0; i < NUM_REGS; i++)
        if (s->cpu.r[i] != c->r[i])
            return 0;
    if (s->cpu.pc != c->pc || s->cpu.ir != c->ir || s->cpu.z != c->z || s->cpu.c != c->c)
        return 0;
    for (i = 0; i < MEM_SIZE; i++)
        if (s->bytes[i] != m->byte[i])
            return 0;
    return 1;
}

/* Runs until HLT, fault or limit steps. Returns the last status; *steps counts every step including HLT or the fault. */
static CpuStatus run(Cpu *c, Memory *m, StepInfo *info, int limit, int *steps)
{
    CpuStatus st = CPU_OK;
    *steps = 0;
    while (*steps < limit && st == CPU_OK) {
        st = cpu_step(c, m, info);
        (*steps)++;
    }
    return st;
}

/* The pow program words from SPEC section 9. */
static const u16 POW_WORDS[11] = { 0x2230, 0x2432, 0x1601, 0x3634, 0xB080, 0xD014,
                                   0x66C8, 0x3634, 0xA4BF, 0xC008, 0x0000 };
/* The fact program words from SPEC section 9. */
static const u16 FACT_WORDS[17] = { 0x1202, 0x3234, 0x2234, 0x2430, 0xB088, 0xF01C, 0x2232, 0x2434,
                                    0x6250, 0x3232, 0x2234, 0xA241, 0x3234, 0xC004, 0x2232, 0x3236, 0x0000 };

/* Returns the word stored at addr (0 if the address is bad). */
static u16 word_at(const Memory *m, u16 addr)
{
    u16 v = 0;
    mem_read16(m, addr, &v);
    return v;
}

/* Runs pow with the given exponent and checks steps and result. */
static void check_pow(int exponent, int want_steps, u16 want_result)
{
    Memory m; Cpu c; StepInfo info; int steps;
    CpuStatus st;
    load_words(&m, POW_WORDS, 11);
    mem_write16(&m, 0x30, 8);
    mem_write16(&m, 0x32, (u16)exponent);
    expect(cpu_reset(&c, &CPU_LAYOUT) == 0, "pow: reset ok");
    st = run(&c, &m, &info, MAX_STEPS, &steps);
    expect(st == CPU_HALT, "pow: halts");
    expect(steps == want_steps, "pow: step count");
    expect(word_at(&m, 0x34) == want_result, "pow: result at 0x34");
    expect(c.r[REG_SP] == 0x0100, "pow: SP back to 0x0100");
}

/* Tests the two real programs from SPEC section 9. */
static void test_programs(void)
{
    Memory m; Cpu c; StepInfo info; int steps;

    check_pow(2, 19, 64);
    check_pow(0, 7, 1);
    check_pow(1, 13, 8);
    check_pow(3, 25, 512);
    check_pow(5, 37, 32768);

    load_words(&m, FACT_WORDS, 17);
    mem_write16(&m, 0x30, 8);
    mem_write16(&m, 0x32, 1);
    cpu_reset(&c, &CPU_LAYOUT);
    expect(run(&c, &m, &info, MAX_STEPS, &steps) == CPU_HALT, "fact: halts");
    expect(steps == 93, "fact: 93 steps");
    expect(word_at(&m, 0x30) == 8 && word_at(&m, 0x32) == 40320 &&
           word_at(&m, 0x34) == 9 && word_at(&m, 0x36) == 40320, "fact: final data");
}

/* Tests layout_valid and cpu_reset. */
static void test_layout_reset(void)
{
    Layout bad;
    Layout heap = { 0x00, 0x30, 0x30, 0x40, 0x40, 0x100, 0x30, 0x40, 0x100, 0x00 };
    Cpu c;
    int i, zero = 1;

    expect(layout_valid(&CPU_LAYOUT) == 1, "layout: CPU_LAYOUT valid");
    expect(layout_valid(&heap) == 1, "layout: heap layout valid");
    bad = CPU_LAYOUT; bad.code_end = 0x2F;
    expect(layout_valid(&bad) == 0, "layout: odd bound rejected");
    bad = CPU_LAYOUT; bad.stack_top = 0x102;
    expect(layout_valid(&bad) == 0, "layout: stack_top 0x102 rejected");
    bad = CPU_LAYOUT; bad.data_start = 0x20;
    expect(layout_valid(&bad) == 0, "layout: data before code_end rejected");
    bad = CPU_LAYOUT; bad.entry = 0x30;
    expect(layout_valid(&bad) == 0, "layout: entry outside code rejected");
    bad = CPU_LAYOUT; bad.heap_ptr_addr = 0x30;
    expect(layout_valid(&bad) == 0, "layout: heap pointer without heap rejected");
    bad = heap; bad.heap_ptr_addr = 0x40;
    expect(layout_valid(&bad) == 0, "layout: heap pointer outside data rejected");

    bad = CPU_LAYOUT; bad.entry = 0x30;
    expect(cpu_reset(&c, &bad) == -1, "reset: invalid layout gives -1");
    for (i = 0; i < NUM_REGS; i++) c.r[i] = 0x1111;
    c.pc = 0x22; c.ir = 0x1234; c.z = 1; c.c = 1;
    expect(cpu_reset(&c, &CPU_LAYOUT) == 0, "reset: ok");
    for (i = 0; i < 7; i++)
        if (c.r[i] != 0) zero = 0;
    expect(zero && c.r[7] == 0x0100, "reset: R0-R6 = 0, R7 = 0x0100");
    expect(c.pc == 0 && c.ir == 0 && c.z == 0 && c.c == 0, "reset: PC, IR, Z, C");
}

/* Tests each opcode through cpu_step, including flags and the StepInfo write record. */
static void test_opcodes(void)
{
    Memory m; Cpu c; StepInfo info; int steps;
    u16 w[8];

    /* LDI, LD, ST, JMP leave flags alone: CMP R0,R0 sets Z=1 first. */
    w[0] = enc_r(OP_CMP, 0, 0, 0);
    w[1] = enc_l(OP_LDI, 1, 300);
    w[2] = enc_i(OP_ST, 1, 0, 0x34);
    w[3] = enc_i(OP_LD, 2, 0, 0x34);
    w[4] = enc_j(OP_JMP, 0x0A);
    w[5] = 0;
    load_words(&m, w, 6);
    cpu_reset(&c, &CPU_LAYOUT);
    cpu_step(&c, &m, &info);
    cpu_step(&c, &m, &info);
    expect(c.r[1] == 300 && c.z == 1 && c.c == 0, "LDI: loads 300, flags unchanged");
    cpu_step(&c, &m, &info);
    expect(info.wrote == 1 && info.waddr == 0x34 && info.before == 0 && info.after == 300, "ST: info records the write");
    expect(word_at(&m, 0x34) == 300 && c.z == 1, "ST: memory written, flags unchanged");
    cpu_step(&c, &m, &info);
    expect(c.r[2] == 300 && info.wrote == 0 && c.z == 1, "LD: loads word, no write");
    cpu_step(&c, &m, &info);
    expect(c.pc == 0x0A && c.z == 1, "JMP: PC changes, flags unchanged");

    /* ALU instructions: MUL, ADD, SUB, AND, ADDI. */
    w[0] = enc_l(OP_LDI, 1, 6);
    w[1] = enc_l(OP_LDI, 2, 7);
    w[2] = enc_r(OP_MUL, 3, 1, 2);      /* R3 = 42 */
    w[3] = enc_r(OP_ADD, 4, 3, 1);      /* R4 = 48 */
    w[4] = enc_r(OP_SUB, 5, 4, 2);      /* R5 = 41 */
    w[5] = enc_r(OP_AND, 5, 5, 1);      /* R5 = 41 & 6 = 0, Z = 1 */
    w[6] = enc_i(OP_ADDI, 6, 0, 0x3B);  /* R6 = 0 + (-5) */
    w[7] = 0;
    load_words(&m, w, 8);
    cpu_reset(&c, &CPU_LAYOUT);
    run(&c, &m, &info, 6, &steps);
    expect(c.r[3] == 42 && c.r[4] == 48, "MUL and ADD results");
    expect(c.r[5] == 0 && c.z == 1, "SUB then AND gives 0 with Z=1");
    cpu_step(&c, &m, &info);
    expect(c.r[6] == 0xFFFB && c.z == 0, "ADDI sign-extends -5, Z=0");

    /* Writing R0 is ignored but flags still update. */
    w[0] = enc_l(OP_LDI, 0, 5);
    w[1] = enc_i(OP_ADDI, 0, 0, 0);     /* R0 = 0 + 0, sets Z */
    w[2] = 0;
    load_words(&m, w, 3);
    cpu_reset(&c, &CPU_LAYOUT);
    c.z = 0;
    cpu_step(&c, &m, &info);
    expect(c.r[0] == 0, "LDI R0,5 leaves R0 = 0");
    cpu_step(&c, &m, &info);
    expect(c.r[0] == 0 && c.z == 1, "ADDI to R0 ignored but sets flags");

    /* CALL and RET: 8004 0000 9000 */
    w[0] = enc_j(OP_CALL, 4);
    w[1] = 0;
    w[2] = (u16)(OP_RET << 12);
    load_words(&m, w, 3);
    cpu_reset(&c, &CPU_LAYOUT);
    expect(cpu_step(&c, &m, &info) == CPU_OK, "CALL ok");
    expect(info.wrote == 1 && info.waddr == 0xFE && info.after == 2 && c.r[7] == 0xFE && c.pc == 4, "CALL: pushes return address 2 at 0xFE");
    expect(cpu_step(&c, &m, &info) == CPU_OK && c.pc == 2 && c.r[7] == 0x100, "RET: PC = 2, SP = 0x100");
    expect(cpu_step(&c, &m, &info) == CPU_HALT, "HLT after RET");
    cpu_reset(&c, &CPU_LAYOUT);
    load_words(&m, w, 3);
    expect(run(&c, &m, &info, MAX_STEPS, &steps) == CPU_HALT && steps == 3 && c.r[7] == 0x100, "CALL, RET, HLT: 3 steps, SP = 0x100");
}

/* Runs: LDI R1,1 / CMP ra,rb / jump to 0x0A / LDI R2,1 / HLT; target does LDI R2,2 / HLT. Returns R2. */
static int jump_result(unsigned jop, unsigned ra, unsigned rb)
{
    Memory m; Cpu c; StepInfo info; int steps;
    u16 w[7];
    w[0] = enc_l(OP_LDI, 1, 1);
    w[1] = enc_r(OP_CMP, 0, ra, rb);
    w[2] = enc_j(jop, 0x0A);
    w[3] = enc_l(OP_LDI, 2, 1);
    w[4] = 0;
    w[5] = enc_l(OP_LDI, 2, 2);
    w[6] = 0;
    load_words(&m, w, 7);
    cpu_reset(&c, &CPU_LAYOUT);
    run(&c, &m, &info, 20, &steps);
    return c.r[2];
}

/* Tests taken and not-taken for every conditional jump. */
static void test_jumps(void)
{
    expect(jump_result(OP_JZ, 1, 1) == 2, "JZ taken when Z=1");
    expect(jump_result(OP_JZ, 0, 1) == 1, "JZ not taken when Z=0");
    expect(jump_result(OP_JNZ, 0, 1) == 2, "JNZ taken when Z=0");
    expect(jump_result(OP_JNZ, 1, 1) == 1, "JNZ not taken when Z=1");
    expect(jump_result(OP_JC, 0, 1) == 2, "JC taken when C=1 (0 < 1)");
    expect(jump_result(OP_JC, 1, 0) == 1, "JC not taken when C=0 (1 >= 0)");
    expect(jump_result(OP_JMP, 1, 1) == 2, "JMP always taken");
}

/* Runs words under layout l until a fault or limit steps. Checks the fault text, the step it happened on,
   and that the faulting step changed no register, flag or memory byte. */
static void expect_fault_in(const Layout *l, const u16 *w, int n, int limit, const char *want, int want_step, const char *name)
{
    Memory m; Cpu c; StepInfo info; Snapshot before;
    int step = 0;
    CpuStatus st = CPU_OK;

    load_words(&m, w, n);
    cpu_reset(&c, l);
    snap_take(&before, &c, &m);
    while (st == CPU_OK && step < limit) {
        snap_take(&before, &c, &m);
        st = cpu_step(&c, &m, &info);
        step++;
    }
    expect(st == CPU_FAULT && info.fault != 0 && str_eq(info.fault, want), name);
    expect(step == want_step, name);
    expect(st == CPU_FAULT && snap_same(&before, &c, &m), name);
}

/* Same, under CPU_LAYOUT. */
static void expect_fault(const u16 *w, int n, const char *want, int want_step, const char *name)
{
    expect_fault_in(&CPU_LAYOUT, w, n, MAX_STEPS, want, want_step, name);
}

/* Tests every fault that CPU_LAYOUT can produce. */
static void test_faults(void)
{
    static const u16 ld_odd[]   = { 0x2231 };
    static const u16 ld_far[]   = { 0x13FF, 0x2441 };
    static const u16 st_odd[]   = { 0x3231 };
    static const u16 st_code[]  = { 0x3200 };
    static const u16 st_below[] = { 0x1240, 0x3040 };
    static const u16 res1[] = { 0x0001 }, res2[] = { 0x4001 }, res3[] = { 0x9001 };
    static const u16 res4[] = { 0xC200 }, res5[] = { 0x8200 };
    static const u16 j_odd[]  = { 0xC001 };
    static const u16 j_out[]  = { 0xC030 };
    static const u16 c_odd[]  = { 0x8001 };
    static const u16 c_self[] = { 0x8000 };
    static const u16 sp_low[] = { 0x1E3E };
    static const u16 sp_odd[] = { 0x1EFF };
    static const u16 ret_empty[] = { 0x9000 };
    static const u16 sp_high[] = { 0xAFC2 };
    static const u16 ret_odd[] = { 0xAFFE, 0x1201, 0x33C0, 0x9000 };
    u16 nops[24];
    int i;

    expect_fault(ld_odd, 1, FAULT_LOAD, 1, "fault: LD odd address");
    expect_fault(ld_far, 2, FAULT_LOAD, 2, "fault: LD beyond memory");
    expect_fault(st_odd, 1, FAULT_STORE_ADDR, 1, "fault: ST odd address");
    expect_fault(st_code, 1, FAULT_STORE_REGION, 1, "fault: ST into code");
    expect_fault(st_below, 2, FAULT_STORE_REGION, 2, "fault: ST below SP outside data");
    expect_fault(res1, 1, FAULT_RESERVED, 1, "fault: reserved bits HLT 0001");
    expect_fault(res2, 1, FAULT_RESERVED, 1, "fault: reserved bits ADD 4001");
    expect_fault(res3, 1, FAULT_RESERVED, 1, "fault: reserved bits RET 9001");
    expect_fault(res4, 1, FAULT_RESERVED, 1, "fault: reserved bits JMP C200");
    expect_fault(res5, 1, FAULT_RESERVED, 1, "fault: reserved bits CALL 8200");
    expect_fault(j_odd, 1, FAULT_TARGET, 1, "fault: jump to odd address");
    expect_fault(j_out, 1, FAULT_TARGET, 1, "fault: jump outside code");
    expect_fault(c_odd, 1, FAULT_TARGET, 1, "fault: call to odd address");
    for (i = 0; i < 24; i++)
        nops[i] = 0x4000;                                     /* ADD R0,R0,R0 */
    expect_fault(nops, 24, FAULT_PC, 25, "fault: PC leaves code on step 25");

    /* Stack faults */
    expect_fault(c_self, 1, FAULT_OVERFLOW, 97, "fault: CALL to itself overflows on step 97");
    expect_fault(sp_low, 1, FAULT_OVERFLOW, 1, "fault: LDI R7,0x3E overflow");
    expect_fault(sp_odd, 1, FAULT_SP_ODD, 1, "fault: LDI R7,255 odd SP");
    expect_fault(ret_empty, 1, FAULT_UNDERFLOW, 1, "fault: RET on empty stack");
    expect_fault(sp_high, 1, FAULT_UNDERFLOW, 1, "fault: SP above stack_top");
    expect_fault(ret_odd, 4, FAULT_RETURN, 4, "fault: RET to odd address");
}

/* Runs words under the heap layout with the heap top word set to top. Returns the status; fills steps and info. */
static CpuStatus run_heap_case(const u16 *w, int n, u16 top, Cpu *c, Memory *m, StepInfo *info, int *steps)
{
    static const Layout hl = { 0x00, 0x30, 0x30, 0x40, 0x40, 0x100, 0x30, 0x40, 0x100, 0x00 };
    load_words(m, w, n);
    mem_write16(m, 0x30, top);
    cpu_reset(c, &hl);
    return run(c, m, info, MAX_STEPS, steps);
}

/* Tests the heap rules with a layout that has a heap and a heap pointer at 0x30. */
static void test_heap(void)
{
    Memory m; Cpu c; StepInfo info; int steps, i;
    u16 pushes[9];
    static const u16 over[]    = { 0xAFFE, 0x1300, 0x3230 };
    static const u16 range[]   = { 0x123E, 0x3230 };
    static const u16 unalloc[] = { 0x1280, 0x3040 };
    static const u16 alloc[]   = { 0x124A, 0x3230, 0x1440, 0x3288, 0x0000 };
    static const u16 live[]    = { 0xAFFE, 0x31C0, 0x0000 };

    for (i = 0; i < 9; i++)
        pushes[i] = 0xAFFE;                                   /* ADDI R7,R7,-2 */
    expect(run_heap_case(pushes, 9, 0x00F0, &c, &m, &info, &steps) == CPU_FAULT && steps == 9 &&
           c.r[7] == 0xF0 && str_eq(info.fault, FAULT_STACK_HEAP), "heap: stack ran into heap on step 9, SP = 0xF0");
    expect(run_heap_case(over, 3, 0x0040, &c, &m, &info, &steps) == CPU_FAULT && steps == 3 &&
           str_eq(info.fault, FAULT_HEAP_STACK), "heap: heap ran into stack on step 3");
    expect(run_heap_case(range, 2, 0x0040, &c, &m, &info, &steps) == CPU_FAULT && steps == 2 &&
           str_eq(info.fault, FAULT_HEAP_RANGE), "heap: bad heap top on step 2");
    expect(run_heap_case(unalloc, 2, 0x0040, &c, &m, &info, &steps) == CPU_FAULT && steps == 2 &&
           str_eq(info.fault, FAULT_STORE_REGION), "heap: store into unallocated heap faults");
    expect(run_heap_case(alloc, 5, 0x0040, &c, &m, &info, &steps) == CPU_HALT && steps == 5 &&
           cpu_heap_top(&c, &m) == 0x4A && word_at(&m, 0x48) == 0x4A, "heap: allocate then store halts after 5 steps");

    /* Store into the live stack is allowed (CPU_LAYOUT has no heap). */
    load_words(&m, live, 3);
    cpu_reset(&c, &CPU_LAYOUT);
    expect(run(&c, &m, &info, MAX_STEPS, &steps) == CPU_HALT && steps == 3, "stack: store into live stack halts after 3 steps");
}

/* C030 only faults when code really ends at 0x30. */
static void test_wide_code(void)
{
    Layout wide = CPU_LAYOUT;
    Memory m; Cpu c; StepInfo info; int steps;
    static const u16 w[] = { 0xC030 };

    wide.code_end = 0x40;
    wide.data_start = 0x40;
    wide.data_end = 0x40;
    wide.stack_limit = 0x40;
    expect(layout_valid(&wide) == 1, "wide layout valid");
    load_words(&m, w, 1);
    cpu_reset(&c, &wide);
    expect(run(&c, &m, &info, MAX_STEPS, &steps) == CPU_HALT && steps == 2, "JMP 0x30 works when code_end is 0x40");
}

/* Runs all tests and returns 1 if any failed. */
int main(void)
{
    test_util();
    test_memory();
    test_alu();
    test_layout_reset();
    test_programs();
    test_opcodes();
    test_jumps();
    test_faults();
    test_heap();
    test_wide_code();
    printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
