/* cpu.c - SJ16 CPU: layout check, reset and one-instruction step. Never prints. Owner: Dhruv

   cpu_step works in two stages so that a faulting instruction changes nothing:
   1. execute() decides what the instruction WOULD change and records it in a Pending struct.
   2. Only if there was no fault, cpu_step commits the Pending changes to registers, flags, memory and PC. */
#include "cpu.h"
#include "alu.h"

/* Changes an instruction wants to make, applied only if it does not fault. */
typedef struct {
    int reg;                 /* register to write, or -1 for none */
    u16 reg_value;
    int set_flags;           /* 1 = update Z and C */
    u8  z, c;
    int mem_write;           /* 1 = write a word to memory */
    u16 mem_addr, mem_value;
    u16 next_pc;
    int halt;                /* 1 = HLT */
} Pending;

/* ---------- layout ---------- */

/* Returns 1 if the layout follows the six rules in SPEC section 3, else 0. */
int layout_valid(const Layout *l)
{
    u16 v[11];
    int i;
    int has_heap = (l->heap_start != l->heap_end);

    v[0] = l->code_start;  v[1] = l->code_end;   v[2] = l->data_start; v[3] = l->data_end;
    v[4] = l->heap_start;  v[5] = l->heap_end;   v[6] = l->stack_limit; v[7] = l->stack_top;
    v[8] = l->entry;
    for (i = 0; i < 9; i++)                                   /* rule 1: even and at most 256 */
        if ((v[i] & 1) != 0 || v[i] > 256)
            return 0;
    if (!(l->code_start < l->code_end && l->code_end <= l->data_start &&
          l->data_start <= l->data_end && l->data_end <= l->stack_limit &&
          l->stack_limit <= l->stack_top))                    /* rule 2: ordering */
        return 0;
    if (l->entry < l->code_start || l->entry >= l->code_end)  /* rule 3: entry inside code */
        return 0;
    if (!has_heap) {                                          /* rule 4: no heap */
        if (l->heap_start != l->heap_end || l->heap_ptr_addr != NO_ADDR)
            return 0;
    } else {                                                  /* rule 5: heap */
        if (!(l->data_end <= l->heap_start && l->heap_start < l->heap_end &&
              l->heap_end <= l->stack_top && l->stack_limit >= l->heap_start))
            return 0;
    }
    if (l->heap_ptr_addr != NO_ADDR) {                        /* rule 6: heap pointer word */
        if ((l->heap_ptr_addr & 1) != 0 ||
            l->heap_ptr_addr < l->data_start || l->heap_ptr_addr + 2 > l->data_end)
            return 0;
    }
    return 1;
}

/* Sets the CPU to its start state for layout l. 0 ok, -1 if the layout is invalid. */
int cpu_reset(Cpu *c, const Layout *l)
{
    int i;
    if (!layout_valid(l))
        return -1;
    for (i = 0; i < NUM_REGS; i++)
        c->r[i] = 0;
    c->r[REG_SP] = l->stack_top;
    c->pc = l->entry;
    c->ir = 0;
    c->z = 0;
    c->c = 0;
    c->layout = l;
    return 0;
}

/* The heap top word if heap_ptr_addr is set, else heap_end. */
u16 cpu_heap_top(const Cpu *c, const Memory *m)
{
    u16 top;
    if (c->layout->heap_ptr_addr != NO_ADDR && mem_read16(m, c->layout->heap_ptr_addr, &top) == 0)
        return top;
    return c->layout->heap_end;
}

/* ---------- checks ---------- */

/* Returns 1 if the 2-byte word at addr lies completely inside [lo, hi). */
static int word_inside(unsigned long addr, unsigned long lo, unsigned long hi)
{
    return addr >= lo && addr + 2 <= hi;
}

/* Checks a jump, call or return target. Returns 0 if ok, else the given fault text. */
static const char *check_target(const Layout *l, unsigned long target, const char *fault)
{
    if ((target & 1) != 0 || target < l->code_start || target + 2 > l->code_end)
        return fault;
    return 0;
}

/* Checks a new stack pointer value, in the order of SPEC section 4. Returns 0 if ok. */
static const char *check_sp(const Cpu *c, const Memory *m, unsigned long sp)
{
    const Layout *l = c->layout;
    if ((sp & 1) != 0)
        return FAULT_SP_ODD;
    if (sp > l->stack_top)
        return FAULT_UNDERFLOW;
    if (l->heap_start != l->heap_end && sp < cpu_heap_top(c, m))
        return FAULT_STACK_HEAP;
    if (sp < l->stack_limit)
        return FAULT_OVERFLOW;
    return 0;
}

