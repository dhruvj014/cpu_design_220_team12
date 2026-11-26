/* alu.h - ALU: arithmetic result plus Z and C flags. Owner: Dhruv */
#ifndef ALU_H
#define ALU_H
#include "isa.h"
typedef struct { u16 value; u8 z, c; } AluOut;
/* op is one of ADD SUB MUL AND ADDI CMP. For ADDI, b is already sign-extended. */
AluOut alu_exec(Opcode op, u16 a, u16 b);
#endif
