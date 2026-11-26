# SJ16 Specification (CMPE 220 CPU Design)

This file is the single source of truth for all code. Every team member and coding agent reads it before writing anything.
If this file and any other document disagree, this file wins. Change it only after all three teammates agree.

Due: Sat Dec 5, 2026, 11:59pm. Target submission: Fri Dec 4. Submit one PDF and one zip.

## 1. Assignment rules

- All programming is in C.
- **No library functions except for printing.** Include only `<stdio.h>` and call only print functions.
  - Do not include `string.h`, `stdlib.h`, `ctype.h`, `stdint.h` or `assert.h`.
  - Do not call `fopen`, `fgets`, `getchar`, `sprintf`, `snprintf`, `strcmp`, `memcpy`, `memset`, `malloc` or `exit`.
  - Set or copy arrays and structs with explicit loops or field-by-field assignment. End the program with `return` from `main`.
- Show the whole memory block, in binary and ASCII, initially and after every executed instruction.
- One program is written in SJ16 assembly (`pow`, 8^2). One is written in C and translated to SJ16 assembly (`fact`, 8!).
- Output and code must be understandable without extra explanation. Comment the code.
- The zip must build and run with no edits, following `README.md`.

## 2. Machine

| Item | Value |
| --- | --- |
| Word and instruction size | 16 bits, fixed width |
| Memory | 256 bytes (0x00 to 0xFF), shared by code and data, byte-addressed, big-endian |
| Word access | Even addresses 0x00 to 0xFE only |
| Registers | R0 to R7, 16 bits each |
| R0 | Always reads 0; writes to it are ignored |
| R7 | Stack pointer (SP). `CALL` and `RET` use it, and the CPU checks every new value |
| R6 | Frame pointer (FP) by software convention only. The CPU treats it as a normal register |
| Special | PC, IR, flags Z and C |
| Reset | R0 to R6, IR, Z and C are 0. R7 = `stack_top` (0x0100). PC = `entry` |
| Program limits | Code bounds, writable data, heap and stack limits come from a per-program `Layout` |

All unwritten memory is 0, and the word 0x0000 is `HLT`.

## 3. Program layouts and memory map

Every range is half-open: `[start, end)` includes `start` and excludes `end`.

| `Layout` field | Meaning |
| --- | --- |
| `code_start`, `code_end` | PC, jump targets, call targets and return addresses must lie here |
| `data_start`, `data_end` | Static data that `ST` may write |
| `heap_start`, `heap_end` | Heap, growing up. Equal values mean no heap |
| `heap_ptr_addr` | Address of the data word that holds the current heap top, or `NO_ADDR` |
| `stack_limit`, `stack_top` | SP must stay in `stack_limit <= SP <= stack_top`. The stack grows down |
| `entry` | First PC value |

**Heap top.** With `heap_ptr_addr` set, the heap top is the word stored there: allocated heap is `[heap_start, heap top)`.
The program image sets that word to an even value in `heap_start` to `heap_end`, normally `heap_start`.
The program's allocator moves it with an ordinary `ST`, and the CPU checks that store.
With a heap and `NO_ADDR`, the whole of `[heap_start, heap_end)` counts as allocated.

**Valid layout** (`layout_valid`):

1. All bounds are even and at most 256.
2. `code_start < code_end <= data_start <= data_end <= stack_limit <= stack_top`.
3. `entry` is even and inside the code range.
4. No heap: `heap_start == heap_end` and `heap_ptr_addr == NO_ADDR`.
5. Heap: `data_end <= heap_start < heap_end <= stack_top`, and `stack_limit >= heap_start`.
6. A `heap_ptr_addr` other than `NO_ADDR` is even and its word lies inside the data range.

### CPU Design memory map (`pow`, `fact`)

`programs/cpu_layout.h`, owner Dhruv:
```c
#ifndef CPU_LAYOUT_H
#define CPU_LAYOUT_H
#include "layout.h"
static const Layout CPU_LAYOUT = {
    0x00, 0x30,       /* code  */
    0x30, 0x40,       /* data  */
    0x00, 0x00,       /* no heap */
    NO_ADDR,
    0x40, 0x100,      /* stack limit, stack top */
    0x00              /* entry */
};
#endif
```

