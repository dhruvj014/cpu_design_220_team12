/* alu.c - ALU: computes a result and the Z and C flags. No printing. Owner: Dhruv */
#include "alu.h"

/* Does one ALU operation on 16-bit a and b. ADDI expects b already sign-extended. */
AluOut alu_exec(Opcode op, u16 a, u16 b)
{
    AluOut out;
    unsigned long wide = 0;   /* full-width result, so carry out of bit 15 is visible */
    int carry = 0;

    switch (op) {
    case OP_ADD:
    case OP_ADDI:
        wide = (unsigned long)a + (unsigned long)b;
        carry = wide > 0xFFFFUL;                  /* carry out of bit 15 */
        break;
    case OP_SUB:
    case OP_CMP:
        wide = (unsigned long)a - (unsigned long)b;
        carry = a < b;                            /* borrow */
        break;
    case OP_MUL:
        wide = (unsigned long)a * (unsigned long)b;
        carry = wide > 0xFFFFUL;                  /* product does not fit in 16 bits */
        break;
    case OP_AND:
        wide = (unsigned long)(a & b);
        carry = 0;
        break;
    default:                                      /* not an ALU opcode: result 0, no carry */
        wide = 0;
        carry = 0;
        break;
    }
    out.value = (u16)(wide & 0xFFFFUL);           /* keep the low 16 bits */
    out.z = (out.value == 0);
    out.c = (u8)carry;
    return out;
}
