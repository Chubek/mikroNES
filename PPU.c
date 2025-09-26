#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MASK_BYTE 0xFF
#define MASK_WORD 0xFFFF
#define MASK_PPU_ADDR 0x3FFF
#define MASK_NMTBL_BASE 0x0FFF
#define MASK_NMTBL_MIRROR 0x03FF

#define PPUREG_CTRL 0x2000
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

#define VRAM_SIZE 0xFFFF
#define OAM_SIZE 0xFF
#define PALETTE_SIZE 32
#define PRIMARY_BUFFER_SIZE 64
#define SECONDARY_BUFFER_SIZE 8

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

static struct
{
  uint8_t oam_addr;
  uint8_t scroll_x;
  uint8_t scroll_y;
  uint8_t data_buffer;
  uint16_t addr;
  uint8_t fine_x;
  bool write_latch;
  bool scroll_latch;
  bool addr_latch;
  uint16_t curr_scanline;
  uint16_t curr_cycle;
  size_t frame_count;
  bool even_frame;
} PPU;

static struct
{
  bool rendering_enbl;
  bool vblank_started;
  bool vblank_supprsd;
} PCONFIG;

static struct
{
  uint8_t vram[VRAM_SIZE];
  uint8_t oam[OAM_SIZE];
  uint8_t palette[PALETTE_SIZE];
} PMEMORY;

static struct
{
  uint8_t x, y;
  uint8_t tile_idx;
  uint8_t attrs;
} PRIMARY_BUFFER[PRIMARY_BUFFER_SIZE];

static struct
{
  uint8_t x, y;
  uint8_t tile_idx;
  uint8_t attrs;
  uint8_t patt_lo, patt_hi;
} SECONDARY_BUFFER[SECONDARY_BUFFER_SIZE];

// Nametable Functions

static inline uint8_t
ppu_nmtbl_get_idx (uint16_t addr)
{
  return (addr & MASK_NMTBL_BASE) >> 10;
}

staitc inline uint16_t
ppu_nmtbl_get_mirror (uint16_t addr)
{
  uint16_t base = addr & MASK_NMTBL_BASE;
  uint8_t nt_idx = base >> 10;

  switch (nt_idx)
    {
    case 1:
      return (base & MASK_NMTBL_MIRROR) | 0x2000;
    case 2:
      return (base & MASK_NMTBL_MIRROR) | 0x2800;
    case 3:
      return (base & MASK_NMTBL_MIRROR) | 0x2800;
    default:
      return addr;
    }
}
