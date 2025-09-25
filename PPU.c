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
#define PPUREG_OAMDMA 0x4014

#define SCANLINE_VISIBLE 240
#define SCANLINE_VBLANK_START 241
#define SCANLINE_FRAME_END 261
#define CYCLES_PER_SCANLINE 341
#define CYCLES_PER_FRAME (SCANLINE_FRAME_END * CYCLES_PER_SCANLINE)

static struct
{
  uint8_t vblank : 1;
  uint8_t sprite_zero : 1;
  uint8_t sprite_ovf : 1;
} STATUS;

static struct
{
  uint8_t nmi_enbl : 1;
  uint8_t master_slave : 1;
  uint8_t sprite_height : 1;
  uint8_t bg_pattern_addr : 1;
  uint8_t sprite_pattern_addr : 1;
  uint8_t increment_mode : 1;
  uint8_t nametbl_addr : 2;
} CTRL;

static struct
{
  uint8_t emph_blue : 1;
  uint8_t emph_green : 1;
  uint8_t emph_red : 1;
  uint8_t sprite_enbl : 1;
  uint8_t bg_enbl : 1;
  uint8_t sprite_lcol : 1;
  uint8_t bg_lcol : 1;
  uint8_t grayscale : 1;
} MASK;