| Address | Use | In the trace |
| --- | --- | --- |
| 0x00 to 0x2F | Code (24 instructions) | printed |
| 0x30 to 0x37 | Data words (variables, then the return slot for compiled C) | printed |
| 0x38 to 0x3F | ASCII tag | printed |
| 0x40 to 0xFF | Stack region. `pow` and `fact` never use it | not printed |

The trace always prints 0x00 to 0x3F. A write outside that window still appears on the `CHANGED` line.

## 4. Instruction set

Fields: `opcode = ir[15:12]`, `rd = ir[11:9]`, `rs1 = ir[8:6]`, `rs2 = ir[5:3]`, `imm6 = ir[5:0]`, `imm9 = ir[8:0]`.

| Format | 15-12 | 11-9 | 8-6 | 5-3 | 2-0 | Instructions | Reserved bits (must be 0) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| R | op | rd | rs1 | rs2 | 000 | ADD SUB MUL AND | 2-0 |
| C | op | 000 | rs1 | rs2 | 000 | CMP | 11-9 and 2-0 |
| I | op | rd (ST: source) | rs1 | imm6 (5-0) | | LD ST ADDI | none |
| L | op | rd | imm9 (8-0) | | | LDI | none |
| J | op | 000 | addr9 (8-0) | | | CALL JMP JZ JNZ JC | 11-9 |
| N | op | 000 | 000 | 000 | 000 | HLT RET | 11-0 |

| Op | Assembly | Operation | Flags |
| --- | --- | --- | --- |
| 0 | `HLT` | Stop | unchanged |
| 1 | `LDI Rd, imm9` | Rd = imm9 (0 to 511) | unchanged |
| 2 | `LD Rd, [Rs1+imm6]` | Rd = mem16[Rs1 + imm6], offset 0 to 63 | unchanged |
| 3 | `ST Rd, [Rs1+imm6]` | mem16[Rs1 + imm6] = Rd, offset 0 to 63 | unchanged |
| 4 | `ADD Rd, Rs1, Rs2` | Rd = Rs1 + Rs2 | Z, C = carry out of bit 15 |
| 5 | `SUB Rd, Rs1, Rs2` | Rd = Rs1 - Rs2 | Z, C = borrow (Rs1 < Rs2) |
| 6 | `MUL Rd, Rs1, Rs2` | Rd = low 16 bits of product | Z, C = product > 0xFFFF |
| 7 | `AND Rd, Rs1, Rs2` | bitwise AND | Z, C = 0 |
| 8 | `CALL addr` | SP = SP - 2; mem16[SP] = address of next instruction; PC = addr | unchanged |
| 9 | `RET` | PC = mem16[SP]; SP = SP + 2 | unchanged |
| 10 | `ADDI Rd, Rs1, simm6` | Rd = Rs1 + sign-extended imm6 (-32 to 31) | Z, C = 16-bit carry out |
| 11 | `CMP Rs1, Rs2` | Rs1 - Rs2, result discarded | Z, C = borrow |
| 12 | `JMP addr` | PC = addr | unchanged |
| 13 | `JZ addr` | if Z = 1, PC = addr | unchanged |
| 14 | `JNZ addr` | if Z = 0, PC = addr | unchanged |
| 15 | `JC addr` | if C = 1, PC = addr | unchanged |

Z = 1 when the 16-bit result is 0. All values are unsigned 16-bit. Flags are set even when Rd is R0.
Addresses are 8 bits wide in practice; bit 8 of `addr9` is caught by the target check.

### Execution cycle (`cpu_step`)

