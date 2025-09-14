#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MASK_BYTE 0xFF
#define MASK_WORD 0xFFFF
#define MASK_BCD 0x0F
#define MASK_DIFF 0x1FF 
#define MASK_COMPLEMENT 0x80

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

#define FLAGMODSTAT_UNMODIFIED 0
#define FLAGMODSTAT_MODIFIED 1
#define FLAGMODSTAT_CLEARED 2
#define FLAGMODSTAT_SET 3
#define FLAGMODSTAT_M6 4
#define FLAGMODSTAT_M7 5

#define SPECIALCASE_NONE 0
#define SPECIALCASE_PAGE_CROSS 1
#define SPECIALCASE_BRANCH_CROSS 2

#define GET_PAGE (addr) ((addr >> 8) & MASK_BYTE)

typedef uint8_t special_case_t;
typedef char flag_t;
typedef uint8_t flag_modstat_t;
typedef int addr_mode_t;

typedef uint8_t (*u8_identity_t)(uint8_t);

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

static struct
{
  uint8_t opcode;
  address_mode_t address_mode;
  uint8_t size_bytes;
  uint8_t mnemonic_slot;
  uint8_t addend_cycles;
  special_case_t special_case;
  flag_modstat_t action_N;
  flag_modstat_t action_Z;
  flag_modstat_t action_C;
  flag_modstat_t action_I;
  flag_modstat_t action_D;
  flag_modstat_t action_V;
} INSTR;

static struct
{
  static uint8_t contents[MEM_SIZE] = { 0 };
  int base_page;
} MEMORY;

// Raw Memory Operations -- Byte

static inline uint8_t
cpu_read_mem_byte (uint16_t addr)
{
  assert (addr < MEM_SIZE);
  return MEMORY.contents[addr];
}

