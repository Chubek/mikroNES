#!/usr/bin/env awk -f

BEGIN { FS = "\t" }

END { emit_end_case ()  }

{
    mnemonic = $1
    flageffs = $2
    addrmode = $3
    opcode = $4
    size = $5
    cycles = $6

    branch_cross = cycles ~ /[0-9]\*\*/ ? 1 : 0
    page_cross = cycles ~ /[0-9]\*/ ? 1 : 0

    split(flageffs, flageffs_split, ",")

    neff = assess_flageff(flageffs_split[0])
    zeff = assess_flageff(flageffs_split[1])
    ceff = assess_flageff(flageffs_split[2])
    ieff = assess_flageff(flageffs_split[3])
    deff = assess_flageff(flageffs_split[4])
    veff = assess_flageff(flageffs_split[5])

    emit_case(opcode, mnemonic, addrmode, size, cycles, neff, zeff, ceff, ieff, deff, veff)
}




