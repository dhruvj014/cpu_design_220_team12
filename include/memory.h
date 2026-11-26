/* memory.h - 256-byte big-endian memory with 16-bit access. Owner: Dhruv */
#ifndef MEMORY_H
#define MEMORY_H
#include "isa.h"
typedef struct { u8 byte[MEM_SIZE]; } Memory;
void mem_clear(Memory *m);
int  mem_read16(const Memory *m, u16 addr, u16 *out);   /* 0 ok, -1 if odd or > 0xFE */
int  mem_write16(Memory *m, u16 addr, u16 value);       /* 0 ok, -1 if odd or > 0xFE */
#endif
