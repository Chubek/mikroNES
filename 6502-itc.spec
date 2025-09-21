LDA:
  CPU.ACC = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
======
LDX:
  CPU.XR = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.XR);
======
LDY:
  CPU.YR = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.YR);
======
STA:
  cpu_mem_write_byte (OPERAND.word, CPU.ACC);
======
STX:
  cpu_mem_write_byte (OPERAND.word, CPU.XR);
======
STY:
  cpu_mem_write_byte (OPERAND.word, CPU.YR);
======
AND:
  CPU.ACC &= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
======
ORA:
  CPU.ACC |= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
======
EOR:
  CPU.ACC ^= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
======
ADC:
  cpu_flag_is_set ('D') && cpu_helper_adc_decimal (OPERAND.byte) 
  	|| cpu_helper_adc_binary (OPERAND.byte);
======
SBC:
  cpu_flag_is_set ('D') && cpu_helper_sbc_decimal (OPERAND.byte) 
  	|| cpu_helper_sbc_binary (OPERAND.byte);
======
ASL:
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  uint8_t written = cpu_helper_rmw (cpu_rmwutil_asl);
  cpu_flag_set_zn (written);
======
LSR:
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_LSR) != 0);
  uint8_t written = cpu_helper_rmw (cpu_rwmutil_lsr);
  cpu_flag_set_zn (written);
======
ROL:
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  uint8_t written = cpu_helper_rmw (cpu_rmwutil_rol);
  cpu_flag_toggle_zn (written);
======
ROR:
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_LSR) != 0);
  uint8_t written = cpu_helper_rmw (cpu_rmwutil_ror);
  cpu_flag_toggle_zn (written);
======
INC:
  uint8_t increased = (cpu_mem_read_byte (OPERAND.word) + 1) & MASK_BYTE;
  cpu_mem_write_byte (OPERAND.word, increased);
  cpu_flag_toggle_zn (increased);
======
DEC:
  uint8_t decreased = (cpu_mem_read_byte (OPERAND.word) - 1) & MASK_BYTE;
  cpu_mem_write_byte (OPERAND.word, decreased);
  cpu_flag_toggle_zn (decreased);
======
BIT:
  cpu_flag_set_if ('Z', ((CPU.ACC & OPERAND.byte) & MASK_BYTE) == 0);
  cpu_flag_set_if ('N', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  cpu_flag_set_if ('V', (OPERAND.byte & MASK_BIT) != 0);
======
TAX:
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
======
TAX:
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
======
TAX:
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
======
TAX:
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
======
TXA:
  CPU.ACC = CPU.XR;
  cpu_flag_toggle_zn (C.ACC);
======
TAY:
  CPU.YR = CPU.ACC;
  cpu_flag_toggle_zn (C.YR);
======
TYA:
  CPU.ACC = CPU.YR;
  cpu_flag_toggle_zn (C.ACC);
======
TSX:
  CPU.XR = CPU.SP;
  cpu_flag_toggle_zn (C.XR);
======
TXS:
  CPU.SP = CPU.XR;
======
INX:
  CPU.XR++;
  cpu_flag_toggle_zn (C.XR);
======
INX:
  CPU.XR++;
  cpu_flag_toggle_zn (C.XR);
======
INY:
  CPU.YR++;
  cpu_flag_toggle_zn (C.YR);
======
DEX:
  CPU.XR--;
  cpu_flag_toggle_zn (C.XR);
======
DEY:
  CPU.YR--;
  cpu_flag_toggle_zn (C.YR);
======
CMP:
  cpu_helper_cmp (CPU.ACC, OPERAND.byte);
======
CPX:
  cpu_helper_cmp (CPU.XR, OPERAND.byte);
======
CPY:
  cpu_helper_cmp (CPU.YR, OPERAND.byte);
======
CLC:
  cpu_flag_unset ('C');
======
SEC:
  cpu_flag_set ('C');
======
CLD:
  cpu_flag_unset ('D');
======
SED:
  cpu_flag_set ('D');
======
CLI:
  cpu_flag_unset ('I');
======
SEI:
  cpu_flag_set ('I');
======
CLV:
  cpu_flag_unset ('V');
======
NOP:
  return;
======
PHA:
  cpu_status_save_acc ();
======
PLA:
  cpu_status_restore_acc ();
======
PHP:
  cpu_status_save_modified_flags ();
======
PLP:
   cpu_status_restore_flags ();
======
JSR:
  uint8_t ret = (CPU.PC - 1) & MASK_SHORT;
  cpu_stack_push_short (ret);
  CPU.PC = OPERAND.word;
======
RTS:
   cpu_status_restore_pc ();
======
RTI:
  cpu_status_restore_flags ();
  cpu_status_restore_pc ();
======
BRK:
  CPU.PC += 2;
  cpu_status_save_pc ();
  cpu_status_save_modified_flags ();
  cpu_flag_set ('I');
  CPU.PC = cpu_mem_write_short (VECADDR_IRQ);
======
JMP:
  CPU.PC = OPERAND.word & MASK_SHORT;

