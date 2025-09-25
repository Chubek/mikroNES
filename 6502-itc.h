static void
cpu_itc_lda (void)
{
  CPU.ACC = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}

static void
cpu_itc_ldx (void)
{
  CPU.XR = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.XR);
}

static void
cpu_itc_ldy (void)
{
  CPU.YR = OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.YR);
}

static void
cpu_itc_sta (void)
{
  cpu_mem_write_byte (OPERAND.word, CPU.ACC);
}

static void
cpu_itc_stx (void)
{
  cpu_mem_write_byte (OPERAND.word, CPU.XR);
}

static void
cpu_itc_sty (void)
{
  cpu_mem_write_byte (OPERAND.word, CPU.YR);
}

static void
cpu_itc_and (void)
{
  CPU.ACC &= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}

static void
cpu_itc_ora (void)
{
  CPU.ACC |= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}

static void
cpu_itc_eor (void)
{
  CPU.ACC ^= OPERAND.byte & MASK_BYTE;
  cpu_flag_toggle_zn (CPU.ACC);
}

static void
cpu_itc_adc (void)
{
  cpu_flag_is_set ('D') && cpu_helper_adc_decimal (OPERAND.byte)
      || cpu_helper_adc_binary (OPERAND.byte);
}

static void
cpu_itc_sbc (void)
{
  cpu_flag_is_set ('D') && cpu_helper_sbc_decimal (OPERAND.byte)
      || cpu_helper_sbc_binary (OPERAND.byte);
}

static void
cpu_itc_asl (void)
{
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  uint8_t written = cpu_helper_rmw (cpu_rmwutil_asl);
  cpu_flag_set_zn (written);
}

static void
cpu_itc_lsr (void)
{
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_LSR) != 0);
  uint8_t written = cpu_helper_rmw (cpu_rwmutil_lsr);
  cpu_flag_set_zn (written);
}

static void
cpu_itc_rol (void)
{
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  uint8_t written = cpu_helper_rmw (cpu_rmwutil_rol);
  cpu_flag_toggle_zn (written);
}

static void
cpu_itc_ror (void)
{
  cpu_flag_set_if ('C', (OPERAND.byte & MASK_LSR) != 0);
  uint8_t written = cpu_helper_rmw (cpu_rmwutil_ror);
  cpu_flag_toggle_zn (written);
}

static void
cpu_itc_inc (void)
{
  uint8_t increased = (cpu_mem_read_byte (OPERAND.word) + 1) & MASK_BYTE;
  cpu_mem_write_byte (OPERAND.word, increased);
  cpu_flag_toggle_zn (increased);
}

static void
cpu_itc_dec (void)
{
  uint8_t decreased = (cpu_mem_read_byte (OPERAND.word) - 1) & MASK_BYTE;
  cpu_mem_write_byte (OPERAND.word, decreased);
  cpu_flag_toggle_zn (decreased);
}

static void
cpu_itc_bit (void)
{
  cpu_flag_set_if ('Z', ((CPU.ACC & OPERAND.byte) & MASK_BYTE) == 0);
  cpu_flag_set_if ('N', (OPERAND.byte & MASK_COMPLEMENT) != 0);
  cpu_flag_set_if ('V', (OPERAND.byte & MASK_BIT) != 0);
}

static void
cpu_itc_tax (void)
{
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
}

static void
cpu_itc_tax (void)
{
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
}

static void
cpu_itc_tax (void)
{
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
}

static void
cpu_itc_tax (void)
{
  CPU.XR = CPU.ACC;
  cpu_flag_toggle_zn (C.XR);
}

static void
cpu_itc_txa (void)
{
  CPU.ACC = CPU.XR;
  cpu_flag_toggle_zn (C.ACC);
}

static void
cpu_itc_tay (void)
{
  CPU.YR = CPU.ACC;
  cpu_flag_toggle_zn (C.YR);
}

static void
cpu_itc_tya (void)
{
  CPU.ACC = CPU.YR;
  cpu_flag_toggle_zn (C.ACC);
}

