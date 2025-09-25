#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "6502-itc.h"

#define MEM_SIZE 0x10000
#define ZERO_PAGE_BEGIN 0x0
#define ZERO_PAGE_END 0xFF
#define STACK_START 0x0100
#define STACK_END 0x01FF
#define PAGE_SIZE 0xFF

#define MASK_BYTE 0xFF
#define MASK_WORD 0xFFFF
#define MASK_BCD 0x0F
#define MASK_DIFF 0x1FF
#define MASK_LSR 0x01
#define MASK_BIT 0x40
#define MASK_COMPLEMENT 0x80

#define VECADDR_NMI 0xFFFA
#define VECADDR_RESET 0xFFFC
#define VECADDR_IRQ 0xFFFE

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

#define BITFIELD_STRUTER (bf) (*((uint8_t *)&bf))
#define BITFIELD_DESTRUTER (bf, num) (*((uint8_t *)&bf) = num)

typedef uint8_t special_case_t;
typedef char flag_t;
typedef uint8_t flag_modstat;
typedef int addr_mode_t;

typedef uint8_t (*rmwutil_fn_t) (uint8_t);
typedef void (*itc_fn_t) (void);
typedef void (*resolver_fn_t) (void);
typedef uint16_t (*addrwrap_fn_t) (uint16_t);

static struct
{
  uint8_t AC;
  uint8_t XR;
  uint8_t YR;
  uint8_t SP;
  uint16_t PC;

  uint64_t total_cycles;
  bool running;

  bool pending_NMI;
  bool pending_RESET;
  bool pending_IRQ;
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
} FLAGS;

static struct
{
  addr_mode_t mode;
  uint16_t eff_addr;
  uint8_t fetched;
  bool page_crossed;
} ADDR;

static struct
{
  const char *mnemonic;
  uint8_t opcode;
  uint8_t size_bytes;
  uint8_t added_cycles;
  resolver_fn_t resolver_fn;
  itc_fn_t itc_fn;
  special_case_t special_case;
  flag_modstat action_N;
  flag_modstat action_Z;
  flag_modstat action_C;
  flag_modstat action_I;
  flag_modstat action_D;
  flag_modstat action_V;
} DISPATCH;

static struct
{
  uint8_t byte;
  uint16_t word;
} OPERAND;

static struct
{
  static uint8_t contents[MEM_SIZE] = { 0 };
  int base_page;
  addrwrap_fn_t wrapaddr_fn;
} MEMORY;

// Raw Memory Operations -- Byte

static inline uint8_t
cpu_mem_read_byte (uint16_t addr)
{
  addr = wrapadde_fn (addr);
  return MEMORY.contents[addr];
}

static inline void
cpu_mem_write_byte (uint16_t addr, uint8_t val)
{
  addr = wrapaddr_fn (addr);
  MEMORY.contents[addr] = val;
}

// Raw Memory Operations -- Word

static inline uint16_t
cpu_mem_read_word (uint16_t addr)
{
  uint8_t lo = cpu_mem_read_byte (addr);
  uint8_t hi = cpu_mem_read_byte (addr + 1);
  return ((hi << 8) | lo);
}

static inline void
cpu_mem_write_word (uint16_t addr, uint16_t val)
{
  uint8_t hi = (val >> 8) & MASK_BYTE;
  uint8_t lo = val & MASK_BYTE;
  cpu_mem_write_byte (addr, lo);
  cpu_mem_write_byte (addr + 1, hi);
}

// Zero Page Memory Operations -- Byte

static inline uint8_t
cpu_zpg_read_byte (uint8_t zp_addr)
{
  return cpu_zpg_read_byte ((uint16_t)zp_addr);
}

static inline void
cpu_zpg_write_byte (uint8_t zp_addr, uint8_t val)
{
  cpu_mem_write_byte ((uint16_t)zp_addr, val);
}

// Zero Page Memory Operations -- Word

static inline uint8_t
cpu_zpg_read_word (uint8_t zp_addr)
{
  uint8_t lo = cpu_zpg_read_byte (zp_addr);
  uint8_t hi = cpu_zpg_read_byte (zp_addr + 1);
  return ((hi << 8) | lo);
}

