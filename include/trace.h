/* trace.h - Interface of the per-step memory trace printer (implemented by Tanisha). Owner: Dhruv */
#ifndef TRACE_H
#define TRACE_H
#include "cpu.h"
#define TRACE_END   0x40     /* the trace prints 0x00 to TRACE_END - 1 */
#define TRACE_TAG   0x38     /* rows from here are labelled "tag" */
/* info == 0 prints the initial snapshot. Never changes state. */
void trace_print(int step, const Cpu *c, const Memory *m, const StepInfo *info);
#endif
