/* layout.h - Layout struct: where code, data, heap and stack live. Owner: Dhruv */
#ifndef LAYOUT_H
#define LAYOUT_H
#include "isa.h"
#define NO_ADDR 0xFFFF
/* Ranges are [start, end). Rules: SPEC section 3. */
typedef struct {
    u16 code_start, code_end;
    u16 data_start, data_end;
    u16 heap_start, heap_end;      /* equal means no heap */
    u16 heap_ptr_addr;             /* data word holding the heap top, or NO_ADDR */
    u16 stack_limit, stack_top;
    u16 entry;
} Layout;
#endif
