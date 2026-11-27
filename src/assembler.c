/* assembler.c - Two-pass SJ16 assembler: turns source text into a memory image. Owner: Tanisha

   A line is:  [label:] [instruction or directive] [; comment]      (rules: SPEC section 7)
   Pass 1 walks every line only to give each label its address.
   Pass 2 walks the lines again, now knowing every label, and writes words and bytes into memory.
   Both passes run the same line code; only pass 2 writes memory or rejects an unknown label.
   Forward jumps (JZ done before "done:") are why two passes are needed.
   Code bounds come from the Layout argument; there is no fixed memory map here.

   Parsing style: each read_ or expect_ helper takes a position in the line and returns the position
   just after what it read, or 0 on an error. A helper given 0 returns 0 at once, so a chain of them
   needs only one error check at the end. */
#include "assembler.h"
#include "util.h"

#define ASM_LINE_MAX 128   /* longest source line (without its comment), including the NUL */
#define LABEL_MAX    15    /* longest label or mnemonic */
#define MAX_LABELS   16
#define TOKEN_MAX    24    /* longest number token */

/* Mnemonics indexed by opcode, in the same order as Opcode in isa.h. */
static const char *const MNEMONIC[16] = {
    "HLT",  "LDI", "LD",   "ST",  "ADD", "SUB", "MUL", "AND",
    "CALL", "RET", "ADDI", "CMP", "JMP", "JZ",  "JNZ", "JC"
};

/* Everything one assembly run needs. */
typedef struct {
    Memory *m;
    const Layout *l;
    int pass;                                   /* 1: find labels, 2: write memory */
    long addr;                                  /* where the next word or byte goes */
    char label_name[MAX_LABELS][LABEL_MAX + 1];
    u16 label_addr[MAX_LABELS];
    int label_count;
} Asm;

/* ---------- small text readers ---------- */

/* Skips spaces and tabs. */
static const char *skip_spaces(const char *p)
{
    if (p == 0)
        return 0;
    while (is_space(*p))
        p++;
    return p;
}

/* Reads a name (a letter or _, then letters, digits or _) of at most LABEL_MAX characters into out. */
static const char *read_name(const char *p, char *out)
{
    int n = 0;
    if (p == 0 || !is_alpha(*p))
        return 0;
    while (is_alpha(*p) || is_digit(*p)) {
        if (n == LABEL_MAX)
            return 0;                           /* too long */
        out[n] = *p;
        n++;
        p++;
    }
    out[n] = '\0';
    return p;
}

/* Skips spaces, then requires the character ch (such as ',' or '['). */
static const char *expect_char(const char *p, char ch)
{
    p = skip_spaces(p);
    if (p == 0 || *p != ch)
        return 0;
    return p + 1;
}

/* Skips spaces, then reads a register R0 to R7 into *reg. */
static const char *read_reg(const char *p, int *reg)
{
    p = skip_spaces(p);
    if (p == 0 || p[0] != 'R' || p[1] < '0' || p[1] > '7')
        return 0;                               /* not R0-R7, e.g. R8 */
    if (is_alpha(p[2]) || is_digit(p[2]))
        return 0;                               /* longer word such as R10 or Rx */
    *reg = p[1] - '0';
    return p + 2;
}

/* Skips spaces, then reads a decimal or 0x hex number. A leading '-' is accepted only if allow_minus is 1. */
static const char *read_number(const char *p, long *value, int allow_minus)
{
    char token[TOKEN_MAX + 1];
    int n = 0;
    p = skip_spaces(p);
    if (p == 0 || (*p == '-' && !allow_minus))
        return 0;
    while (*p == '-' || is_alpha(*p) || is_digit(*p)) {   /* copy the token, e.g. "0x30" or "-1" */
        if (n == TOKEN_MAX)
            return 0;
        token[n] = *p;
        n++;
        p++;
    }
    token[n] = '\0';
    if (parse_number(token, value) != 0)
        return 0;                               /* malformed or above 65535 */
    return p;
}

/* Returns 1 if only spaces are left on the line. */
static int at_end(const char *p)
{
    p = skip_spaces(p);
    return p != 0 && *p == '\0';
}

/* ---------- labels ---------- */

/* Returns the index of a label, or -1 if it is not defined. */
static int find_label(const Asm *a, const char *name)
{
    int i;
    for (i = 0; i < a->label_count; i++)
        if (str_eq(a->label_name[i], name))
            return i;
    return -1;
}

/* Pass 1: records a label at the current address. 0 ok, -1 if duplicate or too many labels. */
static int define_label(Asm *a, const char *name)
{
    int i;
    if (find_label(a, name) >= 0 || a->label_count == MAX_LABELS)
        return -1;
    for (i = 0; name[i] != '\0'; i++)           /* copy the name by hand (no strcpy) */
        a->label_name[a->label_count][i] = name[i];
    a->label_name[a->label_count][i] = '\0';
    a->label_addr[a->label_count] = (u16)a->addr;
    a->label_count++;
    return 0;
}

