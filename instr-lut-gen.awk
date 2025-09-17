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
	    if (flagmod_split[i] == flags[i] "=M7")
		    flagstat[flags[i]] = "FLAGMODSTAT_M7"
	    else if (flagmod_split[i] == flags[i] "=M6")
		    flagstat[flags[i]] = "FLAGMODSTAT_M6"
	    else if (flagmod_split[i] == flags[i] "=0")
		    flagstat[flags[i]] = "FLAGMODSTAT_CLEARED"
	    else if (flagmod_split[i] == flags[i] "=1")
		    flagstat[flags[i]] = "FLAGMODSTAT_SET"
	    else if (flagmod_split[i] == flags[i] "=-")
		    flagstat[flags[i]] = "FLAGMODSTAT_CLEARED"
	    else if (flagmod_split[i] == flags[i] "=+")
		    flagstat[flags[i]] = "FLAGMODSTAT_MODIFIED"
	    else {
		    # UNREACHABLE
            }

    }

    emit_case(opcode, mnemonic, addrmode, size, cycles, special, flagstat)
}

function emit_case(opcode, mnemonic, addrmode, size, cycles, special, flagstat) {
    printf "\tcase %s:\n", opcode
    printf "\t\tINSTR.mnemonic = \"%s\";\n", mnemonic
    printf "\t\tINSTR.size_bytes = %s;\n", size
    printf "\t\tINSTR.resolver = %s;\n", "cpu_addrmode_" map_addr_mode(addrmode)
    printf "\t\tINSTR.num_cycles = %s;\n", cycles
    printf "\t\tINSTR.micro_op = %s;\n", "cpu_op_" tolower(mnemonic)
    printf "\t\tINSTR.special_case = %s;\n", special

    for (flag in flagstat) {
	printf "\t\tINSTR.action_%s = %s;\n", flag, flagstat[flag]
    }

    printf "\t\treturn;\n"
}

function map_addr_mode(mode) {
    if (mode == "immediate")
	return "imm"
    else if (mode == "zeropage")
	return "zpg"
     else if (mode == "zeropageX")
	return "zpgx"
     else if (mode == "zeropageY")
	return "zpgy"
     else if (mode == "absolute")
	return "abs"
     else if (mode == "absoluteX")
	return "absx"
     else if (mode == "absoluteY")
	return "absy"
     else if (mode == "indirect")
	return "ind"
     else if (mode == "indirectX")
	return "xind"
     else if (mode == "indirectY")
	return "yind"
     else if (mode == "implied")
	return "impl"
     else if (mode == "relative")
	return "rel"
     else if (mode == "accumulator")
	return "acc"
    else {
	# UNREACHABLE
    }
}


