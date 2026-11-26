# Handoff: Dhruv -> Tanisha -> Mokshit

State after Dhruv's six phases: `make clean && make && make test` has zero warnings and 190 checks pass (WSL Ubuntu, GCC 15).
The core (`include/*.h`, `util.c`, `memory.c`, `alu.c`, `cpu.c`) is frozen. Any change needs all three people and a re-test of both projects.

## Tanisha (next)

Replace these stubs (each is marked `/* STUB - owner: Tanisha */`): `src/assembler.c`, `src/disasm.c`, `src/trace.c`.
Add `programs/pow.inc` and `tests/test_asm.c`.

- In `src/main.c` replace `static const char *POW_ASM = 0;` with the `#include "pow.inc"` form shown in the comment above it.
- Add a `test_asm` build and run line to the `test` target in `Makefile` (link `util.c memory.c alu.c cpu.c assembler.c disasm.c`).
- Use `util.h` helpers only; no library calls except printing.
- Facts the trace needs: `trace_print(0, c, m, 0)` is the initial snapshot. After HLT the PC stays on the HLT, so NEXT PC shows the HLT's own address.
  `info.wrote` is 1 for ST and CALL; `info.waddr`, `info.before`, `info.after` describe the write. A faulting step is not traced.
- Check: `./cpu pow` prints 20 snapshots, `RESULT: 64   STEPS: 19`, and step 8 matches SPEC section 10.

## Mokshit (last)

Replace the stub `src/compiler.c`. Add `programs/fact.c`, `fact_c.inc`, `fact.inc`, `tests/test_compiler.c`, `tests/test_stack.c`, `tests/rec.inc`.

- In `src/main.c` replace the `FACT_C = 0` and `FACT_ASM = 0` lines with the `#include` forms shown in their comments.
- Add `test_compiler` and `test_stack` lines to the `test` target.
- `./cpu check` must print `CHECK PASSED` by end of Dec 1, else apply the backup rule in SPEC section 11.
- Then: `make traces`, final `README.md` read-through, PDF assembly. Figures and tables from Dhruv: `extras/figures/` and `extras/PDF_TABLES_DHRUV.md`.
- Remove `extras/` and add `output/` traces before zipping.