1. Fault unless PC is even, `PC >= code_start` and `PC + 2 <= code_end`.
2. `info->addr = PC`; IR = mem16[PC]; the next PC is PC + 2.
3. Decode. Fault if any reserved bit is 1.
4. Execute.
   - LD and ST: compute `rs1 value + imm6` in `unsigned long`. Fault if the address is odd or above 0xFE.
   - ST may write a word only if it lies completely inside one of: the data range; the allocated heap;
     the live stack `[SP, stack_top)`.
   - ST to `heap_ptr_addr`: the new value must be even and in `heap_start` to `heap_end`, and must not be above SP.
   - Jumps: check the target only when the jump is taken. A target must be even, `>= code_start`, and `target + 2 <= code_end`.
   - CALL: check the target, then check the new SP (`SP - 2`), then store the return address and set SP and PC.
   - RET: fault if `SP + 2 > stack_top`; read mem16[SP]; check it like a jump target; check `SP + 2`; then set SP and PC.
   - Any other instruction whose destination is R7 (LDI, LD, ADD, SUB, MUL, AND, ADDI) checks the new SP before writing it.
   - For ADDI, sign-extend: if bit 5 is set, OR the value with 0xFFC0.
   - For MUL, cast both operands to `unsigned long` before multiplying (`u16 * u16` can overflow `int`).
5. Return `CPU_HALT` for HLT, `CPU_FAULT` with `info->fault` set on a fault, otherwise `CPU_OK`.

**Stack check** for a new SP value, in this order:

| Condition | Fault macro | Text |
| --- | --- | --- |
| SP is odd | `FAULT_SP_ODD` | stack pointer is odd |
| SP > `stack_top` | `FAULT_UNDERFLOW` | stack underflow |
| there is a heap and SP < heap top | `FAULT_STACK_HEAP` | stack ran into heap |
| SP < `stack_limit` | `FAULT_OVERFLOW` | stack overflow |

SP equal to the heap top is allowed: the two regions touch but do not overlap.

**Other faults:**

| Fault macro | Text |
| --- | --- |
| `FAULT_PC` | PC outside code |
| `FAULT_RESERVED` | reserved bits set |
| `FAULT_LOAD` | bad load address |
| `FAULT_STORE_ADDR` | bad store address |
| `FAULT_STORE_REGION` | store outside writable memory |
| `FAULT_HEAP_RANGE` | bad heap top |
| `FAULT_HEAP_STACK` | heap ran into stack |
| `FAULT_TARGET` | bad jump or call target |
| `FAULT_RETURN` | bad return address |

For a heap top store, check the range first, then the stack.
A faulting instruction changes no register, flag or memory byte. `main` stops with a fault after 1000 steps without HLT.
HLT counts as a step.

## 5. Calling convention

| Register | Role | Kept across a call? |
| --- | --- | --- |
| R0 | Constant zero | yes |
| R1 | Return value; otherwise scratch | no |
| R2 to R5 | Scratch | no |
| R6 | Frame pointer | yes (callee saves it) |
| R7 | Stack pointer | yes (balanced) |

One argument and one local per frame. After the prologue:

| Address | Content |
| --- | --- |
| FP+6 | argument (pushed by the caller) |
| FP+4 | return address (pushed by `CALL`) |
| FP+2 | caller's FP |
| FP+0 | local (SP = FP in the function body) |

Caller (argument in Ra):
```text
ADDI R7, R7, -2      ; make room first: a store below SP faults
ST   Ra, [R7+0]      ; push argument
CALL f
ADDI R7, R7, 2       ; drop argument; the result is in R1
```
Prologue:
```text
ADDI R7, R7, -2
ST   R6, [R7+0]      ; save caller's FP
ADDI R7, R7, -2      ; room for the local
ADD  R6, R7, R0      ; FP = bottom of the frame
```
Epilogue (result already in R1):
```text
ADD  R7, R6, R0      ; SP = FP
LD   R6, [R7+2]      ; restore caller's FP
ADDI R7, R7, 4       ; drop local and saved FP
RET
```

- One complete frame uses 8 bytes: argument 2, return address 2, saved FP 2, local 2.
- `main` starts at `entry` with FP = 0 and an empty stack. It needs no frame of its own and ends with `HLT`.
- Every LD and ST offset is zero or positive. Read the argument through FP: pushing moves SP but not FP.
- Save any value needed after a `CALL` in the frame first.
- `ADD` and `ADDI` change Z and C. Put none of them between `CMP` and its conditional jump.
- With k local words, saved FP moves to FP+2k, the return address to FP+2k+2 and the argument to FP+2k+4.
- The CPU Design compiler does not support functions with arguments or recursion. Recursive code is translated by hand.

