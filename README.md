# SJ16 CPU simulator (CMPE 220, Team 12)

A 16-bit CPU simulated in C. It runs two programs and prints the whole memory block, in binary and ASCII,
before the first instruction and after every instruction:

- `pow`: 8^2, written in SJ16 assembly
- `fact`: 8!, written in C, compiled to SJ16 assembly by our own compiler, then run

All code is C. The only library functions used are printing functions from `<stdio.h>`.

## Requirements

`gcc` (or `clang`) and `make`. Tested on: WSL Ubuntu (GCC 15). *(Add macOS here only after it has been run.)*

## Build

```
make
```
Without make:
```
gcc -std=c99 -fno-builtin -Iinclude -Iprograms src/*.c -o cpu
```

## Run

| Command | What it does |
| --- | --- |
| `./cpu pow` | Assemble and run 8^2, full trace |
| `./cpu fact` | Print the C source, compile it, print the generated assembly, assemble and run 8!, full trace |
| `./cpu fact-hand` | Run the hand-written assembly for 8! |
| `./cpu check` | Check that compiled and hand-written 8! give the same memory image |

Each run ends with `RESULT: <value>   STEPS: <count>`.
`pow` gives 64 in 19 steps; `fact` gives 40320 in 93 steps. Ready-made traces are in `output/`.

## Reading the trace

- `>` marks the row holding the instruction just run; `*` marks the row it wrote.
- Each row shows 4 bytes in binary, then their ASCII characters (`.` if not printable).
- Memory rows 0x00 to 0x3F are shown (code, data, ASCII tag). The stack (0x40 to 0xFF) is not used by these programs.

## Test

```
make test       # unit tests
make symbols    # lists the library functions the program calls (printing only)
```

## Folder layout

```
Makefile  README.md
include/   headers (isa, layout, memory, alu, cpu, util, assembler, disasm, compiler, trace)
src/       main.c util.c memory.c alu.c cpu.c assembler.c disasm.c trace.c compiler.c
programs/  cpu_layout.h pow.inc fact.c fact_c.inc fact.inc
tests/     unit tests
output/    pow_trace.txt fact_trace.txt
```

## Note

The CPU core (`memory.c`, `alu.c`, `cpu.c`) is shared with our Program Layout & Execution submission.
