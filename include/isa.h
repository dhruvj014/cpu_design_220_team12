/* isa.h - Basic types, sizes and the 16 opcodes of SJ16. Owner: Dhruv */
#ifndef ISA_H
#define ISA_H
typedef unsigned char  u8;
typedef unsigned short u16;
#define MEM_SIZE   256
#define NUM_REGS   8
#define REG_FP     6
#define REG_SP     7
#define MAX_STEPS  1000
typedef enum { OP_HLT,  OP_LDI, OP_LD,   OP_ST,  OP_ADD, OP_SUB, OP_MUL, OP_AND,
               OP_CALL, OP_RET, OP_ADDI, OP_CMP, OP_JMP, OP_JZ,  OP_JNZ, OP_JC } Opcode;
#endif
