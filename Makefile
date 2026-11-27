# Makefile - Build, test and trace targets for the SJ16 simulator. Owner: Dhruv
# Usage: make | make test | make symbols | make traces | make clean

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -fno-builtin -Iinclude -Iprograms

# All program sources; every .c file in src/ becomes part of ./cpu
SRCS    = $(wildcard src/*.c)
OBJS    = $(SRCS:.c=.o)

# Files the unit tests link with (the core plus util; only files that exist yet)
CORE    = $(wildcard src/util.c src/memory.c src/alu.c src/cpu.c)

all: cpu

cpu: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o cpu

src/%.o: src/%.c $(wildcard include/*.h programs/*.h)
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run every unit test. Teammates add test_compiler, test_stack here.
test:
	$(CC) $(CFLAGS) tests/test_core.c $(CORE) -o test_core
	./test_core
	$(CC) $(CFLAGS) tests/test_asm.c $(CORE) src/assembler.c src/disasm.c -o test_asm
	./test_asm

# Show which library functions the program and the core objects call
symbols:
	$(CC) $(CFLAGS) -fno-stack-protector $(SRCS) -o cpu
	nm -u cpu
	@echo "--- core objects (only names the core itself defines may appear) ---"
	for f in memory alu cpu; do $(CC) $(CFLAGS) -fno-stack-protector -c src/$$f.c -o src/$$f.o; done
	nm -u src/memory.o src/alu.o src/cpu.o

# Save the full traces of pow and fact for the submission
traces: cpu
	mkdir -p output
	./cpu pow > output/pow_trace.txt
	./cpu fact > output/fact_trace.txt

clean:
	rm -f cpu test_core test_asm test_compiler test_stack src/*.o

.PHONY: all test symbols traces clean
