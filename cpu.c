#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_dispatch.h"

#define MASK_BYTE 0xFF
#define MASK_WORD 0xFFFF

#define MEM_SIZE 0x10000
#define ZERO_PAGE_BEGIN 0x0
#define ZERO_PAGE_END 0xFF
#define STACK_START 0x0100
#define STACK_END 0x01FF
#define PAGE_SIZE 0xFF

#define NMI_VECTOR_ADDR 0xFFFA
#define RES_VECTOR_ADDR 0xFFFC
#define IRQ_VECTOR_ADDR 0xFFFE

#define FLAG_SET 1
#define FLAG_UNSET 0
#define FLAG_ERROR -1

#define ADDRMODE_ACC 0
#define ADDRMODE_ABS 1
#define ADDRMODE_ABSX 2
#define ADDRMODE_ABSY 3
#define ADDRMODE_IMM 4
#define ADDRMODE_IMPL 5
#define ADDRMODE_IND 6
#define ADDRMODE_XIND 8
#define ADDRMODE_INDY 9
#define ADDRMODE_REL 10
#define ADDRMODE_ZPG 11
#define ADDRMODE_ZPGX 12
#define ADDRMODE_ZPGY 13

#define CURRENT_PAGE (addr) ((addr >> 8) & MASK_BYTE)

typedef int flag_status_t;
typedef char flag_t;

typedef int addr_mode_t;

static struct
{
  uint8_t AC;
  uint8_t XR;
  uint8_t YR;
  uint8_t SP;
  uint16_t PC;

  uint64_t cycles;
  bool running;

  bool pending_nmi;
  bool pending_irq;
} CPU;

static struct
{
  uint8_t N : 1;
  uint8_t V : 1;
  uint8_t X : 1;
  uint8_t B : 1;
  uint8_t D : 1;
  uint8_t I : 1;
  uint8_t Z : 1;
  uint8_t C : 1;
} STATUS;

static struct
{
  addr_mode_t mode;
  uint16_t eff_addr;
  uint8_t fetched;
  bool page_crossed;
} ADDR;

static uint8_t MEMORY[MEM_SIZE] = { 0 };

// Raw Memory Operations -- Byte

static inline uint8_t
cpu_read_mem_byte (uint16_t addr)
{
  assert (addr < MEM_SIZE);
  return MEMORY[addr];
}

static inline void
cpu_write_mem_byte (uint16_t addr, uint8_t val)
{
  assert (addr < MEM_SIZE);
  MEMORY[addr] = val;
}

// Raw Memory Operations -- Word

static inline uint16_t
cpu_read_mem_word (uint16_t addr)
{
  assert (addr + 1 < MEM_SIZE);
  uint8_t lo = cpu_read_mem_byte (addr);
  uint8_t hi = cpu_read_mem_byte (addr + 1);
  return ((hi << 8) | lo);
}

static inline void
cpu_write_mem_word (uint16_t addr, uint16_t val)
{
  assert (addr + 1 < MEM_SIZE);
  uint8_t hi = (val >> 8) & MASK_BYTE;
  uint8_t lo = val & MASK_BYTE;
  cpu_write_mem_byte (addr, lo);
  cpu_write_mem_byte (addr + 1, hi);
}

// Zero Page Memory Operations -- Byte

static inline uint8_t
cpu_read_zeropage_byte (uint8_t zp_addr)
{
  assert (zp_addr >= ZERO_PAGE_START && zp_addr < ZERO_PAGE_END);
  return cpu_read_zeropage_byte ((uint16_t)zp_addr);
}

static inline void
cpu_write_zeropage_byte (uint8_t zp_addr, uint8_t val)
{
  assert (zp_addr >= ZERO_PAGE_START && zp_addr < ZERO_PAGE_END);
  cpu_write_mem_byte ((uint16_t)zp_addr, val);
}

// Zero Page Memory Operations -- Word

static inline uint8_t
cpu_read_zeropage_word (uint8_t zp_addr)
{
  assert (zp_addr >= ZERO_PAGE_START && zp_addr + 1 < ZERO_PAGE_END);
  uint8_t lo = cpu_read_zeropage_byte (zp_addr);
  uint8_t hi = cpu_read_zeropage_byte (zp_addr + 1);
  return ((hi << 8) | lo);
}

