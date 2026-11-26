/* disasm.h - Interface of the SJ16 disassembler (implemented by Tanisha). Owner: Dhruv */
#ifndef DISASM_H
#define DISASM_H
#include "isa.h"
void disasm_print(u16 ir);   /* one instruction, no newline */
#endif