static void
cpu_itc_tsx (void)
{
  CPU.XR = CPU.SP;
  cpu_flag_toggle_zn (C.XR);
}

static void
cpu_itc_txs (void)
{
  CPU.SP = CPU.XR;
}

static void
cpu_itc_inx (void)
{
  CPU.XR++;
  cpu_flag_toggle_zn (C.XR);
}

static void
cpu_itc_inx (void)
{
  CPU.XR++;
  cpu_flag_toggle_zn (C.XR);
}

static void
cpu_itc_iny (void)
{
  CPU.YR++;
  cpu_flag_toggle_zn (C.YR);
}

static void
cpu_itc_dex (void)
{
  CPU.XR--;
  cpu_flag_toggle_zn (C.XR);
}

static void
cpu_itc_dey (void)
{
  CPU.YR--;
  cpu_flag_toggle_zn (C.YR);
}

static void
cpu_itc_cmp (void)
{
  cpu_helper_cmp (CPU.ACC, OPERAND.byte);
}

static void
cpu_itc_cpx (void)
{
  cpu_helper_cmp (CPU.XR, OPERAND.byte);
}

static void
cpu_itc_cpy (void)
{
  cpu_helper_cmp (CPU.YR, OPERAND.byte);
}

static void
cpu_itc_clc (void)
{
  cpu_flag_unset ('C');
}

static void
cpu_itc_sec (void)
{
  cpu_flag_set ('C');
}

static void
cpu_itc_cld (void)
{
  cpu_flag_unset ('D');
}

static void
cpu_itc_sed (void)
{
  cpu_flag_set ('D');
}

static void
cpu_itc_cli (void)
{
  cpu_flag_unset ('I');
}

static void
cpu_itc_sei (void)
{
  cpu_flag_set ('I');
}

static void
cpu_itc_clv (void)
{
  cpu_flag_unset ('V');
}

static void
cpu_itc_nop (void)
{
  return;
}

static void
cpu_itc_pha (void)
{
  cpu_status_save_acc ();
}

static void
cpu_itc_pla (void)
{
  cpu_status_restore_acc ();
}

static void
cpu_itc_php (void)
{
  cpu_status_save_modified_flags ();
}

static void
cpu_itc_plp (void)
{
  cpu_status_restore_flags ();
}

static void
cpu_itc_jsr (void)
{
  uint8_t ret = (CPU.PC - 1) & MASK_SHORT;
  cpu_stack_push_short (ret);
  CPU.PC = OPERAND.word;
}

static void
cpu_itc_rts (void)
{
  cpu_status_restore_pc ();
}

static void
cpu_itc_rti (void)
{
  cpu_status_restore_flags ();
  cpu_status_restore_pc ();
}

static void
cpu_itc_brk (void)
{
  CPU.PC += 2;
  cpu_status_save_pc ();
  cpu_status_save_modified_flags ();
  cpu_flag_set ('I');
  CPU.PC = cpu_mem_write_short (VECADDR_IRQ);
}

static void
cpu_itc_jmp (void)
{
  CPU.PC = OPERAND.word & MASK_SHORT;
}

static void
cpu_itc_bpl (void)
{
  cpu_helper_branch (cpu_flag_is_unset ('N'));
}

static void
cpu_itc_bmi (void)
{
  cpu_helper_branch (cpu_flag_is_set ('N'));
}

static void
cpu_itc_bvc (void)
{
  cpu_helper_branch (cpu_flag_is_unset ('V'));
}

static void
cpu_itc_bvs (void)
{
  cpu_helper_branch (cpu_flag_is_set ('V'));
}

static void
cpu_itc_bcc (void)
{
  cpu_helper_branch (cpu_flag_is_unset ('C'));
}

static void
cpu_itc_bcs (void)
{
  cpu_helper_branch (cpu_flag_is_set ('C'));
}

static void
cpu_itc_bne (void)
{
  cpu_helper_branch (cpu_flag_is_unset ('Z'));
}

static void
cpu_itc_beq (void)
{
  cpu_helper_branch (cpu_flag_is_set ('Z'));
}
