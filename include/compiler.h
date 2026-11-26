/* compiler.h - Interface of the tiny C-to-SJ16 compiler (implemented by Mokshit). Owner: Dhruv */
#ifndef COMPILER_H
#define COMPILER_H
#define ASM_CAP 2048
/* Writes NUL-terminated SJ16 assembly to asm_out. 0 ok, else failing line number (from 1). */
int compile(const char *c_source, const char *tag, char *asm_out, int capacity);
#endif