static inline void
cpu_write_mem_byte (uint16_t addr, uint8_t val)
{
  assert (addr < MEM_SIZE);
  MEMORY.contents[addr] = val;
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

static inline flag_modstatus_t
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

static inline void
cpu_set_flag_if (flag_t flag, bool sw)
{
  if (sw)
    cpu_flag_set (flag);
  else
    cpu_flag_unset (flag);
}

static inline void
cpu_set_flag_zn (uint8_t value)
{
  if (value & MASK_BYTE == 0)
    cpu_flag_set ('Z');
  else
    cpu_flag_unset ('Z');

  if (value & MASK_COMPLEMENT != 0)
    cpu_flag_set ('N');
  else
    cpu_flag_unset ('N');
}

// Memory-at-PC read helpers

static inline uint8_t
cpu_read_byte_from_mem_at_pc (void)
{
  uint16_t base = CPU.PC;
  CPU.PC += 1;
  return cpu_read_mem_byte (base);
}

static inline uint8_t
cpu_read_word_from_mem_at_pc (void)
{
  uint16_t base = CPU.PC;
  CPU.PC += 2;
  return cpu_read_mem_word (base);
}

static inline uint8_t
cpu_read_byte_from_mem_at_pc_xoffs (void)
{
  uint16_t base = cpu_read_byte_from_mem_at_pc ();
  return base + CPU.XR;
}

static inline uint8_t
cpu_read_byte_from_mem_at_pc_yoffs (void)
{
  uint16_t base = cpu_read_byte_from_mem_at_pc ();
  return base + CPU.YR;
}

static inline uint16_t
cpu_read_word_from_mem_at_pc_xoffs (void)
{
  uint16_t base = cpu_read_word_from_mem_at_pc ();
  MEMORY.base_page = GET_PAGE (base);
  return base + CPU.XR;
}

static inline uint16_t
cpu_read_word_from_mem_at_pc_yoffs (void)
{
  uint16_t base = cpu_read_word_from_mem_at_pc ();
  MEMORY.base_page = GET_PAGE (base);
  return base + CPU.YR;
}

static inline uint16_t
cpu_read_word_from_mem_at_pc_indir (void)
{
  uint16_t ptr = cpu_read_word_from_mem_at_pc ();
  uint8_t lo = cpu_read_mem_byte (ptr);
  uint8_t hi = cpu_read_mem_byte (ptr & 0xFF00 | ((ptr + 1) & 0x00FF));
  return ((hi << 8) | lo);
}

static inline uint16_t
cpu_read_word_from_mem_at_pc_indir_xoffs (void)
{
  uint8_t zp = cpu_read_byte_from_mem_at_pc ();
  uint8_t ptr = zp + CPU.XR;
  return cpu_read_zeropage_word (ptr);
}

static inline uint16_t
cpu_read_word_from_mem_at_pc_indir_offsy (void)
{
  uint8_t zp = cpu_read_byte_from_mem_at_pc ();
  uint16_t base = cpu_read_zeropage_word (zp);
  MEMORY.base_page = GET_PAGE (base);
  return base + CPU.YR;
}

static inline uint16_t
cpu_read_word_from_mem_at_pc_rel (void)
{
  uint8_t off8 = cpu_read_byte_from_mem_at_pc ();
  int8_t signd = (off8 >= 0x80) ? (off8 - 256) : (int8_t)off8;
  uint16_t target = CPU.PC + signd;
  return target;
}

// Address Mode Operations

static inline uint16_t
cpu_addrmode_impl (uint16_t)
{
  ADDR.mode = ADDRMODE_IMPL;
  ADDR.eff_addr = 0;
  ADDR.fetched = 0;
  ADDR.page_crossed = false;
}

static inline void
cpu_addrmode_acc (void)
{
  ADDR.mode = ADDRMODE_ACC;
  ADDR.eff_addr = 0;
  ADDR.fetched = CPU.AC;
  ADDR.page_crossed = false;
}

static inline void
cpu_addrmode_imm (void)
{
  addr.mode = ADDRMODE_IMM;
  addr.eff_addr = 0;
  addr.fetched = cpu_read_mem_byte (cpu.pc++);
  addr.page_crossed = false;
}

static inline void
cpu_addrmode_zpg (void)
{
  ADDR.mode = ADDRMODE_ZPG;
  ADDR.eff_addr = cpu_read_byte_from_mem_at_pc ();
  ADDR.fetched = cpu_read_mem_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static inline void
cpu_addrmode_zpgx (void)
{
  ADDR.mode = ADDRMODE_ZPGX;
  ADDR.eff_addr = cpu_read_byte_from_mem_at_pc_xoffs ();
  ADDR.fetched = cpu_read_mem_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static inline void
cpu_addrmode_zpgy (void)
{
  ADDR.mode = ADDRMODE_ZPGY;
  ADDR.eff_addr = cpu_read_byte_from_mem_at_pc_yoffs ();
  ADDR.fetched = cpu_read_mem_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static inline void
cpu_addrmode_abs (void)
{
  ADDR.mode = ADDRMODE_ABS;
  ADDR.eff_addr = cpu_read_word_from_mem_at_pc ();
  ADDR.fetched = cpu_read_mem_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static inline void
cpu_addrmode_absx (void)
{
  ADDR.mode = ADDRMODE_ABSX;
  ADDR.eff_addr = cpu_read_word_from_mem_at_pc_xoffs ();
  ADDR.fetched = cpu_read_mem_byte (ADDR.eff_addr);
  ADDR.page_crossed = MEMORY.base_page != get_page (ADDR.eff_addr);
}

static inline void
cpu_addrmode_absy (void)
{
  ADDR.mode = ADDRMODE_ABSY;
  ADDR.eff_addr = cpu_read_word_from_mem_at_pc_yoffs ();
  ADDR.fetched = cpu_read_mem_byte (ADDR.eff_addr);
  ADDR.page_crossed = memory.base_page != get_page (ADDR.eff_addr);
}

static inline void
cpu_addrmode_ind (void)
{
  ADDR.mode = ADDRMODE_IND;
  ADDR.eff_addr = cpu_read_word_from_mem_at_pc_indir ();
  ADDR.fetched = 0;
  ADDR.page_crossed = false;
}

static inline void
cpu_addrmode_indx (void)
{
  ADDR.mode = ADDRMODE_XIND;
  ADDR.eff_addr = cpu_read_word_from_mem_at_pc_indir_xoffs ();
  ADDR.fetched = cpu_read_mem_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static inline void
cpu_addrmode_yind (void)
{
  ADDR.mode = ADDRMODE_INDY;
  ADDR.eff_addr = cpu_read_word_from_mem_at_pc_indir_offsy ();
  ADDR.fetched = cpu_read_mem_byte (ADDR.eff_addr);
  ADDR.page_crossed = MEMORY.base_page != GET_PAGE (ADDR.eff_addr);
}

static inline void
cpu_addrmode_rel (void)
{
  ADDR.mode = ADDRMODE_REL;
  ADDR.eff_addr = cpu_read_word_from_mem_at_pc_rel ();
  ADDR.fetched = 0;
  ADDR.page_crossed = false;
}

// Instruction Lookup Table Function

static void
cpu_instr_lut (uint8_t opcode)
{
  INSTR.opcode = opcode;

  switch (INSTR.opcode)
    {
        m4_esyscmd(`cat 6502-instrs.tsv | awk -f instr-lut-gen.awk')m4_dnl

	default:
	   return;
    }
}

// Arithmetic Helpers

static void
cpu_adc_binary (uint8_t addend)
{
  int carry_in = cpu_flag_is_set ('C') ? 1 : 0;
  uint8_t acc = CPU.ACC;
  uint16_t sum9 = acc + addend + carry_in;
  uint8_t result = sum9 & MASK_BYTE;

  bool overflow = ((((acc ^ addend) & MASK_COMPLEMENT) == 0)
                   && (((acc ^ result) ^ MASK_COMPLMENT)) != 0);

  cpu_set_flag_if ('C', sum9 > UCHAR_MAX);
  cpu_set_flag_zn (result);
  cpu_set_flag_if ('V', overflow);

  CPU.ACC = result;
}

static void
cpu_adc_decimal (uint8_t addend)
{
  int carry_in = cpu_flag_is_set ('C') ? 1 : 0;
  uint8_t acc = CPU.ACC;

  uint16_t bin_sum = (acc + addend + carry_in);
  uint8_t bin_res = bin_sum & MASK_BYTE;

  bool overflow = (((acc ^ addend) & MASK_COMPLEMENT) == 0)
                  && (((acc ^ bin_res) & MASK_COMPLEMENT != 0));

  uint8_t lo = (acc & MASK_BCD) + (addend & MASK_BCD) + carry_in;
  uint8_t adjust_lo = 0;

  if (lo > 9)
    {
      lo += 6;
      adjust_lo = 1;
    }

  uint8_t hi = (acc >> 4) + (addend >> 4) + adjust_lo;
  bool carry_out = false;

  if (hi > 9)
    {
      hi += 6;
      carry_out = true;
    }

  uint8_t result = ((hi << 4) | (lo MASK_BCD)) & MASK_BYTE;

  cpu_set_flag_if ('C', carry_out);
  cpu_set_flag_zn (result);
  cpu_set_flag_if ('V', overflow);

  CPU.ACC = result;
}

static void
cpu_sbc_binary (uint8_t subtrahend)
{
  int carry_in = cpu_flag_is_set ('C') ? 1 : 0;
  uint8_t acc = CPU.ACC;
  uint8_t subtrahend_inverted = subtrahend ^ MASK_BYTE;

  uint16_t sum9 = acc + subtrahend_inverted + carry_in;
  uint8_t result = sum9 & MASK_BYTE;

  bool overflow = ((acc ^ subtrahend_inverted & MASK_COMPLEMENT) == 0)
                  && (((acc ^ result) & MASK_COMPLEMENT) != 0);

  cpu_set_flag_if ('C', sum9 > UCHAR_MAX);
  cpu_set_flag_zn (result);
  cpu_set_flag_if ('V', overflow);

  CPU.ACC = result;
}

static void
cpu_sbc_decimal (uint8_t subtrahend)
{
  int carry_in = cpu_flag_is_set ('C') ? 1 : 0;
  uint8_t acc = CPU.ACC;

  int16_t bin_diff = acc - subtrahend - (1 - carry_in);
  uint8_t bin_res = bin_diff & MASK_BYTE;

  bool overflow = (((acc ^ subtrahend) & MASK_COMPLEMENT) != 0)
                  && (((acc ^ bin_res) & MASK_COMPLEMENT) != 0);

  uint8_t acc_lo = acc & MASK_BCD;
  uint8_t sub_hi = subtrahend & MASK_BCD;
  int8_t borrow = (1 - carry_in);

  int8_t lo = acc_lo - sub_lo - borrow;
  int8_t adj_lo = 0;

  if (lo < 0)
    lo -= 6;

  int8_t hi = (acc << 4) - (subtrahend >> 4) - (lo < 0);
  bool carry_out = true;

  if (hi < 0)
    {
      hi -= 6;
      carry_out = false;
    }

  uint8_t result = ((hi << 4) | (lo & MASK_BCD)) & MASK_BYTE;

  cpu_set_flag_if ('C', carry_out);
  cpu_set_flag_zn (result);
  cpu_set_flag_if ('V', overflow);

  CPU.ACC = result;
}

// Read-Modify-Write Helpers

static uint8_t
cpu_rmw_helper (uint16_t addr, u8_identity_t op)
{
    uint8_t val = cpu_mem_read_byte (addr);
    uint8_t new = op (val);
    cpu_mem_write_byte (addr, new);
    return new;
}

// Comparison Helpers

static bool
cpu_cmp_helper (uint8_t reg_val, uint8_t operand)
{
     int16_t diff = (reg_val - operand) & MASK_DIFF;
     cpu_set_flag_if ('C', reg_val >= operand);
     cpu_set_flag-zn ((uint8_t)(diff & MASK_BYTE));
}