static inline void
cpu_zpg_write_word (uint8_t zp_addr, uint16_t val)
{
  uint8_t hi = (val >> 8) & MASK_BYTE;
  uint8_t lo = val & MASK_BYTE;
  cpu_zpg_write_byte (zp_addr, lo);
  cpu_zpg_write_byte (zp_addr + 1, hi);
}

// Stack Operations -- Byte

static inline void
cpu_stack_push_byte (uint8_t val)
{
  cpu_mem_write_byte (CPU.SP--, val);
}

static inline uint8_t
cpu_stack_pop_byte (void)
{
  return cpu_mem_read_byte (++CP.SP);
}

// Stack Operations -- Word

static inline void
cpu_stack_push_word (uint8_t val)
{
  uint8_t hi = (val >> 8) && MASK_BYTE;
  uint8_t lo = val && MASK_BYTE;
  cpu_stack_push_byte (hi);
  cpu_stack_push_byte (lo);
}

static inline uint16_t
cpu_stack_pop_word (void)
{
  uint8_t lo = cpu_stack_pop_byte ();
  uint8_t hi = cpu_stack_pop_byte ();
  return ((hi << 8) | lo);
}

// Flag Operations

static inline flag_modstatus_t
cpu_flag_toggle (flag_t flag)
{
  switch (flag)
    {
    case 'N':
      FLAGS.N = !FLAGS.N;
      return FLAGS.N;
    case 'V':
      FLAGS.V = !FLAGS.V;
      return FLAGS.V;
    case 'B':
      FLAGS.B = !FLAGS.B;
      return FLAGS.B;
    case 'D':
      FLAGS.D = !FLAGS.D;
      return FLAGS.D;
    case 'X':
      FLAGS.X = !FLAGS.X;
      return FLAGS.X;
    case 'I':
      FLAGS.I = !FLAGS.I;
      return STATIS.I;
    case 'Z':
      FLAGS.Z = !FLAGS.Z;
      return FLAGS.Z;
    case 'C':
      FLAGS.C = !FLAGS.C;
      return FLAGS.C;
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
      return FLAGS.N == FLAG_SET;
    case 'V':
      return FLAGS.V == FLAG_SET;
    case 'B':
      return FLAGS.B == FLAG_SET;
    case 'D':
      return FLAGS.D == FLAG_SET;
    case 'X':
      return FLAGS.X == FLAG_SET;
    case 'I':
      return STATIS.I == FLAG_SET;
    case 'Z':
      return FLAGS.Z == FLAG_SET;
    case 'C':
      return FLAGS.C == FLAG_SET;
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
      FLAGS.N = FLAG_SET;
      return;
    case 'V':
      FLAGS.V = FLAG_SET;
      return;
    case 'B':
      FLAGS.B = FLAG_SET;
      return;
    case 'D':
      FLAGS.D = FLAG_SET;
      return;
    case 'X':
      FLAGS.X = FLAG_SET;
      return;
    case 'I':
      STATIS.I = FLAG_SET;
      return;
    case 'Z':
      FLAGS.Z = FLAG_SET;
      return;
    case 'C':
      FLAGS.C = FLAG_SET;
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
      FLAGS.N = FLAG_UNSET;
      return;
    case 'V':
      FLAGS.V = FLAG_UNSET;
      return;
    case 'B':
      FLAGS.B = FLAG_UNSET;
      return;
    case 'D':
      FLAGS.D = FLAG_UNSET;
      return;
    case 'X':
      FLAGS.X = FLAG_UNSET;
      return;
    case 'I':
      STATIS.I = FLAG_UNSET;
      return;
    case 'Z':
      FLAGS.Z = FLAG_UNSET;
      return;
    case 'C':
      FLAGS.C = FLAG_UNSET;
      return;
    default:
      return;
    }
}

static inline void
cpu_flag_set_if (flag_t flag, bool cond)
{
  if (cond)
    cpu_flag_set (flag);
  else
    cpu_flag_unset (flag);
}

