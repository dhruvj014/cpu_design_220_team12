# SJ16 CPU simulator (CMPE 220, Team 12)

A 16-bit CPU simulated in C. Runs `pow` (8^2, SJ16 assembly) and `fact` (8!, C compiled to SJ16 assembly)
and prints all of memory, in binary and ASCII, after every instruction.

Work in progress: build and run instructions are completed at the end of the project.

## Build and test

```
make          # builds ./cpu
make test     # builds and runs the unit tests
make clean
```
Needs `gcc` and `make` (tested on WSL Ubuntu). Only `<stdio.h>` printing functions are used.
