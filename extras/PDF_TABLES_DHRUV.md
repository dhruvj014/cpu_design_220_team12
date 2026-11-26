# Tables for the PDF (Dhruv)

Mokshit assigns the final table and figure numbers when assembling the PDF. Captions below are ready to paste.
Figures: `figures/fig1_datapath.png` (SVG alongside) and `figures/fig2_cycle.png`.

**Figure: SJ16 datapath** (`fig1_datapath.png`)
**Figure: SJ16 instruction cycle** (`fig2_cycle.png`)

**Table: Registers and special state**

| Name | Size | Role |
| --- | --- | --- |
| R0 | 16 bits | Always 0; writes ignored |
| R1 | 16 bits | Return value; otherwise scratch |
| R2 to R5 | 16 bits | Scratch |
| R6 | 16 bits | Frame pointer (software convention only) |
| R7 | 16 bits | Stack pointer; every new value is checked |
| PC | 16 bits | Address of next instruction (even, inside code) |
| IR | 16 bits | Instruction just fetched |
| Z, C | 1 bit each | Zero flag; carry or borrow flag |

Reset: R0 to R6, IR, Z, C = 0. R7 = 0x0100. PC = 0x0000.

**Table: Memory map (`pow` and `fact`)**

| Address | Use | Shown in trace |
| --- | --- | --- |
| 0x00 to 0x2F | Code (24 instructions) | yes |
| 0x30 to 0x37 | Data words (variables, result) | yes |
| 0x38 to 0x3F | ASCII tag | yes |
| 0x40 to 0xFF | Stack, grows down from 0x100 | no |

256 bytes, big-endian 16-bit words at even addresses.

**Table: Build and run commands**

| Command | What it does |
| --- | --- |
| `make` | Build `./cpu` |
| `make test` | Build and run the unit tests |
| `make symbols` | List library functions the program calls |
| `make traces` | Save `output/pow_trace.txt` and `output/fact_trace.txt` |
| `make clean` | Remove build files |
| `./cpu pow` | Run 8^2 (assembly), full trace |
| `./cpu fact` | Show C, compile, show assembly, run 8!, full trace |
| `./cpu fact-hand` | Run the hand-written 8! |
| `./cpu check` | Compiled and hand-written images must match |

**Table: Faults** (a faulting instruction changes nothing)

| Fault text | Cause |
| --- | --- |
| PC outside code | PC odd or outside the code range |
| reserved bits set | A reserved instruction bit is 1 |
| bad load address | LD address odd or above 0xFE |
| bad store address | ST address odd or above 0xFE |
| store outside writable memory | ST outside data, heap or live stack |
| bad jump or call target | Taken jump or CALL target odd or outside code |
| bad return address | RET target odd or outside code |
| stack pointer is odd | New SP is odd |
| stack underflow | New SP above 0x0100 (RET on empty stack) |
| stack overflow | New SP below the stack limit |
| bad heap top, heap ran into stack, stack ran into heap | Heap rules (not used by `pow` and `fact`) |