## 6. Headers (the contract between modules)

Create these exactly. Each needs an include guard and includes what it uses.

| Group | Headers | Sources | Rule |
| --- | --- | --- | --- |
| Core | `isa.h` `layout.h` `memory.h` `alu.h` `cpu.h` | `memory.c` `alu.c` `cpu.c` | No `<stdio.h>`, no printing, no program images, no trace settings |
| Tools | `util.h` `assembler.h` `disasm.h` | `util.c` `assembler.c` `disasm.c` | No fixed memory map: bounds come from `Layout` |
| CPU Design only | `compiler.h` `trace.h` | `compiler.c` `trace.c` `main.c` and `programs/` | Printing and program images live here |

### `include/isa.h`
```c
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
```

### `include/layout.h`
```c
#ifndef LAYOUT_H
#define LAYOUT_H
#include "isa.h"
#define NO_ADDR 0xFFFF
/* Ranges are [start, end). Rules: SPEC section 3. */
typedef struct {
    u16 code_start, code_end;
    u16 data_start, data_end;
    u16 heap_start, heap_end;      /* equal means no heap */
    u16 heap_ptr_addr;             /* data word holding the heap top, or NO_ADDR */
    u16 stack_limit, stack_top;
    u16 entry;
} Layout;
#endif
```

### `include/memory.h`
```c
#ifndef MEMORY_H
#define MEMORY_H
#include "isa.h"
typedef struct { u8 byte[MEM_SIZE]; } Memory;
void mem_clear(Memory *m);
int  mem_read16(const Memory *m, u16 addr, u16 *out);   /* 0 ok, -1 if odd or > 0xFE */
int  mem_write16(Memory *m, u16 addr, u16 value);       /* 0 ok, -1 if odd or > 0xFE */
#endif
```

### `include/alu.h`
```c
#ifndef ALU_H
#define ALU_H
#include "isa.h"
typedef struct { u16 value; u8 z, c; } AluOut;
/* op is one of ADD SUB MUL AND ADDI CMP. For ADDI, b is already sign-extended. */
AluOut alu_exec(Opcode op, u16 a, u16 b);
#endif
```

### `include/cpu.h`
```c
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
```

### `include/util.h`
```c
#ifndef UTIL_H
#define UTIL_H
int str_eq(const char *a, const char *b);        /* 1 if identical, else 0 */
int str_len(const char *s);
int is_space(char c);                            /* space, tab or carriage return (not newline) */
int is_digit(char c);
int is_alpha(char c);                            /* letters and underscore */
/* Whole string must be decimal or 0x hex, with optional leading '-'.
   Returns 0 and sets *out, or -1 if malformed or magnitude > 65535. */
int parse_number(const char *s, long *out);
#endif
```

### `include/assembler.h`
```c
#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include "layout.h"
#include "memory.h"
/* Instructions and targets must lie in the layout's code range. 0 ok, else failing line number (from 1). */
int assemble(const char *source, Memory *m, const Layout *l);
#endif
```

### `include/disasm.h`
```c
#ifndef DISASM_H
#define DISASM_H
#include "isa.h"
void disasm_print(u16 ir);   /* one instruction, no newline */
#endif
```

### `include/compiler.h`
```c
#ifndef COMPILER_H
#define COMPILER_H
#define ASM_CAP 2048
/* Writes NUL-terminated SJ16 assembly to asm_out. 0 ok, else failing line number (from 1). */
int compile(const char *c_source, const char *tag, char *asm_out, int capacity);
#endif
```

### `include/trace.h`
```c
#ifndef TRACE_H
#define TRACE_H
#include "cpu.h"
#define TRACE_END   0x40     /* the trace prints 0x00 to TRACE_END - 1 */
#define TRACE_TAG   0x38     /* rows from here are labelled "tag" */
/* info == 0 prints the initial snapshot. Never changes state. */
void trace_print(int step, const Cpu *c, const Memory *m, const StepInfo *info);
#endif
```