static inline void
cpu_flag_toggle_zn (uint8_t value)
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

// Address Resolvers

static inline uint8_t
cpu_resvladdr_byte_pc (void)
{
  uint16_t base = CPU.PC;
  CPU.PC += 1;
  MEMORY.base_page = GET_PAGE (base);
  return cpu_mem_read_byte (base);
}

static inline uint8_t
cpu_resvladdr_word_pc (void)
{
  uint16_t base = CPU.PC;
  CPU.PC += 2;
  MEMORY.base_page = GET_PAGE (base);
  return cpu_mem_read_word (base);
}

static inline uint8_t
cpu_resvladdr_byte_pc_xoffs (void)
{
  uint16_t base = cpu_resvladdr_byte_pc ();
  MEMORY.base_page = GET_PAGE (base);
  return base + CPU.XR;
}

static inline uint8_t
cpu_resvladdr_byte_pc_yoffs (void)
{
  uint16_t base = cpu_resvladdr_byte_pc ();
  MEMORY.base_page = GET_PAGE (base);
  return base + CPU.YR;
}

static inline uint16_t
cpu_resvladdr_word_pc_xoffs (void)
{
  uint16_t base = cpu_resvladdr_word_pc ();
  MEMORY.base_page = GET_PAGE (base);
  return base + CPU.XR;
}

static inline uint16_t
cpu_resvladdr_word_pc_yoffs (void)
{
  uint16_t base = cpu_resvladdr_word_pc ();
  MEMORY.base_page = GET_PAGE (base);
  return base + CPU.YR;
}

static inline uint16_t
cpu_resvladdr_word_pc_indir (void)
{
  uint16_t ptr = cpu_resvladdr_word_pc ();
  uint8_t lo = cpu_mem_read_byte (ptr);
  uint8_t hi = cpu_mem_read_byte (ptr & 0xFF00 | ((ptr + 1) & 0x00FF));
  return ((hi << 8) | lo);
}

static inline uint16_t
cpu_resvladdr_word_pc_indir_xoffs (void)
{
  uint8_t zp = cpu_resvladdr_byte_pc ();
  uint8_t ptr = zp + CPU.XR;
  return cpu_zpg_read_word (ptr);
}

static inline uint16_t
cpu_resvladdr_word_pc_indir_offsy (void)
{
  uint8_t zp = cpu_resvladdr_byte_pc ();
  uint16_t base = cpu_zpg_read_word (zp);
  MEMORY.base_page = GET_PAGE (base);
  return base + CPU.YR;
}

static inline uint16_t
cpu_resvladdr_word_pc_rel (void)
{
  uint8_t off8 = cpu_resvladdr_byte_pc ();
  int8_t signd = (off8 >= 0x80) ? (off8 - 256) : (int8_t)off8;
  uint16_t target = CPU.PC + signd;
  return target;
}

// CPU Status Save/Restore Helpers

static inline void
cpu_status_save_flags (void)
{
  cpu_stack_push_byte (BITFIELD_STRUTER (FLAGS));
}

static inline void
cpu_status_save_modified_flags (void)
{
  int old_b = FLAGS.B;
  int old_x = FLAGS.X;
  FLAGS.B = 1;
  FLAGS.X = 1;
  cpu_status_save_flags ();
  FLAGS.B = old_b;
  FLAGS.X = old_x;
}

static inline void
cpu_status_restore_flags (void)
{
  BITFIELD_DESTRUTER (FLAGS, cpu_stack_pop_byte ());
}

static inline void
cpu_status_save_pc (void)
{
  cpu_stack_push_word (CPU.PC);
}

static inline void
cpu_status_restore_pc (void)
{
  CPU.PC = cpu_stack_pop_word ();
}

static inline void
cpu_status_save_acc (void)
{
  cpu_stack_push_byte (PC.ACC);
}

static inline void
cpu_status_restore_acc (void)
{
  PC.ACC = cpu_stack_pop_byte ();
}

// Address Mode Operations

