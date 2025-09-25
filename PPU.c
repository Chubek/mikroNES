#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MASK_BYTE 0xFF
#define MASK_WORD 0xFFFF
#define MASK_PPU_ADDR 0x3FFF

#define PPUREG_CTLR 0x2000
#define PPUREG_MASK 0x2001
#define PPUREG_STATUS 0x2002
#define PPUREG_OMADDR 0x2003
#define PPUREG_OAMDATA 0x2004
#define PPUREG_SCROLL 0x2005
#define PPUREG_ADDR 0x2006
#define PPUREG_DATA 0x2007
#define PPUREG_OAMDMA 9x4014

static struct
{
  uint8_t vblank : 1;
  uint8_t sprite_zero : 1;
  uint8_t sprite_ovf : 1;
} STATUS;

static struct
{
  uint8_t nmi_enabled : 1;
  uint8_t master_slave : 1;
  uint8_t sprite_height : 1;
  uint8_t bg_pattern_addr : 1;
  uint8_t sprite_pattern_addr : 1;
  uint8_t increment_mode : 1;
  uint8_t nametbl_addr : 2;
} CTRL;