## 7. Assembly language

- One statement per line: optional `label:`, then an optional instruction or directive, then an optional `; comment`.
- A `;` inside the quotes of `.ascii` is text, not a comment. A label may stand alone on a line.
- Mnemonics and register names are uppercase. Spaces and tabs are free around tokens. Commas between operands are required.
- Labels: a letter or `_`, then letters, digits or `_`. Case-sensitive, at most 15 characters, at most 16 labels, no duplicates.
- Numbers: decimal or `0x` hex. A leading `-` is allowed only for the ADDI immediate.
- LD and ST operands are written `[Rn+imm]`; the `+imm` part is required.
- A jump or call operand is a label or a number. It must be a valid target in the layout's code range.
- `RET` and `HLT` take no operand.
- Directives: `.org addr` (0 to 255), `.word value` (0 to 65535, even address), `.ascii "text"` (no escapes, must fit in memory).
- Instructions must sit at even addresses inside the layout's code range. For `pow` and `fact` that is below 0x30.
- `.word` and `.ascii` may write any address that exists in memory. A program with a heap sets its heap top word with `.word`.
- `assemble` clears memory first. Pass 1 records labels; pass 2 encodes. It stops at the first error.
- `assemble` and `compile` print nothing. The caller prints the line number they return.

## 8. C subset accepted by the compiler

```text
program  := "unsigned" "short" NAME "(" "void" ")" "{" decl* for return "}" END
decl     := "unsigned" "short" item { "," item } ";"
item     := NAME [ "=" NUMBER ]
for      := "for" "(" NAME "=" NUMBER ";" NAME "<=" NAME ";" NAME "++" ")" body
body     := NAME "=" NAME ( "+" | "-" | "*" ) NAME ";"
return   := "return" NAME ";"
```

- At most 3 variables, names at most 15 characters. Numbers are decimal. No comments and no braces around the loop body.
- Errors: unsupported syntax, undeclared or duplicate name, too many variables, constant out of range
  (initial value 0 to 65535; loop start 0 to 511), output buffer too small.
- Variables get words from 0x30 in declaration order. The return slot is the next word. No initial value means 0.

Emit exactly this instruction order (`@x` means `[R0+0xNN]`, the address of variable x):

```text
        .org 0x00
        LDI  R1, start        ; for init
        ST   R1, @v
loop:   LD   R1, @a           ; condition a <= b
        LD   R2, @b
        CMP  R2, R1
        JC   done
        LD   R1, @x           ; body d = x op y
        LD   R2, @y
        OP   R1, R1, R2       ; ADD, SUB or MUL
        ST   R1, @d
        LD   R1, @w           ; step w++
        ADDI R1, R1, 1
        ST   R1, @w
        JMP  loop
done:   LD   R1, @r           ; return r
        ST   R1, @ret
        HLT
        .org 0x30
        .word ...             ; one per variable, then .word 0 for the return slot
        .org 0x38
        .ascii "tag"          ; the tag argument, at most 8 characters
```

## 9. Programs (golden values)

All hex below was checked with an independent model of section 4. `pow` and `fact` use neither `CALL` nor `RET`.

### `pow`: 8^2 = 64, 19 steps, result at 0x34

```text
ADDR  HEX    SOURCE
0x00  2230          LD   R1, [R0+0x30]    ; R1 = base
0x02  2432          LD   R2, [R0+0x32]    ; R2 = exponent
0x04  1601          LDI  R3, 1            ; result = 1
0x06  3634          ST   R3, [R0+0x34]
0x08  B080  loop:   CMP  R2, R0           ; exponent == 0 ?
0x0A  D014          JZ   done
0x0C  66C8          MUL  R3, R3, R1       ; result *= base
0x0E  3634          ST   R3, [R0+0x34]
0x10  A4BF          ADDI R2, R2, -1
0x12  C008          JMP  loop
0x14  0000  done:   HLT
```
Data: 0x30 = 8, 0x32 = 2, 0x34 = 0, 0x36 = 0. Tag: `POW 8^2 ` (8 bytes, trailing space).
Exponent check with base 8: exp 0 gives 1 (7 steps), 1 gives 8 (13), 3 gives 512 (25), 5 gives 32768 (37).

