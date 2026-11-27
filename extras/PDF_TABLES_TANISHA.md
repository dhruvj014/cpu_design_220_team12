# Tables for the PDF (Tanisha)

Mokshit assigns the final table and figure numbers when assembling the PDF. Captions below are ready to paste.
Every hex and binary value below was produced by `assembler.c` and read back by `disasm.c`.
Trace pages: `extras/trace_pages.py` (see the end of this file).

**Table: Instruction formats** (bit 15 on the left; reserved bits must be 0)

| Format | 15-12 | 11-9 | 8-6 | 5-3 | 2-0 | Instructions |
| --- | --- | --- | --- | --- | --- | --- |
| R | opcode | rd | rs1 | rs2 | 000 | ADD SUB MUL AND |
| C | opcode | 000 | rs1 | rs2 | 000 | CMP |
| I | opcode | rd (ST: source) | rs1 | imm6 (bits 5-0) | | LD ST ADDI |
| L | opcode | rd | imm9 (bits 8-0) | | | LDI |
| J | opcode | 000 | addr9 (bits 8-0) | | | CALL JMP JZ JNZ JC |
| N | opcode | 000 | 000 | 000 | 000 | HLT RET |

imm6 is 0 to 63 for LD and ST, and -32 to 31 (two's complement) for ADDI. imm9 is 0 to 511.

**Table: Instruction encoding** (one example of every opcode; bits grouped by the fields of its format)

| Op | Mnemonic | Format | Operation | Flags | Example | Hex | Binary |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | HLT | N | stop | - | `HLT` | 0000 | 0000 000000000000 |
| 1 | LDI | L | Rd = imm9 | - | `LDI R3, 1` | 1601 | 0001 011 000000001 |
| 2 | LD | I | Rd = mem16[Rs1 + imm6] | - | `LD R1, [R0+0x30]` | 2230 | 0010 001 000 110000 |
| 3 | ST | I | mem16[Rs1 + imm6] = Rd | - | `ST R3, [R0+0x34]` | 3634 | 0011 011 000 110100 |
| 4 | ADD | R | Rd = Rs1 + Rs2 | Z, C = carry | `ADD R6, R7, R0` | 4DC0 | 0100 110 111 000 000 |
| 5 | SUB | R | Rd = Rs1 - Rs2 | Z, C = borrow | `SUB R1, R2, R3` | 5298 | 0101 001 010 011 000 |
| 6 | MUL | R | Rd = low 16 bits of Rs1 * Rs2 | Z, C = overflow | `MUL R3, R3, R1` | 66C8 | 0110 011 011 001 000 |
| 7 | AND | R | Rd = Rs1 AND Rs2 | Z, C = 0 | `AND R1, R2, R3` | 7298 | 0111 001 010 011 000 |
| 8 | CALL | J | SP -= 2; mem16[SP] = next PC; PC = addr | - | `CALL 0x0C` | 800C | 1000 000 000001100 |
| 9 | RET | N | PC = mem16[SP]; SP += 2 | - | `RET` | 9000 | 1001 000000000000 |
| 10 | ADDI | I | Rd = Rs1 + sign-extended imm6 | Z, C = carry | `ADDI R2, R2, -1` | A4BF | 1010 010 010 111111 |
| 11 | CMP | C | Rs1 - Rs2, result dropped | Z, C = borrow | `CMP R2, R0` | B080 | 1011 000 010 000 000 |
| 12 | JMP | J | PC = addr | - | `JMP 0x08` | C008 | 1100 000 000001000 |
| 13 | JZ | J | if Z = 1: PC = addr | - | `JZ 0x14` | D014 | 1101 000 000010100 |
| 14 | JNZ | J | if Z = 0: PC = addr | - | `JNZ 0x08` | E008 | 1110 000 000001000 |
| 15 | JC | J | if C = 1: PC = addr | - | `JC 0x1C` | F01C | 1111 000 000011100 |

**Table: How one source line becomes 16 bits** (`ADDI R2, R2, -1` from `pow`)

| Step | Field | Value | Bits |
| --- | --- | --- | --- |
| 1 | Mnemonic `ADDI` gives the opcode | 10 | 1010 |
| 2 | `R2` is rd | 2 | 010 |
| 3 | `R2` is rs1 | 2 | 010 |
| 4 | `-1` is kept as 6-bit two's complement | -1 = 0x3F | 111111 |
| 5 | Join: `(10 << 12) \| (2 << 9) \| (2 << 6) \| 0x3F` | 0xA4BF | 1010 010 010 111111 |

**Table: Assembly language** (full rules: SPEC section 7)

| Item | Form | Example |
| --- | --- | --- |
| Line | `[label:] [instruction or directive] [; comment]` | `loop:   CMP  R2, R0   ; exponent == 0 ?` |
| Label | letter or `_`, then letters, digits, `_`; at most 15 characters, at most 16 labels | `done:` |
| Number | decimal or `0x` hex; `-` only for the ADDI immediate | `0x30`, `-1` |
| Memory operand | `[Rn+imm]`, the `+imm` is required | `[R0+0x34]` |
| Jump or call target | label or number; even and inside the code range | `JZ done` |
| `.org addr` | next byte goes to addr (0 to 255) | `.org 0x30` |
| `.word value` | one 16-bit word (0 to 65535) at an even address | `.word 8` |
| `.ascii "text"` | one byte per character, no escapes | `.ascii "POW 8^2 "` |

The assembler makes two passes. Pass 1 gives every label its address; pass 2 encodes and writes memory.
Two passes are needed because a forward jump (`JZ done`) is read before the line `done:` that defines its target.
It returns the number of the first failing line, for example an unknown mnemonic, `R8`, a missing comma,
`LDI R1, 512`, an odd or out-of-range target, an unknown or duplicate label, or an instruction at 0x30.

**Table: Reading a trace snapshot**

| Part | Meaning |
| --- | --- |
| `STEP 008` | Instructions run so far (HLT counts as a step); `STEP 000` is memory before the first instruction |
| `ADDR 0x0E` | Address of the instruction just run |
| `IR 0011011000110100   ST R3, [R0+0x34]` | Its 16 bits and its disassembly |
| `R0=0000 ... R7=0100` | All registers after the step, in hex |
| `NEXT PC=0x10   Z=0  C=0` | Next instruction address and the flags |
| `CHANGED: [0x34] 0x0001 -> 0x0008` | Word written by ST or CALL (old -> new), or `none` |
| `>` | Row holding the instruction just run |
| `*` | Row the write landed in (wins if it is also the `>` row) |
| Row `0x34   00000000 00001000 ...  ....   data` | 4 bytes in binary, the same bytes as ASCII (`.` if not printable), then the region |

The trace prints 0x00 to 0x3F only: that is all of code, data and tag. The stack (0x40 to 0xFF) is unused by `pow` and `fact`.

## Trace pages

After the code freeze: `make traces`, then from the project folder
```
python3 extras/trace_pages.py output/pow_trace.txt pow traces_pow.pdf
python3 extras/trace_pages.py output/fact_trace.txt fact traces_fact.pdf
```
Needs `pip install reportlab`. Two snapshots per US Letter page, 9 pt Courier, each captioned
`Snapshot <k> of <n> (pow): step <k - 1>, <instruction>`; the last page also shows the `RESULT` line.
`pow` gives 20 snapshots on 10 pages, `fact` 94 snapshots on 47 pages. Page numbers `##/##` are added by Mokshit.