/* Checks whether ST may write value to addr. Returns 0 if allowed, else a fault text. */
static const char *check_store(const Cpu *c, const Memory *m, unsigned long addr, u16 value)
{
    const Layout *l = c->layout;
    int has_heap = (l->heap_start != l->heap_end);
    if ((addr & 1) != 0 || addr > 0xFE)
        return FAULT_STORE_ADDR;
    if (!(word_inside(addr, l->data_start, l->data_end) ||
          (has_heap && word_inside(addr, l->heap_start, cpu_heap_top(c, m))) ||
          word_inside(addr, c->r[REG_SP], l->stack_top)))
        return FAULT_STORE_REGION;
    if (addr == l->heap_ptr_addr) {                           /* storing a new heap top */
        if ((value & 1) != 0 || value < l->heap_start || value > l->heap_end)
            return FAULT_HEAP_RANGE;
        if (value > c->r[REG_SP])
            return FAULT_HEAP_STACK;
    }
    return 0;
}

/* Returns the bits that must be 0 for this opcode (SPEC section 4, "Reserved bits"). */
static u16 reserved_mask(unsigned op)
{
    switch (op) {
    case OP_ADD: case OP_SUB: case OP_MUL: case OP_AND:
        return 0x0007;                                        /* bits 2-0 */
    case OP_CMP:
        return 0x0E07;                                        /* bits 11-9 and 2-0 */
    case OP_CALL: case OP_JMP: case OP_JZ: case OP_JNZ: case OP_JC:
        return 0x0E00;                                        /* bits 11-9 */
    case OP_HLT: case OP_RET:
        return 0x0FFF;                                        /* bits 11-0 */
    default:
        return 0x0000;                                        /* LDI, LD, ST, ADDI use every bit */
    }
}

/* Records a register write in p. R0 ignores writes; a write to R7 is stack-checked. Returns a fault or 0. */
static const char *set_reg(const Cpu *c, const Memory *m, Pending *p, unsigned rd, u16 value)
{
    if (rd == REG_SP) {
        const char *f = check_sp(c, m, value);
        if (f != 0)
            return f;
    }
    if (rd != 0) {
        p->reg = (int)rd;
        p->reg_value = value;
    }
    return 0;
}

/* Records ALU result and flags in p (flags are set even when rd is R0). */
static const char *set_alu(const Cpu *c, const Memory *m, Pending *p, unsigned rd, AluOut r)
{
    p->set_flags = 1;
    p->z = r.z;
    p->c = r.c;
    return set_reg(c, m, p, rd, r.value);
}

/* ---------- execute ---------- */

