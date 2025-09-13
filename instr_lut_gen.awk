#!/usr/bin/env awk -f

BEGIN { FS = "\t" }

BEGIN {
     flags[0] = "N"
     flags[1] = "Z"
     flags[2] = "C"
     flags[3] = "I"
     flags[4] = "D"
     flags[5] = "V"
}

END { emit_end_case ()  }

{
    mnemonic = $1
    flageffs = $2
    addrmode = $3
    opcode = $4
    size = $5
    cycles = $6

    special[branch_cross] = cycles ~ /[0-9]\*\*/ ? 1 : 0
    special[page_cross] = cycles ~ /[0-9]\*/ ? 1 : 0

    gsub(/\*+/, "", cycles)
    
    n_split = split(flageffs, flageffs_split, ",")
    for (i = 0; i < n_split; i++) {
	if (flageffs_split[i] ~ flags[i] "=" "+") {
	    flags_stat[flags[i]] = "FLAGSTAT_MODIFIED"
	} else if (flageffs_split[i] ~ flags[i] "=" "-") {
	    flags_stat[flags[i]] = "FLAGSTAT_UNMODIFIED"
	} else if (flageffs_split[i] ~ flags[i] "=" "0") {
	    flags_stat[flags[i]] = "FLAGSTAT_CLEARED"
	} else if (flageffs_split[i] ~ flags[i] "=" "1") {
	    flags_stat[flags[i]] = "FLAGSTAT_SET"
	} else if (flageffs_split[i] ~ flags[i] "=" "M6") {
	    flags_stat[flags[i]] = "FLAGSTAT_M6"
        } else if (flageffs_split[i] ~ flags[i] "=" "M7") {
	    flags_stat[flags[i]] = "FLAGSTAT_M7"
	} else { 
	    # UNREACHABLE
	}
    }


    emit_case(opcode, mnemonic, addrmode, size, cycles, spcial, flags_stat)
}




