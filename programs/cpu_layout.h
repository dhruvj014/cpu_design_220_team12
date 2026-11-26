/* cpu_layout.h - Memory map used by the pow and fact programs. Owner: Dhruv */
#ifndef CPU_LAYOUT_H
#define CPU_LAYOUT_H
#include "layout.h"
static const Layout CPU_LAYOUT = {
    0x00, 0x30,       /* code  */
    0x30, 0x40,       /* data  */
    0x00, 0x00,       /* no heap */
    NO_ADDR,
    0x40, 0x100,      /* stack limit, stack top */
    0x00              /* entry */
};
#endif