### `fact`: 8! = 40320, 93 steps, result at 0x36

C source:
```c
unsigned short factorial(void) {
    unsigned short n = 8, result = 1, i;
    for (i = 2; i <= n; i++)
        result = result * i;
    return result;
}
```

```text
ADDR  HEX    SOURCE
0x00  1202          LDI  R1, 2
0x02  3234          ST   R1, [R0+0x34]
0x04  2234  loop:   LD   R1, [R0+0x34]
0x06  2430          LD   R2, [R0+0x30]
0x08  B088          CMP  R2, R1
0x0A  F01C          JC   done
0x0C  2232          LD   R1, [R0+0x32]
0x0E  2434          LD   R2, [R0+0x34]
0x10  6250          MUL  R1, R1, R2
0x12  3232          ST   R1, [R0+0x32]
0x14  2234          LD   R1, [R0+0x34]
0x16  A241          ADDI R1, R1, 1
0x18  3234          ST   R1, [R0+0x34]
0x1A  C004          JMP  loop
0x1C  2232  done:   LD   R1, [R0+0x32]
0x1E  3236          ST   R1, [R0+0x36]
0x20  0000          HLT
```
Initial data: 0x30 = 8, 0x32 = 1, 0x34 = 0, 0x36 = 0. Tag: `FACT 8! `.
Final data: 0x30 = 8, 0x32 = 40320, 0x34 = 9, 0x36 = 40320.

### `rec`: recursive conformance test (not part of the CPU Design submission)

`tests/rec.inc`, owner Mokshit. It proves that the core runs recursion. It computes factorial(5).

Test layout: code 0x00 to 0x3F, empty data range at 0x40, no heap, stack limit 0x40, stack top 0x100, entry 0.
It is 56 bytes, so it does not fit the CPU Design code range.

```text
ADDR  HEX   LABEL   SOURCE
0x00  1205          LDI  R1, 5              ; main: n = 5
0x02  AFFE          ADDI R7, R7, -2         ; push argument
0x04  33C0          ST   R1, [R7+0]
0x06  800C          CALL fact
0x08  AFC2          ADDI R7, R7, 2          ; drop argument; R1 = 120
0x0A  0000          HLT
0x0C  AFFE  fact:   ADDI R7, R7, -2         ; prologue
0x0E  3DC0          ST   R6, [R7+0]         ; save caller FP
0x10  AFFE          ADDI R7, R7, -2         ; room for local
0x12  4DC0          ADD  R6, R7, R0         ; FP = SP
0x14  2386          LD   R1, [R6+6]         ; R1 = n
0x16  1402          LDI  R2, 2
0x18  B050          CMP  R1, R2             ; n < 2 ?
0x1A  F02C          JC   base               ; unsigned: C = 1 means n < 2
0x1C  A27F          ADDI R1, R1, -1         ; n - 1
0x1E  AFFE          ADDI R7, R7, -2         ; push argument
0x20  33C0          ST   R1, [R7+0]
0x22  800C          CALL fact
0x24  AFC2          ADDI R7, R7, 2          ; drop argument; R1 = fact(n-1)
0x26  2586          LD   R2, [R6+6]         ; R2 = n
0x28  6288          MUL  R1, R2, R1         ; R1 = n * fact(n-1)
0x2A  C02E          JMP  done
0x2C  1201  base:   LDI  R1, 1              ; base case
0x2E  3380  done:   ST   R1, [R6+0]         ; local = result
0x30  4F80          ADD  R7, R6, R0         ; epilogue
0x32  2DC2          LD   R6, [R7+2]         ; restore FP
0x34  AFC4          ADDI R7, R7, 4
0x36  9000          RET
```

