/* disasm.c - Prints one SJ16 instruction word as assembly text (forms: SPEC section 10). Owner: Tanisha */
#include <stdio.h>
#include "disasm.h"

/* Mnemonics indexed by opcode, in the same order as Opcode in isa.h. */
static const char *const MNEMONIC[16] = {
    "HLT",  "LDI", "LD",   "ST",  "ADD", "SUB", "MUL", "AND",
    "CALL", "RET", "ADDI", "CMP", "JMP", "JZ",  "JNZ", "JC"
};

/* Bits that must be 0 for each opcode (SPEC section 4 format table). */
static const u16 RESERVED[16] = {
    0x0FFF,                         /* HLT: bits 11-0 */
    0, 0, 0,                        /* LDI LD ST: none */
    0x0007, 0x0007, 0x0007, 0x0007, /* ADD SUB MUL AND: bits 2-0 */
    0x0E00,                         /* CALL: bits 11-9 */
    0x0FFF,                         /* RET: bits 11-0 */
    0,                              /* ADDI: none */
    0x0E07,                         /* CMP: bits 11-9 and 2-0 */
    0x0E00, 0x0E00, 0x0E00, 0x0E00  /* JMP JZ JNZ JC: bits 11-9 */
};

/* Prints one instruction with no newline, or ??? if a reserved bit is set. */
void disasm_print(u16 ir)
{
    int op   = (ir >> 12) & 0xF;   /* bits 15-12 */
    int rd   = (ir >> 9) & 0x7;    /* bits 11-9 */
    int rs1  = (ir >> 6) & 0x7;    /* bits 8-6 */
    int rs2  = (ir >> 3) & 0x7;    /* bits 5-3 */
    int imm6 = ir & 0x3F;          /* bits 5-0 */
    int imm9 = ir & 0x1FF;         /* bits 8-0 */

    if ((ir & RESERVED[op]) != 0) {
        printf("???");
        return;
    }
    switch (op) {
    case OP_HLT:
    case OP_RET:
        printf("%s", MNEMONIC[op]);
        break;
    case OP_LDI:
        printf("LDI R%d, %d", rd, imm9);
        break;
    case OP_LD:
    case OP_ST:
        printf("%s R%d, [R%d+0x%02X]", MNEMONIC[op], rd, rs1, (unsigned)imm6);
        break;
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_AND:
        printf("%s R%d, R%d, R%d", MNEMONIC[op], rd, rs1, rs2);
        break;
    case OP_ADDI:                  /* imm6 is signed: 32 to 63 stand for -32 to -1 */
        printf("ADDI R%d, R%d, %d", rd, rs1, imm6 >= 32 ? imm6 - 64 : imm6);
        break;
    case OP_CMP:
        printf("CMP R%d, R%d", rs1, rs2);
        break;
    default:                       /* CALL JMP JZ JNZ JC */
        printf("%s 0x%02X", MNEMONIC[op], (unsigned)imm9);
        break;
    }
}