/* Decodes ir and fills p with its effects. Returns 0, or the fault text. Changes nothing itself. */
static const char *execute(const Cpu *c, const Memory *m, u16 ir, Pending *p)
{
    const Layout *l = c->layout;
    unsigned op  = (unsigned)(ir >> 12);
    unsigned rd  = (unsigned)((ir >> 9) & 7);
    unsigned rs1 = (unsigned)((ir >> 6) & 7);
    unsigned rs2 = (unsigned)((ir >> 3) & 7);
    unsigned imm6 = (unsigned)(ir & 0x3F);
    unsigned imm9 = (unsigned)(ir & 0x1FF);
    u16 a = c->r[rs1];
    u16 b = c->r[rs2];
    unsigned long addr;
    u16 word;
    const char *f;

    if ((ir & reserved_mask(op)) != 0)
        return FAULT_RESERVED;

    switch (op) {
    case OP_HLT:
        p->halt = 1;
        return 0;
    case OP_LDI:
        return set_reg(c, m, p, rd, (u16)imm9);
    case OP_LD:
        addr = (unsigned long)a + imm6;
        if ((addr & 1) != 0 || addr > 0xFE || mem_read16(m, (u16)addr, &word) != 0)
            return FAULT_LOAD;
        return set_reg(c, m, p, rd, word);
    case OP_ST:
        addr = (unsigned long)a + imm6;
        f = check_store(c, m, addr, c->r[rd]);
        if (f != 0)
            return f;
        p->mem_write = 1;
        p->mem_addr = (u16)addr;
        p->mem_value = c->r[rd];
        return 0;
    case OP_ADD: case OP_SUB: case OP_MUL: case OP_AND:
        return set_alu(c, m, p, rd, alu_exec((Opcode)op, a, b));
    case OP_ADDI:
        word = (u16)imm6;
        if ((imm6 & 0x20) != 0)
            word = (u16)(word | 0xFFC0);                      /* sign-extend the 6-bit value */
        return set_alu(c, m, p, rd, alu_exec(OP_ADDI, a, word));
    case OP_CMP: {
        AluOut r = alu_exec(OP_CMP, a, b);                    /* result discarded, flags kept */
        p->set_flags = 1;
        p->z = r.z;
        p->c = r.c;
        return 0;
    }
    case OP_CALL:
        f = check_target(l, imm9, FAULT_TARGET);
        if (f == 0)
            f = check_sp(c, m, (unsigned long)c->r[REG_SP] - 2);
        if (f != 0)
            return f;
        p->reg = REG_SP;
        p->reg_value = (u16)(c->r[REG_SP] - 2);
        p->mem_write = 1;
        p->mem_addr = p->reg_value;
        p->mem_value = (u16)(c->pc + 2);                      /* return address */
        p->next_pc = (u16)imm9;
        return 0;
    case OP_RET:
        if ((unsigned long)c->r[REG_SP] + 2 > l->stack_top)
            return FAULT_UNDERFLOW;
        if (mem_read16(m, c->r[REG_SP], &word) != 0)
            return FAULT_RETURN;
        f = check_target(l, word, FAULT_RETURN);
        if (f == 0)
            f = check_sp(c, m, (unsigned long)c->r[REG_SP] + 2);
        if (f != 0)
            return f;
        p->reg = REG_SP;
        p->reg_value = (u16)(c->r[REG_SP] + 2);
        p->next_pc = word;
        return 0;
    case OP_JMP: case OP_JZ: case OP_JNZ: case OP_JC:
        if ((op == OP_JMP) || (op == OP_JZ && c->z) || (op == OP_JNZ && !c->z) || (op == OP_JC && c->c)) {
            f = check_target(l, imm9, FAULT_TARGET);          /* checked only when taken */
            if (f != 0)
                return f;
            p->next_pc = (u16)imm9;
        }
        return 0;
    default:
        return FAULT_RESERVED;
    }
}

/* Runs one instruction. Returns CPU_OK, CPU_HALT or CPU_FAULT (state unchanged on a fault). */
CpuStatus cpu_step(Cpu *c, Memory *m, StepInfo *info)
{
    const Layout *l = c->layout;
    Pending p;
    u16 ir = 0;
    const char *fault;

    info->addr = c->pc;
    info->ir = 0;
    info->wrote = 0;
    info->waddr = info->before = info->after = 0;
    info->fault = 0;

    if ((c->pc & 1) != 0 || c->pc < l->code_start || (unsigned long)c->pc + 2 > l->code_end) {
        info->fault = FAULT_PC;
        return CPU_FAULT;
    }
    mem_read16(m, c->pc, &ir);                                /* cannot fail: pc is even and inside code */
    info->ir = ir;

    p.reg = -1;  p.reg_value = 0;  p.set_flags = 0;  p.z = 0;  p.c = 0;
    p.mem_write = 0;  p.mem_addr = 0;  p.mem_value = 0;
    p.next_pc = (u16)(c->pc + 2);
    p.halt = 0;

    fault = execute(c, m, ir, &p);
    if (fault != 0) {
        info->fault = fault;
        return CPU_FAULT;
    }

    if (p.mem_write) {                                        /* commit: memory, then registers, flags, PC */
        u16 before = 0;
        mem_read16(m, p.mem_addr, &before);
        mem_write16(m, p.mem_addr, p.mem_value);
        info->wrote = 1;
        info->waddr = p.mem_addr;
        info->before = before;
        info->after = p.mem_value;
    }
    if (p.reg >= 0)
        c->r[p.reg] = p.reg_value;
    if (p.set_flags) {
        c->z = p.z;
        c->c = p.c;
    }
    c->ir = ir;
    if (p.halt)
        return CPU_HALT;                                      /* PC stays on the HLT */
    c->pc = p.next_pc;
    return CPU_OK;
}