/* ---------- instructions ---------- */

/* Returns the opcode for a mnemonic, or -1 if it is unknown. */
static int find_mnemonic(const char *name)
{
    int op;
    for (op = 0; op < 16; op++)
        if (str_eq(MNEMONIC[op], name))
            return op;
    return -1;
}

/* Returns 1 if t is a valid jump or call target: even and a whole word inside the layout's code range. */
static int valid_target(const Asm *a, long t)
{
    return (t & 1) == 0 && t >= a->l->code_start && t + 2 <= a->l->code_end;
}

/* Reads a jump or call target: a label or a number. In pass 1 a label may not be defined yet;
   it then counts as 0 and is not checked (pass 2 checks it). */
static const char *read_target(const Asm *a, const char *p, long *target)
{
    char name[LABEL_MAX + 1];
    int i;
    p = skip_spaces(p);
    if (p == 0)
        return 0;
    if (is_alpha(*p)) {                         /* a label */
        p = read_name(p, name);
        if (p == 0)
            return 0;
        i = find_label(a, name);
        if (i < 0) {
            if (a->pass == 2)
                return 0;                       /* unknown label */
            *target = 0;
            return p;                           /* pass 1: forward label, value comes later */
        }
        *target = a->label_addr[i];
    } else {                                    /* a number */
        p = read_number(p, target, 0);
        if (p == 0)
            return 0;
    }
    if (!valid_target(a, *target))
        return 0;
    return p;
}

/* Reads the operands of instruction op and builds its 16-bit word in *word (formats: SPEC section 4). */
static const char *encode(const Asm *a, int op, const char *p, u16 *word)
{
    int rd = 0, rs1 = 0, rs2 = 0;
    long n = 0;
    unsigned w = (unsigned)op << 12;            /* opcode goes in bits 15-12 */

    switch (op) {
    case OP_HLT:                                /* N format: no operands */
    case OP_RET:
        break;
    case OP_LDI:                                /* L format: LDI Rd, imm9 */
        p = read_reg(p, &rd);
        p = expect_char(p, ',');
        p = read_number(p, &n, 0);
        if (p == 0 || n > 511)
            return 0;
        w |= (unsigned)(rd << 9) | (unsigned)n;
        break;
    case OP_LD:                                 /* I format: LD Rd, [Rs1+imm6] (ST: Rd is the source) */
    case OP_ST:
        p = read_reg(p, &rd);
        p = expect_char(p, ',');
        p = expect_char(p, '[');
        p = read_reg(p, &rs1);
        p = expect_char(p, '+');
        p = read_number(p, &n, 0);
        p = expect_char(p, ']');
        if (p == 0 || n > 63)
            return 0;
        w |= (unsigned)(rd << 9) | (unsigned)(rs1 << 6) | (unsigned)n;
        break;
    case OP_ADD:                                /* R format: OP Rd, Rs1, Rs2 */
    case OP_SUB:
    case OP_MUL:
    case OP_AND:
        p = read_reg(p, &rd);
        p = expect_char(p, ',');
        p = read_reg(p, &rs1);
        p = expect_char(p, ',');
        p = read_reg(p, &rs2);
        if (p == 0)
            return 0;
        w |= (unsigned)(rd << 9) | (unsigned)(rs1 << 6) | (unsigned)(rs2 << 3);
        break;
    case OP_ADDI:                               /* I format: ADDI Rd, Rs1, simm6 (-32 to 31) */
        p = read_reg(p, &rd);
        p = expect_char(p, ',');
        p = read_reg(p, &rs1);
        p = expect_char(p, ',');
        p = read_number(p, &n, 1);
        if (p == 0 || n < -32 || n > 31)
            return 0;
        w |= (unsigned)(rd << 9) | (unsigned)(rs1 << 6) | (unsigned)(n & 0x3F);   /* keep 6 bits (two's complement) */
        break;
    case OP_CMP:                                /* C format: CMP Rs1, Rs2 */
        p = read_reg(p, &rs1);
        p = expect_char(p, ',');
        p = read_reg(p, &rs2);
        if (p == 0)
            return 0;
        w |= (unsigned)(rs1 << 6) | (unsigned)(rs2 << 3);
        break;
    default:                                    /* J format: CALL JMP JZ JNZ JC addr */
        p = read_target(a, p, &n);
        if (p == 0)
            return 0;
        w |= (unsigned)(n & 0x1FF);
        break;
    }
    *word = (u16)w;
    return p;
}