static inline void
cpu_write_zeropage_word (uint8_t zp_addr, uint16_t val)
{
  assert (zp_addr >= ZERO_PAGE_START && zp_addr + 1 < ZERO_PAGE_END);
  uint8_t hi = (val >> 8) & MASK_BYTE;
  uint8_t lo = val & MASK_BYTE;
  cpu_write_zeropage_byte (zp_addr, lo);
  cpu_write_zeropage_byte (zp_addr + 1, hi);
}

// Stack Operations -- Byte

static inline void
cpu_push_stack_byte (uint8_t val)
{
  assert (CPU.SP >= STACK_START && CPU.SP < STACK_END);
  cpu_write_mem_byte (CPU.SP--, val);
}

static inline uint8_t
cpu_pop_stack_byte (void)
{
  assert (CPU.SP >= STACK_START && CPU.SP < STACK_END);
  return cpu_read_mem_byte (++CP.SP);
}

// Stack Operations -- Word

static inline void
cpu_push_stack_word (uint8_t val)
{
  assert (CPU.SP >= STACK_START && CPU.SP < STACK_END);
  uint8_t hi = (val >> 8) && MASK_BYTE;
  uint8_t lo = val && MASK_BYTE;
  cpu_push_stack_byte (hi);
  cpu_push_stack_byte (lo);
}

static inline uint16_t
cpu_pop_stack_word (void)
{
  assert (CPU.SP >= STACK_START && CPU.SP < STACK_END);
  uint8_t lo = cpu_pop_stack_byte ();
  uint8_t hi = cpu_pop_stack_byte ();
  return ((hi << 8) | lo);
}

// Flag Operations

static inline flag_status_t
cpu_flag_toggle (flag_t flag)
{
  switch (flag)
    {
    case 'N':
      STATUS.N = !STATUS.N;
      return STATUS.N;
    case 'V':
      STATUS.V = !STATUS.V;
      return STATUS.V;
    case 'B':
      STATUS.B = !STATUS.B;
      return STATUS.B;
    case 'D':
      STATUS.D = !STATUS.D;
      return STATUS.D;
    case 'I':
      STATUS.I = !STATUS.I;
      return STATIS.I;
    case 'Z':
      STATUS.Z = !STATUS.Z;
      return STATUS.Z;
    case 'C':
      STATUS.C = !STATUS.C;
      return STATUS.C;
    default:
      return FLAG_ERROR;
    }

  return FLAG_ERROR;
}

static inline bool
cpu_flag_is_set (flag_t flag)
{
  switch (flag)
    {
    case 'N':
      return STATUS.N == FLAG_SET;
    case 'V':
      return STATUS.V == FLAG_SET;
    case 'B':
      return STATUS.B == FLAG_SET;
    case 'D':
      return STATUS.D == FLAG_SET;
    case 'I':
      return STATIS.I == FLAG_SET;
    case 'Z':
      return STATUS.Z == FLAG_SET;
    case 'C':
      return STATUS.C == FLAG_SET;
    default:
      return false;
    }

  return false;
}

static inline void
cpu_flag_set (flag_t flag)
{
  switch (flag)
    {
    case 'N':
      STATUS.N = FLAG_SET;
      return;
    case 'V':
      STATUS.V = FLAG_SET;
      return;
    case 'B':
      STATUS.B = FLAG_SET;
      return;
    case 'D':
      STATUS.D = FLAG_SET;
      return;
    case 'I':
      STATIS.I = FLAG_SET;
      return;
    case 'Z':
      STATUS.Z = FLAG_SET;
      return;
    case 'C':
      STATUS.C = FLAG_SET;
      return;
    default:
      return;
    }
}

static inline void
cpu_flag_unset (flag_t flag)
{
  switch (flag)
    {
    case 'N':
      STATUS.N = FLAG_UNSET;
      return;
    case 'V':
      STATUS.V = FLAG_UNSET;
      return;
    case 'B':
      STATUS.B = FLAG_UNSET;
      return;
    case 'D':
      STATUS.D = FLAG_UNSET;
      return;
    case 'I':
      STATIS.I = FLAG_UNSET;
      return;
    case 'Z':
      STATUS.Z = FLAG_UNSET;
      return;
    case 'C':
      STATUS.C = FLAG_UNSET;
      return;
    default:
      return;
    }
}

// Address Mode Operations
