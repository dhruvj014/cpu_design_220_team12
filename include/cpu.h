/* cpu.h - CPU state, reset and one-instruction step (never prints). Owner: Dhruv */
#ifndef CPU_H
#define CPU_H
#include "isa.h"
#include "layout.h"
#include "memory.h"

#define FAULT_PC           "PC outside code"
#define FAULT_RESERVED     "reserved bits set"
#define FAULT_LOAD         "bad load address"
#define FAULT_STORE_ADDR   "bad store address"
#define FAULT_STORE_REGION "store outside writable memory"
#define FAULT_HEAP_RANGE   "bad heap top"
#define FAULT_HEAP_STACK   "heap ran into stack"
#define FAULT_TARGET       "bad jump or call target"
#define FAULT_RETURN       "bad return address"
#define FAULT_SP_ODD       "stack pointer is odd"
#define FAULT_UNDERFLOW    "stack underflow"
#define FAULT_STACK_HEAP   "stack ran into heap"
#define FAULT_OVERFLOW     "stack overflow"

typedef struct {
    u16 r[NUM_REGS];
    u16 pc, ir;
    u8  z, c;
    const Layout *layout;       /* must stay alive while the CPU is used */
} Cpu;
typedef struct {
    u16 addr, ir;               /* address and bits of the instruction just run */
    int wrote;                  /* 1 if ST or CALL wrote memory */
    u16 waddr, before, after;   /* valid only when wrote is 1 */
    const char *fault;          /* one of the FAULT_ texts on a fault, else 0 */
} StepInfo;
typedef enum { CPU_OK, CPU_HALT, CPU_FAULT } CpuStatus;

int       layout_valid(const Layout *l);                /* 1 if valid, else 0 */
int       cpu_reset(Cpu *c, const Layout *l);           /* 0 ok, -1 if the layout is invalid */
CpuStatus cpu_step(Cpu *c, Memory *m, StepInfo *info);  /* never prints */
/* The heap top word if heap_ptr_addr is set, else heap_end. */
u16       cpu_heap_top(const Cpu *c, const Memory *m);
#endif