| Check | Value |
| --- | --- |
| Steps, including HLT | 104 |
| Final R1, R6, R7 | 120, 0, 0x0100 |
| CALL steps (SP after) | 4 (0xFC), 16 (0xF4), 28 (0xEC), 40 (0xE4), 52 (0xDC) |
| RET steps (R1, SP after) | 66 (1, 0xDE), 75 (2, 0xE6), 84 (6, 0xEE), 93 (24, 0xF6), 102 (120, 0xFE) |
| Lowest SP | 0xD8 |
| Change first word to `1201` (n = 1) | 20 steps, R1 = 1 |
| Change first word to `1208` (n = 8) | 167 steps, R1 = 40320, lowest SP 0xC0 |
| Change first word to `1264` (n = 100) | `FAULT_OVERFLOW` on step 290, SP = 0x40 |

Stack just before the deepest frame (n = 1) runs its epilogue, FP = SP = 0xD8:

| Frame | Argument | Return address | Saved FP | Local |
| --- | --- | --- | --- | --- |
| fact(5) | 0xFE: 5 | 0xFC: 0x0008 | 0xFA: 0x0000 | 0xF8: 0 |
| fact(4) | 0xF6: 4 | 0xF4: 0x0024 | 0xF2: 0x00F8 | 0xF0: 0 |
| fact(3) | 0xEE: 3 | 0xEC: 0x0024 | 0xEA: 0x00F0 | 0xE8: 0 |
| fact(2) | 0xE6: 2 | 0xE4: 0x0024 | 0xE2: 0x00E8 | 0xE0: 0 |
| fact(1) | 0xDE: 1 | 0xDC: 0x0024 | 0xDA: 0x00E0 | 0xD8: 1 |

### `.inc` format

A program file holds only C string literals, one source line each:
```c
"        .org 0x00\n"
"        LD   R1, [R0+0x30]    ; R1 = base\n"
```
`main.c` includes it like this:
```c
static const char *POW_ASM =
#include "pow.inc"
;
```
Files: `pow.inc` and `fact.inc` (assembly, including `.org`, data and tag lines), `fact_c.inc` (the C function above),
`fact.c` (the same function plus a `main` that prints its result, for checking with a normal compiler).

## 10. Output format

Snapshot for step 8 of `pow`. Match the spacing.

```text
==================================================================
 STEP 008   ADDR 0x0E   IR 0011011000110100   ST R3, [R0+0x34]
 R0=0000 R1=0008 R2=0002 R3=0008 R4=0000 R5=0000 R6=0000 R7=0100
 NEXT PC=0x10   Z=0  C=0   CHANGED: [0x34] 0x0001 -> 0x0008
------------------------------------------------------------------
  ADDR   +0       +1       +2       +3        ASCII
  0x00   00100010 00110000 00100100 00110010  "0$2
  0x04   00010110 00000001 00110110 00110100  ..64
  0x08   10110000 10000000 11010000 00010100  ....
> 0x0C   01100110 11001000 00110110 00110100  f.64
  0x10   10100100 10111111 11000000 00001000  ....
  0x14   00000000 00000000 00000000 00000000  ....
  0x18   00000000 00000000 00000000 00000000  ....
  0x1C   00000000 00000000 00000000 00000000  ....
  0x20   00000000 00000000 00000000 00000000  ....
  0x24   00000000 00000000 00000000 00000000  ....
  0x28   00000000 00000000 00000000 00000000  ....
  0x2C   00000000 00000000 00000000 00000000  ....
  0x30   00000000 00001000 00000000 00000010  ....   data
* 0x34   00000000 00001000 00000000 00000000  ....   data
  0x38   01010000 01001111 01010111 00100000  POW    tag
  0x3C   00111000 01011110 00110010 00100000  8^2    tag
==================================================================
```

- `>` marks the row holding the instruction just run. `*` marks the row that ST or CALL wrote, even if the value stayed the same.
- The table covers 0x00 to 0x3F only. If a write is outside it, no row gets `*`; the `CHANGED` line still reports it.
- Bytes outside 0x20 to 0x7E print as `.`.
- With no write, print `CHANGED: none`.
- The initial snapshot has ` STEP 000   initial memory` as its only header line, then the memory table with no markers.
- Disassembly: `HLT`, `LDI R3, 1`, `LD R1, [R0+0x30]`, `ST R3, [R0+0x34]`, `MUL R3, R3, R1`,
  `ADDI R2, R2, -1`, `CMP R2, R0`, `JZ 0x14`, `CALL 0x0C`, `RET`. Print `???` for a word with bad reserved bits.

