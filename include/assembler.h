/* assembler.h - Interface of the SJ16 assembler (implemented by Tanisha). Owner: Dhruv */
#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include "layout.h"
#include "memory.h"
/* Instructions and targets must lie in the layout's code range. 0 ok, else failing line number (from 1). */
int assemble(const char *source, Memory *m, const Layout *l);
#endif
