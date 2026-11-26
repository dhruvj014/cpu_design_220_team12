# Build plan: Dhruv (CPU core)

## Instructions for the coding agent

1. Read `SPEC.md` completely before writing code. It is the source of truth. If something is unclear or looks wrong, stop and ask.
2. Write only the files listed under "Your files". Teammates own the rest. Create their files only as the stubs in Phase 1.
3. Obey the library rule in SPEC section 1. Only `<stdio.h>` and print functions.
4. Work one phase at a time. At the end of each phase: build with zero warnings, run the checks, show the real output,
   explain the new code in plain language, then **stop and wait** for Dhruv.
5. Never change an expected value to make a test pass. Report the mismatch.
6. The core (`memory.c`, `alu.c`, `cpu.c` and their headers) is also reused by another program. It never includes
   `<stdio.h>`, never prints, and contains no program image and no fixed memory map.
7. Do not change a header after Phase 1 without asking. Teammates build against them.
8. Keep the code simple and readable: short functions, clear names, no clever macros. Dhruv must be able to explain every line.
9. Do not commit or push unless Dhruv asks.
10. Commit messages start with the phase, e.g. `Phase 2: add util.c and memory.c with tests`. One commit per phase.

## Your files

`include/*.h`, `programs/cpu_layout.h`, `src/util.c`, `src/memory.c`, `src/alu.c`, `src/cpu.c`, `src/main.c`, `Makefile`,
`tests/test_core.c`, `README.md`, `.gitattributes`, `.gitignore`

Environment: Windows with WSL (Ubuntu). Keep the repo in the Ubuntu home folder (`~/cpu-sim`).
Setup: `wsl --install` in an administrator PowerShell, restart, then in Ubuntu
`sudo apt update && sudo apt install build-essential git zip`.

## Phase 1: skeleton (Wed Nov 25 to Thu Nov 26)

- [ ] Create the folder layout in SPEC section 12 and copy `SPEC.md` into `docs/`.
- [ ] Create every header exactly as in SPEC section 6, and `programs/cpu_layout.h` as in SPEC section 3.
- [ ] `.gitattributes`: `* text=auto eol=lf`. `.gitignore`: `cpu`, `*.o`, `test_core`, `test_asm`, `test_compiler`, `test_stack`, `.DS_Store`.
- [ ] Stubs, each marked `/* STUB - owner: NAME */`:
  - `src/assembler.c`: `assemble` returns 1
  - `src/disasm.c`: `disasm_print` prints `???`
  - `src/trace.c`: `trace_print` prints nothing
  - `src/compiler.c`: `compile` returns 1
  - Mark unused parameters with `(void)name;` so there are no warnings.
- [ ] `Makefile` targets: `all` (builds `cpu` from `src/*.c`), `clean`, `test`, `symbols`, `traces`.
  - `test`: builds `test_core` from `tests/test_core.c` plus `util.c memory.c alu.c cpu.c`, then runs it.
    Teammates add `test_asm`, `test_compiler` and `test_stack` the same way.
  - `symbols`: rebuilds `cpu` with `-fno-stack-protector` added, then runs `nm -u cpu`. It also runs `nm -u` on `memory.o`, `alu.o` and `cpu.o`.
  - `traces`: creates `output/` and saves `./cpu pow` and `./cpu fact` to the two trace files.
- [ ] A minimal `main.c` that prints usage.

**Check:** `make` succeeds with zero warnings.

## Phase 2: helpers, memory, ALU (Thu Nov 26)

- [ ] `util.c`: the six functions in `util.h`. Teammates depend on these, so push them early.
- [ ] `memory.c`: clear, big-endian 16-bit read and write with bounds and alignment checks.
- [ ] `alu.c`: one `switch`, using `unsigned long` for intermediate results.
- [ ] `tests/test_core.c`: a small `expect(condition, "name")` helper that counts failures. `main` returns 1 if any fail.

Required test cases:

| ALU input | value | Z | C |
| --- | --- | --- | --- |
| ADD 0xFFFF, 1 | 0x0000 | 1 | 1 |
| ADD 2, 3 | 0x0005 | 0 | 0 |
| SUB 1, 2 | 0xFFFF | 0 | 1 |
| CMP 5, 5 | 0x0000 | 1 | 0 |
| MUL 5040, 8 | 40320 | 0 | 0 |
| MUL 0xFFFF, 0xFFFF | 0x0001 | 0 | 1 |
| AND 0xF0F0, 0x0F0F | 0x0000 | 1 | 0 |
| AND 0x00FF, 0x0F0F | 0x000F | 0 | 0 |
| ADDI 8, 0xFFFF | 0x0007 | 0 | 1 |

Also test: writing 0x1234 at 0x30 gives bytes 0x12, 0x34; a read at 0xFE succeeds; reads at 0x31, 0xFF and 0x100 fail;
`parse_number` accepts `0`, `511`, `0x3E`, `-32` and rejects an empty string, `-`, `0x`, `12a`, `70000`.

**Check:** `make test` passes.

