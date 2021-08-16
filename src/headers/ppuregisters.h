#ifndef H_PPU_REGISTERS
#define H_PPU_REGISTERS

#include "common.h"

typedef struct {
    u8 controller;  // $2000 w
    u8 mask;        // $2001 r
    u8 status;      // $2002 r
    u8 oam_address; // $2003 w
    u8 oam_data;    // $2004 rw
    u8 scroll;      // $2005 w
    u8 address;     // $2006 w
    u8 data;        // $2007 rw
} PPURegisters;

#endif