## 11. Program behavior (`main.c`)

| Command | Behavior |
| --- | --- |
| `./cpu pow` | Assemble `pow.inc`, run with full trace |
| `./cpu fact` | Print the C source, compile it, print the generated assembly, assemble, run with full trace |
| `./cpu fact-hand` | Assemble `fact.inc`, run with full trace |
| `./cpu check` | Compile and assemble the C text, assemble `fact.inc`, compare all 256 bytes. Print `CHECK PASSED` or the first differing address |
| anything else | Print usage |

`pow`, `fact` and `fact-hand` assemble and run with `CPU_LAYOUT`. `main` stops if `cpu_reset` or `assemble` fails.

A run ends with `RESULT: <value>   STEPS: <count>`. A fault prints `FAULT at 0xNN: <reason>`.
Every failure returns 1; success returns 0.

**Backup rule:** if `./cpu check` is not passing by the end of Tue Dec 1, remove `compiler.c`, `compiler.h` and `fact_c.inc`,
remove the `check` and `fact-hand` commands, and make `./cpu fact` run `fact.inc`. Update the README and PDF to match.

## 12. Layout, build and ownership

```text
cpu-sim/
  Makefile  README.md  .gitattributes  .gitignore
  include/   isa.h layout.h memory.h alu.h cpu.h util.h assembler.h disasm.h compiler.h trace.h
  src/       main.c util.c memory.c alu.c cpu.c assembler.c disasm.c trace.c compiler.c
  programs/  cpu_layout.h pow.inc fact.c fact_c.inc fact.inc
  tests/     test_core.c test_asm.c test_compiler.c test_stack.c rec.inc
  output/    pow_trace.txt fact_trace.txt
  docs/      SPEC.md
```

| Owner | Files | PDF | Also |
| --- | --- | --- | --- |
| Dhruv | headers, `cpu_layout.h`, `util.c`, `memory.c`, `alu.c`, `cpu.c`, `main.c`, `Makefile`, `tests/test_core.c` | Datapath and instruction cycle figures; register, memory map and build command tables | Integration, `README.md` |
| Tanisha | `assembler.c`, `disasm.c`, `trace.c`, `pow.inc`, `tests/test_asm.c` | Instruction format and encoding tables, traces | Zip |
| Mokshit | `compiler.c`, `fact.c`, `fact_c.inc`, `fact.inc`, `tests/test_compiler.c`, `tests/test_stack.c`, `tests/rec.inc` | Listings, title page, final PDF assembly | Final read-through of the PDF against the rubric |

- Compiler flags: `-std=c99 -Wall -Wextra -pedantic -fno-builtin -Iinclude -Iprograms`. Zero warnings on GCC (WSL) and Apple Clang (macOS).
- `make symbols` also runs `nm -u memory.o alu.o cpu.o`: the only undefined names may be ones those three files define.
- Build and test on both Windows (WSL) and macOS before the core is shared or submitted.
- `.gitattributes` contains `* text=auto eol=lf`. Use lowercase file names. Makefile recipes use tabs.
- Every file starts with a comment giving its purpose and owner. Every function has a one-line comment.
- One Git branch per person; another teammate reviews before merging.
- Reviews: Dhruv reviews Mokshit, Mokshit reviews Tanisha, Tanisha reviews Dhruv.

## 13. Submission checklist

- [ ] PDF: title page with all names, pages numbered `##/##`, every figure and table captioned
- [ ] PDF: annotated CPU diagram and instruction encoding table
- [ ] PDF: every snapshot of both programs (20 for `pow`, 94 for `fact`)
- [ ] PDF: figures, tables, listings and output, with no long prose
- [ ] PDF: contributions page, two to three specific and truthful sentences per person
- [ ] Zip: source, `README.md`, both full traces; no binaries, `.git` or `.DS_Store`
- [ ] Unzipped into an empty folder, it builds and runs by following only the README
- [ ] Each person can explain any part of the code