## Phase 3: CPU (Fri Nov 27)

- [ ] `cpu.c`: `layout_valid`, `cpu_reset`, `cpu_heap_top` and `cpu_step`, following SPEC sections 3 and 4 exactly.
      Use small helpers: decode, code target check, stack check, write permission, load/store.
      Every write to R7 goes through the stack check.
- [ ] In `test_core.c`, load the `pow` hex words and data from SPEC section 9 straight into memory, using `CPU_LAYOUT`.
      Run to HLT. Expect 19 steps, 64 at 0x34 and R7 = 0x0100.
- [ ] Repeat with exponents 0, 1, 3 and 5, using the results and step counts in SPEC section 9.
- [ ] Load the `fact` hex words and data. Expect 93 steps and the final data in SPEC section 9.
- [ ] `layout_valid`: accepts `CPU_LAYOUT`; rejects an odd bound, `stack_top` 0x102, data starting before `code_end`,
      an entry outside code, and a heap pointer address with no heap.
- [ ] `8004 0000 9000` (CALL, HLT, RET) halts after 3 steps with SP = 0x100.
- [ ] Fault tests with `CPU_LAYOUT`. Load the words from 0x00. Each must return `CPU_FAULT` with the listed text,
      and the faulting instruction must leave registers, flags and memory unchanged:

| Words | Case | `info.fault` |
| --- | --- | --- |
| `2231` | LD from an odd address | `FAULT_LOAD` |
| `13FF 2441` | LD beyond memory (second word) | `FAULT_LOAD` |
| `3231` | ST to an odd address | `FAULT_STORE_ADDR` |
| `3200` | ST into code | `FAULT_STORE_REGION` |
| `1240 3040` | ST below SP, outside data | `FAULT_STORE_REGION` |
| `0001`, `4001`, `9001`, `C200`, `8200` (one at a time) | Reserved bits | `FAULT_RESERVED` |
| `C001` | Jump to an odd address | `FAULT_TARGET` |
| `C030` | Jump outside code | `FAULT_TARGET` |
| `8001` | Call to an odd address | `FAULT_TARGET` |
| `4000` in all 24 code words | PC leaves code on step 25 | `FAULT_PC` |

- [ ] The same word `C030` does not fault on its own step when `code_end` is 0x40 (use a copy of the layout with
      `data_start`, `data_end` and `stack_limit` also 0x40).
- [ ] Writing R0 leaves it 0 (`1005` is `LDI R0, 5`).

**Check:** `make test` passes. Push. Tell Tanisha and Mokshit that the headers, `util.c` and CPU are ready. Ask a Mac teammate to run `make test`.

## Phase 4: main and integration (Sat Nov 28 to Tue Dec 1)

- [ ] `main.c` run loop: print the initial snapshot, then call `cpu_step` and `trace_print` until HLT, fault or `MAX_STEPS`.
      Count HLT as a step.
- [ ] Add the commands in SPEC section 11. Add each `#include` of a `.inc` file once its owner has pushed it.
      Until then, that command prints `not ready yet`.
- [ ] Compare arguments with `str_eq`. Copy or compare memory with loops.
- [ ] Merge teammates' branches after review. Report bugs to the file's owner instead of rewriting their code.

**Check (Tue Dec 1):** `pow` shows 19 steps and result 64; `fact` and `fact-hand` show 93 steps and result 40320;
`./cpu check` prints `CHECK PASSED`. If it does not, apply the backup rule in SPEC section 11.

## Phase 5: finish (Wed Dec 2 to Fri Dec 4)

- [ ] `make clean && make && make test` with zero warnings.
- [ ] `make symbols`: apart from compiler startup symbols, the only library calls are print functions
      (such as `printf`, `puts`, `putchar`; macOS adds a leading underscore). Investigate `memcpy`, `memset` or anything else.
- [ ] `README.md`, short: requirements, build (`make`, plus the one-line `gcc` command
      `gcc -std=c99 -fno-builtin -Iinclude -Iprograms src/*.c -o cpu`), each command with one line, folder layout.
      List only the platforms actually tested.
- [ ] Figures for the PDF: datapath (including the LDI immediate path into the registers) and instruction cycle.
      They must match the code.
- [ ] The datapath figure shows 256 bytes of memory, the 0x00 to 0x3F trace window, SP (R7) and FP (R6).
- [ ] PDF tables: registers, memory map, and build and run commands. Send them with the figures to Mokshit.
- [ ] Walkthrough with the team on Fri Dec 4.

## Be ready to explain

- How one instruction goes from 16 bits to fields to a change in a register or memory
- Why R0 is fixed at zero and why HLT is all zeros
- How C (carry and borrow) works, and why `JC` implements `i <= n`
- Big-endian order and sign extension
- What `CALL` and `RET` do to SP, memory and PC, and why every write to R7 is checked
- What each field of `Layout` controls, and how the CPU finds the heap top
- Teammates' work: how labels become addresses, how a C `for` loop becomes jumps, how a byte is printed in binary
