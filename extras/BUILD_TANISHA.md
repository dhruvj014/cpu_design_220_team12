# Build plan: Tanisha (assembler and output)

## Instructions for the coding agent

1. Read `SPEC.md` completely before writing code. It is the source of truth. If something is unclear, stop and ask.
2. Write only the files listed under "Your files". Do not edit headers or teammates' files; report problems instead.
3. Obey the library rule in SPEC section 1. Only `<stdio.h>` and print functions. Use the helpers in `util.h`.
4. Work one phase at a time. Build with zero warnings, run the checks, show real output, explain the code, then stop.
5. Never change an expected value to make a test pass. Report the mismatch.
6. Keep the code simple. Tanisha must be able to explain every line. Do not commit or push unless she asks.

## Your files

`src/assembler.c`, `src/disasm.c`, `src/trace.c`, `programs/pow.inc`, `tests/test_asm.c`, and your `Makefile` test lines.

Setup (macOS): `xcode-select --install`.
Start from Dhruv's skeleton and replace your stubs. `util.c` is due Thu Nov 26 and the CPU Fri Nov 27.

## Phase 1: program and assembler (Sat Nov 28 to Sun Nov 29)

- [ ] `pow.inc`: the listing, data and tag in SPEC section 9, in the `.inc` format.
- [ ] `assembler.c` following SPEC section 7. It takes its code bounds from the `Layout` argument; it has no fixed limits.
      It includes `CALL` (one target, like a jump) and `RET` (no operand). Suggested pieces: read one line into a fixed buffer; split off label,
      mnemonic and operands; pass 1 for labels and addresses; pass 2 to encode.
- [ ] `test_asm.c`:
  - Assembled `pow` matches all 11 hex words, the data and the tag byte for byte.
  - A test string holding the `fact` listing in SPEC section 9 matches all 17 hex words.
  - Each of these returns the correct line number: unknown mnemonic, `R8`, missing comma, `LDI R1, 512`,
    `ADDI R1, R1, 32`, offset 64, odd jump target, jump to 0x30, unknown label, duplicate label,
    instruction at 0x30, `.ascii` running past 0xFF, `OR R1, R2, R3`, `CALL` to an odd address, `RET R1`.
    Use `CPU_LAYOUT` for these.
  - With a copy of the layout whose `code_end`, `data_start`, `data_end` and `stack_limit` are 0x40,
    a jump to 0x30 and an instruction at 0x30 are both accepted.
  - `CALL 0x0C` assembles to `800C` and `RET` to `9000`.

**Check:** `make test` passes.

## Phase 2: disassembler and trace (Sun Nov 29 to Mon Nov 30)

- [ ] `disasm.c` (declared in `disasm.h`): the text forms in SPEC section 10, including `CALL` and `RET`; `???` for bad reserved bits.
- [ ] `trace.c`: match SPEC section 10. Print binary with a loop over the 8 bits. It prints 0x00 to `TRACE_END - 1` only,
      and marks `*` for any write (`ST` or `CALL`) inside that window. `trace_print` must not change anything.
- [ ] Test: every word in `pow` and `fact` disassembles to its source form (labels appear as addresses).

**Check (Mon Nov 30):** `./cpu pow` works end to end (Dhruv wires it up), and step 8 matches the sample in SPEC section 10.

## Phase 3: finish (Tue Dec 1 to Fri Dec 4)

- [ ] Fix integration issues. Build on a clean Mac checkout with zero warnings.
- [ ] After code freeze (Wed Dec 2), run `make traces`.
- [ ] PDF (Thu Dec 3): instruction format and encoding tables, and all trace pages (20 snapshots for `pow`,
      94 for `fact`), each captioned and readable. Send them to Mokshit.
- [ ] Zip (Fri Dec 4), from the folder above the project:
      `zip -r cpu-sim.zip cpu-sim -x "*.DS_Store" "*/.git/*" "*.o" "cpu-sim/cpu" "cpu-sim/test_*"`
- [ ] Unzip into an empty folder and follow only the README.

## Be ready to explain

- The two passes and why forward jumps need them
- How one source line becomes 16 bits
- Why the trace prints 0x00 to 0x3F although memory is 256 bytes
- How a byte is printed as binary and ASCII
- Your review of Dhruv's CPU code, and how the compiler's output is used
