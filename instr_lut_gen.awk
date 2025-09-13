@include "mnemonic_slot_gen.awk"

BEGIN { FS = "\t" }

BEGIN {
     flags[1] = "N"
     flags[2] = "Z"
     flags[3] = "C"
     flags[4] = "I"
     flags[5] = "D"
     flags[6] = "V"
}

{
    mnemonic = $1
    flagmod = $2
    addrmode = $3
    opcode = $4
    size = $5
    cycles = $6

    special = "SPECIALCASE_NONE"
    if (cycles ~ /[0-9]\*\*/) {
	special = "SPECIALCASE_BRANCH_CROSS"
    } else if (cycles ~ /[0-9]\*/) {
	special = "SPECIALCASE_PAGE_CROSS"
    }

    gsub(/\*+/, "", cycles)
   
    n_split = split(flagmod, flagmod_split, ",")

    for (i = 1; i < n_split; i++) {
	    if (flagmod_split[i] ~ flags[i] "=M7")
		    flagstat[flags[i]] = "FLAGMODSTAT_M7"
	    else if (flagmod_split[i] ~ flags[i] "=M6")
		    flagstat[flags[i]] = "FLAGMODSTAT_M6"
	    else if (flagmod_split[i] ~ flags[i] "=0")
		    flagstat[flags[i]] = "FLAGMODSTAT_CLEARED"
	    else if (flagmod_split[i] ~ flags[i] "=1")
		    flagstat[flags[i]] = "FLAGMODSTAT_SET"
	    else if (flagmod_split[i] ~ flags[i] "=-")
		    flagstat[flags[i]] = "FLAGMODSTAT_CLEARED"
	    else if (flagmod_split[i] ~ flags[i] "=+")
		    flagstat[flags[i]] = "FLAGMODSTAT_MODIFIED"
	    else {
		    # UNREACHABLE
            }

    }

    emit_case(opcode, mnemonic, addrmode, size, cycles, special, flagstat)
}

function emit_case(opcode, mnemonic, addrmode, size, cycles, special, flagstat) {
    printf "\tcase %s:\n", opcode
    printf "\t\tINSTR.mnemonic_slot = %d;\n", get_mnemonic_slot(mnemonic)
    printf "\t\tINSTR.size_bytes = %s;\n", size
    printf "\t\tINSTR.address_mode = %s;\n", map_addr_mode(addrmode)
    printf "\t\tINSTR.num_cycles = %s;\n", cycles
    printf "\t\tINSTR.special_case = %s;\n", special

    for (flag in flagstat) {
	printf "\t\tINSTR.action_%s = %s;\n", flag, flagstat[flag]
    }

    printf "\t\treturn;\n"
}

function map_addr_mode(mode) {
    if (mode ~ /immediate/)
	return "ADDRMODE_IMM"
    else if (mode ~ /zeropage/)
	return "ADDRMODE_ZPG"
     else if (mode ~ /zeropage,X/)
	return "ADDRMODE_ZPGX"
     else if (mode ~ /zeropage,Y/)
	return "ADDRMODE_ZPGY"
     else if (mode ~ /absolute/)
	return "ADDRMODE_ABS"
     else if (mode ~ /absolute,X/)
	return "ADDRMODE_ABSX"
     else if (mode ~ /absolute,Y/)
	return "ADDRMODE_ABSY"
     else if (mode ~ /indirect/)
	return "ADDRMODE_IND"
     else if (mode ~ /indirect,X/)
	return "ADDRMODE_XIND"
     else if (mode ~ /indirect,Y/)
	return "ADDRMODE_INDY"
     else if (mode ~ /implied/)
	return "ADDRMODE_IMPL"
     else if (mode ~ /relative/)
	return "ADDRMODE_REL"
     else if (mode ~ /accumulator/)
	return "ADDRMODE_ACC"
    else {
	# UNREACHABLE
    }
}