/* ---------- directives ---------- */

/* Handles .org, .word and .ascii. p points just after the '.'. */
static const char *directive(Asm *a, const char *p)
{
    char name[LABEL_MAX + 1];
    long n = 0;

    p = read_name(p, name);
    if (p == 0)
        return 0;
    if (str_eq(name, "org")) {                  /* .org addr: move to addr (0 to 255) */
        p = read_number(p, &n, 0);
        if (p == 0 || n > 255)
            return 0;
        a->addr = n;
        return p;
    }
    if (str_eq(name, "word")) {                 /* .word value: one big-endian word at an even address */
        p = read_number(p, &n, 0);
        if (p == 0 || (a->addr & 1) != 0 || a->addr + 2 > MEM_SIZE)
            return 0;
        if (a->pass == 2)
            mem_write16(a->m, (u16)a->addr, (u16)n);
        a->addr += 2;
        return p;
    }
    if (str_eq(name, "ascii")) {                /* .ascii "text": one byte per character, no escapes */
        p = expect_char(p, '"');
        if (p == 0)
            return 0;
        while (*p != '"') {
            if (*p == '\0' || a->addr >= MEM_SIZE)
                return 0;                       /* no closing quote, or text runs past 0xFF */
            if (a->pass == 2)
                a->m->byte[a->addr] = (u8)*p;
            a->addr++;
            p++;
        }
        return p + 1;                           /* step over the closing quote */
    }
    return 0;                                   /* unknown directive */
}

/* ---------- lines ---------- */

/* Copies the line starting at src[pos] into buf, without its comment or newline.
   A ';' inside the quotes of .ascii is text, not a comment.
   Returns the position of the next line, or -1 if the line is too long. */
static int read_line(const char *src, int pos, char *buf)
{
    int n = 0;
    int in_quotes = 0, in_comment = 0, too_long = 0;

    while (src[pos] != '\0' && src[pos] != '\n') {
        char ch = src[pos];
        pos++;
        if (in_comment)
            continue;                           /* drop the rest of the line */
        if (ch == '"')
            in_quotes = !in_quotes;
        if (ch == ';' && !in_quotes) {
            in_comment = 1;
            continue;
        }
        if (n == ASM_LINE_MAX - 1)
            too_long = 1;
        else {
            buf[n] = ch;
            n++;
        }
    }
    buf[n] = '\0';
    if (src[pos] == '\n')
        pos++;
    return too_long ? -1 : pos;
}

/* Assembles one line (comment already removed). Returns 0 if ok, -1 on any error. */
static int do_line(Asm *a, const char *p)
{
    char name[LABEL_MAX + 1];
    const char *after;
    int op;
    u16 word = 0;

    p = skip_spaces(p);
    if (is_alpha(*p)) {                         /* a word followed by ':' is a label */
        after = skip_spaces(read_name(p, name));
        if (after == 0)
            return -1;                          /* name too long */
        if (*after == ':') {
            if (a->pass == 1 && define_label(a, name) != 0)
                return -1;                      /* duplicate or too many labels */
            p = skip_spaces(after + 1);
        }
    }
    if (*p == '\0')
        return 0;                               /* empty line, or a label on its own */
    if (*p == '.') {                            /* directive */
        p = directive(a, p + 1);
        return at_end(p) ? 0 : -1;
    }

    p = read_name(p, name);                     /* mnemonic */
    if (p == 0)
        return -1;
    op = find_mnemonic(name);
    if (op < 0)
        return -1;                              /* unknown mnemonic, e.g. OR */
    if ((a->addr & 1) != 0 || a->addr < a->l->code_start || a->addr + 2 > a->l->code_end)
        return -1;                              /* instruction outside the code range */
    p = encode(a, op, p, &word);
    if (!at_end(p))
        return -1;                              /* bad operands, or extra text such as RET R1 */
    if (a->pass == 2)
        mem_write16(a->m, (u16)a->addr, word);
    a->addr += 2;
    return 0;
}

/* Clears memory, then assembles source into it. Returns 0 if ok, else the first failing line number (from 1). */
int assemble(const char *source, Memory *m, const Layout *l)
{
    Asm a;
    char buf[ASM_LINE_MAX];
    int pass, pos, line;

    mem_clear(m);
    a.m = m;
    a.l = l;
    a.label_count = 0;
    for (pass = 1; pass <= 2; pass++) {
        a.pass = pass;
        a.addr = l->code_start;                 /* until the first .org */
        pos = 0;
        line = 0;
        while (source[pos] != '\0') {
            line++;
            pos = read_line(source, pos, buf);
            if (pos < 0 || do_line(&a, buf) != 0)
                return line;                    /* stop at the first error */
        }
    }
    return 0;
}