static void
cpu_addrmode_impl (void)
{
  ADDR.mode = ADDRMODE_IMPL;
  ADDR.eff_addr = 0;
  ADDR.fetched = 0;
  ADDR.page_crossed = false;
}

static void
cpu_addrmode_acc (void)
{
  ADDR.mode = ADDRMODE_ACC;
  ADDR.eff_addr = 0;
  ADDR.fetched = CPU.AC;
  ADDR.page_crossed = false;
}

static void
cpu_addrmode_imm (void)
{
  addr.mode = ADDRMODE_IMM;
  addr.eff_addr = 0;
  addr.fetched = cpu_mem_read_byte (CPU.PC++);
  addr.page_crossed = false;
}

static void
cpu_addrmode_zpg (void)
{
  ADDR.mode = ADDRMODE_ZPG;
  ADDR.eff_addr = cpu_resvladdr_byte_pc ();
  ADDR.fetched = cpu_mem_read_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static void
cpu_addrmode_zpgx (void)
{
  ADDR.mode = ADDRMODE_ZPGX;
  ADDR.eff_addr = cpu_resvladdr_byte_pc_xoffs ();
  ADDR.fetched = cpu_mem_read_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static void
cpu_addrmode_zpgy (void)
{
  ADDR.mode = ADDRMODE_ZPGY;
  ADDR.eff_addr = cpu_resvladdr_byte_pc_yoffs ();
  ADDR.fetched = cpu_mem_read_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static void
cpu_addrmode_abs (void)
{
  ADDR.mode = ADDRMODE_ABS;
  ADDR.eff_addr = cpu_resvladdr_word_pc ();
  ADDR.fetched = cpu_mem_read_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static void
cpu_addrmode_absx (void)
{
  ADDR.mode = ADDRMODE_ABSX;
  ADDR.eff_addr = cpu_resvladdr_word_pc_xoffs ();
  ADDR.fetched = cpu_mem_read_byte (ADDR.eff_addr);
  ADDR.page_crossed = MEMORY.base_page != get_page (ADDR.eff_addr);
}

static void
cpu_addrmode_absy (void)
{
  ADDR.mode = ADDRMODE_ABSY;
  ADDR.eff_addr = cpu_resvladdr_word_pc_yoffs ();
  ADDR.fetched = cpu_mem_read_byte (ADDR.eff_addr);
  ADDR.page_crossed = memory.base_page != get_page (ADDR.eff_addr);
}

static void
cpu_addrmode_ind (void)
{
  ADDR.mode = ADDRMODE_IND;
  ADDR.eff_addr = cpu_resvladdr_word_pc_indir ();
  ADDR.fetched = 0;
  ADDR.page_crossed = false;
}

static void
cpu_addrmode_indx (void)
{
  ADDR.mode = ADDRMODE_XIND;
  ADDR.eff_addr = cpu_resvladdr_word_pc_indir_xoffs ();
  ADDR.fetched = cpu_mem_read_byte (ADDR.eff_addr);
  ADDR.page_crossed = false;
}

static void
cpu_addrmode_yind (void)
{
  ADDR.mode = ADDRMODE_INDY;
  ADDR.eff_addr = cpu_resvladdr_word_pc_indir_offsy ();
  ADDR.fetched = cpu_mem_read_byte (ADDR.eff_addr);
  ADDR.page_crossed = MEMORY.base_page != GET_PAGE (ADDR.eff_addr);
}

static void
cpu_addrmode_rel (void)
{
  ADDR.mode = ADDRMODE_REL;
  ADDR.eff_addr = cpu_resvladdr_word_pc_rel ();
  ADDR.fetched = 0;
  ADDR.page_crossed = false;
}

// Arithmetic Helpers

static void
cpu_helper_adc_binary (uint8_t addend)
{
  int carry_in = cpu_flag_is_set ('C') ? 1 : 0;
  uint8_t acc = CPU.ACC;
  uint16_t sum9 = acc + addend + carry_in;
  uint8_t result = sum9 & MASK_BYTE;

  bool overflow = ((((acc ^ addend) & MASK_COMPLEMENT) == 0)
                   && (((acc ^ result) ^ MASK_COMPLMENT)) != 0);

  cpu_flag_set_if ('C', sum9 > UCHAR_MAX);
  cpu_flag_set_if ('V', overflow);
  cpu_flag_toggle_zn (result);

  CPU.ACC = result;
}

static void
cpu_helper_adc_decimal (uint8_t addend)
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

  cpu_flag_set_if ('C', carry_out);
  cpu_flag_set_if ('V', overflow);
  cpu_flag_toggle_zn (result);

  CPU.ACC = result;
}

static void
cpu_helper_sbc_binary (uint8_t subtrahend)
{
  int carry_in = cpu_flag_is_set ('C') ? 1 : 0;
  uint8_t acc = CPU.ACC;
  uint8_t subtrahend_inverted = subtrahend ^ MASK_BYTE;

  uint16_t sum9 = acc + subtrahend_inverted + carry_in;
  uint8_t result = sum9 & MASK_BYTE;

  bool overflow = ((acc ^ subtrahend_inverted & MASK_COMPLEMENT) == 0)
                  && (((acc ^ result) & MASK_COMPLEMENT) != 0);

  cpu_flag_set_if ('C', sum9 > UCHAR_MAX);
  cpu_flag_set_if ('V', overflow);
  cpu_flag_toggle_zn (result);

  CPU.ACC = result;
}

static void
cpu_helper_sbc_decimal (uint8_t subtrahend)
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

  cpu_flag_set_if ('C', carry_out);
  cpu_flag_set_if ('V', overflow);
  cpu_flag_toggle_zn (result);

  CPU.ACC = result;
}

// Execution Helpers

static uint8_t
cpu_helper_rmw (rmwutil_fn_t op)
{
  uint8_t val = cpu_mem_read_byte (ADDR.eff_addr);
  uint8_t new = op (val);
  cpu_mem_write_byte (ADDR.eff_addr, new);
  return new;
}

static bool
cpu_helper_cmp (uint8_t reg_val, uint8_t operand)
{
  int16_t diff = (reg_val - operand) & MASK_DIFF;
  cpu_flag_set_if ('C', reg_val >= operand);
  cpu_flag_toggle_zn ((uint8_t)(diff & MASK_BYTE));
}

static uint8_t
cpu_helper_branch (bool cond, uint16_t target)
{
  if (!cond)
    return 0;
  MEMORY.base_page = GET_PAGE (CPU.PC);
  CPU.PC = target;
  if (GET_PAGE (target) != MEMORY.base_page)
    return 2;
  else
    return 1;
}

// Interrupt Handlers

static void
cpu_handle_nmi (void)
{
  if (!CPU.pending_NMI)
    return;

  CPU.pending_NMI = false;
  cpu_state_save_pc ();
  cpu_state_save_flags ();
  cpu_flag_set ('I');
  CPU.PC = cpu_mem_read_word (VECADDR_NMI);
  CPU.total_cycles += 7;
}

static void
cpu_handle_reset (void)
{
  if (!CPU.pending_RESET)
    return;

  CPU.pending_RESET = false;
  cpu_flag_set ('I');
  CPU.PC = cpu_mem_read_word (VECADDR_RESET);
  CPU.total_cycles += 7;
  CPU.SP -= 3;
}

static void
cpu_handle_irq (void)
{
  if (!CPU.pending_IRQ)
    return;

  CPU.pending_IRQ = false;
  cpu_state_save_pc ();
  cpu_state_save_flags ();
  cpu_flag_set ('I');
  CPU.PC = cpu_mem_read_word (VECADDR_IRQ);
  CPU.total_cycles += 7;
}

// The Indirect Threaded Dispatch Table

static void
cpu_dispatch_table (uint8_t opcode)
{
  DISPATCH.opcode = opcode;

  switch (DISPATCH.opcode)
    {
        m4_esyscmd(`cat 6502-instrs.tsv | awk -f itc-dispatch-gen.awk')m4_dnl

	default:
	   return;
    }
}


