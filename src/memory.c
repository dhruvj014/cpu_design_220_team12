/* memory.c - 256-byte big-endian memory with 16-bit access. Owner: Dhruv */
#include "memory.h"

/* Sets every byte to 0. */
void mem_clear(Memory *m)
{
    int i;
    for (i = 0; i < MEM_SIZE; i++)
        m->byte[i] = 0;
}

/* Reads the word at an even address 0x00-0xFE (high byte first). 0 ok, -1 if odd or too high. */
int mem_read16(const Memory *m, u16 addr, u16 *out)
{
    if ((addr & 1) != 0 || addr > 0xFE)
        return -1;
    *out = (u16)((m->byte[addr] << 8) | m->byte[addr + 1]);
    return 0;
}

/* Writes the word at an even address 0x00-0xFE (high byte first). 0 ok, -1 if odd or too high. */
int mem_write16(Memory *m, u16 addr, u16 value)
{
    if ((addr & 1) != 0 || addr > 0xFE)
        return -1;
    m->byte[addr] = (u8)(value >> 8);
    m->byte[addr + 1] = (u8)(value & 0xFF);
    return 0;
}
