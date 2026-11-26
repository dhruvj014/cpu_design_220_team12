# Build plan: Mokshit (C compiler and C program)

## Instructions for the coding agent

1. Read `SPEC.md` completely before writing code. It is the source of truth. If something is unclear, stop and ask.
2. Write only the files listed under "Your files". Do not edit headers or teammates' files; report problems instead.
3. Obey the library rule in SPEC section 1. Only `<stdio.h>` and print functions. No `sprintf`: write small helpers
   that append text and numbers to the output buffer and check its capacity.
4. Work one phase at a time. Build with zero warnings, run the checks, show real output, explain the code, then stop.
5. Never change an expected value to make a test pass. Report the mismatch.
6. Support only the C subset in SPEC section 8. Do not add features. Mokshit must be able to explain every line.
   Do not commit or push unless he asks.

## Your files

`src/compiler.c`, `programs/fact.c`, `programs/fact_c.inc`, `programs/fact.inc`, `tests/test_compiler.c`,
`tests/test_stack.c`, `tests/rec.inc`, and your `Makefile` test lines.

Setup (macOS): `xcode-select --install`.
Start from Dhruv's skeleton and replace your stub. `util.c` is due Thu Nov 26.

## Phase 1: C program and backup (Sat Nov 28)

- [ ] `fact.c`: the function in SPEC section 9 plus a `main` that prints its result. It must print 40320.
- [ ] `fact_c.inc`: the same function as string lines, without `main`.
- [ ] `fact.inc`: the hand translation in SPEC section 9, with the matching C line as a comment on each group,
      plus data and tag. This is the backup, so push it first.

**Check:** compile and run `fact.c` natively.

## Phase 1b: call and stack tests (Sat Nov 28, after the backup is pushed)

These tests load hex words straight into memory, so they need only the core.
`CPU_FAULT` must carry the listed text, and the faulting instruction must change nothing.

- [ ] `rec.inc`: the `rec` listing in SPEC section 9, in the `.inc` format.
- [ ] `test_stack.c`: run the `rec` hex words under its test layout. Check every row of the table in SPEC section 9,
      including SP after each `CALL` and `RET` and the stack contents table.
- [ ] Faults with `CPU_LAYOUT` (words from 0x00):

| Words | Case | `info.fault` |
| --- | --- | --- |
| `8000` | CALL to itself fills the stack; faults on step 97 with SP = 0x40 | `FAULT_OVERFLOW` |
| `1E3E` | `LDI R7, 0x3E` | `FAULT_OVERFLOW` |
| `1EFF` | `LDI R7, 255` | `FAULT_SP_ODD` |
| `9000` | RET with an empty stack | `FAULT_UNDERFLOW` |
| `AFC2` | SP raised above `stack_top` | `FAULT_UNDERFLOW` |
| `AFFE 1201 33C0 9000` | RET to an odd address, step 4 | `FAULT_RETURN` |

- [ ] `AFFE 31C0 0000` (store into the live stack) halts after 3 steps.
- [ ] Heap layout: code 0x00 to 0x30, data 0x30 to 0x40, heap 0x40 to 0x100, heap pointer at 0x30,
      stack limit 0x40, stack top 0x100, entry 0. Before each case, write the starting heap top to 0x30.

| Start heap top | Words | Result |
| --- | --- | --- |
| 0x00F0 | `AFFE` nine times | `FAULT_STACK_HEAP` on step 9, SP = 0xF0 |
| 0x0040 | `AFFE 1300 3230` | `FAULT_HEAP_STACK` on step 3 |
| 0x0040 | `123E 3230` | `FAULT_HEAP_RANGE` on step 2 |
| 0x0040 | `1280 3040` | `FAULT_STORE_REGION` on step 2 (unallocated heap) |
| 0x0040 | `124A 3230 1440 3288 0000` | halts after 5 steps (allocate, then store) |

- [ ] Report any failure to Dhruv. Once all pass, tell the team the core is ready to be reused.
- [ ] Once Tanisha's assembler is merged: assembling `rec.inc` under the test layout gives the same hex words.

## Phase 2: compiler (Sun Nov 29 to Tue Dec 1)

- [ ] Tokenizer: names, keywords, decimal numbers, `( ) { } , ; = + - * <= ++`. Track the line number.
- [ ] One parsing function per grammar rule in SPEC section 8.
- [ ] Symbol table: up to 3 names with their addresses and initial values.
- [ ] Code generation in exactly the order in SPEC section 8.
- [ ] `test_compiler.c`:
  - Compiling the `fact` text returns 0.
  - Each of these returns the correct line number: undeclared name, duplicate name, four variables,
    `while` instead of `for`, `/` operator, missing semicolon, loop start 512, a buffer capacity of 10.
  - Once Tanisha's assembler is merged: the compiled memory image equals the `fact.inc` image in all 256 bytes.
  - Changing `n = 8` to `n = 5` and running it gives 120.

**Check (end of Tue Dec 1):** `./cpu check` prints `CHECK PASSED`. If not, tell the team and apply the backup rule
in SPEC section 11.

## Phase 3: finish (Wed Dec 2 to Fri Dec 4)

- [ ] Wed Dec 2: confirm the C program, compiler output and hand translation agree.
- [ ] Thu Dec 3: listings (`pow` assembly; `fact` C beside the generated assembly), title page, then assemble the PDF
      with everyone's pages. Number pages `##/##` and number all captions.
- [ ] Contributions page: each person writes their own two to three sentences naming files and functions,
      describing what they actually did.
- [ ] Fri Dec 4: check the PDF against SPEC section 13 and the rubric, then join the walkthrough.

## Be ready to explain

- How text becomes tokens, then assembly
- How the `for` loop becomes `loop:`, `CMP`, `JC` and `JMP`
- What C the compiler rejects, and why the subset is small (it does not compile recursion)
- How a recursive call builds and removes a stack frame, and what each stack fault means
- Your review of Tanisha's assembler, and how the CPU runs the result
