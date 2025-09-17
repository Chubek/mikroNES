LDA!byte
{
  CPU.ACC = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}
======
LDX!byte
{
  CPU.XR = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.XR);
}
======
LDY!byte
{
  CPU.YR = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.YR);
}
======
STA!word
{
  cpu_mem_write_byte (OPERAND.word, CPU.ACC);
}
======
STX!word
{
  cpu_mem_write_byte (OPERAND.word, CPU.XR);
}
======
STY!word
{
  cpu_mem_write_byte (OPERAND.word, CPU.YR);
}
======
AND!byte
{
  CPU.ACC &= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}
======
ORA!byte
{
  CPU.ACC |= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}
======
EOR!byte
{
  CPU.ACC ^= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}
======
ADC!byte
{
  if (cpu_flag_is_set ('D'))
    cpu_helper_adc_decimal (OPERAND.byte);
  else
    cpu_helper_adc_binary (OPERAND.byte);
}
======
SBC!byte
{
  if (cpu_flag_is_set ('D'))
    cpu_helper_sbc_decimal (OPERAND.byte);
  else
    cpu_helper_sbc_binary (OPERAND.byte);
}
======
ASL!byte
{
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  CPU.ACC = (OPERAND.byte << 1) & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}
======
LSR!byte
{
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_LSR) != 0);
  CPU.ACC = (OPERAND.byte >> 1) & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}
======
ROL!byte
{
  int old_flag = cpu_flag_is_set ('C') ? 1 : 0;
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  CPU.ACC = ((OPERAND.byte << 1) | old_flag) & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}
======
ROL!byte
{
  int old_flag = cpu_flag_is_set ('C') ? 1 : 0;
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_LSR) != 0);
  CPU.ACC = ((OPERAND.byte >> 1) | (old_flag << 7)) & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}
======
BIT!byte
{
  cpu_flag_set_if ('Z', ((CPU.ACC & OPERAND.byte) & MASK_BYTE) == 0);
  cpu_flag_set_if ('N', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  cpu_flag_set_if ('V', (OPERAND.byte & MASK_BIT) != 0);
}
======
INC!word
{
  uint8_t new = (cpu_mem_read_byte (OPERAND.word) + 1) & MASK_BYTE;
  cpu_mem_write_byte (OPERAND.word, new);
  cpu_flag_toggle_zn (new);
}
======
INC!word
{
  uint8_t new (cpu_mem_read_byte (OPERAND.word) - 1) & MASK_BYTE;
  cpu_mem_write_byte (OPERAND.word, new);
  cpu_flag_toggle_zn (new);
}
======
TAX!void
{
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
}
======
TAX!void
{
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
}
======
TAX!void
{
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
}
======
TAX!void
{
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
}
======
TXA!void
{
  CPU.ACC = CPU.XR;
  cpu_flag_toggle_zn (C.ACC);
}
======
TAY!void
{
  CPU.YR = CPU.ACC;
  cpu_flag_toggle_zn (C.YR);
}
======
TYA!void
{
  CPU.ACC = CPU.YR;
  cpu_flag_toggle_zn (C.ACC);
}
======
TSX!void
{
  CPU.XR = CPU.SP;
  cpu_flag_toggle_zn (C.XR);
}
======
TXS!void
{
  CPU.SP = CPU.XR;
}
======
INX!void
{
  CPU.XR++;
  cpu_flag_toggle_zn (C.XR);
}
======
INX!void
{
  CPU.XR++;
  cpu_flag_toggle_zn (C.XR);
}
======
INY!void
{
  CPU.YR++;
  cpu_flag_toggle_zn (C.YR);
}
======
DEX!void
{
  CPU.XR--;
  cpu_flag_toggle_zn (C.XR);
}
======
DEY!void
{
  CPU.YR--;
  cpu_flag_toggle_zn (C.YR);
}
======
CMP!byte
{
  cpu_helper_cmp (CPU.ACC, OPERAND.byte);
}
======
CPX!byte
{
  cpu_helper_cmp (CPU.XR, OPERAND.byte);
}
======
CPY!byte
{
  cpu_helper_cmp (CPU.YR, OPERAND.byte);
}
======
CLC!void
{
  cpu_flag_unset ('C');
}
======
SEC!void
{
  cpu_flag_set ('C');
}
======
CLD!void
{
  cpu_flag_unset ('D');
}
======
SED!void
{
  cpu_flag_set ('D');
}
======
CLI!void
{
  cpu_flag_unset ('I');
}
======
SEI!void
{
  cpu_flag_set ('I');
}
======
CLV!void
{
  cpu_flag_unset ('V');
}
======
NOP!void
{
  return;
}
======
PHA!void
{
  cpu_status_save_acc ();
}
======
PLA!void
{
  cpu_status_restore_acc ();
}
======
PHP!void
{
  cpu_flag_set ('X');
  cpu_flag_set ('B');
  cpu_status_save_status ();
}
======
PLP!void
{
   cpu_status_restore_flags ();
}
======
JSR!word
{
  uint8_t ret = (CPU.PC - 1) & MASK_SHORT;
  cpu_stack_push_short (ret);
  CPU.PC = OPERAND.word;
}
======
RTS!void
{
   cpu_status_restore_pc ();
}
======
RTI!void
{
  cpu_status_restore_flags ();
  cpu_status_restore_pc ();
}
======
BRK!void
{
  CPU.PC++;
  cpu_status_save_pc ();
  cpu_status_save_flags ();
  cpu_flag_set ('I');
  CPU.PC = cpu_mem_write_short (VECADDR_IRQ);
}
======
JMP!word
{
  CPU.PC = OPERAND.word & MASK_SHORT;
}

